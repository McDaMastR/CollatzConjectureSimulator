# Vulkan
add_subdirectory(Vulkan-Headers)
add_subdirectory(Vulkan-Utility-Libraries EXCLUDE_FROM_ALL)

set(VOLK_STATIC_DEFINES VK_ENABLE_BETA_EXTENSIONS)
set(VULKAN_HEADERS_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers)
add_subdirectory(volk)

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
