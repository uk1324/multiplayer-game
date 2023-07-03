#pragma once

#include <iostream>
#include <engine/Utils/Assertions.hpp>

void putToStream(std::ostream& os, const char* format);

template<typename T, typename ...Args>
void putToStream(std::ostream& os, const char* format, const T& arg, const Args&... args) {
	int current = 0;
	while (format[current] != '\0') {
		if (format[current] == '%') {
			current++;
			// This is safe because the next one is either a valid character or the string end '\0'.
			if (format[current] == '%') {
				current++;
			} else {
				os << arg;
				putToStream(os, format + current, args...);
				return;
			}
		}
		os << format[current];
		current++;
	}
	// A call with a correct format should always finish at the overload with no Args. 
	CHECK_NOT_REACHED();
}

template<typename ...Args>
void putToStream(std::ostream& os, const char* format, const Args&... args) {
	putToStream(os, format, args...);
}

template<typename ...Args>
void put(const char* format, const Args&... args) {
	putToStream(std::cout, format, args...);
	std::cout << '\n';
}

// nn - no newline
template<typename ...Args>
void putnn(const char* format, const Args&... args) {
	putToStream(std::cout, format, args...);
}