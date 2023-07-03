#include <client/Rendering/Renderer.hpp>
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
#include <engine/Utils/Utf8.hpp>
#include <engine/Utils/Put.hpp>
#include <numeric>

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

static int mode = 0;

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
		Timer timer;
		auto font = fontLoadSdfWithCaching(
			"assets/fonts/RobotoMono-Regular.ttf",
			"generated/font.png",
			"generated/fontInfo.json",
			characterRangesToLoad,
			64
		);
		put("font loading took %", timer.elapsedMilliseconds());
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

	auto cursorPos = Input::cursorPosClipSpace() * camera.clipSpaceToWorldSpace();
	static float scale = 1.0f;
	Debug::scrollInput(scale);
	/*const char* text = reinterpret_cast<const char*>(u8"Pchnąć w tę łódź jeża lub ośm skrzyń fig");*/
	/*const char* text = reinterpret_cast<const char*>(u8"Pchnąć w tę łódź jeża lub ośm skrzyń fig");*/
	ImGui::Combo("mode", &mode, "constant smoothstep based on quad size in pixels\0derivatives of distance\0derivatives of texture position\0show derivatives of distance\0show derivatives of texture position\0derivatives of texture position v2\0\0");
	if (mode == 3) {
		ImGui::Text("zoom out to see");
	}
	static char text[1024] = "Sample text";
	ImGui::InputText("text", text, sizeof(text));
	//const char* text = reinterpret_cast<const char*>(u8"e");
	//const char* text = reinterpret_cast<const char*>(u8"g");
	const auto textInfo = getTextInfo(font, scale, text);
	//cursorPos = Vec2(0.0f);
	// Move the bottom y to the cursor pos.
	cursorPos.y -= textInfo.bottomY;
	cursorPos -= textInfo.size / 2.0f;
	//cursorPos.y -= scale / 2.0f;
	addTextToDraw(textInstances, font, cursorPos, scale, text);

	glActiveTexture(GL_TEXTURE0);
	font.fontAtlas.bind();
	textShader->use();
	textShader->setTexture("fontAtlas", 0);
	static bool showOutlineSize = false;
	if (mode == 0) {
		ImGui::Checkbox("showOutlineSize", &showOutlineSize);
		//chkbox(showOutlineSize)
	}
	
	textShader->set("mode", mode);
	textShader->set("show", showOutlineSize);
	static float elapsed = 0.0f;
	elapsed += 1 / 60.0f;
	textShader->set("time", elapsed);
	fontVao.bind();
	textInstances.drawCall(instancesVbo, INSTANCES_VBO_BYTES_SIZE, std::size(fullscreenQuadIndices));
	textInstances.toDraw.clear();

	drawDebugShapes();

	// Maybe render to only a part of the texture and only read from a part of it in the next pass if needed.
}

static float outlineSize = 15.0f;

Vec2 Renderer::addCharacterToDraw(TextInstances& instances, const Font& font, Vec2 pos, float maxHeight, char32_t character) {
	const auto& characterIt = font.glyphs.find(character);
	if (characterIt == font.glyphs.end()) {
		return pos;
	}
	const auto& info = characterIt->second;

	float scale = maxHeight * (1.0f / font.pixelHeight);
	auto centerPos = pos + Vec2(info.bearingRelativeToOffsetInAtlas.x, -(info.sizeInAtlas.y - info.bearingRelativeToOffsetInAtlas.y)) * scale;
	const auto size = Vec2(info.sizeInAtlas) * scale;
	centerPos += size / 2.0f;

	camera.zoom = 1.0f;
	if (info.isVisible()) {
		const auto pixelSize = maxHeight * camera.zoom * Window::size().y;
		//ImGui::Text("%g", pixelSize);
		textInstances.toDraw.push_back(TextInstance{
			.transform = camera.makeTransform(centerPos, 0.0f, size / 2.0f),
			.offsetInAtlas = Vec2(info.offsetInAtlas) / Vec2(font.fontAtlasPixelSize),
			.sizeInAtlas = Vec2(info.sizeInAtlas) / Vec2(font.fontAtlasPixelSize),
			// This value is incorrect because it uses pixel size of the quad and not the size of the sdf outline. This looks good enough, but might vary between fonts.
			.smoothing = outlineSize / pixelSize
			//.smoothing = 15.0f / pixelSize
		});
	}

	// Advance values are stored as 1/64 of a pixel.
	return Vec2(pos.x + (info.advance.x >> 6) * scale, pos.y);
}

void Renderer::addTextToDraw(TextInstances& instances, const Font& font, Vec2 pos, float maxHeight, std::string_view utf8Text) {
	if (mode == 0)
		ImGui::InputFloat("outlineSize", &outlineSize);
	const char* current = utf8Text.data();
	const char* end = utf8Text.data() + utf8Text.size();
	while (const auto codepoint = Utf8::readCodePoint(current, end)) {
		current = codepoint->next;
		pos = addCharacterToDraw(instances, font, pos, maxHeight, codepoint->codePoint);
	}
}

Renderer::TextInfo Renderer::getTextInfo(const Font& font, float maxHeight, std::string_view utf8Text) {
	const auto scale = maxHeight * (1.0f / font.pixelHeight);

	float width = 0.0f;
	float minY = std::numeric_limits<float>::infinity();
	float maxY = 0.0f;

	const char* current = utf8Text.data();
	const char* end = utf8Text.data() + utf8Text.size();
	while (const auto codepoint = Utf8::readCodePoint(current, end)) {
		current = codepoint->next;
		const auto& characterIt = font.glyphs.find(codepoint->codePoint);
		if (characterIt == font.glyphs.end()) {
			continue;
		}
		const auto& info = characterIt->second;
		const auto advance = (info.advance.x >> 6) * scale;
		width += advance;
		const auto bottomY = -(info.visibleSize.y - info.visibleBearing.y);
		const auto topY = bottomY + info.visibleSize.y;
		minY = std::min(minY, bottomY * scale);
		maxY = std::max(maxY, topY * scale);
	}
	// Don't have only add width istead of advance if it is the last character, because of how advance works.
	// In the glyph metrics image you can see that the character is offset from the origin.
	// https://learnopengl.com/In-Practice/Text-Rendering
	// This might not be correct, but I think it is.
	// You can test it works by for example writing "oo"

	float height = maxY - minY;
	if (isnan(height)) {
		height = 0.0f;
	}

	return TextInfo{
		.size = Vec2(width, height),
		.bottomY = minY
	};
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
	for (const auto& circle : Debug::circles) {
		const auto width = 0.003f * 1920.0f / Window::size().x * 5.0f;
		circleInstances.toDraw.push_back(CircleInstance{
			.transform = camera.makeTransform(circle.pos, 0.0f, Vec2(circle.radius)),
			.color = Vec4(circle.color.x, circle.color.y, circle.color.z, 1.0f),
			.smoothing = width * (2.0f / 3.0f),
			.width = width / circle.radius
		});
	}
	INSTANCED_DRAW_QUAD_PT(circle)
}

Vec2 Renderer::Sprite::scaledSize(float scale) const {
	return scale * Vec2(1.0f, aspectRatio);
}
