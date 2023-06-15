# https://stackoverflow.com/questions/4222326/cmake-compiling-generated-files/8748478#8748478
macro(targetAddGenerated targetName directoryToScan)
	set(GENERATED_FILES)
	file(GLOB_RECURSE DATA_FILES "${directoryToScan}/*.data")

	foreach(DATA_FILE ${DATA_FILES})
		get_filename_component(DATA_FILE_NAME ${DATA_FILE} NAME_WE)
		get_filename_component(DATA_FILE_DIRECTORY ${DATA_FILE} DIRECTORY)
		set(OUT_FILE "${GENERATED_PATH}/${DATA_FILE_NAME}DataGenerated.cpp")
		list(APPEND GENERATED_FILES ${OUT_FILE})
		set(OUT_HPP, "${DATA_FILE_DIRECTORY}/${DATA_FILE_NAME}Data.hpp")
		add_custom_command(
			PRE_BUILD 
			OUTPUT ${OUT_FILE} ${OUT_HPP}
			COMMAND ${CODEGEN_TOOL_PATH} ${DATA_FILE} ${GENERATED_PATH}
			WORKING_DIRECTORY ${CODEGEN_TOOL_WORKING_DIRECTORY}
			DEPENDS ${DATA_FILE}
			MAIN_DEPENDENCY ${DATA_FILE}
		)
		add_custom_target(
			"sharedTarget${DATA_FILE_NAME}"
			DEPENDS ${OUT_FILE} ${OUT_HPP}
		)
		add_dependencies(${targetName} "sharedTarget${DATA_FILE_NAME}")
	endforeach(DATA_FILE)
	target_sources(${targetName} PUBLIC ${GENERATED_FILES})
endmacro()