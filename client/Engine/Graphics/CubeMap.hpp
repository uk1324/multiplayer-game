#pragma once

#include <array>
#include <string_view>

class CubeMap
{
public:
	CubeMap(
		std::string_view right,
		std::string_view left,
		std::string_view top,
		std::string_view bottom,
		std::string_view front,
		std::string_view back);
	CubeMap();
	~CubeMap();

	void bind();

private:
	uint32_t m_handle;
};
