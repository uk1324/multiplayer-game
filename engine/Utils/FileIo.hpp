#pragma once

#include <engine/Json/Json.hpp>
#include <string_view>
#include <optional>


std::string stringFromFile(std::string_view path);
std::optional<std::string> tryLoadStringFromFile(std::string_view path);

Json::Value jsonFromFile(std::string_view path);
std::optional<Json::Value> tryLoadJsonFromFile(std::string_view path);
