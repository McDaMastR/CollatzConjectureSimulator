cmake_minimum_required(VERSION 3.21)
project(CollatzConjectureSimulator VERSION 2.0.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE)
	message(WARNING "Build variant not specified - defaulting to Debug")
	set(CMAKE_BUILD_TYPE Debug)
endif()

add_subdirectory(external)
add_subdirectory(shaders)
add_subdirectory(src)
