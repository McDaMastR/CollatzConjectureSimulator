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

#include "util.h"
#include "alloc.h"
#include "debug.h"
#include "file.h"

char* stime(void)
{
	time_t t = time(NULL);
	return ctime(&t);
}

double program_time(void)
{
	clock_t t = clock();
	return (double) t * CZ_MS_PER_CLOCK;
}

enum CzEndianness get_endianness(void)
{
	int x = 1;
	char c = *(char*) &x;
	return c ? CZ_ENDIANNESS_LITTLE : CZ_ENDIANNESS_BIG;
}

uint32_t ceil_pow2(uint32_t x)
{
	CZ_ASSUME(x != 0);
#if CZ_HAS_BUILTIN(stdc_bit_ceil)
	return __builtin_stdc_bit_ceil(x);
#elif CZ_HAS_BUILTIN(clz) && UINT32_MAX == UINT_MAX
	return x == 1 ? UINT32_C(1) : UINT32_C(2) << (31 - __builtin_clz(x - 1));
#elif CZ_HAS_BUILTIN(clzl) && UINT32_MAX == ULONG_MAX
	return x == 1 ? UINT32_C(1) : UINT32_C(2) << (31 - __builtin_clzl(x - 1));
#elif CZ_WINDOWS
	unsigned long i;
	_BitScanReverse(&i, x - 1);
	return UINT32_C(1) << (i + 1);
#else
	uint32_t y = UINT32_C(1) << 31;
	while (!(x & y)) { y >>= 1; }
	return y << 1;
#endif
}

uint32_t floor_pow2(uint32_t x)
{
	CZ_ASSUME(x != 0);
#if CZ_HAS_BUILTIN(stdc_bit_floor)
	return __builtin_stdc_bit_floor(x);
#elif CZ_HAS_BUILTIN(clz) && UINT32_MAX == UINT_MAX
	return UINT32_C(1) << (31 - __builtin_clz(x));
#elif CZ_HAS_BUILTIN(clzl) && UINT32_MAX == ULONG_MAX
	return UINT32_C(1) << (31 - __builtin_clzl(x));
#elif CZ_WINDOWS
	unsigned long i;
	_BitScanReverse(&i, x);
	return UINT32_C(1) << i;
#else
	uint32_t y = UINT32_C(1) << 31;
	while (!(x & y)) { y >>= 1; }
	return y;
#endif
}

double get_benchmark(clock_t start, clock_t end)
{
	return (double) (end - start) * CZ_MS_PER_CLOCK;
}

bool set_debug_name(VkDevice device, VkObjectType type, uint64_t handle, const char* name)
{
	VkResult vkres;

	VkDebugUtilsObjectNameInfoEXT info = {0};
	info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
	info.objectType = type;
	info.objectHandle = handle;
	info.pObjectName = name;

	if (!vkSetDebugUtilsObjectNameEXT) { return true; }

	VK_CALLR(vkSetDebugUtilsObjectNameEXT, device, &info);
	if CZ_NOEXPECT (vkres) { return false; }
	return true;
}

bool get_buffer_requirements_noext(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements)
{
	VkResult vkres;

	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	VkBuffer buffer;
	VK_CALLR(vkCreateBuffer, device, &bufferInfo, NULL, &buffer);
	if CZ_NOEXPECT (vkres) { return false; }

	VkBufferMemoryRequirementsInfo2 requirementsInfo = {0};
	requirementsInfo.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
	requirementsInfo.buffer = buffer;

	VkMemoryRequirements2 memoryRequirements = {0};
	memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VK_CALL(vkGetBufferMemoryRequirements2, device, &requirementsInfo, &memoryRequirements);
	VK_CALL(vkDestroyBuffer, device, buffer, NULL);

	*requirements = memoryRequirements.memoryRequirements;
	return true;
}

bool get_buffer_requirements_main4(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements)
{
	VkBufferCreateInfo bufferInfo = {0};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;

	VkDeviceBufferMemoryRequirements requirementsInfo = {0};
	requirementsInfo.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
	requirementsInfo.pCreateInfo = &bufferInfo;

	VkMemoryRequirements2 memoryRequirements = {0};
	memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VK_CALL(vkGetDeviceBufferMemoryRequirementsKHR, device, &requirementsInfo, &memoryRequirements);

	*requirements = memoryRequirements.memoryRequirements;
	return true;
}

bool save_pipeline_cache(VkDevice device, VkPipelineCache cache, const char* filename)
{
	VkResult vkres;

	size_t dataSize;
	VK_CALLR(vkGetPipelineCacheData, device, cache, &dataSize, NULL);
	if CZ_NOEXPECT (vkres) { return false; }

	void* restrict data;
	struct CzAllocFlags allocFlags = {0};
	enum CzResult czres = czAlloc(&data, dataSize, allocFlags);
	if CZ_NOEXPECT (czres) { return false; }

	VK_CALLR(vkGetPipelineCacheData, device, cache, &dataSize, data);
	if CZ_NOEXPECT (vkres) { goto err_free_data; }

	size_t offset = 0;
	struct CzFileFlags fileFlags = {0};
	fileFlags.relativeToExe = true;
	fileFlags.truncateFile = true;

	czres = czWriteFile(filename, data, dataSize, offset, fileFlags);
	if CZ_NOEXPECT (czres) { goto err_free_data; }

	czFree(data);
	return true;

err_free_data:
	czFree(data);
	return false;
}

bool read_text(const char* filename, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	const char* mode = "r";
	FILE* file = fopen(filename, mode);
	if CZ_NOEXPECT (!file) {
		FOPEN_FAILURE(file, filename, mode);
		goto err_end_args;
	}

	int ires = vfscanf(file, format, args);
	if CZ_NOEXPECT (ires == EOF) {
		FSCANF_FAILURE(ires, file, format);
		goto err_close_file;
	}

	fclose(file);
	va_end(args);
	return true;

err_close_file:
	fclose(file);
err_end_args:
	va_end(args);
	return false;
}

bool write_text(const char* filename, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	const char* mode = "w";
	FILE* file = fopen(filename, mode);
	if CZ_NOEXPECT (!file) {
		FOPEN_FAILURE(file, filename, mode);
		goto err_end_args;
	}

	int ires = vfprintf(file, format, args);
	if CZ_NOEXPECT (ires < 0) {
		FPRINTF_FAILURE(ires, file, format);
		goto err_close_file;
	}

	fclose(file);
	va_end(args);
	return true;

err_close_file:
	fclose(file);
err_end_args:
	va_end(args);
	return false;
}

#define UINT(sz) uint##sz##_t
#define UMAX_DEF(sz) UINT(sz) maxu##sz(UINT(sz) x, UINT(sz) y) { return x > y ? x : y; }
#define UMIN_DEF(sz) UINT(sz) minu##sz(UINT(sz) x, UINT(sz) y) { return x < y ? x : y; }

UMAX_DEF(8)
UMIN_DEF(8)

UMAX_DEF(16)
UMIN_DEF(16)

UMAX_DEF(32)
UMIN_DEF(32)

UMAX_DEF(64)
UMIN_DEF(64)

uint8_t maxu8v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast8_t max = 0;

	for (size_t i = 0; i < count; i++) {
		uint_fast8_t arg = (uint_fast8_t) va_arg(args, unsigned int);

		if (max < arg) {
			max = arg;
		}
	}

	va_end(args);
	return (uint8_t) max;
}

uint8_t minu8v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast8_t min = UINT8_MAX;

	for (size_t i = 0; i < count; i++) {
		uint_fast8_t arg = (uint_fast8_t) va_arg(args, unsigned int);

		if (min > arg) {
			min = arg;
		}
	}

	va_end(args);
	return (uint8_t) min;
}

uint16_t maxu16v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast16_t max = 0;

	for (size_t i = 0; i < count; i++) {
		uint_fast16_t arg = (uint_fast16_t) va_arg(args, unsigned int);

		if (max < arg) {
			max = arg;
		}
	}

	va_end(args);
	return (uint16_t) max;
}

uint16_t minu16v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast16_t min = UINT16_MAX;

	for (size_t i = 0; i < count; i++) {
		uint_fast16_t arg = (uint_fast16_t) va_arg(args, unsigned int);

		if (min > arg) {
			min = arg;
		}
	}

	va_end(args);
	return (uint16_t) min;
}

uint32_t maxu32v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast32_t max = 0;

	for (size_t i = 0; i < count; i++) {
		uint_fast32_t arg = (uint_fast32_t) va_arg(args, uint32_t);

		if (max < arg) {
			max = arg;
		}
	}

	va_end(args);
	return (uint32_t) max;
}

uint32_t minu32v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast32_t min = UINT32_MAX;

	for (size_t i = 0; i < count; i++) {
		uint_fast32_t arg = (uint_fast32_t) va_arg(args, uint32_t);

		if (min > arg) {
			min = arg;
		}
	}

	va_end(args);
	return (uint32_t) min;
}

uint64_t maxu64v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast64_t max = 0;

	for (size_t i = 0; i < count; i++) {
		uint_fast64_t arg = (uint_fast64_t) va_arg(args, uint64_t);

		if (max < arg) {
			max = arg;
		}
	}

	va_end(args);
	return (uint64_t) max;
}

uint64_t minu64v(size_t count, ...)
{
	CZ_ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast64_t min = UINT64_MAX;

	for (size_t i = 0; i < count; i++) {
		uint_fast64_t arg = (uint_fast64_t) va_arg(args, uint64_t);

		if (min > arg) {
			min = arg;
		}
	}

	va_end(args);
	return (uint64_t) min;
}
