project(server)

add_executable(server "main.cpp" "GameServer.cpp" "../shared/Networking.cpp" "../shared/Gameplay.cpp" "GameServerAdapter.cpp")
target_include_directories(server PUBLIC "." "../dependencies" "../")
target_link_libraries(server "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/yojimbo.lib")