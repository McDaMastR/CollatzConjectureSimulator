cmake_minimum_required(VERSION 3.24...3.31 FATAL_ERROR)

project(cltz
	VERSION 0.2.2.1
	DESCRIPTION "Find the longest total stopping times of Collatz sequences"
	HOMEPAGE_URL "https://github.com/McDaMastR/CollatzConjectureSimulator"
	LANGUAGES C
)

if(NOT CMAKE_BUILD_TYPE)
	message(WARNING "cltz> Build variant not specified - defaulting to Debug")
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(lib)
add_subdirectory(shader)

include(cmake/Config.cmake)

add_subdirectory(common)
add_subdirectory(dynamic)
add_subdirectory(src)
