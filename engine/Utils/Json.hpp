#pragma once
#include <engine/Json/JsonValue.hpp>
#include <engine/Math/Vec2.hpp>
#include <engine/Math/Vec3.hpp>
#include <engine/Math/Vec4.hpp>

// Cannot overload on return type alone.
// Because you cannot overload on return type alone you can't do things like templating to have fromJson of a vector map or any other templated type. To fix this you could have function that takes a pointer to uninitialized memory and initialized the object using the copy constructor into it. You would need to be cerful with constrcutors it might be simplest to move it out of that memory. 
// Or maybe just have a pointer to the type parameter that just should be set with a reinterpret casted nullptr. It could even be defaulted maybe.
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


//template<typename Key, typename Value>
//Json::Value toJson(const std::unordered_map<Key, Value>& map) {
//	auto arrayObject = Json::Value::emptyArray();
//	auto& array = arrayObject.array();
//	for (const auto& [key, value] : map) {
//		array.push_back({
//			{ "key", toJson(key) },
//			{ "value", toJson(value) }
//		});
//	}
//	return arrayObject;
//}

template<typename K, typename V>
Json::Value toJson(const std::unordered_map<K, V>& map) {
	auto json = Json::Value::emptyArray();
	auto& jsonArray = json.array();
	// Maybe serialize std::pair.
	for (const auto& [key, value] : map) {
		auto object = Json::Value::emptyObject();
		object["key"] = toJson(key);
		object["value"] = toJson(value);
		jsonArray.push_back(object);
	}
	return json;
}

template<typename T>
Json::Value toJson(const std::vector<T>& vector) {
	auto json = Json::Value::emptyArray();
	auto& jsonArray = json.array();
	// Maybe serialize std::pair.
	for (const auto& item : vector) {
		jsonArray.push_back(toJson(item));
	}
	return json;
}

// There is no way to specialize templates with templated types 
//template<typename Key, typename Value>
//std::unordered_map<Key, Value> mapFromJson(const Json::Value& json) {
//	
//}


template<typename T>
std::optional<T> tryLoadFromJson(const Json::Value& json) {
	try {
		return fromJson<T>(json);
	} catch (const Json::Value::Exception&) {
		return std::nullopt;
	}
}

//template<typename Key, typename Value> 
//Json::Value mapToJson(std::unordered_map<Key, Value>& map) {
//	auto& arrayObject = json["<name>"] = Json::Value::emptyArray();
//	auto& array = arrayObject.array();
//	for (const auto& [key, value] : map) {
//		array.push_back({
//			{ "key", toJson(dataType, name) }
//			})
//	}
//}