#pragma once

//#include <engine/Graphics/ShaderProgram.hpp>
//#include <engine/Graphics/Texture.hpp>
//#include <client/Rendering/Shaders/textData.hpp>
//#include <string_view>
//
//struct TextRenderer {
//	struct Character {
//		Vec2T<int> atlasOffset;
//		Vec2T<int> size;
//		Vec2T<int> bearing;
//		Vec2T<int> advance;
//	};
//
//	struct Font {
//		Texture fontAtlas;
//		std::unordered_map<char, Character> characters;
//		TextInstances textInstances;
//
//		void addToDraw(const Vec2& )
//	};
//
//	static FontData loadFont(const char* path);
//
//	void drawText(const FontData& font, std::string_view text)
//
//	ShaderProgram& fontShader;
//};