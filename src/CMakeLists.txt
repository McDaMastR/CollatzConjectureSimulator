add_executable(cltz)

set_target_properties(
	cltz

	PROPERTIES

	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS ON

	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_NAME_DEBUG cltz-dbg
)

target_link_libraries(cltz
	PRIVATE
		cltzCommon
		cltzDynamic
)

target_sources(cltz
	PRIVATE
		cli.c
		gpu.c
		main.c
	PRIVATE FILE_SET HEADERS FILES
		cli.h
		gpu.h
)
