#pragma once

#include <Engine/Graphics/ShaderProgram.hpp>
#include <Engine/Graphics/Vao.hpp>
#include <Engine/Graphics/Ibo.hpp>
#include <Engine/Graphics/Texture.hpp>
#include <Engine/Graphics/TextureAtlasGenerator.hpp>
#include <client/Camera.hpp>
#include <engine/Math/Vec4.hpp>
#include <client/Rendering/Shaders/deathAnimationData.hpp>
#include <client/Rendering/Shaders/circleData.hpp>
#include <client/Rendering/Shaders/textData.hpp>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H  

struct Renderer {
	struct Sprite {
		Vec2 offset, size;
		float aspectRatio;

		Vec2 scaledSize(float scale) const;
	};

	Renderer();
	void update();

	Camera camera;

private:
	static constexpr usize INSTANCES_VBO_BYTES_SIZE = 4096;
	Vbo instancesVbo;

	Vao spriteVao;
	Vbo fullscreenQuadPtVbo;
	Ibo fullscreenQuadPtIbo;

	struct TexturedQuadInstanceData {
		Mat3x2 transform = Mat3x2::identity;
		Vec2 atlasOffset;
		Vec2 size;
		Vec4 color;
	};
	Vbo texturedQuadPerInstanceDataVbo;
	TexturedQuadInstanceData texturedQuadPerInstanceData[100];

	ShaderProgram* spriteShader;

	Texture atlasTexture;

	struct SpriteToDraw {
		Sprite sprite;
		Vec2 pos;
		Vec2 size;
		float rotation;
		Vec4 color;
	};
	std::vector<SpriteToDraw> spritesToDraw;

	struct Character {
		Vec2T<int> atlasOffset;
		Vec2T<int> size;
		Vec2T<int> bearing;
		Vec2T<int> advance;
	};
	std::unordered_map<char, Character> characters;
	ShaderProgram* textShader;
	void drawText(std::string_view text);
	Vec2T<int> fontAtlasPixelSize;
	Vao fontVao;
	Texture fontAtlas;
	TextInstances textInstances;
public:
	void drawSprite(Sprite sprite, Vec2 pos, float size, float rotation = 0.0f, Vec4 color = Vec4(1.0f));
	void drawSprite(Sprite sprite, Vec2 pos, Vec2 size, float rotation = 0.0f, Vec4 color = Vec4(1.0f));
	Sprite playerSprite;
	Sprite bulletSprite;
	Sprite bullet2Sprite;
	Sprite bullet3Sprite;

	void playDeathAnimation(Vec2 position, int playerIndex);
private:
	ShaderProgram* backgroundShader;

public:
	struct DeathAnimation {
		Vec2 position;
		float t = 0.0f;
		int playerIndex;
	};

	struct SpawnAnimation {
		int playerIndex;
		float t = 0.0;
	};

	std::vector<DeathAnimation> deathAnimations;
	std::vector<SpawnAnimation> spawnAnimations;
private:
	ShaderProgram* deathAnimationShader;
	Vao deathAnimationVao;
	DeathAnimationInstances deathAnimationInstances;
	
	ShaderProgram* circleShader;
	Vao circleVao;
	CircleInstances circleInstances;
	void drawDebugShapes();
};