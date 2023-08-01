#pragma once

#include <engine/Graphics/Vao.hpp>
#include <engine/Graphics/Ibo.hpp>
#include <engine/Graphics/Texture.hpp>
#include <engine/Graphics/Fbo.hpp>
#include <client/Camera.hpp>
#include <client/Rendering/Shaders/deathAnimationData.hpp>
#include <client/Rendering/Shaders/circleData.hpp>
#include <client/Rendering/Shaders/lineData.hpp>
#include <client/Rendering/Shaders/textData.hpp>
#include <client/Rendering/Shaders/playerData.hpp>
#include <client/Rendering/Shaders/bulletData.hpp>
#include <client/Rendering/Shaders/spaceBackgroundData.hpp>
#include <client/Rendering/Shaders/cooldownTimerData.hpp>
#include <client/Rendering/Shaders/transitionScreenData.hpp>
#include <client/Rendering/Shaders/mapData.hpp>
#include <client/Rendering/Shaders/mapPlayerMakerData.hpp>
#include <client/Rendering/Font.hpp>
#include <shared/PlayerIndex.hpp>

// Maybe split the Renderer into a renderer and an animator. Animator would call the renderer and add new objects to draw.
struct Renderer {
	Renderer();

	void onResize();
	void update();

	Camera camera;

	Vec2 getQuadPixelSize(Vec2 scale) const;
	float getQuadPixelSizeX(float scale) const;
	float getQuadPixelSizeY(float scale) const;
	// For performance could reuse the memory used to store instances for example for debug shapes.
	static constexpr usize INSTANCES_VBO_BYTES_SIZE = 4096;

	// Don't put lower. Code depends on initialization order.
	Vbo instancesVbo;
	Vbo fullscreenQuadPtVbo;
	Ibo fullscreenQuadPtIbo;
	Vao fullscreenQuadPtVao;

	template<typename TypeInstances>
	struct Instances {
		Instances(ShaderProgram& shader, Vao&& vao, Vbo& instancesVbo);

		TypeInstances instances;
		ShaderProgram& shader;
		Vao vao;
	};

	Instances<PlayerInstances> player;
	Instances<BulletInstances> bullet;
	Instances<DeathAnimationInstances> deathAnimation;
	Instances<CooldownTimerInstances> cooldownTimer;
	Instances<TransitionScreenInstances> transitionScreen;
	Instances<MapInstances> map;
	Instances<MapPlayerMarkerInstances> mapPlayerMarker;

	Font font;
	Instances<TextInstances> text;
	Vec2 addCharacterToDraw(TextInstances& instances, const Font& font, Vec2 pos, const Mat3x2& transform, float maxHeight, char32_t character, Vec4 color);
	void addTextToDraw(TextInstances& instances, const Font& font, Vec2 pos, const Mat3x2& transform, float maxHeight, std::string_view utf8Text, Vec4 color = Vec4(1.0f));
	struct TextInfo {
		Vec2 size;
		float bottomY;
	};
	TextInfo getTextInfo(const Font& font, float height, std::string_view utf8Text);

	ShaderProgram& spaceBackgroundShader;
	SpaceBackgroundFragUniforms spaceBackgroundUniforms{
		.color0 = { 0.0f, 0.279424f, 0.509652f },
		.borderColor = { 0.0f, 0.26073f, 0.637066f },
		.borderRadius = 100.0,
	};

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
		float t = 0.0f;
		PlayerIndex playerIndex;
	};

	struct SpawnAnimation {
		PlayerIndex playerIndex;
		float t = 0.0;
	};

	std::vector<DeathAnimation> deathAnimations;
	std::vector<SpawnAnimation> spawnAnimations;

	Instances<CircleInstances> circle;
	Instances<LineInstances> line;
	void drawDebugShapes();
};