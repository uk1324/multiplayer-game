cmake_minimum_required(VERSION 3.15)
project(client)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CODEGEN_TOOL_PATH ./runCodegenTool.bat)
set(CODEGEN_TOOL_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
set(GENERATED_PATH "${CMAKE_CURRENT_SOURCE_DIR}/generated")
set_property(
	DIRECTORY
	APPEND 
	PROPERTY CMAKE_CONFIGURE_DEPENDS 
	"${GENERATED_PATH}/cmake.txt"
)

include("./codeGenTool/targetAddGenerated.cmake")

add_subdirectory("./dependencies/glfw")

add_subdirectory(engine)

add_subdirectory(client)

add_subdirectory(server)

add_subdirectory(shortcutTool)

# Working directory can be set by clicking debug > debug and launch settings for <target> and settings "currentDir": "${projectDir}"
# Tasks https://learn.microsoft.com/en-us/visualstudio/ide/customize-build-and-debug-tasks-in-visual-studio?view=vs-2022
# https://learn.microsoft.com/en-us/cpp/build/tasks-vs-json-schema-reference-cpp?view=msvc-170