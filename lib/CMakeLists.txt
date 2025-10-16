include(FetchContent)

# Vulkan
set(VULKAN_HEADERS_INSTALL_DIR           "" CACHE PATH "Path to the Vulkan Headers installation directory")
set(VULKAN_UTILITY_LIBRARIES_INSTALL_DIR "" CACHE PATH "Path to the Vulkan Utility Libraries installation directory")
set(VOLK_INSTALL_DIR                     "" CACHE PATH "Path to the volk installation directory")

if(VULKAN_HEADERS_INSTALL_DIR)
	list(PREPEND VulkanHeaders_ROOT ${VULKAN_HEADERS_INSTALL_DIR})
else()
	FetchContent_Declare(VulkanHeaders SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers OVERRIDE_FIND_PACKAGE)
endif()

if(VULKAN_UTILITY_LIBRARIES_INSTALL_DIR)
	list(PREPEND VulkanUtilityLibraries_ROOT ${VULKAN_UTILITY_LIBRARIES_INSTALL_DIR})
	find_package(VulkanHeaders 1.4.317 REQUIRED)
	find_package(VulkanUtilityLibraries 1.4.317 REQUIRED)
else()
	add_subdirectory(Vulkan-Utility-Libraries EXCLUDE_FROM_ALL)
endif()

if(VOLK_INSTALL_DIR)
	list(PREPEND volk_ROOT ${VOLK_INSTALL_DIR})
	find_package(volk 317 REQUIRED) # Must be compiled with VOLK_STATIC_DEFINES set to VK_ENABLE_BETA_EXTENSIONS
else()
	if(NOT VULKAN_HEADERS_INSTALL_DIR)
		set(VULKAN_HEADERS_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers)
	endif()
	set(VOLK_STATIC_DEFINES VK_ENABLE_BETA_EXTENSIONS)
	add_subdirectory(volk)
endif()

# cwalk
add_subdirectory(cwalk)
add_library(cwalk::cwalk ALIAS cwalk)

# Where Am I?
add_library(whereami STATIC)
add_library(wai::wai ALIAS whereami)

set_target_properties(whereami PROPERTIES
	C_STANDARD 17
	C_STANDARD_REQUIRED OFF
	C_EXTENSIONS ON
)

target_compile_options(whereami PRIVATE
	$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:-O3>
	$<$<C_COMPILER_ID:MSVC>:/O2>
)

set(WAI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/whereami/src)
target_include_directories(whereami PUBLIC ${WAI_SOURCE_DIR})
target_sources(whereami PRIVATE ${WAI_SOURCE_DIR}/whereami.c)
