add_executable(${CMAKE_PROJECT_NAME})

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE main.c gpu.c debug.c)
target_compile_features(${CMAKE_PROJECT_NAME} PRIVATE c_std_11)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Vulkan::UtilityHeaders volk::volk Threads::Threads)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
	if(${CMAKE_C_COMPILER_ID} MATCHES GNU)
		target_compile_options(
			${CMAKE_PROJECT_NAME} PRIVATE
			-Og
			-Wall
			-Wextra
			-Wpedantic
			-Walloc-zero
			-Walloca
			-Warith-conversion
			-Warray-bounds=2
			-Wattribute-alias=2
			-Wbad-function-cast
			-Wbidi-chars=any,ucn
			-Wc++-compat
			-Wc11-c23-compat
			-Wcast-align=strict
			-Wcast-qual
			-Wdate-time
			-Wdisabled-optimization
			-Wdouble-promotion
			-Wduplicated-branches
			-Wduplicated-cond
			-Wflex-array-member-not-at-end
			-Wfloat-equal
			-Wformat=2
			-Wformat-overflow=2
			-Wformat-signedness
			-Wformat-truncation=2
			-Wimplicit-fallthrough=5
			-Winit-self
			-Winvalid-pch
			-Winvalid-utf8
			-Wlogical-op
			-Wmissing-declarations
			-Wmissing-format-attribute
			-Wmissing-include-dirs
			-Wmissing-noreturn
			-Wmissing-prototypes
			-Wmissing-variable-declarations
			-Wnested-externs
			-Wnull-dereference
			-Wold-style-definition
			-Wopenacc-parallelism
			-Wpacked
			-Wredundant-decls
			-Wshadow
			-Wshift-overflow=2
			-Wsign-conversion
			-Wstack-protector
			-Wstrict-overflow=5
			-Wstrict-prototypes
			-Wstringop-overflow=4
			-Wswitch-default
			-Wsync-nand
			-Wtrampolines
			-Wtrivial-auto-var-init
			-Wundef
			-Wunsafe-loop-optimizations
			-Wunsuffixed-float-constants
			-Wunused-const-variable=1
			-Wunused-macros
			-Wuse-after-free=3
			-Wvector-operation-performance
			-Wwrite-strings
		)
	elseif(${CMAKE_C_COMPILER_ID} MATCHES Clang)
		target_compile_options(
			${CMAKE_PROJECT_NAME} PRIVATE
			-Wall
			-Wextra
		)
	endif()
endif()
