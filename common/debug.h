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

#pragma once

#include "def.h"


struct CzVulkanCallbackData
{
	const char* func;
	const char* file;
	uint64_t line;
};


extern struct CzVulkanCallbackData czgCallbackData;


// Initialisation functions

bool init_debug_logfile(const char* filename);
bool init_alloc_logfile(const char* filename);
bool init_colour_level(enum CzColourLevel level);


// General logging functions

bool log_debug(FILE* stream, const char* format, ...) CZ_PRINTF(2, 3) CZ_NONNULL_ARG(1, 2) CZ_NULLTERM_ARG(2);
bool log_warning(FILE* stream, const char* format, ...) CZ_PRINTF(2, 3) CZ_NONNULL_ARG(1, 2) CZ_NULLTERM_ARG(2);
bool log_error(FILE* stream, const char* format, ...) CZ_PRINTF(2, 3) CZ_NONNULL_ARG(1, 2) CZ_NULLTERM_ARG(2);
bool log_critical(FILE* stream, const char* format, ...) CZ_PRINTF(2, 3) CZ_NONNULL_ARG(1, 2) CZ_NULLTERM_ARG(2);


// Failure functions

void log_fopen_failure(int line, FILE* res, const char* name, const char* mode)
	CZ_COLD CZ_NONNULL_ARG(3, 4) CZ_NULLTERM_ARG(3) CZ_NULLTERM_ARG(4) CZ_NO_ACCESS(2);
void log_fread_failure(int line, size_t res, const void* buf, size_t size, size_t count, FILE* file)
	CZ_COLD CZ_NO_ACCESS(3) CZ_NO_ACCESS(6);
void log_fwrite_failure(int line, size_t res, const void* buf, size_t size, size_t count, FILE* file)
	CZ_COLD CZ_NO_ACCESS(3) CZ_NO_ACCESS(6);

void log_fscanf_failure(int line, int res, FILE* file, const char* fmt)
	CZ_COLD CZ_NONNULL_ARG(4) CZ_NULLTERM_ARG(4) CZ_NO_ACCESS(3);
void log_fprintf_failure(int line, int res, FILE* file, const char* fmt)
	CZ_COLD CZ_NONNULL_ARG(4) CZ_NULLTERM_ARG(4) CZ_NO_ACCESS(3);

void log_pcreate_failure(int line, int res) CZ_COLD;
void log_pcancel_failure(int line, int res) CZ_COLD;
void log_pjoin_failure(int line, int res) CZ_COLD;

void log_vkinit_failure(int line, VkResult res) CZ_COLD;
void log_vulkan_failure(int line, VkResult res, const char* func) CZ_COLD CZ_NONNULL_ARGS CZ_NULLTERM_ARG(3);


// Callback functions

VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) CZ_NONNULL_ARG(3);

VKAPI_ATTR void* VKAPI_CALL allocation_callback(
	void* pUserData, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void* VKAPI_CALL reallocation_callback(
	void* pUserData, void* pOriginal, size_t size, size_t alignment, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void VKAPI_CALL free_callback(void* pUserData, void* pMemory);

VKAPI_ATTR void VKAPI_CALL internal_allocation_callback(
	void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);
VKAPI_ATTR void VKAPI_CALL internal_free_callback(
	void* pUserData, size_t size, VkInternalAllocationType allocationType, VkSystemAllocationScope allocationScope);


// Helper macros

#define FOPEN_FAILURE(res, name, mode) log_fopen_failure( \
	__LINE__, (FILE*) (res), (const char*) (name), (const char*) (mode))
#define FREAD_FAILURE(res, buf, size, count, file) log_fread_failure( \
	__LINE__, (size_t) (res), (const void*) (buf), (size_t) (size), (size_t) (count), (FILE*) (file))
#define FWRITE_FAILURE(res, buf, size, count, file) log_fwrite_failure( \
	__LINE__, (size_t) (res), (const void*) (buf), (size_t) (size), (size_t) (count), (FILE*) (file))

#define FSCANF_FAILURE(res, file, fmt)  log_fscanf_failure(__LINE__, (int) (res), (FILE*) (file), (const char*) (fmt))
#define FPRINTF_FAILURE(res, file, fmt) log_fprintf_failure(__LINE__, (int) (res), (FILE*) (file), (const char*) (fmt))

#define PCREATE_FAILURE(res) log_pcreate_failure(__LINE__, (int) (res))
#define PCANCEL_FAILURE(res) log_pcancel_failure(__LINE__, (int) (res))
#define PJOIN_FAILURE(res)   log_pjoin_failure(__LINE__, (int) (res))

#define VKINIT_FAILURE(res)  log_vkinit_failure(__LINE__, (VkResult) (res))
#define VULKAN_FAILURE(func) log_vulkan_failure(__LINE__, vkres, #func)

#ifdef NDEBUG
	#define VK_CALL(vkfunc, ...)   \
		do {                       \
			(vkfunc)(__VA_ARGS__); \
		}                          \
		while (0)

	#define VK_CALLR(vkfunc, ...)                  \
		do {                                       \
			vkres = (vkfunc)(__VA_ARGS__);         \
			if CZ_NOEXPECT (vkres != VK_SUCCESS) { \
				VULKAN_FAILURE(vkfunc);            \
			}                                      \
		}                                          \
		while (0)
#else
	#define VK_CALL(vkfunc, ...)                \
		do {                                    \
			czgCallbackData.func = #vkfunc;     \
			czgCallbackData.file = CZ_FILENAME; \
			czgCallbackData.line = __LINE__;    \
			(vkfunc)(__VA_ARGS__);              \
		}                                       \
		while (0)

	#define VK_CALLR(vkfunc, ...)                  \
		do {                                       \
			czgCallbackData.func = #vkfunc;        \
			czgCallbackData.file = CZ_FILENAME;    \
			czgCallbackData.line = __LINE__;       \
			vkres = (vkfunc)(__VA_ARGS__);         \
			if CZ_NOEXPECT (vkres != VK_SUCCESS) { \
				VULKAN_FAILURE(vkfunc);            \
			}                                      \
		}                                          \
		while (0)
#endif
