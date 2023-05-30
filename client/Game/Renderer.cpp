#include <Game/Renderer.hpp>
#include <Engine/Window.hpp>
#include <Engine/Input/Input.hpp>
#include <shared/Math/utils.hpp>

struct PtVertex {
	Vec2 pos, texturePos;
};

static constexpr PtVertex fullscreenQuadVerts[]{
	{ Vec2{ -1.0f, 1.0f }, Vec2{ 0.0f, 0.0f } },
	{ Vec2{ 1.0f, 1.0f }, Vec2{ 1.0f, 0.0f } },
	{ Vec2{ -1.0f, -1.0f }, Vec2{ 0.0f, 1.0f } },
	{ Vec2{ 1.0f, -1.0f }, Vec2{ 1.0f, 1.0f } },
};

static constexpr u32 fullscreenQuadIndices[]{
	0, 1, 2, 2, 1, 3
};

Renderer::Renderer() 
	: fullscreenQuadPtVbo(fullscreenQuadVerts, sizeof(fullscreenQuadVerts))
	, fullscreenQuadPtIbo(fullscreenQuadIndices, sizeof(fullscreenQuadIndices))
	, texturedQuadPerInstanceDataVbo(sizeof(texturedQuadPerInstanceData))
	, atlasTexture(Texture::null()) {
	fullscreenQuadPtVao.bind();
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
	}
	{
		std::vector<std::string> paths;

		#define ADD_TO_ATLAS(name, filename) \
			const auto name##Path = "assets/textures/" filename; \
			paths.push_back(name##Path);
		
		#define MAKE_SPRITE(name) \
			{ \
				const auto pos = nameToPos[name##Path]; \
				name##Sprite = Sprite{ .offset = Vec2(pos.offset) / atlasTextureSize, .size = Vec2(pos.size) / atlasTextureSize }; \
			}

		ADD_TO_ATLAS(player, "player.png")
		ADD_TO_ATLAS(bullet, "bullet.png")
		ADD_TO_ATLAS(bullet2, "bullet2.png")

		auto [nameToPos, image] = generateTextureAtlas(paths);
		atlasTexture = Texture(image);
		const auto atlasTextureSize = Vec2(Vec2T<int>(image.width(), image.height()));
		
		MAKE_SPRITE(player);
		MAKE_SPRITE(bullet);
		MAKE_SPRITE(bullet2);

		#undef MAKE_SPRITE
		#undef ADD_TO_ATLAS
	}	
	{
		shader = &createShader("client/Game/shader.vert", "client/Game/shader.frag");
	}
}

void Renderer::update() {
	if (Input::isKeyHeld(KeyCode::F3) && Input::isKeyDown(KeyCode::T)) {
		reloadShaders();
	}

	glViewport(0, 0, Window::size().x, Window::size().y);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto aspectRatio = Window::aspectRatio();
	camera.aspectRatio = aspectRatio;
	const auto cameraTransform = camera.cameraTransform();
	const auto screenScale{ Mat3x2::scale(Vec2{ 1.0f, aspectRatio }) };
	auto makeTransform = [&screenScale, aspectRatio, &cameraTransform](Vec2 translation, float orientation, Vec2 scale) -> Mat3x2 {
		return Mat3x2::rotate(orientation) * screenScale * Mat3x2::scale(scale) * Mat3x2::translate(Vec2{ translation.x, translation.y * aspectRatio }) * cameraTransform;
	};

	//camera.moveOnWasd();

	{
		shader->use();
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		atlasTexture.bind();
		shader->setTexture("textureAtlas", 0);

		int toDraw = 0;
		fullscreenQuadPtIbo.bind();
		texturedQuadPerInstanceDataVbo.bind();
		for (int i = 0; i < spritesToDraw.size(); i++) {
			const auto& sprite = spritesToDraw[i];
			auto& quad = texturedQuadPerInstanceData[toDraw];
			const auto spriteAspectRatio = sprite.sprite.size.y / sprite.sprite.size.x;
			quad.transform = makeTransform(sprite.pos, sprite.rotation, Vec2(sprite.sprite.size.x, sprite.sprite.size.x * spriteAspectRatio));
			quad.atlasOffset = sprite.sprite.offset;
			quad.size = sprite.sprite.size;
			toDraw++;
			if (toDraw == std::size(texturedQuadPerInstanceData) || i == spritesToDraw.size() - 1) {
				Vbo::setData(0, texturedQuadPerInstanceData, toDraw * sizeof(TexturedQuadInstanceData));
				glDrawElementsInstanced(GL_TRIANGLES, std::size(fullscreenQuadIndices), GL_UNSIGNED_INT, nullptr, toDraw);
				toDraw = 0;
			}
		}
		spritesToDraw.clear();
	}

}

void Renderer::drawSprite(Sprite sprite, Vec2 pos, float size, float rotation) {
	spritesToDraw.push_back(SpriteToDraw{ sprite, pos, size, rotation });
}

ShaderProgram& Renderer::createShader(std::string_view vertPath, std::string fragPath) {
	shaders.push_back(ShaderEntry{ .vertPath = vertPath, .fragPath = fragPath, .program = ShaderProgram(vertPath, fragPath) });
	return shaders.back().program;
}

void Renderer::reloadShaders() {
	for (auto& entry : shaders) {
		entry.program = ShaderProgram(entry.vertPath, entry.fragPath);
	}
}
