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
	int  x = 1;
	char c = *(char*) &x;
	return c ? ENDIANNESS_LITTLE : ENDIANNESS_BIG;
}

uint32_t ceil_pow2(uint32_t x)
{
	ASSUME(x != 0);

#if has_builtin(stdc_bit_ceil)
	return __builtin_stdc_bit_ceil(x);
#elif UINT32_MAX == UINT_MAX && has_builtin(clz)
	return x == 1U ? 1U : 2U << (31 - __builtin_clz(x - 1U));
#elif UINT32_MAX == ULONG_MAX && has_builtin(clzl)
	return x == 1UL ? 1UL : 2UL << (31 - __builtin_clzl(x - 1UL));
#elif defined(_MSC_VER) || defined(__MINGW32__)
	unsigned long i;
	_BitScanReverse(&i, x - 1UL);
	return 1UL << (i + 1UL);
#else
	uint32_t y = (uint32_t) 1 << 31;
	while (!(x & y)) { y >>= 1; }
	return y << 1;
#endif
}

uint32_t floor_pow2(uint32_t x)
{
	ASSUME(x != 0);

#if has_builtin(stdc_bit_floor)
	return __builtin_stdc_bit_floor(x);
#elif UINT32_MAX == UINT_MAX && has_builtin(clz)
	return 1U << (31 - __builtin_clz(x));
#elif UINT32_MAX == ULONG_MAX && has_builtin(clzl)
	return 1UL << (31 - __builtin_clzl(x));
#elif defined(_MSC_VER) || defined(__MINGW32__)
	unsigned long i;
	_BitScanReverse(&i, x);
	return 1UL << i;
#else
	uint32_t y = (uint32_t) 1 << 31;
	while (!(x & y)) { y >>= 1; }
	return y;
#endif
}

double get_benchmark(clock_t start, clock_t end)
{
	return (double) (end - start) * MS_PER_CLOCK;
}

bool set_debug_name(VkDevice device, VkObjectType type, uint64_t handle, const char* restrict name)
{
	VkResult vkres;

	VkDebugUtilsObjectNameInfoEXT info = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	info.objectType                    = type;
	info.objectHandle                  = handle;
	info.pObjectName                   = name;

	VK_CALL_RES(vkSetDebugUtilsObjectNameEXT, device, &info);
	if EXPECT_FALSE (vkres) return false;

	return true;
}

bool get_buffer_requirements_noext(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* restrict requirements)
{
	VkResult vkres;

	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size               = size;
	createInfo.usage              = usage;

	VkBuffer buffer;

	VK_CALL_RES(vkCreateBuffer, device, &createInfo, g_allocator, &buffer);
	if EXPECT_FALSE (vkres) return false;

	VkBufferMemoryRequirementsInfo2 requirementsInfo = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
	requirementsInfo.buffer                          = buffer;

	VkMemoryRequirements2 memoryRequirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};

	VK_CALL(vkGetBufferMemoryRequirements2, device, &requirementsInfo, &memoryRequirements);

	VK_CALL(vkDestroyBuffer, device, buffer, g_allocator);

	*requirements = memoryRequirements.memoryRequirements;
	return true;
}

bool get_buffer_requirements_main4(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* restrict requirements)
{
	VkBufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	createInfo.size               = size;
	createInfo.usage              = usage;

	VkDeviceBufferMemoryRequirements requirementsInfo = {VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS};
	requirementsInfo.pCreateInfo                      = &createInfo;

	VkMemoryRequirements2 memoryRequirements = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};

	VK_CALL(vkGetDeviceBufferMemoryRequirementsKHR, device, &requirementsInfo, &memoryRequirements);

	*requirements = memoryRequirements.memoryRequirements;
	return true;
}


bool file_size(const char* restrict filename, size_t* restrict size)
{
	FILE* file = fopen(filename, "rb");

	if (!file) {
		*size = 0;
		return true;
	}

	int ires = fseek(file, 0, SEEK_END);
	if EXPECT_FALSE (ires) { FSEEK_FAILURE(ires, file, 0, SEEK_END); fclose(file); return false; }

	long lres = ftell(file);
	if EXPECT_FALSE (lres == -1) { FTELL_FAILURE(lres, file); fclose(file); return false; }

	fclose(file);

	*size = (size_t) lres;
	return true;
}

bool read_file(const char* restrict filename, void* restrict data, size_t size)
{
	FILE* file = fopen(filename, "rb");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, "rb"); return false; }

	size_t sres = fread(data, sizeof(char), size, file);
	if EXPECT_FALSE (sres != size) { FREAD_FAILURE(sres, data, sizeof(char), size, file); fclose(file); return false; }

	fclose(file);

	return true;
}

bool write_file(const char* restrict filename, const void* restrict data, size_t size)
{
	FILE* file = fopen(filename, "wb");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, "wb"); return false; }

	size_t sres = fwrite(data, sizeof(char), size, file);
	if EXPECT_FALSE (sres != size) { FWRITE_FAILURE(sres, data, sizeof(char), size, file); fclose(file); return false; }

	fclose(file);

	return true;
}

bool read_text(const char* restrict filename, const char* restrict format, ...)
{
	va_list args;
	va_start(args, format);

	FILE* file = fopen(filename, "r");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, "r"); va_end(args); return false; }

	int ires = vfscanf(file, format, args);
	if EXPECT_FALSE (ires == EOF) { FSCANF_FAILURE(ires, file, format); fclose(file); va_end(args); return false; }

	fclose(file);
	va_end(args);

	return true;
}

bool write_text(const char* restrict filename, const char* restrict format, ...)
{
	va_list args;
	va_start(args, format);

	FILE* file = fopen(filename, "w");
	if EXPECT_FALSE (!file) { FOPEN_FAILURE(file, filename, "w"); va_end(args); return false; }

	int ires = vfprintf(file, format, args);
	if EXPECT_FALSE (ires < 0) { FPRINTF_FAILURE(ires, file, format); fclose(file); va_end(args); return false; }

	fclose(file);
	va_end(args);

	return true;
}


void* aligned_malloc(size_t size, size_t alignment)
{
	ASSUME(size != 0);
	ASSUME((alignment & (alignment - 1)) == 0);

	void*     memory;
	uintptr_t address;

#if defined(_MSC_VER) || defined(__MINGW32__)
	if (alignment < alignof(size_t)) { alignment = alignof(size_t); }

	memory = _aligned_offset_malloc(size + sizeof(size_t), alignment, sizeof(size_t));
	if EXPECT_FALSE (!memory) return NULL;

	address = (uintptr_t) memory;

	size_t* info = (size_t*) address;
	*info        = size;

	address += sizeof(size_t);
	memory  = (void*) address;
#else
	if (alignment < alignof(AlignedInfo)) { alignment = alignof(AlignedInfo); }

	int ires = posix_memalign(&memory, alignment, size + alignment + sizeof(AlignedInfo));
	if EXPECT_FALSE (ires) return NULL;

	address = (uintptr_t) memory;
	address += ((sizeof(AlignedInfo) - 1) / alignment + 1) * alignment;
	address -= sizeof(AlignedInfo);

	AlignedInfo* info = (AlignedInfo*) address;
	info->start       = memory;
	info->size        = size;

	address += sizeof(AlignedInfo);
	memory  = (void*) address;
#endif

	return memory;
}

void* aligned_realloc(void* restrict memory, size_t size, size_t alignment)
{
	ASSUME(size != 0);
	ASSUME((alignment & (alignment - 1)) == 0);

	uintptr_t address = (uintptr_t) memory;

	void* newMemory;

#if defined(_MSC_VER) || defined(__MINGW32__)
	address -= sizeof(size_t);
	memory  = (void*) address;

	if (alignment < alignof(size_t)) { alignment = alignof(size_t); }

	newMemory = _aligned_offset_realloc(memory, size + sizeof(size_t), alignment, sizeof(size_t));
	if EXPECT_FALSE (!newMemory) return NULL;

	address = (uintptr_t) newMemory;

	size_t* info = (size_t*) address;
	*info        = size;

	address   += sizeof(size_t);
	newMemory = (void*) address;
#else
	address -= sizeof(AlignedInfo);

	AlignedInfo* info = (AlignedInfo*) address;
	void*  prevMemory = info->start;
	size_t prevSize   = info->size;

	size_t minSize = minz(size, prevSize);

	if (alignment < alignof(AlignedInfo)) { alignment = alignof(AlignedInfo); }

	int ires = posix_memalign(&newMemory, alignment, size + alignment + sizeof(AlignedInfo));
	if EXPECT_FALSE (ires) return NULL;

	address = (uintptr_t) newMemory;
	address += ((sizeof(AlignedInfo) - 1) / alignment + 1) * alignment;
	address -= sizeof(AlignedInfo);

	info        = (AlignedInfo*) address;
	info->start = newMemory;
	info->size  = size;

	address   += sizeof(AlignedInfo);
	newMemory = (void*) address;

	memcpy(newMemory, memory, minSize);
	free(prevMemory);
#endif

	return newMemory;
}

void* aligned_free(void* restrict memory)
{
	uintptr_t address = (uintptr_t) memory;

#if defined(_MSC_VER) || defined(__MINGW32__)
	address -= sizeof(size_t);
	memory  = (void*) address;
	_aligned_free(memory);
#else
	address -= sizeof(AlignedInfo);
	memory  = ((AlignedInfo*) address)->start;
	free(memory);
#endif

	return NULL;
}

size_t aligned_size(const void* restrict memory)
{
	uintptr_t address = (uintptr_t) memory;

	size_t size;

#if defined(_MSC_VER) || defined(__MINGW32__)
	address -= sizeof(size_t);
	size    = *(size_t*) address;
#else
	address -= sizeof(AlignedInfo);
	size    = ((AlignedInfo*) address)->size;
#endif

	return size;
}


uint8_t maxu8(uint8_t x, uint8_t y)
{
	return x > y ? x : y;
}

uint8_t minu8(uint8_t x, uint8_t y)
{
	return x < y ? x : y;
}

uint16_t maxu16(uint16_t x, uint16_t y)
{
	return x > y ? x : y;
}

uint16_t minu16(uint16_t x, uint16_t y)
{
	return x < y ? x : y;
}

uint32_t maxu32(uint32_t x, uint32_t y)
{
	return x > y ? x : y;
}

uint32_t minu32(uint32_t x, uint32_t y)
{
	return x < y ? x : y;
}

uint64_t maxu64(uint64_t x, uint64_t y)
{
	return x > y ? x : y;
}

uint64_t minu64(uint64_t x, uint64_t y)
{
	return x < y ? x : y;
}

uint8_t maxu8v(size_t count, ...)
{
	ASSUME(count != 0);

	va_list args;
	va_start(args, count);

	uint_fast8_t max = 0;

	for (size_t i = 0; i < count; i++) {
		uint_fast8_t arg = (uint_fast8_t) va_arg(args, unsigned int);
		if (max < arg) { max = arg; }
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
		if (min > arg) { min = arg; }
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
		if (max < arg) { max = arg; }
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
		if (min > arg) { min = arg; }
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
		if (max < arg) { max = arg; }
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
		if (min > arg) { min = arg; }
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
		if (max < arg) { max = arg; }
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
		if (min > arg) { min = arg; }
	}

	va_end(args);

	return (uint64_t) min;
}
