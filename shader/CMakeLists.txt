add_custom_target(spirv)

find_program(GLSLANG_EXE NAMES glslang REQUIRED)
find_program(SPV_LNK_EXE NAMES spirv-link REQUIRED)
find_program(SPV_OPT_EXE NAMES spirv-opt)
find_program(SPV_DIS_EXE NAMES spirv-dis)

option(CLTZ_DEBUG_SHADERS       "Whether to compile shaders with debug and nonsemantic information" OFF)
option(CLTZ_OPTIMISE_SHADERS    "Whether to optimise the compiled shaders for improved performance" ON)
option(CLTZ_DISASSEMBLE_SHADERS "Whether to disassemble the compiled shaders into SPIR-V assembly" OFF)

set(USING_DEBUG_INFO   "$<BOOL:${CLTZ_DEBUG_SHADERS}>")
set(USING_OPTIMISER    "$<AND:$<BOOL:${CLTZ_OPTIMISE_SHADERS}>,$<BOOL:${SPV_OPT_EXE}>>")
set(USING_DISASSEMBLER "$<AND:$<BOOL:${CLTZ_DISASSEMBLE_SHADERS}>,$<BOOL:${SPV_DIS_EXE}>>")

if(CLTZ_OPTIMISE_SHADERS AND NOT SPV_OPT_EXE)
	message(WARNING "SPIR-V optimiser not found - shaders will not be optimised")
endif()

if(CLTZ_DISASSEMBLE_SHADERS AND NOT SPV_DIS_EXE)
	message(WARNING "SPIR-V disassembler not found - shaders will not be disassembled")
endif()

macro(compile_intermediate INPUT INTERMEDIATE SPV_MAJOR SPV_MINOR LITTLE_ENDIAN ITER_SIZE STO16 INT16 INT64)
	add_custom_command(
		OUTPUT
			${INTERMEDIATE}
		COMMAND
			${GLSLANG_EXE}
			--quiet
			--target-env vulkan1.4
			--target-env spirv${SPV_MAJOR}.${SPV_MINOR}
			"$<IF:${USING_DEBUG_INFO},-gV,-g0>"
			--define-macro SPV_VER=${SPV_MAJOR}${SPV_MINOR}
			--define-macro LITTLE_ENDIAN=${LITTLE_ENDIAN}
			--define-macro ITER_SIZE=${ITER_SIZE}
			--define-macro STO16=${STO16}
			--define-macro INT16=${INT16}
			--define-macro INT64=${INT64}
			--source-entrypoint main
			--entry-point main-${LITTLE_ENDIAN}-${ITER_SIZE}
			-o ${INTERMEDIATE}
			${INPUT}
		DEPENDS
			${INPUT}
		COMMAND_EXPAND_LISTS
		VERBATIM
	)
endmacro()

if(USING_OPTIMISER)
	macro(link_intermediates INTERMEDIATES OUTPUT SPV_MAJOR SPV_MINOR)
		add_custom_command(
			OUTPUT
				${OUTPUT}
			COMMAND
				${SPV_LNK_EXE}
				--target-env spv${SPV_MAJOR}.${SPV_MINOR}
				-o ${OUTPUT}
				${INTERMEDIATES}
			COMMAND
				${SPV_OPT_EXE}
				--target-env=spv${SPV_MAJOR}.${SPV_MINOR}
				"$<$<NOT:${USING_DEBUG_INFO}>:--strip-debug;--strip-nonsemantic>"
				--remove-unused-interface-variables
				-O
				--unify-const
				--trim-capabilities
				--compact-ids
				-o ${OUTPUT}
				${OUTPUT}
			DEPENDS
				${INTERMEDIATES}
			COMMAND_EXPAND_LISTS
			VERBATIM
		)
	endmacro()
else()
	macro(link_intermediates INTERMEDIATES OUTPUT SPV_MAJOR SPV_MINOR)
		add_custom_command(
			OUTPUT
				${OUTPUT}
			COMMAND
				${SPV_LNK_EXE}
				--target-env spv${SPV_MAJOR}.${SPV_MINOR}
				-o ${OUTPUT}
				${INTERMEDIATES}
			DEPENDS
				${INTERMEDIATES}
			COMMAND_EXPAND_LISTS
			VERBATIM
		)
	endmacro()
endif()

macro(disassemble_output INPUT OUTPUT)
	add_custom_command(
		OUTPUT
			${OUTPUT}
		COMMAND
			${SPV_DIS_EXE}
			--nested-indent
			--comment
			-o ${OUTPUT}
			${INPUT}
		DEPENDS
			${INPUT}
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

	set(OUTPUT_SPV ${CMAKE_SOURCE_DIR}/bin/${SPV_VER}/${SHADER_NAME}.spv)
	set(OUTPUT_ASM ${CMAKE_SOURCE_DIR}/bin/${SPV_VER}/${SHADER_NAME}.spvasm)

	set(ALL_SPV ${ALL_SPV} ${OUTPUT_SPV} PARENT_SCOPE)
	set(ALL_ASM ${ALL_ASM} ${OUTPUT_ASM} PARENT_SCOPE)

	foreach(LITTLE_ENDIAN 0 1)
		foreach(ITER_SIZE 128 256)
			set(INTERMEDIATE ${CMAKE_CURRENT_BINARY_DIR}/${SPV_VER}/${SHADER_NAME}-${LITTLE_ENDIAN}-${ITER_SIZE}.spv)
			list(APPEND INTERMEDIATES ${INTERMEDIATE})
			compile_intermediate(${INPUT_GLSL} ${INTERMEDIATE} ${SPV_MAJOR} ${SPV_MINOR} ${LITTLE_ENDIAN} ${ITER_SIZE} ${STO16} ${INT16} ${INT64})
		endforeach()
	endforeach()

	link_intermediates("${INTERMEDIATES}" ${OUTPUT_SPV} ${SPV_MAJOR} ${SPV_MINOR})
	disassemble_output(${OUTPUT_SPV} ${OUTPUT_ASM})
endfunction()

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

	add_custom_target(spirv-${SPV_MAJOR}-${SPV_MINOR} DEPENDS ${ALL_SPV} "$<${USING_DISASSEMBLER}:${ALL_ASM}>")
	add_dependencies(spirv spirv-${SPV_MAJOR}-${SPV_MINOR})

	unset(ALL_SPV)
	unset(ALL_ASM)
endforeach()
