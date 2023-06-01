#pragma once

#include <Engine/Graphics/ShaderProgram.hpp>
#include <Engine/Graphics/Vao.hpp>
#include <Engine/Graphics/IndexBuffer.hpp>
#include <Engine/Graphics/Texture.hpp>
#include <Engine/Graphics/TextureAtlasGenerator.hpp>
#include <Game/Camera.hpp>
#include <shared/Math/Vec4.hpp>

struct Renderer {
	struct Sprite {
		Vec2 offset, size;
		float aspectRatio;
	};

	Renderer();
	void update(Vec2 playerPos);

	Camera camera;

private:
	Vao fullscreenQuadPtVao;
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

	ShaderProgram* shader;

	Texture atlasTexture;

	struct SpriteToDraw {
		Sprite sprite;
		Vec2 pos;
		float size;
		float rotation;
		Vec4 color;
	};
	std::vector<SpriteToDraw> spritesToDraw;
public:
	void drawSprite(Sprite sprite, Vec2 pos, float size, float rotation = 0.0f, Vec4 color = Vec4(1.0f));

	Sprite playerSprite;
	Sprite bulletSprite;
	Sprite bullet2Sprite;
	Sprite bullet3Sprite;
private:

	ShaderProgram* backgroundShader;

	ShaderProgram& createShader(std::string_view vertPath, std::string fragPath);
	struct ShaderEntry {
		std::string_view vertPath;
		std::string_view fragPath;
		ShaderProgram program;
	};
	std::list<ShaderEntry> shaders;
	void reloadShaders();
};