#include <client/Renderer.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <engine/Math/Utils.hpp>
#include <Gui.hpp>
#include <imgui/imgui.h>
#include <glad/glad.h>
#include <client/Debug.hpp>
#include <engine/Math/Transform.hpp>
#include <engine/Math/LineSegment.hpp>
#include <engine/Math/Polygon.hpp>
#include <iostream>
#include <client/PtVertex.hpp>
#include <client/LineTriangulate.hpp>

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

Renderer::Renderer() 
	: fullscreenQuadPtVbo(fullscreenQuadVerts, sizeof(fullscreenQuadVerts))
	, fullscreenQuadPtIbo(fullscreenQuadIndices, sizeof(fullscreenQuadIndices))
	, texturedQuadPerInstanceDataVbo(sizeof(texturedQuadPerInstanceData))
	, atlasTexture(Texture::null())
	, spriteVao(Vao::generate())
	, deathAnimationVao(Vao::null())
	, circleVao(Vao::null())
	, instancesVbo(INSTANCES_VBO_BYTES_SIZE)
{
	deathAnimationVao = createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo);
	deathAnimationVao.bind();
	instancesVbo.bind();
	addDeathAnimationInstanceAttributesToVao(deathAnimationVao);
	deathAnimationShader = &createShader("client/Shaders/deathAnimation.vert", "client/Shaders/deathAnimation.frag");

	circleVao = createPtVao(fullscreenQuadPtVbo, fullscreenQuadPtIbo);
	circleVao.bind();
	instancesVbo.bind();
	addCircleInstanceAttributesToVao(circleVao);
	circleShader = &createShader("client/Shaders/circle.vert", "client/Shaders/circle.frag");

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
		std::vector<std::string> paths;

		#define ADD_TO_ATLAS(name, filename) \
			const auto name##Path = "assets/textures/" filename; \
			paths.push_back(name##Path);
		
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

		auto [nameToPos, image] = generateTextureAtlas(paths);
		atlasTexture = Texture(image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
		const auto atlasTextureSize = Vec2(Vec2T<int>(image.width(), image.height()));

		MAKE_SPRITE(player);
		MAKE_SPRITE(bullet);
		MAKE_SPRITE(bullet2);
		MAKE_SPRITE(bullet3);

		#undef MAKE_SPRITE
		#undef ADD_TO_ATLAS
	}	
	{
		spriteShader = &createShader("client/shader.vert", "client/shader.frag");
	}
	{
		backgroundShader = &createShader("client/background.vert", "client/background.frag");
	}
	camera.zoom /= 3.0f;
}

#define INSTANCED_DRAW_QUAD_PT(instanceName) \
	instanceName##Shader->use(); \
	instanceName##Vao.bind(); \
	instanceName##Instances.drawCall(instancesVbo, INSTANCES_VBO_BYTES_SIZE, std::size(fullscreenQuadIndices)); \
	instanceName##Instances.toDraw.clear();

void Renderer::update() {
	if (Input::isKeyHeld(KeyCode::F3) && Input::isKeyDown(KeyCode::T)) {
		reloadShaders();
	}
	reloadChangedShaders();

	glViewport(0, 0, Window::size().x, Window::size().y);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	camera.aspectRatio = Window::aspectRatio();
	cameraTransform = camera.cameraTransform();
	screenScale = Mat3x2::scale(Vec2(1.0f, camera.aspectRatio));

	static std::vector<Vec2> line;
	static float time = 0;
	{
		time += 1.0f / 60.0f;
		static std::vector<Vec2> linea = {
			//{-0.465625048, 0.0484374687 }
			/*{-0.465625048, 0.0484374687 },
			{-0.609375000, -0.285937428 },
			{-0.537500024, 0.995312512 },
			{-0.693749964, 0.482812494 }*/
			{ 0.0f, 0.0f}
	/*		{ 0.131250143, -1.26093757 },
			{ 0.343750119, -1.15468740 },
			{ 0.343750119, -1.15468740 },
			{ 0.621875167, -0.795312405 },
			{ 0.621875167, -0.795312405 },
			{ 0.753124952, -0.573437572 },
			{ 0.753124952, -0.573437572 },
			{ 1.05312502, -0.392187595 },
			{ 1.05312502, -0.392187595 },
			{ 1.09687507, -0.214062601 },
			{ 1.09687507, -0.214062601 },
			{ 1.29374993, 0.204687506 },
			{ 1.29374993, 0.204687506 },
			{ 1.34062493, 0.526562452 },
			{ 1.34062493, 0.526562452 },
			{ 1.22499990, 0.832812488 },
			{ 1.22499990, 0.832812488 },
			{ 1.10624993, 0.948437512 },
			{ 1.10624993, 0.948437512 },
			{ 0.375000000, 1.16093755 },
			{ 0.375000000, 1.16093755 },
			{ -0.100000083, 1.23906255 },*/
		};

		if (linea.size() > 20) {
			linea.erase(linea.begin());
		}
		/*float z = sin(time * 2.0f);
		linea.push_back(Vec2(z / 2.0f, 0.0f));
		for (auto& point : linea) {
			point.y += 0.03f;
		}*/
		//if (Input::isMouseButtonHeld(MouseButton::LEFT)) {
		//	if (linea.size() <= 1 || distance(linea.back(), camera.cursorPos()) > 0.01f) {
		//		linea.push_back(camera.cursorPos());
		//	}
		//	 
		//}

		//std::vector<Vec2> r;
		//for (int i = 0; i < line.size() - 1; i++) {
		//	auto a = line[i];
		//	auto b = line[i + 1];
		//	if (distance(a, b) > 0.05) {
		//		r.push_back(a);
		//	}
		//}
		//r.push_back(line[line.size() - 1]);
		//line = r;
		//line = polygonDouglassPeckerSimplify(linea, 0.03f);
		//line = linea;
		//for (const auto p : line) {
		//	Debug::drawCircle(p, 0.02f);
		//}
		//line = linea;
		static float distanceBetweenAddedPoints = 0.4f;
		ImGui::InputFloat("distanceBetweenAddedPoints", &distanceBetweenAddedPoints);
		if (Input::isMouseButtonHeld(MouseButton::LEFT) && distance(linea.back(), camera.cursorPos()) > distanceBetweenAddedPoints) {
			linea.push_back(camera.cursorPos());
		}

		static std::optional<int> grabbed;
		if (Input::isMouseButtonDown(MouseButton::RIGHT)) {
			for (int i = 0; i < linea.size(); i++) {
				if (distance(camera.cursorPos(), linea[i]) < 0.05f) {
					grabbed = i;
					break;
				}
			}
		}

		if (grabbed.has_value()) {
			linea[*grabbed] = camera.cursorPos();
		}

		if (Input::isMouseButtonUp(MouseButton::RIGHT)) {
			grabbed = std::nullopt;
		}

		if (Input::isKeyDown(KeyCode::R)) {
			const auto l = linea.back();
			linea.clear();
			linea.push_back(l);
		}
		line = linea;
		static bool why = false;
		ImGui::Checkbox("simplify", &why);
		if (why) {
			const auto wh = line;
			line = polygonDouglassPeckerSimplify(wh, 0.015f);
		}

		if (line.size() > 0) {
			static float v = 0.01f;
			ImGui::InputFloat("simplify", &v);
			std::vector<Vec2> r;
			for (int i = 0; i < line.size() - 1; i++) {
				auto a = line[i];
				auto b = line[i + 1];
				if (distance(a, b) > v) {
					r.push_back(a);
				}
			}
			r.push_back(line[line.size() - 1]);
			line = r;
		}

		static bool showPoints = false;
		ImGui::Checkbox("show points", &showPoints);
		if (showPoints) {
			for (const auto& p : line) {
				Debug::drawCircle(p, 0.02f);
			}
		}
	}

	{
		backgroundShader->use();
		const auto cameraToWorld = (screenScale * cameraTransform).inversed();
		backgroundShader->set("cameraToWorld", cameraToWorld);
		backgroundShader->set("lineLength", static_cast<int>(line.size()));
		//backgroundShader->set("l", line);
		for (int i = 0; i < line.size(); i++) {
			backgroundShader->set("line[" + std::to_string(i) + "]", line[i]);
		}

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
			quad.transform = makeTransform(sprite.pos, sprite.rotation, sprite.size);
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
			.transform = makeTransform(animation.position, 0.0f, Vec2{ 0.75f }),
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

	std::vector<PtVertex> vertices;
	static float width = 0.2f;
	ImGui::InputFloat("width", &width);
	float len = triangulateLine(line, width, vertices);

	static bool color = true;
	ImGui::Checkbox("color", &color);

	const auto transform = makeTransform(Vec2(0.0f), 0.0f, Vec2(1.0f));
	vao.bind();
	vbo.allocateData(vertices.data(), vertices.size() * sizeof(PtVertex));
	static ShaderProgram& lineShader = createShader("./client/Shaders/line.vert", "./client/Shaders/line.frag");
	lineShader.use();
	lineShader.set("transform", transform);
	lineShader.set("color", color);
	lineShader.set("time", time);
	lineShader.set("len", len);
	static bool test = false;
	ImGui::Checkbox("show wireframe", &test);
	if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
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
	
	if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	
	drawDebugShapes();
}

Mat3x2 Renderer::makeTransform(Vec2 pos, float rotation, Vec2 scale) {
	return Mat3x2::rotate(rotation) * screenScale * Mat3x2::scale(scale) * Mat3x2::translate(Vec2{ pos.x, pos.y * camera.aspectRatio }) * cameraTransform;
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
			.transform = makeTransform(circle.pos, 0.0f, Vec2(circle.radius)),
			.color = Vec4(circle.color.x, circle.color.y, circle.color.z, 1.0f),
			.smoothing = width * (2.0f / 3.0f),
			.width = width / circle.radius
		});
	}
	Debug::circles.clear();
	INSTANCED_DRAW_QUAD_PT(circle)
}

ShaderProgram& Renderer::createShader(std::string_view vertPath, std::string_view fragPath) {
	shaders.push_back(ShaderEntry{ 
		.vertPath = vertPath, 
		.fragPath = fragPath, 
		.program = ShaderProgram::create(vertPath, fragPath) 
	});
	auto& shader = shaders.back();
	shader.vertPathLastWriteTime = std::filesystem::last_write_time(shader.vertPath);
	shader.fragPathLastWriteTime = std::filesystem::last_write_time(shader.fragPath);
	return shader.program;
}

void Renderer::reloadShaders() {
	for (auto& entry : shaders) {
		entry.tryReload();
	}
}
void Renderer::ShaderEntry::tryReload() {
	auto result = ShaderProgram::compile(vertPath, fragPath);
	if (const auto error = std::get_if<ShaderProgram::Error>(&result)) {
		std::cout << "tried to reload vert: " << vertPath << " frag: " << fragPath << '\n';
		std::cout << error->toSingleMessage();
	} else {
		std::cout << "reloaded shader\n"
			<< "vert: " << vertPath << '\n'
			<< "frag: " << fragPath << '\n';
		program = std::move(std::get<ShaderProgram>(result));
	}
}

void Renderer::reloadChangedShaders() {
	for (auto& shader : shaders) {
		try {
			const auto vertLastWriteTime = std::filesystem::last_write_time(shader.vertPath);
			const auto fragLastWriteTime = std::filesystem::last_write_time(shader.fragPath);
			if (shader.vertPathLastWriteTime == vertLastWriteTime && shader.fragPathLastWriteTime == fragLastWriteTime) {
				continue;
			}
			shader.tryReload();
			shader.vertPathLastWriteTime = vertLastWriteTime;
			shader.fragPathLastWriteTime = fragLastWriteTime;
		} catch (std::filesystem::filesystem_error) {

		}
	}
}

Vec2 Renderer::Sprite::scaledSize(float scale) const {
	return scale * Vec2(1.0f, aspectRatio);
}
