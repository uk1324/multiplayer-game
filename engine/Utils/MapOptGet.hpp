#pragma once

template<typename K, typename V>
std::optional<V&> get(std::unordered_map<K, V>& map, const K& key) {
	auto it = map.find(key);
	if (it == map.end()) {
		return std::nullopt;
	}
	return it->second;
}

template<typename K, typename V>
std::optional<const V&> get(const std::unordered_map<K, V>& map, const K& key) {
	// Maybe const cast
	const auto it = map.find(key);
	if (it == map.cend()) {
		return std::nullopt;
	}
	return it->second;
}