#pragma once

#include <optional>

namespace Utf8 {

struct ReadCharResult {
	char32_t codePoint;
	const char* next;
};
std::optional<ReadCharResult> readCodePoint(const char* string, const char* stringEnd);

}