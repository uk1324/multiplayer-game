#pragma once

#include "JsonValue.hpp"

#include <string_view>

namespace Json
{
	class ParsingError : std::exception {};
	Value parse(std::string_view text);
}