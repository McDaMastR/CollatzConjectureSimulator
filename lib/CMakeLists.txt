# Vulkan
add_subdirectory(Vulkan-Headers EXCLUDE_FROM_ALL)
add_subdirectory(Vulkan-Utility-Libraries EXCLUDE_FROM_ALL)

set(VOLK_STATIC_DEFINES VK_ENABLE_BETA_EXTENSIONS)
set(VULKAN_HEADERS_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers)
add_subdirectory(volk EXCLUDE_FROM_ALL)

# Where Am I?
add_library(whereami STATIC EXCLUDE_FROM_ALL)
add_library(wai::wai ALIAS whereami)

set(WAI_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/whereami/src)
target_include_directories(whereami PUBLIC ${WAI_SOURCE_DIR})
target_sources(whereami PRIVATE ${WAI_SOURCE_DIR}/whereami.c)
