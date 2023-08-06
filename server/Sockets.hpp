#pragma once

#include <ostream>

struct pollfd;

#ifdef WIN32
	#if defined(_WIN64)
		using SocketType = unsigned __int64;
	#else
		using SocketType = unsigned int;
	#endif
#else
	using SocketType = int;
#endif

namespace Sockets {
	// https://learn.microsoft.com/en-us/windows/win32/winsock/socket-data-type-2
	#ifdef WIN32
	static constexpr SocketType INVALID_DESCRIPTOR = (SocketType)(~0);
	#else
	static constexpr SocketType INVALID_DESCRIPTOR = -1;
	#endif

	int poll(pollfd* pollFds, int pollFdsCount, int timeout);
	int close(SocketType socket);

	int lastError();
	// Windows requires allocation, but other system don't. 
	// Not sure what is the best way to return the value without allocating.
	// Could make a function that returns the required length. Then the user could allocate enough memory.
	// Using an ostream technically also allows this if you make a custom ostream that just counts the characters.
	// Using an ostream seems the most convinient.
	void errorCodeMessage(std::ostream& os, int errorCode);

	void outputFullErrorMessage(std::ostream& os, const char* functionName, int errorCode);
	void outputFullLastErrorMessage(std::ostream& os, const char* functionName);

	// Call right after calling getaddrinfo
	void getaddrinfoErrorMessage(std::ostream& os, int returnValue);
	void getaddrinfoOutputFullErrorMessage(std::ostream& os, int returnValue);
}