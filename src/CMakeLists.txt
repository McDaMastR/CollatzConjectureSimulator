add_executable(cltzExe)

cltz_configure(cltzExe)

set_target_properties(cltzExe PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
	RUNTIME_OUTPUT_NAME cltz
	RUNTIME_OUTPUT_NAME_DEBUG cltz-dbg
)

target_link_libraries(cltzExe
	PRIVATE
		cltz::common
		cltz::dynamic
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
