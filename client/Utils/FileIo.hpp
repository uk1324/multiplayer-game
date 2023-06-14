#pragma once

#include <shared/Json/Json.hpp>
#include <Utils/ByteBuffer.hpp>

#include <string>
#include <string_view>


std::string stringFromFile(std::string_view path);

Json::Value jsonFromFile(std::string_view path);

ByteBuffer byteBufferFromFile(std::string_view path);