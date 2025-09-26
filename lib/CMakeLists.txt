add_subdirectory(Vulkan-Headers EXCLUDE_FROM_ALL)
add_subdirectory(Vulkan-Utility-Libraries EXCLUDE_FROM_ALL)

set(VOLK_STATIC_DEFINES VK_ENABLE_BETA_EXTENSIONS)
set(VULKAN_HEADERS_INSTALL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Vulkan-Headers)
add_subdirectory(volk EXCLUDE_FROM_ALL)
