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

#pragma once


// ===== Attribute macros =====

// NOTE: These are mainly for compiler warnings.

#ifndef __has_attribute
	#define __has_attribute(attr) 0
#endif

#if __has_attribute(cold)
	#define COLD_FUNC __attribute__((cold))
#else
	#define COLD_FUNC
#endif

#if __has_attribute(const)
	#define CONST_FUNC __attribute__((const))
#else
	#define CONST_FUNC
#endif

#if __has_attribute(nonnull)
	#define NONNULL_ARGS __attribute__((nonnull))
#else
	#define NONNULL_ARGS
#endif

#if __has_attribute(access)
	#define NO_ACCESS(index) __attribute__((access(none, index)))
#else
	#define NO_ACCESS(index)
#endif


// ===== Datatypes =====

#if defined(__SIZEOF_INT128__) && __SIZEOF_INT128__ == 16
	__extension__ typedef unsigned __int128 Value;
#else
	#error "Compiler must support 128-bit unsigned integers via the __int128 datatype"
#endif

typedef uint16_t Steps;

typedef struct DyArray DyArray;

typedef struct CallbackData
{
	const char* funcName;
	uint64_t    lineNum;
} CallbackData;

typedef struct Gpu
{
	VkDebugUtilsMessengerEXT debugUtilsMessenger;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue transferQueue;
	VkQueue computeQueue;

	VkBuffer* deviceLocalBuffers; // Count = buffersPerHeap
	VkBuffer* hostVisibleBuffers; // Count = buffersPerHeap

	VkDeviceMemory* deviceLocalDeviceMemories; // Count = buffersPerHeap
	VkDeviceMemory* hostVisibleDeviceMemories; // Count = buffersPerHeap

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool      descriptorPool;
	VkDescriptorSet*      descriptorSets; // Count = inoutsPerHeap

	VkShaderModule   shaderModule;
	VkPipelineCache  pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline       pipeline;

	VkCommandPool onetimeCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;

	VkCommandBuffer  onetimeCommandBuffer;
	VkCommandBuffer* transferCommandBuffers; // Count = inoutsPerHeap
	VkCommandBuffer* computeCommandBuffers;  // Count = inoutsPerHeap

	VkSemaphore* semaphores; // Count = inoutsPerHeap
	VkQueryPool  queryPool;

	Value** mappedInBuffers;  // Count = inoutsPerHeap, valuesPerInout
	Steps** mappedOutBuffers; // Count = inoutsPerHeap, valuesPerInout

	VkDeviceSize bytesPerIn;
	VkDeviceSize bytesPerOut;
	VkDeviceSize bytesPerInout;
	VkDeviceSize bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout;
	uint32_t valuesPerBuffer;
	uint32_t valuesPerHeap;
	uint32_t inoutsPerBuffer;
	uint32_t inoutsPerHeap;
	uint32_t buffersPerHeap;

	uint32_t workgroupCount;
	uint32_t workgroupSize;

	uint32_t hostVisibleHeapIndex;
	uint32_t deviceLocalHeapIndex;

	uint32_t hostVisibleTypeIndex;
	uint32_t deviceLocalTypeIndex;

	uint32_t transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex;

	uint32_t transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits;

	float timestampPeriod;

	bool hostNonCoherent;
	bool using8BitStorage;
	bool usingBufferDeviceAddress;
	bool usingMaintenance4;
	bool usingMemoryBudget;
	bool usingMemoryPriority;
	bool usingPipelineCreationCacheControl;
	bool usingPortabilitySubset;
	bool usingShaderInt16;
	bool usingShaderInt64;
	bool usingSpirv14;
	bool usingSubgroupSizeControl;
	bool usingVulkan12;
	bool usingVulkan13;

	void* dynamicMemory;
} Gpu;


// ===== Global variables =====

// NOTE: All global variables are initialised in config.c

// Configuration variables (constant)
extern const uint64_t MIN_TEST_VALUE_TOP;
extern const uint64_t MIN_TEST_VALUE_BOTTOM;

extern const uint64_t MAX_STEP_VALUE_TOP;
extern const uint64_t MAX_STEP_VALUE_BOTTOM;

extern const Steps MAX_STEP_COUNT;

extern const float MAX_HEAP_MEMORY;

extern const bool QUERY_BENCHMARKING;
extern const bool LOG_VULKAN_ALLOCATIONS;

extern const bool EXTENSION_LAYERS;
extern const bool PROFILE_LAYERS;
extern const bool VALIDATION_LAYERS;

extern const bool PREFER_INT16;
extern const bool PREFER_INT64;

extern const int ITER_SIZE;

// String variables (constant)
extern const char* const PROGRAM_NAME;

extern const char* const DEBUG_LOG_NAME;
extern const char* const ALLOC_LOG_NAME;
extern const char* const PIPELINE_CACHE_NAME;

extern const char* const VK_KHR_PROFILES_LAYER_NAME;
extern const char* const VK_KHR_VALIDATION_LAYER_NAME;
extern const char* const VK_KHR_SYNCHRONIZATION_2_LAYER_NAME;
extern const char* const VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME;

// Runtime variables (mutable)
extern CallbackData           g_callbackData;
extern VkAllocationCallbacks  g_allocationCallbacks;
extern VkAllocationCallbacks* g_allocator;


// ===== Functions =====

// NOTE: If return type is bool, then function returns true on success, and false elsewise

// Main functions
bool create_instance(Gpu* gpu);
bool select_device(Gpu* gpu);
bool create_device(Gpu* gpu);
bool manage_memory(Gpu* gpu);
bool create_buffers(Gpu* gpu);
bool create_descriptors(Gpu* gpu);
bool create_pipeline(Gpu* gpu);
bool create_commands(Gpu* gpu);
bool submit_commands(Gpu* gpu);
bool destroy_gpu(Gpu* gpu);

void* wait_for_input(void* ptr) NONNULL_ARGS;

void writeInBuffer(Value* mappedInBuffer, Value* firstValue, uint32_t valuesPerInoutBuffer, uint32_t valuesPerHeap) NONNULL_ARGS;
void readOutBuffer(const Steps* mappedOutBuffer, Value* firstValue, Value* prev, Steps* longest, DyArray* highestStepValues, DyArray* highestStepCounts, uint32_t valuesPerInoutBuffer) NONNULL_ARGS;

// Utility functions
uint32_t floor_pow2(uint32_t n) CONST_FUNC;
float get_benchmark(clock_t start, clock_t end) CONST_FUNC;

bool set_debug_name               (VkDevice device, VkObjectType type, uint64_t handle, const char* name);
bool get_buffer_requirements_noext(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* memoryRequirements) NONNULL_ARGS;
bool get_buffer_requirements_main4(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* memoryRequirements) NONNULL_ARGS;

bool file_size (const char* filename, size_t* size) NONNULL_ARGS;
bool read_file (const char* filename, void* data, size_t size) NONNULL_ARGS;
bool write_file(const char* filename, const void* data, size_t size) NONNULL_ARGS;

// Logfile functions
bool init_debug_logfile(void);
bool init_alloc_logfile(void);

// Failure functions
NO_ACCESS(2) void print_malloc_failure(int line, const void* ptr, size_t size) COLD_FUNC;
NO_ACCESS(2) void print_calloc_failure(int line, const void* ptr, size_t numOfElements, size_t sizeOfElements) COLD_FUNC;

NO_ACCESS(2) NO_ACCESS(3) void print_realloc_failure(int line, const void* ptr, const void* memory, size_t newSize) COLD_FUNC;

NO_ACCESS(2) void print_fopen_failure(int line, const FILE* file, const char* filename, const char* mode) COLD_FUNC;
NO_ACCESS(3) void print_fseek_failure(int line, int  result, const FILE* file, long offset, int origin) COLD_FUNC;
NO_ACCESS(3) void print_ftell_failure(int line, long result, const FILE* file) COLD_FUNC;

NO_ACCESS(3) NO_ACCESS(6) void print_fread_failure (int line, size_t result, const void* dstBuf, size_t elementSize, size_t count, const FILE* file) COLD_FUNC;
NO_ACCESS(3) NO_ACCESS(6) void print_fwrite_failure(int line, size_t result, const void* str,    size_t size,        size_t count, const FILE* file) COLD_FUNC;

NO_ACCESS(3) NO_ACCESS(4) NO_ACCESS(5) void print_pcreate_failure(int line, int result, const pthread_t* th, const pthread_attr_t* attr, const void* arg) COLD_FUNC;

NO_ACCESS(4) void print_pjoin_failure(int line, int result, pthread_t t, const void* const* res) COLD_FUNC;

void print_pcancel_failure(int line, int result, pthread_t t) COLD_FUNC;

void print_vkinit_failure(int line, VkResult result) COLD_FUNC;
void print_vkvers_failure(int line, uint32_t apiVersion) COLD_FUNC;
void print_vulkan_failure(int line, const char* func, VkResult result) COLD_FUNC;

// Callback functions
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

VKAPI_ATTR void* VKAPI_CALL allocation_callback  (void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void* VKAPI_CALL reallocation_callback(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void  VKAPI_CALL free_callback        (void* pUserData, void* pMemory);

VKAPI_ATTR void VKAPI_CALL internal_allocation_callback(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void VKAPI_CALL internal_free_callback      (void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);


// ===== Helper macros =====

#define MS_PER_CLOCK (1000.f / CLOCKS_PER_SEC)
#define PROGRAM_TIME() ((clock_t) (clock() * MS_PER_CLOCK))

#define NEWLINE() putchar('\n');

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

#define TOP_128BIT_INT(val)    ((uint64_t) ((val) >> 64))
#define BOTTOM_128BIT_INT(val) ((uint64_t) ((val) & ~0ULL))

#define SET_128BIT_INT(val, top, bottom) { (val) = (top); (val) <<= 64; (val) |= (bottom); }

#define MALLOC_FAILURE(res, size)       print_malloc_failure (__LINE__, (const void*) (res), (size_t) (size));
#define CALLOC_FAILURE(res, num, size)  print_calloc_failure (__LINE__, (const void*) (res), (size_t) (num), (size_t) (size));
#define REALLOC_FAILURE(res, ptr, size) print_realloc_failure(__LINE__, (const void*) (res), (const void*) (ptr), (size_t) (size));

#define FOPEN_FAILURE(res, name, mode)     print_fopen_failure(__LINE__, (const FILE*) (res), (const char*) (name), (const char*) (mode));
#define FSEEK_FAILURE(res, file, off, ori) print_fseek_failure(__LINE__, (int) (res), (const FILE*) (file), (long) (off), (int) (ori));
#define FTELL_FAILURE(res, file)           print_ftell_failure(__LINE__, (long) (res), (const FILE*) (file));

#define FREAD_FAILURE(res, buf, size, count, file)  print_fread_failure (__LINE__, (size_t) (res), (const void*) (buf), (size_t) (size), (size_t) (count), (const FILE*) (file));
#define FWRITE_FAILURE(res, str, size, count, file) print_fwrite_failure(__LINE__, (size_t) (res), (const void*) (str), (size_t) (size), (size_t) (count), (const FILE*) (file));

#define PCREATE_FAILURE(res, thr, atr, arg) print_pcreate_failure(__LINE__, (int) (res), (const pthread_t*) (thr), (const pthread_attr_t*) (atr), (const void*) (arg));
#define PJOIN_FAILURE(res, thr, _res)       print_pjoin_failure  (__LINE__, (int) (res), (pthread_t) (thr), (const void* const*) (_res));
#define PCANCEL_FAILURE(res, thr)           print_pcancel_failure(__LINE__, (int) (res), (pthread_t) (thr));

#define VKINIT_FAILURE(res)  print_vkinit_failure(__LINE__, (VkResult) (res));
#define VKVERS_FAILURE(ver)  print_vkvers_failure(__LINE__, (uint32_t) (ver));
#define VULKAN_FAILURE(func) print_vulkan_failure(__LINE__, #func, vkres);

#define CHECK_RESULT(func) if (!(func)) { destroy_gpu(&gpu); puts("EXIT FAILURE"); return EXIT_FAILURE; }

#ifdef NDEBUG
	#define BEGIN_FUNC
	#define END_FUNC

	#define VK_CALL(func, ...)     (func)(__VA_ARGS__);
	#define VK_CALL_RES(func, ...) vkres = (func)(__VA_ARGS__);
#else
	#define BEGIN_FUNC printf("BEGIN %s\n", __func__);
	#define END_FUNC   printf("END %s\n\n", __func__);

	#define VK_CALL(func, ...)     { g_callbackData.funcName = #func; g_callbackData.lineNum = __LINE__; (func)(__VA_ARGS__); }
	#define VK_CALL_RES(func, ...) { g_callbackData.funcName = #func; g_callbackData.lineNum = __LINE__; vkres = (func)(__VA_ARGS__); }
#endif
