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
	clock_t     programTime = program_time();
	const char* sCurrTime   = stime();

	FILE* file = fopen(DEBUG_LOG_NAME, "w");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, DEBUG_LOG_NAME, "w"); return false; }

	fprintf(
		file,
		"VULKAN DEBUG CALLBACK LOGFILE\n"
		"PROGRAM: %s %" PRIu32 ".%" PRIu32 ".%" PRIu32 " (%s)\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %ldms\n\n",
		PROGRAM_NAME, PROGRAM_VER_MAJOR, PROGRAM_VER_MINOR, PROGRAM_VER_PATCH, PROGRAM_EXE,
		sCurrTime, programTime);

	fclose(file);

	return true;
}

bool init_alloc_logfile(void)
{
	clock_t     programTime = program_time();
	const char* sCurrTime   = stime();

	FILE* file = fopen(ALLOC_LOG_NAME, "w");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "w"); return false; }

	fprintf(
		file,
		"VULKAN ALLOCATION CALLBACK LOGFILE\n"
		"PROGRAM: %s %" PRIu32 ".%" PRIu32 ".%" PRIu32 " (%s)\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %ldms\n\n",
		PROGRAM_NAME, PROGRAM_VER_MAJOR, PROGRAM_VER_MINOR, PROGRAM_VER_PATCH, PROGRAM_EXE,
		sCurrTime, programTime);

	fclose(file);

	return true;
}


static void print_debug_callback(
	FILE* restrict stream,
	clock_t time,
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* restrict pCallbackData,
	uint64_t callbackCount,
	const char* restrict func,
	uint64_t line)
{
	const char* messageIdName   = pCallbackData->pMessageIdName ?: "";
	int32_t     messageIdNumber = pCallbackData->messageIdNumber;
	const char* message         = pCallbackData->pMessage;

	uint32_t queueLabelCount  = pCallbackData->queueLabelCount;
	uint32_t cmdBufLabelCount = pCallbackData->cmdBufLabelCount;
	uint32_t objectCount      = pCallbackData->objectCount;

	const char* sMessageSeverity = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);

	fprintf(stream, "Debug callback %" PRIu64 " (%ldms)\n", callbackCount, time);

	if (line) {
		fprintf(stream, "%s (%" PRIu64 ")\n", func, line);
	}

	fprintf(stream, "Severity: %s\nTypes:   ", sMessageSeverity);

	for (uint32_t i = 0; i < sizeof(messageTypes) * CHAR_BIT; i++) {
		VkDebugUtilsMessageTypeFlagBitsEXT messageType = messageTypes & ((uint32_t) 1 << i);

		if (messageType) {
			const char* sMessageType = string_VkDebugUtilsMessageTypeFlagBitsEXT(messageType);
			fprintf(stream, " %s", sMessageType);
		}
	}

	fprintf(stream, "\nID:       %s (%" PRId32 ")\n", messageIdName, messageIdNumber);

	// VkDebugUtilsLabelEXT active in the current VkQueue
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

	fprintf(stream, "%s\n\n", message);
}

VkBool32 debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* restrict pCallbackData,
	void* restrict pUserData)
{
	clock_t time = program_time();

	CallbackData data = *(CallbackData*) pUserData;

	g_debugCallbackCount++;

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		print_debug_callback(
			stderr, time, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.funcName,
			data.lineNum);
	}
	else if (messageTypes & ~(VkDebugUtilsMessageTypeFlagsEXT) VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		print_debug_callback(
			stdout, time, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.funcName,
			data.lineNum);
	}

	FILE* file = fopen(DEBUG_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, DEBUG_LOG_NAME, "a"); return VK_FALSE; }

	print_debug_callback(
		file, time, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.funcName, data.lineNum);

	fclose(file);

	return VK_FALSE;
}


static void print_allocation_callback(
	FILE* restrict stream,
	clock_t time,
	uint64_t allocationCount,
	const char* restrict func,
	uint64_t line,
	size_t totalSize,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* restrict memory)
{
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream, "Allocation callback %" PRIu64 " (%ldms)\n", allocationCount, time);

	if (line) {
		fprintf(stream, "%s (%" PRIu64 ")\n", func, line);
	}

	fprintf(
		stream,
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Size:      %zu\n"
		"Alignment: %zu\n"
		"Scope:     %s\n"
		"Address:   0x%016" PRIxPTR "\n\n",
		totalSize, (double) totalSize / 1024, (double) totalSize / 1048576,
		size, alignment, sAllocationScope, (uintptr_t) memory);
}

void* allocation_callback(
	void* restrict pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	clock_t time = program_time();

	CallbackData data = *(CallbackData*) pUserData;

	void* memory = size ? aligned_malloc(size, alignment) : NULL;

	g_allocCount++;
	g_totalAllocSize += size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return memory; }

	print_allocation_callback(
		file, time, g_allocCount, data.funcName, data.lineNum, g_totalAllocSize, size, alignment, allocationScope,
		memory);

	fclose(file);

	return memory;
}

static void print_reallocation_callback(
	FILE* restrict stream,
	clock_t time,
	uint64_t reallocationCount,
	const char* restrict func,
	uint64_t line,
	size_t totalSize,
	size_t originalSize,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* originalAddr,
	const void* memory)
{
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream, "Reallocation callback %" PRIu64 " (%ldms)\n", reallocationCount, time);

	if (line) {
		fprintf(stream, "%s (%" PRIu64 ")\n", func, line);
	}

	fprintf(
		stream,
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Original size:     %zu\n"
		"Allocated size:    %zu\n"
		"Alignment:         %zu\n"
		"Scope:             %s\n"
		"Original address:  0x%016" PRIxPTR "\n"
		"Allocated address: 0x%016" PRIxPTR "\n\n",
		totalSize, (double) totalSize / 1024, (double) totalSize / 1048576,
		originalSize, size, alignment, sAllocationScope, (uintptr_t) originalAddr, (uintptr_t) memory);
}

void* reallocation_callback(
	void* restrict pUserData,
	void* restrict pOriginal,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = program_time();

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

	print_reallocation_callback(
		file, time, g_reallocCount, data.funcName, data.lineNum, g_totalAllocSize, originalSize, size, alignment,
		allocationScope, pOriginal, memory);

	fclose(file);

	return memory;
}

static void print_free_callback(
	FILE* restrict stream,
	clock_t time,
	uint64_t freeCount,
	const char* restrict func,
	uint64_t line,
	size_t totalSize,
	size_t size,
	const void* restrict memory)
{
	fprintf(stream, "Free callback %" PRIu64 " (%ldms)\n", freeCount, time);

	if (line) {
		fprintf(stream, "%s (%" PRIu64 ")\n", func, line);
	}

	fprintf(
		stream,
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Size:    %zu\n"
		"Address: 0x%016" PRIxPTR "\n\n",
		totalSize, (double) totalSize / 1024, (double) totalSize / 1048576,
		size, (uintptr_t) memory);
}

void free_callback(void* restrict pUserData, void* restrict pMemory)
{
	clock_t time = program_time();

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

	print_free_callback(file, time, g_freeCount, data.funcName, data.lineNum, g_totalAllocSize, size, pMemory);

	fclose(file);
}

static void print_internal_allocation_callback(
	FILE* restrict stream,
	clock_t time,
	uint64_t internalAllocationCount,
	const char* restrict func,
	uint64_t line,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	const char* sAllocationType  = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream, "Internal allocation callback %" PRIu64 " (%ldms)\n", internalAllocationCount, time);

	if (line) {
		fprintf(stream, "%s (%" PRIu64 ")\n", func, line);
	}

	fprintf(
		stream,
		"Size:  %zu\n"
		"Type:  %s\n"
		"Scope: %s\n\n",
		size, sAllocationType, sAllocationScope);
}

void internal_allocation_callback(
	void* restrict pUserData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = program_time();

	CallbackData data = *(CallbackData*) pUserData;

	g_internalAllocCount++;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	print_internal_allocation_callback(
		file, time, g_internalAllocCount, data.funcName, data.lineNum, size, allocationType, allocationScope);

	fclose(file);
}

static void print_internal_free_callback(
	FILE* restrict stream,
	clock_t time,
	uint64_t internalFreeCount,
	const char* restrict func,
	uint64_t line,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	const char* sAllocationType  = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream, "Internal free callback %" PRIu64 " (%ldms)\n", internalFreeCount, time);

	if (line) {
		fprintf(stream, "%s (%" PRIu64 ")\n", func, line);
	}

	fprintf(
		stream,
		"Size:  %zu\n"
		"Type:  %s\n"
		"Scope: %s\n\n",
		size, sAllocationType, sAllocationScope);
}

void internal_free_callback(
	void* restrict pUserData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = program_time();

	CallbackData data = *(CallbackData*) pUserData;

	g_internalFreeCount++;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	print_internal_free_callback(
		file, time, g_internalFreeCount, data.funcName, data.lineNum, size, allocationType, allocationScope);

	fclose(file);
}


void print_malloc_failure(int line, void* result, size_t size)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"Memory failure at line %d (%ldms)\n"
		"Failed function call 'malloc' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tsize = %zu\n\n",
		line, time, (uintptr_t) result, size);
}

void print_calloc_failure(int line, void* result, size_t num, size_t size)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"Memory failure at line %d (%ldms)\n"
		"Failed function call 'calloc' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tnum  = %zu\n"
		"\tsize = %zu\n\n",
		line, time, (uintptr_t) result, num, size);
}

void print_realloc_failure(int line, void* result, void* ptr, size_t size)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"Memory failure at line %d (%ldms)\n"
		"Failed function call 'realloc' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tptr  = 0x%016" PRIxPTR "\n"
		"\tsize = %zu\n\n",
		line, time, (uintptr_t) result, (uintptr_t) ptr, size);
}

void print_fopen_failure(int line, FILE* result, const char* filename, const char* mode)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'fopen' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tfilename = %s\n"
		"\tmode     = %s\n\n",
		line, time, (uintptr_t) result, filename, mode);
}

void print_fseek_failure(int line, int result, FILE* file, long offset, int origin)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'fseek' with %d\n"
		"Arguments:\n"
		"\tfile   = 0x%016" PRIxPTR "\n"
		"\toffset = %ld\n"
		"\torigin = %d\n\n",
		line, time, result, (uintptr_t) file, offset, origin);
}

void print_ftell_failure(int line, long result, FILE* file)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'ftell' with %ld\n"
		"Arguments:\n"
		"\tfile = 0x%016" PRIxPTR "\n\n",
		line, time, result, (uintptr_t) file);
}

void print_fread_failure(int line, size_t result, const void* buffer, size_t size, size_t count, FILE* file)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'fread' with %zu\n"
		"Arguments:\n"
		"\tbuffer = 0x%016" PRIxPTR "\n"
		"\tsize   = %zu\n"
		"\tcount  = %zu\n"
		"\tfile   = 0x%016" PRIxPTR "\n\n",
		line, time, result, (uintptr_t) buffer, size, count, (uintptr_t) file);
}

void print_fwrite_failure(int line, size_t result, const void* buffer, size_t size, size_t count, FILE* file)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'fwrite' with %zu\n"
		"Arguments:\n"
		"\tbuffer = 0x%016" PRIxPTR "\n"
		"\tsize   = %zu\n"
		"\tcount  = %zu\n"
		"\tfile   = 0x%016" PRIxPTR "\n\n",
		line, time, result, (uintptr_t) buffer, size, count, (uintptr_t) file);
}

void print_fscanf_failure(int line, int result, FILE* file, const char* format)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'fscanf' with %d\n"
		"Arguments:\n"
		"\tfile   = 0x%016" PRIxPTR "\n"
		"\tformat = %s\n\n",
		line, time, result, (uintptr_t) file, format);
}

void print_fprintf_failure(int line, int result, FILE* file, const char* format)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"IO error at line %d (%ldms)\n"
		"Failed function call 'fprintf' with %d\n"
		"Arguments:\n"
		"\tfile   = 0x%016" PRIxPTR "\n"
		"\tformat = %s\n\n",
		line, time, result, (uintptr_t) file, format);
}

void print_pcreate_failure(int line, int result, pthread_t* thread, pthread_attr_t* attr)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"Thread failure at line %d (%ldms)\n"
		"Failed function call 'pthread_create' with %d\n"
		"Arguments:\n"
		"\tthread = 0x%016" PRIxPTR "\n"
		"\tattr   = 0x%016" PRIxPTR "\n\n",
		line, time, result, (uintptr_t) thread, (uintptr_t) attr);
}

void print_pjoin_failure(int line, int result, pthread_t thread, void** retval)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"Thread failure at line %d (%ldms)\n"
		"Failed function call 'pthread_join' with %d\n"
		"Arguments:\n"
		"\tthread = 0x%016" PRIxPTR "\n"
		"\tretval = 0x%016" PRIxPTR "\n\n",
		line, time, result, thread, (uintptr_t) retval);
}

void print_pcancel_failure(int line, int result, pthread_t thread)
{
	clock_t time = program_time();

	fprintf(
		stderr,
		"Thread failure at line %d (%ldms)\n"
		"Failed function call 'pthread_cancel' with %d\n"
		"Arguments:\n"
		"\tthread = 0x%016" PRIxPTR "\n\n",
		line, time, result, thread);
}

void print_vkinit_failure(int line, VkResult result)
{
	clock_t time = program_time();

	const char* sResult = string_VkResult(result);

	fprintf(
		stderr,
		"Vulkan failure at line %d (%ldms)\n"
		"Failed function call 'volkInitialize' with %s\n\n",
		line, time, sResult);
}

void print_vkvers_failure(int line, uint32_t result)
{
	clock_t time = program_time();

	uint32_t variant = VK_API_VERSION_VARIANT(result);
	uint32_t major   = VK_API_VERSION_MAJOR(result);
	uint32_t minor   = VK_API_VERSION_MINOR(result);
	uint32_t patch   = VK_API_VERSION_PATCH(result);

	fprintf(
		stderr,
		"Vulkan failure at line %d (%ldms)\n"
		"Failed function call 'volkGetInstanceVersion' with %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n\n",
		line, time, variant, major, minor, patch);
}

void print_vulkan_failure(int line, VkResult result, const char* func)
{
	clock_t time = program_time();

	const char* sResult = string_VkResult(result);

	fprintf(
		stderr,
		"Vulkan failure at line %d (%ldms)\n"
		"Failed function call '%s' with %s\n\n",
		line, time, func, sResult);
}
