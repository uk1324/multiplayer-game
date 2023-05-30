#include <Engine/Graphics/TextureAtlasGenerator.hpp>

TextureAtlasResult generateTextureAtlas(const std::vector<std::string>& filePaths) {
	struct Image {
		std::string name;
		Image32 image;
	};

	std::vector<Image> textures;
	for (const auto& path : filePaths) {
		textures.push_back(Image{ .name = path, .image = Image32(path.data()) });
	}

	int combinedArea = 0;
	int maxWidth = -1;
	for (const auto& texture : textures) {
		combinedArea += texture.image.width() * texture.image.height();
		const auto width = texture.image.width();
		if (width > maxWidth) {
			maxWidth = width;
		}
	}

	struct Pos {
		int index;
		TextureAtlasResult::Pos pos;
	};

	std::unordered_map<std::string, Pos> textureNameToPos;

	const auto outputWidth = std::max(static_cast<int>(sqrt(combinedArea) * 2), maxWidth);
	std::sort(textures.begin(), textures.end(), [](const Image& a, const Image& b) { return a.image.height() < b.image.height(); });
	int rowYOffset = 0;
	int maxHeightInRow = 0;
	int currentXOffset = 0;
	for (int i = 0; i < textures.size();) {
		const auto& texture = textures[i];

		const auto width = texture.image.width();
		const auto height = texture.image.height();
		if (height > maxHeightInRow) {
			maxHeightInRow = height;
		}

		if (currentXOffset + width < outputWidth) {
			const auto pos = TextureAtlasResult::Pos{ .offset = Vec2T<int>(currentXOffset, rowYOffset), .size = Vec2T<int>(width, height) };
			textureNameToPos[texture.name] = Pos{ .index = i, .pos = pos };
			currentXOffset += width;
			i++;
		} else {
			rowYOffset += maxHeightInRow;
			currentXOffset = 0;
		}
	}

	std::unordered_map<std::string, TextureAtlasResult::Pos> nameToPos;
	Image32 result(outputWidth, currentXOffset + maxHeightInRow);
	for (const auto& [name, pos] : textureNameToPos) {
		const auto& texture = textures[pos.index];
		const auto p = pos.pos;
		for (int x = 0; x < p.size.x; x++) {
			for (int y = 0; y < p.size.y; y++) {
				result.set(x + p.offset.x, y + p.offset.y, texture.image.get(x, y));
			}
		}
		nameToPos[name] = p;
	}

	return TextureAtlasResult{
		.textureNameToPos = std::move(nameToPos),
		.atlasImage = result,
	};
}
