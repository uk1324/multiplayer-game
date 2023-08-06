#pragma once

#include <vector>
#include <engine/Utils/StringStream.hpp>
#include <engine/Utils/Mutex.hpp>
#include <server/HttpServer/RequestsData.hpp>
#include <server/Sockets.hpp>

// This server could be used for debugging queries like getting the RTTs for all clients. But could also be used for things like getting the info about a server to for example display the player count.

// Using yojimbo for messaging probably wouldn't make much sense, because it's model isn't really suited for a query, response, disconnect communication. 

// One issue with writing a http server from scratch is that it won't support https. 
// Some libraries that support https.
// https://stackoverflow.com/questions/4161257/lightweight-http-server-c
// https://github.com/yhirose/cpp-httplib
// https://github.com/cesanta/mongoose
// Issues with not using https
// https://www.troyhunt.com/heres-why-your-static-website-needs-https/

// Maybe make a custom protocol like
// https://developer.valvesoftware.com/wiki/Server_queries

struct sockaddr_storage;

struct HttpServer {
	void run();
	void onNewConnection(int clientSocket, const sockaddr_storage& clientAddress);
	void onMessage(int clientSocket, std::string_view message);

	Mutex<std::vector<RequestPlayer>> players;

	// Maybe it would be better to only have one mesage stored and make it synchronous.
	Mutex<StringStream> messages;
	// Could use put() with a macro that automatically appends MESSAGE_END.
	static constexpr char MESSAGE_END = '\0';

private:
	StringStream sendBuffer;
	StringStream responseContentBuffer;

private:
	SocketType createListeningSocket(const char* port);
	void networkingError(const char* functionName);
};

template<typename T>
Mutex<T>::Lock::Lock(T& value, std::mutex& mutex)
	: value(value)
	, lock(mutex) {}

template<typename T>
T* Mutex<T>::Lock::operator->() {
	return &value;
}

template<typename T>
T& Mutex<T>::Lock::operator*() {
	return value;
}

template<typename T>
const T& Mutex<T>::Lock::operator*() const {
	return value;
}

template<typename T>
Mutex<T>::Lock Mutex<T>::lock() {
	return Lock(value, mutex);
}
