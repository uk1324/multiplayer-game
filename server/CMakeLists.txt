project(server)

include("../codeGenTool/targetAddGenerated.cmake")

add_executable(server "main.cpp" "GameServer.cpp" "../shared/Networking.cpp" "../shared/WindowsUtils.cpp" "../shared/Gameplay.cpp" "GameServerAdapter.cpp")

#targetAddGenerated(server "${CMAKE_CURRENT_SOURCE_DIR}")
target_sources(server PUBLIC ${SHARED_GENERATED_FILES})

target_include_directories(server PUBLIC "." "../dependencies" "../")
target_link_libraries(server "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/yojimbo.lib")
target_link_libraries(server "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/raylib/raylib.lib")
target_link_libraries(server "winmm.lib")
target_link_libraries(server "kernel32.lib")
target_link_libraries(server "opengl32.lib")
target_link_libraries(server "gdi32.lib")