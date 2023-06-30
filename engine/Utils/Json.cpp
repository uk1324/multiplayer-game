#include "Json.hpp"

template<>
Vec2 fromJson<Vec2>(const Json::Value& json) {
	return Vec2{
		json.at("x").number(),
		json.at("y").number(),
	};
}

Json::Value toJson(const Vec2& value) {
	auto json = Json::Value::emptyObject();
	json["x"] = value.x;
	json["y"] = value.y;
	return json;
}

template<>
Vec2T<int> fromJson<Vec2T<int>>(const Json::Value& json) {
	return Vec2T<int>{
		json.at("x").intNumber(),
		json.at("y").intNumber(),
	};
}

Json::Value toJson(const Vec2T<int>& value) {
	auto json = Json::Value::emptyObject();
	json["x"] = value.x;
	json["y"] = value.y;
	return json;
}

template<>
Vec3 fromJson<Vec3>(const Json::Value& json) {
	return Vec3{
		json.at("x").number(),
		json.at("y").number(),
		json.at("z").number(),
	};
}

Json::Value toJson(const Vec3& value) {
	auto json = Json::Value::emptyObject();
	json["x"] = value.x;
	json["y"] = value.y;
	json["z"] = value.z;
	return json;
}

template<>
Vec4 fromJson<Vec4>(const Json::Value& json) {
	return Vec4{
		json.at("x").number(),
		json.at("y").number(),
		json.at("z").number(),
		json.at("w").number(),
	};
}

Json::Value toJson(const Vec4& value) {
	auto json = Json::Value::emptyObject();
	json["x"] = value.x;
	json["y"] = value.y;
	json["z"] = value.z;
	json["w"] = value.w;
	return json;
}
