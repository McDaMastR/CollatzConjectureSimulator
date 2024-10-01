add_executable(CollatzSim)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set_target_properties(CollatzSim PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_NAME_DEBUG CollatzSim-Debug
)

target_sources(         CollatzSim PRIVATE main.c gpu.c util.c dyarray.c config.c debug.c)
target_link_libraries(  CollatzSim PRIVATE volk::volk Vulkan::UtilityHeaders Threads::Threads)
target_compile_features(CollatzSim PRIVATE c_std_11)

target_precompile_headers(CollatzSim PRIVATE
	<volk.h>
	<vulkan/vk_enum_string_helper.h>

	<stdatomic.h>
	<stdbool.h>
	<stdio.h>
	<stdlib.h>

	<pthread.h>
	<string.h>
)

set(COMPILE_WARNINGS_GNU
	-Wall                           -Wextra
	-Wpedantic                      -Walloc-zero
	-Walloca                        -Warith-conversion
	-Warray-bounds=2                -Wattribute-alias=2
	-Wbad-function-cast             -Wbidi-chars=any,ucn
	-Wc++-compat                    -Wc11-c23-compat
	-Wcast-align=strict             -Wcast-qual
	-Wdate-time                     -Wdisabled-optimization
	-Wdouble-promotion              -Wduplicated-branches
	-Wduplicated-cond               -Wflex-array-member-not-at-end
	-Wfloat-conversion              -Wfloat-equal
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
	-Wredundant-decls               -Wshadow
	-Wshift-overflow=2              -Wsign-conversion
	-Wstack-protector               -Wstrict-overflow=5
	-Wstrict-prototypes             -Wstringop-overflow=4
	-Wswitch-default                -Wsync-nand
	-Wtrampolines                   -Wtrivial-auto-var-init
	-Wundef                         -Wunsafe-loop-optimizations
	-Wunsuffixed-float-constants    -Wunused-const-variable=1
	-Wunused-macros                 -Wuse-after-free=3
	-Wvector-operation-performance  -Wwrite-strings
	-Wno-missing-field-initializers
)

set(COMPILE_WARNINGS_CLANG
	-Weverything                       -Wno-declaration-after-statement
	-Wno-implicit-int-float-conversion -Wno-missing-field-initializers
	-Wno-padded                        -Wno-switch-enum
	-Wno-unsafe-buffer-usage
)

if(${CMAKE_BUILD_TYPE} STREQUAL Debug)
	if(${CMAKE_C_COMPILER_ID} STREQUAL GNU)
		target_compile_options(CollatzSim PRIVATE -Og ${COMPILE_WARNINGS_GNU})
	elseif(${CMAKE_C_COMPILER_ID} STREQUAL Clang)
		target_compile_options(CollatzSim PRIVATE -Og ${COMPILE_WARNINGS_CLANG})
	endif()
endif()
