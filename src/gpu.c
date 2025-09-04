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

#include "gpu.h"
#include "debug.h"
#include "dystring.h"
#include "util.h"


// TODO make actual API for recursive memory freeing
typedef struct DyData
{
	void* data;
	void (*free)(void*);
} DyData;


static void* dyarray_append_str(DyArray array, const char* string)
{
	return dyarray_append(array, &string);
}

static void dyarray_destroy_stub(void* array)
{
	dyarray_destroy(array);
}

static void dystring_destroy_stub(void* string)
{
	dystring_destroy(string);
}

static void free_recursive(DyArray array)
{
	size_t count = dyarray_size(array);

	for (size_t i = 0; i < count; i++) {
		DyData dyData;
		dyarray_get(array, &dyData, i);
		dyData.free(dyData.data);
	}

	dyarray_destroy(array);
}


bool create_instance(Gpu* restrict gpu)
{
	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 3;

	/*
	 * Any memory dynamically allocated in this function is recorded in this array, and is all freed by calling
	 * free_recursive(dyMem) before this function returns.
	 */
	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	vkres = volkInitialize();
	if EXPECT_FALSE (vkres) { VKINIT_FAILURE(vkres); free_recursive(dyMem); return false; }

	uint32_t instApiVersion = volkGetInstanceVersion();
	if EXPECT_FALSE (instApiVersion == VK_API_VERSION_1_0) {
		VKVERS_FAILURE(instApiVersion); free_recursive(dyMem); return false; }

	if (g_config.logAllocations) {
		g_allocator = &g_allocationCallbacks;

		bool bres = init_alloc_logfile();
		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

#ifndef NDEBUG
	bool bres = init_debug_logfile();
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
#endif

	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {0};
	messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	messengerCreateInfo.messageSeverity =
		// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerCreateInfo.pfnUserCallback = debug_callback;
	messengerCreateInfo.pUserData = &g_callbackData;

	uint32_t layerCount;
	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	uint32_t extCount;
	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	size_t allocSize = layerCount * sizeof(VkLayerProperties) + extCount * sizeof(VkExtensionProperties);
	void* p = malloc(allocSize);
	if EXPECT_FALSE (!p && allocSize) { MALLOC_FAILURE(p, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	VkLayerProperties* layersProps = p;
	VkExtensionProperties* extsProps = (VkExtensionProperties*) (layersProps + layerCount);

	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerCount, layersProps);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	const char* extLayerName = NULL;
	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, extLayerName, &extCount, extsProps);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	elmSize = sizeof(const char*);
	elmCount = 4;

	DyArray enabledLayers = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!enabledLayers) { free_recursive(dyMem); return false; }

	dyData.data = enabledLayers;
	dyData.free = dyarray_destroy_stub;

	dyarray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < layerCount; i++) {
		const char* layerName = layersProps[i].layerName;

		if (
			(g_config.extensionLayers && (
				!strcmp(layerName, VK_KHR_SYNCHRONIZATION_2_LAYER_NAME) ||
				!strcmp(layerName, VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME))) ||
			(g_config.profileLayers && !strcmp(layerName, VK_KHR_PROFILES_LAYER_NAME)) ||
			(g_config.validationLayers && !strcmp(layerName, VK_KHR_VALIDATION_LAYER_NAME)))
		{
			dyarray_append(enabledLayers, &layerName);
		}
	}

	const void* nextChain = NULL;
	const void** next = &nextChain;

	bool usingPortabilityEnumeration = false;
	bool usingDebugUtils = false;

	elmSize = sizeof(const char*);
	elmCount = 2;

	DyArray enabledExts = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!enabledExts) { free_recursive(dyMem); return false; }

	dyData.data = enabledExts;
	dyData.free = dyarray_destroy_stub;

	dyarray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < extCount; i++) {
		const char* extName = extsProps[i].extensionName;

		if (!strcmp(extName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			(void) next; // Shut up warning for unused variable
			dyarray_append(enabledExts, &extName);
			usingPortabilityEnumeration = true;
		}

#ifndef NDEBUG
		else if (!strcmp(extName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			PNEXT_ADD(next, messengerCreateInfo);
			dyarray_append(enabledExts, &extName);
			usingDebugUtils = true;
		}
#endif
	}

	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = PROGRAM_NAME;
	appInfo.applicationVersion = PROGRAM_VERSION;
	appInfo.apiVersion = VK_API_VERSION_1_4;

	uint32_t enabledLayerCount = (uint32_t) dyarray_size(enabledLayers);
	const char** enabledLayerNames = dyarray_raw(enabledLayers);

	uint32_t enabledExtCount = (uint32_t) dyarray_size(enabledExts);
	const char** enabledExtNames = dyarray_raw(enabledExts);

	VkInstanceCreateInfo instanceCreateInfo = {0};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nextChain;
	instanceCreateInfo.flags = usingPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledLayerCount = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames = enabledLayerNames;
	instanceCreateInfo.enabledExtensionCount = enabledExtCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtNames;

	if (g_config.outputLevel > OUTPUT_LEVEL_DEFAULT) {
		printf("Enabled instance layers (%" PRIu32 "):\n", enabledLayerCount);
		for (uint32_t i = 0; i < enabledLayerCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledLayerNames[i]);
		}
		NEWLINE();

		printf("Enabled instance extensions (%" PRIu32 "):\n", enabledExtCount);
		for (uint32_t i = 0; i < enabledExtCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtNames[i]);
		}
		NEWLINE();
	}

	VkInstance instance;
	VK_CALL_RES(vkCreateInstance, &instanceCreateInfo, g_allocator, &instance);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	volkLoadInstanceOnly(instance);

	if (usingDebugUtils) {
		VkDebugUtilsMessengerEXT messenger;
		VK_CALL_RES(vkCreateDebugUtilsMessengerEXT, instance, &messengerCreateInfo, g_allocator, &messenger);

		if (vkres == VK_SUCCESS) {
			gpu->debugUtilsMessenger = messenger;
		}
	}

	free_recursive(dyMem);
	return true;
}

bool select_device(Gpu* restrict gpu)
{
	VkInstance instance = volkGetLoadedInstance();

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 2;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	uint32_t deviceCount;
	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &deviceCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	if EXPECT_FALSE (!deviceCount) {
		log_critical(stderr, "No physical devices are accessible to the Vulkan instance");
		free_recursive(dyMem);
		return false;
	}

	size_t allocCount = 1;
	size_t allocSize =
		deviceCount * sizeof(VkPhysicalDevice) +
		deviceCount * sizeof(VkQueueFamilyProperties2*) +
		deviceCount * sizeof(VkExtensionProperties*) +
		deviceCount * sizeof(VkPhysicalDeviceMemoryProperties2) +
		deviceCount * sizeof(VkPhysicalDeviceProperties2) +
		deviceCount * sizeof(VkPhysicalDeviceFeatures2) +
		deviceCount * sizeof(VkPhysicalDevice16BitStorageFeatures) +
		deviceCount * sizeof(uint32_t) * 2;

	void* p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	VkPhysicalDevice* devices = p;

	VkQueueFamilyProperties2** qfsProps = (VkQueueFamilyProperties2**) (devices + deviceCount);
	VkExtensionProperties** extsProps = (VkExtensionProperties**) (qfsProps + deviceCount);

	VkPhysicalDeviceMemoryProperties2* devsMemoryProps = (VkPhysicalDeviceMemoryProperties2*) (extsProps + deviceCount);
	VkPhysicalDeviceProperties2* devsProps = (VkPhysicalDeviceProperties2*) (devsMemoryProps + deviceCount);

	VkPhysicalDeviceFeatures2* devsFeats = (VkPhysicalDeviceFeatures2*) (devsProps + deviceCount);
	VkPhysicalDevice16BitStorageFeatures* devs16BitStorageFeats = (VkPhysicalDevice16BitStorageFeatures*) (
		devsFeats + deviceCount);

	uint32_t* extCounts = (uint32_t*) (devs16BitStorageFeats + deviceCount);
	uint32_t* qfCounts = (uint32_t*) (extCounts + deviceCount);

	size_t qfTotal = 0;
	size_t extTotal = 0;

	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &deviceCount, devices);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];

		VkQueueFamilyProperties2* qfProps = NULL;
		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, device, &qfCounts[i], qfProps);

		const char* extLayerName = NULL;
		VkExtensionProperties* extProps = NULL;

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, device, extLayerName, &extCounts[i], extProps);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		qfTotal += qfCounts[i];
		extTotal += extCounts[i];
	}

	allocCount = 1;
	allocSize = qfTotal * sizeof(VkQueueFamilyProperties2) + extTotal * sizeof(VkExtensionProperties);

	p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	qfsProps[0] = p;
	extsProps[0] = (VkExtensionProperties*) (qfsProps[0] + qfTotal);

	for (uint32_t i = 1; i < deviceCount; i++) {
		qfsProps[i] = qfsProps[i - 1] + qfCounts[i - 1];
		extsProps[i] = extsProps[i - 1] + extCounts[i - 1];
	}

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];
		uint32_t qfCount = qfCounts[i];

		const char* extLayerName = NULL;
		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, device, extLayerName, &extCounts[i], extsProps[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		for (uint32_t j = 0; j < qfCount; j++) {
			qfsProps[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, device, &qfCounts[i], qfsProps[i]);

		devsMemoryProps[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		VK_CALL(vkGetPhysicalDeviceMemoryProperties2, device, &devsMemoryProps[i]);

		devsProps[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		VK_CALL(vkGetPhysicalDeviceProperties2, device, &devsProps[i]);

		devs16BitStorageFeats[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;

		devsFeats[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		devsFeats[i].pNext = &devs16BitStorageFeats[i];

		VK_CALL(vkGetPhysicalDeviceFeatures2, device, &devsFeats[i]);
	}

	uint32_t deviceIndex = UINT32_MAX;
	uint32_t bestScore = 0;

	bool using16BitStorage = false;
	bool usingMaintenance4 = false;
	bool usingMaintenance5 = false;
	bool usingMaintenance6 = false;
	bool usingMaintenance7 = false;
	bool usingMaintenance8 = false;
	bool usingMaintenance9 = false;
	bool usingMemoryBudget = false;
	bool usingMemoryPriority = false;
	bool usingPipelineCreationCacheControl = false;
	bool usingPipelineExecutableProperties = false;
	bool usingPortabilitySubset = false;
	bool usingShaderInt16 = false;
	bool usingShaderInt64 = false;
	bool usingSpirv14 = false;
	bool usingSubgroupSizeControl = false;
	bool usingVulkan12 = false;
	bool usingVulkan13 = false;
	bool usingVulkan14 = false;

	/*
	 * Examine the properties and features of each physical device, and give each a score.
	 * A greater score means a better physical device (for our purposes).
	 * Find the physical device with the highest score and use that as the sole logical device.
	 */
	for (uint32_t i = 0; i < deviceCount; i++) {
		uint32_t extCount = extCounts[i];
		uint32_t qfCount = qfCounts[i];
		uint32_t memoryTypeCount = devsMemoryProps[i].memoryProperties.memoryTypeCount;

		bool hasVulkan11 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasVulkan12 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_2;
		bool hasVulkan13 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_3;
		bool hasVulkan14 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_4;

		bool hasShaderInt16 = devsFeats[i].features.shaderInt16;
		bool hasShaderInt64 = devsFeats[i].features.shaderInt64;

		bool hasStorageBuffer16BitAccess = devs16BitStorageFeats[i].storageBuffer16BitAccess;

		bool hasDedicatedTransfer = false;
		bool hasDedicatedCompute = false;
		bool hasCompute = false;

		for (uint32_t j = 0; j < qfCount; j++) {
			VkQueueFlags queueFlags = qfsProps[i][j].queueFamilyProperties.queueFlags;

			bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
			bool isDedicatedCompute = queueFlags == VK_QUEUE_COMPUTE_BIT;
			bool isCompute = queueFlags & VK_QUEUE_COMPUTE_BIT;

			if (isDedicatedTransfer) { hasDedicatedTransfer = true; }
			if (isDedicatedCompute)  { hasDedicatedCompute = true; }
			if (isCompute)           { hasCompute = true; }
		}

		bool hasDeviceNonHost = false;
		bool hasDeviceLocal = false;

		bool hasHostCachedNonCoherent = false;
		bool hasHostCached = false;
		bool hasHostNonCoherent = false;
		bool hasHostVisible = false;

		for (uint32_t j = 0; j < memoryTypeCount; j++) {
			VkMemoryPropertyFlags propFlags = devsMemoryProps[i].memoryProperties.memoryTypes[j].propertyFlags;

			bool isDeviceLocal = propFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			bool isHostVisible = propFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			bool isHostCoherent = propFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			bool isHostCached = propFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			bool isLazilyAllocated = propFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
			bool isProtected = propFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT;
			bool isDeviceCoherent = propFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD;

			if (isLazilyAllocated) { continue; }
			if (isProtected)       { continue; }
			if (isDeviceCoherent)  { continue; }

			if (isDeviceLocal) {
				hasDeviceLocal = true;
				if (!isHostVisible) { hasDeviceNonHost = true; }
			}

			if (isHostVisible) {
				hasHostVisible = true;
				if (isHostCached && !isHostCoherent) { hasHostCachedNonCoherent = true; }
				if (isHostCached)                    { hasHostCached = true; }
				if (!isHostCoherent)                 { hasHostNonCoherent = true; }
			}
		}

		bool hasMaintenance4 = false;
		bool hasMaintenance5 = false;
		bool hasMaintenance6 = false;
		bool hasMaintenance7 = false;
		bool hasMaintenance8 = false;
		bool hasMaintenance9 = false;
		bool hasPipelineExecutableProperties = false;
		bool hasPortabilitySubset = false;
		bool hasSpirv14 = false;
		bool hasSynchronization2 = false;
		bool hasTimelineSemaphore = false;
		bool hasMemoryBudget = false;
		bool hasMemoryPriority = false;
		bool hasPipelineCreationCacheControl = false;
		bool hasSubgroupSizeControl = false;

		for (uint32_t j = 0; j < extCount; j++) {
			const char* extName = extsProps[i][j].extensionName;

			if (!strcmp(extName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))      { hasMaintenance4 = true; }
			else if (!strcmp(extName, VK_KHR_MAINTENANCE_5_EXTENSION_NAME)) { hasMaintenance5 = true; }
			else if (!strcmp(extName, VK_KHR_MAINTENANCE_6_EXTENSION_NAME)) { hasMaintenance6 = true; }
			else if (!strcmp(extName, VK_KHR_MAINTENANCE_7_EXTENSION_NAME)) { hasMaintenance7 = true; }
			else if (!strcmp(extName, VK_KHR_MAINTENANCE_8_EXTENSION_NAME)) { hasMaintenance8 = true; }
			else if (!strcmp(extName, VK_KHR_MAINTENANCE_9_EXTENSION_NAME)) { hasMaintenance9 = true; }
			else if (!strcmp(extName, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME)) {
				hasPipelineExecutableProperties = true; }
			else if (!strcmp(extName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) { hasPortabilitySubset = true; }
			else if (!strcmp(extName, VK_KHR_SPIRV_1_4_EXTENSION_NAME))          { hasSpirv14 = true; }
			else if (!strcmp(extName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))  { hasSynchronization2 = true; }
			else if (!strcmp(extName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) { hasTimelineSemaphore = true; }
			else if (!strcmp(extName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))      { hasMemoryBudget = true; }
			else if (!strcmp(extName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))    { hasMemoryPriority = true; }
			else if (!strcmp(extName, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME)) {
				hasPipelineCreationCacheControl = true; }
			else if (!strcmp(extName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) { hasSubgroupSizeControl = true; }
		}

		uint32_t currScore = 1;

		if (!hasVulkan11) { continue; }

		if (!hasDeviceLocal) { continue; }
		if (!hasHostVisible) { continue; }

		if (!hasCompute) { continue; }

		if (!hasSynchronization2)  { continue; }
		if (!hasTimelineSemaphore) { continue; }

		if (hasVulkan12) { currScore += 50; }
		if (hasVulkan13) { currScore += 50; }
		if (hasVulkan14) { currScore += 50; }

		if (g_config.preferInt16 && hasShaderInt16) { currScore += 1000; }
		if (g_config.preferInt64 && hasShaderInt64) { currScore += 1000; }

		if (hasDeviceNonHost) { currScore += 50; }

		if (hasHostCachedNonCoherent) { currScore += 1000; }
		else if (hasHostCached)       { currScore += 500; }
		else if (hasHostNonCoherent)  { currScore += 100; }

		if (hasDedicatedTransfer) { currScore += 100; }
		if (hasDedicatedCompute)  { currScore += 100; }

		if (hasMaintenance4)                 { currScore += 10; }
		if (hasMaintenance5)                 { currScore += 10; }
		if (hasMaintenance6)                 { currScore += 10; }
		if (hasMaintenance7)                 { currScore += 10; }
		if (hasMaintenance8)                 { currScore += 10; }
		if (hasMaintenance9)                 { currScore += 10; }
		if (hasMemoryBudget)                 { currScore += 10; }
		if (hasMemoryPriority)               { currScore += 10; }
		if (hasPipelineCreationCacheControl) { currScore += 10; }
		if (hasSpirv14)                      { currScore += 10; }
		if (hasStorageBuffer16BitAccess)     { currScore += 100; }
		if (hasSubgroupSizeControl)          { currScore += 10; }

		if (g_config.capturePipelines && hasPipelineExecutableProperties) { currScore += 10; }

		if (currScore > bestScore) {
			bestScore = currScore;
			deviceIndex = i;

			using16BitStorage = hasStorageBuffer16BitAccess;
			usingMaintenance4 = hasMaintenance4;
			usingMaintenance5 = hasMaintenance5;
			usingMaintenance6 = hasMaintenance6;
			usingMaintenance7 = hasMaintenance7;
			usingMaintenance8 = hasMaintenance8;
			usingMaintenance9 = hasMaintenance9;
			usingMemoryBudget = hasMemoryBudget;
			usingMemoryPriority = hasMemoryPriority;
			usingPipelineCreationCacheControl = hasPipelineCreationCacheControl;
			usingPipelineExecutableProperties = g_config.capturePipelines && hasPipelineExecutableProperties;
			usingPortabilitySubset = hasPortabilitySubset;
			usingShaderInt16 = g_config.preferInt16 && hasShaderInt16;
			usingShaderInt64 = g_config.preferInt64 && hasShaderInt64;
			usingSpirv14 = hasSpirv14;
			usingSubgroupSizeControl = hasSubgroupSizeControl;
			usingVulkan12 = hasVulkan12;
			usingVulkan13 = hasVulkan13;
			usingVulkan14 = hasVulkan14;
		}
	}

	if (deviceIndex == UINT32_MAX) {
		log_critical(
			stderr,
			"No physical device meets program requirements; "
			"see device_requirements.md for comprehensive physical device requirements");

		free_recursive(dyMem);
		return false;
	}

	const char* deviceName = devsProps[deviceIndex].properties.deviceName;
	uint32_t qfCount = qfCounts[deviceIndex];

	uint32_t vkVerMajor = 1;
	uint32_t vkVerMinor = 1;
	uint32_t spvVerMajor = 1;
	uint32_t spvVerMinor = 3;

	if (usingVulkan14) {
		vkVerMinor = 4;
		spvVerMinor = 6;
	}
	else if (usingVulkan13) {
		vkVerMinor = 3;
		spvVerMinor = 6;
	}
	else if (usingVulkan12) {
		vkVerMinor = 2;
		spvVerMinor = 5;
	}
	else if (usingSpirv14) {
		spvVerMinor = 4;
	}

	uint32_t transferQfIndex = 0;
	uint32_t computeQfIndex = 0;

	bool hasDedicatedTransfer = false;
	bool hasDedicatedCompute = false;
	bool hasTransfer = false;
	bool hasCompute = false;

	for (uint32_t i = 0; i < qfCount; i++) {
		VkQueueFlags queueFlags = qfsProps[deviceIndex][i].queueFamilyProperties.queueFlags;

		bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
		bool isDedicatedCompute = queueFlags == VK_QUEUE_COMPUTE_BIT;
		bool isTransfer = queueFlags & VK_QUEUE_TRANSFER_BIT;
		bool isCompute = queueFlags & VK_QUEUE_COMPUTE_BIT;

		if (isTransfer) {
			if (isDedicatedTransfer && !hasDedicatedTransfer) {
				transferQfIndex = i;
				hasDedicatedTransfer = true;
				hasTransfer = true;
			}
			else if (!hasTransfer) {
				transferQfIndex = i;
				hasTransfer = true;
			}
		}

		if (isCompute) {
			if (isDedicatedCompute && !hasDedicatedCompute) {
				computeQfIndex = i;
				hasDedicatedCompute = true;
				hasCompute = true;
			}
			else if (!hasCompute) {
				computeQfIndex = i;
				hasCompute = true;
			}
		}
	}

	if (!hasTransfer) {
		transferQfIndex = computeQfIndex;
	}

	gpu->physicalDevice = devices[deviceIndex];

	gpu->transferQueueFamilyIndex = transferQfIndex;
	gpu->computeQueueFamilyIndex = computeQfIndex;

	gpu->vkVerMajor = vkVerMajor;
	gpu->vkVerMinor = vkVerMinor;
	gpu->spvVerMajor = spvVerMajor;
	gpu->spvVerMinor = spvVerMinor;

	gpu->using16BitStorage = using16BitStorage;
	gpu->usingMaintenance4 = usingMaintenance4;
	gpu->usingMaintenance5 = usingMaintenance5;
	gpu->usingMaintenance6 = usingMaintenance6;
	gpu->usingMaintenance7 = usingMaintenance7;
	gpu->usingMaintenance8 = usingMaintenance8;
	gpu->usingMaintenance9 = usingMaintenance9;
	gpu->usingMemoryBudget = usingMemoryBudget;
	gpu->usingMemoryPriority = usingMemoryPriority;
	gpu->usingPipelineCreationCacheControl = usingPipelineCreationCacheControl;
	gpu->usingPipelineExecutableProperties = usingPipelineExecutableProperties;
	gpu->usingPortabilitySubset = usingPortabilitySubset;
	gpu->usingShaderInt16 = usingShaderInt16;
	gpu->usingShaderInt64 = usingShaderInt64;
	gpu->usingSubgroupSizeControl = usingSubgroupSizeControl;

	if (g_config.queryBenchmarks) {
		gpu->transferQueueFamilyTimestampValidBits =
			qfsProps[deviceIndex][transferQfIndex].queueFamilyProperties.timestampValidBits;
		gpu->computeQueueFamilyTimestampValidBits =
			qfsProps[deviceIndex][computeQfIndex].queueFamilyProperties.timestampValidBits;
		gpu->timestampPeriod = devsProps[deviceIndex].properties.limits.timestampPeriod;
	}

	switch (g_config.outputLevel) {
		case OUTPUT_LEVEL_DEFAULT:
			printf(
				"Device: %s\n"
				"\tVulkan version:    %" PRIu32 ".%" PRIu32 "\n"
				"\tSPIR-V version:    %" PRIu32 ".%" PRIu32 "\n"
				"\tTransfer QF index: %" PRIu32 "\n"
				"\tCompute QF index:  %" PRIu32 "\n",
				deviceName, vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor, transferQfIndex, computeQfIndex);

			if (g_config.preferInt16) {
				printf("\tshaderInt16:       %d\n", usingShaderInt16);
			}
			if (g_config.preferInt64) {
				printf("\tshaderInt64:       %d\n", usingShaderInt64);
			}

			NEWLINE();
			break;

		case OUTPUT_LEVEL_VERBOSE:
			printf(
				"Device: %s\n"
				"\tScore:                             %" PRIu32 "\n"
				"\tVulkan version:                    %" PRIu32 ".%" PRIu32 "\n"
				"\tSPIR-V version:                    %" PRIu32 ".%" PRIu32 "\n"
				"\tTransfer QF index:                 %" PRIu32 "\n"
				"\tCompute QF index:                  %" PRIu32 "\n"
				"\tmaintenance4                       %d\n"
				"\tmaintenance5                       %d\n"
				"\tmaintenance6                       %d\n"
				"\tmaintenance7                       %d\n"
				"\tmaintenance8                       %d\n"
				"\tmaintenance9                       %d\n"
				"\tmemoryPriority:                    %d\n"
				"\tpipelineCreationCacheControl:      %d\n"
				"\tpipelineExecutableProperties:      %d\n"
				"\tshaderInt16:                       %d\n"
				"\tshaderInt64:                       %d\n"
				"\tstorageBuffer16BitAccess:          %d\n"
				"\tsubgroupSizeControl:               %d\n\n",
				deviceName, bestScore, vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor, transferQfIndex,
				computeQfIndex, usingMaintenance4, usingMaintenance5, usingMaintenance6, usingMaintenance7,
				usingMaintenance8, usingMaintenance9, usingMemoryPriority, usingPipelineCreationCacheControl,
				usingPipelineExecutableProperties, usingShaderInt16, usingShaderInt64, using16BitStorage,
				usingSubgroupSizeControl);

			break;

		default:
			break;
	}

	free_recursive(dyMem);
	return true;
}

bool create_device(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;

	uint32_t transferQfIndex = gpu->transferQueueFamilyIndex;
	uint32_t computeQfIndex = gpu->computeQueueFamilyIndex;

	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 1;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	elmSize = sizeof(const char*);
	elmCount = 19;

	DyArray enabledExts = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!enabledExts) { free_recursive(dyMem); return false; }

	dyData.data = enabledExts;
	dyData.free = dyarray_destroy_stub;

	dyarray_append(dyMem, &dyData);

	// Required extensions
	dyarray_append_str(enabledExts, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	dyarray_append_str(enabledExts, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

	// Optional KHR extensions
	if (gpu->usingMaintenance4) {
		dyarray_append_str(enabledExts, VK_KHR_MAINTENANCE_4_EXTENSION_NAME);
	}
	if (gpu->usingMaintenance5) {
		dyarray_append_str(enabledExts, VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME);
		dyarray_append_str(enabledExts, VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME);
		dyarray_append_str(enabledExts, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME);
		dyarray_append_str(enabledExts, VK_KHR_MAINTENANCE_5_EXTENSION_NAME);
	}
	if (gpu->usingMaintenance6) {
		dyarray_append_str(enabledExts, VK_KHR_MAINTENANCE_6_EXTENSION_NAME);
	}
	if (gpu->usingMaintenance7) {
		dyarray_append_str(enabledExts, VK_KHR_MAINTENANCE_7_EXTENSION_NAME);
	}
	if (gpu->usingMaintenance8) {
		dyarray_append_str(enabledExts, VK_KHR_MAINTENANCE_8_EXTENSION_NAME);
	}
	if (gpu->usingMaintenance9) {
		dyarray_append_str(enabledExts, VK_KHR_MAINTENANCE_9_EXTENSION_NAME);
	}
	if (gpu->usingPipelineExecutableProperties) {
		dyarray_append_str(enabledExts, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
	}
	if (gpu->usingPortabilitySubset) {
		dyarray_append_str(enabledExts, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);
	}
	if (spvVerMinor >= 4) {
		dyarray_append_str(enabledExts, VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		dyarray_append_str(enabledExts, VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	}
	// Optional EXT extensions
	if (gpu->usingMemoryBudget) {
		dyarray_append_str(enabledExts, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);
	}
	if (gpu->usingMemoryPriority) {
		dyarray_append_str(enabledExts, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);
	}
	if (gpu->usingPipelineCreationCacheControl) {
		dyarray_append_str(enabledExts, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME);
	}
	if (gpu->usingSubgroupSizeControl) {
		dyarray_append_str(enabledExts, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME);
	}

	VkPhysicalDeviceFeatures2 devFeats = {0};
	devFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	devFeats.features.shaderInt64 = gpu->usingShaderInt64;
	devFeats.features.shaderInt16 = gpu->usingShaderInt16;

	VkPhysicalDevice16BitStorageFeatures dev16BitStorageFeats = {0};
	dev16BitStorageFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
	dev16BitStorageFeats.storageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeatures devDynamicRenderingFeats = {0};
	devDynamicRenderingFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	devDynamicRenderingFeats.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceMaintenance4Features devMaintenance4Feats = {0};
	devMaintenance4Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
	devMaintenance4Feats.maintenance4 = VK_TRUE;

	VkPhysicalDeviceMaintenance5Features devMaintenance5Feats = {0};
	devMaintenance5Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
	devMaintenance5Feats.maintenance5 = VK_TRUE;

	VkPhysicalDeviceMaintenance6Features devMaintenance6Feats = {0};
	devMaintenance6Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES;
	devMaintenance6Feats.maintenance6 = VK_TRUE;

	VkPhysicalDeviceMaintenance7FeaturesKHR devMaintenance7Feats = {0};
	devMaintenance7Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR;
	devMaintenance7Feats.maintenance7 = VK_TRUE;

	VkPhysicalDeviceMaintenance8FeaturesKHR devMaintenance8Feats = {0};
	devMaintenance8Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_8_FEATURES_KHR;
	devMaintenance8Feats.maintenance8 = VK_TRUE;

	VkPhysicalDeviceMaintenance9FeaturesKHR devMaintenance9Feats = {0};
	devMaintenance9Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR;
	devMaintenance9Feats.maintenance9 = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT devMemoryPriorityFeats = {0};
	devMemoryPriorityFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
	devMemoryPriorityFeats.memoryPriority = VK_TRUE;

	VkPhysicalDevicePipelineCreationCacheControlFeatures devPipelineCreationCacheControlFeats = {0};
	devPipelineCreationCacheControlFeats.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES;
	devPipelineCreationCacheControlFeats.pipelineCreationCacheControl = VK_TRUE;

	VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR devPipelineExecutablePropertiesFeats = {0};
	devPipelineExecutablePropertiesFeats.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
	devPipelineExecutablePropertiesFeats.pipelineExecutableInfo = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeatures devSubgroupSizeControlFeats = {0};
	devSubgroupSizeControlFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
	devSubgroupSizeControlFeats.subgroupSizeControl = VK_TRUE;

	VkPhysicalDeviceSynchronization2Features devSynchronization2Feats = {0};
	devSynchronization2Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	devSynchronization2Feats.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeatures devTimelineSemaphoreFeats = {0};
	devTimelineSemaphoreFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	devTimelineSemaphoreFeats.timelineSemaphore = VK_TRUE;

	void** next = &devFeats.pNext;

	PNEXT_ADD(next, devSynchronization2Feats);
	PNEXT_ADD(next, devTimelineSemaphoreFeats);

	if (gpu->using16BitStorage) {
		PNEXT_ADD(next, dev16BitStorageFeats);
	}
	if (gpu->usingMaintenance4) {
		PNEXT_ADD(next, devMaintenance4Feats);
	}
	if (gpu->usingMaintenance5) {
		PNEXT_ADD(next, devDynamicRenderingFeats);
		PNEXT_ADD(next, devMaintenance5Feats);
	}
	if (gpu->usingMaintenance6) {
		PNEXT_ADD(next, devMaintenance6Feats);
	}
	if (gpu->usingMaintenance7) {
		PNEXT_ADD(next, devMaintenance7Feats);
	}
	if (gpu->usingMaintenance8) {
		PNEXT_ADD(next, devMaintenance8Feats);
	}
	if (gpu->usingMaintenance9) {
		PNEXT_ADD(next, devMaintenance9Feats);
	}
	if (gpu->usingMemoryPriority) {
		PNEXT_ADD(next, devMemoryPriorityFeats);
	}
	if (gpu->usingPipelineCreationCacheControl) {
		PNEXT_ADD(next, devPipelineCreationCacheControlFeats);
	}
	if (gpu->usingPipelineExecutableProperties) {
		PNEXT_ADD(next, devPipelineExecutablePropertiesFeats);
	}
	if (gpu->usingSubgroupSizeControl) {
		PNEXT_ADD(next, devSubgroupSizeControlFeats);
	}

	float computeQueuePriority = 1;
	float transferQueuePriority = 0;

	VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].queueFamilyIndex = computeQfIndex;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &computeQueuePriority;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = transferQfIndex;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = &transferQueuePriority;

	uint32_t enabledExtCount = (uint32_t) dyarray_size(enabledExts);
	const char** enabledExtNames = dyarray_raw(enabledExts);

	VkDeviceCreateInfo deviceCreateInfo = {0};
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &devFeats;
	deviceCreateInfo.queueCreateInfoCount = computeQfIndex == transferQfIndex ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount = enabledExtCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtNames;

	if (g_config.outputLevel > OUTPUT_LEVEL_DEFAULT) {
		printf("Enabled device extensions (%" PRIu32 "):\n", enabledExtCount);
		for (uint32_t i = 0; i < enabledExtCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtNames[i]);
		}
		NEWLINE();
	}

	VkDevice device;
	VK_CALL_RES(vkCreateDevice, physicalDevice, &deviceCreateInfo, g_allocator, &device);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	gpu->device = device;

	volkLoadDevice(device);

	VkDeviceQueueInfo2 transferQueueInfo = {0};
	transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	transferQueueInfo.queueFamilyIndex = transferQfIndex;
	transferQueueInfo.queueIndex = 0;

	VK_CALL(vkGetDeviceQueue2, device, &transferQueueInfo, &gpu->transferQueue);

	VkDeviceQueueInfo2 computeQueueInfo = {0};
	computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	computeQueueInfo.queueFamilyIndex = computeQfIndex;
	computeQueueInfo.queueIndex = 0;

	VK_CALL(vkGetDeviceQueue2, device, &computeQueueInfo, &gpu->computeQueue);

	free_recursive(dyMem);
	return true;
}

bool manage_memory(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;
	VkDevice device = gpu->device;

	bool (*get_buffer_requirements)(VkDevice, VkDeviceSize, VkBufferUsageFlags, VkMemoryRequirements*) =
		gpu->usingMaintenance4 ? get_buffer_requirements_main4 : get_buffer_requirements_noext;

	VkPhysicalDeviceMaintenance4Properties devMaintenance4Props = {0};
	devMaintenance4Props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;

	VkPhysicalDeviceMaintenance3Properties devMaintenance3Props = {0};
	devMaintenance3Props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
	devMaintenance3Props.pNext = gpu->usingMaintenance4 ? &devMaintenance4Props : NULL;

	VkPhysicalDeviceProperties2 devProps = {0};
	devProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	devProps.pNext = &devMaintenance3Props;

	VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevice, &devProps);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT devMemoryBudgetProps = {0};
	devMemoryBudgetProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

	VkPhysicalDeviceMemoryProperties2 devMemoryProps = {0};
	devMemoryProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	devMemoryProps.pNext = gpu->usingMemoryBudget ? &devMemoryBudgetProps : NULL;

	VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevice, &devMemoryProps);

	VkDeviceSize maxMemorySize = devMaintenance3Props.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize = gpu->usingMaintenance4 ? devMaintenance4Props.maxBufferSize : maxMemorySize;

	uint32_t maxStorageBufferRange = devProps.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryCount = devProps.properties.limits.maxMemoryAllocationCount;
	uint32_t maxWorkgroupCount = devProps.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxWorkgroupSize = devProps.properties.limits.maxComputeWorkGroupSize[0];
	uint32_t memoryTypeCount = devMemoryProps.memoryProperties.memoryTypeCount;

	// HV = host visible
	VkBufferUsageFlags hvUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VkMemoryRequirements hvRequirements;

	bool bres = get_buffer_requirements(device, sizeof(char), hvUsage, &hvRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	// DL = device local
	VkBufferUsageFlags dlUsage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VkMemoryRequirements dlRequirements;

	bres = get_buffer_requirements(device, sizeof(char), dlUsage, &dlRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	uint32_t dlMemoryTypeBits = dlRequirements.memoryTypeBits;
	uint32_t hvMemoryTypeBits = hvRequirements.memoryTypeBits;

	uint32_t dlHeapIndex = 0;
	uint32_t hvHeapIndex = 0;
	uint32_t dlTypeIndex = 0;
	uint32_t hvTypeIndex = 0;

	bool hasDeviceNonHost = false;
	bool hasDeviceLocal = false;

	bool hasHostCachedNonCoherent = false;
	bool hasHostCached = false;
	bool hasHostNonCoherent = false;
	bool hasHostVisible = false;

	for (uint32_t i = 0; i < memoryTypeCount; i++) {
		uint32_t memoryTypeBit = UINT32_C(1) << i;
		uint32_t heapIndex = devMemoryProps.memoryProperties.memoryTypes[i].heapIndex;
		VkMemoryPropertyFlags propFlags = devMemoryProps.memoryProperties.memoryTypes[i].propertyFlags;

		bool isDeviceLocal = propFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bool isHostVisible = propFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		bool isHostCoherent = propFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bool isHostCached = propFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		bool isLazilyAllocated = propFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
		bool isProtected = propFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT;
		bool isDeviceCoherent = propFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD;

		if (isLazilyAllocated) { continue; }
		if (isProtected)       { continue; }
		if (isDeviceCoherent)  { continue; }

		if (isDeviceLocal && (dlMemoryTypeBits & memoryTypeBit)) {
			if (!isHostVisible && !hasDeviceNonHost) {
				dlHeapIndex = heapIndex;
				dlTypeIndex = i;
				hasDeviceNonHost = true;
				hasDeviceLocal = true;
			}
			else if (!hasDeviceLocal) {
				dlHeapIndex = heapIndex;
				dlTypeIndex = i;
				hasDeviceLocal = true;
			}
		}

		if (isHostVisible && (hvMemoryTypeBits & memoryTypeBit)) {
			if (isHostCached && !isHostCoherent && !hasHostCachedNonCoherent) {
				hvHeapIndex = heapIndex;
				hvTypeIndex = i;
				hasHostCachedNonCoherent = true;
				hasHostCached = true;
				hasHostNonCoherent = true;
				hasHostVisible = true;
			}
			else if (isHostCached && !hasHostCached) {
				hvHeapIndex = heapIndex;
				hvTypeIndex = i;
				hasHostCached = true;
				hasHostNonCoherent = false;
				hasHostVisible = true;
			}
			else if (!isHostCoherent && !hasHostCached && !hasHostNonCoherent) {
				hvHeapIndex = heapIndex;
				hvTypeIndex = i;
				hasHostNonCoherent = true;
				hasHostVisible = true;
			}
			else if (!hasHostVisible) {
				hvHeapIndex = heapIndex;
				hvTypeIndex = i;
				hasHostVisible = true;
			}
		}
	}

	VkDeviceSize hvHeapBudget = devMemoryBudgetProps.heapBudget[hvHeapIndex];
	VkDeviceSize dlHeapBudget = devMemoryBudgetProps.heapBudget[dlHeapIndex];

	VkDeviceSize hvHeapSize = devMemoryProps.memoryProperties.memoryHeaps[hvHeapIndex].size;
	VkDeviceSize dlHeapSize = devMemoryProps.memoryProperties.memoryHeaps[dlHeapIndex].size;

	VkDeviceSize bytesPerHvHeap = gpu->usingMemoryBudget ? hvHeapBudget : hvHeapSize;
	VkDeviceSize bytesPerDlHeap = gpu->usingMemoryBudget ? dlHeapBudget : dlHeapSize;

	VkDeviceSize bytesPerHeap = minu64(bytesPerHvHeap, bytesPerDlHeap);
	bytesPerHeap = (VkDeviceSize) ((float) bytesPerHeap * g_config.maxMemory); // User-given limit on heap memory

	if (dlHeapIndex == hvHeapIndex) {
		bytesPerHeap /= 2; // Evenly partition heap into HV memory and DL memory
	}

	VkDeviceSize bytesPerBuffer = minu64v(3, maxMemorySize, maxBufferSize, bytesPerHeap);
	uint32_t buffersPerHeap = (uint32_t) (bytesPerHeap / bytesPerBuffer);

	// Can we squeeze in another buffer?
	if (buffersPerHeap < maxMemoryCount && bytesPerHeap % bytesPerBuffer) {
		VkDeviceSize excessBytes = bytesPerBuffer - bytesPerHeap % bytesPerBuffer;

		buffersPerHeap++;
		bytesPerBuffer -= excessBytes / buffersPerHeap;

		if (excessBytes % buffersPerHeap) {
			bytesPerBuffer--;
		}
	}
	else if (buffersPerHeap > maxMemoryCount) {
		buffersPerHeap = maxMemoryCount; // Don't use too many allocations
	}

	uint32_t workgroupSize = floor_pow2(maxWorkgroupSize);
	uint32_t workgroupCount = minu32(
		maxWorkgroupCount, (uint32_t) (maxStorageBufferRange / (workgroupSize * sizeof(StartValue))));

	uint32_t valuesPerInout = workgroupSize * workgroupCount;
	VkDeviceSize bytesPerInout = valuesPerInout * (sizeof(StartValue) + sizeof(StopTime));
	uint32_t inoutsPerBuffer = (uint32_t) (bytesPerBuffer / bytesPerInout);

	// Can we squeeze in another inout-buffer?
	if (bytesPerBuffer % bytesPerInout > inoutsPerBuffer * workgroupSize * (sizeof(StartValue) + sizeof(StopTime))) {
		uint32_t excessValues =
			valuesPerInout - (uint32_t) (bytesPerBuffer % bytesPerInout / (sizeof(StartValue) + sizeof(StopTime)));

		inoutsPerBuffer++;
		valuesPerInout -= excessValues / inoutsPerBuffer;

		if (excessValues % inoutsPerBuffer) {
			valuesPerInout--;
		}

		valuesPerInout &= ~(workgroupSize - 1); // Round down to multiple of workgroupSize

		// Squeeze worked
		if (valuesPerInout) {
			workgroupCount = valuesPerInout / workgroupSize;
		}
		// Squeeze failed
		else {
			inoutsPerBuffer--;
			valuesPerInout = workgroupSize * workgroupCount;
		}
	}

	VkDeviceSize bytesPerIn = valuesPerInout * sizeof(StartValue);
	VkDeviceSize bytesPerOut = valuesPerInout * sizeof(StopTime);

	bytesPerInout = bytesPerIn + bytesPerOut;
	bytesPerBuffer = bytesPerInout * inoutsPerBuffer;

	uint32_t valuesPerBuffer = valuesPerInout * inoutsPerBuffer;
	uint32_t valuesPerHeap = valuesPerBuffer * buffersPerHeap;
	uint32_t inoutsPerHeap = inoutsPerBuffer * buffersPerHeap;

	bres = get_buffer_requirements(device, bytesPerBuffer, hvUsage, &hvRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	bres = get_buffer_requirements(device, bytesPerBuffer, dlUsage, &dlRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	VkDeviceSize bytesPerHvMemory = hvRequirements.size;
	VkDeviceSize bytesPerDlMemory = dlRequirements.size;

	/*
	 * maxWorkgroupSize is guaranteed to be at least 128
	 * workgroupSize is maxComputeWorkGroupSize rounded down to a power of 2
	 * => workgroupSize is a multiple of 128
	 *
	 * valuesPerInout is a multiple of workgroupSize
	 * => valuesPerInout is a multiple of 128
	 *
	 * bytesPerIn and bytesPerOut are multiples of valuesPerInout * 2
	 * => bytesPerIn and bytesPerOut are multiples of 256
	 *
	 * nonCoherentAtomSize and minStorageBufferOffsetAlignment are guaranteed to be at most 256
	 * => bytesPerIn and bytesPerOut are multiples of nonCoherentAtomSize and minStorageBufferOffsetAlignment
	 */

	gpu->bytesPerIn = bytesPerIn;
	gpu->bytesPerOut = bytesPerOut;
	gpu->bytesPerInout = bytesPerInout;
	gpu->bytesPerBuffer = bytesPerBuffer;
	gpu->bytesPerHostVisibleMemory = bytesPerHvMemory;
	gpu->bytesPerDeviceLocalMemory = bytesPerDlMemory;

	gpu->valuesPerInout = valuesPerInout;
	gpu->valuesPerBuffer = valuesPerBuffer;
	gpu->valuesPerHeap = valuesPerHeap;
	gpu->inoutsPerBuffer = inoutsPerBuffer;
	gpu->inoutsPerHeap = inoutsPerHeap;
	gpu->buffersPerHeap = buffersPerHeap;

	gpu->workgroupSize = workgroupSize;
	gpu->workgroupCount = workgroupCount;

	gpu->hostVisibleHeapIndex = hvHeapIndex;
	gpu->deviceLocalHeapIndex = dlHeapIndex;
	gpu->hostVisibleTypeIndex = hvTypeIndex;
	gpu->deviceLocalTypeIndex = dlTypeIndex;

	gpu->hostNonCoherent = hasHostNonCoherent;

	switch (g_config.outputLevel) {
		case OUTPUT_LEVEL_DEFAULT:
			printf(
				"Memory information:\n"
				"\tHV memory type index:    %" PRIu32 "\n"
				"\tDL memory type index:    %" PRIu32 "\n"
				"\tWorkgroup size:          %" PRIu32 "\n"
				"\tWorkgroup count:         %" PRIu32 "\n"
				"\tValues per inout-buffer: %" PRIu32 "\n"
				"\tInout-buffers per heap:  %" PRIu32 "\n\n",
				hvTypeIndex, dlTypeIndex, workgroupSize, workgroupCount, valuesPerInout, inoutsPerHeap);

			break;

		case OUTPUT_LEVEL_VERBOSE:
			printf(
				"Memory information:\n"
				"\tHV non-coherent memory:   %d\n"
				"\tHV memory heap index:     %" PRIu32 "\n"
				"\tDL memory heap index:     %" PRIu32 "\n"
				"\tHV memory type index:     %" PRIu32 "\n"
				"\tDL memory type index:     %" PRIu32 "\n"
				"\tWorkgroup size:           %" PRIu32 "\n"
				"\tWorkgroup count:          %" PRIu32 "\n"
				"\tValues per inout-buffer:  %" PRIu32 "\n"
				"\tInout-buffers per buffer: %" PRIu32 "\n"
				"\tBuffers per heap:         %" PRIu32 "\n"
				"\tValues per heap:          %" PRIu32 "\n\n",
				hasHostNonCoherent, hvHeapIndex, dlHeapIndex, hvTypeIndex, dlTypeIndex,
				workgroupSize, workgroupCount, valuesPerInout, inoutsPerBuffer, buffersPerHeap, valuesPerHeap);

			break;

		default:
			break;
	}

	size_t allocCount = 1;
	size_t allocSize =
		inoutsPerHeap * sizeof(StartValue*) +
		inoutsPerHeap * sizeof(StopTime*) +
		buffersPerHeap * sizeof(VkBuffer) * 2 +
		buffersPerHeap * sizeof(VkDeviceMemory) * 2 +
		inoutsPerHeap * sizeof(VkDescriptorSet) +
		inoutsPerHeap * sizeof(VkCommandBuffer) * 2 +
		inoutsPerHeap * sizeof(VkSemaphore);

	gpu->dynamicMemory = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!gpu->dynamicMemory) { CALLOC_FAILURE(gpu->dynamicMemory, allocCount, allocSize); return false; }

	gpu->mappedInBuffers = gpu->dynamicMemory;
	gpu->mappedOutBuffers = (StopTime**) (gpu->mappedInBuffers + inoutsPerHeap);

	gpu->hostVisibleBuffers = (VkBuffer*) (gpu->mappedOutBuffers + inoutsPerHeap);
	gpu->deviceLocalBuffers = (VkBuffer*) (gpu->hostVisibleBuffers + buffersPerHeap);

	gpu->hostVisibleDeviceMemories = (VkDeviceMemory*) (gpu->deviceLocalBuffers + buffersPerHeap);
	gpu->deviceLocalDeviceMemories = (VkDeviceMemory*) (gpu->hostVisibleDeviceMemories + buffersPerHeap);

	gpu->descriptorSets = (VkDescriptorSet*) (gpu->deviceLocalDeviceMemories + buffersPerHeap);

	gpu->transferCommandBuffers = (VkCommandBuffer*) (gpu->descriptorSets + inoutsPerHeap);
	gpu->computeCommandBuffers = (VkCommandBuffer*) (gpu->transferCommandBuffers + inoutsPerHeap);

	gpu->semaphores = (VkSemaphore*) (gpu->computeCommandBuffers + inoutsPerHeap);

	return true;
}

bool create_buffers(Gpu* restrict gpu)
{
	VkBuffer* hvBuffers = gpu->hostVisibleBuffers;
	VkBuffer* dlBuffers = gpu->deviceLocalBuffers;
	VkDeviceMemory* hvMemories = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* dlMemories = gpu->deviceLocalDeviceMemories;
	StartValue** mappedInBuffers = gpu->mappedInBuffers;
	StopTime** mappedOutBuffers = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerInout = gpu->bytesPerInout;
	VkDeviceSize bytesPerBuffer = gpu->bytesPerBuffer;
	VkDeviceSize bytesPerHvMemory = gpu->bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDlMemory = gpu->bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout = gpu->valuesPerInout;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;
	uint32_t hvTypeIndex = gpu->hostVisibleTypeIndex;
	uint32_t dlTypeIndex = gpu->deviceLocalTypeIndex;

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 1;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	size_t allocCount = 1;
	size_t allocSize =
		buffersPerHeap * sizeof(VkMemoryAllocateInfo) * 2 +
		buffersPerHeap * sizeof(VkMemoryDedicatedAllocateInfo) * 2 +
		buffersPerHeap * sizeof(VkBindBufferMemoryInfo) * 2;

	void* p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	VkMemoryAllocateInfo* hvAllocateInfos = p;
	VkMemoryAllocateInfo* dlAllocateInfos = (VkMemoryAllocateInfo*) (hvAllocateInfos + buffersPerHeap);

	VkMemoryDedicatedAllocateInfo* hvDedicatedInfos = (VkMemoryDedicatedAllocateInfo*) (
		dlAllocateInfos + buffersPerHeap);
	VkMemoryDedicatedAllocateInfo* dlDedicatedInfos = (VkMemoryDedicatedAllocateInfo*) (
		hvDedicatedInfos + buffersPerHeap);

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) (
		dlDedicatedInfos + buffersPerHeap);

	VkBufferCreateInfo hvBufferCreateInfo = {0};
	hvBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hvBufferCreateInfo.size = bytesPerBuffer;
	hvBufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hvBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo dlBufferCreateInfo = {0};
	dlBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	dlBufferCreateInfo.size = bytesPerBuffer;
	dlBufferCreateInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	dlBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkBuffer hvBuffer;
		VK_CALL_RES(vkCreateBuffer, device, &hvBufferCreateInfo, g_allocator, &hvBuffer);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		hvBuffers[i] = hvBuffer;

		VkBuffer dlBuffer;
		VK_CALL_RES(vkCreateBuffer, device, &dlBufferCreateInfo, g_allocator, &dlBuffer);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		dlBuffers[i] = dlBuffer;
	}

	VkMemoryPriorityAllocateInfoEXT hvPriorityInfo = {0};
	hvPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	hvPriorityInfo.priority = 0;

	VkMemoryPriorityAllocateInfoEXT dlPriorityInfo = {0};
	dlPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	dlPriorityInfo.priority = 1;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		hvDedicatedInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		hvDedicatedInfos[i].pNext = gpu->usingMemoryPriority ? &hvPriorityInfo : NULL;
		hvDedicatedInfos[i].buffer = hvBuffers[i];

		dlDedicatedInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		dlDedicatedInfos[i].pNext = gpu->usingMemoryPriority ? &dlPriorityInfo : NULL;
		dlDedicatedInfos[i].buffer = dlBuffers[i];

		hvAllocateInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		hvAllocateInfos[i].pNext = &hvDedicatedInfos[i];
		hvAllocateInfos[i].allocationSize = bytesPerHvMemory;
		hvAllocateInfos[i].memoryTypeIndex = hvTypeIndex;

		dlAllocateInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		dlAllocateInfos[i].pNext = &dlDedicatedInfos[i];
		dlAllocateInfos[i].allocationSize = bytesPerDlMemory;
		dlAllocateInfos[i].memoryTypeIndex = dlTypeIndex;
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkDeviceMemory hvMemory;
		VK_CALL_RES(vkAllocateMemory, device, &hvAllocateInfos[i], g_allocator, &hvMemory);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		hvMemories[i] = hvMemory;

		VkDeviceMemory dlMemory;
		VK_CALL_RES(vkAllocateMemory, device, &dlAllocateInfos[i], g_allocator, &dlMemory);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		dlMemories[i] = dlMemory;
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		bindBufferMemoryInfos[i][0].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
		bindBufferMemoryInfos[i][0].buffer = hvBuffers[i];
		bindBufferMemoryInfos[i][0].memory = hvMemories[i];

		bindBufferMemoryInfos[i][1].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
		bindBufferMemoryInfos[i][1].buffer = dlBuffers[i];
		bindBufferMemoryInfos[i][1].memory = dlMemories[i];
	}

	uint32_t bindInfoCount = buffersPerHeap * ARR_SIZE(bindBufferMemoryInfos[0]);
	VK_CALL_RES(vkBindBufferMemory2, device, bindInfoCount, (VkBindBufferMemoryInfo*) bindBufferMemoryInfos);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkDeviceSize offset = 0;
		VkMemoryMapFlags flags = 0;
		void* mappedMemory;

		VK_CALL_RES(vkMapMemory, device, hvMemories[i], offset, bytesPerHvMemory, flags, &mappedMemory);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		mappedInBuffers[j] = mappedMemory;
		mappedOutBuffers[j] = (StopTime*) (mappedInBuffers[j] + valuesPerInout);
		j++;

		for (uint32_t k = 1; k < inoutsPerBuffer; j++, k++) {
			mappedInBuffers[j] = mappedInBuffers[j - 1] + bytesPerInout / sizeof(StartValue);
			mappedOutBuffers[j] = mappedOutBuffers[j - 1] + bytesPerInout / sizeof(StopTime);
		}
	}

#ifndef NDEBUG
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		char objectName[37];
		sprintf(objectName, "Host visible (%" PRIu32 "/%" PRIu32 ")", i + 1, buffersPerHeap);

		set_debug_name(device, VK_OBJECT_TYPE_BUFFER, (uint64_t) hvBuffers[i], objectName);
		set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) hvMemories[i], objectName);

		strcpy(objectName, "Device local");
		objectName[12] = ' '; // Remove '\0' from strcpy

		set_debug_name(device, VK_OBJECT_TYPE_BUFFER, (uint64_t) dlBuffers[i], objectName);
		set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) dlMemories[i], objectName);
	}
#endif

	free_recursive(dyMem);
	return true;
}

bool create_descriptors(Gpu* restrict gpu)
{
	const VkBuffer* dlBuffers = gpu->deviceLocalBuffers;
	VkDescriptorSet* descSets = gpu->descriptorSets;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 1;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	size_t allocCount = 1;
	size_t allocSize =
		inoutsPerHeap * sizeof(VkDescriptorSetLayout) +
		inoutsPerHeap * sizeof(VkWriteDescriptorSet) +
		inoutsPerHeap * sizeof(VkDescriptorBufferInfo) * 2;

	void* p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	VkDescriptorSetLayout* descSetLayouts = p;
	VkWriteDescriptorSet* writeDescSets = (VkWriteDescriptorSet*) (descSetLayouts + inoutsPerHeap);

	VkDescriptorBufferInfo (*descBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (writeDescSets + inoutsPerHeap);

	VkDescriptorSetLayoutBinding descSetLayoutBindings[2] = {0};
	descSetLayoutBindings[0].binding = 0;
	descSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descSetLayoutBindings[0].descriptorCount = 1;
	descSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	descSetLayoutBindings[1].binding = 1;
	descSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descSetLayoutBindings[1].descriptorCount = 1;
	descSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = {0};
	descSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descSetLayoutCreateInfo.bindingCount = ARR_SIZE(descSetLayoutBindings);
	descSetLayoutCreateInfo.pBindings = descSetLayoutBindings;

	VkDescriptorSetLayout descSetLayout;
	VK_CALL_RES(vkCreateDescriptorSetLayout, device, &descSetLayoutCreateInfo, g_allocator, &descSetLayout);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	gpu->descriptorSetLayout = descSetLayout;

	VkDescriptorPoolSize descPoolSizes[1];
	descPoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descPoolSizes[0].descriptorCount = inoutsPerHeap * 2;

	VkDescriptorPoolCreateInfo descPoolCreateInfo = {0};
	descPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descPoolCreateInfo.maxSets = inoutsPerHeap;
	descPoolCreateInfo.poolSizeCount = ARR_SIZE(descPoolSizes);
	descPoolCreateInfo.pPoolSizes = descPoolSizes;

	VkDescriptorPool descPool;
	VK_CALL_RES(vkCreateDescriptorPool, device, &descPoolCreateInfo, g_allocator, &descPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	gpu->descriptorPool = descPool;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		descSetLayouts[i] = descSetLayout;
	}

	VkDescriptorSetAllocateInfo descSetAllocateInfo = {0};
	descSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descSetAllocateInfo.descriptorPool = descPool;
	descSetAllocateInfo.descriptorSetCount = inoutsPerHeap;
	descSetAllocateInfo.pSetLayouts = descSetLayouts;

	VK_CALL_RES(vkAllocateDescriptorSets, device, &descSetAllocateInfo, descSets);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			descBufferInfos[j][0].buffer = dlBuffers[i];
			descBufferInfos[j][0].offset = bytesPerInout * k;
			descBufferInfos[j][0].range = bytesPerIn;

			descBufferInfos[j][1].buffer = dlBuffers[i];
			descBufferInfos[j][1].offset = bytesPerInout * k + bytesPerIn;
			descBufferInfos[j][1].range = bytesPerOut;

			writeDescSets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescSets[j].dstSet = descSets[j];
			writeDescSets[j].dstBinding = 0;
			writeDescSets[j].dstArrayElement = 0;
			writeDescSets[j].descriptorCount = ARR_SIZE(descBufferInfos[j]);
			writeDescSets[j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescSets[j].pBufferInfo = descBufferInfos[j];
		}
	}

	uint32_t writeDescCount = inoutsPerHeap;
	uint32_t copyDescCount = 0;
	VkCopyDescriptorSet* copyDescSets = NULL;

	VK_CALL(vkUpdateDescriptorSets, device, writeDescCount, writeDescSets, copyDescCount, copyDescSets);

#ifndef NDEBUG
	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			char objectName[58];
			sprintf(
				objectName,
				"Inout %" PRIu32 "/%" PRIu32 ", Buffer %" PRIu32 "/%" PRIu32,
				k + 1, inoutsPerBuffer, i + 1, buffersPerHeap);

			set_debug_name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descSets[j], objectName);
		}
	}
#endif

	free_recursive(dyMem);
	return true;
}

bool create_pipeline(Gpu* restrict gpu)
{
	VkDevice device = gpu->device;

	VkDescriptorSetLayout descSetLayout = gpu->descriptorSetLayout;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t workgroupSize = gpu->workgroupSize;

	uint32_t transferQfTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;
	uint32_t computeQfTimestampValidBits = gpu->computeQueueFamilyTimestampValidBits;

	uint32_t spvVerMajor = gpu->spvVerMajor;
	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 1;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	char shaderName[52];
	sprintf(
		shaderName,
		"./v%" PRIu32 "%" PRIu32 "/spirv%s%s%s.spv",
		spvVerMajor, spvVerMinor,
		gpu->using16BitStorage ? "-sto16" : "",
		gpu->usingShaderInt16  ? "-int16" : "",
		gpu->usingShaderInt64  ? "-int64" : "");

	/*
	 * Vulkan guarantees the endianness of the CPU and GPU are the same.
	 * From the Vulkan spec (version 1.4.326, section 3.1):
	 * "The representation and endianness of [integers] on the host must match the representation and endianness of
	 * [integers] on every physical device supported."
	 */
	char entryPointName[37];
	Endianness endianness = get_endianness();

	sprintf(entryPointName, "main-%u-%lu", endianness, g_config.iterSize);

	if (g_config.outputLevel > OUTPUT_LEVEL_QUIET) {
		printf("Selected shader: %s\nSelected entry point: %s\n\n", shaderName, entryPointName);
	}

	size_t shaderSize;
	bool bres = file_size(shaderName, &shaderSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	size_t cacheSize;
	bres = file_size(PIPELINE_CACHE_NAME, &cacheSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	size_t allocSize = shaderSize + cacheSize;
	void* p = malloc(allocSize);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	uint32_t* shaderCode = p;
	void* cacheData = (char*) shaderCode + shaderSize;

	bres = read_file(shaderName, shaderCode, shaderSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	if (cacheSize) {
		bres = read_file(PIPELINE_CACHE_NAME, cacheData, cacheSize);
		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

	VkShaderModuleCreateInfo moduleCreateInfo = {0};
	moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	moduleCreateInfo.codeSize = shaderSize;
	moduleCreateInfo.pCode = shaderCode;

	VkShaderModule shaderModule = VK_NULL_HANDLE;

	if (!gpu->usingMaintenance5) {
		VK_CALL_RES(vkCreateShaderModule, device, &moduleCreateInfo, g_allocator, &shaderModule);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		gpu->shaderModule = shaderModule;
	}

	VkPipelineCacheCreateInfo cacheCreateInfo = {0};
	cacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheCreateInfo.flags =
		gpu->usingPipelineCreationCacheControl ? VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT : 0;
	cacheCreateInfo.initialDataSize = cacheSize;
	cacheCreateInfo.pInitialData = cacheData;

	VkPipelineCache pipelineCache;
	VK_CALL_RES(vkCreatePipelineCache, device, &cacheCreateInfo, g_allocator, &pipelineCache);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	gpu->pipelineCache = pipelineCache;

	VkPipelineLayoutCreateInfo layoutCreateInfo = {0};
	layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layoutCreateInfo.setLayoutCount = 1;
	layoutCreateInfo.pSetLayouts = &descSetLayout;

	VkPipelineLayout pipelineLayout;
	VK_CALL_RES(vkCreatePipelineLayout, device, &layoutCreateInfo, g_allocator, &pipelineLayout);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	gpu->pipelineLayout = pipelineLayout;

	uint32_t specialisationData[1];
	specialisationData[0] = workgroupSize;

	VkSpecializationMapEntry specialisationMapEntries[1];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset = 0;
	specialisationMapEntries[0].size = sizeof(specialisationData[0]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = ARR_SIZE(specialisationMapEntries);
	specialisationInfo.pMapEntries = specialisationMapEntries;
	specialisationInfo.dataSize = sizeof(specialisationData);
	specialisationInfo.pData = specialisationData;

	VkPipelineShaderStageCreateInfo stageCreateInfo = {0};
	stageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	stageCreateInfo.pNext = gpu->usingMaintenance5 ? &moduleCreateInfo : NULL;
	stageCreateInfo.flags =
		gpu->usingSubgroupSizeControl ? VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT : 0;
	stageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stageCreateInfo.module = shaderModule;
	stageCreateInfo.pName = entryPointName;
	stageCreateInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo pipelineCreateInfos[1] = {0};
	pipelineCreateInfos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineCreateInfos[0].flags =
		gpu->usingPipelineExecutableProperties ?
		VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR | VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR :
		0;
	pipelineCreateInfos[0].stage = stageCreateInfo;
	pipelineCreateInfos[0].layout = pipelineLayout;

	VkPipeline pipeline;
	VK_CALL_RES(vkCreateComputePipelines,
		device, pipelineCache, ARR_SIZE(pipelineCreateInfos), pipelineCreateInfos, g_allocator, &pipeline);

	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	gpu->pipeline = pipeline;

	bres = save_pipeline_cache(device, pipelineCache, PIPELINE_CACHE_NAME);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	if (transferQfTimestampValidBits || computeQfTimestampValidBits) {
		VkQueryPoolCreateInfo queryPoolCreateInfo = {0};
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = inoutsPerHeap * 4;

		VkQueryPool queryPool;
		VK_CALL_RES(vkCreateQueryPool, device, &queryPoolCreateInfo, g_allocator, &queryPool);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		gpu->queryPool = queryPool;
	}

	if (gpu->usingPipelineExecutableProperties) {
		bres = capture_pipeline(device, pipeline);
		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

	VK_CALL(vkDestroyDescriptorSetLayout, device, descSetLayout, g_allocator);
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	VK_CALL(vkDestroyPipelineCache, device, pipelineCache, g_allocator);
	gpu->pipelineCache = VK_NULL_HANDLE;

	if (!gpu->usingMaintenance5) {
		VK_CALL(vkDestroyShaderModule, device, shaderModule, g_allocator);
		gpu->shaderModule = VK_NULL_HANDLE;
	}

	free_recursive(dyMem);
	return true;
}

static bool record_transfer_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	VkBuffer hvBuffer,
	VkBuffer dlBuffer,
	const VkBufferCopy* inBufferRegion,
	const VkBufferCopy* outBufferRegion,
	const VkDependencyInfo* depInfos,
	VkQueryPool queryPool,
	uint32_t firstQuery,
	uint32_t timestampValidBits)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CALL_RES(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if EXPECT_FALSE (vkres) { return false; }

	if (timestampValidBits) {
		// TODO Remove (vkCmdResetQueryPool not guaranteed to work on dedicated transfer queue families)
		uint32_t queryCount = 2;
		VK_CALL(vkCmdResetQueryPool, cmdBuffer, queryPool, firstQuery, queryCount);

		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
		uint32_t query = firstQuery;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	uint32_t inBufferRegionCount = 1;
	VK_CALL(vkCmdCopyBuffer, cmdBuffer, hvBuffer, dlBuffer, inBufferRegionCount, inBufferRegion);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[0]);
	
	uint32_t outBufferRegionCount = 1;
	VK_CALL(vkCmdCopyBuffer, cmdBuffer, dlBuffer, hvBuffer, outBufferRegionCount, outBufferRegion);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[1]);

	if (timestampValidBits) {
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_COPY_BIT;
		uint32_t query = firstQuery + 1;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	VK_CALL_RES(vkEndCommandBuffer, cmdBuffer);
	if EXPECT_FALSE (vkres) { return false; }

	return true;
}

static bool record_compute_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	VkPipeline pipeline,
	VkPipelineLayout layout,
	const VkDescriptorSet* descSet,
	const VkDependencyInfo* depInfos,
	VkQueryPool queryPool,
	uint32_t firstQuery,
	uint32_t timestampValidBits,
	uint32_t workgroupCount)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CALL_RES(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if EXPECT_FALSE (vkres) { return false; }

	if (timestampValidBits) {
		uint32_t queryCount = 2;
		VK_CALL(vkCmdResetQueryPool, cmdBuffer, queryPool, firstQuery, queryCount);

		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
		uint32_t query = firstQuery;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[0]);

	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	uint32_t firstDescSet = 0;
	uint32_t descSetCount = 1;
	uint32_t dynOffsetCount = 0;
	uint32_t* dynOffsets = NULL;

	VK_CALL(vkCmdBindDescriptorSets,
		cmdBuffer, bindPoint, layout, firstDescSet, descSetCount, descSet, dynOffsetCount, dynOffsets);

	VK_CALL(vkCmdBindPipeline, cmdBuffer, bindPoint, pipeline);

	uint32_t workgroupCountX = workgroupCount;
	uint32_t workgroupCountY = 1;
	uint32_t workgroupCountZ = 1;

	// Use vkCmdDispatchBase if want to alter base value of workgroup index (first value of WorkgroupId)
	VK_CALL(vkCmdDispatch, cmdBuffer, workgroupCountX, workgroupCountY, workgroupCountZ);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[1]);

	if (timestampValidBits) {
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		uint32_t query = firstQuery + 1;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	VK_CALL_RES(vkEndCommandBuffer, cmdBuffer);
	if EXPECT_FALSE (vkres) { return false; }

	return true;
}

bool create_commands(Gpu* restrict gpu)
{
	const VkBuffer* hvBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* dlBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* descSets  = gpu->descriptorSets;

	VkCommandBuffer* transferCmdBuffers = gpu->transferCommandBuffers;
	VkCommandBuffer* computeCmdBuffers = gpu->computeCommandBuffers;
	VkSemaphore* semaphores = gpu->semaphores;

	VkDevice device = gpu->device;

	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;
	VkPipeline pipeline = gpu->pipeline;
	VkQueryPool queryPool = gpu->queryPool;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;
	uint32_t workgroupCount = gpu->workgroupCount;

	uint32_t transferQfIndex = gpu->transferQueueFamilyIndex;
	uint32_t computeQfIndex = gpu->computeQueueFamilyIndex;
	uint32_t transferQfTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;
	uint32_t computeQfTimestampValidBits = gpu->computeQueueFamilyTimestampValidBits;

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 1;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	size_t allocCount = 1;
	size_t allocSize =
		inoutsPerBuffer * sizeof(VkBufferCopy) * 2 +
		inoutsPerHeap * sizeof(VkBufferMemoryBarrier2) * 5 +
		inoutsPerHeap * sizeof(VkDependencyInfo) * 4;

	void* p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	VkBufferCopy* inBufferCopies = p;
	VkBufferCopy* outBufferCopies = (VkBufferCopy*) (inBufferCopies + inoutsPerBuffer);

	VkBufferMemoryBarrier2 (*transferBufferMemoryBarriers)[3] = (VkBufferMemoryBarrier2(*)[]) (
		outBufferCopies + inoutsPerBuffer);
	VkBufferMemoryBarrier2 (*computeBufferMemoryBarriers)[2] = (VkBufferMemoryBarrier2(*)[]) (
		transferBufferMemoryBarriers + inoutsPerHeap);

	VkDependencyInfo (*transferDependencyInfos)[2] = (VkDependencyInfo(*)[]) (
		computeBufferMemoryBarriers + inoutsPerHeap);
	VkDependencyInfo (*computeDependencyInfos)[2] = (VkDependencyInfo(*)[]) (transferDependencyInfos + inoutsPerHeap);

	const char* transferCmdPoolName = "Transfer";
	uint32_t transferCmdBufferCount = inoutsPerHeap;

	bool bres = create_command_handles(
		device, transferQfIndex, &gpu->transferCommandPool, transferCmdPoolName, transferCmdBufferCount,
		transferCmdBuffers);

	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	const char* computeCmdPoolName = "Compute";
	uint32_t computeCmdBufferCount = inoutsPerHeap;

	bres = create_command_handles(
		device, computeQfIndex, &gpu->computeCommandPool, computeCmdPoolName, computeCmdBufferCount, computeCmdBuffers);

	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < inoutsPerBuffer; i++) {
		inBufferCopies[i].srcOffset = bytesPerInout * i;
		inBufferCopies[i].dstOffset = bytesPerInout * i;
		inBufferCopies[i].size = bytesPerIn;

		outBufferCopies[i].srcOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].dstOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].size = bytesPerOut;
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer hvBuffer = hvBuffers[i];
		VkBuffer dlBuffer = dlBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			transferBufferMemoryBarriers[j][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][0].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferQfIndex;
			transferBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeQfIndex;
			transferBufferMemoryBarriers[j][0].buffer = dlBuffer;
			transferBufferMemoryBarriers[j][0].offset = bytesPerInout * k;
			transferBufferMemoryBarriers[j][0].size = bytesPerIn;

			transferBufferMemoryBarriers[j][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][1].dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][1].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			transferBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeQfIndex;
			transferBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferQfIndex;
			transferBufferMemoryBarriers[j][1].buffer = dlBuffer;
			transferBufferMemoryBarriers[j][1].offset = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers[j][1].size = bytesPerOut;

			transferBufferMemoryBarriers[j][2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][2].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][2].dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
			transferBufferMemoryBarriers[j][2].dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
			transferBufferMemoryBarriers[j][2].buffer = hvBuffer;
			transferBufferMemoryBarriers[j][2].offset = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers[j][2].size = bytesPerOut;

			computeBufferMemoryBarriers[j][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			computeBufferMemoryBarriers[j][0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][0].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
			computeBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferQfIndex;
			computeBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeQfIndex;
			computeBufferMemoryBarriers[j][0].buffer = dlBuffer;
			computeBufferMemoryBarriers[j][0].offset = bytesPerInout * k;
			computeBufferMemoryBarriers[j][0].size = bytesPerIn;

			computeBufferMemoryBarriers[j][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			computeBufferMemoryBarriers[j][1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][1].srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			computeBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeQfIndex;
			computeBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferQfIndex;
			computeBufferMemoryBarriers[j][1].buffer = dlBuffer;
			computeBufferMemoryBarriers[j][1].offset = bytesPerInout * k + bytesPerIn;
			computeBufferMemoryBarriers[j][1].size = bytesPerOut;

			transferDependencyInfos[j][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			transferDependencyInfos[j][0].bufferMemoryBarrierCount = 2;
			transferDependencyInfos[j][0].pBufferMemoryBarriers = &transferBufferMemoryBarriers[j][0];

			transferDependencyInfos[j][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			transferDependencyInfos[j][1].bufferMemoryBarrierCount = 1;
			transferDependencyInfos[j][1].pBufferMemoryBarriers = &transferBufferMemoryBarriers[j][2];

			computeDependencyInfos[j][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			computeDependencyInfos[j][0].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[j][0].pBufferMemoryBarriers = &computeBufferMemoryBarriers[j][0];

			computeDependencyInfos[j][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			computeDependencyInfos[j][1].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[j][1].pBufferMemoryBarriers = &computeBufferMemoryBarriers[j][1];
		}
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			uint32_t firstTransferQuery = j * 4;
			bres = record_transfer_cmdbuffer(
				transferCmdBuffers[j], hvBuffers[i], dlBuffers[i], &inBufferCopies[k], &outBufferCopies[k],
				transferDependencyInfos[j], queryPool, firstTransferQuery, transferQfTimestampValidBits);

			if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

			uint32_t firstComputeQuery = j * 4 + 2;
			bres = record_compute_cmdbuffer(
				computeCmdBuffers[j], pipeline, pipelineLayout, &descSets[j], computeDependencyInfos[j], queryPool,
				firstComputeQuery, computeQfTimestampValidBits, workgroupCount);

			if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
		}
	}

	VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {0};
	semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	semaphoreTypeCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {0};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VkSemaphore semaphore;
		VK_CALL_RES(vkCreateSemaphore, device, &semaphoreCreateInfo, g_allocator, &semaphore);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		semaphores[i] = semaphore;
	}

#ifndef NDEBUG
	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			char objectName[68];
			char specs[60];

			sprintf(
				specs,
				", Inout %" PRIu32 "/%" PRIu32 ", Buffer %" PRIu32 "/%" PRIu32,
				k + 1, inoutsPerBuffer, i + 1, buffersPerHeap);

			strcpy(objectName, "Transfer");
			strcat(objectName, specs);

			set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) transferCmdBuffers[j], objectName);

			strcpy(objectName, "Compute");
			strcat(objectName, specs);

			set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) computeCmdBuffers[j], objectName);
		}
	}
#endif

	if (gpu->usingMaintenance4) {
		VK_CALL(vkDestroyPipelineLayout, device, pipelineLayout, g_allocator);
		gpu->pipelineLayout = VK_NULL_HANDLE;
	}

	free_recursive(dyMem);
	return true;
}

bool submit_commands(Gpu* restrict gpu)
{
	const VkDeviceMemory* hvMemories = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer* transferCmdBuffers = gpu->transferCommandBuffers;
	const VkCommandBuffer* computeCmdBuffers = gpu->computeCommandBuffers;
	const VkSemaphore* semaphores = gpu->semaphores;

	StartValue* const* mappedInBuffers = gpu->mappedInBuffers;
	StopTime* const* mappedOutBuffers = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkQueue transferQueue = gpu->transferQueue;
	VkQueue computeQueue = gpu->computeQueue;

	VkQueryPool queryPool = gpu->queryPool;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t valuesPerInout = gpu->valuesPerInout;
	uint32_t valuesPerHeap = gpu->valuesPerHeap;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	uint32_t transferQfTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;
	uint32_t computeQfTimestampValidBits = gpu->computeQueueFamilyTimestampValidBits;

	double timestampPeriod = (double) gpu->timestampPeriod;
	bool hostNonCoherent = gpu->hostNonCoherent;

	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 3;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	size_t allocCount = 1;
	size_t allocSize =
		inoutsPerHeap * sizeof(StartValue) +
		inoutsPerHeap * sizeof(VkMappedMemoryRange) * 2 +
		inoutsPerHeap * sizeof(VkSubmitInfo2) * 2 +
		inoutsPerHeap * sizeof(VkCommandBufferSubmitInfo) * 2 +
		inoutsPerHeap * sizeof(VkSemaphoreSubmitInfo) * 4 +
		inoutsPerHeap * sizeof(VkSemaphoreWaitInfo) * 2;

	void* p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	StartValue* testedValues = p;

	VkMappedMemoryRange* hvInBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (testedValues + inoutsPerHeap);
	VkMappedMemoryRange* hvOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (
		hvInBuffersMappedMemoryRanges + inoutsPerHeap);

	VkSubmitInfo2* transferSubmitInfos = (VkSubmitInfo2*) (hvOutBuffersMappedMemoryRanges + inoutsPerHeap);
	VkSubmitInfo2* computeSubmitInfos = (VkSubmitInfo2*) (transferSubmitInfos + inoutsPerHeap);

	VkCommandBufferSubmitInfo* transferCmdBufferSubmitInfos = (VkCommandBufferSubmitInfo*) (
		computeSubmitInfos + inoutsPerHeap);
	VkCommandBufferSubmitInfo* computeCmdBufferSubmitInfos = (VkCommandBufferSubmitInfo*) (
		transferCmdBufferSubmitInfos + inoutsPerHeap);

	VkSemaphoreSubmitInfo* transferWaitSemaphoreSubmitInfos = (VkSemaphoreSubmitInfo*) (
		computeCmdBufferSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfo* transferSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfo*) (
		transferWaitSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfo* computeWaitSemaphoreSubmitInfos = (VkSemaphoreSubmitInfo*) (
		transferSignalSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfo* computeSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfo*) (
		computeWaitSemaphoreSubmitInfos + inoutsPerHeap);

	VkSemaphoreWaitInfo* transferSemaphoreWaitInfos = (VkSemaphoreWaitInfo*) (
		computeSignalSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreWaitInfo* computeSemaphoreWaitInfos = (VkSemaphoreWaitInfo*) (
		transferSemaphoreWaitInfos + inoutsPerHeap);
	
	elmSize = sizeof(StartValue);
	elmCount = 32;

	DyArray bestStartValues = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!bestStartValues) { free_recursive(dyMem); return false; }

	dyData.data = bestStartValues;
	dyData.free = dyarray_destroy_stub;

	dyarray_append(dyMem, &dyData);

	elmSize = sizeof(StopTime);
	elmCount = 32;

	DyArray bestStopTimes = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!bestStopTimes) { free_recursive(dyMem); return false; }

	dyData.data = bestStopTimes;
	dyData.free = dyarray_destroy_stub;

	dyarray_append(dyMem, &dyData);

	size_t fileSize;
	bool bres = file_size(PROGRESS_FILE_NAME, &fileSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	Position position = {0};
	position.val0mod1off[0] = 1;
	position.curStartValue = 3;

	if (!g_config.restart && fileSize) {
		uint64_t val0mod1off0Upper, val0mod1off0Lower;
		uint64_t val0mod1off1Upper, val0mod1off1Lower;
		uint64_t val0mod1off2Upper, val0mod1off2Lower;
		uint64_t val1mod6off0Upper, val1mod6off0Lower;
		uint64_t val1mod6off1Upper, val1mod6off1Lower;
		uint64_t val1mod6off2Upper, val1mod6off2Lower;
		uint64_t curValueUpper, curValueLower;
		uint16_t bestTime;

		bres = read_text(
			PROGRESS_FILE_NAME,
			"%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n"
			"%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n"
			"%" SCNx64 " %" SCNx64 "\n%" SCNx16,
			&val0mod1off0Upper, &val0mod1off0Lower,
			&val0mod1off1Upper, &val0mod1off1Lower,
			&val0mod1off2Upper, &val0mod1off2Lower,
			&val1mod6off0Upper, &val1mod6off0Lower,
			&val1mod6off1Upper, &val1mod6off1Lower,
			&val1mod6off2Upper, &val1mod6off2Lower,
			&curValueUpper, &curValueLower, &bestTime);

		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

		position.val0mod1off[0] = INT128(val0mod1off0Upper, val0mod1off0Lower);
		position.val0mod1off[1] = INT128(val0mod1off1Upper, val0mod1off1Lower);
		position.val0mod1off[2] = INT128(val0mod1off2Upper, val0mod1off2Lower);

		position.val1mod6off[0] = INT128(val1mod6off0Upper, val1mod6off0Lower);
		position.val1mod6off[1] = INT128(val1mod6off1Upper, val1mod6off1Lower);
		position.val1mod6off[2] = INT128(val1mod6off2Upper, val1mod6off2Lower);

		position.curStartValue = INT128(curValueUpper, curValueLower);
		position.bestStopTime = bestTime;
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkDeviceMemory hvMemory = hvMemories[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			hvInBuffersMappedMemoryRanges[j].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			hvInBuffersMappedMemoryRanges[j].memory = hvMemory;
			hvInBuffersMappedMemoryRanges[j].offset = bytesPerInout * k;
			hvInBuffersMappedMemoryRanges[j].size = bytesPerIn;

			hvOutBuffersMappedMemoryRanges[j].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			hvOutBuffersMappedMemoryRanges[j].memory = hvMemory;
			hvOutBuffersMappedMemoryRanges[j].offset = bytesPerInout * k + bytesPerIn;
			hvOutBuffersMappedMemoryRanges[j].size = bytesPerOut;
		}
	}

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferCmdBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		transferCmdBufferSubmitInfos[i].commandBuffer = transferCmdBuffers[i];

		computeCmdBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		computeCmdBufferSubmitInfos[i].commandBuffer = computeCmdBuffers[i];

		transferWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		transferWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferWaitSemaphoreSubmitInfos[i].value = 0;
		transferWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include acquire op

		transferSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		transferSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferSignalSemaphoreSubmitInfos[i].value = 1;
		transferSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include release op

		computeWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		computeWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeWaitSemaphoreSubmitInfos[i].value = 1;
		computeWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include acquire op

		computeSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		computeSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeSignalSemaphoreSubmitInfos[i].value = 2;
		computeSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include release op

		transferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		transferSubmitInfos[i].waitSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pWaitSemaphoreInfos = &transferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos[i].commandBufferInfoCount = 1;
		transferSubmitInfos[i].pCommandBufferInfos = &transferCmdBufferSubmitInfos[i];
		transferSubmitInfos[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pSignalSemaphoreInfos = &transferSignalSemaphoreSubmitInfos[i];

		computeSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		computeSubmitInfos[i].waitSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pWaitSemaphoreInfos = &computeWaitSemaphoreSubmitInfos[i];
		computeSubmitInfos[i].commandBufferInfoCount = 1;
		computeSubmitInfos[i].pCommandBufferInfos = &computeCmdBufferSubmitInfos[i];
		computeSubmitInfos[i].signalSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pSignalSemaphoreInfos = &computeSignalSemaphoreSubmitInfos[i];

		transferSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		transferSemaphoreWaitInfos[i].semaphoreCount = 1;
		transferSemaphoreWaitInfos[i].pSemaphores = &semaphores[i];
		transferSemaphoreWaitInfos[i].pValues = &transferSignalSemaphoreSubmitInfos[i].value;

		computeSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		computeSemaphoreWaitInfos[i].semaphoreCount = 1;
		computeSemaphoreWaitInfos[i].pSemaphores = &semaphores[i];
		computeSemaphoreWaitInfos[i].pValues = &computeSignalSemaphoreSubmitInfos[i].value;
	}

	clock_t totalBmStart = clock();
	StartValue tested = position.curStartValue;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		testedValues[i] = tested;
		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);
		tested += valuesPerInout * 4;
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hvInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	uint32_t transferSubmitInfoCount = inoutsPerHeap;
	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, transferSubmitInfoCount, transferSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	uint32_t computeSubmitInfoCount = inoutsPerHeap;
	VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, computeSubmitInfoCount, computeSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	atomic_bool input;
	atomic_init(&input, false);

	pthread_t waitThread;
	int ires = pthread_create(&waitThread, NULL, wait_for_input, &input);
	if EXPECT_FALSE (ires) { PCREATE_FAILURE(ires); free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		uint64_t transferSemaphoreTimeout = UINT64_MAX;
		VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[i], transferSemaphoreTimeout);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);

		transferWaitSemaphoreSubmitInfos[i].value += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hvInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	transferSubmitInfoCount = inoutsPerHeap;
	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, transferSubmitInfoCount, transferSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	StartValue total = 0;
	StartValue initialStartValue = position.curStartValue;

	// ===== Enter main loop =====
	for (uint64_t i = 0; i < g_config.maxLoops && !atomic_load(&input); i++) {
		clock_t mainLoopBmStart = clock();
		StartValue initialValue = position.curStartValue;

		double readBmTotal = 0;
		double writeBmTotal = 0;
		double waitComputeBmTotal = 0;
		double waitTransferBmTotal = 0;
		double computeBmTotal = 0;
		double transferBmTotal = 0;

		if (g_config.outputLevel > OUTPUT_LEVEL_SILENT) {
			printf("Loop #%" PRIu64 "\n", i + 1);
		}

		/*
		 * The following loop has two invocations of the vkGetQueryPoolResults function. On my Windows/Linux PC, these
		 * functions always return VK_SUCCESS. But on my Macbook, they very rarely return VK_NOT_READY. I'm yet to find
		 * a consistent pattern regarding when these failures occur, nor have I found a way to reliably replicate them.
		 * TODO Figure out what on Earth is going on here???
		 */
		for (uint32_t j = 0; j < inoutsPerHeap; j++) {
			uint64_t timestamps[2];

			double computeBmark = 0;
			double transferBmark = 0;

			clock_t waitComputeBmStart = clock();

			uint64_t computeSemaphoreTimeout = UINT64_MAX;
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &computeSemaphoreWaitInfos[j], computeSemaphoreTimeout);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			clock_t waitComputeBmEnd = clock();

			if (computeQfTimestampValidBits) {
				uint32_t firstQuery = j * 4 + 2;
				uint32_t queryCount = 2;
				VkQueryResultFlags queryFlags = VK_QUERY_RESULT_64_BIT;

				VK_CALL_RES(vkGetQueryPoolResults,
					device, queryPool, firstQuery, queryCount, sizeof(timestamps), timestamps, sizeof(timestamps[0]),
					queryFlags);

				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
				computeBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			computeWaitSemaphoreSubmitInfos[j].value += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			computeSubmitInfoCount = 1;
			VK_CALL_RES(vkQueueSubmit2KHR,
				computeQueue, computeSubmitInfoCount, &computeSubmitInfos[j], VK_NULL_HANDLE);

			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			clock_t waitTransferBmStart = clock();

			uint64_t transferSemaphoreTimeout = UINT64_MAX;
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[j], transferSemaphoreTimeout);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			clock_t waitTransferBmEnd = clock();

			if (transferQfTimestampValidBits) {
				uint32_t firstQuery = j * 4;
				uint32_t queryCount = 2;
				VkQueryResultFlags queryFlags = VK_QUERY_RESULT_64_BIT;

				VK_CALL_RES(vkGetQueryPoolResults,
					device, queryPool, firstQuery, queryCount, sizeof(timestamps), timestamps, sizeof(timestamps[0]),
					queryFlags);

				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
				transferBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (hostNonCoherent) {
				uint32_t rangeCount = 1;
				VK_CALL_RES(vkInvalidateMappedMemoryRanges, device, rangeCount, &hvOutBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			}

			clock_t readBmStart = clock();
			read_outbuffer(mappedOutBuffers[j], &position, bestStartValues, bestStopTimes, valuesPerInout);
			clock_t readBmEnd = clock();

			clock_t writeBmStart = clock();
			write_inbuffer(mappedInBuffers[j], &testedValues[j], valuesPerInout, valuesPerHeap);
			clock_t writeBmEnd = clock();

			if (hostNonCoherent) {
				uint32_t rangeCount = 1;
				VK_CALL_RES(vkFlushMappedMemoryRanges, device, rangeCount, &hvInBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			}

			transferWaitSemaphoreSubmitInfos[j].value += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			transferSubmitInfoCount = 1;
			VK_CALL_RES(vkQueueSubmit2KHR,
				transferQueue, transferSubmitInfoCount, &transferSubmitInfos[j], VK_NULL_HANDLE);

			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return NULL; }

			double readBmark = get_benchmark(readBmStart, readBmEnd);
			double writeBmark = get_benchmark(writeBmStart, writeBmEnd);
			double waitComputeBmark = get_benchmark(waitComputeBmStart, waitComputeBmEnd);
			double waitTransferBmark = get_benchmark(waitTransferBmStart, waitTransferBmEnd);

			readBmTotal += readBmark;
			writeBmTotal += writeBmark;
			computeBmTotal += computeBmark;
			transferBmTotal += transferBmark;
			waitComputeBmTotal += waitComputeBmark;
			waitTransferBmTotal += waitTransferBmark;

			if (g_config.outputLevel > OUTPUT_LEVEL_QUIET) {
				printf(
					"Inout-buffer %" PRIu32 "/%" PRIu32 "\n"
					"\tReading buffers:    %8.0fms\n"
					"\tWriting buffers:    %8.0fms\n"
					"\tCompute execution:  %8.0fms\n"
					"\tTransfer execution: %8.0fms\n"
					"\tIdle (compute):     %8.0fms\n"
					"\tIdle (transfer):    %8.0fms\n",
					j + 1, inoutsPerHeap,
					readBmark, writeBmark, computeBmark, transferBmark, waitComputeBmark, waitTransferBmark);
			}
		}

		total += valuesPerHeap * 4;

		clock_t mainLoopBmEnd = clock();
		double mainLoopBmark = get_benchmark(mainLoopBmStart, mainLoopBmEnd);

		switch (g_config.outputLevel) {
			case OUTPUT_LEVEL_QUIET:
				printf(
					"Main loop: %.0fms\n"
					"Current value: 0x %016" PRIx64 " %016" PRIx64 "\n\n",
					mainLoopBmark, INT128_UPPER(position.curStartValue - 3), INT128_LOWER(position.curStartValue - 3));

				break;

			case OUTPUT_LEVEL_DEFAULT:
				printf(
					"Main loop: %.0fms\n"
					"Reading buffers:    %8.1fms\n"
					"Writing buffers:    %8.1fms\n"
					"Compute execution:  %8.1fms\n"
					"Transfer execution: %8.1fms\n"
					"Idle (compute):     %8.1fms\n"
					"Idle (transfer):    %8.1fms\n"
					"Initial value: 0x %016" PRIx64 " %016" PRIx64 "\n"
					"Current value: 0x %016" PRIx64 " %016" PRIx64 "\n\n",
					mainLoopBmark,
					readBmTotal / (double) inoutsPerHeap, writeBmTotal / (double) inoutsPerHeap,
					computeBmTotal / (double) inoutsPerHeap, transferBmTotal / (double) inoutsPerHeap,
					waitComputeBmTotal / (double) inoutsPerHeap, waitTransferBmTotal / (double) inoutsPerHeap,
					INT128_UPPER(initialValue - 2), INT128_LOWER(initialValue - 2),
					INT128_UPPER(position.curStartValue - 3), INT128_LOWER(position.curStartValue - 3));

				break;

			case OUTPUT_LEVEL_VERBOSE:
				printf(
					"Main loop: %.0fms\n"
					"|      Benchmark     | Total (ms) | Average (ms) |\n"
					"|    Reading buffers | %10.0f | %12.1f |\n"
					"|    Writing buffers | %10.0f | %12.1f |\n"
					"|  Compute execution | %10.0f | %12.1f |\n"
					"| Transfer execution | %10.0f | %12.1f |\n"
					"|     Idle (compute) | %10.0f | %12.1f |\n"
					"|    Idle (transfer) | %10.0f | %12.1f |\n"
					"Initial value: 0x %016" PRIx64 " %016" PRIx64 "\n"
					"Current value: 0x %016" PRIx64 " %016" PRIx64 "\n\n",
					mainLoopBmark,
					readBmTotal, readBmTotal / (double) inoutsPerHeap,
					writeBmTotal, writeBmTotal / (double) inoutsPerHeap,
					computeBmTotal, computeBmTotal / (double) inoutsPerHeap,
					transferBmTotal, transferBmTotal / (double) inoutsPerHeap,
					waitComputeBmTotal, waitComputeBmTotal / (double) inoutsPerHeap,
					waitTransferBmTotal, waitTransferBmTotal / (double) inoutsPerHeap,
					INT128_UPPER(initialValue - 2), INT128_LOWER(initialValue - 2),
					INT128_UPPER(position.curStartValue - 3), INT128_LOWER(position.curStartValue - 3));

				break;

			default:
				break;
		}
	}
	NEWLINE();

	clock_t totalBmEnd = clock();
	double totalBmark = get_benchmark(totalBmStart, totalBmEnd);

	if (atomic_load(&input)) {
		ires = pthread_join(waitThread, NULL);
		if EXPECT_FALSE (ires) { PJOIN_FAILURE(ires); free_recursive(dyMem); return false; }
	}
	else {
		atomic_store(&input, true);
		ires = pthread_cancel(waitThread);
		if EXPECT_FALSE (ires) { PCANCEL_FAILURE(ires); free_recursive(dyMem); return false; }
	}

	if (g_config.outputLevel > OUTPUT_LEVEL_SILENT) {
		printf(
			"Set of starting values tested: [0x %016" PRIx64 " %016" PRIx64 ", 0x %016" PRIx64 " %016" PRIx64 "]\n",
			INT128_UPPER(initialStartValue - 2), INT128_LOWER(initialStartValue - 2),
			INT128_UPPER(position.curStartValue - 3), INT128_LOWER(position.curStartValue - 3));
	}

	uint32_t bestCount = (uint32_t) dyarray_size(bestStartValues);

	if (bestCount) {
		printf(
			"New highest total stopping times (%" PRIu32 "):\n"
			"|   #   |   Starting value (hexadecimal)    | Total stopping time |\n",
			bestCount);
	}

	for (uint32_t i = 0; i < bestCount; i++) {
		StartValue startValue;
		StopTime stopTime;

		dyarray_get(bestStartValues, &startValue, i);
		dyarray_get(bestStopTimes, &stopTime, i);

		printf(
			"| %5" PRIu32 " | %016" PRIx64 " %016" PRIx64 " | %19" PRIu16 " |\n",
			i + 1, INT128_UPPER(startValue), INT128_LOWER(startValue), stopTime);
	}

	if (g_config.outputLevel > OUTPUT_LEVEL_SILENT) {
		printf(
			"\n"
			"Time: %.3fms\n"
			"Speed: %.3f/s\n",
			totalBmark, (double) (1000 * total) / totalBmark);
	}

	if (!g_config.restart) {
		bres = write_text(
			PROGRESS_FILE_NAME,
			"%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n"
			"%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n"
			"%016" PRIx64 " %016" PRIx64 "\n%04"  PRIx16,
			INT128_UPPER(position.val0mod1off[0]), INT128_LOWER(position.val0mod1off[0]),
			INT128_UPPER(position.val0mod1off[1]), INT128_LOWER(position.val0mod1off[1]),
			INT128_UPPER(position.val0mod1off[2]), INT128_LOWER(position.val0mod1off[2]),
			INT128_UPPER(position.val1mod6off[0]), INT128_LOWER(position.val1mod6off[0]),
			INT128_UPPER(position.val1mod6off[1]), INT128_LOWER(position.val1mod6off[1]),
			INT128_UPPER(position.val1mod6off[2]), INT128_LOWER(position.val1mod6off[2]),
			INT128_UPPER(position.curStartValue), INT128_LOWER(position.curStartValue), position.bestStopTime);

		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

	free_recursive(dyMem);
	return true;
}

bool destroy_gpu(Gpu* restrict gpu)
{
	VkInstance instance = volkGetLoadedInstance();

	const VkBuffer* hvBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* dlBuffers = gpu->deviceLocalBuffers;
	const VkDeviceMemory* hvMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* dlMemories = gpu->deviceLocalDeviceMemories;
	const VkSemaphore* semaphores = gpu->semaphores;

	VkDevice device = gpu->device;

	VkDescriptorSetLayout descSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool descPool = gpu->descriptorPool;
	VkShaderModule shaderModule = gpu->shaderModule;
	VkPipelineCache pipelineCache = gpu->pipelineCache;
	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;
	VkPipeline pipeline = gpu->pipeline;
	VkCommandPool computeCmdPool = gpu->computeCommandPool;
	VkCommandPool transferCmdPool = gpu->transferCommandPool;
	VkQueryPool queryPool = gpu->queryPool;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	VkResult vkres;

	if (device) {
		VK_CALL(vkDestroyShaderModule, device, shaderModule, g_allocator);
		VK_CALL(vkDestroyPipelineCache, device, pipelineCache, g_allocator);
		VK_CALL(vkDestroyPipelineLayout, device, pipelineLayout, g_allocator);
		VK_CALL(vkDestroyDescriptorSetLayout, device, descSetLayout, g_allocator);

		// Make sure no command buffers are in the pending state
		VK_CALL_RES(vkDeviceWaitIdle, device);

		for (uint32_t i = 0; i < inoutsPerHeap; i++) {
			VK_CALL(vkDestroySemaphore, device, semaphores[i], g_allocator);
		}

		VK_CALL(vkDestroyCommandPool, device, computeCmdPool, g_allocator);
		VK_CALL(vkDestroyCommandPool, device, transferCmdPool, g_allocator);

		VK_CALL(vkDestroyPipeline, device, pipeline, g_allocator);
		VK_CALL(vkDestroyQueryPool, device, queryPool, g_allocator);
		VK_CALL(vkDestroyDescriptorPool, device, descPool, g_allocator);

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			VK_CALL(vkDestroyBuffer, device, hvBuffers[i], g_allocator);
			VK_CALL(vkDestroyBuffer, device, dlBuffers[i], g_allocator);

			VK_CALL(vkFreeMemory, device, hvMemories[i], g_allocator);
			VK_CALL(vkFreeMemory, device, dlMemories[i], g_allocator);
		}

		VK_CALL(vkDestroyDevice, device, g_allocator);
	}

	if (instance) {
#ifndef NDEBUG
		VK_CALL(vkDestroyDebugUtilsMessengerEXT, instance, gpu->debugUtilsMessenger, g_allocator);
#endif
		VK_CALL(vkDestroyInstance, instance, g_allocator);
	}

	volkFinalize();

	free(gpu->dynamicMemory);
	return true;
}

bool create_command_handles(
	VkDevice device,
	uint32_t queueFamilyIndex,
	VkCommandPool* commandPool,
	const char* name,
	uint32_t commandBufferCount,
	VkCommandBuffer* commandBuffers)
{
	(void) name;
	VkResult vkres;

	VkCommandPoolCreateInfo createInfo = {0};
	createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	createInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool newCommandPool;
	VK_CALL_RES(vkCreateCommandPool, device, &createInfo, g_allocator, &newCommandPool);
	if EXPECT_FALSE (vkres) { return false; }
	*commandPool = newCommandPool;

#ifndef NDEBUG
	if (name) {
		bool bres = set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) newCommandPool, name);
		if EXPECT_FALSE (!bres) { return false; }
	}
#endif

	VkCommandBufferAllocateInfo allocateInfo = {0};
	allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocateInfo.commandPool = newCommandPool;
	allocateInfo.commandBufferCount = commandBufferCount;

	VK_CALL_RES(vkAllocateCommandBuffers, device, &allocateInfo, commandBuffers);
	if EXPECT_FALSE (vkres) { return false; }

	return true;
}

bool capture_pipeline(VkDevice device, VkPipeline pipeline)
{
	VkResult vkres;

	DyData dyData;
	size_t elmSize = sizeof(DyData);
	size_t elmCount = 3;

	DyArray dyMem = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!dyMem) { return false; }

	VkPipelineInfoKHR pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
	pipelineInfo.pipeline = pipeline;

	uint32_t executableCount;
	VkPipelineExecutablePropertiesKHR* executableProperties = NULL;

	VK_CALL_RES(vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, executableProperties);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	if EXPECT_FALSE (!executableCount) {
		log_warning(stdout, "No pipeline executables are available for capture");
		free_recursive(dyMem);
		return true;
	}

	size_t allocCount = 1;
	size_t allocSize =
		executableCount * sizeof(VkPipelineExecutablePropertiesKHR) +
		executableCount * sizeof(VkPipelineExecutableStatisticKHR*) +
		executableCount * sizeof(VkPipelineExecutableInfoKHR) +
		executableCount * sizeof(uint32_t);

	void* p = calloc(allocCount, allocSize);
	if EXPECT_FALSE (!p) { CALLOC_FAILURE(p, allocCount, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	VkPipelineExecutablePropertiesKHR* pipelineExecutablesProperties = p;
	VkPipelineExecutableStatisticKHR** pipelineExecutablesStatistics = (VkPipelineExecutableStatisticKHR**) (
		pipelineExecutablesProperties + executableCount);
	VkPipelineExecutableInfoKHR* pipelineExecutablesInfo = (VkPipelineExecutableInfoKHR*) (
		pipelineExecutablesStatistics + executableCount);

	uint32_t* statisticCounts = (uint32_t*) (pipelineExecutablesInfo + executableCount);

	VK_CALL_RES(vkGetPipelineExecutablePropertiesKHR,
		device, &pipelineInfo, &executableCount, pipelineExecutablesProperties);

	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < executableCount; i++) {
		pipelineExecutablesInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
		pipelineExecutablesInfo[i].pipeline = pipeline;
		pipelineExecutablesInfo[i].executableIndex = i;
	}

	uint32_t statisticTotal = 0;

	for (uint32_t i = 0; i < executableCount; i++) {
		VkPipelineExecutableStatisticKHR* executableStatistics = NULL;
		VK_CALL_RES(vkGetPipelineExecutableStatisticsKHR,
			device, &pipelineExecutablesInfo[i], &statisticCounts[i], executableStatistics);

		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		statisticTotal += statisticCounts[i];
	}

	allocSize = statisticTotal * sizeof(VkPipelineExecutableStatisticKHR);
	p = malloc(allocSize);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, allocSize); free_recursive(dyMem); return false; }

	dyData.data = p;
	dyData.free = free;

	dyarray_append(dyMem, &dyData);

	pipelineExecutablesStatistics[0] = p;

	for (uint32_t i = 1; i < executableCount; i++) {
		pipelineExecutablesStatistics[i] = pipelineExecutablesStatistics[i - 1] + statisticCounts[i - 1];
	}

	for (uint32_t i = 0; i < executableCount; i++) {
		for (uint32_t j = 0; j < statisticCounts[i]; j++) {
			pipelineExecutablesStatistics[i][j].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
		}

		VK_CALL_RES(vkGetPipelineExecutableStatisticsKHR,
			device, &pipelineExecutablesInfo[i], &statisticCounts[i], pipelineExecutablesStatistics[i]);
		
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	DyString message = dystring_create(1024);
	if EXPECT_FALSE (!message) { free_recursive(dyMem); return false; }

	dyData.data = message;
	dyData.free = dystring_destroy_stub;

	dyarray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < executableCount; i++) {
		VkShaderStageFlags stages = pipelineExecutablesProperties[i].stages;
		uint32_t statisticCount = statisticCounts[i];
		char str[21];

		dystring_append(message, "\n");
		dystring_append(message, pipelineExecutablesProperties[i].name);
		dystring_append(message, "\n");

		for (uint32_t j = 0; j < sizeof(stages) * CHAR_BIT; j++) {
			VkShaderStageFlagBits stage = stages & (UINT32_C(1) << j);

			if (stage) {
				const char* sStage = string_VkShaderStageFlagBits(stage);
				dystring_append(message, sStage);
				dystring_append(message, " ");
			}
		}

		sprintf(str, "(%" PRIu32 ")\n",  pipelineExecutablesProperties[i].subgroupSize);

		dystring_append(message, str);
		dystring_append(message, pipelineExecutablesProperties[i].description);
		dystring_append(message, "\n");

		for (uint32_t j = 0; j < statisticCount; j++) {
			dystring_append(message, "\n\t");
			dystring_append(message, pipelineExecutablesStatistics[i][j].name);
			dystring_append(message, ": ");

			switch (pipelineExecutablesStatistics[i][j].format) {
				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
					strcpy(str, pipelineExecutablesStatistics[i][j].value.b32 ? "True" : "False");
					break;

				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
					sprintf(str, "%" PRId64, pipelineExecutablesStatistics[i][j].value.i64);
					break;

				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
					sprintf(str, "%" PRIu64, pipelineExecutablesStatistics[i][j].value.u64);
					break;

				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
					sprintf(str, "%g", pipelineExecutablesStatistics[i][j].value.f64);
					break;

				default:
					break;
			}

			dystring_append(message, str);
			dystring_append(message, "\n\t");
			dystring_append(message, pipelineExecutablesStatistics[i][j].description);
			dystring_append(message, "\n");
		}
	}

	const char* rawMessage = dystring_raw(message);

	bool bres = write_text(
		CAPTURE_FILE_NAME,
		"PIPELINE CAPTURE DATA\n"
		"\n"
		"Total # executables: %" PRIu32 "\n"
		"Total # statistics:  %" PRIu32 "\n"
		"\n"
		"%s",
		executableCount, statisticTotal, rawMessage);
	
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	free_recursive(dyMem);
	return true;
}

void* wait_for_input(void* ptr)
{
	atomic_bool* input = (atomic_bool*) ptr;

	puts("Calculating... press Enter/Return to stop\n");
	getchar();

	if (!atomic_load(input)) {
		puts("Stopping...\n");
		atomic_store(input, true);
	}

	return NULL;
}

void write_inbuffer(
	StartValue* restrict mappedInBuffer,
	StartValue* restrict firstStartValue,
	uint32_t valuesPerInout,
	uint32_t valuesPerHeap)
{
	ASSUME(*firstStartValue % 8 == 3);
	ASSUME(valuesPerInout % 128 == 0);
	ASSUME(valuesPerInout != 0);

	StartValue startValue = *firstStartValue;

	for (uint32_t i = 0; i < valuesPerInout; i++) {
		mappedInBuffer[i] = startValue;
		startValue += 4;
	}

	*firstStartValue += valuesPerHeap * 4;
}

void read_outbuffer(
	const StopTime* restrict mappedOutBuffer,
	Position* restrict position,
	DyArray bestStartValues,
	DyArray bestStopTimes,
	uint32_t valuesPerInout)
{
	ASSUME(position->curStartValue % 8 == 3);
	ASSUME(valuesPerInout % 128 == 0);
	ASSUME(valuesPerInout != 0);

	StartValue val0mod1off[3];
	StartValue val1mod6off[3];

	memcpy(val0mod1off, position->val0mod1off, sizeof(val0mod1off));
	memcpy(val1mod6off, position->val1mod6off, sizeof(val1mod6off));

	StartValue curValue = position->curStartValue - 2;
	StopTime bestTime = position->bestStopTime;

	for (uint32_t i = 0; i < valuesPerInout; i++) {
		curValue++; // curValue % 8 == 2

		if (curValue == val0mod1off[0] * 2) {
			StopTime newBestTime = bestTime + 1;
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);
		}
		else {
			for (uint32_t j = 2; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 1] || val0mod1off[j] * 2 != curValue) { continue; }

				val0mod1off[j - 1] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 3

		if (mappedOutBuffer[i] > bestTime) {
			StopTime newBestTime = mappedOutBuffer[i];
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);
		}
		else if (mappedOutBuffer[i] == bestTime && !val1mod6off[0] && curValue % 6 == 1) {
			val1mod6off[0] = curValue;
		}
		else {
			for (uint32_t j = 1; j < ARR_SIZE(val0mod1off); j++) {
				if (mappedOutBuffer[i] + j != bestTime) { continue; }

				if (!val0mod1off[j])                      { val0mod1off[j] = curValue; }
				if (!val1mod6off[j] && curValue % 6 == 1) { val1mod6off[j] = curValue; }

				break;
			}
		}

		curValue++; // curValue % 8 == 4

		if (curValue == val0mod1off[1] * 4) {
			StopTime newBestTime = bestTime + 1;
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);
		}
		else {
			for (uint32_t j = 3; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 2] || val0mod1off[j] * 4 != curValue) { continue; }

				val0mod1off[j - 2] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 5

		if (curValue % 6 == 1) {
			for (uint32_t j = 0; j < ARR_SIZE(val0mod1off); j++) {
				if (val1mod6off[j] || val0mod1off[j] + 1 != curValue) { continue; }

				val1mod6off[j] = curValue;
				break;
			}
		}

		i++;
		curValue++; // curValue % 8 == 6

		if (curValue == val0mod1off[0] * 2) {
			StopTime newBestTime = bestTime + 1;
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);
		}
		else {
			for (uint32_t j = 2; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 1] || val0mod1off[j] * 2 != curValue) { continue; }

				val0mod1off[j - 1] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 7

		if (mappedOutBuffer[i] > bestTime) {
			StopTime newBestTime = mappedOutBuffer[i];
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);
		}
		else if (mappedOutBuffer[i] == bestTime && !val1mod6off[0] && curValue % 6 == 1) {
			val1mod6off[0] = curValue;
		}
		else {
			for (uint32_t j = 1; j < ARR_SIZE(val0mod1off); j++) {
				if (mappedOutBuffer[i] + j != bestTime) { continue; }

				if (!val0mod1off[j])                      { val0mod1off[j] = curValue; }
				if (!val1mod6off[j] && curValue % 6 == 1) { val1mod6off[j] = curValue; }
			
				break;
			}
		}

		curValue++; // curValue % 8 == 0

		if (curValue == val0mod1off[2] * 8) {
			StopTime newBestTime = bestTime + 1;
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);
		}
		else {
			for (uint32_t j = 4; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 3] || val0mod1off[j] * 8 != curValue) { continue; }

				val0mod1off[j - 3] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 1

		for (uint32_t j = 0; j < ARR_SIZE(val0mod1off); j++) {
			if (!val1mod6off[j] || val1mod6off[j] * 4 != curValue * 3 + 1) { continue; }

			StopTime newBestTime = (StopTime) (bestTime + 3 - j);
			new_high(&curValue, &bestTime, newBestTime, val0mod1off, val1mod6off, bestStartValues, bestStopTimes);

			break;
		}
	}

	memcpy(position->val0mod1off, val0mod1off, sizeof(val0mod1off));
	memcpy(position->val1mod6off, val1mod6off, sizeof(val1mod6off));

	position->curStartValue = curValue + 2;
	position->bestStopTime = bestTime;
}

void new_high(
	const StartValue* restrict startValue,
	StopTime* restrict curBestTime,
	StopTime newBestTime,
	StartValue* restrict val0mod1off,
	StartValue* restrict val1mod6off,
	DyArray bestStartValues,
	DyArray bestStopTimes)
{
	uint32_t difTime = minu32((uint32_t) (newBestTime - *curBestTime), 3);
	*curBestTime = newBestTime;

	memmove(val0mod1off + difTime, val0mod1off, sizeof(StartValue) * (3 - difTime));
	memmove(val1mod6off + difTime, val1mod6off, sizeof(StartValue) * (3 - difTime));

	memset(val0mod1off + 1, 0, sizeof(StartValue) * (difTime - 1));
	memset(val1mod6off + 1, 0, sizeof(StartValue) * (difTime - 1));

	val0mod1off[0] = *startValue;
	val1mod6off[0] = *startValue % 6 == 1 ? *startValue : 0;

	dyarray_append(bestStartValues, startValue);
	dyarray_append(bestStopTimes, curBestTime);
}
