#pragma once

#include <Engine/Graphics/ShaderProgram.hpp>
#include <Engine/Graphics/Vao.hpp>
#include <Engine/Graphics/Ibo.hpp>
#include <Engine/Graphics/Texture.hpp>
#include <Engine/Graphics/Fbo.hpp>
#include <Engine/Graphics/TextureAtlasGenerator.hpp>
#include <client/Camera.hpp>
#include <engine/Math/Vec4.hpp>
#include <client/Rendering/Shaders/deathAnimationData.hpp>
#include <client/Rendering/Shaders/circleData.hpp>
#include <client/Rendering/Shaders/lineData.hpp>
#include <client/Rendering/Shaders/textData.hpp>
#include <client/Rendering/Shaders/playerData.hpp>
#include <client/Rendering/Font.hpp>
#include <filesystem>

struct Renderer {
	struct Sprite {
		Vec2 offset, size;
		float aspectRatio;

		Vec2 scaledSize(float scale) const;
	};

	Renderer();

	void onResize();
	void update();

	Camera camera;

	Vec2 getQuadPixelSize(Vec2 scale) const;
	float getQuadPixelSizeX(float scale) const;
	float getQuadPixelSizeY(float scale) const;
	// For performance could reuse the memory used to store instances for example for debug shapes.
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

	PlayerInstances playerInstances;
	ShaderProgram& playerShader;
	Vao playerVao;

	Font font;
	ShaderProgram* textShader;
	Vao fontVao;
	TextInstances textInstances;
	Vec2 addCharacterToDraw(TextInstances& instances, const Font& font, Vec2 pos, float maxHeight, char32_t character);
	void addTextToDraw(TextInstances& instances, const Font& font, Vec2 pos, float maxHeight, std::string_view utf8Text);
	/*void addTextToDrawCenteredX(TextInstances& instances, const Font& font, Vec2 pos, float height, std::string_view utf8Text);*/
	struct TextInfo {
		Vec2 size;
		float bottomY;
	};
	TextInfo getTextInfo(const Font& font, float height, std::string_view utf8Text);

	void drawSprite(Sprite sprite, Vec2 pos, float size, float rotation = 0.0f, Vec4 color = Vec4(1.0f));
	void drawSprite(Sprite sprite, Vec2 pos, Vec2 size, float rotation = 0.0f, Vec4 color = Vec4(1.0f));
	Sprite playerSprite;
	Sprite bulletSprite;
	Sprite bullet2Sprite;
	Sprite bullet3Sprite;

	ShaderProgram* backgroundShader;
	ShaderProgram& spaceBackgroundShader;

	Fbo postProcessFbo0;
	Texture postprocessTexture0;
	Fbo postProcessFbo1;
	Texture postprocessTexture1;

	int currentFboIndex = 0;
	Fbo& currentWriteFbo();
	Texture& currentReadTexture();
	void swapFbos();

	ShaderProgram& postprocessShader;

	struct DeathAnimation {
		Vec2 position;
		Vec3 color;
		float t = 0.0f;
		int playerIndex;
	};

	struct SpawnAnimation {
		int playerIndex;
		float t = 0.0;
	};

	std::vector<DeathAnimation> deathAnimations;
	std::vector<SpawnAnimation> spawnAnimations;

	ShaderProgram* deathAnimationShader;
	Vao deathAnimationVao;
	DeathAnimationInstances deathAnimationInstances;
	
	ShaderProgram* circleShader;
	Vao circleVao;
	CircleInstances circleInstances;

	ShaderProgram* lineShader;
	Vao lineVao;
	LineInstances lineInstances;
	void drawDebugShapes();
};