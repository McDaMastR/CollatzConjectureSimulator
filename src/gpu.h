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

#include "common.h"
#include "dynamic.h"

struct Gpu
{
	DyRecord allocRecord;

	VkDebugUtilsMessengerEXT debugMessenger;
	VkAllocationCallbacks* allocator;

	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkQueue computeQueue;
	VkQueue transferQueue;

	VkBuffer* restrict hostVisibleBuffers; // Count = buffersPerHeap
	VkBuffer* restrict deviceLocalBuffers; // Count = buffersPerHeap

	VkDeviceMemory* restrict hostVisibleDeviceMemories; // Count = buffersPerHeap
	VkDeviceMemory* restrict deviceLocalDeviceMemories; // Count = buffersPerHeap

	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet* restrict descriptorSets; // Count = inoutsPerHeap

	VkShaderModule shaderModule;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;

	VkQueryPool queryPool;

	VkCommandPool initialCmdPool;
	VkCommandPool computeCmdPool;
	VkCommandPool transferCmdPool;

	VkCommandBuffer initialCmdBuffer;
	VkCommandBuffer* restrict computeCmdBuffers; // Count = inoutsPerHeap
	VkCommandBuffer* restrict transferCmdBuffers; // Count = inoutsPerHeap

	VkSemaphore* restrict semaphores; // Count = inoutsPerHeap

	CzU128** restrict mappedInBuffers; // Count = inoutsPerHeap, valuesPerInout
	CzU16** restrict mappedOutBuffers; // Count = inoutsPerHeap, valuesPerInout

	VkDeviceSize bytesPerIn;
	VkDeviceSize bytesPerOut;
	VkDeviceSize bytesPerInout;
	VkDeviceSize bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDeviceLocalMemory;

	CzU32 valuesPerInout;
	CzU32 valuesPerBuffer;
	CzU32 valuesPerHeap;
	CzU32 inoutsPerBuffer;
	CzU32 inoutsPerHeap;
	CzU32 buffersPerHeap;

	CzU32 workgroupSize;
	CzU32 workgroupCount;

	CzU32 hostVisibleHeapIndex;
	CzU32 deviceLocalHeapIndex;
	CzU32 hostVisibleTypeIndex;
	CzU32 deviceLocalTypeIndex;

	CzU32 computeFamilyIndex;
	CzU32 transferFamilyIndex;
	CzU32 computeQueueIndex;
	CzU32 transferQueueIndex;
	CzU32 computeFamilyTimestampValidBits;
	CzU32 transferFamilyTimestampValidBits;

	CzU32 vkVerMajor;
	CzU32 vkVerMinor;
	CzU32 spvVerMajor;
	CzU32 spvVerMinor;

	float timestampPeriod;

	bool hostNonCoherent;
	bool using16BitStorage;
	bool usingMaintenance4;
	bool usingMaintenance5;
	bool usingMaintenance7;
	bool usingMaintenance8;
	bool usingMaintenance9;
	bool usingMemoryBudget;
	bool usingMemoryPriority;
	bool usingPipelineCreationCacheControl;
	bool usingPipelineExecutableProperties;
	bool usingPortabilitySubset;
	bool usingShaderInt16;
	bool usingShaderInt64;
	bool usingSubgroupSizeControl;
};

struct Position
{
	/* 
	 * Suppose the current longest total stopping time is T. Then val-a-mod-m-off[k] gives the least starting value x
	 * with total stopping time t such that (1) x ≡ a (mod m) and (2) t + k = T.
	 */
	CzU128 val0mod1off[3];
	CzU128 val1mod6off[3];

	CzU128 curStartValue; // First starting value being checked in the current dispatch.
	CzU16 bestStopTime; // Current longest total stopping time.
};

// If the return type is bool, then the function returns true on success and false elsewise

CZ_NONNULL_ARGS()
bool create_instance(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool select_device(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool create_device(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool manage_memory(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool create_buffers(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool create_descriptors(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool create_pipeline(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool create_commands(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool submit_commands(struct Gpu* gpu);
CZ_NONNULL_ARGS()
bool destroy_gpu(struct Gpu* gpu);

CZ_NONNULL_ARGS()
bool capture_pipeline(VkDevice device, VkPipeline pipeline);

CZ_NONNULL_ARGS()
void* wait_for_input(void* ptr);

CZ_NONNULL_ARGS()
void write_inbuffer(CzU128* mappedInBuffer, CzU128* firstStartValue, CzU32 valuesPerInout, CzU32 valuesPerHeap);

CZ_NONNULL_ARGS()
void read_outbuffer(
	const CzU16* mappedOutBuffer,
	struct Position* position,
	DyArray bestStartValues,
	DyArray bestStopTimes,
	CzU32 valuesPerInout);

CZ_NONNULL_ARGS()
void new_high(
	const CzU128* startValue,
	CzU16* curBestTime,
	CzU16 newBestTime,
	CzU128* val0mod1off,
	CzU128* val1mod6off,
	DyArray bestStartValues,
	DyArray bestStopTimes);
