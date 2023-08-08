#include <engine/Graphics/TextureAtlasGenerator.hpp>

TextureAtlasResult generateTextureAtlas(std::vector<TextureAtlasInputImage>& textures) {
	struct Image {
		std::string name;
		Image32 image;
	};

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
	std::sort(
		textures.begin(), 
		textures.end(), 
		[](const TextureAtlasInputImage& a, const TextureAtlasInputImage& b) {
			return a.image.height() < b.image.height(); 
		});
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
			textureNameToPos.insert({ texture.name, Pos{.index = i, .pos = pos } });
			currentXOffset += width;
			i++;
		} else {
			rowYOffset += maxHeightInRow;
			currentXOffset = 0;
		}
	}

	std::unordered_map<std::string, TextureAtlasResult::Pos> nameToPos;
	Image32 result(outputWidth, rowYOffset + maxHeightInRow);
	for (int i = 0; i < result.width() * result.height(); i++) {
		result.data()[i] = Pixel32(0xFF, 0xFF, 0xFF, 0xFF);
	}
	for (const auto& [name, pos] : textureNameToPos) {
		const auto& texture = textures[pos.index];
		const auto& image = texture.image;
		const auto p = pos.pos;

		for (int x = 0; x < image.width(); x++) {
			result(x + p.offset.x, p.offset.y - 1) = image(x, 0);
		}
		for (int x = 0; x < image.width(); x++) {
			result(x + p.offset.x, p.offset.y + image.height()) = image(x, image.height() - 1);
		}

		for (int y = 0; y < image.height(); y++) {
			result(p.offset.x - 1, y + p.offset.y) = image(0, y);
		}
		for (int y = 0; y < image.height(); y++) {
			result(p.offset.x + image.width(), y + p.offset.y) = image(image.width() - 1, y);
		}

		result(p.offset.x - 1, p.offset.y - 1) = image(0, 0);
		result(p.offset.x - 1, p.offset.y + image.height()) = image(0, image.height() - 1);
		result(p.offset.x + image.width(), p.offset.y - 1) = image(image.width() - 1, 0);
		result(p.offset.x + image.width(), p.offset.y + image.height()) = image(image.width() - 1, image.height() - 1);

		for (int x = 0; x < texture.image.width(); x++) {
			for (int y = 0; y < texture.image.height(); y++) {
				result(x + p.offset.x, y + p.offset.y) = image(x, y);
			}
		}
		nameToPos[name] = p;
	}

	return TextureAtlasResult{
		.nameToPos = std::move(nameToPos),
		.atlasImage = result,
	};
}
