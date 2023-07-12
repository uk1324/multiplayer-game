#pragma once

template<typename K, typename V>
std::optional<V&> get(std::unordered_map<K, V>& map, const K& key) {
	auto it = map.find(key);
	if (it == map.end()) {
		return std::nullopt;
	}
	return it->second;
}