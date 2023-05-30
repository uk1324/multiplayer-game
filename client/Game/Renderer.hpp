#pragma once

#include <Engine/Graphics/ShaderProgram.hpp>
#include <Engine/Graphics/Vao.hpp>
#include <Engine/Graphics/IndexBuffer.hpp>
#include <Engine/Graphics/Texture.hpp>
#include <Engine/Graphics/TextureAtlasGenerator.hpp>
#include <Game/Camera.hpp>

struct Renderer {
	struct Sprite {
		Vec2 offset, size;
	};

	Renderer();
	void update();

	Camera camera;

private:
	Vao fullscreenQuadPtVao;
	Vbo fullscreenQuadPtVbo;
	Ibo fullscreenQuadPtIbo;

	struct TexturedQuadInstanceData {
		Mat3x2 transform = Mat3x2::identity;
		Vec2 atlasOffset;
		Vec2 size;
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
	};
	std::vector<SpriteToDraw> spritesToDraw;
public:
	void drawSprite(Sprite sprite, Vec2 pos, float size, float rotation = 0.0f);

	Sprite playerSprite;
	Sprite bulletSprite;
	Sprite bullet2Sprite;
private:

	ShaderProgram& createShader(std::string_view vertPath, std::string fragPath);
	struct ShaderEntry {
		std::string_view vertPath;
		std::string_view fragPath;
		ShaderProgram program;
	};
	std::list<ShaderEntry> shaders;
	void reloadShaders();
};