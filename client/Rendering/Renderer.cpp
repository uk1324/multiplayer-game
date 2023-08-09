#include <client/Rendering/Renderer.hpp>
#include <engine/Window.hpp>
#include <engine/Input/Input.hpp>
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
#include <engine/Utils/FileIo.hpp>
#include <engine/Json/JsonPrinter.hpp>
#include <engine/Json/JsonParser.hpp>
#include <shared/Gameplay.hpp>

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


#define INSTANCES_DRAW_QUAD(type) \
		type.shader.use(); \
		type.vao.bind(); \
		type.instances.drawCall(instancesVbo, INSTANCES_VBO_BYTES_SIZE, std::size(fullscreenQuadIndices)); \
		type.instances.toDraw.clear(); \

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

#ifdef FINAL_RELEASE
#include <generated/shaderSources.hpp>
#define CREATE_GENERATED_SHADER(name) ShaderManager::createShaderFromSource(name##_SHADER_VERT_SOURCE, name##_SHADER_FRAG_SOURCE)
#else
#define CREATE_GENERATED_SHADER(name) ShaderManager::createShader(name##_SHADER_VERT_PATH, name##_SHADER_FRAG_PATH)
#endif

//#define QUAD_SHADER_LIST(macro) \
//	macro()

Renderer::Renderer()
	: fullscreenQuadPtVbo(fullscreenQuadVerts, sizeof(fullscreenQuadVerts))
	, fullscreenQuadPtIbo(fullscreenQuadIndices, sizeof(fullscreenQuadIndices))
	, fullscreenQuadPtVao(Vao::generate())
	, instancesVbo(INSTANCES_VBO_BYTES_SIZE)
	, font([]() {
		Timer timer;
		auto font = fontLoadSdfWithCaching(
			"assets/fonts/RobotoMono-Regular.ttf",
			"cached/font.png",
			"cached/fontInfo.json",
			ASCII_CHARACTER_RANGES,
			64
		);
		put("font loading took %", timer.elapsedMilliseconds());
		if (font.has_value()) {
			return std::move(*font);
		}
		LOG_FATAL("failed to load font: \n %s", font.error().message.c_str());
	}())
	, postProcessFbo0(Fbo::generate())
	, postProcessFbo1(Fbo::generate())
	, postprocessTexture0(Texture::generate())
	, postprocessTexture1(Texture::generate())
#ifdef FINAL_RELEASE
	, postprocessShader(ShaderManager::createShaderFromSource(POSTPROCESS_SHADER_VERT_SOURCE, BLOOM_SHADER_FRAG_SOURCE))
#else
	, postprocessShader(ShaderManager::createShader(SHADERS_PATH "Postprocess/postprocess.vert", SHADERS_PATH "Postprocess/bloom.frag"))
#endif

	, spaceBackgroundShader(CREATE_GENERATED_SHADER(SPACE_BACKGROUND))

//  Could use a function instead. That would require creating a empty type that that would be passed as a template.
#define GENERATED_ARGS(NAME) CREATE_GENERATED_SHADER(NAME), createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo), instancesVbo

	, bullet(GENERATED_ARGS(BULLET))
	, player(GENERATED_ARGS(PLAYER))
	, deathAnimation(GENERATED_ARGS(DEATH_ANIMATION))
	, text(GENERATED_ARGS(TEXT))
	, circle(GENERATED_ARGS(CIRCLE))
	, line(GENERATED_ARGS(LINE))
	, cooldownTimer(GENERATED_ARGS(COOLDOWN_TIMER))
	, transitionScreen(GENERATED_ARGS(TRANSITION_SCREEN))
	, map(GENERATED_ARGS(MAP))
	, mapPlayerMarker(GENERATED_ARGS(MAP_PLAYER_MARKER))

#undef GENERATED_ARGS
	{

	{
		fullscreenQuadPtVao.bind();
		fullscreenQuadPtVbo.bind();
		boundVaoSetAttribute(0, ShaderDataType::Float, 2, offsetof(PtVertex, pos), sizeof(PtVertex));
		boundVaoSetAttribute(1, ShaderDataType::Float, 2, offsetof(PtVertex, texturePos), sizeof(PtVertex));
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

	camera.zoom /= 3.0f;
}

void Renderer::onResize() {
	const auto newSize = Vec2T<GLsizei>(Window::size());

	if (newSize.x <= 0 && newSize.y <= 0) {
		return;
	}
		

	put("resized window size: %", newSize);
	auto updatePostprocessTexture = [&newSize](Texture& texture) {
		texture.bind();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F /* Not sure why this format */, newSize.x, newSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
	};

	updatePostprocessTexture(postprocessTexture0);
	updatePostprocessTexture(postprocessTexture1);

	glViewport(0, 0, newSize.x, newSize.y);
}

void Renderer::update() {
	ShaderManager::update();
	if (Input::isKeyHeld(KeyCode::F3) && Input::isKeyDown(KeyCode::T)) {
		ShaderManager::reloadAllShaders();
	}

	// Thechnically this is incorrect, because this uses the window resize function not the framebuffer resize function, but it shouldn't matter on most system. On windows and on the x window manager it is exacly the same thing. The function to get framebuffer calls the function to get the window size on those systems.
	if (Window::resized()) {
		onResize();
	}
	camera.aspectRatio = Window::aspectRatio();

	currentWriteFbo().bind();
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	{
		spaceBackgroundShader.use();
		shaderSetUniforms(spaceBackgroundShader, SpaceBackgroundVertUniforms{ 
			.clipToWorld = camera.clipSpaceToWorldSpace()
		});
		spaceBackgroundUniforms.time += 1.0 / 60.0f;
		spaceBackgroundUniforms.borderRadius = BORDER_RADIUS;
		//GUI_PROPERTY_EDITOR(gui(spaceBackgroundUniforms));
		shaderSetUniforms(spaceBackgroundShader, spaceBackgroundUniforms);
		fullscreenQuadPtVao.bind();
		fullscreenQuadPtIbo.bind();

		glDrawElements(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr);
	}
	
	INSTANCES_DRAW_QUAD(bullet);
	INSTANCES_DRAW_QUAD(player);

	//ANIMATION_DEFULAT_SPAWN(deathAnimations, DeathAnimation{ .position = Vec2(0.0f), .t = 0.0f, .playerIndex = 0});
	ANIMATION_UPDATE(deathAnimations, 0.025f);

	//ANIMATION_DEFULAT_SPAWN(spawnAnimations, SpawnAnimation{ .playerIndex = 0 });
	ANIMATION_UPDATE(spawnAnimations, 0.05f);

	INSTANCES_DRAW_QUAD(deathAnimation);

	// Maybe render to only a part of the texture and only read from a part of it in the next pass if needed (for example for downscaled blur. 

	swapFbos();
	Fbo::unbind();

	postprocessShader.use();

	glActiveTexture(GL_TEXTURE0);
	currentReadTexture().bind();
	postprocessShader.setTexture("colorBuffer", 0);

	fullscreenQuadPtVao.bind();
	glDrawElements(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr);

	glActiveTexture(GL_TEXTURE0);
	font.fontAtlas.bind();
	text.shader.setTexture("fontAtlas", 0);
	INSTANCES_DRAW_QUAD(text);

	INSTANCES_DRAW_QUAD(cooldownTimer);

	INSTANCES_DRAW_QUAD(map);
	INSTANCES_DRAW_QUAD(mapPlayerMarker);

	drawDebugShapes();

	INSTANCES_DRAW_QUAD(transitionScreen);
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

Vec2 Renderer::addCharacterToDraw(TextInstances& instances, const Font& font, Vec2 pos, const Mat3x2& transform, float maxHeight, char32_t character, Vec4 color) {
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
		const auto characterTransform = makeObjectTransform(centerPos, 0.0f, size / 2.0f) * transform;
		//const auto scaleTransform = (objectTransform(Vec2(0.0f), 0.0f, Vec2(0.0f, maxHeight)) * transform).removedTranslation();
		// Using size.y for the scale pixel size calculation instead of max height seems more correct, because the all objects should have more similar smoothing then I think.
		const auto scaleTransform = (makeObjectTransform(Vec2(0.0f), 0.0f, Vec2(0.0f, size.y)) * transform).removedTranslation();
		const auto pixelSize = (Vec2(1.0f) * scaleTransform * Window::size()).y;
		instances.toDraw.push_back(TextInstance{
			.transform = characterTransform,
			.offsetInAtlas = Vec2(info.offsetInAtlas) / Vec2(font.fontAtlasPixelSize),
			.sizeInAtlas = Vec2(info.sizeInAtlas) / Vec2(font.fontAtlasPixelSize),
			// This value is incorrect because it uses pixel size of the quad and not the size of the sdf outline. This looks good enough, but might vary between fonts.
			.smoothing = 15.0f / pixelSize,
			.color = color
		});
	}

	// Advance values are stored as 1/64 of a pixel.
	return Vec2(pos.x + (info.advance.x >> 6) * scale, pos.y);
}

void Renderer::addTextToDraw(TextInstances& instances, const Font& font, Vec2 pos, const Mat3x2& transform, float maxHeight, std::string_view utf8Text, Vec4 color) {
	const char* current = utf8Text.data();
	const char* end = utf8Text.data() + utf8Text.size();
	while (const auto codepoint = Utf8::readCodePoint(current, end)) {
		current = codepoint->next;
		pos = addCharacterToDraw(instances, font, pos, transform, maxHeight, codepoint->codePoint, color);
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

Fbo& Renderer::currentWriteFbo() {
	if (currentFboIndex == 0) {
		return postProcessFbo0;
	} else {
		return postProcessFbo1;
	}
}

Texture& Renderer::currentReadTexture() {
	if (currentFboIndex == 0) {
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
		this->line.instances.toDraw.push_back(LineInstance{
			.transform = camera.makeTransform(line.start + vector / 2.0f, vector.angle(), Vec2(vector.length() / 2.0f + width, width)),
			.color = Vec4(line.color, 1.0f),
			.smoothing = 3.0f / pixelWidth,
			.lineWidth = width,
			.lineLength = vector.length() + width * 2.0f,
		});
	}
	INSTANCES_DRAW_QUAD(line);

	for (const auto& circle : Debug::circles) {
		const auto pixelSize = getQuadPixelSizeY(circle.radius);
		this->circle.instances.toDraw.push_back(CircleInstance{
			.transform = camera.makeTransform(circle.pos, 0.0f, Vec2(circle.radius)),
			.color = Vec4(circle.color.x, circle.color.y, circle.color.z, 1.0f),
			.smoothing = 5.0f / pixelSize,
		});
	}
	INSTANCES_DRAW_QUAD(circle);

	for (const auto& text : Debug::texts) {
		const auto info = getTextInfo(font, text.height, text.text);
		Vec2 pos = text.pos;
		pos.y -= info.bottomY;
		pos -= info.size / 2.0f;
		addTextToDraw(this->text.instances, font, pos, camera.worldToCameraToNdc(), text.height, text.text);
	}
}

template<typename TypeInstances>
Renderer::Instances<TypeInstances>::Instances(ShaderProgram& shader, Vao&& vao, Vbo& instancesVbo)
	: shader(shader)
	, vao(std::move(vao)) {
	this->vao.bind();
	instancesVbo.bind();
	TypeInstances::addInstanceAttributesToVao(this->vao);
}

