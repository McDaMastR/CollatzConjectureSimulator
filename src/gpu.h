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

bool create_instance   (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool select_device     (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool create_device     (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool manage_memory     (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool create_buffers    (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool create_descriptors(Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool create_pipeline   (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool create_commands   (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool submit_commands   (Gpu* restrict gpu) NONNULL_ARGS_ALL;
bool destroy_gpu       (Gpu* restrict gpu) NONNULL_ARGS_ALL;

bool retrieve_queue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* queue, const char* name)
	NONNULL_ARGS(1, 4) NULTSTR_ARG(5);

bool create_command_handles(
	VkDevice device,
	uint32_t queueFamilyIndex,
	VkCommandPool* commandPool,
	const char* name,
	uint32_t commandBufferCount,
	VkCommandBuffer* commandBuffers) NONNULL_ARGS(1, 3, 6) NULTSTR_ARG(4);

bool capture_pipeline(VkDevice device, VkPipeline pipeline);

void* wait_for_input(void* ptr) NONNULL_ARGS_ALL;

void write_inbuffer(
	Value* restrict mappedInBuffer, Value* restrict firstValue, uint32_t valuesPerInout, uint32_t valuesPerHeap)
	NONNULL_ARGS_ALL;

void read_outbuffer(
	const Count* restrict mappedOutBuffer,
	ValueInfo* restrict prevValues,
	DyArray bestValues,
	DyArray bestCounts,
	uint32_t valuesPerInout) NONNULL_ARGS_ALL;

void new_high(
	const Value* restrict value,
	Count* restrict count,
	Count newCount,
	Value* restrict val0mod1off,
	Value* restrict val1mod6off,
	DyArray bestValues,
	DyArray bestCounts) NONNULL_ARGS_ALL;
