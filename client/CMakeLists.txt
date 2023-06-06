add_library(
	client-lib 
	"${PROJECT_SOURCE_DIR}/dependencies/glad/src/glad.c"
	"${PROJECT_SOURCE_DIR}/dependencies/stb_image/src/stb_image.c"
	"Json/JsonValue.cpp"
	"Json/JsonPrinter.cpp"
	"Json/JsonParser.cpp"
	"Log/Log.hpp" "Log/Log.cpp" "Engine/Window.hpp" "Engine/Window.cpp" "Json/Json.hpp" "Utils/FileIo.hpp" "Utils/FileIo.cpp"   "Log/Log.hpp" "Log/Log.cpp" "Engine/Engine.hpp" "Engine/Engine.cpp" "Engine/Time.cpp" "Engine/Time.hpp" "Engine/Engine.cpp"   "Engine/Graphics/Shader.cpp" "Engine/Graphics/Shader.hpp" "Engine/Graphics/ShaderProgram.hpp" "Engine/Graphics/ShaderProgram.cpp" "Engine/Input/MouseButton.hpp" "Engine/Graphics/Texture.hpp" "Engine/Graphics/Texture.cpp" "Engine/Graphics/Vbo.hpp"  "Engine/Graphics/Vbo.cpp" "Engine/Graphics/Vao.hpp" "Engine/Graphics/Vao.cpp" "Engine/Graphics/TextureArray.hpp" "Engine/Graphics/TextureArray.cpp" "Engine/Graphics/CubeMap.hpp" "Engine/Graphics/CubeMap.cpp" "Engine/Graphics/IndexBuffer.hpp" "Engine/Graphics/IndexBuffer.cpp" "Utils/TerminalColors.hpp" "../shared/Utils/Color32.cpp" "../shared/Utils/Image32.cpp" "Utils/Timer.hpp" "Engine/Graphics/Fbo.hpp" "Engine/Graphics/Fbo.cpp" "Utils/ByteBuffer.hpp" "Utils/ByteBuffer.cpp" "Utils/Span.hpp" "Game/GameClient.cpp" "Engine/Input/Input.hpp" "Engine/Input/Input.cpp" "../shared/Networking.cpp" "Game/GameClientAdapter.cpp" "Game/Renderer.cpp" "Game/Camera.cpp" "../shared/Math/Aabb.cpp" "Engine/Graphics/TextureAtlasGenerator.cpp")

target_link_libraries(client-lib glfw)