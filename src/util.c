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

#include "defs.h"

uint32_t floor_pow2(uint32_t n)
{
	for (uint32_t i = 0; n & (n - 1); i++)
		n &= ~(1U << i);

	return n;
}

float get_benchmark(clock_t start, clock_t end)
{
	return (float) (end - start) * MS_PER_CLOCK;
}

bool set_debug_name(VkDevice device, VkObjectType type, uint64_t handle, const char* restrict name)
{
	VkResult vkres;

	VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT};
	debugUtilsObjectNameInfo.objectType   = type;
	debugUtilsObjectNameInfo.objectHandle = handle;
	debugUtilsObjectNameInfo.pObjectName  = name;

	VK_CALL_RES(vkSetDebugUtilsObjectNameEXT, device, &debugUtilsObjectNameInfo)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkSetDebugUtilsObjectNameEXT)
		return false;
	}
#endif

	return true;
}

bool get_buffer_requirements_noext(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* restrict memoryRequirements)
{
	VkResult vkres;

	VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferCreateInfo.size        = size;
	bufferCreateInfo.usage       = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VK_CALL_RES(vkCreateBuffer, device, &bufferCreateInfo, g_allocator, &buffer)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateBuffer)
		return false;
	}
#endif

	VkBufferMemoryRequirementsInfo2 bufferMemoryRequirementsInfo2 = {VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2};
	bufferMemoryRequirementsInfo2.buffer = buffer;

	VkMemoryRequirements2 memoryRequirements2 = {VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2};

	VK_CALL(vkGetBufferMemoryRequirements2, device, &bufferMemoryRequirementsInfo2, &memoryRequirements2)

	VK_CALL(vkDestroyBuffer, device, buffer, g_allocator)

	*memoryRequirements = memoryRequirements2.memoryRequirements;

	return true;
}

bool get_buffer_requirements_main4(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* restrict memoryRequirements)
{
	VkBufferCreateInfo bufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	bufferCreateInfo.size        = size;
	bufferCreateInfo.usage       = usage;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

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
#ifndef NDEBUG
	if (ires) {
		FSEEK_FAILURE(ires, file, 0, SEEK_END)
		fclose(file);
		return false;
	}
#endif

	long lres = ftell(file);
#ifndef NDEBUG
	if (lres == -1) {
		FTELL_FAILURE(lres, file)
		fclose(file);
		return false;
	}
#endif

	*size = (size_t) lres;

	fclose(file);
	return true;
}

bool read_file(const char* restrict filename, void* restrict data, size_t size)
{
	FILE* file = fopen(filename, "rb");
#ifndef NDEBUG
	if (!file) {
		FOPEN_FAILURE(file, filename, "rb")
		return false;
	}
#endif

	size_t sres = fread(data, sizeof(char), size, file);
#ifndef NDEBUG
	if (sres != size) {
		FREAD_FAILURE(sres, data, sizeof(char), size, file)
		fclose(file);
		return false;
	}
#endif

	fclose(file);
	return true;
}

bool write_file(const char* restrict filename, const void* restrict data, size_t size)
{
	FILE* file = fopen(filename, "wb");
#ifndef NDEBUG
	if (!file) {
		FOPEN_FAILURE(file, filename, "wb")
		return false;
	}
#endif

	size_t sres = fwrite(data, sizeof(char), size, file);
#ifndef NDEBUG
	if (sres != size) {
		FWRITE_FAILURE(sres, data, sizeof(char), size, file)
		fclose(file);
		return false;
	}
#endif

	fclose(file);
	return true;
}
