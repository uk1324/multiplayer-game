project(server)

include("../codeGenTool/targetAddGenerated.cmake")
add_definitions(-DSERVER)

add_executable(server "main.cpp" "GameServer.cpp" "../shared/Networking.cpp" "../shared/WindowsUtils.cpp" "../shared/Gameplay.cpp" "GameServerAdapter.cpp" "../engine/Utils/Put.cpp" "../replayTool/DebugDraw.cpp" "../shared/ReplayRecorder.cpp" "../generated/ReplayDataGenerated.cpp" "../generated/GameplayStateDataGenerated.cpp" "../engine/Utils/Json.cpp" "ServerGameplayContext.cpp" "../shared/PlayerIndex.cpp" "GetAddress.cpp"  "HttpServer/HttpServer.cpp" "Sockets.cpp" "../engine/Utils/StringStream.cpp" "../engine/Utils/Assertions.cpp")

targetAddGenerated(server "${CMAKE_CURRENT_SOURCE_DIR}")
target_sources(server PUBLIC ${SHARED_GENERATED_FILES})

target_include_directories(server PUBLIC "." "../dependencies" "../")

target_link_libraries(server math)
target_link_libraries(server json)

target_link_libraries(server yojimboLib)

if(WIN32)
    target_link_libraries(server "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/raylib/raylib.lib")
    target_link_libraries(server "winmm.lib")
    target_link_libraries(server "kernel32.lib")
    target_link_libraries(server "opengl32.lib")
    target_link_libraries(server "gdi32.lib")
endif()