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

#include <stdarg.h>


// ===== Enum stringification functions =====

static const char* stringify_VkResult(const VkResult result)
{
	switch (result) {
		case VK_SUCCESS											  : return "SUCCESS";
		case VK_NOT_READY										  : return "NOT_READY";
		case VK_TIMEOUT											  : return "TIMEOUT";
		case VK_EVENT_SET										  : return "EVENT_SET";
		case VK_EVENT_RESET										  : return "EVENT_RESET";
		case VK_INCOMPLETE										  : return "INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY						  : return "ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY						  : return "ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED						  : return "ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST								  : return "ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED							  : return "ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT							  : return "ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT						  : return "ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT						  : return "ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER						  : return "ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS							  : return "ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED						  : return "ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL							  : return "ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN									  : return "ERROR_UNKNOWN";
		case VK_ERROR_OUT_OF_POOL_MEMORY						  : return "ERROR_OUT_OF_POOL_MEMORY";
		case VK_ERROR_INVALID_EXTERNAL_HANDLE					  : return "ERROR_INVALID_EXTERNAL_HANDLE";
		case VK_ERROR_FRAGMENTATION								  : return "ERROR_FRAGMENTATION";
		case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS			  : return "ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		case VK_PIPELINE_COMPILE_REQUIRED						  : return "PIPELINE_COMPILE_REQUIRED";
		case VK_ERROR_SURFACE_LOST_KHR							  : return "ERROR_SURFACE_LOST_KHR";
		case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR					  : return "ERROR_NATIVE_WINDOW_IN_USE_KHR";
		case VK_SUBOPTIMAL_KHR									  : return "SUBOPTIMAL_KHR";
		case VK_ERROR_OUT_OF_DATE_KHR							  : return "ERROR_OUT_OF_DATE_KHR";
		case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR					  : return "ERROR_INCOMPATIBLE_DISPLAY_KHR";
		case VK_ERROR_VALIDATION_FAILED_EXT						  : return "ERROR_VALIDATION_FAILED_EXT";
		case VK_ERROR_INVALID_SHADER_NV							  : return "ERROR_INVALID_SHADER_NV";
		case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR				  : return "ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR	  : return "ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR	  : return "ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR	  : return "ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR		  : return "ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR		  : return "ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT: return "ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		case VK_ERROR_NOT_PERMITTED_KHR							  : return "ERROR_NOT_PERMITTED_KHR";
		case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT		  : return "ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		case VK_THREAD_IDLE_KHR									  : return "THREAD_IDLE_KHR";
		case VK_THREAD_DONE_KHR									  : return "THREAD_DONE_KHR";
		case VK_OPERATION_DEFERRED_KHR							  : return "OPERATION_DEFERRED_KHR";
		case VK_OPERATION_NOT_DEFERRED_KHR						  : return "OPERATION_NOT_DEFERRED_KHR";
		case VK_ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR			  : return "ERROR_INVALID_VIDEO_STD_PARAMETERS_KHR";
		case VK_ERROR_COMPRESSION_EXHAUSTED_EXT					  : return "ERROR_COMPRESSION_EXHAUSTED_EXT";
		case VK_INCOMPATIBLE_SHADER_BINARY_EXT			          : return "INCOMPATIBLE_SHADER_BINARY_EXT";
		default													  : return "INVALID VkResult";
	}
}

static const char* stringify_VkObjectType(const VkObjectType objectType)
{
	switch (objectType) {
		case VK_OBJECT_TYPE_UNKNOWN						   : return "UNKNOWN";
		case VK_OBJECT_TYPE_INSTANCE					   : return "INSTANCE";
		case VK_OBJECT_TYPE_PHYSICAL_DEVICE				   : return "PHYSICAL_DEVICE";
		case VK_OBJECT_TYPE_DEVICE						   : return "DEVICE";
		case VK_OBJECT_TYPE_QUEUE						   : return "QUEUE";
		case VK_OBJECT_TYPE_SEMAPHORE					   : return "SEMAPHORE";
		case VK_OBJECT_TYPE_COMMAND_BUFFER				   : return "COMMAND_BUFFER";
		case VK_OBJECT_TYPE_FENCE						   : return "FENCE";
		case VK_OBJECT_TYPE_DEVICE_MEMORY				   : return "DEVICE_MEMORY";
		case VK_OBJECT_TYPE_BUFFER						   : return "BUFFER";
		case VK_OBJECT_TYPE_IMAGE						   : return "IMAGE";
		case VK_OBJECT_TYPE_EVENT						   : return "EVENT";
		case VK_OBJECT_TYPE_QUERY_POOL					   : return "QUERY_POOL";
		case VK_OBJECT_TYPE_BUFFER_VIEW					   : return "BUFFER_VIEW";
		case VK_OBJECT_TYPE_IMAGE_VIEW					   : return "IMAGE_VIEW";
		case VK_OBJECT_TYPE_SHADER_MODULE				   : return "SHADER_MODULE";
		case VK_OBJECT_TYPE_PIPELINE_CACHE				   : return "PIPELINE_CACHE";
		case VK_OBJECT_TYPE_PIPELINE_LAYOUT				   : return "PIPELINE_LAYOUT";
		case VK_OBJECT_TYPE_RENDER_PASS					   : return "RENDER_PASS";
		case VK_OBJECT_TYPE_PIPELINE					   : return "PIPELINE";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT		   : return "DESCRIPTOR_SET_LAYOUT";
		case VK_OBJECT_TYPE_SAMPLER						   : return "SAMPLER";
		case VK_OBJECT_TYPE_DESCRIPTOR_POOL				   : return "DESCRIPTOR_POOL";
		case VK_OBJECT_TYPE_DESCRIPTOR_SET				   : return "DESCRIPTOR_SET";
		case VK_OBJECT_TYPE_FRAMEBUFFER					   : return "FRAMEBUFFER";
		case VK_OBJECT_TYPE_COMMAND_POOL				   : return "COMMAND_POOL";
		case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION	   : return "SAMPLER_YCBCR_CONVERSION";
		case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE	   : return "DESCRIPTOR_UPDATE_TEMPLATE";
		case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT			   : return "PRIVATE_DATA_SLOT";
		case VK_OBJECT_TYPE_SURFACE_KHR					   : return "SURFACE_KHR";
		case VK_OBJECT_TYPE_SWAPCHAIN_KHR				   : return "SWAPCHAIN_KHR";
		case VK_OBJECT_TYPE_DISPLAY_KHR					   : return "DISPLAY_KHR";
		case VK_OBJECT_TYPE_DISPLAY_MODE_KHR			   : return "DISPLAY_MODE_KHR";
		case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT	   : return "DEBUG_REPORT_CALLBACK_EXT";
		case VK_OBJECT_TYPE_VIDEO_SESSION_KHR			   : return "VIDEO_SESSION_KHR";
		case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR   : return "VIDEO_SESSION_PARAMETERS_KHR";
		case VK_OBJECT_TYPE_CU_MODULE_NVX				   : return "CU_MODULE_NVX";
		case VK_OBJECT_TYPE_CU_FUNCTION_NVX				   : return "CU_FUNCTION_NVX";
		case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT	   : return "DEBUG_UTILS_MESSENGER_EXT";
		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR	   : return "ACCELERATION_STRUCTURE_KHR";
		case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT		   : return "VALIDATION_CACHE_EXT";
		case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV	   : return "ACCELERATION_STRUCTURE_NV";
		case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL: return "PERFORMANCE_CONFIGURATION_INTEL";
		case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR		   : return "DEFERRED_OPERATION_KHR";
		case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV	   : return "INDIRECT_COMMANDS_LAYOUT_NV";
		case VK_OBJECT_TYPE_CUDA_MODULE_NV                 : return "CUDA_MODULE_NV";
		case VK_OBJECT_TYPE_CUDA_FUNCTION_NV               : return "CUDA_FUNCTION_NV";
		case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA	   : return "BUFFER_COLLECTION_FUCHSIA";
		case VK_OBJECT_TYPE_MICROMAP_EXT				   : return "MICROMAP_EXT";
		case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV		   : return "OPTICAL_FLOW_SESSION_NV";
		case VK_OBJECT_TYPE_SHADER_EXT					   : return "SHADER_EXT";
		default											   : return "INVALID VkObjectType";
	}
}

static const char* stringify_VkSystemAllocationScope(const VkSystemAllocationScope systemAllocationScope)
{
	switch (systemAllocationScope) {
		case VK_SYSTEM_ALLOCATION_SCOPE_COMMAND	: return "COMMAND";
		case VK_SYSTEM_ALLOCATION_SCOPE_OBJECT	: return "OBJECT";
		case VK_SYSTEM_ALLOCATION_SCOPE_CACHE	: return "CACHE";
		case VK_SYSTEM_ALLOCATION_SCOPE_DEVICE	: return "DEVICE";
		case VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE: return "INSTANCE";
		default									: return "INVALID VkSystemAllocationScope";
	}
}

static const char* stringify_VkInternalAllocationType(const VkInternalAllocationType internalAllocationType)
{
	switch (internalAllocationType) {
		case VK_INTERNAL_ALLOCATION_TYPE_EXECUTABLE: return "EXECUTABLE";
		default									   : return "INVALID VkInternalAllocationType";
	}
}


// ===== Debug callback functions =====

bool init_debug_logfile(void)
{
	FILE* const file = fopen(LOG_NAME, "w");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to initialise debug callback logfile\n", LOG_NAME);
		return false;
	}

	const time_t currentTime = time(NULL);
	const clock_t programTime = clock() * MS_PER_CLOCK;
	fprintf(file,
		"VULKAN DEBUG CALLBACK LOGFILE\n"
		"PROGRAM NAME: %s\n"
		"CURRENT LOCAL TIME: %s"
		"TIME SINCE LAUNCH: %ld ms\n\n",
		PROGRAM_NAME, ctime(&currentTime), programTime
	);

	fclose(file);
	return true;
}

static void print_debug_callback(
	FILE* const stream,
	const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	const VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* const pCallbackData,
	const uint64_t userData)
{
	// Beginning info
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stream,
		"Vulkan debug callback %llu\n"
		"Time since launch: %ld ms\n",
		userData, time
	);

	// Message severity
	switch (messageSeverity) {
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT	: fputs("===== Severity: ERROR =====\n", stream); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: fputs("--- Severity: Warning ---\n",   stream); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT	: fputs("< Severity: Info >\n",          stream); break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: fputs("Severity: Verbose\n",           stream); break;
		default: fprintf(stream, "Severity: UNKNOWN (0x%x)\n", messageSeverity); break;
	}

	// Message types
	fputs("Types:", stream);
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT	  ) fputs(" General",     stream);
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT ) fputs(" Validation",  stream);
	if (messageTypes & VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT) fputs(" Performance", stream);
	putc('\n', stream);

	// Message ID
	fprintf(stream,
		"ID: %s (0x%x)\n",
		pCallbackData->pMessageIdName ? pCallbackData->pMessageIdName : "",
		(unsigned int) pCallbackData->messageIdNumber
	);

	// VkDebugUtilsLabelEXT active in the current VkQueue
	fprintf(stream, "Queue labels (%u):\n", pCallbackData->queueLabelCount);
	for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++)
		fprintf(stream,
			"\t%s (R%f G%f B%f A%f)\n",
			pCallbackData->pQueueLabels[i].pLabelName,
			(double) pCallbackData->pQueueLabels[i].color[0],
			(double) pCallbackData->pQueueLabels[i].color[1],
			(double) pCallbackData->pQueueLabels[i].color[2],
			(double) pCallbackData->pQueueLabels[i].color[3]
		);
	
	// VkDebugUtilsLabelEXT active in the current VkCommandBuffer
	fprintf(stream, "Command buffer labels (%u):\n", pCallbackData->cmdBufLabelCount);
	for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++)
		fprintf(stream,
			"\t%s (R%f G%f B%f A%f)\n",
			pCallbackData->pCmdBufLabels[i].pLabelName,
			(double) pCallbackData->pCmdBufLabels[i].color[0],
			(double) pCallbackData->pCmdBufLabels[i].color[1],
			(double) pCallbackData->pCmdBufLabels[i].color[2],
			(double) pCallbackData->pCmdBufLabels[i].color[3]
		);

	// VkDebugUtilsObjectNameInfoEXT related to the callback
	fprintf(stream, "Objects (%u):\n", pCallbackData->objectCount);
	for (uint32_t i = 0; i < pCallbackData->objectCount; i++)
		fprintf(stream,
			"\t%s (Type: %s, Handle: 0x%llx)\n",
			pCallbackData->pObjects[i].pObjectName ? pCallbackData->pObjects[i].pObjectName : "",
			stringify_VkObjectType(pCallbackData->pObjects[i].objectType),
			pCallbackData->pObjects[i].objectHandle
		);

	// Main callback message
	fprintf(stream, "%s\n\n", pCallbackData->pMessage);
}

VkBool32 debug_callback(
	const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	const VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* const pCallbackData,
	void* const pUserData)
{
	uint64_t* const callbackCount = (uint64_t*) pUserData;
	FILE* const file = fopen(LOG_NAME, "a");

	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to log Vulkan debug callback %llu\n", LOG_NAME, *callbackCount);
		print_debug_callback(
			messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ? stderr : stdout,
			messageSeverity, messageTypes, pCallbackData, *callbackCount
		);
	}
	else {
		if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			print_debug_callback(stderr, messageSeverity, messageTypes, pCallbackData, *callbackCount);
		else if (messageTypes & ~VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT)
			print_debug_callback(stdout, messageSeverity, messageTypes, pCallbackData, *callbackCount);

		print_debug_callback(file, messageSeverity, messageTypes, pCallbackData, *callbackCount);
		fclose(file);
	}

	(*callbackCount)++;
	return VK_FALSE;
}


// ===== Allocation callback functions =====

static void print_allocation_callback(
	FILE* const stream,
	const uint64_t userData,
	const size_t size,
	const size_t alignment,
	const VkSystemAllocationScope allocationScope,
	const void* const memory)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	const char* const sAllocationScope = stringify_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host allocation callback %llu\n"
		"Time since launch: %ld ms\n"
		"Size: %zu\n"
		"Alignment: %zu\n"
		"Allocation scope: %s\n"
		"Allocated address: %p\n\n",
		userData, time, size, alignment, sAllocationScope, memory
	);
}

void* allocation_callback(
	void* const pUserData,
	const size_t size,
	const size_t alignment,
	const VkSystemAllocationScope allocationScope)
{
	uint64_t* const allocationCount = &((AllocationCallbackCounts_t*) pUserData)->allocationCount;
	void* const memory = _aligned_malloc(size, alignment);

	FILE* const file = fopen(LOG_NAME, "a");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to log Vulkan host allocation callback %llu\n", LOG_NAME, *allocationCount);
		print_allocation_callback(memory == NULL ? stderr : stdout, *allocationCount, size, alignment, allocationScope, memory);
	}
	else {
		if (memory == NULL)
			print_allocation_callback(stderr, *allocationCount, size, alignment, allocationScope, memory);
		
		print_allocation_callback(file, *allocationCount, size, alignment, allocationScope, memory);
		fclose(file);
	}

	(*allocationCount)++;
	return memory;
}

static void print_reallocation_callback(
	FILE* const stream,
	const uint64_t userData,
	const size_t originalSize,
	const size_t size,
	const size_t alignment,
	const VkSystemAllocationScope allocationScope,
	const void* const originalAddr,
	const void* const memory)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	const char* const sAllocationScope = stringify_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host reallocation callback %llu\n"
		"Time since launch: %ld ms\n"
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
	void* const pUserData,
	void* const pOriginal,
	const size_t size,
	const size_t alignment,
	const VkSystemAllocationScope allocationScope)
{
	uint64_t* const reallocationCount = &((AllocationCallbackCounts_t*) pUserData)->reallocationCount;
	const size_t originalSize = _aligned_msize(pOriginal, alignment, (size_t) 0);
	void* const memory = _aligned_realloc(pOriginal, size, alignment);

	FILE* const file = fopen(LOG_NAME, "a");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to log Vulkan host reallocation callback %llu\n", LOG_NAME, *reallocationCount);
		print_reallocation_callback(memory == NULL ? stderr : stdout, *reallocationCount, originalSize, size, alignment, allocationScope, pOriginal, memory);
	}
	else {
		if (memory == NULL)
			print_reallocation_callback(stderr, *reallocationCount, originalSize, size, alignment, allocationScope, pOriginal, memory);
		
		print_reallocation_callback(file, *reallocationCount, originalSize, size, alignment, allocationScope, pOriginal, memory);
		fclose(file);
	}

	(*reallocationCount)++;
	return memory;
}

static void print_free_callback(
	FILE* const stream,
	const uint64_t userData,
	const void* const memory)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stream,
		"Vulkan host free callback %llu\n"
		"Time since launch: %ld ms\n"
		"Freed address: %p\n\n",
		userData, time, memory
	);
}

void free_callback(
	void* const pUserData,
	void* const pMemory)
{
	uint64_t* const freeCount = &((AllocationCallbackCounts_t*) pUserData)->freeCount;
	_aligned_free(pMemory);

	FILE* const file = fopen(LOG_NAME, "a");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to log Vulkan host free callback %llu\n", LOG_NAME, *freeCount);
		print_free_callback(stdout, *freeCount, pMemory);
	}
	else {
		print_free_callback(file, *freeCount, pMemory);
		fclose(file);
	}

	(*freeCount)++;
}

static void print_internal_allocation_callback(
	FILE* const stream,
	const uint64_t userData,
	const size_t size,
	const VkInternalAllocationType allocationType,
	const VkSystemAllocationScope allocationScope)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	const char* const sAllocationType = stringify_VkInternalAllocationType(allocationType);
	const char* const sAllocationScope = stringify_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host internal allocation callback %llu\n"
		"Time since launch: %ld ms\n"
		"Size: %zu\n"
		"Allocation type: %s\n"
		"Allocation scope: %s\n\n",
		userData, time, size, sAllocationType, sAllocationScope
	);
}

void internal_allocation_callback(
	void* const pUserData,
	const size_t size,
	const VkInternalAllocationType allocationType,
	const VkSystemAllocationScope allocationScope)
{
	uint64_t* const internalAllocationCount = &((AllocationCallbackCounts_t*) pUserData)->internalAllocationCount;

	FILE* const file = fopen(LOG_NAME, "a");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to log Vulkan host internal allocation callback %llu\n", LOG_NAME, *internalAllocationCount);
		print_internal_allocation_callback(stdout, *internalAllocationCount, size, allocationType, allocationScope);
	}
	else {
		print_internal_allocation_callback(file, *internalAllocationCount, size, allocationType, allocationScope);
		fclose(file);
	}

	(*internalAllocationCount)++;
}

static void print_internal_free_callback(
	FILE* const stream,
	const uint64_t userData,
	const size_t size,
	const VkInternalAllocationType allocationType,
	const VkSystemAllocationScope allocationScope)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	const char* const sAllocationType = stringify_VkInternalAllocationType(allocationType);
	const char* const sAllocationScope = stringify_VkSystemAllocationScope(allocationScope);
	fprintf(stream,
		"Vulkan host internal free callback %llu\n"
		"Time since launch: %ld ms\n"
		"Size: %zu\n"
		"Allocation type: %s\n"
		"Allocation scope: %s\n\n",
		userData, time, size, sAllocationType, sAllocationScope
	);
}

void internal_free_callback(
	void* const pUserData,
	const size_t size,
	const VkInternalAllocationType allocationType,
	const VkSystemAllocationScope allocationScope)
{
	uint64_t* const internalFreeCount = &((AllocationCallbackCounts_t*) pUserData)->internalFreeCount;

	FILE* const file = fopen(LOG_NAME, "a");
	if (file == NULL) {
		fprintf(stderr, "Failed to open file '%s' to log Vulkan host internal free callback %llu\n", LOG_NAME, *internalFreeCount);
		print_internal_free_callback(stdout, *internalFreeCount, size, allocationType, allocationScope);
	}
	else {
		print_internal_free_callback(file, *internalFreeCount, size, allocationType, allocationScope);
		fclose(file);
	}

	(*internalFreeCount)++;
}


// ===== Failure functions =====

void print_malloc_failure(const int line, const void* const ptr, const size_t _Size)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'malloc' with void* = %p\n"
		"Arguments:\n"
		"\tsize_t _Size = %zu\n\n",
		line, time, ptr, _Size
	);
}

void print_calloc_failure(const int line, const void* const ptr, const size_t _NumOfElements, const size_t _SizeOfElements)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'calloc' with void* = %p\n"
		"Arguments:\n"
		"\tsize_t _NumOfElements = %zu\n"
		"\tsize_t _SizeOfElements = %zu\n\n",
		line, time, ptr, _NumOfElements, _SizeOfElements
	);
}

void print_realloc_failure(const int line, const void* const ptr, const void* const _Memory, const size_t _NewSize)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Memory failure at line %d (%ld ms)\n"
		"Failed function call 'realloc' with void* = %p\n"
		"Arguments:\n"
		"\tvoid* _Memory = %p\n"
		"\tsize_t _NewSize = %zu\n\n",
		line, time, ptr, _Memory, _NewSize
	);
}

void print_fopen_failure(const int line, const FILE* const file, const char* const _Filename, const char* const _Mode)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fopen' with FILE* = %p\n"
		"Arguments:\n"
		"\tconst char* _Filename = %s\n"
		"\tconst char* _Mode = %s\n\n",
		line, time, (const void*) file, _Filename, _Mode
	);
}

void print_fclose_failure(const int line, const int result, const FILE* const _File)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'fclose' with int = %d\n"
		"Arguments:\n"
		"\tFILE* _File = %p\n\n",
		line, time, result, (const void*) _File
	);
}

void print_fseek_failure(const int line, const int result, const FILE* const _File, const long _Offset, const int _Origin)
{
	const clock_t time = clock() * MS_PER_CLOCK;
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

void print_ftell_failure(const int line, const long result, const FILE* const _File)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"IO error at line %d (%ld ms)\n"
		"Failed function call 'ftell' with long = %ld\n"
		"Arguments:\n"
		"\tFILE* _File = %p\n\n",
		line, time, result, (const void*) _File
	);
}

void print_fread_failure(const int line, const size_t result, const void* const _DstBuf, const size_t _ElementSize, const size_t _Count, const FILE* const _File)
{
	const clock_t time = clock() * MS_PER_CLOCK;
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

void print_fwrite_failure(const int line, const size_t result, const void* const _Str, const size_t _Size, const size_t _Count, const FILE* const _File)
{
	const clock_t time = clock() * MS_PER_CLOCK;
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

void print_pcreate_failure(const int line, const int result, const pthread_t* const th, const pthread_attr_t* const attr, const char* const func, const void* const arg)
{
	const clock_t time = clock() * MS_PER_CLOCK;
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

void print_pjoin_failure(const int line, const int result, const pthread_t t, const void* const* const res)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_join' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_t t = 0x%llx\n"
		"\tvoid** res = %p\n\n",
		line, time, result, t, (const void*) res
	);
}

void print_pcancel_failure(const int line, const int result, const pthread_t t)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_cancel' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_t t = 0x%llx\n\n",
		line, time, result, t
	);
}

void print_pinit_failure(const int line, const int result, const pthread_mutex_t* const m, const pthread_mutexattr_t* const a)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_init' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n"
		"\tconst pthread_mutexattr_t* a = %p\n\n",
		line, time, result, (const void*) m, (const void*) a
	);
}

void print_plock_failure(const int line, const int result, const pthread_mutex_t* const m)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_lock' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n\n",
		line, time, result, (const void*) m
	);
}

void print_punlock_failure(const int line, const int result, const pthread_mutex_t* const m)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_unlock' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n\n",
		line, time, result, (const void*) m
	);
}

void print_pdestroy_failure(const int line, const int result, const pthread_mutex_t* const m)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Thread failure at line %d (%ld ms)\n"
		"Function 'pthread_mutex_destroy' returned int = %d\n"
		"Arguments:\n"
		"\tpthread_mutex_t* m = %p\n\n",
		line, time, result, (const void*) m
	);
}

void print_vulkan_failure(const int line, const char* const func, const VkResult result, const unsigned int count, ...)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	const char* const sResult = stringify_VkResult(result);
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

void print_instprocaddr_failure(const int line, const VkInstance instance, const char* const pName)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call 'vkGetInstanceProcAddr' with PFN_vkVoidFunction = %p\n"
		"Arguments:\n"
		"\tVkInstance instance = %p\n"
		"\tconst char* pName = %s\n\n",
		line, time, NULL, (void*) instance, pName
	);
}

void print_devprocaddr_failure(const int line, const VkDevice device, const char* const pName)
{
	const clock_t time = clock() * MS_PER_CLOCK;
	fprintf(stderr,
		"Vulkan failure at line %d (%ld ms)\n"
		"Failed function call 'vkGetDeviceProcAddr' with PFN_vkVoidFunction = %p\n"
		"Arguments:\n"
		"\tVkInstance instance = %p\n"
		"\tconst char* pName = %s\n\n",
		line, time, NULL, (void*) device, pName
	);
}
