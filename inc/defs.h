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
#include <vulkan/vulkan.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>


// * ===== Macros =====

#define PROGRAM_NAME "Collatz Conjecture Simulator"
#define VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME "VK_LAYER_KHRONOS_validation"

#define LOG_NAME "log.txt"
#define SHADER32_NAME "shader.spv"
#define SHADER64_NAME "shader64.spv"
#define PIPELINE_CACHE_NAME "pipeline_cache.bin"

// Milliseconds per clock
#define MS_PER_CLOCK (1000.0F / CLOCKS_PER_SEC)

// Set a 128-bit integer to a given value
#define SET_128BIT_INT(val, top, bottom) (val) = (top); (val) <<= 64; (val) |= (bottom);

// Get the top 64 bits of a 128-bit integer
#define GET_TOP_128BIT_INT(val) ((uint64_t) ((val) >> 64))

// Get the bottom 64 bits of a 128-bit integer
#define GET_BOTTOM_128BIT_INT(val) ((uint64_t) ((val) & UINT64_MAX))

// Minimum/first starting value to test (MUST BE ODD)
#define MIN_TEST_VALUE_TOP    0X0000000000000000ULL
#define MIN_TEST_VALUE_BOTTOM 0X0000000000000003ULL
#define SET_MIN_TEST_VALUE(val) SET_128BIT_INT(val, MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM)

// Starting value with highest step count found so far
#define MAX_STEP_VALUE_TOP    0X0000000000000000ULL
#define MAX_STEP_VALUE_BOTTOM 0X0000000000000001ULL
#define SET_MAX_STEP_VALUE(val) SET_128BIT_INT(val, MAX_STEP_VALUE_TOP, MAX_STEP_VALUE_BOTTOM)

// Highest step count found so far
#define MAX_STEP_COUNT 0U

// Maximum proportion of available GPU heap memory to use
#define MAX_HEAP_MEMORY 0.8F

// When to end program
// 1 -> On user input
// 2 -> On # loops completed
// 3 -> On new highest step count
#define END_ON 1

// Whether to benchmark Vulkan commands via queries
#define QUERY_BENCHMARKING 1

// Whether to log all memory allocations from Vulkan (must be in debug build to enable)
#define LOG_VULKAN_ALLOCATIONS 0

// If 1, use Shader Storage Buffer Object (SSBO)
// If 2, use Uniform Buffer Object (UBO)
// If changing, must change in shaders as well
#define IN_BUFFER_TYPE 1

// Just a newline
#define NEWLINE putchar('\n');

// Failure macros
#define MALLOC_FAILURE(ptr, size) print_malloc_failure(__LINE__, (const void*) (ptr), (size_t) (size));
#define CALLOC_FAILURE(ptr, num, size) print_calloc_failure(__LINE__, (const void*) (ptr), (size_t) (num), (size_t) (size));
#define REALLOC_FAILURE(ptr, mem, size) print_realloc_failure(__LINE__, (const void*) (ptr), (const void*) (mem), (size_t) (size));
#define FOPEN_FAILURE(name, mode) print_fopen_failure(__LINE__, file, (const char*) (name), (const char*) (mode));
#define FCLOSE_FAILURE() print_fclose_failure(__LINE__, fileResult, file);
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
#define VULKAN_FAILURE(func, count, ...) print_vulkan_failure(__LINE__, #func, result, (unsigned int) (count), __VA_ARGS__);
#define INSTPROCADDR_FAILURE(inst, func) print_instprocaddr_failure(__LINE__, (VkInstance) (inst), #func);
#define DEVPROCADDR_FAILURE(dev, func) print_devprocaddr_failure(__LINE__, (VkDevice) (dev), #func);

#ifdef NDEBUG
	#define CHECK_RESULT(func) func(&gpu);
	#define CHECK_HANDLE(handle)
	#define GET_RESULT(func) func;
	#define GET_FILE_RESULT(func) func;
	#define GET_READ_RESULT(func) func;
	#define GET_WRITE_RESULT(func) func;
	#define GET_THRD_RESULT(func) func;
	#define BEGIN_FUNC
	#define END_FUNC
	#define GLOBAL_PROC_ADDR(func) const PFN_##func func = (PFN_##func) vkGetInstanceProcAddr(NULL, #func);
	#define INSTANCE_PROC_ADDR(func) const PFN_##func func = (PFN_##func) vkGetInstanceProcAddr(instance, #func);
	#define DEVICE_PROC_ADDR(func) const PFN_##func func = (PFN_##func) vkGetDeviceProcAddr(device, #func);
#else
	#define CHECK_RESULT(func) if (!func(&gpu)) {destroy_gpu(&gpu); puts("EXIT FAILURE"); return EXIT_FAILURE;}
	#define CHECK_HANDLE(handle) if ((handle) != VK_NULL_HANDLE)
	#define GET_RESULT(func) result = func;
	#define GET_FILE_RESULT(func) fileResult = func;
	#define GET_READ_RESULT(func) readResult = func;
	#define GET_WRITE_RESULT(func) writeResult = func;
	#define GET_THRD_RESULT(func) threadResult = func;
	#define BEGIN_FUNC printf("BEGIN %s\n", __func__);
	#define END_FUNC printf("END %s\n\n", __func__);
	#define GLOBAL_PROC_ADDR(func)                                               \
		const PFN_##func func = (PFN_##func) vkGetInstanceProcAddr(NULL, #func); \
		if (!func) {INSTPROCADDR_FAILURE(NULL, func) return false;}
	#define INSTANCE_PROC_ADDR(func)                                                 \
		const PFN_##func func = (PFN_##func) vkGetInstanceProcAddr(instance, #func); \
		if (!func) {INSTPROCADDR_FAILURE(instance, func) return false;}
	#define DEVICE_PROC_ADDR(func)                                               \
		const PFN_##func func = (PFN_##func) vkGetDeviceProcAddr(device, #func); \
		if (!func) {DEVPROCADDR_FAILURE(device, func) return false;}
#endif // NDEBUG

#define SET_DEBUG_NAME                                                          \
	GET_RESULT(vkSetDebugUtilsObjectNameEXT(device, &debugUtilsObjectNameInfo)) \
	if (result != VK_SUCCESS)                                                   \
		VULKAN_FAILURE(vkSetDebugUtilsObjectNameEXT, 2, 'p', device, 'p', &debugUtilsObjectNameInfo)

#define HOST_VISIBLE_BUFFER_USAGE VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
#define OUT_BUFFER_DESCRIPTOR_TYPE VK_DESCRIPTOR_TYPE_STORAGE_BUFFER

#if IN_BUFFER_TYPE == 1
	#define DEVICE_LOCAL_BUFFER_USAGE VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
	#define IN_BUFFER_DESCRIPTOR_TYPE VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
#elif IN_BUFFER_TYPE == 2
	#define DEVICE_LOCAL_BUFFER_USAGE VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
	#define IN_BUFFER_DESCRIPTOR_TYPE VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
#endif // IN_BUFFER_TYPE


// * ===== Typedefs =====

// Data type of values to test
__extension__ typedef unsigned __int128 value_t;

// Data type of step count
typedef uint16_t step_t;

// Structure containing all Vulkan memory allocation callback counts
typedef struct AllocationCallbackCounts
{
	uint64_t allocationCount;
	uint64_t reallocationCount;
	uint64_t freeCount;
	uint64_t internalAllocationCount;
	uint64_t internalFreeCount;
} AllocationCallbackCounts_t;

// Structure containing all relevant Vulkan info
typedef struct Gpu
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	uint32_t transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex;

	uint32_t hostVisibleMemoryHeapIndex;
	uint32_t deviceLocalMemoryHeapIndex;
	uint32_t hostVisibleMemoryTypeIndex;
	uint32_t deviceLocalMemoryTypeIndex;

	VkQueue transferQueue;
	VkQueue computeQueue;

	VkDeviceMemory* hostVisibleDeviceMemories; // Count = deviceMemoriesPerHeap
	VkDeviceMemory* deviceLocalDeviceMemories; // Count = deviceMemoriesPerHeap

	VkDeviceSize inBufferAlignment;
	VkDeviceSize outBufferAlignment;
	VkDeviceSize hostVisibleBufferAlignment;
	VkDeviceSize deviceLocalBufferAlignment;

	VkBuffer* hostVisibleBuffers; // Count = buffersPerHeap
	VkBuffer* deviceLocalBuffers; // Count = buffersPerHeap

	value_t** mappedHostVisibleInBuffers; // Count = inoutBuffersPerHeap, valuesPerInoutBuffer
	step_t** mappedHostVisibleOutBuffers; // Count = inoutBuffersPerHeap, valuesPerInoutBuffer

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet* descriptorSets; // Count = inoutBuffersPerHeap

	VkShaderModule shaderModule;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkCommandPool onetimeCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;

	/*
		Copy operation: HV-in -> DL-in
		Availability operation: (copy operation, DL-in) -> device domain
		Release operation: DL-in -> compute QF
	*/
	VkCommandBuffer onetimeCommandBuffer;

	/*
		Copy operation: HV-in -> DL-in
		Availability operation: (copy operation, DL-in) -> device domain
		Release operation: DL-in -> compute QF

		Aquire operation: compute QF -> DL-out
		Visibility operation: device domain -> (copy operation; DL-out)
		Copy operation: DL-out -> HV-out
		Availability operation: (copy operation; HV-out) -> device domain
		Memory domain operation: device domain -> host domain
	*/
	VkCommandBuffer* transferCommandBuffers; // Count = inoutBuffersPerHeap

	/*
		Bind pipeline
		Bind descriptor set

		Aquire operation: transfer QF -> DL-in
		Visibility operation: device domain -> (dispatch operation; DL-in)
		Dispatch operation: DL-in -> DL-out
		Availability operation: (dispatch operation; DL-out) -> device domain
		Release operation: DL-out -> transfer QF
	*/
	VkCommandBuffer* computeCommandBuffers; // Count = inoutBuffersPerHeap

	VkSemaphore onetimeSemaphore;
	VkSemaphore* semaphores; // Count = inoutBuffersPerHeap

	VkDeviceSize bytesPerInBuffer;
	VkDeviceSize bytesPerOutBuffer;
	VkDeviceSize bytesPerHostVisibleInoutBuffer;
	VkDeviceSize bytesPerDeviceLocalInoutBuffer;
	VkDeviceSize bytesPerHostVisibleBuffer;
	VkDeviceSize bytesPerDeviceLocalBuffer;
	VkDeviceSize bytesPerHostVisibleDeviceMemory;
	VkDeviceSize bytesPerDeviceLocalDeviceMemory;
	VkDeviceSize bytesPerHostVisibleHeap;
	VkDeviceSize bytesPerDeviceLocalHeap;

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

	bool usingShaderInt64;
	bool usingMemoryBudget;
	bool usingMemoryPriority;
	void* dynamicMemory;

	uint32_t transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits;
	float timestampPeriod;
	VkQueryPool queryPool;

#ifndef NDEBUG
	uint64_t debugCallbackCount;
	VkDebugUtilsMessengerEXT debugMessenger;
#endif // NDEBUG

	const VkAllocationCallbacks* allocator;
#if LOG_VULKAN_ALLOCATIONS
	AllocationCallbackCounts_t allocationCallbackCounts;
	VkAllocationCallbacks allocationCallbacks;
#endif // LOG_VULKAN_ALLOCATIONS
} Gpu_t;


// * ===== Functions =====

// Creates Vulkan instance
// Returns true if successful, false otherwise
bool create_instance(Gpu_t* gpu);

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

// Failure functions
void print_malloc_failure(int line, const void* ptr, size_t _Size);
void print_calloc_failure(int line, const void* ptr, size_t _NumOfElements, size_t _SizeOfElements);
void print_realloc_failure(int line, const void* ptr, const void* _Memory, size_t _NewSize);
void print_fopen_failure(int line, const FILE* file, const char* _Filename, const char* _Mode);
void print_fclose_failure(int line, int result, const FILE* _File);
void print_fseek_failure(int line, int result, const FILE* _File, long _Offset, int _Origin);
void print_ftell_failure(int line, long result, const FILE* _File);
void print_fread_failure(int line, size_t result, const void* _DstBuf, size_t _ElementSize, size_t _Count, const FILE* _File);
void print_fwrite_failure(int line, size_t result, const void* _Str, size_t _Size, size_t _Count, const FILE* _File);
void print_pcreate_failure(int line, int result, const pthread_t* th, const pthread_attr_t* attr, const char* func, const void* arg);
void print_pjoin_failure(int line, int result, pthread_t t, const void* const* res);
void print_pcancel_failure(int line, int result, pthread_t t);
void print_pinit_failure(int line, int result, const pthread_mutex_t* m, const pthread_mutexattr_t* a);
void print_plock_failure(int line, int result, const pthread_mutex_t* m);
void print_punlock_failure(int line, int result, const pthread_mutex_t* m);
void print_pdestroy_failure(int line, int result, const pthread_mutex_t* m);
void print_vulkan_failure(int line, const char* func, VkResult result, unsigned int count, ...);
void print_instprocaddr_failure(int line, VkInstance instance, const char* pName);
void print_devprocaddr_failure(int line, VkDevice device, const char* pName);

// Callback functions
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback              (VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
VKAPI_ATTR void*    VKAPI_CALL allocation_callback         (void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void*    VKAPI_CALL reallocation_callback       (void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void     VKAPI_CALL free_callback               (void* pUserData, void* pMemory);
VKAPI_ATTR void     VKAPI_CALL internal_allocation_callback(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void     VKAPI_CALL internal_free_callback      (void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
