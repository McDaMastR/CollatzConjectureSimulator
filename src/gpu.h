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

#include "dyarray.h"


typedef struct Gpu
{
	VkDebugUtilsMessengerEXT debugUtilsMessenger;

	VkPhysicalDevice physicalDevice;
	VkDevice         device;

	VkQueue transferQueue;
	VkQueue computeQueue;

	VkBuffer* restrict hostVisibleBuffers; // Count = buffersPerHeap
	VkBuffer* restrict deviceLocalBuffers; // Count = buffersPerHeap

	VkDeviceMemory* restrict hostVisibleDeviceMemories; // Count = buffersPerHeap
	VkDeviceMemory* restrict deviceLocalDeviceMemories; // Count = buffersPerHeap

	VkDescriptorSetLayout     descriptorSetLayout;
	VkDescriptorPool          descriptorPool;
	VkDescriptorSet* restrict descriptorSets; // Count = inoutsPerHeap

	VkShaderModule   shaderModule;
	VkPipelineCache  pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline       pipeline;

	VkQueryPool queryPool;

	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;

	VkCommandBuffer* restrict transferCommandBuffers; // Count = inoutsPerHeap
	VkCommandBuffer* restrict computeCommandBuffers;  // Count = inoutsPerHeap

	VkSemaphore* restrict semaphores; // Count = inoutsPerHeap

	Value** restrict mappedInBuffers;  // Count = inoutsPerHeap, valuesPerInout
	Count** restrict mappedOutBuffers; // Count = inoutsPerHeap, valuesPerInout

	VkDeviceSize bytesPerIn;
	VkDeviceSize bytesPerOut;
	VkDeviceSize bytesPerInout;
	VkDeviceSize bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout;
	uint32_t valuesPerBuffer;
	uint32_t valuesPerHeap;
	uint32_t inoutsPerBuffer;
	uint32_t inoutsPerHeap;
	uint32_t buffersPerHeap;

	uint32_t workgroupSize;
	uint32_t workgroupCount;

	uint32_t hostVisibleHeapIndex;
	uint32_t deviceLocalHeapIndex;
	uint32_t hostVisibleTypeIndex;
	uint32_t deviceLocalTypeIndex;

	uint32_t transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex;
	uint32_t transferQueueFamilyTimestampValidBits;
	uint32_t computeQueueFamilyTimestampValidBits;

	uint32_t vkVerMajor;
	uint32_t vkVerMinor;
	uint32_t spvVerMajor;
	uint32_t spvVerMinor;

	float timestampPeriod;

	bool hostNonCoherent;
	bool using8BitStorage;
	bool using16BitStorage;
	bool usingBufferDeviceAddress;
	bool usingMaintenance4;
	bool usingMemoryBudget;
	bool usingMemoryPriority;
	bool usingPipelineCreationCacheControl;
	bool usingPipelineExecutableProperties;
	bool usingPortabilitySubset;
	bool usingShaderInt16;
	bool usingShaderInt64;
	bool usingSubgroupSizeControl;

	CliOutput outputLevel;

	uint32_t iterSize;
	uint64_t maxLoops;
	float    maxMemory;

	bool preferInt16;
	bool preferInt64;

	bool extensionLayers;
	bool profileLayers;
	bool validationLayers;

	bool restartCount;
	bool queryBenchmarking;
	bool logAllocations;
	bool capturePipelines;

	void* dynamicMemory;
} Gpu;

typedef struct ValueInfo
{
	Value val0mod1off[3];
	Value val1mod6off[3];

	Value curValue;
	Count curCount;
} ValueInfo;


// If the return type is bool, then that function returns true on success, and false elsewise

bool parse_cmdline(Gpu* gpu, int argc, char** argv) NONNULL_ARGS_ALL WR_ACCESS(1) RE_ACCESS(3);

bool create_instance(Gpu* gpu) NONNULL_ARGS_ALL;
bool select_device(Gpu* gpu) NONNULL_ARGS_ALL;
bool create_device(Gpu* gpu) NONNULL_ARGS_ALL;
bool manage_memory(Gpu* gpu) NONNULL_ARGS_ALL;
bool create_buffers(Gpu* gpu) NONNULL_ARGS_ALL;
bool create_descriptors(Gpu* gpu) NONNULL_ARGS_ALL;
bool create_pipeline(Gpu* gpu) NONNULL_ARGS_ALL;
bool create_commands(Gpu* gpu) NONNULL_ARGS_ALL;
bool submit_commands(Gpu* gpu) NONNULL_ARGS_ALL;
bool destroy_gpu(Gpu* gpu) NONNULL_ARGS_ALL;

void* wait_for_input(void* ptr) NONNULL_ARGS_ALL WR_ACCESS(1);

void write_inbuffer(Value* mappedInBuffer, Value* firstValue, uint32_t valuesPerInout, uint32_t valuesPerHeap)
	NONNULL_ARGS_ALL WR_ACCESS(1);
void read_outbuffer(
	const Count* mappedOutBuffer,
	ValueInfo* prevValues,
	DyArray bestValues,
	DyArray bestCounts,
	uint32_t valuesPerInout) NONNULL_ARGS_ALL RE_ACCESS(1);

void new_high(
	const Value* value,
	Count* count,
	Count newCount,
	Value* val0mod1off,
	Value* val1mod6off,
	DyArray bestValues,
	DyArray bestCounts) NONNULL_ARGS_ALL RE_ACCESS(1);
