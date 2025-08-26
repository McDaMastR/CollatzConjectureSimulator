/* 
 * Copyright (C) 2024-2025 Seth McDonald
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


const char* const PROGRAM_NAME      = CLTZ_NAME;
const char* const PROGRAM_EXE       = CLTZ_EXECUTABLE;
const char* const PROGRAM_COPYRIGHT = "Copyright (C) 2025 Seth McDonald";
const char* const PROGRAM_LICENCE   = "Licence GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>";

const char* const DEBUG_LOG_NAME      = "debug.log";
const char* const ALLOC_LOG_NAME      = "alloc.log";
const char* const PIPELINE_CACHE_NAME = "pipeline_cache.bin";
const char* const PROGRESS_FILE_NAME  = "position.txt";
const char* const CAPTURE_FILE_NAME   = "pipeline_capture.txt";

const char* const VK_KHR_PROFILES_LAYER_NAME           = "VK_LAYER_KHRONOS_profiles";
const char* const VK_KHR_VALIDATION_LAYER_NAME         = "VK_LAYER_KHRONOS_validation";
const char* const VK_KHR_SYNCHRONIZATION_2_LAYER_NAME  = "VK_LAYER_KHRONOS_synchronization2";
const char* const VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME = "VK_LAYER_KHRONOS_timeline_semaphore";

const uint32_t PROGRAM_VERSION   = VK_MAKE_API_VERSION(0, CLTZ_VERSION_MAJOR, CLTZ_VERSION_MINOR, CLTZ_VERSION_PATCH);
const uint32_t PROGRAM_VER_MAJOR = CLTZ_VERSION_MAJOR;
const uint32_t PROGRAM_VER_MINOR = CLTZ_VERSION_MINOR;
const uint32_t PROGRAM_VER_PATCH = CLTZ_VERSION_PATCH;

const double MS_PER_CLOCK = 1000. / CLOCKS_PER_SEC;

const VkAllocationCallbacks* g_allocator = NULL;

const VkAllocationCallbacks g_allocationCallbacks = {
	.pUserData             = &g_callbackData,
	.pfnAllocation         = allocation_callback,
	.pfnReallocation       = reallocation_callback,
	.pfnFree               = free_callback,
	.pfnInternalAllocation = internal_allocation_callback,
	.pfnInternalFree       = internal_free_callback
};

ProgramConfig g_config = {
	.outputLevel      = OUTPUT_LEVEL_DEFAULT,
	.colourLevel      = COLOUR_LEVEL_TTY,
	.iterSize         = 128,
	.maxLoops         = ULONG_LONG_MAX,
	.maxMemory        = .4f,
	.preferInt16      = false,
	.preferInt64      = false,
	.extensionLayers  = false,
	.profileLayers    = false,
	.validationLayers = false,
	.restart          = false,
	.queryBenchmarks  = true,
	.logAllocations   = false,
	.capturePipelines = false
};
