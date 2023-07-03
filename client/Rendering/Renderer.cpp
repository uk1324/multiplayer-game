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
#include <engine/Math/Color.hpp>
#include <client/Rendering/Shaders/Postprocess/bloomData.hpp>

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

#define SHADERS_PATH "client/Rendering/Shaders/"

#define CREATE_GENERATED_SHADER(name) ShaderManager::createShader(name##_SHADER_VERT_PATH, name##_SHADER_FRAG_PATH)

Renderer::Renderer()
	: fullscreenQuadPtVbo(fullscreenQuadVerts, sizeof(fullscreenQuadVerts))
	, fullscreenQuadPtIbo(fullscreenQuadIndices, sizeof(fullscreenQuadIndices))
	, texturedQuadPerInstanceDataVbo(sizeof(texturedQuadPerInstanceData))
	, atlasTexture(Texture::null())
	, spriteVao(Vao::generate())
	, deathAnimationVao(Vao::null())
	, circleVao(Vao::null())
	, lineVao(Vao::null())
	, instancesVbo(INSTANCES_VBO_BYTES_SIZE)
	, fontVao(Vao::null())
	, font([]() {
		Timer timer;
		auto font = fontLoadSdfWithCaching(
			"assets/fonts/RobotoMono-Regular.ttf",
			"generated/font.png",
			"generated/fontInfo.json",
			POLISH_CHARACTER_RANGES,
			64
		);
		put("font loading took %", timer.elapsedMilliseconds());
		if (font.has_value()) {
			return std::move(*font);
		}
		LOG_FATAL("failed to load font %s", font.error().message.c_str());
	}())
	, postProcessFbo0(Fbo::generate())
	, postProcessFbo1(Fbo::generate())
	, postprocessTexture0(Texture::generate())
	, postprocessTexture1(Texture::generate())
	, postprocessShader(ShaderManager::createShader(SHADERS_PATH "Postprocess/postprocess.vert", SHADERS_PATH "Postprocess/bloom.frag")) {

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

	lineVao = createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo);
	lineVao.bind();
	instancesVbo.bind();
	addLineInstanceAttributesToVao(lineVao);
	lineShader = &CREATE_GENERATED_SHADER(LINE);

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

	{
		auto initializePostProcessFbo = [](Fbo& fbo, Texture& color) {
			fbo.bind();
			color.bind();
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, color.handle(), 0);
			Fbo::unbind();
		};
		initializePostProcessFbo(postProcessFbo0, postprocessTexture0);
		initializePostProcessFbo(postProcessFbo1, postprocessTexture1);
	}
}

void Renderer::onResize() {
	const auto newSize = Window::size();
	put("resized window size: %", newSize);
	auto updatePostprocessTexture = [&newSize](Texture& texture) {
		texture.bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F /* Not sure why this format */, newSize.x, newSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
	};

	updatePostprocessTexture(postprocessTexture0);
	updatePostprocessTexture(postprocessTexture1);

	glViewport(0, 0, newSize.x, newSize.y);
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

	// Thechnically this is incorrect, because this uses the window resize function not the framebuffer resize function, but it shouldn't matter on most system. On windows and on the x window manager it is exacly the same thing. The function to get framebuffer calls the function to get the window size on those systems.
	if (Window::resized()) {
		onResize();
	}
	currentWriteFbo().bind();

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

	drawDebugShapes();

	glActiveTexture(GL_TEXTURE0);
	font.fontAtlas.bind();
	textShader->use();
	textShader->setTexture("fontAtlas", 0);
	fontVao.bind();
	textInstances.drawCall(instancesVbo, INSTANCES_VBO_BYTES_SIZE, std::size(fullscreenQuadIndices));
	textInstances.toDraw.clear();

	// Maybe render to only a part of the texture and only read from a part of it in the next pass if needed.

	{
		static BloomFragUniforms uniforms{
			.color = Color3::WHITE
		};
		GUI_PROPERTY_EDITOR(gui(uniforms));
		shaderSetUniforms(postprocessShader, uniforms);

	}

	swapFbos();
	Fbo::unbind();
	postprocessShader.use();
	glActiveTexture(GL_TEXTURE0);
	currentReadTexture().bind();
	postprocessShader.setTexture("colorBuffer", 0);
	spriteVao.bind();
	glDrawElements(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr);
}

Vec2 Renderer::getQuadPixelSize(Vec2 scale) const {
	return Vec2(getQuadPixelSizeX(scale.x), getQuadPixelSizeX(scale.y));
}

float Renderer::getQuadPixelSizeX(float scale) const {
	return scale * camera.zoom * Window::size().x;
}

float Renderer::getQuadPixelSizeY(float scale) const {
	return scale * camera.zoom * Window::size().y;
}

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

	if (info.isVisible()) {
		const auto pixelSize = getQuadPixelSizeY(maxHeight);
		textInstances.toDraw.push_back(TextInstance{
			.transform = camera.makeTransform(centerPos, 0.0f, size / 2.0f),
			.offsetInAtlas = Vec2(info.offsetInAtlas) / Vec2(font.fontAtlasPixelSize),
			.sizeInAtlas = Vec2(info.sizeInAtlas) / Vec2(font.fontAtlasPixelSize),
			// This value is incorrect because it uses pixel size of the quad and not the size of the sdf outline. This looks good enough, but might vary between fonts.
			.smoothing = 15.0f / pixelSize
		});
	}

	// Advance values are stored as 1/64 of a pixel.
	return Vec2(pos.x + (info.advance.x >> 6) * scale, pos.y);
}

void Renderer::addTextToDraw(TextInstances& instances, const Font& font, Vec2 pos, float maxHeight, std::string_view utf8Text) {
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

Fbo& Renderer::currentWriteFbo() {
	if (currentFboIndex = 0) {
		return postProcessFbo0;
	} else {
		return postProcessFbo1;
	}
}

Texture& Renderer::currentReadTexture() {
	if (currentFboIndex = 0) {
		return postprocessTexture0;
	} else {
		return postprocessTexture1;
	}
}

void Renderer::swapFbos() {
	if (currentFboIndex == 0) {
		currentFboIndex = 1;
	} else {
		currentFboIndex = 0;
	}
}

void Renderer::drawDebugShapes() {
	for (const auto& line : Debug::lines) {
		const auto vector = line.end - line.start;
		const auto direction = vector.normalized();
		const auto width = line.width.has_value() ? *line.width : 20.0f / Window::size().y;
		const auto pixelWidth = getQuadPixelSizeY(width);
		lineInstances.toDraw.push_back(LineInstance{
			.transform = camera.makeTransform(line.start + vector / 2.0f, vector.angle(), Vec2(vector.length() / 2.0f + width, width)),
			.color = Vec4(line.color, 1.0f),
			.smoothing = 3.0f / pixelWidth,
			.lineWidth = width,
			.lineLength = vector.length() + width * 2.0f,
		});
	}
	INSTANCED_DRAW_QUAD_PT(line);

	for (const auto& circle : Debug::circles) {
		const auto pixelSize = getQuadPixelSizeY(circle.radius);
		circleInstances.toDraw.push_back(CircleInstance{
			.transform = camera.makeTransform(circle.pos, 0.0f, Vec2(circle.radius)),
			.color = Vec4(circle.color.x, circle.color.y, circle.color.z, 1.0f),
			.smoothing = 5.0f / pixelSize,
		});
	}
	INSTANCED_DRAW_QUAD_PT(circle);

	for (const auto& text : Debug::texts) {
		const auto info = getTextInfo(font, text.height, text.text);
		Vec2 pos = text.pos;
		pos.y -= info.bottomY;
		pos -= info.size / 2.0f;
		addTextToDraw(textInstances, font, pos, text.height, text.text);
	}
}

Vec2 Renderer::Sprite::scaledSize(float scale) const {
	return scale * Vec2(1.0f, aspectRatio);
}
