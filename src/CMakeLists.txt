add_executable(cltz)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(EXCESS_WARNINGS "Whether to compile the program with potentially excessive warnings"   OFF)
option(STATIC_ANALYSIS "Whether to perform static analysis on the program during compilation" OFF)

set(USING_EXCESS_WARNINGS "$<BOOL:${EXCESS_WARNINGS}>")
set(USING_STATIC_ANALYSIS "$<BOOL:${STATIC_ANALYSIS}>")

set(USING_MINGW "$<BOOL:${MINGW}>")

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
	-Weverything                         -Wno-covered-switch-default
	-Wno-declaration-after-statement     -Wno-gnu-conditional-omitted-operand
	-Wno-implicit-int-float-conversion   -Wno-missing-field-initializers
	-Wno-padded                          -Wno-pre-c11-compat
	-Wno-switch-enum                     -Wno-unsafe-buffer-usage
)

set_target_properties(
	cltz

	PROPERTIES

	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS ON

	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_NAME_DEBUG cltz-dbg
)

target_compile_definitions(cltz
	PRIVATE
		CLTZ_NAME="Collatz Conjecture Simulator"
		CLTZ_EXECUTABLE="$<TARGET_FILE_NAME:cltz>"

		CLTZ_VERSION_MAJOR=${CollatzConjectureSimulator_VERSION_MAJOR}
		CLTZ_VERSION_MINOR=${CollatzConjectureSimulator_VERSION_MINOR}
		CLTZ_VERSION_PATCH=${CollatzConjectureSimulator_VERSION_PATCH}

		"$<$<NOT:$<CONFIG:DEBUG>>:NDEBUG=1>"

		#"$<$<AND:${USING_EXCESS_WARNINGS},${USING_MINGW}>:__MINGW_MSVC_COMPAT_WARNINGS=1>"
		"$<$<OR:$<C_COMPILER_ID:MSVC>,${USING_MINGW}>:_CRT_SECURE_NO_WARNINGS=1>"
)

target_compile_options(cltz
	PRIVATE
		"$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:DEBUG>>:-Og;-g>"
		"$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:RELEASE>>:-O3>"
		"$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:MINSIZEREL>>:-Os>"
		"$<$<AND:$<C_COMPILER_ID:GNU,Clang>,$<CONFIG:RELWITHDEBINFO>>:-O2;-g>"

		"$<$<AND:${USING_EXCESS_WARNINGS},$<C_COMPILER_ID:GNU>>:${COMPILE_WARNINGS_GNU}>"
		"$<$<AND:${USING_EXCESS_WARNINGS},$<C_COMPILER_ID:Clang>>:${COMPILE_WARNINGS_CLANG}>"
		"$<$<AND:${USING_STATIC_ANALYSIS},$<C_COMPILER_ID:GNU>>:-fanalyzer>"
)

target_link_libraries(cltz
	PRIVATE
		volk::volk
		Vulkan::UtilityHeaders
		Threads::Threads
)

target_precompile_headers(cltz
	PRIVATE
		<volk.h>
		<vulkan/vk_enum_string_helper.h>
		<pthread.h>

		<inttypes.h>
		<limits.h>
		<signal.h>
		<string.h>
		<time.h>

		<stdalign.h>
		<stdarg.h>
		<stdatomic.h>
		<stdbool.h>
		<stddef.h>
		<stdint.h>
		<stdio.h>
		<stdlib.h>

		"$<$<OR:$<C_COMPILER_ID:GNU,Clang>,${USING_MINGW}>:<unistd.h$<ANGLE-R>>"
		"$<$<OR:$<C_COMPILER_ID:MSVC>,${USING_MINGW}>:<intrin.h$<ANGLE-R>;<windows.h$<ANGLE-R>>"
)

target_sources(cltz
	PRIVATE
		cli.c
		defs.c
		debug.c
		dyarray.c
		dyqueue.c
		dystring.c
		gpu.c
		main.c
		util.c
	PRIVATE FILE_SET HEADERS FILES
		cli.h
		defs.h
		debug.h
		dyarray.h
		dyqueue.h
		dystring.h
		gpu.h
		util.h
)
