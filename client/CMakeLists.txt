# https://stackoverflow.com/questions/55190247/how-do-i-do-edit-and-continue-builds-with-cmake-projects-in-visual-studio-2019
if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  set(CMAKE_CXX_FLAGS_DEBUG "/ZI")
  set(CMAKE_SHARED_LINKER_FLAGS "/SAFESEH:NO")
  set(CMAKE_EXE_LINKER_FLAGS "/SAFESEH:NO")
endif()

project(client)

add_definitions(-DCLIENT)
add_executable(
	client
	"main.cpp" 
	"../dependencies/imgui/imgui.cpp"
	"../dependencies/imgui/imgui_draw.cpp"
	"../dependencies/imgui/imgui_impl_glfw.cpp"
	"../dependencies/imgui/imgui_tables.cpp"
	"../dependencies/imgui/imgui_widgets.cpp"
	"../dependencies/imgui/imgui_impl_opengl3.cpp"
	"../dependencies/imgui/imgui_impl_opengl3.h" 
	"../dependencies/imgui/imgui_demo.cpp"
	"../dependencies/glad/src/glad.c"
	"../shared/Gameplay.cpp" 
	"../shared/WindowsUtils.cpp" 
	"../shared/PlayerIndex.cpp" 
	"GameClient.cpp" "GameClientAdapter.cpp" "Rendering/Renderer.cpp" "Camera.cpp" "MainLoop.cpp"
 "Debug.cpp" "Rendering/LineTriangulate.cpp" "Rendering/Font.cpp" "Rendering/ShaderManager.cpp" "GameplayContext.cpp")

if (FINAL_RELEASE)
	if (WIN32)
		set_target_properties(client PROPERTIES WIN32_EXECUTABLE TRUE)
	endif()
endif()

target_link_libraries(client engine)
target_link_libraries(client math)
target_link_libraries(client json)
target_link_libraries(client glfw)

targetAddGenerated(client "../shared")
set(SHARED_GENERATED_FILES, ${GENERATED_FILES})
targetAddGenerated(client "./")

target_link_libraries(client yojimboLib)
if (WIN32)
	target_link_libraries(client "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/freetype.lib")
else()
	target_link_libraries(client freetype)
endif()

if (FINAL_RELEASE)
	set(EMBED_TOOL "$<TARGET_FILE:embedTool>")
	add_custom_command(
		TARGET client PRE_BUILD
		COMMAND ${EMBED_TOOL}
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		DEPENDS ${EMBED_TOOL}
	)
	set(BUILD_TOOL "$<TARGET_FILE:buildTool>")
	set(targetName "client")
	add_custom_command(
		TARGET ${targetName} POST_BUILD
		COMMAND ${BUILD_TOOL} $<TARGET_FILE:client>
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		DEPENDS ${BUILD_TOOL}
	)
endif()