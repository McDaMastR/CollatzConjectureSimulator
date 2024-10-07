add_executable(CollatzSim)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(COMPILE_WARNINGS_GNU
	-Wall                     -Wextra
	-Walloc-zero              -Walloca
	-Warith-conversion        -Warray-bounds=2
	-Wattribute-alias=2       -Wbad-function-cast
	-Wbidi-chars=any,ucn      -Wc++-compat
	-Wc11-c23-compat          -Wcast-align=strict
	-Wcast-qual               -Wconversion
	-Wdate-time               -Wdisabled-optimization
	-Wdouble-promotion        -Wduplicated-branches
	-Wduplicated-cond         -Wflex-array-member-not-at-end
	-Wfloat-equal             -Wformat=2
	-Wformat-overflow=2       -Wformat-signedness
	-Wformat-truncation=2     -Wimplicit-fallthrough=5
	-Winit-self               -Winvalid-pch
	-Winvalid-utf8            -Wlogical-op
	-Wmissing-declarations    -Wmissing-format-attribute
	-Wmissing-include-dirs    -Wmissing-noreturn
	-Wmissing-prototypes      -Wmissing-variable-declarations
	-Wnested-externs          -Wnull-dereference
	-Wold-style-definition    -Wopenacc-parallelism
	-Wpacked                  -Wredundant-decls
	-Wshadow                  -Wshift-overflow=2
	-Wstack-protector         -Wstrict-overflow=5
	-Wstrict-prototypes       -Wstringop-overflow=4
	-Wswitch-default          -Wsync-nand
	-Wtrampolines             -Wtrivial-auto-var-init
	-Wundef                   -Wunsafe-loop-optimizations
	-Wunused-const-variable=2 -Wunused-macros
	-Wuse-after-free=3        -Wvector-operation-performance
	-Wwrite-strings           -Wno-missing-field-initializers
)

set(COMPILE_WARNINGS_CLANG
	-Weverything                         -Wno-declaration-after-statement
	-Wno-gnu-conditional-omitted-operand -Wno-implicit-int-float-conversion
	-Wno-missing-field-initializers      -Wno-padded
	-Wno-switch-enum                     -Wno-unsafe-buffer-usage
)

set_target_properties(
	CollatzSim

	PROPERTIES

	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS ON

	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_NAME_DEBUG CollatzSimDebug
)

target_compile_definitions(CollatzSim
	PRIVATE
		"$<$<CONFIG:RELEASE,MINSIZEREL,RELWITHDEBINFO>:NDEBUG=1>"
)

target_compile_options(CollatzSim
	PRIVATE
		"$<$<AND:$<C_COMPILER_ID:GNU,CLANG>,$<CONFIG:DEBUG>>:-Og;-g>"
		"$<$<AND:$<C_COMPILER_ID:GNU,CLANG>,$<CONFIG:RELEASE>>:-O3>"
		"$<$<AND:$<C_COMPILER_ID:GNU,CLANG>,$<CONFIG:MINSIZEREL>>:-Os>"
		"$<$<AND:$<C_COMPILER_ID:GNU,CLANG>,$<CONFIG:RELWITHDEBINFO>>:-O2;-g>"
		"$<$<AND:$<C_COMPILER_ID:GNU>,$<CONFIG:DEBUG,RELWITHDEBINFO>>:-fanalyzer>"
		"$<$<AND:$<C_COMPILER_ID:GNU>,$<CONFIG:DEBUG,RELWITHDEBINFO>>:${COMPILE_WARNINGS_GNU}>"
		"$<$<AND:$<C_COMPILER_ID:CLANG>,$<CONFIG:DEBUG,RELWITHDEBINFO>>:${COMPILE_WARNINGS_CLANG}>"
)

target_link_libraries(CollatzSim
	PRIVATE
		volk::volk
		Vulkan::UtilityHeaders
		Threads::Threads
)

target_precompile_headers(CollatzSim
	PRIVATE
		<volk.h>
		<vulkan/vk_enum_string_helper.h>
		<pthread.h>

		<stdatomic.h>
		<stdbool.h>
		<stdio.h>
		<stdlib.h>
		<string.h>
)

target_sources(CollatzSim
	PRIVATE
		main.c
		config.c
		debug.c
		dyarray.c
		gpu.c
		util.c
	PRIVATE FILE_SET HEADERS FILES
		config.h
		debug.h
		dyarray.h
		gpu.h
		util.h
)
