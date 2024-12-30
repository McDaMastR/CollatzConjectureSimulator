find_program(GLSLANG_EXE NAMES glslang    REQUIRED)
find_program(SPV_LNK_EXE NAMES spirv-link REQUIRED)
find_program(SPV_OPT_EXE NAMES spirv-opt)
find_program(SPV_DIS_EXE NAMES spirv-dis)

option(DEBUG_SHADERS       "Whether to compile shaders with debug and nonsemantic information" OFF)
option(OPTIMISE_SHADERS    "Whether to optimise the compiled shaders for improved performance" ON)
option(DISASSEMBLE_SHADERS "Whether to disassemble the compiled shaders into SPIR-V assembly"  OFF)

set(USING_DEBUG_INFO   "$<BOOL:${DEBUG_SHADERS}>")
set(USING_OPTIMISER    "$<AND:$<BOOL:${OPTIMISE_SHADERS}>,$<BOOL:${SPV_OPT_EXE}>>")
set(USING_DISASSEMBLER "$<AND:$<BOOL:${DISASSEMBLE_SHADERS}>,$<BOOL:${SPV_DIS_EXE}>>")

if(OPTIMISE_SHADERS AND NOT SPV_OPT_EXE)
	message(WARNING "SPIR-V optimiser not found - shaders will not be optimised")
endif()

if(DISASSEMBLE_SHADERS AND NOT SPV_DIS_EXE)
	message(WARNING "SPIR-V disassembler not found - shaders will not be disassembled")
endif()

macro(compile_intermediate INPUT INTERMEDIATE SPV_MAJOR SPV_MINOR LITTLE_ENDIAN ITER_SIZE STO16 INT16 INT64)
	add_custom_command(
		OUTPUT ${INTERMEDIATE}
		COMMAND
			${GLSLANG_EXE} --target-env vulkan1.3 --target-env spirv${SPV_MAJOR}.${SPV_MINOR} --quiet
			"$<IF:${USING_DEBUG_INFO},-gV,-g0>" -DSPV_VER=${SPV_MAJOR}${SPV_MINOR} -DLITTLE_ENDIAN=${LITTLE_ENDIAN}
			-DITER_SIZE=${ITER_SIZE} -DSTO16=${STO16} -DINT16=${INT16} -DINT64=${INT64} --sep main -e
			main-${LITTLE_ENDIAN}-${ITER_SIZE} -o ${INTERMEDIATE} ${INPUT}
		DEPENDS ${INPUT}
		COMMAND_EXPAND_LISTS
		VERBATIM
	)
endmacro()

if(USING_OPTIMISER)
	macro(link_intermediates INTERMEDIATES OUTPUT SPV_MAJOR SPV_MINOR)
		add_custom_command(
			OUTPUT ${OUTPUT}
			COMMAND ${SPV_LNK_EXE} --target-env spv${SPV_MAJOR}.${SPV_MINOR} -o ${OUTPUT} ${INTERMEDIATES}
			COMMAND
				${SPV_OPT_EXE} --target-env=spv${SPV_MAJOR}.${SPV_MINOR}
				"$<$<NOT:${USING_DEBUG_INFO}>:--strip-debug;--strip-nonsemantic>" --remove-unused-interface-variables -O
				--unify-const --trim-capabilities --compact-ids -o ${OUTPUT} ${OUTPUT}
			DEPENDS ${INTERMEDIATES}
			COMMAND_EXPAND_LISTS
			VERBATIM
		)
	endmacro()
else()
	macro(link_intermediates INTERMEDIATES OUTPUT SPV_MAJOR SPV_MINOR)
		add_custom_command(
			OUTPUT ${OUTPUT}
			COMMAND ${SPV_LNK_EXE} --target-env spv${SPV_MAJOR}.${SPV_MINOR} -o ${OUTPUT} ${INTERMEDIATES}
			DEPENDS ${INTERMEDIATES}
			COMMAND_EXPAND_LISTS
			VERBATIM
		)
	endmacro()
endif()

macro(disassemble_output INPUT OUTPUT)
	add_custom_command(
		OUTPUT ${OUTPUT}
		COMMAND ${SPV_DIS_EXE} --nested-indent --comment -o ${OUTPUT} ${INPUT}
		DEPENDS ${INPUT}
		COMMAND_EXPAND_LISTS
		VERBATIM
	)
endmacro()

function(compile_shader INPUT_GLSL SPV_MAJOR SPV_MINOR STO16 INT16 INT64)
	set(SPV_VER v${SPV_MAJOR}${SPV_MINOR})
	set(SHADER_NAME spirv)

	if(STO16)
		list(APPEND SHADER_NAME sto16)
	endif()
	if(INT16)
		list(APPEND SHADER_NAME int16)
	endif()
	if(INT64)
		list(APPEND SHADER_NAME int64)
	endif()

	list(JOIN SHADER_NAME - SHADER_NAME)

	set(INTERMEDIATE_0_128 ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}-0-128.spv)
	set(INTERMEDIATE_0_256 ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}-0-256.spv)
	set(INTERMEDIATE_1_128 ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}-1-128.spv)
	set(INTERMEDIATE_1_256 ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}-1-256.spv)
	set(INTERMEDIATES ${INTERMEDIATE_0_128} ${INTERMEDIATE_0_256} ${INTERMEDIATE_1_128} ${INTERMEDIATE_1_256})

	set(OUTPUT_SPV ${CMAKE_SOURCE_DIR}/bin/${SPV_VER}/${SHADER_NAME}.spv)
	set(OUTPUT_ASM ${CMAKE_SOURCE_DIR}/bin/${SPV_VER}/${SHADER_NAME}.spvasm)

	set(ALL_SPV ${ALL_SPV} ${OUTPUT_SPV} PARENT_SCOPE)
	set(ALL_ASM ${ALL_ASM} ${OUTPUT_ASM} PARENT_SCOPE)

	compile_intermediate(${INPUT_GLSL} ${INTERMEDIATE_0_128} ${SPV_MAJOR} ${SPV_MINOR} 0 128 ${STO16} ${INT16} ${INT64})
	compile_intermediate(${INPUT_GLSL} ${INTERMEDIATE_0_256} ${SPV_MAJOR} ${SPV_MINOR} 0 256 ${STO16} ${INT16} ${INT64})
	compile_intermediate(${INPUT_GLSL} ${INTERMEDIATE_1_128} ${SPV_MAJOR} ${SPV_MINOR} 1 128 ${STO16} ${INT16} ${INT64})
	compile_intermediate(${INPUT_GLSL} ${INTERMEDIATE_1_256} ${SPV_MAJOR} ${SPV_MINOR} 1 256 ${STO16} ${INT16} ${INT64})

	link_intermediates("${INTERMEDIATES}" ${OUTPUT_SPV} ${SPV_MAJOR} ${SPV_MINOR})
	disassemble_output(${OUTPUT_SPV} ${OUTPUT_ASM})
endfunction()

add_custom_target(Spirv)

set(INPUT_GLSL ${CMAKE_CURRENT_SOURCE_DIR}/shader.comp)
set(SPV_MAJOR 1)

foreach(SPV_MINOR RANGE 3 6)
	foreach(INT64 0 1)
		foreach(INT16 0 1)
			foreach(STO16 0 1)
				compile_shader(${INPUT_GLSL} ${SPV_MAJOR} ${SPV_MINOR} ${STO16} ${INT16} ${INT64})
			endforeach()
		endforeach()
	endforeach()

	add_custom_target(Spirv-${SPV_MAJOR}-${SPV_MINOR} DEPENDS ${ALL_SPV} "$<${USING_DISASSEMBLER}:${ALL_ASM}>")
	add_dependencies(Spirv Spirv-${SPV_MAJOR}-${SPV_MINOR})

	unset(ALL_SPV)
	unset(ALL_ASM)
endforeach()
