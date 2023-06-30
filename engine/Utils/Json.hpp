#pragma once
#include <engine/Json/JsonValue.hpp>
#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Math/Vec4.hpp>

// Cannot overload on return type alone.
template<typename T>
T fromJson(const Json::Value& json);

template<>
Vec2 fromJson<Vec2>(const Json::Value& json);
Json::Value toJson(const Vec2& value);

template<>
Vec2T<int> fromJson<Vec2T<int>>(const Json::Value& json);
Json::Value toJson(const Vec2T<int>& value);

template<>
Vec3 fromJson<Vec3>(const Json::Value& json);
Json::Value toJson(const Vec3& value);

template<>
Vec4 fromJson<Vec4>(const Json::Value& json);
Json::Value toJson(const Vec4& value);

template<typename T>
std::optional<T> tryLoadFromJson(const Json::Value& json) {
	try {
		return fromJson<T>(json);
	} catch (const Json::Value::Exception&) {
		return std::nullopt;
	}
}
