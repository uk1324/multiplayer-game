#pragma once

#include <engine/Graphics/ShaderProgram.hpp>
#include <engine/Graphics/Texture.hpp>
#include <client/Rendering/FontData.hpp>
#include <string_view>
#include <expected>

static constexpr std::pair<char32_t, char32_t> ASCII_CHARACTER_RANGES[]{
	{ 0, 127 },
};

// https://character-table.netlify.app/polish/#unicode-ranges
static constexpr std::pair<char32_t, char32_t> POLISH_CHARACTER_RANGES[]{
	{ 0, 127 },
	{ 0x20, 0x5F },
	{ 0x61, 0x70 },
	{ 0x72, 0x75 },
	{ 0x77, 0x77 },
	{ 0x79, 0x7E },
	{ 0xA0, 0xA0 },
	{ 0xA7, 0xA7 },
	{ 0xA9, 0xA9 },
	{ 0xAB, 0xAB },
	{ 0xB0, 0xB0 },
	{ 0xBB, 0xBB },
	{ 0xD3, 0xD3 },
	{ 0xF3, 0xF3 },
	{ 0x104, 0x107 },
	{ 0x118, 0x119 },
	{ 0x141, 0x144 },
	{ 0x15A, 0x15B },
	{ 0x179, 0x17C },
	{ 0x2010, 0x2011 },
	{ 0x2013, 0x2014 },
	{ 0x201D, 0x201E },
	{ 0x2020, 0x2021 },
	{ 0x2026, 0x2026 },
	{ 0x2030, 0x2030 },
	{ 0x2032, 0x2033 },
	{ 0x20AC, 0x20AC },
};

struct Font {
	struct LoadError {
		std::string message;
	};

	int pixelHeight;
	Texture fontAtlas;
	Vec2T<int> fontAtlasPixelSize;
	std::unordered_map<char32_t, Glyph> glyphs;
};

// The font info is cached so when it changes it knows it needs to reload the sdf. It might be simpler to just store the ranges and read the info from the font file. Haven't though about it much it might not work idk.
std::expected<Font, Font::LoadError> fontLoadSdfWithCaching(
	const char* fontPath, 
	const char* cachedSdfPath, 
	const char* cachedFontInfoPath,
	std::span<const std::pair<char32_t, char32_t>> rangesToLoad,
	int fontPixelHeight);
