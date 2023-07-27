project(embedTool)
add_executable(embedTool "main.cpp" "../engine/Utils/PreprocessIncludes.cpp" "../engine/Utils/FileIo.cpp" "../engine/Log/Log.cpp")

target_link_libraries(embedTool json)

target_include_directories(embedTool PUBLIC "../" "../engine/Utils")