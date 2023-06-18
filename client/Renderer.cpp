#include <client/Renderer.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <engine/Math/Utils.hpp>
#include <Gui.hpp>
#include <imgui/imgui.h>
#include <glad/glad.h>
#include <client/Debug.hpp>
#include <engine/Math/LineSegment.hpp>
#include <iostream>

struct PtVertex {
	Vec2 pos, texturePos;
};

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
	, fullscreenQuadPtIbo(fullscreenQuadIndices, std::size(fullscreenQuadIndices))
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

	{
		backgroundShader->use();
		const auto cameraToWorld = (screenScale * cameraTransform).inversed();
		backgroundShader->set("cameraToWorld", cameraToWorld);

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


	ANIMATION_DEFULAT_SPAWN(deathAnimations, DeathAnimation{ .position = Vec2(0.0f), .t = 0.0f, .playerIndex = 0 });
	ANIMATION_UPDATE_DEBUG(deathAnimations, 0.025f);

	//ANIMATION_DEFULAT_SPAWN(spawnAnimations, SpawnAnimation{ .playerIndex = 0 });
	ANIMATION_UPDATE(spawnAnimations, 0.02f);

	for (const auto& animation : deathAnimations) {
		deathAnimationInstances.toDraw.push_back(DeathAnimationInstance{
			.transform = makeTransform(animation.position, 0.0f, Vec2{ 0.75f }),
			.time = animation.t,
			});
	}
	INSTANCED_DRAW_QUAD_PT(deathAnimation);

	drawDebugShapes();

	static std::vector<Vec2> line = {
		/*Vec2(-1.14062, 0.495313),
		Vec2(-0.0375, 1.18281),
		Vec2(0.840625, 1.03906),
		Vec2(1.4375, 0.142188),
		Vec2(1.325, -0.795312),
		Vec2(-0.0187501, -1.34531),*/
		Vec2(0.0f)
	};
	//if (Input::isMouseButtonDown(MouseButton::LEFT)) {
	//	line.push_back(camera.cursorPos());
	//	//renderer.camera.screenSpaceToCameraSpace(Input::cursorPos())
	//}

	if (line.size() > 50) {
		line.erase(line.begin());
	}
	if (Input::isMouseButtonHeld(MouseButton::LEFT) && distance(line.back(), camera.cursorPos()) > 0.1f) {
		line.push_back(camera.cursorPos());
	}
	/*
	if (Input::isKeyDown(KeyCode::R)) {
		for (const auto& p : line) {
			std::cout << "Vec2(" << p.x << ", " << p.y << "),\n";
		}
	}*/

	/*static constexpr u32 fullscreenQuadIndices[]{
	0, 1, 2, 2, 1, 3
	};*/
	std::vector<PtVertex> vertices = {
		/*{ Vec2{ -1.0f, 1.0f }, Vec2{ 0.0f, 1.0f } },
		{ Vec2{ 1.0f, 1.0f }, Vec2{ 1.0f, 1.0f } },
		{ Vec2{ -1.0f, -1.0f }, Vec2{ 0.0f, 0.0f } },
		{ Vec2{ -1.0f, -1.0f }, Vec2{ 0.0f, 0.0f } },
		{ Vec2{ 1.0f, 1.0f }, Vec2{ 1.0f, 1.0f } },
		{ Vec2{ 1.0f, -1.0f }, Vec2{ 1.0f, 0.0f } },*/
	};
	static Vbo vbo(1024);
	static const auto vao = [&]() {
		auto v = Vao::generate();
		v.bind();
		vbo.bind();
		boundVaoSetAttribute(0, ShaderDataType::Float, 2, offsetof(PtVertex, pos), sizeof(PtVertex), false);
		boundVaoSetAttribute(1, ShaderDataType::Float, 2, offsetof(PtVertex, texturePos), sizeof(PtVertex), false);
		return v;
	}();

	auto addQuad = [&vertices](Vec2 v0, Vec2 v1, Vec2 v2, Vec2 v3) {
		/*const int v[] = { 0, 1, 2, 2, 1, 3 };
		for (const auto a : v) {

		}*/
		vertices.push_back({ v0, Vec2(0.0f) });
		vertices.push_back({ v1, Vec2(0.0f) });
		vertices.push_back({ v2, Vec2(0.0f) });
		vertices.push_back({ v1, Vec2(0.0f) });
		vertices.push_back({ v2, Vec2(0.0f) });
		vertices.push_back({ v3, Vec2(0.0f) });
	};

	auto addLine = [&](Vec2 s, Vec2 e) {
		addQuad(s, e, s, e);
	};

	float width = 0.1f;
	Vec2 previousV1(0.0f);
	Vec2 previousV3(0.0f);
	static bool a;
	ImGui::Checkbox("a", &a);
	for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
		const auto current = line[i];
		const auto next = line[i + 1];
		const auto currentToNext = (next - current);
		const auto normalToLine0 = currentToNext.rotBy90deg().normalized();

		/*const auto v0 = i == 0 ? current + normalToLine0 * width : previousV1;*/
		const auto v0 = current + normalToLine0 * width;
		auto v1 = v0 + currentToNext;
		/*const auto v2 = i == 0 ? current - normalToLine0 * width : previousV3;*/
		const auto v2 = current - normalToLine0 * width;
		auto v3 = v2 + currentToNext;

		if (i == line.size() - 2) {
			addQuad(previousV1, previousV3, v1, v3);
			break;
		}

		const auto nextNext = line[i + 2];
		const auto nextToNextNext = (nextNext - next);
		const auto normalToLine1 = nextToNextNext.rotBy90deg().normalized();

		//addQuad(v0, v1, v2, v3);
		const auto nextV0 = next + normalToLine1 * width;
		const auto nextV1 = nextV0 + nextToNextNext;
		const auto nextV2 = next - normalToLine1 * width;
		const auto nextV3 = nextV2 + nextToNextNext;

		//// TODO: Parallel
		const auto intersection0 = intersectLineSegments(v0, v1, nextV0, nextV1);
		const auto intersection1 = intersectLineSegments(v2, v3, nextV2, nextV3);
		//ASSERT(!(intersection0.has_value() && intersection1.has_value()));
		Vec2 newV1;
		Vec2 newV3;
		if (intersection0.has_value()) {
			newV1 = *intersection0;
			newV3 = *Line(v2, v3).intersection(Line(nextV2, nextV3));
			/*addtoNextV0toV1Length = (v1 - v0) - (newV1 - v0);
			addtoNextV2toV3Length = (v3 - v2) - (newV3 - v2);
			v1 = newV1;
			v3 = newV3;*/
			/*Debug::drawCircle(newV1, 0.02f);
			Debug::drawCircle(newV3, 0.02f);*/
		} else if (intersection1.has_value()) {
			newV3 = *intersection1;
			newV1 = *Line(v0, v1).intersection(Line(nextV0, nextV1));
			/*addtoNextV0toV1Length = (v1 - v0) - (newV1 - v0);
			addtoNextV2toV3Length = (v3 - v2) - (newV3 - v2);
			v1 = newV1;
			v3 = newV3;*/
			/*Debug::drawCircle(newV1, 0.04f);
			Debug::drawCircle(newV3, 0.04f);*/
		} else {
			const auto v3io = Line(v2, v3).intersection(Line(nextV2, nextV3));
			const auto v1io = Line(v0, v1).intersection(Line(nextV0, nextV1));
			if (!v3io.has_value() || !v1io.has_value()) {
				ImGui::Text("error");
				continue;
			}

			if (signedDistance(Line(v3, v3 + normalToLine0), *v3io) < 0.0f) {
				newV3 = v3;
				newV1 = *v1io;
			} else {
				
				newV3 = *v3io;
				newV1 = v1;
			}
			//ImGui::Text("error");
			///*newV3 = v3;
			//newV1 = v1;*/
			//Debug::drawCircle(next, 0.04f);

			///*addLine(v0, v1);
			//addLine(nextV0, nextV1);
			//addLine(v2, v3);
			//addLine(nextV2, nextV3);*/
			//Debug::drawCircle(next, 0.02f);
			//continue;
			/*addLine(v0, v1);
			addLine(nextV0, nextV1);

			addLine(v2, v3);
			addLine(nextV2, nextV3);*/
			//addLine(v2, v3);
			//ASSERT_NOT_REACHED();
		}


		if (a) {
			if (i == 0) {
				addQuad(v0, v2, newV1, newV3);
			} else {
				addQuad(previousV1, previousV3, newV1, newV3);
			}
		}
		previousV1 = newV1;
		previousV3 = newV3;
		///*const auto v2 = v0 + vectorToNext;
		//const auto v3 = v1 + vectorToNext*/;
		//addQuad(v0, v1, v2, v3);
		//previousV1 = v1;
		//previousV3 = v3;
	}

	/*for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
		const auto current = line[i];m
		const auto next = line[i + 1];
		addQuad(current, next, current, next);
	}*/

	const auto transform = makeTransform(Vec2(0.0f), 0.0f, Vec2(1.0f));
	for (auto& vertex : vertices) {
		vertex.pos *= transform;
	}
	vbo.allocateData(vertices.data(), vertices.size() * sizeof(PtVertex));
	//vbo.bind();
	//boundVboSetData();
	static ShaderProgram& lineShader = createShader("./client/Shaders/line.vert", "./client/Shaders/line.frag");
	lineShader.use();
	/*lineShader.set("transform", transform);*/
	/*lineShader.set("transform", Mat3x2::scale(0.1f * Vec2(1.0f, camera.aspectRatio)));*/
	lineShader.set("transform", Mat3x2::identity);
	ImGui::Text("aspect %g", camera.aspectRatio);
	vao.bind();
	static bool test;
	ImGui::Checkbox("test", &test);
	if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLES, 0, vertices.size());
	if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
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
		const auto vertLastWriteTime = std::filesystem::last_write_time(shader.vertPath);
		const auto fragLastWriteTime = std::filesystem::last_write_time(shader.fragPath);
		if (shader.vertPathLastWriteTime == vertLastWriteTime && shader.fragPathLastWriteTime == fragLastWriteTime) {
			continue;
		}
		shader.tryReload();
		shader.vertPathLastWriteTime = vertLastWriteTime;
		shader.fragPathLastWriteTime = fragLastWriteTime;
	}
}

Vec2 Renderer::Sprite::scaledSize(float scale) const {
	return scale * Vec2(1.0f, aspectRatio);
}
