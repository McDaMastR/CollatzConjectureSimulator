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

macro(compile_intermediate INPUT INTERMEDIATE SPV_MAJOR SPV_MINOR ENTRY_POINT)
	add_custom_command(
		OUTPUT ${INTERMEDIATE}
		COMMAND ${GLSLANG_EXE} --target-env vulkan1.3 --target-env spirv${SPV_MAJOR}.${SPV_MINOR} -DSPV_VER=${SPV_MAJOR}${SPV_MINOR} "$<IF:${USING_DEBUG_INFO},-gV,-g0;--quiet>" --sep main -e ${ENTRY_POINT} -o ${INTERMEDIATE} ${INPUT}
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
			COMMAND ${SPV_OPT_EXE} --target-env=spv${SPV_MAJOR}.${SPV_MINOR} "$<$<NOT:${USING_DEBUG_INFO}>:--strip-debug;--strip-nonsemantic>" --unify-const -O --compact-ids -o ${OUTPUT} ${OUTPUT}
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

add_custom_target(Shaders)

function(compile_shader SHADER_NAME)
	set(INPUT_128 ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_NAME}_128.comp)
	set(INPUT_256 ${CMAKE_CURRENT_SOURCE_DIR}/${SHADER_NAME}_256.comp)

	set(SPV_MAJOR 1)
	foreach(SPV_MINOR RANGE 3 6)
		set(SPV_VER v${SPV_MAJOR}${SPV_MINOR})

		set(INTERMEDIATE_128 ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}_128.spv)
		set(INTERMEDIATE_256 ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}_256.spv)
		set(INTERMEDIATES ${INTERMEDIATE_128} ${INTERMEDIATE_256})

		set(OUTPUT_SPV_${SPV_VER} ${CMAKE_SOURCE_DIR}/bin/${SPV_VER}/${SHADER_NAME}.spv)
		set(OUTPUT_ASM_${SPV_VER} ${CMAKE_SOURCE_DIR}/bin/${SPV_VER}/${SHADER_NAME}.spvasm)

		list(APPEND OUTPUT_SPV ${OUTPUT_SPV_${SPV_VER}})
		list(APPEND OUTPUT_ASM ${OUTPUT_ASM_${SPV_VER}})

		compile_intermediate(${INPUT_128} ${INTERMEDIATE_128} ${SPV_MAJOR} ${SPV_MINOR} main128)
		compile_intermediate(${INPUT_256} ${INTERMEDIATE_256} ${SPV_MAJOR} ${SPV_MINOR} main256)

		link_intermediates("${INTERMEDIATES}" ${OUTPUT_SPV_${SPV_VER}} ${SPV_MAJOR} ${SPV_MINOR})
		disassemble_output(${OUTPUT_SPV_${SPV_VER}} ${OUTPUT_ASM_${SPV_VER}})
	endforeach()

	add_custom_target(${SHADER_NAME} DEPENDS ${OUTPUT_SPV} "$<${USING_DISASSEMBLER}:${OUTPUT_ASM}>")
	add_dependencies(Shaders ${SHADER_NAME})
endfunction()

set(SHADERS shader shader_16 shader_64 shader_16_64)

foreach(SHADER_NAME IN LISTS SHADERS)
	compile_shader(${SHADER_NAME})
endforeach()
