#include <shared/Json/JsonValue.hpp>
#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Math/Vec4.hpp>

template<typename T>
T fromJson(const Json::Value& json);

template<>
Vec2 fromJson<Vec2>(const Json::Value& json);
Json::Value toJson(const Vec2& value);

template<>
Vec3 fromJson<Vec3>(const Json::Value& json);
Json::Value toJson(const Vec3& value);

template<>
Vec4 fromJson<Vec4>(const Json::Value& json);
Json::Value toJson(const Vec4& value);
