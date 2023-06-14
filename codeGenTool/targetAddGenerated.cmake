macro(targetAddGenerated targetName directoryToScan)
	set(GENERATED_FILES)
	file(GLOB_RECURSE DATA_FILES "${directoryToScan}/*.data")
	message("data files ${DATA_FILES}")
	message("directory to scan ${directoryToScan}")
	set_property(
		DIRECTORY
		APPEND 
		PROPERTY CMAKE_CONFIGURE_DEPENDS 
		"${generatedDirectory}/cmake.txt"
	)

	foreach(DATA_FILE ${DATA_FILES})
		get_filename_component(DATA_FILE_NAME ${DATA_FILE} NAME_WE)
		get_filename_component(DATA_FILE_DIRECTORY ${DATA_FILE} DIRECTORY)
		set(OUT_FILE "${GENERATED_PATH}/${DATA_FILE_NAME}DataGenerated.cpp")
		list(APPEND GENERATED_FILES ${OUT_FILE})
		list(APPEND GENERATED_FILES "${DATA_FILE_DIRECTORY}/${DATA_FILE_NAME}Data.hpp")
		add_custom_command(
			OUTPUT ${OUT_FILE}
			COMMAND ${CODEGEN_TOOL_PATH} ${DATA_FILE} ${GENERATED_PATH}
			WORKING_DIRECTORY ${CODEGEN_TOOL_WORKING_DIRECTORY}
			DEPENDS ${DATA_FILE}
		)
	endforeach(DATA_FILE)
	target_sources(${targetName} PUBLIC ${GENERATED_FILES})
	message("data files ${GENERATED_FILES}")
endmacro()