cmake_minimum_required(VERSION 3.23 FATAL_ERROR)
project(CollatzConjectureSimulator VERSION 2.1.4 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
	message(WARNING "Build variant not specified - defaulting to Debug")
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(external)
add_subdirectory(shaders)
add_subdirectory(src)
