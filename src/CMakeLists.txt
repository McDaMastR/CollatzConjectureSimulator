add_executable(cltzExe)

set_target_properties(cltzExe PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS ON

	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
	RUNTIME_OUTPUT_NAME cltz
	RUNTIME_OUTPUT_NAME_DEBUG cltz-dbg
)

target_link_libraries(cltzExe
	PRIVATE
		cltzCommon
		cltzDynamic
)

target_sources(cltzExe
	PRIVATE
		cli.c
		config.c
		gpu.c
		main.c
	PRIVATE FILE_SET HEADERS FILES
		cli.h
		config.h
		gpu.h
)
