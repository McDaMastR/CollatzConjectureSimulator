option(CZ_EXCESS_WARNINGS "Whether to compile the program with potentially excessive warnings" OFF)
option(CZ_STATIC_ANALYSIS "Whether to perform static analysis on the program during compilation" OFF)

set(USING_CYGWIN  "$<BOOL:${CYGWIN}>")
set(USING_DARWIN  "$<BOOL:${APPLE}>")
set(USING_MINGW   "$<BOOL:${MINGW}>")
set(USING_UNIX    "$<BOOL:${UNIX}>")
set(USING_WINDOWS "$<BOOL:${WIN32}>")

include(cmake/Definitions.cmake)
include(cmake/Headers.cmake)
include(cmake/Libraries.cmake)
include(cmake/Options.cmake)

function(configure TARGET)
	set_target_properties(${TARGET} PROPERTIES
		C_STANDARD 17
		C_STANDARD_REQUIRED ON
		C_EXTENSIONS ON
	)

	target_compile_definitions(${TARGET} PRIVATE ${MACRO_DEFINITIONS})
	target_compile_options(${TARGET} PRIVATE ${COMPILE_OPTIONS})
	target_link_libraries(${TARGET} PRIVATE ${LINK_LIBRARIES})
endfunction()

add_library(cltzPch OBJECT)
add_library(cltz::pch ALIAS cltzPch)

configure(cltzPch)
target_precompile_headers(cltzPch PUBLIC ${INCLUDE_HEADERS})
target_sources(cltzPch PRIVATE "${CMAKE_CURRENT_LIST_DIR}/fakepch.c")

function(cltz_configure TARGET)
	configure(${TARGET})
	target_precompile_headers(${TARGET} REUSE_FROM cltz::pch)
endfunction()
