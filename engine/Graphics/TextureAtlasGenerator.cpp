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

	static constexpr auto PADDING_TO_PREVENT_BLEEDING = 2;

	const auto outputWidth = std::max(static_cast<int>(sqrt(combinedArea) * 2), maxWidth);
	std::sort(textures.begin(), textures.end(), [](const Image& a, const Image& b) { return a.image.height() < b.image.height(); });
	int rowYOffset = 0;
	int maxHeightInRow = 0;
	int currentXOffset = 0;
	for (int i = 0; i < textures.size();) {
		const auto& texture = textures[i];

		const auto width = texture.image.width() + PADDING_TO_PREVENT_BLEEDING;
		const auto height = texture.image.height() + PADDING_TO_PREVENT_BLEEDING;
		if (height > maxHeightInRow) {
			maxHeightInRow = height;
		}

		if (currentXOffset + width < outputWidth) {
			const auto pos = TextureAtlasResult::Pos{ 
				.offset = Vec2T<int>(currentXOffset + 1, rowYOffset + 1),
				.size = Vec2T<int>(texture.image.width(), texture.image.height())
			};
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
	for (int i = 0; i < result.width() * result.height(); i++) {
		result.data()[i] = 0xFFFFFFFF;
	}
	for (const auto& [name, pos] : textureNameToPos) {
		const auto& texture = textures[pos.index];
		const auto& image = texture.image;
		const auto p = pos.pos;

		for (int x = 0; x < image.width(); x++) {
			result.set(x + p.offset.x, p.offset.y - 1, image.get(x, 0));
		}
		for (int x = 0; x < image.width(); x++) {
			result.set(x + p.offset.x, p.offset.y + image.height(), image.get(x, image.height() - 1));
		}

		for (int y = 0; y < image.height(); y++) {
			result.set(p.offset.x - 1, y + p.offset.y, image.get(0, y));
		}
		for (int y = 0; y < image.height(); y++) {
			result.set(p.offset.x + image.width(), y + p.offset.y, image.get(image.width() - 1, y));
		}

		result.set(p.offset.x - 1, p.offset.y - 1, image.get(0, 0));
		result.set(p.offset.x - 1, p.offset.y + image.height(), image.get(0, image.height() - 1));
		result.set(p.offset.x + image.width(), p.offset.y - 1, image.get(image.width() - 1, 0));
		result.set(p.offset.x + image.width(), p.offset.y + image.height(), image.get(image.width() - 1, image.height() - 1));


		for (int x = 0; x < texture.image.width(); x++) {
			for (int y = 0; y < texture.image.height(); y++) {
				result.set(x + p.offset.x, y + p.offset.y, image.get(x, y));
			}
		}
		nameToPos[name] = p;
	}

	return TextureAtlasResult{
		.textureNameToPos = std::move(nameToPos),
		.atlasImage = result,
	};
}
