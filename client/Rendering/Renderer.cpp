﻿#include <client/Rendering/Renderer.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <engine/Math/Utils.hpp>
#include <Gui.hpp>
#include <imgui/imgui.h>
#include <glad/glad.h>
#include <client/Debug.hpp>
#include <engine/Math/Transform.hpp>
#include <engine/Math/LineSegment.hpp>
#include <client/PtVertex.hpp>
#include <client/Rendering/LineTriangulate.hpp>
#include <engine/Utils/Timer.hpp>
#include <client/Rendering/ShaderManager.hpp>
#include <engine/Log/Log.hpp>
#include <iostream>

static constexpr PtVertex fullscreenQuadVerts[]{
	{ Vec2{ -1.0f, 1.0f }, Vec2{ 0.0f, 1.0f } },
	{ Vec2{ 1.0f, 1.0f }, Vec2{ 1.0f, 1.0f } },
	{ Vec2{ -1.0f, -1.0f }, Vec2{ 0.0f, 0.0f } },
	{ Vec2{ 1.0f, -1.0f }, Vec2{ 1.0f, 0.0f } },
};

static constexpr u32 fullscreenQuadIndices[]{
	0, 1, 2, 2, 1, 3
};

Vao createPtVao(Vbo& vbo, Ibo& ibo) {
	auto vao = Vao::generate();
	vao.bind();
	vbo.bind();
	// Index buffers belong to a vao and are not global state so they must be bound after a vao is bound.
	// https://www.khronos.org/opengl/wiki/Vertex_Specification#Index_buffers
	ibo.bind();
	boundVaoSetAttribute(0, ShaderDataType::Float, 2, offsetof(PtVertex, pos), sizeof(PtVertex), false);
	boundVaoSetAttribute(1, ShaderDataType::Float, 2, offsetof(PtVertex, texturePos), sizeof(PtVertex), false);
	Vao::unbind();
	Ibo::unbind();
	return vao;
}

#define CREATE_GENERATED_SHADER(name) ShaderManager::createShader(name##_SHADER_VERT_PATH, name##_SHADER_FRAG_PATH)

// https://character-table.netlify.app/polish/#unicode-ranges
std::pair<char32_t, char32_t> characterRangesToLoad[]{
	{ 0, 127 },
	//{ 0x104, 0x107 },
	{ 0x20, 0x5F },
	{ 0x61, 0x70 },
	{ 0x72, 0x75 },
	{ 0x77, 0x77 },
	{ 0x79, 0x7E },
	{ 0xA0, 0xA0 },
	{ 0xA7, 0xA7 },
	{ 0xA9, 0xA9 },
	{ 0xAB, 0xAB },
	{ 0xB0, 0xB0 },
	{ 0xBB, 0xBB },
	{ 0xD3, 0xD3 },
	{ 0xF3, 0xF3 },
	{ 0x104, 0x107 },
	{ 0x118, 0x119 },
	{ 0x141, 0x144 },
	{ 0x15A, 0x15B },
	{ 0x179, 0x17C },
	{ 0x2010, 0x2011 },
	{ 0x2013, 0x2014 },
	{ 0x201D, 0x201E },
	{ 0x2020, 0x2021 },
	{ 0x2026, 0x2026 },
	{ 0x2030, 0x2030 },
	{ 0x2032, 0x2033 },
	{ 0x20AC, 0x20AC },
};

Renderer::Renderer()
	: fullscreenQuadPtVbo(fullscreenQuadVerts, sizeof(fullscreenQuadVerts))
	, fullscreenQuadPtIbo(fullscreenQuadIndices, sizeof(fullscreenQuadIndices))
	, texturedQuadPerInstanceDataVbo(sizeof(texturedQuadPerInstanceData))
	, atlasTexture(Texture::null())
	, spriteVao(Vao::generate())
	, deathAnimationVao(Vao::null())
	, circleVao(Vao::null())
	, instancesVbo(INSTANCES_VBO_BYTES_SIZE)
	, fontVao(Vao::null())
	, font([]() {
		auto font = fontLoadSdfWithCaching(
			"assets/fonts/RobotoMono-Regular.ttf",
			"generated/font.png",
			"generated/fontInfo.json",
			characterRangesToLoad,
			64
		);
		if (font.has_value()) {
			return std::move(*font);
		}
		LOG_FATAL("failed to load font %s", font.error().message.c_str());
	}()) {

	deathAnimationVao = createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo);
	deathAnimationVao.bind();
	instancesVbo.bind();
	addDeathAnimationInstanceAttributesToVao(deathAnimationVao);
	
	deathAnimationShader = &CREATE_GENERATED_SHADER(DEATH_ANIMATION);

	circleVao = createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo);
	circleVao.bind();
	instancesVbo.bind();
	addCircleInstanceAttributesToVao(circleVao);
	circleShader = &CREATE_GENERATED_SHADER(CIRCLE);

	spriteVao.bind();
	{
		fullscreenQuadPtVbo.bind();
		boundVaoSetAttribute(0, ShaderDataType::Float, 2, offsetof(PtVertex, pos), sizeof(PtVertex));
		boundVaoSetAttribute(1, ShaderDataType::Float, 2, offsetof(PtVertex, texturePos), sizeof(PtVertex));
	}
	{
		texturedQuadPerInstanceDataVbo.bind();
		boundVaoSetAttribute(2, ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, transform), sizeof(TexturedQuadInstanceData));
		glVertexAttribDivisor(2, 1);
		boundVaoSetAttribute(3, ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, transform) + sizeof(Vec2), sizeof(TexturedQuadInstanceData));
		glVertexAttribDivisor(3, 1);
		boundVaoSetAttribute(4, ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, transform) + sizeof(Vec2) * 2, sizeof(TexturedQuadInstanceData));
		glVertexAttribDivisor(4, 1);

		boundVaoSetAttribute(5, ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, atlasOffset), sizeof(TexturedQuadInstanceData));
		glVertexAttribDivisor(5, 1);

		boundVaoSetAttribute(6, ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, size), sizeof(TexturedQuadInstanceData));
		glVertexAttribDivisor(6, 1);

		boundVaoSetAttribute(7, ShaderDataType::Float, 4, offsetof(TexturedQuadInstanceData, color), sizeof(TexturedQuadInstanceData));
		glVertexAttribDivisor(7, 1);
	}
	{
		std::vector<TextureAtlasInputImage> textureAtlasImages;

		#define ADD_TO_ATLAS(spriteName, filename) \
			const auto spriteName##Path = "assets/textures/" filename; \
			textureAtlasImages.push_back(TextureAtlasInputImage{ .name = spriteName##Path, .image = Image32(spriteName##Path) });
		
		#define MAKE_SPRITE(name) \
			{ \
				const auto pos = nameToPos[name##Path]; \
				name##Sprite = Sprite{ \
					.offset = Vec2(pos.offset) / atlasTextureSize, \
					.size = Vec2(pos.size) / atlasTextureSize, \
					.aspectRatio = static_cast<float>(pos.size.y) / static_cast<float>(pos.size.x) \
				}; \
			}

		ADD_TO_ATLAS(player, "player.png")
		ADD_TO_ATLAS(bullet, "bullet.png")
		ADD_TO_ATLAS(bullet2, "bullet2.png")
		ADD_TO_ATLAS(bullet3, "bullet3.png")

		auto [nameToPos, image] = generateTextureAtlas(textureAtlasImages);
		const Texture::Settings settings {
			.wrapS = Texture::Wrap::CLAMP_TO_EDGE,
			.wrapT = Texture::Wrap::CLAMP_TO_EDGE,
		};
		atlasTexture = Texture(image, settings);
		const auto atlasTextureSize = Vec2(Vec2T<int>(image.width(), image.height()));

		MAKE_SPRITE(player);
		MAKE_SPRITE(bullet);
		MAKE_SPRITE(bullet2);
		MAKE_SPRITE(bullet3);

		#undef MAKE_SPRITE
		#undef ADD_TO_ATLAS
	}	
	{
		spriteShader = &ShaderManager::createShader("client/shader.vert", "client/shader.frag");
	}
	{
		backgroundShader = &ShaderManager::createShader("client/background.vert", "client/background.frag");
	}
	camera.zoom /= 3.0f;

	{
		textShader = &CREATE_GENERATED_SHADER(TEXT);
		fontVao = createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo);
		fontVao.bind();
		instancesVbo.bind();
		addTextInstanceAttributesToVao(fontVao);
	}
}

#define INSTANCED_DRAW_QUAD_PT(instanceName) \
	instanceName##Shader->use(); \
	instanceName##Vao.bind(); \
	instanceName##Instances.drawCall(instancesVbo, INSTANCES_VBO_BYTES_SIZE, std::size(fullscreenQuadIndices)); \
	instanceName##Instances.toDraw.clear();

void Renderer::update() {
	ShaderManager::update();
	if (Input::isKeyHeld(KeyCode::F3) && Input::isKeyDown(KeyCode::T)) {
		ShaderManager::reloadAllShaders();
	}

	glViewport(0, 0, Window::size().x, Window::size().y);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera.aspectRatio = Window::aspectRatio();

	{
		backgroundShader->use();
		backgroundShader->set("clipToWorld", camera.clipSpaceToWorldSpace());
		spriteVao.bind();
		fullscreenQuadPtIbo.bind();

		glDrawElements(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr);
	}

	{
		spriteShader->use();
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		atlasTexture.bind();
		spriteShader->setTexture("textureAtlas", 0);

		int toDraw = 0;
		spriteVao.bind();
		texturedQuadPerInstanceDataVbo.bind();
		for (int i = 0; i < spritesToDraw.size(); i++) {
			const auto& sprite = spritesToDraw[i];
			auto& quad = texturedQuadPerInstanceData[toDraw];
			quad.transform = camera.makeTransform(sprite.pos, sprite.rotation, sprite.size);
			quad.atlasOffset = sprite.sprite.offset;
			quad.size = sprite.sprite.size;
			quad.color = sprite.color;
			toDraw++;
			if (toDraw == std::size(texturedQuadPerInstanceData) || i == spritesToDraw.size() - 1) {
				boundVboSetData(0, texturedQuadPerInstanceData, toDraw * sizeof(TexturedQuadInstanceData));
				glDrawElementsInstanced(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr, toDraw);
				toDraw = 0;
			}
		}
		spritesToDraw.clear();
	}

#define ANIMATION_UPDATE(animations, amount) \
	do { \
		for (auto& animation : animations) { \
			animation.t += amount; \
		} \
		std::erase_if(animations, [](const auto& animation) { return animation.t >= 1.0f; }); \
	} while (false)

#define ANIMATION_UPDATE_DEBUG(animations, amount) \
	do { \
		static auto updateAnimations = true; \
		static auto speed = amount; \
		ImGui::Checkbox("update", &updateAnimations); \
		ImGui::SliderFloat("speed", &speed, 0.0f, 1.0f); \
		int i = 0; \
		for (auto& animation : animations) { \
			if (updateAnimations) animation.t += speed; \
			ImGui::PushID(i); \
			ImGui::SliderFloat("t", &animation.t, 0.0f, 1.0f); \
			ImGui::PopID(); \
			i++; \
		} \
		std::erase_if(animations, [](const auto& animation) { return animation.t >= 1.0f; }); \
	} while (false)

#define ANIMATION_DEFULAT_SPAWN(animations, ...) \
	do { \
		if (ImGui::Button("spawn")) { \
			animations.push_back((__VA_ARGS__)); \
		} \
	} while (false)


	//ANIMATION_DEFULAT_SPAWN(deathAnimations, DeathAnimation{ .position = Vec2(0.0f), .t = 0.0f, .playerIndex = 0 });
	ANIMATION_UPDATE(deathAnimations, 0.025f);

	//ANIMATION_DEFULAT_SPAWN(spawnAnimations, SpawnAnimation{ .playerIndex = 0 });
	ANIMATION_UPDATE(spawnAnimations, 0.02f);

	for (const auto& animation : deathAnimations) {
		deathAnimationInstances.toDraw.push_back(DeathAnimationInstance{
			.transform = camera.makeTransform(animation.position, 0.0f, Vec2{ 0.75f }),
			.time = animation.t,
		});
	}
	INSTANCED_DRAW_QUAD_PT(deathAnimation);

	static Vbo vbo = Vbo::generate();
	static Ibo ibo = Ibo::generate();
	static const auto vao = [&]() {
		auto v = Vao::generate();
		v.bind();
		vbo.bind();
		boundVaoSetAttribute(0, ShaderDataType::Float, 3, offsetof(PtVertex, pos), sizeof(PtVertex), false);
		boundVaoSetAttribute(1, ShaderDataType::Float, 2, offsetof(PtVertex, texturePos), sizeof(PtVertex), false);
		return v;
	}();

	/*std::vector<PtVertex> vertices;
	static float width = 0.2f;
	ImGui::InputFloat("width", &width);
	float len = triangulateLine(line, width, vertices);

	static bool color = true;
	ImGui::Checkbox("color", &color);

	const auto transform = makeTransform(Vec2(0.0f), 0.0f, Vec2(1.0f));
	vao.bind();
	vbo.allocateData(vertices.data(), vertices.size() * sizeof(PtVertex));
	static ShaderProgram& lineShader = ShaderManager::createShader("./client/Rendering/Shaders/line.vert", "./client/Rendering/Shaders/line.frag");
	lineShader.use();
	lineShader.set("transform", transform);
	lineShader.set("color", color);
	lineShader.set("time", time);
	lineShader.set("len", len);
	static bool test = false;
	ImGui::Checkbox("show wireframe", &test);
	if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size());*/
	//glEnable(GL_DEPTH_TEST);
	//{
	//	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	//	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	//	glDepthFunc(GL_LEQUAL);
	//	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	//	glDepthFunc(GL_LESS);
	//}
	//glDisable(GL_DEPTH_TEST);
	
	//if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	//drawText("The quick brown fox jumps over the lazy dog");
	drawText(U"1234567890Pchnąć w tę łódź jeża lub ośm skrzyń fig");	

	glActiveTexture(GL_TEXTURE0);
	font.fontAtlas.bind();
	textShader->use();
	textShader->setTexture("fontAtlas", 0);
	static float elapsed = 0.0f;
	elapsed += 1 / 60.0f;
	textShader->set("time", elapsed);
	fontVao.bind();
	textInstances.drawCall(instancesVbo, INSTANCES_VBO_BYTES_SIZE, std::size(fullscreenQuadIndices));
	textInstances.toDraw.clear();

	drawDebugShapes();
}

void Renderer::drawText(std::basic_string_view<char32_t> text) {
	const auto pos = Input::cursorPosClipSpace() * camera.clipSpaceToWorldSpace();
	float x = pos.x;
	float y = pos.y;
	static float size = 1.0f;
	ImGui::InputFloat("size", &size);
	for (const auto& c : text) {
		const auto& characterIt = font.glyphs.find(c);
		if (characterIt == font.glyphs.end()) {
			continue;
		}
		const auto& info = characterIt->second;

		/*float scale = 0.005f * size;*/
		/*float scale = 0.005f * size;*/
		float scale = size * (1.0f / font.pixelHeight);
		float xpos = x + info.bearing.x * scale;
		float ypos = y - (info.size.y - info.bearing.y) * scale;

		float w = info.size.x * scale;
		float h = info.size.y * scale;
		
		// now advance cursors for next glyph (note that advance is number of 1/64 pixels)
		xpos += w / 2.0f;
		ypos += h / 2.0f;

		if (info.isVisible()) {
			textInstances.toDraw.push_back(TextInstance{
				.transform = camera.makeTransform(Vec2(xpos, ypos), 0.0f, Vec2(w, h) / 2.0f),
				.offsetInAtlas = Vec2(info.atlasOffset) / Vec2(font.fontAtlasPixelSize),
				.sizeInAtlas = Vec2(info.size) / Vec2(font.fontAtlasPixelSize),
			});
		}
		x += (info.advance.x >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64)

	}
}

void Renderer::drawSprite(Sprite sprite, Vec2 pos, float size, float rotation, Vec4 color) {
	spritesToDraw.push_back(SpriteToDraw{ sprite, pos, sprite.scaledSize(size), rotation, color});
}

void Renderer::drawSprite(Sprite sprite, Vec2 pos, Vec2 size, float rotation, Vec4 color) {
	spritesToDraw.push_back(SpriteToDraw{ sprite, pos, size, rotation, color });
}

void Renderer::playDeathAnimation(Vec2 position, int playerIndex) {
	deathAnimations.push_back(DeathAnimation{
		.position = position,
		.playerIndex = playerIndex
	});
}



void Renderer::drawDebugShapes() {
	const auto pos = Vec2(0.5f);
	ImGui::Text("%g, %g", pos.x, pos.y);
	Debug::drawCircle(pos, 0.1f);
	if (Input::isKeyHeld(KeyCode::W)) {
		camera.pos.y += 0.01;
	}
	if (Input::isKeyHeld(KeyCode::S)) {
		camera.pos.y -= 0.01;
	}

	if (Input::isKeyHeld(KeyCode::D)) {
		camera.pos.x += 0.01;
	}
	if (Input::isKeyHeld(KeyCode::A)) {
		camera.pos.x -= 0.01;
	}

	if (Input::isKeyHeld(KeyCode::J)) {
		camera.zoom *= 1.02;
	}
	if (Input::isKeyHeld(KeyCode::K)) {
		camera.zoom /= 1.02;
	}

	Debug::drawCircle(camera.pos, 0.1f);

	for (const auto& circle : Debug::circles) {
		const auto width = 0.003f * 1920.0f / Window::size().x * 5.0f;
		circleInstances.toDraw.push_back(CircleInstance{
			.transform = camera.makeTransform(circle.pos, 0.0f, Vec2(circle.radius)),
			.color = Vec4(circle.color.x, circle.color.y, circle.color.z, 1.0f),
			.smoothing = width * (2.0f / 3.0f),
			.width = width / circle.radius
		});
	}
	Debug::circles.clear();
	INSTANCED_DRAW_QUAD_PT(circle)
}

Vec2 Renderer::Sprite::scaledSize(float scale) const {
	return scale * Vec2(1.0f, aspectRatio);
}
