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

#include "defs.h"

#include <vulkan/vk_enum_string_helper.h> // vulkan.h, stddef.h, stdint.h
#include <stdarg.h>


// ===== Initialisation functions =====

bool init_debug_logfile(void)
{
	time_t currentTime = time(NULL);
	clock_t programTime = PROGRAM_TIME;

	FILE* file = fopen(DEBUG_LOG_NAME, "w");
	if (!file) {
		FOPEN_FAILURE(DEBUG_LOG_NAME, "w")
		return false;
	}

	const char* sCurrentTime = ctime(&currentTime);
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
	time_t currentTime = time(NULL);
	clock_t programTime = PROGRAM_TIME;

	FILE* file = fopen(ALLOC_LOG_NAME, "w");
	if (!file) {
		FOPEN_FAILURE(ALLOC_LOG_NAME, "w")
		return false;
	}

	const char* sCurrentTime = ctime(&currentTime);
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


// ===== Debug callback functions =====

static void print_debug_callback(
	FILE* restrict stream,
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* restrict pCallbackData,
	uint64_t userData)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stream,
		"Vulkan debug callback %llu (%ld ms)\n",
		userData, time
	);

	const char* sMessageSeverity = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);
	fprintf(stream, "Severity: %s\n", sMessageSeverity);

	fputs("Types:", stream);
	for (uint8_t i = 0; i < CHAR_BIT * sizeof(messageTypes); i++) {
		VkDebugUtilsMessageTypeFlagBitsEXT messageType = messageTypes & 1U << i;
		if (messageType) {
			const char* sMessageType = string_VkDebugUtilsMessageTypeFlagBitsEXT(messageType);
			fprintf(stream, " %s", sMessageType);
		}
	}
	putc('\n', stream);

	fprintf(stream,
		"ID: %s (0x%x)\n",
		pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "",
		(unsigned int) pCallbackData->messageIdNumber
	);

	// VkDebugUtilsLabelEXT active in the current VkQueue
	fprintf(stream, "Queue labels (%u):\n", pCallbackData->queueLabelCount);
	for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
		fprintf(stream,
			"\t%s (R%f G%f B%f A%f)\n",
			pCallbackData->pQueueLabels[i].pLabelName,
			(double) pCallbackData->pQueueLabels[i].color[0],
			(double) pCallbackData->pQueueLabels[i].color[1],
			(double) pCallbackData->pQueueLabels[i].color[2],
			(double) pCallbackData->pQueueLabels[i].color[3]
		);
	}
	
	// VkDebugUtilsLabelEXT active in the current VkCommandBuffer
	fprintf(stream, "Command buffer labels (%u):\n", pCallbackData->cmdBufLabelCount);
	for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
		fprintf(stream,
			"\t%s (R%f G%f B%f A%f)\n",
			pCallbackData->pCmdBufLabels[i].pLabelName,
			(double) pCallbackData->pCmdBufLabels[i].color[0],
			(double) pCallbackData->pCmdBufLabels[i].color[1],
			(double) pCallbackData->pCmdBufLabels[i].color[2],
			(double) pCallbackData->pCmdBufLabels[i].color[3]
		);
	}

	// VkDebugUtilsObjectNameInfoEXT related to the callback
	fprintf(stream, "Objects (%u):\n", pCallbackData->objectCount);
	for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
		fprintf(stream,
			"\t%s (Type: %s, Handle: 0x%llx)\n",
			pCallbackData->pObjects[i].pObjectName ? pCallbackData->pObjects[i].pObjectName : "",
			string_VkObjectType(pCallbackData->pObjects[i].objectType),
			pCallbackData->pObjects[i].objectHandle
		);
	}

	fprintf(stream, "%s\n\n", pCallbackData->pMessage);
}

VkBool32 debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	uint64_t* callbackCount = (uint64_t*) pUserData;

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
		print_debug_callback(stderr, messageSeverity, messageTypes, pCallbackData, *callbackCount);
	else if (messageTypes & ~(VkDebugUtilsMessageTypeFlagsEXT) VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
		print_debug_callback(stdout, messageSeverity, messageTypes, pCallbackData, *callbackCount);

	FILE* file = fopen(DEBUG_LOG_NAME, "a");
	if (file) {
		print_debug_callback(file, messageSeverity, messageTypes, pCallbackData, *callbackCount);
		fclose(file);
	}
	else
		FOPEN_FAILURE(DEBUG_LOG_NAME, "a")

	(*callbackCount)++;
	return VK_FALSE;
}


// ===== Allocation callback functions =====
#if LOG_VULKAN_ALLOCATIONS

static void print_allocation_callback(
	FILE* restrict stream,
	uint64_t userData,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* restrict memory)
{
	clock_t time = PROGRAM_TIME;
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host allocation callback %llu (%ld ms)\n"
		"Size: %zu\n"
		"Alignment: %zu\n"
		"Allocation scope: %s\n"
		"Allocated address: %p\n\n",
		userData, time, size, alignment, sAllocationScope, memory
	);
}

void* allocation_callback(
	void* pUserData,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope)
{
	uint64_t* allocationCount = &((AllocationCallbackCounts_t*) pUserData)->allocationCount;

	void* memory = size ? _aligned_malloc(size, alignment) : NULL;
	if (!memory && size)
		print_allocation_callback(stderr, *allocationCount, size, alignment, allocationScope, memory);

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if (file) {
		print_allocation_callback(file, *allocationCount, size, alignment, allocationScope, memory);
		fclose(file);
	}
	else
		FOPEN_FAILURE(ALLOC_LOG_NAME, "a")

	(*allocationCount)++;
	return memory;
}

static void print_reallocation_callback(
	FILE* restrict stream,
	uint64_t userData,
	size_t originalSize,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* restrict originalAddr,
	const void* restrict memory)
{
	clock_t time = PROGRAM_TIME;
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host reallocation callback %llu (%ld ms)\n"
		"Original size: %zu\n"
		"Allocated size: %zu\n"
		"Alignment: %zu\n"
		"Allocation scope: %s\n"
		"Original address: %p\n"
		"Allocated address: %p\n\n",
		userData, time, originalSize, size, alignment, sAllocationScope, originalAddr, memory
	);
}

void* reallocation_callback(
	void* pUserData,
	void* pOriginal,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope)
{
	uint64_t* reallocationCount = &((AllocationCallbackCounts_t*) pUserData)->reallocationCount;

	size_t originalSize = pOriginal ? _aligned_msize(pOriginal, alignment, (size_t) 0) : 0;
	void* memory = pOriginal || size ? _aligned_realloc(pOriginal, size, alignment) : NULL;
	if (!memory && size)
		print_reallocation_callback(stderr, *reallocationCount, originalSize, size, alignment, allocationScope, pOriginal, memory);

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if (file) {
		print_reallocation_callback(file, *reallocationCount, originalSize, size, alignment, allocationScope, pOriginal, memory);
		fclose(file);
	}
	else
		FOPEN_FAILURE(ALLOC_LOG_NAME, "a")

	(*reallocationCount)++;
	return memory;
}

static void print_free_callback(
	FILE* restrict stream,
	uint64_t userData,
	const void* restrict memory)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stream,
		"Vulkan host free callback %llu (%ld ms)\n"
		"Freed address: %p\n\n",
		userData, time, memory
	);
}

void free_callback(
	void* pUserData,
	void* pMemory)
{
	uint64_t* freeCount = &((AllocationCallbackCounts_t*) pUserData)->freeCount;

	_aligned_free(pMemory);

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if (file) {
		print_free_callback(file, *freeCount, pMemory);
		fclose(file);
	}
	else
		FOPEN_FAILURE(ALLOC_LOG_NAME, "a")

	(*freeCount)++;
}

static void print_internal_allocation_callback(
	FILE* restrict stream,
	uint64_t userData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = PROGRAM_TIME;
	const char* sAllocationType = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host internal allocation callback %llu (%ld ms)\n"
		"Size: %zu\n"
		"Allocation type: %s\n"
		"Allocation scope: %s\n\n",
		userData, time, size, sAllocationType, sAllocationScope
	);
}

void internal_allocation_callback(
	void* pUserData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	uint64_t* internalAllocationCount = &((AllocationCallbackCounts_t*) pUserData)->internalAllocationCount;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if (file) {
		print_internal_allocation_callback(file, *internalAllocationCount, size, allocationType, allocationScope);
		fclose(file);
	}
	else
		FOPEN_FAILURE(ALLOC_LOG_NAME, "a")

	(*internalAllocationCount)++;
}

static void print_internal_free_callback(
	FILE* restrict stream,
	uint64_t userData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	clock_t time = PROGRAM_TIME;
	const char* sAllocationType = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host internal free callback %llu (%ld ms)\n"
		"Size: %zu\n"
		"Allocation type: %s\n"
		"Allocation scope: %s\n\n",
		userData, time, size, sAllocationType, sAllocationScope
	);
}

void internal_free_callback(
	void* pUserData,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	uint64_t* internalFreeCount = &((AllocationCallbackCounts_t*) pUserData)->internalFreeCount;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if (file) {
		print_internal_free_callback(file, *internalFreeCount, size, allocationType, allocationScope);
		fclose(file);
	}
	else
		FOPEN_FAILURE(ALLOC_LOG_NAME, "a")

	(*internalFreeCount)++;
}

#endif // LOG_VULKAN_ALLOCATIONS


// ===== Failure functions =====

void print_malloc_failure(
	int line,
	const void* ptr,
	size_t _Size)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'malloc' with void* = %p\n"
		"Arguments:\n"
		"\tsize_t _Size = %zu\n\n",
		line, time, ptr, _Size
	);
}

void print_calloc_failure(
	int line,
	const void* ptr,
	size_t _NumOfElements,
	size_t _SizeOfElements)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'calloc' with void* = %p\n"
		"Arguments:\n"
		"\tsize_t _NumOfElements = %zu\n"
		"\tsize_t _SizeOfElements = %zu\n\n",
		line, time, ptr, _NumOfElements, _SizeOfElements
	);
}

void print_realloc_failure(
	int line,
	const void* ptr,
	const void* _Memory,
	size_t _NewSize)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'realloc' with void* = %p\n"
		"Arguments:\n"
		"\tvoid* _Memory = %p\n"
		"\tsize_t _NewSize = %zu\n\n",
		line, time, ptr, _Memory, _NewSize
	);
}

void print_fopen_failure(
	int line,
	const FILE* file,
	const char* _Filename,
	const char* _Mode)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fopen' with FILE* = %p\n"
		"Arguments:\n"
		"\tconst char* _Filename = %s\n"
		"\tconst char* _Mode = %s\n\n",
		line, time, (const void*) file, _Filename, _Mode
	);
}

void print_fseek_failure(
	int line,
	int result,
	const FILE* _File,
	long _Offset,
	int _Origin)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fseek' with int = %d\n"
		"Arguments:\n"
		"\tFILE* _File = %p\n"
		"\tlong _Offset = %ld\n"
		"\tint _Origin = %d\n\n",
		line, time, result, (const void*) _File, _Offset, _Origin
	);
}

void print_ftell_failure(
	int line,
	long result,
	const FILE* _File)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'ftell' with long = %ld\n"
		"Arguments:\n"
		"\tFILE* _File = %p\n\n",
		line, time, result, (const void*) _File
	);
}

void print_fread_failure(
	int line,
	size_t result,
	const void* _DstBuf,
	size_t _ElementSize,
	size_t _Count,
	const FILE* _File)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fread' with size_t = %zu\n"
		"Arguments:\n"
		"\tvoid* _DstBuf = %p\n"
		"\tsize_t _ElementSize = %zu\n"
		"\tsize_t _Count = %zu\n"
		"\tFILE* _File = %p\n\n",
		line, time, result, _DstBuf, _ElementSize, _Count, (const void*) _File
	);
}

void print_fwrite_failure(
	int line,
	size_t result,
	const void* _Str,
	size_t _Size,
	size_t _Count,
	const FILE* _File)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fwrite' with size_t = %zu\n"
		"Arguments:\n"
		"\tconst void* _Str = %p\n"
		"\tsize_t _Size = %zu\n"
		"\tsize_t _Count = %zu\n"
		"\tFILE* _File = %p\n\n",
		line, time, result, _Str, _Size, _Count, (const void*) _File
	);
}

void print_pcreate_failure(
	int line,
	int result,
	const pthread_t* th,
	const pthread_attr_t* attr,
	const char* func,
	const void* arg)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_create' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_t* th = %p\n"
		"\tconst pthread_attr_t* attr = %p\n"
		"\tvoid* (*func)(void*) = %s\n"
		"\tvoid* arg = %p\n\n",
		line, time, result, (const void*) th, (const void*) attr, func, arg
	);
}

void print_pjoin_failure(
	int line,
	int result,
	pthread_t t,
	const void* const* res)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_join' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_t t = 0x%llx\n"
		"\tvoid** res = %p\n\n",
		line, time, result, t, (const void*) res
	);
}

void print_pcancel_failure(
	int line,
	int result,
	pthread_t t)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_cancel' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_t t = 0x%llx\n\n",
		line, time, result, t
	);
}

void print_pinit_failure(
	int line,
	int result,
	const pthread_mutex_t* m,
	const pthread_mutexattr_t* a)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_init' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n"
		"\tconst pthread_mutexattr_t* a = %p\n\n",
		line, time, result, (const void*) m, (const void*) a
	);
}

void print_plock_failure(
	int line,
	int result,
	const pthread_mutex_t* m)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_lock' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n\n",
		line, time, result, (const void*) m
	);
}

void print_punlock_failure(
	int line,
	int result,
	const pthread_mutex_t* m)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_unlock' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n\n",
		line, time, result, (const void*) m
	);
}

void print_pdestroy_failure(
	int line,
	int result,
	const pthread_mutex_t* m)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_destroy' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n\n",
		line, time, result, (const void*) m
	);
}

void print_vinit_failure(
	int line,
	VkResult result)
{
	clock_t time = PROGRAM_TIME;
	const char* sResult = string_VkResult(result);
	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call 'volkInitialize' with VkResult = %s\n\n",
		line, time, sResult
	);
}

void print_vinstvers_failure(
	int line,
	uint32_t apiVersion)
{
	clock_t time = PROGRAM_TIME;
	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call 'volkGetInstanceVersion' with uint32_t = %u.%u.%u.%u\n\n",
		line, time,
		VK_API_VERSION_VARIANT(apiVersion), VK_API_VERSION_MAJOR(apiVersion),
		VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion)
	);
}

void print_vulkan_failure(
	int line,
	const char* func,
	VkResult result,
	unsigned int count,
	...)
{
	clock_t time = PROGRAM_TIME;
	const char* sResult = string_VkResult(result);
	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call '%s' with VkResult = %s\n"
		"Arguments (%u):\n",
		line, time, func, sResult, count
	);

	va_list args;
	va_start(args, count);

	for (unsigned int i = 0; i < count; i++)
		switch (va_arg(args, int)) {
			case 'c': fprintf(stderr, "\t%u = %c\n"	 , i, va_arg(args, int		   )); break;
			case 's': fprintf(stderr, "\t%u = %s\n"	 , i, va_arg(args, const char* )); break;
			case 'i': fprintf(stderr, "\t%u = %i\n"	 , i, va_arg(args, int		   )); break;
			case 'd': fprintf(stderr, "\t%u = %d\n"	 , i, va_arg(args, int		   )); break;
			case 'u': fprintf(stderr, "\t%u = %u\n"	 , i, va_arg(args, unsigned int)); break;
			case 'o': fprintf(stderr, "\t%u = 0%o\n" , i, va_arg(args, unsigned int)); break;
			case 'x': fprintf(stderr, "\t%u = 0x%x\n", i, va_arg(args, unsigned int)); break;
			case 'X': fprintf(stderr, "\t%u = 0X%X\n", i, va_arg(args, unsigned int)); break;
			case 'f': fprintf(stderr, "\t%u = %f\n"	 , i, va_arg(args, double	   )); break;
			case 'F': fprintf(stderr, "\t%u = %F\n"	 , i, va_arg(args, double	   )); break;
			case 'e': fprintf(stderr, "\t%u = %e\n"	 , i, va_arg(args, double	   )); break;
			case 'E': fprintf(stderr, "\t%u = %E\n"	 , i, va_arg(args, double	   )); break;
			case 'a': fprintf(stderr, "\t%u = %a\n"	 , i, va_arg(args, double	   )); break;
			case 'A': fprintf(stderr, "\t%u = %A\n"	 , i, va_arg(args, double	   )); break;
			case 'g': fprintf(stderr, "\t%u = %g\n"	 , i, va_arg(args, double	   )); break;
			case 'G': fprintf(stderr, "\t%u = %G\n"	 , i, va_arg(args, double	   )); break;
			case 'p': fprintf(stderr, "\t%u = %p\n"	 , i, va_arg(args, void*	   )); break;
			default	: fprintf(stderr, "\t%u = Unknown type\n", i); va_arg(args, int	); break;
		}

	va_end(args);
	NEWLINE
}
