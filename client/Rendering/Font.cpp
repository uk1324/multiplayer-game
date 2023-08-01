#include <client/Rendering/Font.hpp>
#include <engine/Graphics/TextureAtlasGenerator.hpp>
#include <engine/Json/JsonPrinter.hpp>
#include <engine/Utils/FileIo.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H  
#include <filesystem>
#include <fstream>

Json::Value toJson(char32_t v) {
	return Json::Value(std::to_string(v));
}

template<>
char32_t fromJson<char32_t>(const Json::Value& json) {
	return std::stoi(json.string());
}

template<typename K, typename V>
static Json::Value mapToJson(const std::unordered_map<K, V>& map) {
	auto json = Json::Value::emptyArray();
	auto& jsonArray = json.array();
	// Maybe serialize std::pair.
	for (const auto& [key, value] : map) {
		auto object = Json::Value::emptyObject();
		object["key"] = toJson(key);
		object["value"] = toJson(value);
		jsonArray.push_back(object);
	}
	return json;
}

template<typename K, typename V>
static std::unordered_map<K, V> fromJson(const Json::Value& value) {
	std::unordered_map<K, V> result;
	for (const auto& pair : value.array()) {
		result[fromJson<K>(pair.at("key"))] = fromJson<V>(pair.at("value"));
	}
	return result;
}

#include <iostream>

std::expected<Font, Font::LoadError> fontLoadSdfWithCaching(
	const char* fontPath,
	const char* cachedSdfPath,
	const char* cachedFontInfoPath,
	std::span<const std::pair<char32_t, char32_t>> rangesToLoad,
	int fontPixelHeight) {
	const Texture::Settings atlasTextureSettings{
		.format = Texture::Format::RGBA,
		.wrapS = Texture::Wrap::CLAMP_TO_EDGE,
		.wrapT = Texture::Wrap::CLAMP_TO_EDGE,
	};

	auto tryLoadCached = [&]() -> std::optional<Font> {
		const auto cachedSdfImage = Image32::fromFile(cachedSdfPath);

		if (!cachedSdfImage.has_value()) {
			return std::nullopt;
		}

		const auto fontInfo = tryLoadJsonFromFile(cachedFontInfoPath);
		if (!fontInfo.has_value()) {
			return std::nullopt;
		}

		try {
			const auto cachedFontHeight = fontInfo->at("pixelHeight").intNumber();
			if (cachedFontHeight != fontPixelHeight) {
				return std::nullopt;
			}
			auto glyphs = fromJson<char32_t, Glyph>(fontInfo->at("glyphs"));

			for (const auto& range : rangesToLoad) {
				for (char32_t character = range.first; character <= range.second; character++) {
					if (glyphs.find(character) == glyphs.end()) {
						return std::nullopt;
					}
				}
			}

			return Font{
				.pixelHeight = cachedFontHeight,
				.fontAtlas = Texture(*cachedSdfImage, atlasTextureSettings),
				.fontAtlasPixelSize = Vec2T<int>(cachedSdfImage->size()),
				.glyphs = std::move(glyphs)
			};
		} catch (const Json::Value::Exception&) {
			return std::nullopt;
		}
	};

	if (auto result = tryLoadCached()) {
		return std::move(*result);
	}

	FT_Library ft;
	if (FT_Init_FreeType(&ft)) {
		return std::unexpected(Font::LoadError{ "could not init FreeType Library" });
	}

	FT_Face face;
	if (FT_New_Face(ft, fontPath, 0, &face)) {
		return std::unexpected(Font::LoadError{ "failed to load font" });
	}

	FT_Set_Pixel_Sizes(face, 0, fontPixelHeight);

	std::unordered_map<char32_t, Glyph> glyphs;
	std::vector<TextureAtlasInputImage> textureAtlasInput;
	for (const auto& range : rangesToLoad) {
		// Ignore invalid ranges first > second
		for (char32_t character = range.first; character <= range.second; character++) {
			const auto glyphIndex = FT_Get_Char_Index(face, character);

			if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_DEFAULT)) {
				return std::unexpected(Font::LoadError{ 
					"failed to load glyph with index " + std::to_string(glyphIndex) + " = U+" + std::to_string(character)
				});
			}

			// if (const auto error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_SDF)) {
			if (const auto error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
				std::cout << error << '\n';
				return std::unexpected(Font::LoadError{ "failed to render glyph with index " + std::to_string(glyphIndex) });
			}

			const FT_Bitmap& bitmap = face->glyph->bitmap;

			
			glyphs[character] = Glyph{
				.offsetInAtlas = Vec2T<int>(0), // Set after the atlas is constructed
				.sizeInAtlas = { static_cast<int>(bitmap.width), static_cast<int>(bitmap.rows) },
				.visibleSize = { static_cast<int>(face->glyph->metrics.width) >> 6, static_cast<int>(face->glyph->metrics.height) >> 6  },
				.bearingRelativeToOffsetInAtlas = { face->glyph->bitmap_left, face->glyph->bitmap_top },
				.visibleBearing = { static_cast<int>(face->glyph->metrics.horiBearingX) >> 6, static_cast<int>(face->glyph->metrics.horiBearingY) >> 6  },
				.advance = { face->glyph->advance.x, face->glyph->advance.y },
			};

			if (bitmap.rows == 0 || bitmap.width == 0) {
				continue;
			}

			Image32 image(bitmap.width, bitmap.rows);

			for (unsigned int y = 0; y < bitmap.rows; y++) {
				for (unsigned int x = 0; x < bitmap.width; x++) {
					const auto value = bitmap.buffer[y * bitmap.pitch + x];
					image(x, y) = Pixel32{ value };
				}
			}

			const auto charBytes = reinterpret_cast<char*>(&character);
			// TODO: Maybe check for duplicates. Check if intersection between ranges maybe.
			textureAtlasInput.push_back(TextureAtlasInputImage{ 
				.name = std::string(charBytes, charBytes + sizeof(char32_t)), 
				.image = std::move(image) 
			});
		}
	}

	auto output = generateTextureAtlas(textureAtlasInput);

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	Vec2T<int> fontAtlasPixelSize(output.atlasImage.size());

	for (auto& [character, info] : glyphs) {
		const auto charBytes = reinterpret_cast<const char*>(&character);
		const auto atlasInfo = output.nameToPos.find(std::string(charBytes, charBytes + sizeof(char32_t)));
		if (atlasInfo == output.nameToPos.end()) {
			continue;
		}

		info.offsetInAtlas = atlasInfo->second.offset;
	}

	auto info = Json::Value::emptyObject();
	info["glyphs"] = mapToJson(glyphs);
	info["pixelHeight"] = fontPixelHeight;

	auto createDirectories = [](const char* filePath) {
		using namespace std::filesystem;
		create_directories(path(filePath).parent_path());
	};

	createDirectories(cachedFontInfoPath);
	std::ofstream jsonFile(cachedFontInfoPath);
	Json::print(jsonFile, info);

	createDirectories(cachedSdfPath);
	output.atlasImage.saveToPng(cachedSdfPath);

	return Font{
		.pixelHeight = fontPixelHeight,
		.fontAtlas = Texture(output.atlasImage, atlasTextureSettings),
		.fontAtlasPixelSize = Vec2T<int>(output.atlasImage.size()),
		.glyphs = std::move(glyphs) 
	};
}

bool Glyph::isVisible() const {
	return visibleSize.x > 0 && visibleSize.y > 0;
}