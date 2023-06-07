#include <Game/Renderer.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <shared/Math/utils.hpp>
#include <imgui/imgui.h>

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

Renderer::Renderer() 
	: fullscreenQuadPtVbo(fullscreenQuadVerts, sizeof(fullscreenQuadVerts))
	, fullscreenQuadPtIbo(fullscreenQuadIndices, sizeof(fullscreenQuadIndices))
	, texturedQuadPerInstanceDataVbo(sizeof(texturedQuadPerInstanceData))
	, atlasTexture(Texture::null()) {
	spriteVao.bind();
	{
		fullscreenQuadPtVbo.bind();
		Vao::setAttribute(0, BufferLayout(ShaderDataType::Float, 2, offsetof(PtVertex, pos), sizeof(PtVertex), false));
		Vao::setAttribute(1, BufferLayout(ShaderDataType::Float, 2, offsetof(PtVertex, texturePos), sizeof(PtVertex), false));
	}
	{
		texturedQuadPerInstanceDataVbo.bind();
		Vao::setAttribute(2, BufferLayout(ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, transform), sizeof(TexturedQuadInstanceData), false));
		glVertexAttribDivisor(2, 1);
		Vao::setAttribute(3, BufferLayout(ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, transform) + sizeof(Vec2), sizeof(TexturedQuadInstanceData), false));
		glVertexAttribDivisor(3, 1);
		Vao::setAttribute(4, BufferLayout(ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, transform) + sizeof(Vec2) * 2, sizeof(TexturedQuadInstanceData), false));
		glVertexAttribDivisor(4, 1);

		Vao::setAttribute(5, BufferLayout(ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, atlasOffset), sizeof(TexturedQuadInstanceData), false));
		glVertexAttribDivisor(5, 1);

		Vao::setAttribute(6, BufferLayout(ShaderDataType::Float, 2, offsetof(TexturedQuadInstanceData, size), sizeof(TexturedQuadInstanceData), false));
		glVertexAttribDivisor(6, 1);

		Vao::setAttribute(7, BufferLayout(ShaderDataType::Float, 4, offsetof(TexturedQuadInstanceData, color), sizeof(TexturedQuadInstanceData), false));
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
		spriteShader = &createShader("client/Game/shader.vert", "client/Game/shader.frag");
	}
	{
		backgroundShader = &createShader("client/Game/background.vert", "client/Game/background.frag");
	}
	{
		deathAnimationShader = &createShader("client/Game/quadShader.vert", "client/Game/deathAnimationShader.frag");
	}
	camera.zoom /= 3.0f;
}

void Renderer::update(Vec2 playerPos) {
	if (Input::isKeyHeld(KeyCode::F3) && Input::isKeyDown(KeyCode::T)) {
		reloadShaders();
	}

	glViewport(0, 0, Window::size().x, Window::size().y);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto aspectRatio = Window::aspectRatio();
	camera.aspectRatio = aspectRatio;
	camera.pos = playerPos;
	const auto cameraTransform = camera.cameraTransform();
	const auto screenScale{ Mat3x2::scale(Vec2{ 1.0f, aspectRatio }) };
	auto makeTransform = [&screenScale, aspectRatio, &cameraTransform](Vec2 translation, float orientation, Vec2 scale) -> Mat3x2 {
		return Mat3x2::rotate(orientation) * screenScale * Mat3x2::scale(scale) * Mat3x2::translate(Vec2{ translation.x, translation.y * aspectRatio }) * cameraTransform;
	};

	{
		backgroundShader->use();
		const auto cameraToWorld = (screenScale * cameraTransform).inversed();
		backgroundShader->setMat3x2("cameraToWorld", cameraToWorld);

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
		fullscreenQuadPtIbo.bind();
		//texturedQuadPerInstanceDataVbo.bind();
		for (int i = 0; i < spritesToDraw.size(); i++) {
			const auto& sprite = spritesToDraw[i];
			/*auto sprite = spritesToDraw[i];
			sprite.sprite = Sprite{ .offset = Vec2(0, 0), .size = Vec2(1, 1),.aspectRatio = 1.0f, };*/
			auto& quad = texturedQuadPerInstanceData[toDraw];
			quad.transform = makeTransform(sprite.pos, sprite.rotation, sprite.size * Vec2(1.0f, sprite.sprite.aspectRatio));
			quad.atlasOffset = sprite.sprite.offset;
			quad.size = sprite.sprite.size;
			quad.color = sprite.color;
			toDraw++;
			if (toDraw == std::size(texturedQuadPerInstanceData) || i == spritesToDraw.size() - 1) {
				Vbo::setData(0, texturedQuadPerInstanceData, toDraw * sizeof(TexturedQuadInstanceData));
				glDrawElementsInstanced(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr, toDraw);
				toDraw = 0;
			}
		}
		spritesToDraw.clear();
	}

	for (auto& animation : deathAnimations) {
		animation.t += 0.025f;
	}
	std::erase_if(deathAnimations, [](const DeathAnimation& animation) { return animation.t >= 1.0f; });

	ImGui::Text("%d", deathAnimations.size());

	{
		spriteVao.bind();
		deathAnimationShader->use();
		fullscreenQuadPtIbo.bind();
		for (auto& animation : deathAnimations) {
			ImGui::SliderFloat("t", &animation.t, 0.0f, 0.9999f);
			const auto transform = makeTransform(animation.position, 0.0f, Vec2{ 0.75f });
			deathAnimationShader->setMat3x2("transform", transform);
			deathAnimationShader->setFloat("time", animation.t);
			glDrawElements(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr);
		}
	}
}

void Renderer::drawSprite(Sprite sprite, Vec2 pos, float size, float rotation, Vec4 color) {
	spritesToDraw.push_back(SpriteToDraw{ sprite, pos, size, rotation, color });
}

void Renderer::playDeathAnimation(Vec2 position) {
	deathAnimations.push_back(DeathAnimation{
		.position = position
	});
}

ShaderProgram& Renderer::createShader(std::string_view vertPath, std::string_view fragPath) {
	shaders.push_back(ShaderEntry{ .vertPath = vertPath, .fragPath = fragPath, .program = ShaderProgram(vertPath, fragPath) });
	return shaders.back().program;
}

void Renderer::reloadShaders() {
	for (auto& entry : shaders) {
		entry.program = ShaderProgram(entry.vertPath, entry.fragPath);
	}
}
