#include "Utf8.hpp"

static_assert(sizeof(char) == 1);
std::optional<Utf8::ReadCharResult> Utf8::readCodePoint(const char* string, const char* stringEnd) {
	for (;;) {
		if (string >= stringEnd) {
			return std::nullopt;
		}
		char c = *string;
		int octets;
		char32_t codePoint;
		if ((c & 0b1111'1000) == 0b1111'0000) {
			codePoint = c & 0b0000'0111;
			octets = 3;
		} else if ((c & 0b1111'0000) == 0b1110'0000) {
			codePoint = c & 0b0000'1111;
			octets = 2;
		} else if ((c & 0b1110'0000) == 0b1100'0000) {
			codePoint = c & 0b0001'1111;
			octets = 1;
		} else if ((c & 0b1000'0000) == 0b0000'0000) {
			codePoint = c;
			octets = 0;
		} else {
			// Illegal start byte.
			return std::nullopt;
		}
		string++;

		for (int i = 0; i < octets; i++) {
			const char c = *string;
			if ((c &0b1100'0000) != 0b1000'0000) {
				// Illegal continuation byte.
				return std::nullopt;
			}
			codePoint <<= 6;
			codePoint |= c & 0b0011'1111;
			string++;
		}

		if ((codePoint >= 0xD800) && (codePoint <= 0xDFFF)) {
			// Illegal code point.
			return std::nullopt;
		}

		return Utf8::ReadCharResult{
			.codePoint = codePoint,
			.next = string
		};
	}
}
