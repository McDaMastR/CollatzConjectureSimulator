add_executable(${CMAKE_PROJECT_NAME})

set_target_properties(
	${CMAKE_PROJECT_NAME} PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Vulkan::Vulkan)
target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE Threads::Threads)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/inc)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE main.c gpu.c)

if(${CMAKE_BUILD_TYPE} MATCHES Debug)
	target_sources(${CMAKE_PROJECT_NAME} PRIVATE debug.c)

	target_compile_options(
		${CMAKE_PROJECT_NAME} PRIVATE
		-Og
		-Wall
		-Wextra
		-Wpedantic
		-Wshadow
		-Wstrict-prototypes
		-Wmissing-prototypes
		-Wmissing-declarations
		-Wredundant-decls
		-Wold-style-definition
		-Wpacked
		-Wbad-function-cast
		-Warith-conversion
		-Wcast-align
		-Wc++-compat
		-Winit-self
		-Wundef
		-Wunused-macros
		-Wunsafe-loop-optimizations
		-Wwrite-strings
		-Wformat=2
		-Wformat-signedness
		-Winline
		-Wmissing-include-dirs
		-Wlogical-op
		-Wdouble-promotion
		-Wshift-overflow=2
		-Wsync-nand
		-Wunused-const-variable=2
		-Walloc-zero
		-Wattribute-alias=2
		-Wduplicated-branches
		-Wduplicated-cond
		-Wtrampolines
		-Wnested-externs
		-Wcast-qual
	)
endif()