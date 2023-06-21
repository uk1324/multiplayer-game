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
#include <client/LineTriangulator.hpp>

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

#include <client/Clipper2/CPP/Clipper2Lib/include/clipper2/clipper.h>

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
	{
		static std::vector<Vec2> linea = {
			Vec2(0.0f)
		};

		if (linea.size() > 50) {
			linea.erase(linea.begin());
		}
		if (Input::isMouseButtonHeld(MouseButton::LEFT) && distance(linea.back(), camera.cursorPos()) > 0.05f) {
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
		//if (v.size() != 0 && v.size() != sizeBefore) {
		//	Debug::drawPoint(v[0]);
		//}

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

	// It might be possible to do tesslate an arc both on top and the bottom. And move the uv.x faster on the bottom and slower on the top.

	//std::vector<PtVertex> vertices;
	static Vbo vbo = Vbo::generate();
	static Ibo ibo = Ibo::generate();
	static const auto vao = [&]() {
		auto v = Vao::generate();
		v.bind();
		vbo.bind();
		ibo.bind();
		boundVaoSetAttribute(0, ShaderDataType::Float, 3, offsetof(Vert, pos), sizeof(Vert), false);
		boundVaoSetAttribute(1, ShaderDataType::Float, 2, offsetof(Vert, texturePos), sizeof(Vert), false);
		Vao::unbind();
		ibo.unbind();
		return v;
	}();
	// IN 3D REQUIRES CLIPPING AND CULLING
	float distanceAlong = 0.0f;
	float currentLength = 0.0f;
	float jointLength = 0.0f;
	//auto addQuad = [&](Vec2 v0, Vec2 v1, Vec2 v2, Vec2 v3, float upTexturePosX, float downTexturePosX) {
	//	vertices.push_back({ v0, Vec2(previousUpTexturePosX, 0.0f) });
	//	vertices.push_back({ v1, Vec2(previousDownTexturePosY, 1.0f) });
	//	vertices.push_back({ v2, Vec2(upTexturePosX, 0.0f) });
	//	vertices.push_back({ v1, Vec2(previousDownTexturePosY, 1.0f) });
	//	vertices.push_back({ v2, Vec2(upTexturePosX, 0.0f) });
	//	vertices.push_back({ v3, Vec2(downTexturePosX, 1.0f) });
	//	/*vertices.push_back({ up0, Vec2(distanceAlong - currentLength, 0.0f) });
	//	vertices.push_back({ up1, Vec2(distanceAlong - currentLength, 1.0f) });
	//	vertices.push_back({ down0, Vec2(distanceAlong, 0.0f) });
	//	vertices.push_back({ up1, Vec2(distanceAlong - currentLength, 1.0f) });
	//	vertices.push_back({ down0, Vec2(distanceAlong, 0.0f) });
	//	vertices.push_back({ down1, Vec2(distanceAlong, 1.0f) });*/
	//};
	//auto addQuad = [&](PtVertex up0, PtVertex up1, PtVertex down0, PtVertex down1) {
	//	vertices.push_back(up0);
	//	vertices.push_back(up1);
	//	vertices.push_back(down0);
	//	vertices.push_back(up1);
	//	vertices.push_back(down0);
	//	vertices.push_back(down1);
	//	/*vertices.push_back({ up0, Vec2(distanceAlong - currentLength, 0.0f) });
	//	vertices.push_back({ up1, Vec2(distanceAlong - currentLength, 1.0f) });
	//	vertices.push_back({ down0, Vec2(distanceAlong, 0.0f) });
	//	vertices.push_back({ up1, Vec2(distanceAlong - currentLength, 1.0f) });
	//	vertices.push_back({ down0, Vec2(distanceAlong, 0.0f) });
	//	vertices.push_back({ down1, Vec2(distanceAlong, 1.0f) });*/
	//};
	//

//	static float width = 0.1f;
//	ImGui::SliderFloat("width", &width, 0.0f, 1.0f);
//	Vec2 previousUp1(0.0f);
//	Vec2 previousDown1(0.0f);
//	static bool a = true;
//	ImGui::Checkbox("a", &a);
//
//	static bool b = true;
//	ImGui::Checkbox("b", &b);
//
//	static bool color = true;
//	ImGui::Checkbox("color", &color);
//
//	static bool c = true;
//	ImGui::Checkbox("c", &c);
//
//	PtVertex nextSegmentUp0;
//	PtVertex nextSegmentDown0;
//	for (i32 i = 0; i < static_cast<i32>(line.size()) - 1; i++) {
//		const auto current = line[i];
//		const auto next = line[i + 1];
//		const auto currentToNext = (next - current);
//		const auto normalToLine0 = currentToNext.rotBy90deg().normalized();
//		currentLength = currentToNext.length();
//		const auto up0 = current + normalToLine0 * width;
//		auto up1 = up0 + currentToNext;
//		const auto down0 = current - normalToLine0 * width;
//		auto down1 = down0 + currentToNext;
//		if (i == 0) {
//			nextSegmentUp0 = { up0, Vec2(0.0f) };
//			nextSegmentDown0 = { down0, Vec2(0.0f) };
//		}
//
//		if (i == line.size() - 2) {
//			distanceAlong += currentLength;
//			addQuad(nextSegmentUp0, PtVertex{ up1, Vec2{ distanceAlong, 1.0f } }, nextSegmentDown0, PtVertex{ down1, Vec2{ distanceAlong, 0.0f } });
//			/*addQuad(previousUp1, previousDown1, up1, down1, currentLength + distanceAlong, currentLength + distanceAlong);*/
//			break;
//		}
//
//		const auto nextNext = line[i + 2];
//		const auto nextToNextNext = (nextNext - next);
//		const auto normalToLine1 = nextToNextNext.rotBy90deg().normalized();
//
//		const auto nextUp0 = next + normalToLine1 * width;
//		const auto nextUp1 = nextUp0 + nextToNextNext;
//		const auto nextDown0 = next - normalToLine1 * width;
//		const auto nextDown1 = nextDown0 + nextToNextNext;
//
//		auto roundJoin = [&vertices](PtVertex start, Vec2 end, Vec2 roundingCenter, PtVertex segmentsIntersection, int count) -> float {
//			//float sub = 0.5;
//			/*if (aroundTextureY == 1.0f) {
//				sub = -sub;
//			}*/
//			/*vertices.push_back({ start, Vec2(distanceAlong, startTextureY) });
//			vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
//			vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });
//			vertices.push_back({ end, Vec2(0.0f, startTextureY) });
//			vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
//			vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });*/
//			/*vertices.push_back({ end, Vec2(0.0f, startTextureY) });
//			vertices.push_back({ around, Vec2(0.0f, aroundTextureY) });
//			vertices.push_back({ p, Vec2(0.0f, startTextureY) });*/
//			const auto roundingCircleRadiusVector = start.pos - roundingCenter;
//			auto startAngle = (roundingCircleRadiusVector).angle();
//			auto endAngle = (end - roundingCenter).angle();
//			auto angleRange = std::abs(endAngle - startAngle);
//			float angleStep;
//			if (angleRange > PI<float>) {
//				angleRange = TAU<float> -angleRange;
//				angleStep = -(angleRange / count);
//			} else {
//				angleStep = (angleRange / count);
//			}
//
//			if (startAngle > endAngle) {
//				angleStep = -angleStep;
//			}
//
//			Rotation rotationStep(angleRange / count);
//			//auto along = distanceAlong + currentLength;
//			const auto roundingRadius = roundingCircleRadiusVector.length();
//			const auto step = std::abs(angleStep) * roundingRadius;
//			for (int i = 0; i < count; i++) {
//				/*vertices.push_back({ around + Vec2::oriented(startAngle + i * angleStep) * length, Vec2(along, startTextureY) });
//				vertices.push_back({ around, Vec2(along, aroundTextureY + sub) });
//				vertices.push_back({ around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length, Vec2(along + step , startTextureY) });
//				along += step;*/
//			}
//			return step * count;
//		};
//
//		//auto roundJoin = [&](Vec2 start, Vec2 end, Vec2 around, int count, Vec2 p, float startTextureY, float aroundTextureY) -> float {\
//		//	float sub = 0.5;
//		//	if (aroundTextureY == 1.0f) {
//		//		sub = -sub;
//		//	}
//		//	vertices.push_back({ start, Vec2(distanceAlong, startTextureY) });
//		//	vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
//		//	vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });
//		//	vertices.push_back({ end, Vec2(0.0f, startTextureY) });
//		//	vertices.push_back({ around, Vec2(0.0f, aroundTextureY + sub) });
//		//	vertices.push_back({ p, Vec2(0.0f, aroundTextureY) });
//		//	/*vertices.push_back({ end, Vec2(0.0f, startTextureY) });
//		//	vertices.push_back({ around, Vec2(0.0f, aroundTextureY) });
//		//	vertices.push_back({ p, Vec2(0.0f, startTextureY) });*/
//		//	auto startAngle = (start - around).angle();
//		//	auto endAngle = (end - around).angle();
//		//	auto angleRange = std::abs(endAngle - startAngle);
//		//	auto angleStep = angleRange / count;
//		//	if (angleRange > PI<float>) {
//		//		//Debug::drawCircle(around, 0.02f);
//		//		angleRange = TAU<float> - angleRange;
//		//		//std::swap(startAngle, endAngle);
//		//		angleStep = -(angleRange / count);
//		//	}
//		//	if (startAngle > endAngle) {
//		//		angleStep = -angleStep;
//		//	}
//
//		//	if (abs(endAngle - (startAngle + angleStep * count)) > 0.01f) {
// 	//			int x = 5;
//		//	}
//
//		//	Rotation rotationStep(angleRange / count);
//		//	Vec2 previous = start;
//		//	Vec2 r = start - around;
//		//	auto along = distanceAlong + currentLength;
//		//	const auto length = (around - start).length();
//		//	const auto step = std::abs(angleStep) * length;
//		//	for (int i = 0; i < count; i++) {
//		//		vertices.push_back({ around + Vec2::oriented(startAngle + i * angleStep) * length, Vec2(along, startTextureY) });
//		//		vertices.push_back({ around, Vec2(along, aroundTextureY + sub) });
//		//		vertices.push_back({ around + Vec2::oriented(startAngle + (i + 1) * angleStep) * length, Vec2(along + step , startTextureY) });
//		//		along += step;
//		//	}
//		//	jointLength = step * count;
//		//};
//
//		/*Vec2 newUp1;
//		Vec2 newDown1;
//		Vec2 afterDown0;
//		Vec2 afterUp0;
//		float upTexturePosX;
//		float downTexturePosX;
//		jointLength = 0.0f;
//		upTexturePosX = currentLength + distanceAlong;
//		downTexturePosX = currentLength + distanceAlong;*/
//
//		distanceAlong += currentLength;
//		const auto intersectionUp = intersectLineSegments(up0, up1, nextUp0, nextUp1);
//		const auto intersectionDown = intersectLineSegments(down0, down1, nextDown0, nextDown1);
//		if (intersectionUp.has_value()) {
//			const auto shorten = (up0 - *intersectionUp) - (up0 - up1);
//			const auto upVertex = PtVertex{ *intersectionUp, Vec2(distanceAlong - shorten.length(), 1.0f)};
//			const auto downVertex = PtVertex{ down1, Vec2(distanceAlong, 0.0f) };
//			addQuad(nextSegmentUp0, upVertex, nextSegmentDown0, downVertex);
//			const auto jointLength = roundJoin(downVertex, nextDown0, next, upVertex, 5);
//			distanceAlong += jointLength;
//
//			/*nextSegmentUp0 = upVertex;
//			nextSegmentDown0 = PtVertex{ nextDown0, Vec2(distanceAlong, 0.0f) };*/
//			//roundJoin(down1, nextDown0, next, 5, *intersectionUp, 1.0f, 0.0f);
//// 
//			//nextSegmentUp0
//			/*newUp1 = *intersectionUp;
//			
//			afterUp0 = newUp1;
//			afterDown0 = nextDown0;
//			newDown1 = down1;*/
//
//			/*if (c) {
//				const auto shorten = std::abs((up0 - newUp1).length() - (up0 - up1).length());
//				upTexturePosX = currentLength + distanceAlong - shorten;
//				downTexturePosX = currentLength + distanceAlong;
//				previousUpTexturePosX = upTexturePosX;
//				previousDownTexturePosY = currentLength + distanceAlong + jointLength;
//			}*/
//
//		} /*else if (intersectionDown.has_value()) {
//			newDown1 = *intersectionDown;
//			newUp1 = up1;
//			roundJoin(up1, nextUp0, next, 5, newDown1, 0.0f, 1.0f);
//			afterDown0 = newDown1;
//			afterUp0 = nextUp0;
//
//			if (c) {
//				const auto shorten = std::abs((down0 - newDown1).length() - (down0 - down1).length());
//				upTexturePosX = currentLength + distanceAlong;
//				downTexturePosX = currentLength + distanceAlong - shorten;
//				previousDownTexturePosY = downTexturePosX;
//				previousUpTexturePosX = currentLength + distanceAlong + jointLength;
//			}
//
//		} else {
//			upTexturePosX = currentLength + distanceAlong;
//			downTexturePosX = currentLength + distanceAlong;
//			newUp1 = up1;
//			newDown1 = down1;
//			afterUp0 = nextUp0;
//			afterDown0 = nextDown0;
//			const auto v3io = Line(down0, down1).intersection(Line(nextDown0, nextDown1));
//			const auto v1io = Line(up0, up1).intersection(Line(nextUp0, nextUp1));
//			if (!v3io.has_value() || !v1io.has_value()) {
//				ImGui::Text("error");
//				continue;
//			}
//
//			if (signedDistance(Line(down1, down1 + normalToLine0), *v3io) < 0.0f) {
//				roundJoin(up1, nextUp0, next, 5, next, 0.0f, 0.0f);
//
//			} else {
//				roundJoin(down1, nextDown0, next, 5, next, 0.0f, 0.0f);
//			}
//		
//		}*/
//
//
//		/*if (a) {
//			if (i == 0) {
//				addQuad(up0, down0, newUp1, newDown1, upTexturePosX, downTexturePosX);
//			} else {
//				addQuad(previousUp1, previousDown1, newUp1, newDown1, upTexturePosX, downTexturePosX);
//			}
//		}
//		distanceAlong += currentLength;
//		distanceAlong += jointLength;
//
//		previousUp1 = afterUp0;
//		previousDown1 = afterDown0;*/
//
//	}
	//distanceAlong += currentLength;

	static LineTriangulator triangulate;
	auto output = triangulate(line);
	auto& vertices = output.vertices;
	auto& indices = output.indices;

	using namespace Clipper2Lib;
	PathsD polyline, solution;
	std::vector<float> linePath;
	for (const auto& p : line) {
		linePath.push_back(p.x);
		linePath.push_back(p.y);
	}
	polyline.push_back(MakePathD(linePath));
	// offset polyline
	solution = InflatePaths(polyline, 0.2f, JoinType::Round, EndType::Round);
	for (const auto& path : solution) {
		for (const auto& point : path) {
			//Debug::drawCircle(Vec2(point.x, point.y), 0.02f);
		}
	}
	vertices.clear();

	for (const auto& path : solution) {
		for (int i = 0; i < path.size() - 1; i++) {
			vertices.push_back(Vert{ Vec3(path[i].x, path[i].y, 0.0f), Vec2(0.0f) });
			vertices.push_back(Vert{ Vec3(path[i + 1].x, path[i + 1].y, 0.0f), Vec2(0.0f) });
			vertices.push_back(Vert{ Vec3(path[i].x, path[i].y, 0.0f), Vec2(0.0f) });
		}
		/*for (const auto& point : path) {
			Debug::drawCircle(Vec2(point.x, point.y), 0.02f);
		}*/
	}

	static bool color = true;
	ImGui::Checkbox("color", &color);

	const auto transform = makeTransform(Vec2(0.0f), 0.0f, Vec2(1.0f));
	for (auto& vertex : vertices) {
		auto pos = Vec2(vertex.pos.x, vertex.pos.y);
		pos *= transform;
		vertex.pos.x = pos.x;
		vertex.pos.y = pos.y;
		vertex.pos.z /= vertices.size();
		//vertex.texturePos.x /= distanceAlong;
	}
	vao.bind();
	vbo.allocateData(vertices.data(), vertices.size() * sizeof(Vertex));
	ibo.allocateData(indices.data(), indices.size() * sizeof(indices[0]));
	ibo.bind();
	static ShaderProgram& lineShader = createShader("./client/Shaders/line.vert", "./client/Shaders/line.frag");
	lineShader.use();
	lineShader.set("transform", Mat3x2::identity);
	lineShader.set("color", color);
	ImGui::Text("aspect %g", camera.aspectRatio);
	static bool test = true;
	ImGui::Checkbox("test", &test);
	if (test) glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//glDrawElements(GL_TRIANGLES, vertices.size(), GL_UNSIGNED_INT, nullptr);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
	/*glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	*/
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

	
 // polyline.push_back(MakePathD({100,100, 1500,100, 100,1500, 1500,1500}));
	//// offset polyline
 // solution = InflatePaths(polyline, 200, JoinType::Miter, EndType::Square);

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
