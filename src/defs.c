/* 
 * Copyright (C) 2024 Seth McDonald <seth.i.mcdonald@gmail.com>
 * 
 * This file is part of Collatz Conjecture Simulator.
 * 
 * Collatz Conjecture Simulator is free software: you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software Foundation, either
 * version 3 of the License, or (at your option) any later version.
 * 
 * Collatz Conjecture Simulator is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along with Collatz Conjecture
 * Simulator. If not, see <https://www.gnu.org/licenses/>.
 */

#include "defs.h"
#include "debug.h"


const char* const PROGRAM_NAME      = COLLATZSIM_NAME;
const char* const PROGRAM_EXE       = COLLATZSIM_EXECUTABLE;
const char* const PROGRAM_COPYRIGHT = "Copyright (C) 2024 Seth McDonald";
const char* const PROGRAM_LICENCE   = "Licence GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>";

const char* const DEBUG_LOG_NAME      = "debug.log";
const char* const ALLOC_LOG_NAME      = "alloc.log";
const char* const PIPELINE_CACHE_NAME = "pipeline_cache.bin";
const char* const PROGRESS_NAME       = "position.txt";

const char* const VK_KHR_PROFILES_LAYER_NAME           = "VK_LAYER_KHRONOS_profiles";
const char* const VK_KHR_VALIDATION_LAYER_NAME         = "VK_LAYER_KHRONOS_validation";
const char* const VK_KHR_SYNCHRONIZATION_2_LAYER_NAME  = "VK_LAYER_KHRONOS_synchronization2";
const char* const VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME = "VK_LAYER_KHRONOS_timeline_semaphore";

const char* const VK_KHR_8BIT_STORAGE_EXTENSION_NAME                   = "VK_KHR_8bit_storage";
const char* const VK_KHR_16BIT_STORAGE_EXTENSION_NAME                  = "VK_KHR_16bit_storage";
const char* const VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME          = "VK_KHR_buffer_device_address";
const char* const VK_KHR_MAINTENANCE_4_EXTENSION_NAME                  = "VK_KHR_maintenance4";
const char* const VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME = "VK_KHR_pipeline_executable_properties";
const char* const VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME        = "VK_KHR_portability_enumeration";
const char* const VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME             = "VK_KHR_portability_subset";
const char* const VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME          = "VK_KHR_shader_float_controls";
const char* const VK_KHR_SPIRV_1_4_EXTENSION_NAME                      = "VK_KHR_spirv_1_4";
const char* const VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME              = "VK_KHR_synchronization2";
const char* const VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME             = "VK_KHR_timeline_semaphore";

const char* const VK_EXT_DEBUG_UTILS_EXTENSION_NAME                     = "VK_EXT_debug_utils";
const char* const VK_EXT_MEMORY_BUDGET_EXTENSION_NAME                   = "VK_EXT_memory_budget";
const char* const VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME                 = "VK_EXT_memory_priority";
const char* const VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME = "VK_EXT_pipeline_creation_cache_control";
const char* const VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME           = "VK_EXT_subgroup_size_control";


const uint32_t PROGRAM_VERSION   = VK_MAKE_API_VERSION(
	0, COLLATZSIM_VERSION_MAJOR, COLLATZSIM_VERSION_MINOR, COLLATZSIM_VERSION_PATCH);
const uint32_t PROGRAM_VER_MAJOR = COLLATZSIM_VERSION_MAJOR;
const uint32_t PROGRAM_VER_MINOR = COLLATZSIM_VERSION_MINOR;
const uint32_t PROGRAM_VER_PATCH = COLLATZSIM_VERSION_PATCH;


const VkAllocationCallbacks* g_allocator = NULL;

const VkAllocationCallbacks g_allocationCallbacks = {
	.pUserData             = &g_callbackData,
	.pfnAllocation         = allocation_callback,
	.pfnReallocation       = reallocation_callback,
	.pfnFree               = free_callback,
	.pfnInternalAllocation = internal_allocation_callback,
	.pfnInternalFree       = internal_free_callback
};


const float MS_PER_CLOCK = 1000.f / CLOCKS_PER_SEC;
