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

#include "dyarray.h"


typedef struct Gpu
{
	VkDebugUtilsMessengerEXT debugUtilsMessenger;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

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

	VkCommandPool onetimeCommandPool;
	VkCommandPool transferCommandPool;
	VkCommandPool computeCommandPool;

	VkCommandBuffer           onetimeCommandBuffer;
	VkCommandBuffer* restrict transferCommandBuffers; // Count = inoutsPerHeap
	VkCommandBuffer* restrict computeCommandBuffers;  // Count = inoutsPerHeap

	VkSemaphore* restrict semaphores; // Count = inoutsPerHeap

	Value** restrict mappedInBuffers;  // Count = inoutsPerHeap, valuesPerInout
	Steps** restrict mappedOutBuffers; // Count = inoutsPerHeap, valuesPerInout

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

	uint32_t workgroupCount;
	uint32_t workgroupSize;

	uint32_t hostVisibleHeapIndex;
	uint32_t deviceLocalHeapIndex;

	uint32_t hostVisibleTypeIndex;
	uint32_t deviceLocalTypeIndex;

	uint32_t transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex;

	uint32_t transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits;

	float timestampPeriod;

	bool hostNonCoherent;
	bool using8BitStorage;
	bool usingBufferDeviceAddress;
	bool usingMaintenance4;
	bool usingMemoryBudget;
	bool usingMemoryPriority;
	bool usingPipelineCreationCacheControl;
	bool usingPortabilitySubset;
	bool usingShaderInt16;
	bool usingShaderInt64;
	bool usingSpirv14;
	bool usingSubgroupSizeControl;
	bool usingVulkan12;
	bool usingVulkan13;

	void* dynamicMemory;
} Gpu;


// If the return type is bool, then that function returns true on success, and false elsewise

bool create_instance(Gpu* gpu)    NONNULL_ARGS(1);
bool select_device(Gpu* gpu)      NONNULL_ARGS(1);
bool create_device(Gpu* gpu)      NONNULL_ARGS(1);
bool manage_memory(Gpu* gpu)      NONNULL_ARGS(1);
bool create_buffers(Gpu* gpu)     NONNULL_ARGS(1);
bool create_descriptors(Gpu* gpu) NONNULL_ARGS(1);
bool create_pipeline(Gpu* gpu)    NONNULL_ARGS(1);
bool create_commands(Gpu* gpu)    NONNULL_ARGS(1);
bool submit_commands(Gpu* gpu)    NONNULL_ARGS(1);
bool destroy_gpu(Gpu* gpu)        NONNULL_ARGS(1);

void* wait_for_input(void* ptr) NONNULL_ARGS(1) WR_ACCESS(1);

void writeInBuffer(Value* mappedInBuffer, Value* firstValue, uint32_t valuesPerInout, uint32_t valuesPerHeap) NONNULL_ARGS(1, 2) WR_ACCESS(1) RW_ACCESS(2);
void readOutBuffer(const Steps* mappedOutBuffer, Value* firstValue, DyArray highestStepValues, DyArray highestStepCounts, uint32_t valuesPerInout) NONNULL_ARGS(1, 2, 3, 4) RE_ACCESS(1) RW_ACCESS(2) RW_ACCESS(3) RW_ACCESS(4);
