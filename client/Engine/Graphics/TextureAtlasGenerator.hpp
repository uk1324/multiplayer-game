#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <shared/Math/Vec2.hpp>
#include <shared/Utils/Image32.hpp>

//https://github.com/nothings/stb/blob/master/stb_rect_pack.h
struct TextureAtlasResult {
	struct Pos {
		Vec2T<int> offset;
		Vec2T<int> size;
	};
	std::unordered_map<std::string, Pos> textureNameToPos;
	Image32 atlasImage;
};
TextureAtlasResult generateTextureAtlas(const std::vector<std::string>& filePaths);