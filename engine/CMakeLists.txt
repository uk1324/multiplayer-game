add_subdirectory(math)

add_library(
	engine 
	"../dependencies/stb_image/stb_image.cpp"
	"../dependencies/stb_image/stb_image_write.cpp"
	"../dependencies/stb_image/stb_image_resize.cpp"
	"Graphics/Shader.cpp" "Graphics/Shader.hpp" "Graphics/ShaderProgram.hpp" "Graphics/ShaderProgram.cpp" "Graphics/Texture.hpp" "Graphics/Texture.cpp" "Graphics/Vbo.hpp" "Graphics/Vbo.cpp" "Graphics/Vao.hpp" "Graphics/Vao.cpp" "Graphics/TextureArray.hpp" "Graphics/TextureArray.cpp" "Graphics/CubeMap.hpp" "Graphics/CubeMap.cpp" "Graphics/Ibo.cpp" "Graphics/Fbo.hpp" "Graphics/Fbo.cpp" "Graphics/TextureAtlasGenerator.cpp" "Input/Input.hpp" "Input/Input.cpp" "Window.hpp" "Window.cpp" "Engine.hpp"  "Engine.cpp"   "Input/MouseButton.hpp" "Log/Log.cpp" "Utils/Color32.cpp" "Utils/Image32.cpp" "Utils/Timer.hpp" "Utils/ByteBuffer.hpp" "Utils/ByteBuffer.cpp" "../shared/Networking.cpp" "Utils/Timer.cpp" "Utils/Gui.cpp"  "Utils/Json.cpp" "Utils/FileIo.hpp" "../engine/Utils/FileIo.cpp"
)

target_include_directories(
	engine PUBLIC
	"../dependencies/glad/include"
	"../dependencies/glfw/include"
	"../dependencies/stb_image/include"
	"../dependencies/imgui"
	"../dependencies"
	"../"
	"./Utils"
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

target_include_directories(engine PRIVATE "./")