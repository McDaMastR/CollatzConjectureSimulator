cmake_minimum_required(VERSION 3.21)
project(CollatzConjectureSimulator VERSION 1.5 LANGUAGES C)

add_subdirectory(src)

set(INT_SIZE 128)

find_program(glslc_exe NAMES glslc REQUIRED)
add_custom_target(
	Shaders
	ALL
	DEPENDS
		${CMAKE_SOURCE_DIR}/bin/shader_16_64.spv
		${CMAKE_SOURCE_DIR}/bin/shader_16.spv
		${CMAKE_SOURCE_DIR}/bin/shader_64.spv
		${CMAKE_SOURCE_DIR}/bin/shader.spv
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/bin/shader_16_64.spv
	COMMAND
		${glslc_exe}
		--target-env=vulkan1.1 -O
		-DINT_SIZE=${INT_SIZE}
		-o ${CMAKE_SOURCE_DIR}/bin/shader_16_64.spv
		${CMAKE_SOURCE_DIR}/src/shader_16_64.comp
	DEPENDS ${CMAKE_SOURCE_DIR}/src/shader_16_64.comp
	VERBATIM
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/bin/shader_16.spv
	COMMAND
		${glslc_exe}
		--target-env=vulkan1.1 -O
		-DINT_SIZE=${INT_SIZE}
		-o ${CMAKE_SOURCE_DIR}/bin/shader_16.spv
		${CMAKE_SOURCE_DIR}/src/shader_16.comp
	DEPENDS ${CMAKE_SOURCE_DIR}/src/shader_16.comp
	VERBATIM
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/bin/shader_64.spv
	COMMAND
		${glslc_exe}
		--target-env=vulkan1.1 -O
		-DINT_SIZE=${INT_SIZE}
		-o ${CMAKE_SOURCE_DIR}/bin/shader_64.spv
		${CMAKE_SOURCE_DIR}/src/shader_64.comp
	DEPENDS ${CMAKE_SOURCE_DIR}/src/shader_64.comp
	VERBATIM
)

add_custom_command(
	OUTPUT ${CMAKE_SOURCE_DIR}/bin/shader.spv
	COMMAND
		${glslc_exe}
		--target-env=vulkan1.1 -O
		-DINT_SIZE=${INT_SIZE}
		-o ${CMAKE_SOURCE_DIR}/bin/shader.spv
		${CMAKE_SOURCE_DIR}/src/shader.comp
	DEPENDS ${CMAKE_SOURCE_DIR}/src/shader.comp
	VERBATIM
)
