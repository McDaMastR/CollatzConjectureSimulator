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

#include "debug.h"
#include "util.h"


CallbackData g_callbackData = {
	.funcName = "",
	.lineNum  = 0
};


static uint64_t g_debugCallbackCount = 0;

static uint64_t g_allocCount         = 0;
static uint64_t g_reallocCount       = 0;
static uint64_t g_freeCount          = 0;
static uint64_t g_internalAllocCount = 0;
static uint64_t g_internalFreeCount  = 0;

static size_t g_totalAllocSize = 0;


bool init_debug_logfile(void)
{
	clock_t programTime = program_time();
	const char* sCurrentTime = stime();

	FILE* file = fopen(DEBUG_LOG_NAME, "w");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, DEBUG_LOG_NAME, "w"); return false; }

	fprintf(file,
		"VULKAN DEBUG CALLBACK LOGFILE\n"
		"PROGRAM NAME: %s\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %ld ms\n\n",
		PROGRAM_NAME, sCurrentTime, programTime
	);

	fclose(file);
	return true;
}

bool init_alloc_logfile(void)
{
	clock_t programTime = program_time();
	const char* sCurrentTime = stime();

	FILE* file = fopen(ALLOC_LOG_NAME, "w");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "w"); return false; }

	fprintf(file,
		"VULKAN ALLOCATION CALLBACK LOGFILE\n"
		"PROGRAM NAME: %s\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %ld ms\n\n",
		PROGRAM_NAME, sCurrentTime, programTime
	);

	fclose(file);
	return true;
}


static void print_debug_callback(
	FILE* stream,
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	uint64_t callbackCount,
	const char* func,
	uint64_t line)
{
	clock_t time = program_time();
	const char* sMessageSeverity = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);

	fprintf(stream,
		"Debug callback %" PRIu64 " (%ld ms)\n"
		"%s (%" PRIu64 ")\n"
		"Severity: %s\n"
		"Types:   ",
		callbackCount, time, func, line, sMessageSeverity
	);

	for (uint32_t i = 0; i < sizeof(messageTypes) * CHAR_BIT; i++) {
		VkDebugUtilsMessageTypeFlagBitsEXT messageType = messageTypes & (1U << i);

		if (messageType) {
			const char* sMessageType = string_VkDebugUtilsMessageTypeFlagBitsEXT(messageType);
			fprintf(stream, " %s", sMessageType);
		}
	}

	const char* messageIdName = pCallbackData->pMessageIdName ?: "";
	uint32_t messageIdNumber  = (uint32_t) pCallbackData->messageIdNumber;

	fprintf(stream, "\nID:       %s (0x%08" PRIx32 ")\n", messageIdName, messageIdNumber);

	// VkDebugUtilsLabelEXT active in the current VkQueue
	uint32_t queueLabelCount = pCallbackData->queueLabelCount;

	if (queueLabelCount) {
		fprintf(stream, "Queue labels (%" PRIu32 "):\n", queueLabelCount);

		for (uint32_t i = 0; i < queueLabelCount; i++) {
			const char* labelName = pCallbackData->pQueueLabels[i].pLabelName;
			double r = (double) pCallbackData->pQueueLabels[i].color[0];
			double g = (double) pCallbackData->pQueueLabels[i].color[1];
			double b = (double) pCallbackData->pQueueLabels[i].color[2];
			double a = (double) pCallbackData->pQueueLabels[i].color[3];

			fprintf(stream, "\t%s (%f, %f, %f, %f)\n", labelName, r, g, b, a);
		}
	}

	// VkDebugUtilsLabelEXT active in the current VkCommandBuffer
	uint32_t cmdBufLabelCount = pCallbackData->cmdBufLabelCount;

	if (cmdBufLabelCount) {
		fprintf(stream, "Command buffer labels (%" PRIu32 "):\n", cmdBufLabelCount);

		for (uint32_t i = 0; i < cmdBufLabelCount; i++) {
			const char* labelName = pCallbackData->pCmdBufLabels[i].pLabelName;
			double r = (double) pCallbackData->pCmdBufLabels[i].color[0];
			double g = (double) pCallbackData->pCmdBufLabels[i].color[1];
			double b = (double) pCallbackData->pCmdBufLabels[i].color[2];
			double a = (double) pCallbackData->pCmdBufLabels[i].color[3];

			fprintf(stream, "\t%s (%f, %f, %f, %f)\n", labelName, r, g, b, a);
		}
	}

	// VkDebugUtilsObjectNameInfoEXT related to the callback
	uint32_t objectCount = pCallbackData->objectCount;

	if (objectCount) {
		fprintf(stream, "Objects (%" PRIu32 "):\n", objectCount);

		for (uint32_t i = 0; i < objectCount; i++) {
			const char*  objectName   = pCallbackData->pObjects[i].pObjectName ?: "";
			VkObjectType objectType   = pCallbackData->pObjects[i].objectType;
			uint64_t     objectHandle = pCallbackData->pObjects[i].objectHandle;

			const char* sObjectType = string_VkObjectType(objectType);

			fprintf(stream, "\t%s (%s, 0x%016" PRIx64 ")\n", objectName, sObjectType, objectHandle);
		}
	}

	const char* message = pCallbackData->pMessage;

	fprintf(stream, "%s\n\n", message);
}

VkBool32 debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
	CallbackData data = *(CallbackData*) pUserData;

	g_debugCallbackCount++;

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		print_debug_callback(stderr, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.funcName, data.lineNum);
	}
	else if (messageTypes & ~(VkDebugUtilsMessageTypeFlagsEXT) VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		print_debug_callback(stdout, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.funcName, data.lineNum);
	}

	FILE* file = fopen(DEBUG_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, DEBUG_LOG_NAME, "a"); return VK_FALSE; }

	print_debug_callback(file, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.funcName, data.lineNum);

	fclose(file);
	return VK_FALSE;
}


static void print_allocation_callback(
	FILE* stream,
	uint64_t allocationCount,
	const char* func,
	uint64_t line,
	size_t totalSize,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* memory)
{
	clock_t time = program_time();
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream,
		"Allocation callback %" PRIu64 " (%ld ms)\n"
		"%s (%" PRIu64 ")\n"
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Size:      %zu\n"
		"Alignment: %zu\n"
		"Scope:     %s\n"
		"Address:   %p\n\n",
		allocationCount, time, func, line, totalSize, (double) totalSize / 1024, (double) totalSize / 1048576, size, alignment, sAllocationScope, memory
	);
}

void* allocation_callback(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	CallbackData data = *(CallbackData*) pUserData;

	void* memory = size ? aligned_malloc(size, alignment) : NULL;

	g_allocCount++;
	g_totalAllocSize += size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return memory; }

	print_allocation_callback(file, g_allocCount, data.funcName, data.lineNum, g_totalAllocSize, size, alignment, allocationScope, memory);

	fclose(file);
	return memory;
}

static void print_reallocation_callback(
	FILE* stream,
	uint64_t reallocationCount,
	const char* func,
	uint64_t line,
	size_t totalSize,
	size_t originalSize,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* originalAddr,
	const void* memory)
{
	clock_t time = program_time();
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream,
		"Reallocation callback %" PRIu64 " (%ld ms)\n"
		"%s (%" PRIu64 ")\n"
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Original size:     %zu\n"
		"Allocated size:    %zu\n"
		"Alignment:         %zu\n"
		"Scope:             %s\n"
		"Original address:  %p\n"
		"Allocated address: %p\n\n",
		reallocationCount, time, func, line, totalSize, (double) totalSize / 1024, (double) totalSize / 1048576, originalSize, size, alignment, sAllocationScope, originalAddr, memory
	);
}

void* reallocation_callback(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	CallbackData data = *(CallbackData*) pUserData;

	size_t originalSize;
	void*  memory;

	if (pOriginal) {
		originalSize = aligned_size(pOriginal);
		memory       = size ? aligned_realloc(pOriginal, size, alignment) : aligned_free(pOriginal);
	}
	else {
		originalSize = 0;
		memory       = size ? aligned_malloc(size, alignment) : NULL;
	}

	g_reallocCount++;
	g_totalAllocSize -= originalSize;
	g_totalAllocSize += size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return memory; }

	print_reallocation_callback(file, g_reallocCount, data.funcName, data.lineNum, g_totalAllocSize, originalSize, size, alignment, allocationScope, pOriginal, memory);

	fclose(file);
	return memory;
}

static void print_free_callback(
	FILE* stream,
	uint64_t freeCount,
	const char* func,
	uint64_t line,
	size_t totalSize,
	size_t size,
	const void* memory)
{
	clock_t time = program_time();

	fprintf(stream,
		"Free callback %" PRIu64 " (%ld ms)\n"
		"%s (%" PRIu64 ")\n"
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Size:    %zu\n"
		"Address: %p\n\n",
		freeCount, time, func, line, totalSize, (double) totalSize / 1024, (double) totalSize / 1048576, size, memory
	);
}

void free_callback(void* pUserData, void* pMemory)
{
	CallbackData data = *(CallbackData*) pUserData;

	size_t size;

	if (pMemory) {
		size = aligned_size(pMemory);
		aligned_free(pMemory);
	}
	else {
		size = 0;
	}

	g_freeCount++;
	g_totalAllocSize -= size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	print_free_callback(file, g_freeCount, data.funcName, data.lineNum, g_totalAllocSize, size, pMemory);

	fclose(file);
}

static void print_internal_allocation_callback(
	FILE* stream,
	uint64_t internalAllocationCount,
	const char* func,
	uint64_t line,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = program_time();
	const char* sAllocationType  = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream,
		"Internal allocation callback %" PRIu64 " (%ld ms)\n"
		"%s (%" PRIu64 ")\n"
		"Size:  %zu\n"
		"Type:  %s\n"
		"Scope: %s\n\n",
		internalAllocationCount, time, func, line, size, sAllocationType, sAllocationScope
	);
}

void internal_allocation_callback(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	CallbackData data = *(CallbackData*) pUserData;

	g_internalAllocCount++;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	print_internal_allocation_callback(file, g_internalAllocCount, data.funcName, data.lineNum, size, allocationType, allocationScope);

	fclose(file);
}

static void print_internal_free_callback(
	FILE* stream,
	uint64_t internalFreeCount,
	const char* func,
	uint64_t line,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = program_time();
	const char* sAllocationType  = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream,
		"Internal free callback %" PRIu64 " (%ld ms)\n"
		"%s (%" PRIu64 ")\n"
		"Size:  %zu\n"
		"Type:  %s\n"
		"Scope: %s\n\n",
		internalFreeCount, time, func, line, size, sAllocationType, sAllocationScope
	);
}

void internal_free_callback(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	CallbackData data = *(CallbackData*) pUserData;

	g_internalFreeCount++;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	print_internal_free_callback(file, g_internalFreeCount, data.funcName, data.lineNum, size, allocationType, allocationScope);

	fclose(file);
}


void print_malloc_failure(int line, void* result, size_t size)
{
	clock_t time = program_time();

	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'malloc' with void* = %p\n"
		"Arguments:\n"
		"\tsize_t size = %zu\n\n",
		line, time, result, size
	);
}

void print_calloc_failure(int line, void* result, size_t num, size_t size)
{
	clock_t time = program_time();

	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'calloc' with void* = %p\n"
		"Arguments:\n"
		"\tsize_t num = %zu\n"
		"\tsize_t size = %zu\n\n",
		line, time, result, num, size
	);
}

void print_realloc_failure(int line, void* result, void* ptr, size_t size)
{
	clock_t time = program_time();

	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'realloc' with void* = %p\n"
		"Arguments:\n"
		"\tvoid* ptr = %p\n"
		"\tsize_t size = %zu\n\n",
		line, time, result, ptr, size
	);
}

void print_fopen_failure(int line, FILE* result, const char* filename, const char* mode)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fopen' with FILE* = %p\n"
		"Arguments:\n"
		"\tconst char* filename = %s\n"
		"\tconst char* mode = %s\n\n",
		line, time, (void*) result, filename, mode
	);
}

void print_fseek_failure(int line, int result, FILE* file, long offset, int origin)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fseek' with int = %d\n"
		"Arguments:\n"
		"\tFILE* file = %p\n"
		"\tlong offset = %ld\n"
		"\tint origin = %d\n\n",
		line, time, result, (void*) file, offset, origin
	);
}

void print_ftell_failure(int line, long result, FILE* file)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'ftell' with long = %ld\n"
		"Arguments:\n"
		"\tFILE* file = %p\n\n",
		line, time, result, (void*) file
	);
}

void print_fread_failure(int line, size_t result, const void* buffer, size_t size, size_t count, FILE* file)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fread' with size_t = %zu\n"
		"Arguments:\n"
		"\tvoid* buffer = %p\n"
		"\tsize_t size = %zu\n"
		"\tsize_t count = %zu\n"
		"\tFILE* file = %p\n\n",
		line, time, result, buffer, size, count, (void*) file
	);
}

void print_fwrite_failure(int line, size_t result, const void* buffer, size_t size, size_t count, FILE* file)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fwrite' with size_t = %zu\n"
		"Arguments:\n"
		"\tconst void* buffer = %p\n"
		"\tsize_t size = %zu\n"
		"\tsize_t count = %zu\n"
		"\tFILE* file = %p\n\n",
		line, time, result, buffer, size, count, (void*) file
	);
}

void print_fprintf_failure(int line, int result, FILE* file, const char* format)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fprintf' with int = %d\n"
		"Arguments:\n"
		"\tFILE* file = %p\n"
		"\tconst char* format = %s\n\n",
		line, time, result, (void*) file, format
	);
}

void print_fscanf_failure(int line, int result, FILE* file, const char* format)
{
	clock_t time = program_time();

	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fscanf' with int = %d\n"
		"Arguments:\n"
		"\tFILE* file = %p\n"
		"\tconst char* format = %s\n\n",
		line, time, result, (void*) file, format
	);
}

void print_pcreate_failure(int line, int result, pthread_t* thread, pthread_attr_t* attr)
{
	clock_t time = program_time();

	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Failed function call 'pthread_create' with int = %d\n"
		"Arguments:\n"
		"\tpthread_t* thread = %p\n"
		"\tconst pthread_attr_t* attr = %p\n\n",
		line, time, result, (void*) thread, (void*) attr
	);
}

void print_pjoin_failure(int line, int result, pthread_t thread, void** retval)
{
	clock_t time = program_time();

	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Failed function call 'pthread_join' with int = %d\n"
		"Arguments:\n"
		"\tpthread_t thread = 0x%" PRIxPTR "\n"
		"\tvoid** retval = %p\n\n",
		line, time, result, thread, retval
	);
}

void print_pcancel_failure(int line, int result, pthread_t thread)
{
	clock_t time = program_time();

	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Failed function call 'pthread_cancel' with int = %d\n"
		"Arguments:\n"
		"\tpthread_t thread = 0x%" PRIxPTR "\n\n",
		line, time, result, thread
	);
}

void print_vkinit_failure(int line, VkResult result)
{
	clock_t time = program_time();
	const char* sResult = string_VkResult(result);

	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call 'volkInitialize' with VkResult = %s\n\n",
		line, time, sResult
	);
}

void print_vkvers_failure(int line, uint32_t result)
{
	clock_t time = program_time();
	uint32_t variant = VK_API_VERSION_VARIANT(result);
	uint32_t major   = VK_API_VERSION_MAJOR(result);
	uint32_t minor   = VK_API_VERSION_MINOR(result);
	uint32_t patch   = VK_API_VERSION_PATCH(result);

	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call 'volkGetInstanceVersion' with uint32_t = %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n\n",
		line, time, variant, major, minor, patch
	);
}

void print_vulkan_failure(int line, VkResult result, const char* func)
{
	clock_t time = program_time();
	const char* sResult = string_VkResult(result);

	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call '%s' with VkResult = %s\n\n",
		line, time, func, sResult
	);
}
