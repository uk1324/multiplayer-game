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
		set(OUT_HPP, "${DATA_FILE_DIRECTORY}/${DATA_FILE_NAME}Data.hpp")
		#list(APPEND GENERATED_FILES)
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
	message("data files ${GENERATED_FILES}")
endmacro()

macro(targetAddGenerated2 targetName directoryToScan)
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
		set(OUT_HPP, "${DATA_FILE_DIRECTORY}/${DATA_FILE_NAME}Data.hpp")
		#list(APPEND GENERATED_FILES)
		add_custom_target("sharedTarget${DATA_FILE_NAME}"
			COMMAND ${CODEGEN_TOOL_PATH} ${DATA_FILE} ${GENERATED_PATH}
			WORKING_DIRECTORY ${CODEGEN_TOOL_WORKING_DIRECTORY}
			DEPENDS ${DATA_FILE}
		)
		add_dependencies(${targetName} "sharedTarget${DATA_FILE_NAME}")
	endforeach(DATA_FILE)
	target_sources(${targetName} PUBLIC ${GENERATED_FILES})
	message("data files ${GENERATED_FILES}")
endmacro()