cmake_minimum_required(VERSION 3.21)
project(CollatzConjectureSimulator VERSION 1.1 LANGUAGES C)

add_subdirectory(src)

find_program(glslc_exe NAMES glslc REQUIRED)
add_custom_target(
	Shaders
	ALL
	DEPENDS
		${CMAKE_SOURCE_DIR}/bin/shader.spv
		${CMAKE_SOURCE_DIR}/bin/shader64.spv
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/bin/shader.spv
	COMMAND
		${glslc_exe}
		--target-env=vulkan1.2 -O
		-o ${CMAKE_SOURCE_DIR}/bin/shader.spv
		${CMAKE_SOURCE_DIR}/src/shader.comp
	DEPENDS ${CMAKE_SOURCE_DIR}/src/shader.comp
	VERBATIM
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/bin/shader64.spv
	COMMAND
		${glslc_exe}
		--target-env=vulkan1.2 -O
		-o ${CMAKE_SOURCE_DIR}/bin/shader64.spv
		${CMAKE_SOURCE_DIR}/src/shader64.comp
	DEPENDS ${CMAKE_SOURCE_DIR}/src/shader64.comp
	VERBATIM
)
