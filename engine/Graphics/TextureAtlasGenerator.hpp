#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <engine/Math/Vec2.hpp>
#include <Image32.hpp>

// https://github.com/nothings/stb/blob/master/stb_rect_pack.h
struct TextureAtlasResult {
	struct Pos {
		Vec2T<int> offset;
		Vec2T<int> size;
	};
	std::unordered_map<std::string, Pos> nameToPos;
	Image32 atlasImage;
};

struct TextureAtlasInputImage {
	std::string name;
	Image32 image;
};
//TextureAtlasResult generateTextureAtlas(const std::vector<std::string>& filePaths);
// Maybe a better api would be if the input was just image sizes and names (for loading images could use stbi_info) and another argument would be a function that would called only when the final atlas in assembled.
// Or maybe have a class that allows the user the add the images and do the assembly stage themselved by giving them an iterator with the name and position. 
// Could also use ints for ids (may be more difficult to debug though that shouldn't matter much). 
TextureAtlasResult generateTextureAtlas(std::vector<TextureAtlasInputImage>& textures);