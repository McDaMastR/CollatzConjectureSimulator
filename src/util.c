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

#include "util.h"
#include "debug.h"


char* stime(void)
{
	time_t t = time(NULL);
	return ctime(&t);
}

clock_t program_time(void)
{
	clock_t t = clock();
	return (clock_t) ((float) t * MS_PER_CLOCK);
}

uint32_t clz(uint32_t x)
{
	ASSUME(x != 0);

#if has_builtin(clz)
	return (uint32_t) __builtin_clz(x);
#else
	uint32_t c;
	for (c = 0; x & (1U << (31U - c)) == 0; c++);
	return c;
#endif
}

uint32_t floor_pow2(uint32_t x)
{
	ASSUME(x != 0);
	return 1U << (31U - clz(x));
}

float get_benchmark(clock_t start, clock_t end)
{
	return (float) (end - start) * MS_PER_CLOCK;
}

bool set_debug_name(restrict VkDevice device, VkObjectType type, uint64_t handle, const char* restrict name)
{
	VkResult vkres;

	VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	debugUtilsObjectNameInfo.objectType   = type;
	debugUtilsObjectNameInfo.objectHandle = handle;
	debugUtilsObjectNameInfo.pObjectName  = name;

	VK_CALL_RES(vkSetDebugUtilsObjectNameEXT, device, &debugUtilsObjectNameInfo)
	if (EXPECT_FALSE(vkres)) { return false; }

	return true;
}

bool get_buffer_requirements_noext(restrict VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* restrict memoryRequirements)
{
	VkResult vkres;

	VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferCreateInfo.size  = size;
	bufferCreateInfo.usage = usage;

	VkBuffer buffer;
	VK_CALL_RES(vkCreateBuffer, device, &bufferCreateInfo, g_allocator, &buffer)
	if (EXPECT_FALSE(vkres)) { return false; }

	VkBufferMemoryRequirementsInfo2 bufferMemoryRequirementsInfo2 = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
	bufferMemoryRequirementsInfo2.buffer = buffer;

	VkMemoryRequirements2 memoryRequirements2 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};

	VK_CALL(vkGetBufferMemoryRequirements2, device, &bufferMemoryRequirementsInfo2, &memoryRequirements2)

	VK_CALL(vkDestroyBuffer, device, buffer, g_allocator)

	*memoryRequirements = memoryRequirements2.memoryRequirements;
	return true;
}

bool get_buffer_requirements_main4(restrict VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* restrict memoryRequirements)
{
	VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferCreateInfo.size  = size;
	bufferCreateInfo.usage = usage;

	VkDeviceBufferMemoryRequirementsKHR deviceBufferMemoryRequirements = {VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS_KHR};
	deviceBufferMemoryRequirements.pCreateInfo = &bufferCreateInfo;

	VkMemoryRequirements2 memoryRequirements2 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};

	VK_CALL(vkGetDeviceBufferMemoryRequirementsKHR, device, &deviceBufferMemoryRequirements, &memoryRequirements2)

	*memoryRequirements = memoryRequirements2.memoryRequirements;
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
	if (EXPECT_FALSE(ires)) { FSEEK_FAILURE(ires, file, 0, SEEK_END); fclose(file); return false; }

	long lres = ftell(file);
	if (EXPECT_FALSE(lres == -1)) { FTELL_FAILURE(lres, file); fclose(file); return false; }

	*size = (size_t) lres;

	fclose(file);
	return true;
}

bool read_file(const char* restrict filename, void* restrict data, size_t size)
{
	FILE* file = fopen(filename, "rb");
	if (EXPECT_FALSE(!file)) { FOPEN_FAILURE(file, filename, "rb"); return false; }

	size_t sres = fread(data, sizeof(char), size, file);
	if (EXPECT_FALSE(sres != size)) { FREAD_FAILURE(sres, data, sizeof(char), size, file); fclose(file); return false; }

	fclose(file);
	return true;
}

bool write_file(const char* restrict filename, const void* restrict data, size_t size)
{
	FILE* file = fopen(filename, "wb");
	if (EXPECT_FALSE(!file)) { FOPEN_FAILURE(file, filename, "wb"); return false; }

	size_t sres = fwrite(data, sizeof(char), size, file);
	if (EXPECT_FALSE(sres != size)) { FWRITE_FAILURE(sres, data, sizeof(char), size, file); fclose(file); return false; }

	fclose(file);
	return true;
}


typedef struct AlignedInfo
{
	void* start;
	size_t size;
} AlignedInfo;


void* aligned_malloc(size_t size, size_t alignment)
{
	ASSUME(size != 0);
	ASSUME((alignment & (alignment - 1)) == 0);

	void*     memory;
	uintptr_t address;

#if defined(_MSC_VER) || defined(__MINGW32__)
	if (alignment < alignof(size_t)) { alignment = alignof(size_t); }

	memory = _aligned_offset_malloc(size + sizeof(size_t), alignment, sizeof(size_t));
	if (EXPECT_FALSE(!memory)) { return NULL; }

	address = (uintptr_t) memory;

	size_t* info = (size_t*) address;
	*info = size;

	address += sizeof(size_t);
	memory = (void*) address;
#else
	if (alignment < alignof(AlignedInfo)) { alignment = alignof(AlignedInfo); }

	int ires = posix_memalign(&memory, alignment, size + alignment + sizeof(AlignedInfo));
	if (EXPECT_FALSE(ires)) { return NULL; }

	address = (uintptr_t) memory;
	address += ((sizeof(AlignedInfo) - 1) / alignment + 1) * alignment;
	address -= sizeof(AlignedInfo);

	AlignedInfo* info = (AlignedInfo*) address;
	info->start = memory;
	info->size  = size;

	address += sizeof(AlignedInfo);
	memory = (void*) address;
#endif

	return memory;
}

void* aligned_realloc(void* restrict memory, size_t size, size_t alignment)
{
	ASSUME(size != 0);
	ASSUME((alignment & (alignment - 1)) == 0);

	void* newMemory;

	uintptr_t address = (uintptr_t) memory;

#if defined(_MSC_VER) || defined(__MINGW32__)
	address -= sizeof(size_t);
	memory = (void*) address;

	if (alignment < alignof(size_t)) { alignment = alignof(size_t); }

	newMemory = _aligned_offset_realloc(memory, size + sizeof(size_t), alignment, sizeof(size_t));
	if (EXPECT_FALSE(!newMemory)) { return NULL; }

	address = (uintptr_t) newMemory;

	size_t* info = (size_t*) address;
	*info = size;

	address += sizeof(size_t);
	newMemory = (void*) address;
#else
	address -= sizeof(AlignedInfo);

	AlignedInfo* info = (AlignedInfo*) address;
	void*  prevMemory = info->start;
	size_t prevSize   = info->size;

	size_t minSize = size < prevSize ? size : prevSize;

	if (alignment < alignof(AlignedInfo)) { alignment = alignof(AlignedInfo); }

	int ires = posix_memalign(&newMemory, alignment, size + alignment + sizeof(AlignedInfo));
	if (EXPECT_FALSE(ires)) { return NULL; }

	address = (uintptr_t) newMemory;
	address += ((sizeof(AlignedInfo) - 1) / alignment + 1) * alignment;
	address -= sizeof(AlignedInfo);

	info = (AlignedInfo*) address;
	info->start = newMemory;
	info->size  = size;

	address += sizeof(AlignedInfo);
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
	memory = (void*) address;
	_aligned_free(memory);
#else
	address -= sizeof(AlignedInfo);
	AlignedInfo* info = (AlignedInfo*) address;
	memory = info->start;
	free(memory);
#endif

	return NULL;
}

size_t aligned_size(const void* restrict memory)
{
	size_t size;

	uintptr_t address = (uintptr_t) memory;

#if defined(_MSC_VER) || defined(__MINGW32__)
	address -= sizeof(size_t);
	size_t* info = (size_t*) address;
	size = *info;
#else
	address -= sizeof(AlignedInfo);
	AlignedInfo* info = (AlignedInfo*) address;
	size = info->size;
#endif

	return size;
}
