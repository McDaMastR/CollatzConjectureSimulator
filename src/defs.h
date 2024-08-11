/* 
 * Collatz Conjecture Simulator
 * Copyright (C) 2024  Seth Isaiah McDonald <seth.i.mcdonald@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#define VK_ENABLE_BETA_EXTENSIONS
#include <volk.h> // vulkan.h, stddef.h, stdint.h

#include <pthread.h> // stddef.h, limits.h, time.h
#include <stdbool.h>
#include <stdlib.h> // limits.h
#include <stdio.h>


// * ===== Macros =====

#define PROGRAM_NAME "Collatz Conjecture Simulator"
#define VK_KHR_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"
#define VK_KHR_SYNCHRONIZATION_2_LAYER_NAME "VK_LAYER_KHRONOS_synchronization2"

#define DEBUG_LOG_NAME "debug_log.txt"
#define ALLOC_LOG_NAME "alloc_log.txt"
#define SHADER_16_64_NAME "shader_16_64.spv"
#define SHADER_16_NAME "shader_16.spv"
#define SHADER_64_NAME "shader_64.spv"
#define SHADER_NOEXT_NAME "shader.spv"
#define PIPELINE_CACHE_NAME "pipeline_cache.bin"

#define MS_PER_CLOCK (1000.0f / CLOCKS_PER_SEC)
#define PROGRAM_TIME (clock() * MS_PER_CLOCK)

// Get top 64 bits of 128-bit integer
#define TOP_128BIT_INT(val) ((uint64_t) ((val) >> 64))

// Get bottom 64 bits of 128-bit integer
#define BOTTOM_128BIT_INT(val) ((uint64_t) ((val) & ~0ULL))

// Set 128-bit integer to given 128-bit value (split into top and bottom 64 bits)
#define SET_128BIT_INT(val, top, bottom) {(val) = (top); (val) <<= 64; (val) |= (bottom);}

// First starting value to test (MUST BE ODD)
#define MIN_TEST_VALUE_TOP    0x0000000000000000ULL
#define MIN_TEST_VALUE_BOTTOM 0x0000000000000003ULL
#define SET_MIN_TEST_VALUE(val) SET_128BIT_INT(val, MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM)

// Starting value with highest step count found so far
#define MAX_STEP_VALUE_TOP    0x0000000000000000ULL
#define MAX_STEP_VALUE_BOTTOM 0x0000000000000001ULL
#define SET_MAX_STEP_VALUE(val) SET_128BIT_INT(val, MAX_STEP_VALUE_TOP, MAX_STEP_VALUE_BOTTOM)

// Highest step count found so far
#define MAX_STEP_COUNT 0U

// Maximum proportion of available GPU heap memory to use
#define MAX_HEAP_MEMORY 0.8f

// Whether to benchmark Vulkan commands via queries
#define QUERY_BENCHMARKING 1

// Whether to log all memory allocations from Vulkan
#define LOG_VULKAN_ALLOCATIONS 0

// Whether to use Khronos extension layers
#define EXTENSION_LAYERS 1

// Whether to use Khronos validation layers
#define VALIDATION_LAYERS 0

// When to end program
// 1 -> On user input
// 2 -> On # loops completed
// 3 -> On new highest step count
#define END_ON 1

// Just a newline
#define NEWLINE putchar('\n');

#define MALLOC_FAILURE(ptr) print_malloc_failure(__LINE__, (const void*) (ptr), size);
#define CALLOC_FAILURE(ptr, num, size) print_calloc_failure(__LINE__, (const void*) (ptr), (size_t) (num), (size_t) (size));
#define REALLOC_FAILURE(ptr, mem, size) print_realloc_failure(__LINE__, (const void*) (ptr), (const void*) (mem), (size_t) (size));

#define FOPEN_FAILURE(name, mode) print_fopen_failure(__LINE__, file, (const char*) (name), (const char*) (mode));
#define FSEEK_FAILURE(off, ori) print_fseek_failure(__LINE__, fileResult, file, (long) (off), (int) (ori));
#define FTELL_FAILURE() print_ftell_failure(__LINE__, fileSize, file);
#define FREAD_FAILURE(buf, size, count) print_fread_failure(__LINE__, readResult, (const void*) (buf), (size_t) (size), (size_t) (count), file);
#define FWRITE_FAILURE(str, size, count) print_fwrite_failure(__LINE__, writeResult, (const void*) (str), (size_t) (size), (size_t) (count), file);

#define PCREATE_FAILURE(thrd, atr, func, arg) print_pcreate_failure(__LINE__, threadResult, (const pthread_t*) (thrd), (const pthread_attr_t*) (atr), #func, (const void*) (arg));
#define PJOIN_FAILURE(thrd, res) print_pjoin_failure(__LINE__, threadResult, (pthread_t) (thrd), (const void* const*) (res));
#define PCANCEL_FAILURE(thrd) print_pcancel_failure(__LINE__, threadResult, (pthread_t) (thrd));
#define PINIT_FAILURE(mtx, atr) print_pinit_failure(__LINE__, threadResult, (const pthread_mutex_t*) (mtx), (const pthread_mutexattr_t*) (atr));
#define PLOCK_FAILURE(mtx) print_plock_failure(__LINE__, threadResult, (const pthread_mutex_t*) (mtx));
#define PUNLOCK_FAILURE(mtx) print_punlock_failure(__LINE__, threadResult, (const pthread_mutex_t*) (mtx));
#define PDESTROY_FAILURE(mtx) print_pdestroy_failure(__LINE__, threadResult, (const pthread_mutex_t*) (mtx));

#define VINIT_FAILURE() print_vinit_failure(__LINE__, result);
#define VINSTVERS_FAILURE(vers) print_vinstvers_failure(__LINE__, vers);
#define VULKAN_FAILURE(func, count, ...) print_vulkan_failure(__LINE__, #func, result, (unsigned) (count), __VA_ARGS__);

#define CHECK_RESULT(func) if (!func) {destroy_gpu(&gpu); puts("EXIT FAILURE"); return EXIT_FAILURE;}

#ifdef NDEBUG
	#define GET_RESULT(func) func;
	#define GET_INIT_RESULT(func) func;
	#define GET_FILE_RESULT(func) func;
	#define GET_READ_RESULT(func) func;
	#define GET_WRITE_RESULT(func) func;
	#define GET_THRD_RESULT(func) func;
	#define BEGIN_FUNC
	#define END_FUNC
#else
	#define GET_RESULT(func) result = func;
	#define GET_INIT_RESULT(func) initResult = func;
	#define GET_FILE_RESULT(func) fileResult = func;
	#define GET_READ_RESULT(func) readResult = func;
	#define GET_WRITE_RESULT(func) writeResult = func;
	#define GET_THRD_RESULT(func) threadResult = func;
	#define BEGIN_FUNC printf("BEGIN %s\n", __func__);
	#define END_FUNC printf("END %s\n\n", __func__);

	#define SET_DEBUG_NAME()                                                          \
		GET_RESULT(vkSetDebugUtilsObjectNameEXT(g_device, &debugUtilsObjectNameInfo)) \
		if (result)                                                                   \
			VULKAN_FAILURE(vkSetDebugUtilsObjectNameEXT, 2, 'p', g_device, 'p', &debugUtilsObjectNameInfo)
#endif


// * ===== Datatypes =====

// Data type of tested values
#if defined(__SIZEOF_INT128__) && __SIZEOF_INT128__ == 16
	__extension__ typedef unsigned __int128 value_t;
#else
	#error "Compiler must support 128-bit unsigned integers via the '__int128' type"
#endif

// Data type of step counts
typedef uint16_t step_t;

// All Vulkan allocation callback counts
typedef struct AllocationCallbackCounts
{
	uint64_t allocationCount;
	uint64_t reallocationCount;
	uint64_t freeCount;
	uint64_t internalAllocationCount;
	uint64_t internalFreeCount;
} AllocationCallbackCounts_t;

// Relevant Vulkan info
typedef struct Gpu
{
	VkDeviceMemory* hostVisibleDeviceMemories; // Count = deviceMemoriesPerHeap
	VkDeviceMemory* deviceLocalDeviceMemories; // Count = deviceMemoriesPerHeap

	VkBuffer* hostVisibleBuffers; // Count = buffersPerHeap
	VkBuffer* deviceLocalBuffers; // Count = buffersPerHeap

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool      descriptorPool;
	VkDescriptorSet*      descriptorSets; // Count = inoutBuffersPerHeap

	VkShaderModule   shaderModule;
	VkPipelineCache  pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline       pipeline;

	VkCommandPool onetimeCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;

	VkCommandBuffer  onetimeCommandBuffer;
	VkCommandBuffer* transferCommandBuffers; // Count = inoutBuffersPerHeap
	VkCommandBuffer* computeCommandBuffers;  // Count = inoutBuffersPerHeap

	VkSemaphore* semaphores; // Count = inoutBuffersPerHeap
	VkQueryPool  queryPool;

	value_t** mappedHostVisibleInBuffers;  // Count = inoutBuffersPerHeap, valuesPerInoutBuffer
	step_t**  mappedHostVisibleOutBuffers; // Count = inoutBuffersPerHeap, valuesPerInoutBuffer

	VkDeviceSize bytesPerInBuffer;
	VkDeviceSize bytesPerOutBuffer;
	VkDeviceSize bytesPerInoutBuffer;
	VkDeviceSize bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleBuffer;
	VkDeviceSize bytesPerDeviceLocalBuffer;
	VkDeviceSize bytesPerHostVisibleDeviceMemory;
	VkDeviceSize bytesPerDeviceLocalDeviceMemory;

	uint32_t valuesPerInoutBuffer;
	uint32_t valuesPerBuffer;
	uint32_t valuesPerDeviceMemory;
	uint32_t valuesPerHeap;
	uint32_t inoutBuffersPerBuffer;
	uint32_t inoutBuffersPerDeviceMemory;
	uint32_t inoutBuffersPerHeap;
	uint32_t buffersPerDeviceMemory;
	uint32_t buffersPerHeap;
	uint32_t deviceMemoriesPerHeap;

	uint32_t computeWorkGroupCount;
	uint32_t computeWorkGroupSize;

	uint32_t hostVisibleMemoryHeapIndex;
	uint32_t deviceLocalMemoryHeapIndex;

	uint32_t hostVisibleMemoryTypeIndex;
	uint32_t deviceLocalMemoryTypeIndex;

	uint32_t transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex;

	uint32_t transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits;

	float timestampPeriod;

	bool hostNonCoherent;
	bool usingShaderInt16;
	bool usingShaderInt64;
	bool usingMaintenance4;
	bool usingMemoryBudget;
	bool usingMemoryPriority;
	bool usingSubgroupSizeControl;
	bool usingPortabilitySubset;

	void* dynamicMemory;
} Gpu_t;


// * ===== Functions =====

// Creates Vulkan instance
// Returns true if successful, false otherwise
bool create_instance(void);

// Selects physical device
// Returns true if successful, false otherwise
bool select_device(Gpu_t* gpu);

// Creates logical device
// Returns true if successful, false otherwise
bool create_device(Gpu_t* gpu);

// Calculates values describing memory management
// Returns true if successful, false otherwise
bool manage_memory(Gpu_t* gpu);

// Allocates memory and creates buffers
// Returns true if successful, false otherwise
bool create_buffers(Gpu_t* gpu);

// Creates descriptor sets
// Returns true if successful, false otherwise
bool create_descriptors(Gpu_t* gpu);

// Creates compute pipeline
// Returns true if successful, false otherwise
bool create_pipeline(Gpu_t* gpu);

// Creates command buffers and semaphores
// Returns true if successful, false otherwise
bool create_commands(Gpu_t* gpu);

// Starts main loop
// Returns true if successful, false otherwise
bool submit_commands(Gpu_t* gpu);

// Destroys all Vulkan handles, frees all allocated memory
// Returns true if successful, false otherwise
bool destroy_gpu(Gpu_t* gpu);

// Initialise the debug logfile
// Returns true if successful, false otherwise
bool init_debug_logfile(void);

// Initialise the allocation logfile
// Returns true if successful, false otherwise
bool init_alloc_logfile(void);

// Failure functions
void print_malloc_failure (int line, const void* ptr, size_t _Size);
void print_calloc_failure (int line, const void* ptr, size_t _NumOfElements, size_t _SizeOfElements);
void print_realloc_failure(int line, const void* ptr, const void* _Memory, size_t _NewSize);

void print_fopen_failure (int line, const FILE* file, const char* _Filename, const char* _Mode);
void print_fseek_failure (int line, int result, const FILE* _File, long _Offset, int _Origin);
void print_ftell_failure (int line, long result, const FILE* _File);
void print_fread_failure (int line, size_t result, const void* _DstBuf, size_t _ElementSize, size_t _Count, const FILE* _File);
void print_fwrite_failure(int line, size_t result, const void* _Str, size_t _Size, size_t _Count, const FILE* _File);

void print_pcreate_failure (int line, int result, const pthread_t* th, const pthread_attr_t* attr, const char* func, const void* arg);
void print_pjoin_failure   (int line, int result, pthread_t t, const void* const* res);
void print_pcancel_failure (int line, int result, pthread_t t);
void print_pinit_failure   (int line, int result, const pthread_mutex_t* m, const pthread_mutexattr_t* a);
void print_plock_failure   (int line, int result, const pthread_mutex_t* m);
void print_punlock_failure (int line, int result, const pthread_mutex_t* m);
void print_pdestroy_failure(int line, int result, const pthread_mutex_t* m);

void print_vinit_failure    (int line, VkResult result);
void print_vinstvers_failure(int line, uint32_t apiVersion);
void print_vulkan_failure   (int line, const char* func, VkResult result, unsigned count, ...);

// Callback functions
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback              (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
VKAPI_ATTR void*    VKAPI_CALL allocation_callback         (void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void*    VKAPI_CALL reallocation_callback       (void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void     VKAPI_CALL free_callback               (void* pUserData, void* pMemory);
VKAPI_ATTR void     VKAPI_CALL internal_allocation_callback(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void     VKAPI_CALL internal_free_callback      (void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
