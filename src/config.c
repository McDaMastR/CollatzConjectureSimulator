/* 
 * Copyright (C) 2024  Seth McDonald <seth.i.mcdonald@gmail.com>
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

// First starting value to test (must be odd)
const uint64_t MIN_TEST_VALUE_TOP    = 0x0000000000000000ULL;
const uint64_t MIN_TEST_VALUE_BOTTOM = 0x0000000000000003ULL;

// Starting value with highest step count found so far
const uint64_t MAX_STEP_VALUE_TOP    = 0x0000000000000000ULL;
const uint64_t MAX_STEP_VALUE_BOTTOM = 0x0000000000000001ULL;

const Steps MAX_STEP_COUNT = 0; // Highest step count found so far

const float MAX_HEAP_MEMORY = .4f; // Maximum proportion of available GPU heap memory to use

const bool QUERY_BENCHMARKING     = true; // Whether to benchmark Vulkan commands via queries
const bool LOG_VULKAN_ALLOCATIONS = false; // Whether to log all memory allocations from Vulkan

const bool EXTENSION_LAYERS  = false; // Whether to use the Khronos extension layers, if present
const bool PROFILE_LAYERS    = false; // Whether to use the Khronos profiles layer, if present
const bool VALIDATION_LAYERS = false; // Whether to use the Khronos validation layer, if present

const bool PREFER_INT16 = false; // Whether to prefer shaders that use 16-bit integers over 32-bit integers where appropriate
const bool PREFER_INT64 = false; // Whether to prefer shaders that use 64-bit integers over 32-bit integers where appropriate

const int ITER_SIZE = 128; // The integer size for shaders to use when iterating (must be 128 or 256)


const char* const PROGRAM_NAME = "Collatz Conjecture Simulator";

const char* const DEBUG_LOG_NAME      = "debug.log";
const char* const ALLOC_LOG_NAME      = "alloc.log";
const char* const PIPELINE_CACHE_NAME = "pipeline_cache.bin";

const char* const VK_KHR_PROFILES_LAYER_NAME           = "VK_LAYER_KHRONOS_profiles";
const char* const VK_KHR_VALIDATION_LAYER_NAME         = "VK_LAYER_KHRONOS_validation";
const char* const VK_KHR_SYNCHRONIZATION_2_LAYER_NAME  = "VK_LAYER_KHRONOS_synchronization2";
const char* const VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME = "VK_LAYER_KHRONOS_timeline_semaphore";


VkAllocationCallbacks* g_allocator = LOG_VULKAN_ALLOCATIONS ? &g_allocationCallbacks : NULL;

CallbackData g_callbackData = {
	.funcName = "",
	.lineNum  = 0
};

VkAllocationCallbacks g_allocationCallbacks = {
	.pUserData             = &g_callbackData,
	.pfnAllocation         = allocation_callback,
	.pfnReallocation       = reallocation_callback,
	.pfnFree               = free_callback,
	.pfnInternalAllocation = internal_allocation_callback,
	.pfnInternalFree       = internal_free_callback
};
