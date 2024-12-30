/* 
 * Copyright (C) 2024 Seth McDonald
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

#include "defs.h"


typedef struct CallbackData
{
	const char* funcName;
	uint64_t    lineNum;
} CallbackData;


extern CallbackData g_callbackData;


// Initialisation functions

bool init_debug_logfile(void);
bool init_alloc_logfile(void);


// Failure functions

void print_malloc_failure(int line, void* result, size_t size) COLD_FUNC NO_ACCESS(2);
void print_calloc_failure(int line, void* result, size_t num, size_t size) COLD_FUNC NO_ACCESS(2);
void print_realloc_failure(int line, void* result, void* ptr, size_t size) COLD_FUNC NO_ACCESS(2) NO_ACCESS(3);

void print_fopen_failure(int line, FILE* result, const char* filename, const char* mode)
	COLD_FUNC NONNULL_ARGS(3, 4) NULTSTR_ARG(3) NULTSTR_ARG(4) NO_ACCESS(2) RE_ACCESS(3) RE_ACCESS(4);
void print_fseek_failure(int line, int result, FILE* file, long offset, int origin) COLD_FUNC NO_ACCESS(3);
void print_ftell_failure(int line, long result, FILE* file) COLD_FUNC NO_ACCESS(3);

void print_fread_failure(int line, size_t result, const void* buffer, size_t size, size_t count, FILE* file)
	COLD_FUNC NO_ACCESS(3) NO_ACCESS(6);
void print_fwrite_failure(int line, size_t result, const void* buffer, size_t size, size_t count, FILE* file)
	COLD_FUNC NO_ACCESS(3) NO_ACCESS(6);

void print_fscanf_failure(int line, int result, FILE* file, const char* format)
	COLD_FUNC NONNULL_ARGS(4) NULTSTR_ARG(4) NO_ACCESS(3) RE_ACCESS(4);
void print_fprintf_failure(int line, int result, FILE* file, const char* format)
	COLD_FUNC NONNULL_ARGS(4) NULTSTR_ARG(4) NO_ACCESS(3) RE_ACCESS(4);

void print_pcreate_failure(int line, int result, pthread_t* thread, pthread_attr_t* attr)
	COLD_FUNC NO_ACCESS(3) NO_ACCESS(4);
void print_pjoin_failure(int line, int result, pthread_t thread, void** retval) COLD_FUNC NO_ACCESS(4);
void print_pcancel_failure(int line, int result, pthread_t thread) COLD_FUNC;

void print_vkinit_failure(int line, VkResult result) COLD_FUNC;
void print_vkvers_failure(int line, uint32_t result) COLD_FUNC;
void print_vulkan_failure(int line, VkResult result, const char* func)
	COLD_FUNC NONNULL_ARGS_ALL NULTSTR_ARG(3) RE_ACCESS(3);


// Callback functions

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) NONNULL_ARGS(3);

VKAPI_ATTR void* VKAPI_CALL allocation_callback(
	void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void* VKAPI_CALL reallocation_callback(
	void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void  VKAPI_CALL free_callback(void* pUserData, void* pMemory);

VKAPI_ATTR void VKAPI_CALL internal_allocation_callback(
	void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void VKAPI_CALL internal_free_callback(
	void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);


// Helper macros

#define MALLOC_FAILURE(res, size)       print_malloc_failure(__LINE__, (void*) (res), (size_t) (size))
#define CALLOC_FAILURE(res, num, size)  print_calloc_failure(__LINE__, (void*) (res), (size_t) (num), (size_t) (size))
#define REALLOC_FAILURE(res, ptr, size) print_realloc_failure(__LINE__, (void*) (res), (void*) (ptr), (size_t) (size))

#define FOPEN_FAILURE(res, name, mode)     print_fopen_failure(                                        \
	__LINE__, (FILE*) (res), (const char*) (name), (const char*) (mode))
#define FSEEK_FAILURE(res, file, off, ori) print_fseek_failure(                                        \
	__LINE__, (int) (res), (FILE*) (file), (long) (off), (int) (ori))
#define FTELL_FAILURE(res, file)           print_ftell_failure(__LINE__, (long) (res), (FILE*) (file))

#define FREAD_FAILURE(res, buf, size, count, file)  print_fread_failure(                              \
	__LINE__, (size_t) (res), (const void*) (buf), (size_t) (size), (size_t) (count), (FILE*) (file))
#define FWRITE_FAILURE(res, buf, size, count, file) print_fwrite_failure(                             \
	__LINE__, (size_t) (res), (const void*) (buf), (size_t) (size), (size_t) (count), (FILE*) (file))

#define FSCANF_FAILURE(res, file, fmt)  print_fscanf_failure(   \
	__LINE__, (int) (res), (FILE*) (file), (const char*) (fmt))
#define FPRINTF_FAILURE(res, file, fmt) print_fprintf_failure(  \
	__LINE__, (int) (res), (FILE*) (file), (const char*) (fmt))

#define PCREATE_FAILURE(res, thr, atr) print_pcreate_failure(                                                        \
	__LINE__, (int) (res), (pthread_t*) (thr), (pthread_attr_t*) (atr))
#define PJOIN_FAILURE(res, thr, ret)   print_pjoin_failure(__LINE__, (int) (res), (pthread_t) (thr), (void**) (ret))
#define PCANCEL_FAILURE(res, thr)      print_pcancel_failure(__LINE__, (int) (res), (pthread_t) (thr))

#define VKINIT_FAILURE(res)  print_vkinit_failure(__LINE__, (VkResult) (res))
#define VKVERS_FAILURE(res)  print_vkvers_failure(__LINE__, (uint32_t) (res))
#define VULKAN_FAILURE(func) print_vulkan_failure(__LINE__, vkres, #func)

#ifdef NDEBUG
	#define VK_CALL(func, ...)   \
		do {                     \
			(func)(__VA_ARGS__); \
		}                        \
		while (0)

	#define VK_CALL_RES(func, ...)                  \
		do {                                        \
			vkres = (func)(__VA_ARGS__);            \
			if EXPECT_FALSE (vkres != VK_SUCCESS) { \
				VULKAN_FAILURE(func);               \
			}                                       \
		}                                           \
		while (0)
#else
	#define VK_CALL(func, ...)                  \
		do {                                    \
			g_callbackData.funcName = #func;    \
			g_callbackData.lineNum  = __LINE__; \
			(func)(__VA_ARGS__);                \
		}                                       \
		while (0)

	#define VK_CALL_RES(func, ...)                  \
		do {                                        \
			g_callbackData.funcName = #func;        \
			g_callbackData.lineNum  = __LINE__;     \
			vkres = (func)(__VA_ARGS__);            \
			if EXPECT_FALSE (vkres != VK_SUCCESS) { \
				VULKAN_FAILURE(func);               \
			}                                       \
		}                                           \
		while (0)
#endif
