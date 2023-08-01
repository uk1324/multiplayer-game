#pragma once

#include <engine/Log/Log.hpp>
#include <expected>
#include <sstream>

template<typename T, typename E>
T unwrap(std::expected<T, E>&& v) {
	if (!v.has_value()) {
		std::stringstream s;
		s << v.error();
		LOG_FATAL("%s", s.str().c_str());
	}

	auto e = std::move(*v);
	return e;
}