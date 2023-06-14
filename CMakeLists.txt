cmake_minimum_required(VERSION 3.15)
# set(CMAKE_VERBOSE_MAKEFILE ON)
project(client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# if linking errors change option
# googletest\googletest\CMakeLists.txt


# if(MSVC)
#   add_compile_options(/W4)
# else()
#   add_compile_options(-Wall -Wextra -Wpedantic)
# endif()

# could use git submodule to for dependencies.
include_directories(
	client
	dependencies/glad/include
	dependencies/glfw/include
	dependencies/stb_image/include
	dependencies/imgui
	dependencies
	"."
)

file(GLOB DATA_FILES ${PROJECT_SOURCE_DIR}/client/*.data)

set_property(
  DIRECTORY 
  APPEND 
  PROPERTY CMAKE_CONFIGURE_DEPENDS 
  ${DATA_FILES}
)
# "generated/cmake.txt"

set(GENERATED_PATH ${CMAKE_CURRENT_SOURCE_DIR}/generated)
include("generated/cmake.txt")

execute_process(
	COMMAND generateCmakeDependencyList.bat
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# https://cmake.org/cmake/help/latest/command/add_custom_command.html
add_custom_command(
	OUTPUT ${GENERATED_FILES}
	COMMAND runCodeGenTool.bat
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
	DEPENDS ${DEPENDENCIES_FILES}
)

add_subdirectory(client)
# Working directory can be set by clicking debug > debug and launch settings for <target> and settings "currentDir": "${projectDir}"
# Tasks https://learn.microsoft.com/en-us/visualstudio/ide/customize-build-and-debug-tasks-in-visual-studio?view=vs-2022
# https://learn.microsoft.com/en-us/cpp/build/tasks-vs-json-schema-reference-cpp?view=msvc-170
add_executable(client "client/main.cpp" "dependencies/stb_image/src/stb_image.c"
	"dependencies/imgui/imgui.cpp"
	"dependencies/imgui/imgui_draw.cpp"
	"dependencies/imgui/imgui_impl_glfw.cpp"
	"dependencies/imgui/imgui_tables.cpp"
	"dependencies/imgui/imgui_widgets.cpp"
	"dependencies/imgui/imgui_impl_opengl3.cpp"
	"dependencies/imgui/imgui_impl_opengl3.h" "shared/Gameplay.cpp" "shared/WindowsUtils.cpp" "dependencies/imgui/imgui_demo.cpp" 
	${GENERATED_FILES}
)

target_link_libraries(client client-lib)
target_include_directories(client PUBLIC "/dependencies")
target_link_libraries(client "${CMAKE_CURRENT_SOURCE_DIR}/dependencies/yojimbo.lib")

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(dependencies/glfw)

add_subdirectory(server)
add_subdirectory(shortcutTool)