add_library(cltzCommon OBJECT)
add_library(cltz::common ALIAS cltzCommon)

include(CheckIncludeFile)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

option(CZ_EXCESS_WARNINGS "Whether to compile the program with potentially excessive warnings" OFF)
option(CZ_STATIC_ANALYSIS "Whether to perform static analysis on the program during compilation" OFF)

set(USING_EXCESS_WARNINGS "$<BOOL:${CZ_EXCESS_WARNINGS}>")
set(USING_STATIC_ANALYSIS "$<BOOL:${CZ_STATIC_ANALYSIS}>")

check_include_file(aio.h        HAVE_AIO_H)
check_include_file(stdcountof.h HAVE_STDCOUNTOF_H)
check_include_file(sys/mman.h   HAVE_SYS_MMAN_H)

set(USING_AIO_H        "$<BOOL:${HAVE_AIO_H}>")
set(USING_STDCOUNTOF_H "$<BOOL:${HAVE_STDCOUNTOF_H}>")
set(USING_SYS_MMAN_H   "$<BOOL:${HAVE_SYS_MMAN_H}>")

set(COMPILE_WARNINGS_CLANG
	# Warnings ON
	-Weverything

	# Warnings OFF
	-Wno-bad-function-cast             -Wno-c++-compat
	-Wno-c2y-extensions                -Wno-covered-switch-default
	-Wno-declaration-after-statement   -Wno-gnu-conditional-omitted-operand
	-Wno-implicit-int-float-conversion -Wno-missing-field-initializers
	-Wno-padded                        -Wno-switch-enum
	-Wno-unsafe-buffer-usage           -Wno-unused-function
)

set(COMPILE_WARNINGS_GNU
	# Warnings ON
	-Wall                           -Wextra
	-Walloc-zero                    -Walloca
	-Warray-bounds=2                -Wattribute-alias=2
	-Wbidi-chars=any,ucn            -Wc11-c23-compat
	-Wcast-align=strict             -Wcast-qual
	-Wconversion                    -Wdate-time
	-Wdisabled-optimization         -Wdouble-promotion
	-Wduplicated-branches           -Wduplicated-cond
	-Wflex-array-member-not-at-end  -Wfloat-equal
	-Wformat=2                      -Wformat-overflow=2
	-Wformat-signedness             -Wformat-truncation=2
	-Wimplicit-fallthrough=5        -Winit-self
	-Winvalid-pch                   -Winvalid-utf8
	-Wlogical-op                    -Wmissing-declarations
	-Wmissing-format-attribute      -Wmissing-include-dirs
	-Wmissing-noreturn              -Wmissing-prototypes
	-Wmissing-variable-declarations -Wnested-externs
	-Wnull-dereference              -Wold-style-definition
	-Wopenacc-parallelism           -Wpacked
	-Wpointer-arith                 -Wredundant-decls
	-Wshadow                        -Wshift-overflow=2
	-Wstack-protector               -Wstrict-overflow=5
	-Wstrict-prototypes             -Wstringop-overflow=4
	-Wswitch-default                -Wsync-nand
	-Wtrampolines                   -Wtrivial-auto-var-init
	-Wundef                         -Wunsafe-loop-optimizations
	-Wunused-const-variable=2       -Wunused-macros
	-Wuse-after-free=3              -Wvector-operation-performance
	-Wwrite-strings

	# Warnings OFF
	-Wno-missing-field-initializers -Wno-unused-function
)

set(COMPILE_WARNINGS_MSVC
	# Warnings ON
	/Wall

	# Warnings OFF
	/wd4061 # Don't need to have an explicit switch case for every possible value of an enum
	/wd4062 # Don't need to handle every possible value of an enum in a switch statement
	/wd4820 # Don't care about struct padding
)

set(MACRO_DEFINITIONS_APPLE
	_DARWIN_C_SOURCE=1
	_DARWIN_USE_64_BIT_INODE=1
)

set(MACRO_DEFINITIONS_MINGW
	$<${USING_EXCESS_WARNINGS}:__MINGW_MSVC_COMPAT_WARNINGS=1>
)

set(MACRO_DEFINITIONS_UNIX
	_FILE_OFFSET_BITS=64
	_TIME_BITS=64
)

set(MACRO_DEFINITIONS_WIN32
	_CRT_SECURE_NO_WARNINGS=1
)

set(LINK_LIBRARIES_APPLE
	$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>
	$<LINK_LIBRARY:FRAMEWORK,IOKit>
)

set(INCLUDE_HEADERS_APPLE
	<CoreFoundation/CoreFoundation.h$<ANGLE-R>
	<IOKit/pwr_mgt/IOPMLib.h$<ANGLE-R>
)

set(INCLUDE_HEADERS_UNIX
	<fcntl.h$<ANGLE-R>
	<unistd.h$<ANGLE-R>
	<sys/select.h$<ANGLE-R>
	<sys/stat.h$<ANGLE-R>
	<sys/syscall.h$<ANGLE-R>
	<sys/types.h$<ANGLE-R>
	<sys/uio.h$<ANGLE-R>
)

set(INCLUDE_HEADERS_WIN32
	<intrin.h$<ANGLE-R>
	<io.h$<ANGLE-R>
	<windows.h$<ANGLE-R>
)

set_target_properties(cltzCommon PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS ON
)

target_compile_definitions(cltzCommon
	PUBLIC
		CZ_NAME="Collatz Conjecture Simulator"
		CZ_EXECUTABLE="$<TARGET_FILE_NAME:cltzExe>"
		"CZ_COPYRIGHT=\"Copyright (C) 2025 Seth McDonald\"" # The '(C)' causes a syntax error without extra quotes
		CZ_LICENCE="Licence GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>"

		CZ_VERSION_MAJOR=${CollatzConjectureSimulator_VERSION_MAJOR}
		CZ_VERSION_MINOR=${CollatzConjectureSimulator_VERSION_MINOR}
		CZ_VERSION_PATCH=${CollatzConjectureSimulator_VERSION_PATCH}

		$<$<NOT:$<CONFIG:DEBUG>>:NDEBUG=1>
		$<${USING_APPLE}:${MACRO_DEFINITIONS_APPLE}>
		$<${USING_MINGW}:${MACRO_DEFINITIONS_MINGW}>
		$<${USING_UNIX}:${MACRO_DEFINITIONS_UNIX}>
		$<${USING_WIN32}:${MACRO_DEFINITIONS_WIN32}>
)

target_compile_options(cltzCommon
	PUBLIC
		$<$<C_COMPILER_ID:MSVC>:/Oi>

		$<$<AND:$<C_COMPILER_ID:AppleClang,Clang,GNU>,$<CONFIG:DEBUG>>:-Og;-g>
		$<$<AND:$<C_COMPILER_ID:AppleClang,Clang,GNU>,$<CONFIG:RELEASE>>:-O3>
		$<$<AND:$<C_COMPILER_ID:AppleClang,Clang,GNU>,$<CONFIG:MINSIZEREL>>:-Os>
		$<$<AND:$<C_COMPILER_ID:AppleClang,Clang,GNU>,$<CONFIG:RELWITHDEBINFO>>:-O2;-g>

		$<$<AND:${USING_EXCESS_WARNINGS},$<C_COMPILER_ID:AppleClang,Clang>>:${COMPILE_WARNINGS_CLANG}>
		$<$<AND:${USING_EXCESS_WARNINGS},$<C_COMPILER_ID:GNU>>:${COMPILE_WARNINGS_GNU}>
		$<$<AND:${USING_EXCESS_WARNINGS},$<C_COMPILER_ID:MSVC>>:${COMPILE_WARNINGS_MSVC}>

		$<$<AND:${USING_STATIC_ANALYSIS},$<C_COMPILER_ID:GNU>>:-fanalyzer>
		$<$<AND:${USING_STATIC_ANALYSIS},$<C_COMPILER_ID:MSVC>>:/analyze>
)

target_link_libraries(cltzCommon
	PUBLIC
		# Required libraries
		Threads::Threads
		cwalk::cwalk
		wai::wai

		Vulkan::Headers
		Vulkan::UtilityHeaders
		volk::volk

		# OS-specific libraries
		$<${USING_APPLE}:${LINK_LIBRARIES_APPLE}>
)

target_precompile_headers(cltzCommon
	PUBLIC
		# std*.h headers
		<stdalign.h>
		<stdarg.h>
		<stdatomic.h>
		<stdbool.h>
		<stddef.h>
		<stdint.h>
		<stdio.h>
		<stdlib.h>
		<stdnoreturn.h>

		# Other standard library headers
		<assert.h>
		<complex.h>
		<ctype.h>
		<errno.h>
		<fenv.h>
		<float.h>
		<inttypes.h>
		<limits.h>
		<locale.h>
		<math.h>
		<setjmp.h>
		<signal.h>
		<string.h>
		<time.h>
		# <uchar.h> # Not supported on Mac
		<wchar.h>
		<wctype.h>

		# Required libraries' headers
		<cwalk.h>
		<pthread.h>
		<volk.h>
		<vulkan/vk_enum_string_helper.h>
		<whereami.h>

		# Optional headers
		$<${USING_AIO_H}:<aio.h$<ANGLE-R>>
		$<${USING_STDCOUNTOF_H}:<stdcountof.h$<ANGLE-R>>
		$<${USING_SYS_MMAN_H}:<sys/mman.h$<ANGLE-R>>

		# OS-specific headers
		$<${USING_APPLE}:${INCLUDE_HEADERS_APPLE}>
		$<${USING_UNIX}:${INCLUDE_HEADERS_UNIX}>
		$<${USING_WIN32}:${INCLUDE_HEADERS_WIN32}>
)

target_sources(cltzCommon
	PRIVATE
		alloc.c
		debug.c
		file.c
		util.c
		wrap.c
	PUBLIC FILE_SET HEADERS FILES
		alloc.h
		common.h
		debug.h
		def.h
		file.h
		util.h
		wrap.h
)
