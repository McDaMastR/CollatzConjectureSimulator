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

#include "config.h"


// Datatypes
typedef struct CallbackData
{
	const char* funcName;
	uint64_t    lineNum;
} CallbackData;


// Globals
extern CallbackData g_callbackData;


// Logfile functions
bool init_debug_logfile(void);
bool init_alloc_logfile(void);


// Failure functions
void print_malloc_failure (int line, const void* ptr, size_t size)                                 NO_ACCESS(2) COLD_FUNC;
void print_calloc_failure (int line, const void* ptr, size_t numOfElements, size_t sizeOfElements) NO_ACCESS(2) COLD_FUNC;
void print_realloc_failure(int line, const void* ptr, const void* memory, size_t newSize)          NO_ACCESS(2) NO_ACCESS(3) COLD_FUNC;

void print_fopen_failure(int line, const FILE* file, const char* filename, const char* mode) NULTSTR_ARG(3) NULTSTR_ARG(4) NO_ACCESS(2) RE_ACCESS(3) RE_ACCESS(4) COLD_FUNC;
void print_fseek_failure(int line, int  result, const FILE* file, long offset, int origin)   NO_ACCESS(3) COLD_FUNC;
void print_ftell_failure(int line, long result, const FILE* file)                            NO_ACCESS(3) COLD_FUNC;

void print_fread_failure (int line, size_t result, const void* dstBuf, size_t elementSize, size_t count, const FILE* file) NO_ACCESS(3) NO_ACCESS(6) COLD_FUNC;
void print_fwrite_failure(int line, size_t result, const void* str,    size_t size,        size_t count, const FILE* file) NO_ACCESS(3) NO_ACCESS(6) COLD_FUNC;

void print_pcreate_failure(int line, int result, const pthread_t* th, const pthread_attr_t* attr, const void* arg) NO_ACCESS(3) NO_ACCESS(4) NO_ACCESS(5) COLD_FUNC;
void print_pjoin_failure  (int line, int result, pthread_t t, const void* const* res)                              NO_ACCESS(4) COLD_FUNC;
void print_pcancel_failure(int line, int result, pthread_t t)                                                      COLD_FUNC;

void print_vkinit_failure(int line, VkResult result)                   COLD_FUNC;
void print_vkvers_failure(int line, uint32_t apiVersion)               COLD_FUNC;
void print_vulkan_failure(int line, const char* func, VkResult result) NULTSTR_ARG(2) RE_ACCESS(2) COLD_FUNC;


// Callback functions
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) NONNULL_ARGS(3) RE_ACCESS(3);

VKAPI_ATTR void* VKAPI_CALL allocation_callback  (void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void* VKAPI_CALL reallocation_callback(void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void  VKAPI_CALL free_callback        (void* pUserData, void* pMemory);

VKAPI_ATTR void VKAPI_CALL internal_allocation_callback(void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void VKAPI_CALL internal_free_callback      (void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);


// Helper macros
#define MALLOC_FAILURE(res, size)       print_malloc_failure (__LINE__, (const void*) (res), (size_t) (size));
#define CALLOC_FAILURE(res, num, size)  print_calloc_failure (__LINE__, (const void*) (res), (size_t) (num),      (size_t) (size));
#define REALLOC_FAILURE(res, ptr, size) print_realloc_failure(__LINE__, (const void*) (res), (const void*) (ptr), (size_t) (size));

#define FOPEN_FAILURE(res, name, mode)     print_fopen_failure(__LINE__, (const FILE*) (res), (const char*) (name), (const char*) (mode));
#define FSEEK_FAILURE(res, file, off, ori) print_fseek_failure(__LINE__, (int)  (res), (const FILE*) (file), (long) (off), (int) (ori));
#define FTELL_FAILURE(res, file)           print_ftell_failure(__LINE__, (long) (res), (const FILE*) (file));

#define FREAD_FAILURE(res, buf, size, count, file)  print_fread_failure (__LINE__, (size_t) (res), (const void*) (buf), (size_t) (size), (size_t) (count), (const FILE*) (file));
#define FWRITE_FAILURE(res, str, size, count, file) print_fwrite_failure(__LINE__, (size_t) (res), (const void*) (str), (size_t) (size), (size_t) (count), (const FILE*) (file));

#define PCREATE_FAILURE(res, thr, atr, arg) print_pcreate_failure(__LINE__, (int) (res), (const pthread_t*) (thr), (const pthread_attr_t*) (atr), (const void*) (arg));
#define PJOIN_FAILURE(res, thr, _res)       print_pjoin_failure  (__LINE__, (int) (res), (pthread_t) (thr), (const void* const*) (_res));
#define PCANCEL_FAILURE(res, thr)           print_pcancel_failure(__LINE__, (int) (res), (pthread_t) (thr));

#define VKINIT_FAILURE(res)  print_vkinit_failure(__LINE__, (VkResult) (res));
#define VKVERS_FAILURE(ver)  print_vkvers_failure(__LINE__, (uint32_t) (ver));
#define VULKAN_FAILURE(func) print_vulkan_failure(__LINE__, #func, vkres);

#ifdef NDEBUG
	#define BEGIN_FUNC
	#define END_FUNC

	#define VK_CALL(func, ...)     (func)(__VA_ARGS__);
	#define VK_CALL_RES(func, ...) vkres = (func)(__VA_ARGS__);
#else
	#define BEGIN_FUNC printf("BEGIN %s\n", __func__);
	#define END_FUNC   printf("END %s\n\n", __func__);

	#define VK_CALL(func, ...)     { g_callbackData.funcName = #func; g_callbackData.lineNum = __LINE__; (func)(__VA_ARGS__);         }
	#define VK_CALL_RES(func, ...) { g_callbackData.funcName = #func; g_callbackData.lineNum = __LINE__; vkres = (func)(__VA_ARGS__); }
#endif
