project(replayTool)

add_executable(replayTool "main.cpp" "DebugDraw.cpp" "../shared/ReplayRecorder.cpp" "../generated/ReplayDataGenerated.cpp" "../generated/GameplayStateDataGenerated.cpp" "../engine/Utils/Json.cpp" "../engine/Utils/FileIo.cpp" "../engine/Log/Log.cpp"
	"../dependencies/imgui/imgui.cpp"
	"../dependencies/imgui/imgui_draw.cpp"
	"../dependencies/imgui/imgui_impl_win32.cpp"
	"../dependencies/imgui/imgui_tables.cpp"
	"../dependencies/imgui/imgui_widgets.cpp"
	"../dependencies/imgui/imgui_impl_opengl3.cpp"
	"../dependencies/imgui/imgui_impl_opengl3.h" 
	"../dependencies/imgui/imgui_demo.cpp"
 "Init.cpp")

target_sources(replayTool PUBLIC ${SHARED_GENERATED_FILES})

target_include_directories(replayTool PUBLIC "." "../dependencies" "../")

target_link_libraries(replayTool math)
target_link_libraries(replayTool json)

target_link_libraries(replayTool "${CMAKE_CURRENT_SOURCE_DIR}/../dependencies/raylib/raylib.lib")
target_link_libraries(replayTool "winmm.lib")
target_link_libraries(replayTool "kernel32.lib")
target_link_libraries(replayTool "opengl32.lib")
target_link_libraries(replayTool "gdi32.lib")
