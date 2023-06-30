#pragma once

#include <engine/Graphics/ShaderProgram.hpp>
#include <engine/Graphics/Texture.hpp>
#include <client/Rendering/FontData.hpp>
#include <string_view>
#include <expected>

struct Font {
	struct LoadError {
		std::string message;
	};

	int pixelHeight;
	Texture fontAtlas;
	Vec2T<int> fontAtlasPixelSize;
	std::unordered_map<char32_t, Glyph> glyphs;
};

std::expected<Font, Font::LoadError> fontLoadSdfWithCaching(
	const char* fontPath, 
	const char* cachedSdfPath, 
	const char* cachedFontInfoPath,
	std::span<std::pair<char32_t, char32_t>> rangesToLoad,
	int fontPixelHeight);
