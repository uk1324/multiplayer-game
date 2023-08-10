#include "Sockets.hpp"


#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <poll.h>
#include <string.h>
#include <netdb.h>
#endif

int Sockets::poll(pollfd* pollFds, int pollFdsCount, int timeout) {
	#ifdef WIN32
	return WSAPoll(pollFds, pollFdsCount, timeout);
	#else
	return ::poll(pollFds, pollFdsCount, timeout);
	#endif
}

int Sockets::close(SocketType socket) {
	#ifdef WIN32
	return closesocket(socket);
	#else
	return ::close(socket);
	#endif
}

int Sockets::lastError() {
    #ifdef WIN32
	return WSAGetLastError();
    #else
    return errno;
    #endif
}

void Sockets::errorCodeMessage(std::ostream& os, int errorCode) {
	#ifdef WIN32

	char* message;
	const auto messageLength = FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		nullptr,
		errorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		reinterpret_cast<LPSTR>(&message),
		0,
		nullptr
	);

	if (messageLength == 0) {
		os << "Invalid or unknown error code";
		return;
	}

	os << message;

	LocalFree(message);

	#else
	const char* message = strerror(errorCode);
	os << message;
	#endif
}

void Sockets::outputFullErrorMessage(std::ostream& os, const char* functionName, int errorCode) {
	os << functionName << " error " << "code = " << errorCode << ", message = '";
	errorCodeMessage(os, errorCode);
	os << "'\n";
}

void Sockets::outputFullLastErrorMessage(std::ostream& os, const char* functionName) {
	outputFullErrorMessage(os, functionName, lastError());
}

void Sockets::getaddrinfoErrorMessage(std::ostream& os, int returnValue) {
	#ifdef WIN32
	// On windows it is reccomended to use WSAGetLastError, because to function to the the error message is not thread safe.
	errorCodeMessage(os, WSAGetLastError());
    #else
	os << gai_strerror(returnValue);
    #endif
}

void Sockets::getaddrinfoOutputFullErrorMessage(std::ostream& os, int returnValue) {
	os << "getaddrinfo error " << "code = " << returnValue << ", message = '";
	getaddrinfoErrorMessage(os, returnValue);
	os << "'\n";
}

