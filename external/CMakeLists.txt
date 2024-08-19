add_subdirectory(Vulkan-Headers)

set(UPDATE_DEPS OFF)
set(VUL_WERROR OFF)
set(VUL_MOCK_ANDROID OFF)
set(BUILD_TESTS OFF)
add_subdirectory(Vulkan-Utility-Libraries)

set(VOLK_STATIC_DEFINES VK_ENABLE_BETA_EXTENSIONS)
set(VOLK_PULL_IN_VULKAN OFF)
set(VOLK_INSTALL OFF)
set(VOLK_HEADERS_ONLY OFF)
add_subdirectory(volk)
