#include <server/HttpServer/HttpServer.hpp>
#include <engine/Utils/Types.hpp>
#include <engine/Utils/Json.hpp>
#include <engine/Json/JsonPrinter.hpp>
#include "Sockets.hpp"
#include <optional>

#ifdef WIN32
#include <WinSock2.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <poll.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

#include <stdio.h>

const void* getInAddr(const sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(reinterpret_cast<const sockaddr_in*>(sa)->sin_addr);
    }
    return &(reinterpret_cast<const sockaddr_in6*>(sa)->sin6_addr);
}

void HttpServer::run() {
    const char* port = "9034";

    const auto serverSocket = createListeningSocket(port);

    if (serverSocket == -1) {
        auto lock = messages.lock();
        *lock << "failed to create http server\n" << MESSAGE_END;
        return;
    }

    {
        auto lock = messages.lock();
        *lock << "server started on port " << port << '\n' << MESSAGE_END;
    }

    std::vector<pollfd> pfds;

    pfds.push_back(pollfd{
        .fd = serverSocket,
        .events = POLLIN
    });

    for (;;) {
        const auto returnedEventCount = Sockets::poll(pfds.data(), pfds.size(), -1);

        if (returnedEventCount == -1) {
            networkingError("poll");
            continue;
        }

        for (int i = 0; i < pfds.size(); i++) {
            // Pointers can get invalidated.
            if (const auto returnedEvent = pfds[i].revents & POLLIN) {
                if (const auto newConnection = pfds[i].fd == serverSocket) {
                    sockaddr_storage clientAddress;
                    socklen_t clientAddressSize = sizeof(clientAddress);
                    const auto clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddress), &clientAddressSize);

                    if (clientSocket == Sockets::INVALID_DESCRIPTOR) {
                        networkingError("accept");
                        continue;
                    }

                    pfds.push_back(pollfd{
                        .fd = clientSocket,
                        .events = POLLIN
                    });

                    onNewConnection(pfds[i].fd, clientAddress);
                } else {
                    char recvBuffer[4096];
                    const auto bytesReceived = recv(pfds[i].fd, recvBuffer, sizeof(recvBuffer), 0);

                    if (bytesReceived <= 0) {
                        if (bytesReceived == 0) {
                            auto lock = messages.lock();
                            *lock << "socket " << pfds[i].fd << " disconnected\n" << MESSAGE_END;
                        } else {
                            networkingError("recv");
                        }

                        Sockets::close(pfds[i].fd);

                        // TODO: Pop and swap function that calls move and destructs the other object.
                        pfds[i] = pfds.back();
                        pfds.pop_back();
                        continue;
                    } 

                    int messageLength = bytesReceived;
                    if (bytesReceived > sizeof(recvBuffer)) {
                        auto lock = messages.lock();
                        *lock << "message too long socket = " << pfds[i].fd << MESSAGE_END;
                        messageLength = sizeof(recvBuffer);
                    }

                    onMessage(pfds[i].fd, std::string_view(recvBuffer, messageLength));
                }
            }
        }
    }

}

void HttpServer::onNewConnection(int clientSocket, const sockaddr_storage& clientAddress) {
    char remoteAddressString[INET6_ADDRSTRLEN];
    const auto status = inet_ntop(
        clientAddress.ss_family, 
        getInAddr(reinterpret_cast<const sockaddr*>(&clientAddress)), 
        remoteAddressString, 
        sizeof(remoteAddressString));
    if (status == nullptr) {
        networkingError("inet_ntop");
        return;
    }

    auto lock = messages.lock();
    *lock << "new connection socket = " << clientSocket << ", address = " << remoteAddressString << '\n' << MESSAGE_END;
}

void HttpServer::onMessage(int clientSocket, std::string_view message) {
    enum class HttpMethod {
        GET,
    };

    auto httpMethodString = [](HttpMethod method) -> const char* {
        using enum HttpMethod;
        switch (method) {
        case GET: return "GET";
        }
        return "";
    };

    struct HttpRequest {
        HttpMethod method;
        std::string_view url;
    };

    auto parseHttpRequest = [](std::string_view text) -> std::optional<HttpRequest> {
        const auto methodEnd = text.find_first_of(" ");
        std::string_view methodString = text.substr(0, methodEnd);
        text = text.substr(methodEnd + 1);
        if (methodEnd == std::string_view::npos) {
            return std::nullopt;
        }

        HttpMethod method;
        if (methodString == "GET") {
            method = HttpMethod::GET;
        } else {
            return std::nullopt;
        }

        const auto urlEnd = text.find_first_of(" ");
        if (urlEnd == std::string_view::npos) {
            return std::nullopt;
        }
        std::string_view url = text.substr(0, urlEnd);

        return HttpRequest{
            .method = method,
            .url = url
        };
    };

    enum class HttpStatusCode {
        OK = 200,
        BAD_REQUEST = 400,
        NOT_FOUND = 404,
    };

    auto httpStatusCodeMessage = [](HttpStatusCode code) -> const char* {
        using enum HttpStatusCode;
        switch (code) {
        case OK: return "OK";
        case BAD_REQUEST: return "Bad Request";
        case NOT_FOUND: return "Not Found";
        }
        return "";
    };

    auto sendMessage = [&clientSocket, &sendBuffer = sendBuffer, &httpStatusCodeMessage](
        HttpStatusCode code, 
        const char* contentType,
        std::string_view message) {
        sendBuffer.string().clear();
        sendBuffer << "HTTP/1.1 " << static_cast<int>(code) << " " << httpStatusCodeMessage(code) << "\r\n"
            << "Content-Type: " << contentType << "\r\n"
            << "Content-Length: " << message.size() << "\r\n"
            /*<< "Cache-Control: private, max-age=0, s-maxage=0\r\n"*/
            /*<< "Cache-Control: no-cache, no-store, must-revalidate\r\n"
            << "Pragma: no-cache\r\n"
            << "Expires: 0\r\n"*/
            << "\r\n"
            << message;

        send(clientSocket, sendBuffer.string().c_str(), sendBuffer.string().length(), 0);
    };

    const auto textContentType = "text/html";
    const auto jsonContentType = "application/json";

    const auto request = parseHttpRequest(message);
    if (!request.has_value()) {
        sendMessage(HttpStatusCode::BAD_REQUEST, textContentType, "");
        return;
    }

    {
        auto lock = messages.lock();
        *lock << "request method = " << httpMethodString(request->method) << " url = " << request->url << '\n' << MESSAGE_END;
    }

    if (request->method == HttpMethod::GET && request->url == "/players") {
        const auto json = [&] {
            const auto lock = players.lock();
            // To make this not allocate could generate code to just output the json without allocating the json object.
            return toJson(*lock);
        }();

        responseContentBuffer.string().clear();
        Json::prettyPrint(responseContentBuffer, json);

        sendMessage(HttpStatusCode::OK, jsonContentType, responseContentBuffer.string());
        return;
    }

    sendMessage(HttpStatusCode::NOT_FOUND, textContentType, "not found");
}

SocketType HttpServer::createListeningSocket(const char* port) {
    const addrinfo hints {
        .ai_flags = AI_PASSIVE,
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
    };

    addrinfo* info;
    if (const auto status = getaddrinfo(nullptr, port, &hints, &info); status != 0) {
        auto lock = messages.lock();
        Sockets::getaddrinfoOutputFullErrorMessage(*lock, status);
        *lock << MESSAGE_END;
        return -1;
    }

    SocketType listenerSocket;
    addrinfo* current = info;
    for (; current != nullptr; current = current->ai_next) {
        listenerSocket = socket(current->ai_family, current->ai_socktype, current->ai_protocol);
        if (listenerSocket == Sockets::INVALID_DESCRIPTOR) {
            continue;
        }

        // Disable "address already in use" error message
        int yes = 1;
        setsockopt(listenerSocket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

        if (bind(listenerSocket, current->ai_addr, current->ai_addrlen) == -1) {
            Sockets::close(listenerSocket);
            continue;
        }

        break;
    }

    freeaddrinfo(info);

    if (current == nullptr) {
        return -1;
    }

    const auto backlog = 10;
    if (listen(listenerSocket, backlog) == -1) {
        networkingError("listen");
        return -1;
    }

    return listenerSocket;   
}

void HttpServer::networkingError(const char* functionName) {
    auto lock = messages.lock();
    auto& os = *lock;
    Sockets::outputFullErrorMessage(os, functionName, Sockets::lastError());
    os << MESSAGE_END;
}