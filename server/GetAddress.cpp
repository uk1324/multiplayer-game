#include "GetAddress.hpp"
#include <iostream>
#include <string.h>
#include "Sockets.hpp"

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#endif

bool getAddress(char* address, int addressBufferSize) {
    // Maximum host name length is 255 + 1 for null.
    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == -1) {
        Sockets::outputFullLastErrorMessage(std::cerr, "gethostname");
        return false;
    }

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* result;
    if (const auto status = getaddrinfo(hostName, nullptr, &hints, &result); status != 0) {
        Sockets::getaddrinfoOutputFullErrorMessage(std::cerr, status);
        return false;
    }

    const addrinfo* ipv6Address = nullptr;
    for (const addrinfo* entry = result; result != nullptr; entry = result->ai_next) {
        if (entry->ai_family == AF_INET) {
            const sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(entry->ai_addr);
            const void* addr = &(ipv4->sin_addr);
            if (inet_ntop(entry->ai_family, addr, address, addressBufferSize) == nullptr) {
                Sockets::outputFullLastErrorMessage(std::cerr, "inet_ntop");
                return false;
            }
            // Prefer IPv4 over IPv6
            return true;
        } else {
            ipv6Address = entry;
        }
    }

    if (ipv6Address != nullptr) {
        const sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(ipv6Address->ai_addr);
        const void* addr = &(ipv6->sin6_addr);
        if (inet_ntop(ipv6Address->ai_family, addr, address, addressBufferSize) == nullptr) {
            Sockets::outputFullErrorMessage(std::cerr, "inet_ntop", Sockets::lastError());
            return false;
        }
        return true;
    }

    freeaddrinfo(result);
    return false;
}