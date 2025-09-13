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

#include "debug.h"
#include "util.h"


CallbackData g_callbackData = {.func = "", .file = "", .line = 0};

static uint64_t g_debugCallbackCount = 0;
static uint64_t g_allocCount = 0;
static uint64_t g_reallocCount = 0;
static uint64_t g_freeCount = 0;
static uint64_t g_internalAllocCount = 0;
static uint64_t g_internalFreeCount = 0;
static size_t g_totalAllocSize = 0;


bool init_debug_logfile(void)
{
	double programTime = program_time();
	const char* sCurrTime = stime();

	bool bres = write_text(
		DEBUG_LOG_NAME,
		"VULKAN DEBUG CALLBACK LOGFILE\n"
		"PROGRAM: %s %" PRIu32 ".%" PRIu32 ".%" PRIu32 " (%s)\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %.3fms\n\n",
		PROGRAM_NAME,
		PROGRAM_VER_MAJOR, PROGRAM_VER_MINOR, PROGRAM_VER_PATCH,
		PROGRAM_EXE,
		sCurrTime, programTime);

	if EXPECT_FALSE (!bres) { return false; }
	return true;
}

bool init_alloc_logfile(void)
{
	double programTime = program_time();
	const char* sCurrTime = stime();

	bool bres = write_text(
		ALLOC_LOG_NAME,
		"VULKAN ALLOCATION CALLBACK LOGFILE\n"
		"PROGRAM: %s %" PRIu32 ".%" PRIu32 ".%" PRIu32 " (%s)\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %.3fms\n\n",
		PROGRAM_NAME,
		PROGRAM_VER_MAJOR, PROGRAM_VER_MINOR, PROGRAM_VER_PATCH,
		PROGRAM_EXE,
		sCurrTime, programTime);

	if EXPECT_FALSE (!bres) { return false; }
	return true;
}

#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wformat-nonliteral"
	#pragma GCC diagnostic ignored "-Wmissing-format-attribute"
#endif

static bool log_colour(
	FILE* stream,
	const char* fmt,
	const char* sgr1,
	const char* sgr2,
	const char* prefix,
	const char* postfix,
	va_list args)
{
	ColourLevel colourLevel = g_config.colourLevel;
	bool tty = fisatty(stream);

	/* 
	 * Different methods of printing are used depending on whether the stream is a TTY, according to their benchmarked
	 * speed in each case.
	 */

	if (colourLevel == COLOUR_LEVEL_ALL && !tty) {
		fputs(sgr1, stream);
		fputs(prefix, stream);
		vfprintf(stream, fmt, args);
		fputs(postfix, stream);
		fputs(sgr2, stream);
	}
	else if ((colourLevel == COLOUR_LEVEL_ALL || colourLevel == COLOUR_LEVEL_TTY) && tty) {
		size_t lenSgr1 = strlen(sgr1);
		size_t lenSgr2 = strlen(sgr2);
		size_t lenPre = strlen(prefix);
		size_t lenPost = strlen(postfix);
		size_t lenFmt = strlen(fmt);

		size_t size = lenSgr1 + lenPre + lenFmt + lenPost + lenSgr2 + 1;

		char* newFmt = malloc(size);
		if EXPECT_FALSE (!newFmt) { MALLOC_FAILURE(newFmt, size); return false; }

		strcpy(newFmt, sgr1);
		strcpy(newFmt + lenSgr1, prefix);
		strcpy(newFmt + lenSgr1 + lenPre, fmt);
		strcpy(newFmt + lenSgr1 + lenPre + lenFmt, postfix);
		strcpy(newFmt + lenSgr1 + lenPre + lenFmt + lenPost, sgr2);

		vfprintf(stream, newFmt, args);
		free(newFmt);
	}
	else if (!tty) {
		fputs(prefix, stream);
		vfprintf(stream, fmt, args);
		fputs(postfix, stream);
	}
	else {
		size_t lenPre = strlen(prefix);
		size_t lenPost = strlen(postfix);
		size_t lenFmt = strlen(fmt);

		size_t size = lenPre + lenFmt + lenPost + 1;

		char* newFmt = malloc(size);
		if EXPECT_FALSE (!newFmt) { MALLOC_FAILURE(newFmt, size); return false; }

		strcpy(newFmt, prefix);
		strcpy(newFmt + lenPre, fmt);
		strcpy(newFmt + lenPre + lenFmt, postfix);

		vfprintf(stream, newFmt, args);
		free(newFmt);
	}

	return true;
}

#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif

bool log_debug(FILE* stream, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	bool bres = log_colour(stream, format, SGR_FG_GREEN, SGR_RESET, "Debug: ", "\n", args);

	va_end(args);
	return bres;
}

bool log_warning(FILE* stream, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	bool bres = log_colour(stream, format, SGR_FG_YELLOW, SGR_RESET, "Warning: ", "\n", args);

	va_end(args);
	return bres;
}

bool log_error(FILE* stream, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	bool bres = log_colour(stream, format, SGR_FG_RED, SGR_RESET, "Error: ", "\n", args);

	va_end(args);
	return bres;
}

bool log_critical(FILE* stream, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	bool bres = log_colour(stream, format, SGR_FG_BLACK SGR_BG_RED, SGR_RESET, "CRITICAL: ", "\n", args);

	va_end(args);
	return bres;
}


static void log_debug_callback(
	FILE* stream,
	double time,
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	uint64_t callbackCount,
	const char* func,
	const char* file,
	uint64_t line)
{
	const char* message = pCallbackData->pMessage;
	const char* messageIdName = pCallbackData->pMessageIdName ?: "";
	int32_t messageIdNumber = pCallbackData->messageIdNumber;

	uint32_t queueLabelCount = pCallbackData->queueLabelCount;
	uint32_t cmdBufLabelCount = pCallbackData->cmdBufLabelCount;
	uint32_t objectCount = pCallbackData->objectCount;

	const char* sMessageSeverity = string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity);

	fprintf(stream, "Debug callback %" PRIu64 " (%.3fms)\n", callbackCount, time);

	if (line) {
		fprintf(stream, "%s (%s, %" PRIu64 ")\n", func, file, line);
	}

	fprintf(stream, "Severity: %s\nTypes:   ", sMessageSeverity);

	for (uint32_t i = 0; i < sizeof(messageTypes) * CHAR_BIT; i++) {
		VkDebugUtilsMessageTypeFlagBitsEXT messageType = messageTypes & (UINT32_C(1) << i);

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
			const char* objectName = pCallbackData->pObjects[i].pObjectName ?: "";
			VkObjectType objectType = pCallbackData->pObjects[i].objectType;
			uint64_t objectHandle = pCallbackData->pObjects[i].objectHandle;

			const char* sObjectType = string_VkObjectType(objectType);

			fprintf(stream, "\t%s (%s, 0x%016" PRIx64 ")\n", objectName, sObjectType, objectHandle);
		}
	}

	fprintf(stream, "%s\n\n", message);
}

VkBool32 debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData)
{
	double time = program_time();
	CallbackData data = *(CallbackData*) pUserData;

	g_debugCallbackCount++;

	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
		log_debug_callback(
			stderr, time, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.func, data.file,
			data.line);
	}
	else if (messageTypes & ~(VkDebugUtilsMessageTypeFlagsEXT) VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT) {
		log_debug_callback(
			stdout, time, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.func, data.file,
			data.line);
	}

	FILE* file = fopen(DEBUG_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, DEBUG_LOG_NAME, "a"); return VK_FALSE; }

	log_debug_callback(
		file, time, messageSeverity, messageTypes, pCallbackData, g_debugCallbackCount, data.func, data.file,
		data.line);

	fclose(file);
	return VK_FALSE;
}


static void log_allocation_callback(
	FILE* stream,
	double time,
	uint64_t allocationCount,
	const char* func,
	const char* file,
	uint64_t line,
	size_t totalSize,
	size_t size,
	size_t alignment,
	VkSystemAllocationScope allocationScope,
	const void* memory)
{
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);
	double totalSizeKiB = (double) totalSize / KiB_SIZE;
	double totalSizeMiB = (double) totalSize / MiB_SIZE;

	fprintf(stream, "Allocation callback %" PRIu64 " (%.3fms)\n", allocationCount, time);

	if (line) {
		fprintf(stream, "%s (%s, %" PRIu64 ")\n", func, file, line);
	}

	fprintf(
		stream,
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Size:      %zu\n"
		"Alignment: %zu\n"
		"Scope:     %s\n"
		"Address:   0x%016" PRIxPTR "\n\n",
		totalSize, totalSizeKiB, totalSizeMiB,
		size, alignment, sAllocationScope, (uintptr_t) memory);
}

void* allocation_callback(void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	double time = program_time();
	CallbackData data = *(CallbackData*) pUserData;

	void* memory = size ? aligned_malloc(size, alignment) : NULL;

	g_allocCount++;
	g_totalAllocSize += size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return memory; }

	log_allocation_callback(
		file, time, g_allocCount, data.func, data.file, data.line, g_totalAllocSize, size, alignment, allocationScope,
		memory);

	fclose(file);
	return memory;
}

static void log_reallocation_callback(
	FILE* stream,
	double time,
	uint64_t reallocationCount,
	const char* func,
	const char* file,
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
	double totalSizeKiB = (double) totalSize / KiB_SIZE;
	double totalSizeMiB = (double) totalSize / MiB_SIZE;

	fprintf(stream, "Reallocation callback %" PRIu64 " (%.3fms)\n", reallocationCount, time);

	if (line) {
		fprintf(stream, "%s (%s, %" PRIu64 ")\n", func, file, line);
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
		totalSize, totalSizeKiB, totalSizeMiB,
		originalSize, size, alignment, sAllocationScope,
		(uintptr_t) originalAddr, (uintptr_t) memory);
}

void* reallocation_callback(
	void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope)
{
	double time = program_time();
	CallbackData data = *(CallbackData*) pUserData;

	size_t originalSize = 0;
	void* memory = NULL;

	if (pOriginal && size) {
		originalSize = aligned_size(pOriginal);
		memory = aligned_realloc(pOriginal, size, alignment);
	}
	else if (pOriginal) {
		originalSize = aligned_size(pOriginal);
		aligned_free(pOriginal);
	}
	else if (size) {
		memory = aligned_malloc(size, alignment);
	}

	g_reallocCount++;
	g_totalAllocSize -= originalSize;
	g_totalAllocSize += size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return memory; }

	log_reallocation_callback(
		file, time, g_reallocCount, data.func, data.file, data.line, g_totalAllocSize, originalSize, size, alignment,
		allocationScope, pOriginal, memory);

	fclose(file);
	return memory;
}

static void log_free_callback(
	FILE* stream,
	double time,
	uint64_t freeCount,
	const char* func,
	const char* file,
	uint64_t line,
	size_t totalSize,
	size_t size,
	const void* memory)
{
	double totalSizeKiB = (double) totalSize / KiB_SIZE;
	double totalSizeMiB = (double) totalSize / MiB_SIZE;

	fprintf(stream, "Free callback %" PRIu64 " (%.3fms)\n", freeCount, time);

	if (line) {
		fprintf(stream, "%s (%s, %" PRIu64 ")\n", func, file, line);
	}

	fprintf(
		stream,
		"Memory usage: %zu B (%.2f KiB, %.2f MiB)\n"
		"Size:    %zu\n"
		"Address: 0x%016" PRIxPTR "\n\n",
		totalSize, totalSizeKiB, totalSizeMiB,
		size, (uintptr_t) memory);
}

void free_callback(void* pUserData, void* pMemory)
{
	double time = program_time();
	CallbackData data = *(CallbackData*) pUserData;

	size_t size = 0;

	if (pMemory) {
		size = aligned_size(pMemory);
		aligned_free(pMemory);
	}

	g_freeCount++;
	g_totalAllocSize -= size;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	log_free_callback(file, time, g_freeCount, data.func, data.file, data.line, g_totalAllocSize, size, pMemory);
	fclose(file);
}

static void log_internal_allocation_callback(
	FILE* stream,
	double time,
	uint64_t internalAllocationCount,
	const char* func,
	const char* file,
	uint64_t line,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	const char* sAllocationType = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream, "Internal allocation callback %" PRIu64 " (%.3fms)\n", internalAllocationCount, time);

	if (line) {
		fprintf(stream, "%s (%s, %" PRIu64 ")\n", func, file, line);
	}

	fprintf(
		stream,
		"Size:  %zu\n"
		"Type:  %s\n"
		"Scope: %s\n\n",
		size, sAllocationType, sAllocationScope);
}

void internal_allocation_callback(
	void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	double time = program_time();
	CallbackData data = *(CallbackData*) pUserData;

	g_internalAllocCount++;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	log_internal_allocation_callback(
		file, time, g_internalAllocCount, data.func, data.file, data.line, size, allocationType, allocationScope);

	fclose(file);
}

static void log_internal_free_callback(
	FILE* stream,
	double time,
	uint64_t internalFreeCount,
	const char* func,
	const char* file,
	uint64_t line,
	size_t size,
	VkInternalAllocationType allocationType,
	VkSystemAllocationScope allocationScope)
{
	const char* sAllocationType = string_VkInternalAllocationType(allocationType);
	const char* sAllocationScope = string_VkSystemAllocationScope(allocationScope);

	fprintf(stream, "Internal free callback %" PRIu64 " (%.3fms)\n", internalFreeCount, time);

	if (line) {
		fprintf(stream, "%s (%s, %" PRIu64 ")\n", func, file, line);
	}

	fprintf(
		stream,
		"Size:  %zu\n"
		"Type:  %s\n"
		"Scope: %s\n\n",
		size, sAllocationType, sAllocationScope);
}

void internal_free_callback(
	void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope)
{
	double time = program_time();
	CallbackData data = *(CallbackData*) pUserData;

	g_internalFreeCount++;

	FILE* file = fopen(ALLOC_LOG_NAME, "a");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, ALLOC_LOG_NAME, "a"); return; }

	log_internal_free_callback(
		file, time, g_internalFreeCount, data.func, data.file, data.line, size, allocationType, allocationScope);

	fclose(file);
}


void log_malloc_failure(int line, void* res, size_t size)
{
	double time = program_time();

	log_error(
		stderr,
		"Memory failure at line %d (%.3fms)\n"
		"Failed function call 'malloc' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tsize = %zu\n",
		line, time, (uintptr_t) res, size);
}

void log_calloc_failure(int line, void* res, size_t num, size_t size)
{
	double time = program_time();

	log_error(
		stderr,
		"Memory failure at line %d (%.3fms)\n"
		"Failed function call 'calloc' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tnum  = %zu\n"
		"\tsize = %zu\n",
		line, time, (uintptr_t) res, num, size);
}

void log_realloc_failure(int line, void* res, void* ptr, size_t size)
{
	double time = program_time();

	log_error(
		stderr,
		"Memory failure at line %d (%.3fms)\n"
		"Failed function call 'realloc' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tptr  = 0x%016" PRIxPTR "\n"
		"\tsize = %zu\n",
		line, time, (uintptr_t) res, (uintptr_t) ptr, size);
}

void log_fopen_failure(int line, FILE* res, const char* name, const char* mode)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'fopen' with 0x%016" PRIxPTR "\n"
		"Arguments:\n"
		"\tname = %s\n"
		"\tmode = %s\n",
		line, time, (uintptr_t) res, name, mode);
}

void log_fseek_failure(int line, int res, FILE* file, long offset, int origin)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'fseek' with %d\n"
		"Arguments:\n"
		"\tfile   = 0x%016" PRIxPTR "\n"
		"\toffset = %ld\n"
		"\torigin = %d\n",
		line, time, res, (uintptr_t) file, offset, origin);
}

void log_ftell_failure(int line, long res, FILE* file)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'ftell' with %ld\n"
		"Arguments:\n"
		"\tfile = 0x%016" PRIxPTR "\n",
		line, time, res, (uintptr_t) file);
}

void log_fread_failure(int line, size_t res, const void* buf, size_t size, size_t count, FILE* file)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'fread' with %zu\n"
		"Arguments:\n"
		"\tbuffer = 0x%016" PRIxPTR "\n"
		"\tsize   = %zu\n"
		"\tcount  = %zu\n"
		"\tfile   = 0x%016" PRIxPTR "\n",
		line, time, res, (uintptr_t) buf, size, count, (uintptr_t) file);
}

void log_fwrite_failure(int line, size_t res, const void* buf, size_t size, size_t count, FILE* file)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'fwrite' with %zu\n"
		"Arguments:\n"
		"\tbuffer = 0x%016" PRIxPTR "\n"
		"\tsize   = %zu\n"
		"\tcount  = %zu\n"
		"\tfile   = 0x%016" PRIxPTR "\n",
		line, time, res, (uintptr_t) buf, size, count, (uintptr_t) file);
}

void log_fscanf_failure(int line, int res, FILE* file, const char* fmt)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'fscanf' with %d\n"
		"Arguments:\n"
		"\tfile   = 0x%016" PRIxPTR "\n"
		"\tformat = %s\n",
		line, time, res, (uintptr_t) file, fmt);
}

void log_fprintf_failure(int line, int res, FILE* file, const char* fmt)
{
	double time = program_time();

	log_error(
		stderr,
		"IO error at line %d (%.3fms)\n"
		"Failed function call 'fprintf' with %d\n"
		"Arguments:\n"
		"\tfile   = 0x%016" PRIxPTR "\n"
		"\tformat = %s\n",
		line, time, res, (uintptr_t) file, fmt);
}

void log_pcreate_failure(int line, int res)
{
	double time = program_time();

	log_error(
		stderr,
		"Thread failure at line %d (%.3fms)\n"
		"Failed function call 'pthread_create' with %d\n",
		line, time, res);
}

void log_pcancel_failure(int line, int res)
{
	double time = program_time();

	log_error(
		stderr,
		"Thread failure at line %d (%.3fms)\n"
		"Failed function call 'pthread_cancel' with %d\n",
		line, time, res);
}

void log_pjoin_failure(int line, int res)
{
	double time = program_time();

	log_error(
		stderr,
		"Thread failure at line %d (%.3fms)\n"
		"Failed function call 'pthread_join' with %d\n",
		line, time, res);
}

void log_vkinit_failure(int line, VkResult res)
{
	double time = program_time();
	const char* sRes = string_VkResult(res);

	log_error(
		stderr,
		"Vulkan failure at line %d (%.3fms)\n"
		"Failed function call 'volkInitialize' with %s\n",
		line, time, sRes);
}

void log_vkvers_failure(int line, uint32_t res)
{
	double time = program_time();

	uint32_t variant = VK_API_VERSION_VARIANT(res);
	uint32_t major = VK_API_VERSION_MAJOR(res);
	uint32_t minor = VK_API_VERSION_MINOR(res);
	uint32_t patch = VK_API_VERSION_PATCH(res);

	log_error(
		stderr,
		"Vulkan failure at line %d (%.3fms)\n"
		"Failed function call 'volkGetInstanceVersion' with %" PRIu32 ".%" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n",
		line, time, variant, major, minor, patch);
}

void log_vulkan_failure(int line, VkResult res, const char* func)
{
	double time = program_time();
	const char* sRes = string_VkResult(res);

	log_error(
		stderr,
		"Vulkan failure at line %d (%.3fms)\n"
		"Failed function call '%s' with %s\n",
		line, time, func, sRes);
}
