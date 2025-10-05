cmake_minimum_required(VERSION 3.24...3.31 FATAL_ERROR)

project(CollatzConjectureSimulator
	VERSION 0.2.2.1
	DESCRIPTION "Find the longest total stopping times"
	HOMEPAGE_URL "https://github.com/McDaMastR/CollatzConjectureSimulator"
	LANGUAGES C
)

if(NOT CMAKE_BUILD_TYPE)
	message(WARNING "cltz> Build variant not specified - defaulting to Debug")
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(lib)
add_subdirectory(shader)

set(USING_APPLE  "$<BOOL:${APPLE}>")
set(USING_CYGWIN "$<BOOL:${CYGWIN}>")
set(USING_MINGW  "$<BOOL:${MINGW}>")
set(USING_UNIX   "$<BOOL:${UNIX}>")
set(USING_WIN32  "$<BOOL:${WIN32}>")

add_subdirectory(common)
add_subdirectory(dynamic)
add_subdirectory(src)
