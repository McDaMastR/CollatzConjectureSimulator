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
#include "debug.h"


#ifdef _MSC_VER
	#pragma intrinsic(_BitScanReverse)
#endif


typedef struct AlignedInfo
{
	void* start;
	size_t size;
} AlignedInfo;


bool fisatty(FILE* stream)
{
#if defined(_MSC_VER) || defined(__MINGW32__)
	int tty = _isatty(_fileno(stream));
#else
	int tty = isatty(fileno(stream));
#endif

	return (bool) tty;
}

char* stime(void)
{
	time_t t = time(NULL);
	return ctime(&t);
}

double program_time(void)
{
	clock_t t = clock();
	return (double) t * MS_PER_CLOCK;
}

Endianness get_endianness(void)
{
	int x = 1;
	char c = *(char*) &x;
	return c ? ENDIANNESS_LITTLE : ENDIANNESS_BIG;
}

uint32_t ceil_pow2(uint32_t x)
{
	ASSUME(x != 0);

#if has_builtin(stdc_bit_ceil)
	return __builtin_stdc_bit_ceil(x);
#elif has_builtin(clz) && UINT32_MAX == UINT_MAX
	return x == 1 ? UINT32_C(1) : UINT32_C(2) << (31 - __builtin_clz(x - 1));
#elif has_builtin(clzl) && UINT32_MAX == ULONG_MAX
	return x == 1 ? UINT32_C(1) : UINT32_C(2) << (31 - __builtin_clzl(x - 1));
#elif defined(_MSC_VER) || defined(__MINGW32__)
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
	ASSUME(x != 0);

#if has_builtin(stdc_bit_floor)
	return __builtin_stdc_bit_floor(x);
#elif has_builtin(clz) && UINT32_MAX == UINT_MAX
	return UINT32_C(1) << (31 - __builtin_clz(x));
#elif has_builtin(clzl) && UINT32_MAX == ULONG_MAX
	return UINT32_C(1) << (31 - __builtin_clzl(x));
#elif defined(_MSC_VER) || defined(__MINGW32__)
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
	return (double) (end - start) * MS_PER_CLOCK;
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

	VK_CALL_RES(vkSetDebugUtilsObjectNameEXT, device, &info);
	if EXPECT_FALSE (vkres) { return false; }

	return true;
}

bool get_buffer_requirements_noext(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements)
{
	VkResult vkres;

	VkBufferCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;

	VkBuffer buffer;
	VK_CALL_RES(vkCreateBuffer, device, &createInfo, g_allocator, &buffer);
	if EXPECT_FALSE (vkres) { return false; }

	VkBufferMemoryRequirementsInfo2 requirementsInfo = {0};
	requirementsInfo.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
	requirementsInfo.buffer = buffer;

	VkMemoryRequirements2 memoryRequirements = {0};
	memoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;

	VK_CALL(vkGetBufferMemoryRequirements2, device, &requirementsInfo, &memoryRequirements);
	VK_CALL(vkDestroyBuffer, device, buffer, g_allocator);

	*requirements = memoryRequirements.memoryRequirements;
	return true;
}

bool get_buffer_requirements_main4(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements)
{
	VkBufferCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	createInfo.size = size;
	createInfo.usage = usage;

	VkDeviceBufferMemoryRequirements requirementsInfo = {0};
	requirementsInfo.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS;
	requirementsInfo.pCreateInfo = &createInfo;

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
	VK_CALL_RES(vkGetPipelineCacheData, device, cache, &dataSize, NULL);
	if EXPECT_FALSE (vkres) { return false; }

	void* data = malloc(dataSize);
	if EXPECT_FALSE (!data) { MALLOC_FAILURE(data, dataSize); return false; }

	VK_CALL_RES(vkGetPipelineCacheData, device, cache, &dataSize, data);
	if EXPECT_FALSE (vkres) { free(data); return false; }

	bool bres = write_file(filename, data, dataSize);
	if EXPECT_FALSE (!bres) { free(data); return false; }

	free(data);
	return true;
}


bool file_size(const char* filename, size_t* size)
{
	const char* mode = "rb";
	FILE* file = fopen(filename, mode);

	if (!file) {
		*size = 0;
		return true;
	}

	long offset = 0;
	int origin = SEEK_END;

	int ires = fseek(file, offset, origin);
	if EXPECT_FALSE (ires) { FSEEK_FAILURE(ires, file, offset, origin); fclose(file); return false; }

	long lres = ftell(file);
	if EXPECT_FALSE (lres == -1) { FTELL_FAILURE(lres, file); fclose(file); return false; }

	fclose(file);
	*size = (size_t) lres;

	return true;
}

bool read_file(const char* filename, void* data, size_t size)
{
	const char* mode = "rb";
	FILE* file = fopen(filename, mode);
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, mode); return false; }

	size_t objSize = sizeof(char);
	size_t objCount = size;

	size_t sres = fread(data, objSize, objCount, file);
	if EXPECT_FALSE (sres != size) { FREAD_FAILURE(sres, data, objSize, objCount, file); fclose(file); return false; }

	fclose(file);
	return true;
}

bool write_file(const char* filename, const void* data, size_t size)
{
	const char* mode = "wb";
	FILE* file = fopen(filename, mode);
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, mode); return false; }

	size_t objSize = sizeof(char);
	size_t objCount = size;

	size_t sres = fwrite(data, objSize, objCount, file);
	if EXPECT_FALSE (sres != size) { FWRITE_FAILURE(sres, data, objSize, objCount, file); fclose(file); return false; }

	fclose(file);
	return true;
}

bool read_text(const char* filename, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	const char* mode = "r";
	FILE* file = fopen(filename, mode);
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, mode); va_end(args); return false; }

	int ires = vfscanf(file, format, args);
	if EXPECT_FALSE (ires == EOF) { FSCANF_FAILURE(ires, file, format); fclose(file); va_end(args); return false; }

	fclose(file);
	va_end(args);

	return true;
}

bool write_text(const char* filename, const char* format, ...)
{
	va_list args;
	va_start(args, format);

	const char* mode = "w";
	FILE* file = fopen(filename, mode);
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, mode); va_end(args); return false; }

	int ires = vfprintf(file, format, args);
	if EXPECT_FALSE (ires < 0) { FPRINTF_FAILURE(ires, file, format); fclose(file); va_end(args); return false; }

	fclose(file);
	va_end(args);

	return true;
}

void* aligned_malloc(size_t size, size_t alignment)
{
	ASSUME(size != 0);
	ASSUME((alignment & (alignment - 1)) == 0); // alignment is a power of two

	void* memory;
	AlignedInfo* info;

#if defined(_MSC_VER) || defined(__MINGW32__)
	size_t allocSize = size + sizeof(AlignedInfo);
	size_t allocAlignment = maxz(alignment, alignof(AlignedInfo));
	size_t offset = sizeof(AlignedInfo);

	memory = _aligned_offset_malloc(allocSize, allocAlignment, offset);
	if EXPECT_FALSE (!memory) { return NULL; }

	info = memory;
#else
	size_t allocAlignment = maxz(alignment, alignof(AlignedInfo));
	size_t allocSize = size + maxz(alignment, sizeof(AlignedInfo));

	int ires = posix_memalign(&memory, allocAlignment, allocSize);
	if EXPECT_FALSE (ires) { return NULL; }

	info = (AlignedInfo*) memory + maxz(alignment, sizeof(AlignedInfo)) / sizeof(AlignedInfo) - 1;
#endif

	info->start = memory;
	info->size = size;

	return info + 1;
}

void* aligned_realloc(void* memory, size_t size, size_t alignment)
{
	ASSUME(size != 0);
	ASSUME((alignment & (alignment - 1)) == 0); // alignment is a power of two

	AlignedInfo* info = (AlignedInfo*) memory - 1;
	void* oldMemory = info->start;
	size_t oldSize = info->size;

	void* newMemory;

#if defined(_MSC_VER) || defined(__MINGW32__)
	size_t allocSize = size + sizeof(AlignedInfo);
	size_t allocAlignment = maxz(alignment, alignof(AlignedInfo));
	size_t offset = sizeof(AlignedInfo);

	newMemory = _aligned_offset_realloc(oldMemory, allocSize, allocAlignment, offset);
	if EXPECT_FALSE (!newMemory) { return NULL; }

	info = newMemory;
#else
	size_t allocAlignment = maxz(alignment, alignof(AlignedInfo));
	size_t allocSize = size + maxz(alignment, sizeof(AlignedInfo));

	int ires = posix_memalign(&newMemory, allocAlignment, allocSize);
	if EXPECT_FALSE (ires) { return NULL; }

	info = (AlignedInfo*) newMemory + maxz(alignment, sizeof(AlignedInfo)) / sizeof(AlignedInfo) - 1;

	void* cpyDest = info + 1;
	void* cpySrc = memory;
	size_t cpySize = minz(size, oldSize);

	memcpy(cpyDest, cpySrc, cpySize);
	free(oldMemory);
#endif

	info->start = newMemory;
	info->size = size;

	return info + 1;
}

void aligned_free(void* memory)
{
	AlignedInfo* info = (AlignedInfo*) memory - 1;

#if defined(_MSC_VER) || defined(__MINGW32__)
	_aligned_free(info->start);
#else
	free(info->start);
#endif
}

size_t aligned_size(const void* memory)
{
	const AlignedInfo* info = (const AlignedInfo*) memory - 1;
	return info->size;
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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
	ASSUME(count != 0);

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
