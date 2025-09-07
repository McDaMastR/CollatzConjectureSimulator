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
#include "dymemory.h"
#include "util.h"


bool create_instance(Gpu* restrict gpu)
{
	VkResult vkres;
	bool bres;

	/*
	 * Any memory dynamically allocated in this function is recorded to this object, and is all freed by destroying this
	 * object before the function returns.
	 */
	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	vkres = volkInitialize();
	if EXPECT_FALSE (vkres) { VKINIT_FAILURE(vkres); dyrecord_destroy(allocRecord); return false; }

	uint32_t instanceApiVersion = volkGetInstanceVersion();
	if EXPECT_FALSE (instanceApiVersion == VK_API_VERSION_1_0) {
		VKVERS_FAILURE(instanceApiVersion); dyrecord_destroy(allocRecord); return false; }

	if (g_config.logAllocations) {
		g_allocator = &g_allocationCallbacks;

		bres = init_alloc_logfile();
		if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }
	}

#ifndef NDEBUG
	bres = init_debug_logfile();
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }
#endif

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {0};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity =
		// VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugMessengerInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugMessengerInfo.pfnUserCallback = debug_callback;
	debugMessengerInfo.pUserData = &g_callbackData;

	uint32_t layerCount;
	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerCount, NULL);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	uint32_t extensionCount;
	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extensionCount, NULL);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	VkLayerProperties* layersProps = NULL;
	VkExtensionProperties* extensionsProps = NULL;

	if (layerCount) {
		size_t allocSize = layerCount * sizeof(VkLayerProperties);
		layersProps = dyrecord_malloc(allocRecord, allocSize);
		if EXPECT_FALSE (!layersProps) { dyrecord_destroy(allocRecord); return false; }

		VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerCount, layersProps);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	}

	if (extensionCount) {
		size_t allocSize = extensionCount * sizeof(VkExtensionProperties);
		extensionsProps = dyrecord_malloc(allocRecord, allocSize);
		if EXPECT_FALSE (!extensionsProps) { dyrecord_destroy(allocRecord); return false; }

		VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extensionCount, extensionsProps);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	}

	size_t elmSize = sizeof(const char*);
	size_t elmCount = 4;

	DyArray enabledLayers = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!enabledLayers) { dyrecord_destroy(allocRecord); return false; }

	bres = dyrecord_add(allocRecord, enabledLayers, dyarray_destroy_stub);
	if EXPECT_FALSE (!bres) { dyarray_destroy(enabledLayers); dyrecord_destroy(allocRecord); return false; }

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

	DyArray enabledExtensions = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!enabledExtensions) { dyrecord_destroy(allocRecord); return false; }

	bres = dyrecord_add(allocRecord, enabledExtensions, dyarray_destroy_stub);
	if EXPECT_FALSE (!bres) { dyarray_destroy(enabledExtensions); dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0; i < extensionCount; i++) {
		const char* extensionName = extensionsProps[i].extensionName;

		if (!strcmp(extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			(void) next; // Shut up warning for unused variable
			dyarray_append(enabledExtensions, &extensionName);
			usingPortabilityEnumeration = true;
		}

#ifndef NDEBUG
		else if (!strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			PNEXT_ADD(next, debugMessengerInfo);
			dyarray_append(enabledExtensions, &extensionName);
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

	uint32_t enabledExtensionCount = (uint32_t) dyarray_size(enabledExtensions);
	const char** enabledExtensionNames = dyarray_raw(enabledExtensions);

	VkInstanceCreateInfo instanceInfo = {0};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = nextChain;
	instanceInfo.flags = usingPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = enabledLayerCount;
	instanceInfo.ppEnabledLayerNames = enabledLayerNames;
	instanceInfo.enabledExtensionCount = enabledExtensionCount;
	instanceInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if (g_config.outputLevel > OUTPUT_LEVEL_DEFAULT) {
		printf("Enabled instance layers (%" PRIu32 "):\n", enabledLayerCount);
		for (uint32_t i = 0; i < enabledLayerCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledLayerNames[i]);
		}
		NEWLINE();

		printf("Enabled instance extensions (%" PRIu32 "):\n", enabledExtensionCount);
		for (uint32_t i = 0; i < enabledExtensionCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtensionNames[i]);
		}
		NEWLINE();
	}

	VkInstance instance;
	VK_CALL_RES(vkCreateInstance, &instanceInfo, g_allocator, &instance);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	volkLoadInstanceOnly(instance);

	if (usingDebugUtils) {
		VkDebugUtilsMessengerEXT messenger;
		VK_CALL_RES(vkCreateDebugUtilsMessengerEXT, instance, &debugMessengerInfo, g_allocator, &messenger);

		if (vkres == VK_SUCCESS) {
			gpu->debugMessenger = messenger;
		}
	}

	dyrecord_destroy(allocRecord);
	return true;
}

bool select_device(Gpu* restrict gpu)
{
	VkResult vkres;

	VkInstance instance = volkGetLoadedInstance();
	if EXPECT_FALSE (!instance) { return false; }

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	uint32_t deviceCount;
	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &deviceCount, NULL);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	if EXPECT_FALSE (!deviceCount) {
		log_critical(stderr, "No physical devices are accessible to the Vulkan instance");
		dyrecord_destroy(allocRecord);
		return false;
	}

	size_t allocCount = deviceCount;
	size_t allocSize =
		sizeof(VkPhysicalDevice) +
		sizeof(VkQueueFamilyProperties2*) +
		sizeof(VkExtensionProperties*) +
		sizeof(VkPhysicalDeviceMemoryProperties2) +
		sizeof(VkPhysicalDeviceProperties2) +
		sizeof(VkPhysicalDeviceFeatures2) +
		sizeof(VkPhysicalDevice16BitStorageFeatures) +
		sizeof(uint32_t) * 2;

	void* p = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!p) { dyrecord_destroy(allocRecord); return false; }

	VkPhysicalDevice* devices = p;

	VkQueueFamilyProperties2** queueFamiliesProps = (VkQueueFamilyProperties2**) (devices + deviceCount);
	VkExtensionProperties** extensionsProps = (VkExtensionProperties**) (queueFamiliesProps + deviceCount);

	VkPhysicalDeviceMemoryProperties2* devicesMemoryProps = (VkPhysicalDeviceMemoryProperties2*) (
		extensionsProps + deviceCount);
	VkPhysicalDeviceProperties2* devicesProps = (VkPhysicalDeviceProperties2*) (devicesMemoryProps + deviceCount);

	VkPhysicalDeviceFeatures2* devicesFeats = (VkPhysicalDeviceFeatures2*) (devicesProps + deviceCount);
	VkPhysicalDevice16BitStorageFeatures* devices16BitStorageFeats = (VkPhysicalDevice16BitStorageFeatures*) (
		devicesFeats + deviceCount);

	uint32_t* extensionCounts = (uint32_t*) (devices16BitStorageFeats + deviceCount);
	uint32_t* queueFamilyCounts = (uint32_t*) (extensionCounts + deviceCount);

	size_t queueFamilyTotal = 0;
	size_t extensionTotal = 0;

	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &deviceCount, devices);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];

		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, device, &queueFamilyCounts[i], NULL);

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, device, NULL, &extensionCounts[i], NULL);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

		queueFamilyTotal += queueFamilyCounts[i];
		extensionTotal += extensionCounts[i];
	}

	allocCount = queueFamilyTotal;
	allocSize = sizeof(VkQueueFamilyProperties2);

	queueFamiliesProps[0] = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!queueFamiliesProps[0]) { dyrecord_destroy(allocRecord); return false; }

	allocCount = extensionTotal;
	allocSize = sizeof(VkExtensionProperties);

	extensionsProps[0] = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!extensionsProps[0]) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 1; i < deviceCount; i++) {
		queueFamiliesProps[i] = queueFamiliesProps[i - 1] + queueFamilyCounts[i - 1];
		extensionsProps[i] = extensionsProps[i - 1] + extensionCounts[i - 1];
	}

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];
		uint32_t queueFamilyCount = queueFamilyCounts[i];

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, device, NULL, &extensionCounts[i], extensionsProps[i]);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

		for (uint32_t j = 0; j < queueFamilyCount; j++) {
			queueFamiliesProps[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, device, &queueFamilyCounts[i], queueFamiliesProps[i]);

		devicesMemoryProps[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		VK_CALL(vkGetPhysicalDeviceMemoryProperties2, device, &devicesMemoryProps[i]);

		devicesProps[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		VK_CALL(vkGetPhysicalDeviceProperties2, device, &devicesProps[i]);

		devices16BitStorageFeats[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;

		devicesFeats[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		devicesFeats[i].pNext = &devices16BitStorageFeats[i];

		VK_CALL(vkGetPhysicalDeviceFeatures2, device, &devicesFeats[i]);
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
		uint32_t extensionCount = extensionCounts[i];
		uint32_t queueFamilyCount = queueFamilyCounts[i];
		uint32_t memoryTypeCount = devicesMemoryProps[i].memoryProperties.memoryTypeCount;

		bool hasVulkan11 = devicesProps[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasVulkan12 = devicesProps[i].properties.apiVersion >= VK_API_VERSION_1_2;
		bool hasVulkan13 = devicesProps[i].properties.apiVersion >= VK_API_VERSION_1_3;
		bool hasVulkan14 = devicesProps[i].properties.apiVersion >= VK_API_VERSION_1_4;

		bool hasShaderInt16 = devicesFeats[i].features.shaderInt16;
		bool hasShaderInt64 = devicesFeats[i].features.shaderInt64;

		bool hasStorageBuffer16BitAccess = devices16BitStorageFeats[i].storageBuffer16BitAccess;

		bool hasCompute = false;
		bool hasDedicatedCompute = false;
		bool hasDedicatedTransfer = false;

		for (uint32_t j = 0; j < queueFamilyCount; j++) {
			VkQueueFlags queueFlags = queueFamiliesProps[i][j].queueFamilyProperties.queueFlags;

			bool isCompute = queueFlags & VK_QUEUE_COMPUTE_BIT;
			bool isGraphics = queueFlags & VK_QUEUE_GRAPHICS_BIT;
			bool isTransfer = queueFlags & VK_QUEUE_TRANSFER_BIT;
			bool isDedicatedCompute = isCompute && !isGraphics;
			bool isDedicatedTransfer = isTransfer && !isCompute && !isGraphics;

			if (isCompute)           { hasCompute = true; }
			if (isDedicatedCompute)  { hasDedicatedCompute = true; }
			if (isDedicatedTransfer) { hasDedicatedTransfer = true; }
		}

		bool hasDeviceNonHost = false;
		bool hasDeviceLocal = false;

		bool hasHostCachedNonCoherent = false;
		bool hasHostCached = false;
		bool hasHostNonCoherent = false;
		bool hasHostVisible = false;

		for (uint32_t j = 0; j < memoryTypeCount; j++) {
			VkMemoryPropertyFlags propFlags = devicesMemoryProps[i].memoryProperties.memoryTypes[j].propertyFlags;

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

		for (uint32_t j = 0; j < extensionCount; j++) {
			const char* extensionName = extensionsProps[i][j].extensionName;

			if (!strcmp(extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))      { hasMaintenance4 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_5_EXTENSION_NAME)) { hasMaintenance5 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_6_EXTENSION_NAME)) { hasMaintenance6 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_7_EXTENSION_NAME)) { hasMaintenance7 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_8_EXTENSION_NAME)) { hasMaintenance8 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_9_EXTENSION_NAME)) { hasMaintenance9 = true; }
			else if (!strcmp(extensionName, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME)) {
				hasPipelineExecutableProperties = true; }
			else if (!strcmp(extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) { hasPortabilitySubset = true; }
			else if (!strcmp(extensionName, VK_KHR_SPIRV_1_4_EXTENSION_NAME))          { hasSpirv14 = true; }
			else if (!strcmp(extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))  { hasSynchronization2 = true; }
			else if (!strcmp(extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) { hasTimelineSemaphore = true; }
			else if (!strcmp(extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))      { hasMemoryBudget = true; }
			else if (!strcmp(extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))    { hasMemoryPriority = true; }
			else if (!strcmp(extensionName, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME)) {
				hasPipelineCreationCacheControl = true; }
			else if (!strcmp(extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) {
				hasSubgroupSizeControl = true; }
		}

		uint32_t currentScore = 1;

		if (!hasVulkan11) { continue; }

		if (!hasDeviceLocal) { continue; }
		if (!hasHostVisible) { continue; }

		if (!hasCompute) { continue; }

		if (!hasSynchronization2)  { continue; }
		if (!hasTimelineSemaphore) { continue; }

		if (hasVulkan12) { currentScore += 50; }
		if (hasVulkan13) { currentScore += 50; }
		if (hasVulkan14) { currentScore += 50; }

		if (g_config.preferInt16 && hasShaderInt16) { currentScore += 1000; }
		if (g_config.preferInt64 && hasShaderInt64) { currentScore += 1000; }

		if (hasDeviceNonHost) { currentScore += 50; }

		if (hasHostCachedNonCoherent) { currentScore += 1000; }
		else if (hasHostCached)       { currentScore += 500; }
		else if (hasHostNonCoherent)  { currentScore += 100; }

		if (hasDedicatedCompute)  { currentScore += 100; }
		if (hasDedicatedTransfer) { currentScore += 100; }

		if (hasMaintenance4)                 { currentScore += 10; }
		if (hasMaintenance5)                 { currentScore += 10; }
		if (hasMaintenance6)                 { currentScore += 10; }
		if (hasMaintenance7)                 { currentScore += 10; }
		if (hasMaintenance8)                 { currentScore += 10; }
		if (hasMaintenance9)                 { currentScore += 10; }
		if (hasMemoryBudget)                 { currentScore += 10; }
		if (hasMemoryPriority)               { currentScore += 10; }
		if (hasPipelineCreationCacheControl) { currentScore += 10; }
		if (hasSpirv14)                      { currentScore += 10; }
		if (hasStorageBuffer16BitAccess)     { currentScore += 100; }
		if (hasSubgroupSizeControl)          { currentScore += 10; }

		if (g_config.capturePipelines && hasPipelineExecutableProperties) { currentScore += 10; }

		if (currentScore > bestScore) {
			bestScore = currentScore;
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

		dyrecord_destroy(allocRecord);
		return false;
	}

	const char* deviceName = devicesProps[deviceIndex].properties.deviceName;
	uint32_t queueFamilyCount = queueFamilyCounts[deviceIndex];

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

	uint32_t computeQueueFamilyIndex = 0;
	uint32_t transferQueueFamilyIndex = 0;
	uint32_t computeQueueIndex = 0;
	uint32_t transferQueueIndex = 0;

	bool hasCompute = false;
	bool hasTransfer = false;
	bool hasDedicatedCompute = false;
	bool hasDedicatedTransfer = false;

	// TODO implement algorithm to choose queue indices too
	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkQueueFlags queueFlags = queueFamiliesProps[deviceIndex][i].queueFamilyProperties.queueFlags;

		bool isCompute = queueFlags & VK_QUEUE_COMPUTE_BIT;
		bool isGraphics = queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool isTransfer = queueFlags & VK_QUEUE_TRANSFER_BIT;
		bool isDedicatedCompute = isCompute && !isGraphics;
		bool isDedicatedTransfer = isTransfer && !isCompute && !isGraphics;

		if (isCompute) {
			if (!hasDedicatedCompute && isDedicatedCompute) {
				computeQueueFamilyIndex = i;
				hasDedicatedCompute = true;
				hasCompute = true;
			}
			else if (!hasCompute) {
				computeQueueFamilyIndex = i;
				hasCompute = true;
			}
		}

		if (isTransfer) {
			if (!hasDedicatedTransfer && isDedicatedTransfer) {
				transferQueueFamilyIndex = i;
				hasDedicatedTransfer = true;
				hasTransfer = true;
			}
			else if (!hasTransfer) {
				transferQueueFamilyIndex = i;
				hasTransfer = true;
			}
		}
	}

	if (!hasTransfer) {
		transferQueueFamilyIndex = computeQueueFamilyIndex;
	}

	if (computeQueueFamilyIndex == transferQueueFamilyIndex) {
		uint32_t queueFamilyIndex = computeQueueFamilyIndex;
		uint32_t queueCount = queueFamiliesProps[deviceIndex][queueFamilyIndex].queueFamilyProperties.queueCount;

		if (queueCount > 1) {
			computeQueueIndex = 0;
			transferQueueIndex = 1;
		}
	}

	gpu->physicalDevice = devices[deviceIndex];

	gpu->computeQueueFamilyIndex = computeQueueFamilyIndex;
	gpu->transferQueueFamilyIndex = transferQueueFamilyIndex;
	gpu->computeQueueIndex = computeQueueIndex;
	gpu->transferQueueIndex = transferQueueIndex;

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
		gpu->computeQueueFamilyTimestampValidBits =
			queueFamiliesProps[deviceIndex][computeQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
		gpu->transferQueueFamilyTimestampValidBits =
			queueFamiliesProps[deviceIndex][transferQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
		gpu->timestampPeriod = devicesProps[deviceIndex].properties.limits.timestampPeriod;
	}

	switch (g_config.outputLevel) {
		case OUTPUT_LEVEL_DEFAULT:
			printf(
				"Device: %s\n"
				"\tVulkan version:    %" PRIu32 ".%" PRIu32 "\n"
				"\tSPIR-V version:    %" PRIu32 ".%" PRIu32 "\n"
				"\tCompute QF index:  %" PRIu32 "\n"
				"\tTransfer QF index: %" PRIu32 "\n",
				deviceName, vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor,
				computeQueueFamilyIndex, transferQueueFamilyIndex);

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
				"\tCompute queue family index:        %" PRIu32 "\n"
				"\tTransfer queue family index:       %" PRIu32 "\n"
				"\tCompute queue index:               %" PRIu32 "\n"
				"\tTransfer queue index:              %" PRIu32 "\n"
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
				deviceName, bestScore, vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor,
				computeQueueFamilyIndex, transferQueueFamilyIndex, computeQueueIndex, transferQueueIndex,
				usingMaintenance4, usingMaintenance5, usingMaintenance6, usingMaintenance7, usingMaintenance8,
				usingMaintenance9, usingMemoryPriority, usingPipelineCreationCacheControl,
				usingPipelineExecutableProperties, usingShaderInt16, usingShaderInt64, using16BitStorage,
				usingSubgroupSizeControl);

			break;

		default:
			break;
	}

	dyrecord_destroy(allocRecord);
	return true;
}

bool create_device(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;

	uint32_t computeQueueFamilyIndex = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;
	uint32_t computeQueueIndex = gpu->computeQueueIndex;
	uint32_t transferQueueIndex = gpu->transferQueueIndex;

	uint32_t spvVerMinor = gpu->spvVerMinor;
	
	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	size_t elmSize = sizeof(const char*);
	size_t elmCount = 19;

	DyArray enabledExtensions = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!enabledExtensions) { dyrecord_destroy(allocRecord); return false; }

	bool bres = dyrecord_add(allocRecord, enabledExtensions, dyarray_destroy_stub);
	if EXPECT_FALSE (!bres) { dyarray_destroy(enabledExtensions); dyrecord_destroy(allocRecord); return false; }

	// Required extensions
	const char* extensionName = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
	dyarray_append(enabledExtensions, &extensionName);

	extensionName = VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME;
	dyarray_append(enabledExtensions, &extensionName);

	// Optional KHR extensions
	if (gpu->usingMaintenance4) {
		extensionName = VK_KHR_MAINTENANCE_4_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingMaintenance5) {
		extensionName = VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);

		extensionName = VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);

		extensionName = VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);

		extensionName = VK_KHR_MAINTENANCE_5_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingMaintenance6) {
		extensionName = VK_KHR_MAINTENANCE_6_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingMaintenance7) {
		extensionName = VK_KHR_MAINTENANCE_7_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingMaintenance8) {
		extensionName = VK_KHR_MAINTENANCE_8_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingMaintenance9) {
		extensionName = VK_KHR_MAINTENANCE_9_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingPipelineExecutableProperties) {
		extensionName = VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingPortabilitySubset) {
		extensionName = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (spvVerMinor >= 4) {
		extensionName = VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);

		extensionName = VK_KHR_SPIRV_1_4_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	// Optional EXT extensions
	if (gpu->usingMemoryBudget) {
		extensionName = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingMemoryPriority) {
		extensionName = VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingPipelineCreationCacheControl) {
		extensionName = VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}
	if (gpu->usingSubgroupSizeControl) {
		extensionName = VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME;
		dyarray_append(enabledExtensions, &extensionName);
	}

	VkPhysicalDeviceFeatures2 deviceFeats = {0};
	deviceFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeats.features.shaderInt64 = gpu->usingShaderInt64;
	deviceFeats.features.shaderInt16 = gpu->usingShaderInt16;

	VkPhysicalDevice16BitStorageFeatures device16BitStorageFeats = {0};
	device16BitStorageFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
	device16BitStorageFeats.storageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeatures deviceDynamicRenderingFeats = {0};
	deviceDynamicRenderingFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	deviceDynamicRenderingFeats.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceMaintenance4Features deviceMaintenance4Feats = {0};
	deviceMaintenance4Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
	deviceMaintenance4Feats.maintenance4 = VK_TRUE;

	VkPhysicalDeviceMaintenance5Features deviceMaintenance5Feats = {0};
	deviceMaintenance5Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
	deviceMaintenance5Feats.maintenance5 = VK_TRUE;

	VkPhysicalDeviceMaintenance6Features deviceMaintenance6Feats = {0};
	deviceMaintenance6Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES;
	deviceMaintenance6Feats.maintenance6 = VK_TRUE;

	VkPhysicalDeviceMaintenance7FeaturesKHR deviceMaintenance7Feats = {0};
	deviceMaintenance7Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR;
	deviceMaintenance7Feats.maintenance7 = VK_TRUE;

	VkPhysicalDeviceMaintenance8FeaturesKHR deviceMaintenance8Feats = {0};
	deviceMaintenance8Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_8_FEATURES_KHR;
	deviceMaintenance8Feats.maintenance8 = VK_TRUE;

	VkPhysicalDeviceMaintenance9FeaturesKHR deviceMaintenance9Feats = {0};
	deviceMaintenance9Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR;
	deviceMaintenance9Feats.maintenance9 = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT deviceMemoryPriorityFeats = {0};
	deviceMemoryPriorityFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
	deviceMemoryPriorityFeats.memoryPriority = VK_TRUE;

	VkPhysicalDevicePipelineCreationCacheControlFeatures devicePipelineCreationCacheControlFeats = {0};
	devicePipelineCreationCacheControlFeats.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES;
	devicePipelineCreationCacheControlFeats.pipelineCreationCacheControl = VK_TRUE;

	VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR devicePipelineExecutablePropertiesFeats = {0};
	devicePipelineExecutablePropertiesFeats.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
	devicePipelineExecutablePropertiesFeats.pipelineExecutableInfo = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeatures deviceSubgroupSizeControlFeats = {0};
	deviceSubgroupSizeControlFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
	deviceSubgroupSizeControlFeats.subgroupSizeControl = VK_TRUE;

	VkPhysicalDeviceSynchronization2Features deviceSynchronization2Feats = {0};
	deviceSynchronization2Feats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	deviceSynchronization2Feats.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeatures deviceTimelineSemaphoreFeats = {0};
	deviceTimelineSemaphoreFeats.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	deviceTimelineSemaphoreFeats.timelineSemaphore = VK_TRUE;

	void** next = &deviceFeats.pNext;

	PNEXT_ADD(next, deviceSynchronization2Feats);
	PNEXT_ADD(next, deviceTimelineSemaphoreFeats);

	if (gpu->using16BitStorage) {
		PNEXT_ADD(next, device16BitStorageFeats);
	}
	if (gpu->usingMaintenance4) {
		PNEXT_ADD(next, deviceMaintenance4Feats);
	}
	if (gpu->usingMaintenance5) {
		PNEXT_ADD(next, deviceDynamicRenderingFeats);
		PNEXT_ADD(next, deviceMaintenance5Feats);
	}
	if (gpu->usingMaintenance6) {
		PNEXT_ADD(next, deviceMaintenance6Feats);
	}
	if (gpu->usingMaintenance7) {
		PNEXT_ADD(next, deviceMaintenance7Feats);
	}
	if (gpu->usingMaintenance8) {
		PNEXT_ADD(next, deviceMaintenance8Feats);
	}
	if (gpu->usingMaintenance9) {
		PNEXT_ADD(next, deviceMaintenance9Feats);
	}
	if (gpu->usingMemoryPriority) {
		PNEXT_ADD(next, deviceMemoryPriorityFeats);
	}
	if (gpu->usingPipelineCreationCacheControl) {
		PNEXT_ADD(next, devicePipelineCreationCacheControlFeats);
	}
	if (gpu->usingPipelineExecutableProperties) {
		PNEXT_ADD(next, devicePipelineExecutablePropertiesFeats);
	}
	if (gpu->usingSubgroupSizeControl) {
		PNEXT_ADD(next, deviceSubgroupSizeControlFeats);
	}

	float computeQueuePriorities[2];
	computeQueuePriorities[0] = 1;
	computeQueuePriorities[1] = 0;

	float transferQueuePriorities[1];
	transferQueuePriorities[0] = computeQueuePriorities[1];

	VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].queueFamilyIndex = computeQueueFamilyIndex;
	queueCreateInfos[0].queueCount =
		computeQueueFamilyIndex != transferQueueFamilyIndex || computeQueueIndex == transferQueueIndex ? 1 : 2;
	queueCreateInfos[0].pQueuePriorities = computeQueuePriorities;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = transferQueueFamilyIndex;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = transferQueuePriorities;

	uint32_t enabledExtensionCount = (uint32_t) dyarray_size(enabledExtensions);
	const char** enabledExtensionNames = dyarray_raw(enabledExtensions);

	VkDeviceCreateInfo deviceInfo = {0};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = &deviceFeats;
	deviceInfo.queueCreateInfoCount = computeQueueFamilyIndex == transferQueueFamilyIndex ? 1 : 2;
	deviceInfo.pQueueCreateInfos = queueCreateInfos;
	deviceInfo.enabledExtensionCount = enabledExtensionCount;
	deviceInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if (g_config.outputLevel > OUTPUT_LEVEL_DEFAULT) {
		printf("Enabled device extensions (%" PRIu32 "):\n", enabledExtensionCount);
		for (uint32_t i = 0; i < enabledExtensionCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtensionNames[i]);
		}
		NEWLINE();
	}

	VkDevice device;
	VK_CALL_RES(vkCreateDevice, physicalDevice, &deviceInfo, g_allocator, &device);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	gpu->device = device;

	volkLoadDevice(device);

	VkDeviceQueueInfo2 computeQueueInfo = {0};
	computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	computeQueueInfo.queueFamilyIndex = computeQueueFamilyIndex;
	computeQueueInfo.queueIndex = computeQueueIndex;

	VK_CALL(vkGetDeviceQueue2, device, &computeQueueInfo, &gpu->computeQueue);

	VkDeviceQueueInfo2 transferQueueInfo = {0};
	transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	transferQueueInfo.queueFamilyIndex = transferQueueFamilyIndex;
	transferQueueInfo.queueIndex = transferQueueIndex;

	VK_CALL(vkGetDeviceQueue2, device, &transferQueueInfo, &gpu->transferQueue);

	dyrecord_destroy(allocRecord);
	return true;
}

bool manage_memory(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;
	VkDevice device = gpu->device;

	bool (*get_buffer_requirements)(VkDevice, VkDeviceSize, VkBufferUsageFlags, VkMemoryRequirements*) =
		gpu->usingMaintenance4 ? get_buffer_requirements_main4 : get_buffer_requirements_noext;

	VkPhysicalDeviceMaintenance4Properties deviceMaintenance4Props = {0};
	deviceMaintenance4Props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;

	VkPhysicalDeviceMaintenance3Properties deviceMaintenance3Props = {0};
	deviceMaintenance3Props.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
	deviceMaintenance3Props.pNext = gpu->usingMaintenance4 ? &deviceMaintenance4Props : NULL;

	VkPhysicalDeviceProperties2 deviceProps = {0};
	deviceProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProps.pNext = &deviceMaintenance3Props;

	VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevice, &deviceProps);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT deviceMemoryBudgetProps = {0};
	deviceMemoryBudgetProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

	VkPhysicalDeviceMemoryProperties2 deviceMemoryProps = {0};
	deviceMemoryProps.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	deviceMemoryProps.pNext = gpu->usingMemoryBudget ? &deviceMemoryBudgetProps : NULL;

	VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevice, &deviceMemoryProps);

	VkDeviceSize maxMemorySize = deviceMaintenance3Props.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize = gpu->usingMaintenance4 ? deviceMaintenance4Props.maxBufferSize : maxMemorySize;

	uint32_t maxStorageBufferRange = deviceProps.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryCount = deviceProps.properties.limits.maxMemoryAllocationCount;
	uint32_t maxWorkgroupCount = deviceProps.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxWorkgroupSize = deviceProps.properties.limits.maxComputeWorkGroupSize[0];
	uint32_t memoryTypeCount = deviceMemoryProps.memoryProperties.memoryTypeCount;

	VkBufferUsageFlags hostVisibleBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	VkMemoryRequirements hostVisibleMemoryRequirements;

	bool bres = get_buffer_requirements(device, sizeof(char), hostVisibleBufferUsage, &hostVisibleMemoryRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	VkBufferUsageFlags deviceLocalBufferUsage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	VkMemoryRequirements deviceLocalMemoryRequirements;

	bres = get_buffer_requirements(device, sizeof(char), deviceLocalBufferUsage, &deviceLocalMemoryRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	uint32_t deviceLocalMemoryTypeBits = deviceLocalMemoryRequirements.memoryTypeBits;
	uint32_t hostVisibleMemoryTypeBits = hostVisibleMemoryRequirements.memoryTypeBits;

	uint32_t deviceLocalHeapIndex = 0;
	uint32_t hostVisibleHeapIndex = 0;
	uint32_t deviceLocalTypeIndex = 0;
	uint32_t hostVisibleTypeIndex = 0;

	bool hasDeviceNonHost = false;
	bool hasDeviceLocal = false;

	bool hasHostCachedNonCoherent = false;
	bool hasHostCached = false;
	bool hasHostNonCoherent = false;
	bool hasHostVisible = false;

	for (uint32_t i = 0; i < memoryTypeCount; i++) {
		uint32_t memoryTypeBit = UINT32_C(1) << i;
		uint32_t heapIndex = deviceMemoryProps.memoryProperties.memoryTypes[i].heapIndex;
		VkMemoryPropertyFlags propFlags = deviceMemoryProps.memoryProperties.memoryTypes[i].propertyFlags;

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

		if (isDeviceLocal && (deviceLocalMemoryTypeBits & memoryTypeBit)) {
			if (!hasDeviceNonHost && !isHostVisible) {
				deviceLocalHeapIndex = heapIndex;
				deviceLocalTypeIndex = i;
				hasDeviceNonHost = true;
				hasDeviceLocal = true;
			}
			else if (!hasDeviceLocal) {
				deviceLocalHeapIndex = heapIndex;
				deviceLocalTypeIndex = i;
				hasDeviceLocal = true;
			}
		}

		if (isHostVisible && (hostVisibleMemoryTypeBits & memoryTypeBit)) {
			if (!hasHostCachedNonCoherent && isHostCached && !isHostCoherent) {
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
				hasHostCachedNonCoherent = true;
				hasHostCached = true;
				hasHostNonCoherent = true;
				hasHostVisible = true;
			}
			else if (!hasHostCached && isHostCached) {
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
				hasHostCached = true;
				hasHostNonCoherent = false;
				hasHostVisible = true;
			}
			else if (!hasHostCached && !hasHostNonCoherent && !isHostCoherent) {
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
				hasHostNonCoherent = true;
				hasHostVisible = true;
			}
			else if (!hasHostVisible) {
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
				hasHostVisible = true;
			}
		}
	}

	VkDeviceSize hostVisibleHeapBudget = deviceMemoryBudgetProps.heapBudget[hostVisibleHeapIndex];
	VkDeviceSize deviceLocalHeapBudget = deviceMemoryBudgetProps.heapBudget[deviceLocalHeapIndex];

	VkDeviceSize hostVisibleHeapSize = deviceMemoryProps.memoryProperties.memoryHeaps[hostVisibleHeapIndex].size;
	VkDeviceSize deviceLocalHeapSize = deviceMemoryProps.memoryProperties.memoryHeaps[deviceLocalHeapIndex].size;

	VkDeviceSize bytesPerHostVisibleHeap = gpu->usingMemoryBudget ? hostVisibleHeapBudget : hostVisibleHeapSize;
	VkDeviceSize bytesPerDeviceLocalHeap = gpu->usingMemoryBudget ? deviceLocalHeapBudget : deviceLocalHeapSize;

	VkDeviceSize bytesPerHeap = minu64(bytesPerHostVisibleHeap, bytesPerDeviceLocalHeap);
	bytesPerHeap = (VkDeviceSize) ((float) bytesPerHeap * g_config.maxMemory); // User-given limit on heap memory

	if (deviceLocalHeapIndex == hostVisibleHeapIndex) {
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

	bres = get_buffer_requirements(device, bytesPerBuffer, hostVisibleBufferUsage, &hostVisibleMemoryRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	bres = get_buffer_requirements(device, bytesPerBuffer, deviceLocalBufferUsage, &deviceLocalMemoryRequirements);
	if EXPECT_FALSE (!bres) { return false; }

	VkDeviceSize bytesPerHostVisibleMemory = hostVisibleMemoryRequirements.size;
	VkDeviceSize bytesPerDeviceLocalMemory = deviceLocalMemoryRequirements.size;

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
	gpu->bytesPerHostVisibleMemory = bytesPerHostVisibleMemory;
	gpu->bytesPerDeviceLocalMemory = bytesPerDeviceLocalMemory;

	gpu->valuesPerInout = valuesPerInout;
	gpu->valuesPerBuffer = valuesPerBuffer;
	gpu->valuesPerHeap = valuesPerHeap;
	gpu->inoutsPerBuffer = inoutsPerBuffer;
	gpu->inoutsPerHeap = inoutsPerHeap;
	gpu->buffersPerHeap = buffersPerHeap;

	gpu->workgroupSize = workgroupSize;
	gpu->workgroupCount = workgroupCount;

	gpu->hostVisibleHeapIndex = hostVisibleHeapIndex;
	gpu->deviceLocalHeapIndex = deviceLocalHeapIndex;
	gpu->hostVisibleTypeIndex = hostVisibleTypeIndex;
	gpu->deviceLocalTypeIndex = deviceLocalTypeIndex;

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
				hostVisibleTypeIndex, deviceLocalTypeIndex,
				workgroupSize, workgroupCount, valuesPerInout, inoutsPerHeap);

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
				hasHostNonCoherent, hostVisibleHeapIndex, deviceLocalHeapIndex, hostVisibleTypeIndex,
				deviceLocalTypeIndex, workgroupSize, workgroupCount, valuesPerInout, inoutsPerBuffer, buffersPerHeap,
				valuesPerHeap);

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

	gpu->computeCommandBuffers = (VkCommandBuffer*) (gpu->descriptorSets + inoutsPerHeap);
	gpu->transferCommandBuffers = (VkCommandBuffer*) (gpu->computeCommandBuffers + inoutsPerHeap);

	gpu->semaphores = (VkSemaphore*) (gpu->transferCommandBuffers + inoutsPerHeap);

	return true;
}

bool create_buffers(Gpu* restrict gpu)
{
	VkBuffer* hostVisibleBuffers = gpu->hostVisibleBuffers;
	VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;
	VkDeviceMemory* hostVisibleMemories = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* deviceLocalMemories = gpu->deviceLocalDeviceMemories;
	StartValue** mappedInBuffers = gpu->mappedInBuffers;
	StopTime** mappedOutBuffers = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerInout = gpu->bytesPerInout;
	VkDeviceSize bytesPerBuffer = gpu->bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleMemory = gpu->bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDeviceLocalMemory = gpu->bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout = gpu->valuesPerInout;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;
	uint32_t hostVisibleTypeIndex = gpu->hostVisibleTypeIndex;
	uint32_t deviceLocalTypeIndex = gpu->deviceLocalTypeIndex;

	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	size_t allocCount = buffersPerHeap;
	size_t allocSize =
		sizeof(VkMemoryAllocateInfo) * 2 +
		sizeof(VkMemoryDedicatedAllocateInfo) * 2 +
		sizeof(VkBindBufferMemoryInfo) * 2;

	void* p = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!p) { dyrecord_destroy(allocRecord); return false; }

	VkMemoryAllocateInfo* hostVisibleAllocInfos = p;
	VkMemoryAllocateInfo* deviceLocalAllocInfos = (VkMemoryAllocateInfo*) (hostVisibleAllocInfos + buffersPerHeap);

	VkMemoryDedicatedAllocateInfo* hostVisibleDedicatedInfos = (VkMemoryDedicatedAllocateInfo*) (
		deviceLocalAllocInfos + buffersPerHeap);
	VkMemoryDedicatedAllocateInfo* deviceLocalDedicatedInfos = (VkMemoryDedicatedAllocateInfo*) (
		hostVisibleDedicatedInfos + buffersPerHeap);

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) (
		deviceLocalDedicatedInfos + buffersPerHeap);

	VkBufferCreateInfo hostVisibleBufferInfo = {0};
	hostVisibleBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hostVisibleBufferInfo.size = bytesPerBuffer;
	hostVisibleBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hostVisibleBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo deviceLocalBufferInfo = {0};
	deviceLocalBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	deviceLocalBufferInfo.size = bytesPerBuffer;
	deviceLocalBufferInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	deviceLocalBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkBuffer hostVisibleBuffer;
		VK_CALL_RES(vkCreateBuffer, device, &hostVisibleBufferInfo, g_allocator, &hostVisibleBuffer);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		hostVisibleBuffers[i] = hostVisibleBuffer;

		VkBuffer deviceLocalBuffer;
		VK_CALL_RES(vkCreateBuffer, device, &deviceLocalBufferInfo, g_allocator, &deviceLocalBuffer);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		deviceLocalBuffers[i] = deviceLocalBuffer;
	}

	VkMemoryPriorityAllocateInfoEXT hostVisiblePriorityInfo = {0};
	hostVisiblePriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	hostVisiblePriorityInfo.priority = 0;

	VkMemoryPriorityAllocateInfoEXT deviceLocalPriorityInfo = {0};
	deviceLocalPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	deviceLocalPriorityInfo.priority = 1;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		hostVisibleDedicatedInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		hostVisibleDedicatedInfos[i].pNext = gpu->usingMemoryPriority ? &hostVisiblePriorityInfo : NULL;
		hostVisibleDedicatedInfos[i].buffer = hostVisibleBuffers[i];

		deviceLocalDedicatedInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		deviceLocalDedicatedInfos[i].pNext = gpu->usingMemoryPriority ? &deviceLocalPriorityInfo : NULL;
		deviceLocalDedicatedInfos[i].buffer = deviceLocalBuffers[i];

		hostVisibleAllocInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		hostVisibleAllocInfos[i].pNext = &hostVisibleDedicatedInfos[i];
		hostVisibleAllocInfos[i].allocationSize = bytesPerHostVisibleMemory;
		hostVisibleAllocInfos[i].memoryTypeIndex = hostVisibleTypeIndex;

		deviceLocalAllocInfos[i].sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		deviceLocalAllocInfos[i].pNext = &deviceLocalDedicatedInfos[i];
		deviceLocalAllocInfos[i].allocationSize = bytesPerDeviceLocalMemory;
		deviceLocalAllocInfos[i].memoryTypeIndex = deviceLocalTypeIndex;
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkDeviceMemory hostVisibleMemory;
		VK_CALL_RES(vkAllocateMemory, device, &hostVisibleAllocInfos[i], g_allocator, &hostVisibleMemory);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		hostVisibleMemories[i] = hostVisibleMemory;

		VkDeviceMemory deviceLocalMemory;
		VK_CALL_RES(vkAllocateMemory, device, &deviceLocalAllocInfos[i], g_allocator, &deviceLocalMemory);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		deviceLocalMemories[i] = deviceLocalMemory;
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		bindBufferMemoryInfos[i][0].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
		bindBufferMemoryInfos[i][0].buffer = hostVisibleBuffers[i];
		bindBufferMemoryInfos[i][0].memory = hostVisibleMemories[i];

		bindBufferMemoryInfos[i][1].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
		bindBufferMemoryInfos[i][1].buffer = deviceLocalBuffers[i];
		bindBufferMemoryInfos[i][1].memory = deviceLocalMemories[i];
	}

	uint32_t bindInfoCount = buffersPerHeap * ARRAY_SIZE(bindBufferMemoryInfos[0]);
	VkBindBufferMemoryInfo* bindInfos = (VkBindBufferMemoryInfo*) bindBufferMemoryInfos;

	VK_CALL_RES(vkBindBufferMemory2, device, bindInfoCount, bindInfos);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkDeviceSize offset = 0;
		VkMemoryMapFlags flags = 0;
		void* mappedMemory;

		VK_CALL_RES(vkMapMemory,
			device, hostVisibleMemories[i], offset, bytesPerHostVisibleMemory, flags, &mappedMemory);

		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

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

		set_debug_name(device, VK_OBJECT_TYPE_BUFFER, (uint64_t) hostVisibleBuffers[i], objectName);
		set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) hostVisibleMemories[i], objectName);

		strcpy(objectName, "Device local");
		objectName[12] = ' '; // Remove '\0' from strcpy

		set_debug_name(device, VK_OBJECT_TYPE_BUFFER, (uint64_t) deviceLocalBuffers[i], objectName);
		set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) deviceLocalMemories[i], objectName);
	}
#endif

	dyrecord_destroy(allocRecord);
	return true;
}

bool create_descriptors(Gpu* restrict gpu)
{
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;
	VkDescriptorSet* descriptorSets = gpu->descriptorSets;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	size_t allocCount = inoutsPerHeap;
	size_t allocSize =
		sizeof(VkDescriptorSetLayout) +
		sizeof(VkWriteDescriptorSet) +
		sizeof(VkDescriptorBufferInfo) * 2;

	void* p = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!p) { dyrecord_destroy(allocRecord); return false; }

	VkDescriptorSetLayout* descriptorSetLayouts = p;
	VkWriteDescriptorSet* writeDescriptorSets = (VkWriteDescriptorSet*) (descriptorSetLayouts + inoutsPerHeap);
	VkDescriptorBufferInfo (*descriptorBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (
		writeDescriptorSets + inoutsPerHeap);

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2] = {0};
	descriptorSetLayoutBindings[0].binding = 0;
	descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[0].descriptorCount = 1;
	descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	descriptorSetLayoutBindings[1].binding = 1;
	descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[1].descriptorCount = 1;
	descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo = {0};
	descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutInfo.bindingCount = ARRAY_SIZE(descriptorSetLayoutBindings);
	descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings;

	VkDescriptorSetLayout descriptorSetLayout;
	VK_CALL_RES(vkCreateDescriptorSetLayout, device, &descriptorSetLayoutInfo, g_allocator, &descriptorSetLayout);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	gpu->descriptorSetLayout = descriptorSetLayout;

	VkDescriptorPoolSize descriptorPoolSizes[1];
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[0].descriptorCount = inoutsPerHeap * 2;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {0};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.maxSets = inoutsPerHeap;
	descriptorPoolInfo.poolSizeCount = ARRAY_SIZE(descriptorPoolSizes);
	descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;

	VkDescriptorPool descriptorPool;
	VK_CALL_RES(vkCreateDescriptorPool, device, &descriptorPoolInfo, g_allocator, &descriptorPool);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	gpu->descriptorPool = descriptorPool;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		descriptorSetLayouts[i] = descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {0};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = inoutsPerHeap;
	descriptorSetAllocInfo.pSetLayouts = descriptorSetLayouts;

	VK_CALL_RES(vkAllocateDescriptorSets, device, &descriptorSetAllocInfo, descriptorSets);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			descriptorBufferInfos[j][0].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[j][0].offset = bytesPerInout * k;
			descriptorBufferInfos[j][0].range = bytesPerIn;

			descriptorBufferInfos[j][1].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[j][1].offset = bytesPerInout * k + bytesPerIn;
			descriptorBufferInfos[j][1].range = bytesPerOut;

			writeDescriptorSets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[j].dstSet = descriptorSets[j];
			writeDescriptorSets[j].dstBinding = 0;
			writeDescriptorSets[j].dstArrayElement = 0;
			writeDescriptorSets[j].descriptorCount = ARRAY_SIZE(descriptorBufferInfos[j]);
			writeDescriptorSets[j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[j].pBufferInfo = descriptorBufferInfos[j];
		}
	}

	uint32_t writeDescriptorCount = inoutsPerHeap;
	uint32_t copyDescriptorCount = 0;
	VkCopyDescriptorSet* copyDescriptorSets = NULL;

	VK_CALL(vkUpdateDescriptorSets,
		device, writeDescriptorCount, writeDescriptorSets, copyDescriptorCount, copyDescriptorSets);

#ifndef NDEBUG
	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			char objectName[58];
			sprintf(
				objectName,
				"Inout %" PRIu32 "/%" PRIu32 ", Buffer %" PRIu32 "/%" PRIu32,
				k + 1, inoutsPerBuffer, i + 1, buffersPerHeap);

			set_debug_name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptorSets[j], objectName);
		}
	}
#endif

	dyrecord_destroy(allocRecord);
	return true;
}

bool create_pipeline(Gpu* restrict gpu)
{
	VkDevice device = gpu->device;

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t workgroupSize = gpu->workgroupSize;

	uint32_t computeQueueFamilyTimestampValidBits = gpu->computeQueueFamilyTimestampValidBits;
	uint32_t transferQueueFamilyTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;

	uint32_t spvVerMajor = gpu->spvVerMajor;
	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

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
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	if EXPECT_FALSE (!shaderSize) {
		log_critical(stderr, "Selected shader '%s' not found", shaderName);
		dyrecord_destroy(allocRecord);
		return false;
	}

	uint32_t* shaderCode = dyrecord_malloc(allocRecord, shaderSize);
	if EXPECT_FALSE (!shaderCode) { dyrecord_destroy(allocRecord); return false; }

	bres = read_file(shaderName, shaderCode, shaderSize);
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	size_t cacheSize;
	bres = file_size(PIPELINE_CACHE_NAME, &cacheSize);
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	void* cacheData = NULL;

	if (cacheSize) {
		cacheData = dyrecord_malloc(allocRecord, cacheSize);
		if EXPECT_FALSE (!cacheData) { dyrecord_destroy(allocRecord); return false; }

		bres = read_file(PIPELINE_CACHE_NAME, cacheData, cacheSize);
		if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }
	}

	VkShaderModuleCreateInfo shaderInfo = {0};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = shaderSize;
	shaderInfo.pCode = shaderCode;

	VkShaderModule shader = VK_NULL_HANDLE;

	if (!gpu->usingMaintenance5) {
		VK_CALL_RES(vkCreateShaderModule, device, &shaderInfo, g_allocator, &shader);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		gpu->shaderModule = shader;
	}

	VkPipelineCacheCreateInfo cacheInfo = {0};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheInfo.flags =
		gpu->usingPipelineCreationCacheControl ? VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT : 0;
	cacheInfo.initialDataSize = cacheSize;
	cacheInfo.pInitialData = cacheData;

	VkPipelineCache cache;
	VK_CALL_RES(vkCreatePipelineCache, device, &cacheInfo, g_allocator, &cache);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	gpu->pipelineCache = cache;

	VkDescriptorSetLayout descriptorSetLayouts[1];
	descriptorSetLayouts[0] = descriptorSetLayout;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = ARRAY_SIZE(descriptorSetLayouts);
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;

	VkPipelineLayout pipelineLayout;
	VK_CALL_RES(vkCreatePipelineLayout, device, &pipelineLayoutInfo, g_allocator, &pipelineLayout);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	gpu->pipelineLayout = pipelineLayout;

	uint32_t specialisationData[1];
	specialisationData[0] = workgroupSize;

	VkSpecializationMapEntry specialisationMapEntries[1];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset = 0;
	specialisationMapEntries[0].size = sizeof(specialisationData[0]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = ARRAY_SIZE(specialisationMapEntries);
	specialisationInfo.pMapEntries = specialisationMapEntries;
	specialisationInfo.dataSize = sizeof(specialisationData);
	specialisationInfo.pData = specialisationData;

	VkPipelineShaderStageCreateInfo shaderStageInfo = {0};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext = gpu->usingMaintenance5 ? &shaderInfo : NULL;
	shaderStageInfo.flags =
		gpu->usingSubgroupSizeControl ? VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT : 0;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = shader;
	shaderStageInfo.pName = entryPointName;
	shaderStageInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo pipelineInfos[1] = {0};
	pipelineInfos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfos[0].flags =
		gpu->usingPipelineExecutableProperties ?
		VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR | VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR :
		0;
	pipelineInfos[0].stage = shaderStageInfo;
	pipelineInfos[0].layout = pipelineLayout;

	VkPipeline pipeline;
	VK_CALL_RES(vkCreateComputePipelines,
		device, cache, ARRAY_SIZE(pipelineInfos), pipelineInfos, g_allocator, &pipeline);

	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	gpu->pipeline = pipeline;

	bres = save_pipeline_cache(device, cache, PIPELINE_CACHE_NAME);
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	if (computeQueueFamilyTimestampValidBits || transferQueueFamilyTimestampValidBits) {
		VkQueryPoolCreateInfo queryPoolInfo = {0};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolInfo.queryCount = inoutsPerHeap * 4;

		VkQueryPool queryPool;
		VK_CALL_RES(vkCreateQueryPool, device, &queryPoolInfo, g_allocator, &queryPool);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		gpu->queryPool = queryPool;
	}

	if (gpu->usingPipelineExecutableProperties) {
		bres = capture_pipeline(device, pipeline);
		if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }
	}

	VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, g_allocator);
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	VK_CALL(vkDestroyPipelineCache, device, cache, g_allocator);
	gpu->pipelineCache = VK_NULL_HANDLE;

	if (!gpu->usingMaintenance5) {
		VK_CALL(vkDestroyShaderModule, device, shader, g_allocator);
		gpu->shaderModule = VK_NULL_HANDLE;
	}

	dyrecord_destroy(allocRecord);
	return true;
}

static bool record_transfer_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	VkBuffer hostVisibleBuffer,
	VkBuffer deviceLocalBuffer,
	const VkBufferCopy* inBufferRegion,
	const VkBufferCopy* outBufferRegion,
	const VkDependencyInfo* dependencyInfos,
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
	VK_CALL(vkCmdCopyBuffer, cmdBuffer, hostVisibleBuffer, deviceLocalBuffer, inBufferRegionCount, inBufferRegion);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[0]);
	
	uint32_t outBufferRegionCount = 1;
	VK_CALL(vkCmdCopyBuffer, cmdBuffer, deviceLocalBuffer, hostVisibleBuffer, outBufferRegionCount, outBufferRegion);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[1]);

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
	VkPipelineLayout pipelineLayout,
	const VkDescriptorSet* descriptorSet,
	const VkDependencyInfo* dependencyInfos,
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

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[0]);

	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
	uint32_t firstDescriptorSet = 0;
	uint32_t descriptorSetCount = 1;
	uint32_t dynamicOffsetCount = 0;
	uint32_t* dynamicOffsets = NULL;

	VK_CALL(vkCmdBindDescriptorSets,
		cmdBuffer, bindPoint, pipelineLayout, firstDescriptorSet, descriptorSetCount, descriptorSet, dynamicOffsetCount,
		dynamicOffsets);

	VK_CALL(vkCmdBindPipeline, cmdBuffer, bindPoint, pipeline);

	uint32_t workgroupCountX = workgroupCount;
	uint32_t workgroupCountY = 1;
	uint32_t workgroupCountZ = 1;

	// Use vkCmdDispatchBase if want to alter base value of workgroup index (first value of WorkgroupId)
	VK_CALL(vkCmdDispatch, cmdBuffer, workgroupCountX, workgroupCountY, workgroupCountZ);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[1]);

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
	const VkBuffer* hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* descriptorSets  = gpu->descriptorSets;

	VkCommandBuffer* computeCmdBuffers = gpu->computeCommandBuffers;
	VkCommandBuffer* transferCmdBuffers = gpu->transferCommandBuffers;
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

	uint32_t computeQueueFamilyIndex = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;
	uint32_t computeQueueFamilyTimestampValidBits = gpu->computeQueueFamilyTimestampValidBits;
	uint32_t transferQueueFamilyTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;

	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	size_t allocCount = 1;
	size_t allocSize =
		inoutsPerBuffer * sizeof(VkBufferCopy) * 2 +
		inoutsPerHeap * sizeof(VkBufferMemoryBarrier2) * 5 +
		inoutsPerHeap * sizeof(VkDependencyInfo) * 4;

	void* p = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!p) { dyrecord_destroy(allocRecord); return false; }

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
		device, transferQueueFamilyIndex, &gpu->transferCommandPool, transferCmdPoolName, transferCmdBufferCount,
		transferCmdBuffers);

	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	const char* computeCmdPoolName = "Compute";
	uint32_t computeCmdBufferCount = inoutsPerHeap;

	bres = create_command_handles(
		device, computeQueueFamilyIndex, &gpu->computeCommandPool, computeCmdPoolName, computeCmdBufferCount,
		computeCmdBuffers);

	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerBuffer; i++) {
		inBufferCopies[i].srcOffset = bytesPerInout * i;
		inBufferCopies[i].dstOffset = bytesPerInout * i;
		inBufferCopies[i].size = bytesPerIn;

		outBufferCopies[i].srcOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].dstOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].size = bytesPerOut;
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer hostVisibleBuffer = hostVisibleBuffers[i];
		VkBuffer deviceLocalBuffer = deviceLocalBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			transferBufferMemoryBarriers[j][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][0].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers[j][0].buffer = deviceLocalBuffer;
			transferBufferMemoryBarriers[j][0].offset = bytesPerInout * k;
			transferBufferMemoryBarriers[j][0].size = bytesPerIn;

			transferBufferMemoryBarriers[j][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][1].dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][1].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			transferBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers[j][1].buffer = deviceLocalBuffer;
			transferBufferMemoryBarriers[j][1].offset = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers[j][1].size = bytesPerOut;

			transferBufferMemoryBarriers[j][2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][2].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][2].dstStageMask = VK_PIPELINE_STAGE_2_HOST_BIT;
			transferBufferMemoryBarriers[j][2].dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
			transferBufferMemoryBarriers[j][2].buffer = hostVisibleBuffer;
			transferBufferMemoryBarriers[j][2].offset = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers[j][2].size = bytesPerOut;

			computeBufferMemoryBarriers[j][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			computeBufferMemoryBarriers[j][0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][0].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
			computeBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers[j][0].buffer = deviceLocalBuffer;
			computeBufferMemoryBarriers[j][0].offset = bytesPerInout * k;
			computeBufferMemoryBarriers[j][0].size = bytesPerIn;

			computeBufferMemoryBarriers[j][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			computeBufferMemoryBarriers[j][1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][1].srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			computeBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers[j][1].buffer = deviceLocalBuffer;
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
				transferCmdBuffers[j], hostVisibleBuffers[i], deviceLocalBuffers[i], &inBufferCopies[k],
				&outBufferCopies[k], transferDependencyInfos[j], queryPool, firstTransferQuery,
				transferQueueFamilyTimestampValidBits);

			if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

			uint32_t firstComputeQuery = j * 4 + 2;
			bres = record_compute_cmdbuffer(
				computeCmdBuffers[j], pipeline, pipelineLayout, &descriptorSets[j], computeDependencyInfos[j],
				queryPool, firstComputeQuery, computeQueueFamilyTimestampValidBits, workgroupCount);

			if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }
		}
	}

	VkSemaphoreTypeCreateInfo semaphoreTypeInfo = {0};
	semaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	semaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	semaphoreTypeInfo.initialValue = 0;

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = &semaphoreTypeInfo;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VkSemaphore semaphore;
		VK_CALL_RES(vkCreateSemaphore, device, &semaphoreInfo, g_allocator, &semaphore);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
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

	dyrecord_destroy(allocRecord);
	return true;
}

bool submit_commands(Gpu* restrict gpu)
{
	const VkDeviceMemory* hostVisibleMemories = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer* computeCmdBuffers = gpu->computeCommandBuffers;
	const VkCommandBuffer* transferCmdBuffers = gpu->transferCommandBuffers;
	const VkSemaphore* semaphores = gpu->semaphores;

	StartValue* const* mappedInBuffers = gpu->mappedInBuffers;
	StopTime* const* mappedOutBuffers = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkQueue computeQueue = gpu->computeQueue;
	VkQueue transferQueue = gpu->transferQueue;

	VkQueryPool queryPool = gpu->queryPool;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t valuesPerInout = gpu->valuesPerInout;
	uint32_t valuesPerHeap = gpu->valuesPerHeap;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	uint32_t computeQueueFamilyTimestampValidBits = gpu->computeQueueFamilyTimestampValidBits;
	uint32_t transferQueueFamilyTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;

	double timestampPeriod = (double) gpu->timestampPeriod;
	bool hostNonCoherent = gpu->hostNonCoherent;

	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	size_t allocCount = inoutsPerHeap;
	size_t allocSize =
		sizeof(StartValue) +
		sizeof(VkMappedMemoryRange) * 2 +
		sizeof(VkSubmitInfo2) * 2 +
		sizeof(VkCommandBufferSubmitInfo) * 2 +
		sizeof(VkSemaphoreSubmitInfo) * 4 +
		sizeof(VkSemaphoreWaitInfo) * 2;

	void* p = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!p) { dyrecord_destroy(allocRecord); return false; }

	StartValue* testedValues = p;

	VkMappedMemoryRange* hostVisibleInBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (testedValues + inoutsPerHeap);
	VkMappedMemoryRange* hostVisibleOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (
		hostVisibleInBuffersMappedMemoryRanges + inoutsPerHeap);

	VkSubmitInfo2* transferSubmitInfos = (VkSubmitInfo2*) (hostVisibleOutBuffersMappedMemoryRanges + inoutsPerHeap);
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
	
	size_t elmSize = sizeof(StartValue);
	size_t elmCount = 32;

	DyArray bestStartValues = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!bestStartValues) { dyrecord_destroy(allocRecord); return false; }

	bool bres = dyrecord_add(allocRecord, bestStartValues, dyarray_destroy_stub);
	if EXPECT_FALSE (!bres) { dyarray_destroy(bestStartValues); dyrecord_destroy(allocRecord); return false; }

	elmSize = sizeof(StopTime);
	elmCount = 32;

	DyArray bestStopTimes = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!bestStopTimes) { dyrecord_destroy(allocRecord); return false; }

	bres = dyrecord_add(allocRecord, bestStopTimes, dyarray_destroy_stub);
	if EXPECT_FALSE (!bres) { dyarray_destroy(bestStopTimes); dyrecord_destroy(allocRecord); return false; }

	size_t fileSize;
	bres = file_size(PROGRESS_FILE_NAME, &fileSize);
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

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

		if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

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
		VkDeviceMemory hostVisibleMemory = hostVisibleMemories[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			hostVisibleInBuffersMappedMemoryRanges[j].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			hostVisibleInBuffersMappedMemoryRanges[j].memory = hostVisibleMemory;
			hostVisibleInBuffersMappedMemoryRanges[j].offset = bytesPerInout * k;
			hostVisibleInBuffersMappedMemoryRanges[j].size = bytesPerIn;

			hostVisibleOutBuffersMappedMemoryRanges[j].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
			hostVisibleOutBuffersMappedMemoryRanges[j].memory = hostVisibleMemory;
			hostVisibleOutBuffersMappedMemoryRanges[j].offset = bytesPerInout * k + bytesPerIn;
			hostVisibleOutBuffersMappedMemoryRanges[j].size = bytesPerOut;
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
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hostVisibleInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	}

	uint32_t transferSubmitInfoCount = inoutsPerHeap;
	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, transferSubmitInfoCount, transferSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	uint32_t computeSubmitInfoCount = inoutsPerHeap;
	VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, computeSubmitInfoCount, computeSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	atomic_bool input;
	atomic_init(&input, false);

	pthread_t waitThread;
	int ires = pthread_create(&waitThread, NULL, wait_for_input, &input);
	if EXPECT_FALSE (ires) { PCREATE_FAILURE(ires); dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		uint64_t transferSemaphoreTimeout = UINT64_MAX;
		VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[i], transferSemaphoreTimeout);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);

		transferWaitSemaphoreSubmitInfos[i].value += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hostVisibleInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	}

	transferSubmitInfoCount = inoutsPerHeap;
	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, transferSubmitInfoCount, transferSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

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
			if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

			clock_t waitComputeBmEnd = clock();

			if (computeQueueFamilyTimestampValidBits) {
				uint32_t firstQuery = j * 4 + 2;
				uint32_t queryCount = 2;
				VkQueryResultFlags queryFlags = VK_QUERY_RESULT_64_BIT;

				VK_CALL_RES(vkGetQueryPoolResults,
					device, queryPool, firstQuery, queryCount, sizeof(timestamps), timestamps, sizeof(timestamps[0]),
					queryFlags);

				if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
				computeBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			computeWaitSemaphoreSubmitInfos[j].value += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			computeSubmitInfoCount = 1;
			VK_CALL_RES(vkQueueSubmit2KHR,
				computeQueue, computeSubmitInfoCount, &computeSubmitInfos[j], VK_NULL_HANDLE);

			if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

			clock_t waitTransferBmStart = clock();

			uint64_t transferSemaphoreTimeout = UINT64_MAX;
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[j], transferSemaphoreTimeout);
			if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

			clock_t waitTransferBmEnd = clock();

			if (transferQueueFamilyTimestampValidBits) {
				uint32_t firstQuery = j * 4;
				uint32_t queryCount = 2;
				VkQueryResultFlags queryFlags = VK_QUERY_RESULT_64_BIT;

				VK_CALL_RES(vkGetQueryPoolResults,
					device, queryPool, firstQuery, queryCount, sizeof(timestamps), timestamps, sizeof(timestamps[0]),
					queryFlags);

				if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
				transferBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (hostNonCoherent) {
				uint32_t rangeCount = 1;
				VK_CALL_RES(vkInvalidateMappedMemoryRanges,
					device, rangeCount, &hostVisibleOutBuffersMappedMemoryRanges[j]);

				if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
			}

			clock_t readBmStart = clock();
			read_outbuffer(mappedOutBuffers[j], &position, bestStartValues, bestStopTimes, valuesPerInout);
			clock_t readBmEnd = clock();

			clock_t writeBmStart = clock();
			write_inbuffer(mappedInBuffers[j], &testedValues[j], valuesPerInout, valuesPerHeap);
			clock_t writeBmEnd = clock();

			if (hostNonCoherent) {
				uint32_t rangeCount = 1;
				VK_CALL_RES(vkFlushMappedMemoryRanges, device, rangeCount, &hostVisibleInBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
			}

			transferWaitSemaphoreSubmitInfos[j].value += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			transferSubmitInfoCount = 1;
			VK_CALL_RES(vkQueueSubmit2KHR,
				transferQueue, transferSubmitInfoCount, &transferSubmitInfos[j], VK_NULL_HANDLE);

			if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return NULL; }

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
		if EXPECT_FALSE (ires) { PJOIN_FAILURE(ires); dyrecord_destroy(allocRecord); return false; }
	}
	else {
		atomic_store(&input, true);
		ires = pthread_cancel(waitThread);
		if EXPECT_FALSE (ires) { PCANCEL_FAILURE(ires); dyrecord_destroy(allocRecord); return false; }
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

		if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }
	}

	dyrecord_destroy(allocRecord);
	return true;
}

bool destroy_gpu(Gpu* restrict gpu)
{
	VkInstance instance = volkGetLoadedInstance();

	const VkBuffer* hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDeviceMemory* hostVisibleMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* deviceLocalMemories = gpu->deviceLocalDeviceMemories;
	const VkSemaphore* semaphores = gpu->semaphores;

	VkDevice device = gpu->device;

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool descriptorPool = gpu->descriptorPool;
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
		VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, g_allocator);

		// Make sure no command buffers are in the pending state
		VK_CALL_RES(vkDeviceWaitIdle, device);

		for (uint32_t i = 0; i < inoutsPerHeap; i++) {
			VK_CALL(vkDestroySemaphore, device, semaphores[i], g_allocator);
		}

		VK_CALL(vkDestroyCommandPool, device, computeCmdPool, g_allocator);
		VK_CALL(vkDestroyCommandPool, device, transferCmdPool, g_allocator);

		VK_CALL(vkDestroyPipeline, device, pipeline, g_allocator);
		VK_CALL(vkDestroyQueryPool, device, queryPool, g_allocator);
		VK_CALL(vkDestroyDescriptorPool, device, descriptorPool, g_allocator);

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			VK_CALL(vkDestroyBuffer, device, hostVisibleBuffers[i], g_allocator);
			VK_CALL(vkDestroyBuffer, device, deviceLocalBuffers[i], g_allocator);

			VK_CALL(vkFreeMemory, device, hostVisibleMemories[i], g_allocator);
			VK_CALL(vkFreeMemory, device, deviceLocalMemories[i], g_allocator);
		}

		VK_CALL(vkDestroyDevice, device, g_allocator);
	}

	if (instance) {
#ifndef NDEBUG
		VK_CALL(vkDestroyDebugUtilsMessengerEXT, instance, gpu->debugMessenger, g_allocator);
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
	VkCommandPool* cmdPool,
	const char* name,
	uint32_t cmdBufferCount,
	VkCommandBuffer* cmdBuffers)
{
	(void) name;
	VkResult vkres;

	VkCommandPoolCreateInfo cmdPoolInfo = {0};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;

	VkCommandPool newCmdPool;
	VK_CALL_RES(vkCreateCommandPool, device, &cmdPoolInfo, g_allocator, &newCmdPool);
	if EXPECT_FALSE (vkres) { return false; }
	*cmdPool = newCmdPool;

#ifndef NDEBUG
	if (name) {
		bool bres = set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) newCmdPool, name);
		if EXPECT_FALSE (!bres) { return false; }
	}
#endif

	VkCommandBufferAllocateInfo cmdBufferAllocInfo = {0};
	cmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdBufferAllocInfo.commandPool = newCmdPool;
	cmdBufferAllocInfo.commandBufferCount = cmdBufferCount;

	VK_CALL_RES(vkAllocateCommandBuffers, device, &cmdBufferAllocInfo, cmdBuffers);
	if EXPECT_FALSE (vkres) { return false; }

	return true;
}

bool capture_pipeline(VkDevice device, VkPipeline pipeline)
{
	VkResult vkres;

	DyRecord allocRecord = dyrecord_create();
	if EXPECT_FALSE (!allocRecord) { return false; }

	VkPipelineInfoKHR pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
	pipelineInfo.pipeline = pipeline;

	uint32_t executableCount;
	VK_CALL_RES(vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, NULL);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	if EXPECT_FALSE (!executableCount) {
		log_warning(stdout, "No pipeline executables are available for capture");
		dyrecord_destroy(allocRecord);
		return true;
	}

	size_t allocCount = executableCount;
	size_t allocSize =
		sizeof(VkPipelineExecutablePropertiesKHR) +
		sizeof(VkPipelineExecutableStatisticKHR*) +
		sizeof(VkPipelineExecutableInfoKHR) +
		sizeof(uint32_t);

	void* p = dyrecord_calloc(allocRecord, allocCount, allocSize);
	if EXPECT_FALSE (!p) { dyrecord_destroy(allocRecord); return false; }

	VkPipelineExecutablePropertiesKHR* executablesProperties = p;
	VkPipelineExecutableStatisticKHR** executablesStatistics = (VkPipelineExecutableStatisticKHR**) (
		executablesProperties + executableCount);
	VkPipelineExecutableInfoKHR* executablesInfo = (VkPipelineExecutableInfoKHR*) (
		executablesStatistics + executableCount);

	uint32_t* statisticCounts = (uint32_t*) (executablesInfo + executableCount);

	VK_CALL_RES(vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, executablesProperties);
	if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0; i < executableCount; i++) {
		executablesInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
		executablesInfo[i].pipeline = pipeline;
		executablesInfo[i].executableIndex = i;
	}

	uint32_t statisticTotal = 0;

	for (uint32_t i = 0; i < executableCount; i++) {
		VkPipelineExecutableStatisticKHR* executableStatistics = NULL;
		VK_CALL_RES(vkGetPipelineExecutableStatisticsKHR,
			device, &executablesInfo[i], &statisticCounts[i], executableStatistics);

		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
		statisticTotal += statisticCounts[i];
	}

	allocSize = statisticTotal * sizeof(VkPipelineExecutableStatisticKHR);
	executablesStatistics[0] = dyrecord_malloc(allocRecord, allocSize);
	if EXPECT_FALSE (!executablesStatistics[0]) { dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 1; i < executableCount; i++) {
		executablesStatistics[i] = executablesStatistics[i - 1] + statisticCounts[i - 1];
	}

	for (uint32_t i = 0; i < executableCount; i++) {
		for (uint32_t j = 0; j < statisticCounts[i]; j++) {
			executablesStatistics[i][j].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
		}

		VK_CALL_RES(vkGetPipelineExecutableStatisticsKHR,
			device, &executablesInfo[i], &statisticCounts[i], executablesStatistics[i]);
		
		if EXPECT_FALSE (vkres) { dyrecord_destroy(allocRecord); return false; }
	}

	DyString message = dystring_create(1024);
	if EXPECT_FALSE (!message) { dyrecord_destroy(allocRecord); return false; }

	bool bres = dyrecord_add(allocRecord, message, dystring_destroy_stub);
	if EXPECT_FALSE (!bres) { dystring_destroy(message); dyrecord_destroy(allocRecord); return false; }

	for (uint32_t i = 0; i < executableCount; i++) {
		VkShaderStageFlags stages = executablesProperties[i].stages;
		uint32_t statisticCount = statisticCounts[i];
		char str[21];

		dystring_append(message, "\n");
		dystring_append(message, executablesProperties[i].name);
		dystring_append(message, "\n");

		for (uint32_t j = 0; j < sizeof(stages) * CHAR_BIT; j++) {
			VkShaderStageFlagBits stage = stages & (UINT32_C(1) << j);

			if (stage) {
				const char* sStage = string_VkShaderStageFlagBits(stage);
				dystring_append(message, sStage);
				dystring_append(message, " ");
			}
		}

		sprintf(str, "(%" PRIu32 ")\n",  executablesProperties[i].subgroupSize);

		dystring_append(message, str);
		dystring_append(message, executablesProperties[i].description);
		dystring_append(message, "\n");

		for (uint32_t j = 0; j < statisticCount; j++) {
			dystring_append(message, "\n\t");
			dystring_append(message, executablesStatistics[i][j].name);
			dystring_append(message, ": ");

			switch (executablesStatistics[i][j].format) {
				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
					strcpy(str, executablesStatistics[i][j].value.b32 ? "True" : "False");
					break;

				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
					sprintf(str, "%" PRId64, executablesStatistics[i][j].value.i64);
					break;

				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
					sprintf(str, "%" PRIu64, executablesStatistics[i][j].value.u64);
					break;

				case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
					sprintf(str, "%g", executablesStatistics[i][j].value.f64);
					break;

				default:
					break;
			}

			dystring_append(message, str);
			dystring_append(message, "\n\t");
			dystring_append(message, executablesStatistics[i][j].description);
			dystring_append(message, "\n");
		}
	}

	const char* rawMessage = dystring_raw(message);

	bres = write_text(
		CAPTURE_FILE_NAME,
		"PIPELINE CAPTURE DATA\n"
		"\n"
		"Total # executables: %" PRIu32 "\n"
		"Total # statistics:  %" PRIu32 "\n"
		"\n"
		"%s",
		executableCount, statisticTotal, rawMessage);
	
	if EXPECT_FALSE (!bres) { dyrecord_destroy(allocRecord); return false; }

	dyrecord_destroy(allocRecord);
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
			for (uint32_t j = 2; j < ARRAY_SIZE(val0mod1off); j++) {
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
			for (uint32_t j = 1; j < ARRAY_SIZE(val0mod1off); j++) {
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
			for (uint32_t j = 3; j < ARRAY_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 2] || val0mod1off[j] * 4 != curValue) { continue; }

				val0mod1off[j - 2] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 5

		if (curValue % 6 == 1) {
			for (uint32_t j = 0; j < ARRAY_SIZE(val0mod1off); j++) {
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
			for (uint32_t j = 2; j < ARRAY_SIZE(val0mod1off); j++) {
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
			for (uint32_t j = 1; j < ARRAY_SIZE(val0mod1off); j++) {
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
			for (uint32_t j = 4; j < ARRAY_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 3] || val0mod1off[j] * 8 != curValue) { continue; }

				val0mod1off[j - 3] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 1

		for (uint32_t j = 0; j < ARRAY_SIZE(val0mod1off); j++) {
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
