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
#include "config.h"

bool create_instance(struct Gpu* restrict gpu)
{
	VkResult vkres;
	bool bres;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	DyRecord gpuRecord = dyrecord_create();
	if CZ_NOEXPECT (!gpuRecord) { dyrecord_destroy(localRecord); return false; }
	gpu->allocRecord = gpuRecord;

	// Load global-level Vulkan functions via volk
	vkres = volkInitialize();
	if CZ_NOEXPECT (vkres) { VKINIT_FAILURE(vkres); dyrecord_destroy(localRecord); return false; }

	uint32_t instanceApiVersion = volkGetInstanceVersion();
	if CZ_NOEXPECT (instanceApiVersion == VK_API_VERSION_1_0) {
		log_error(stderr, "Vulkan instance version (1.0) does not satisfy minimum version requirement (1.1)");
		dyrecord_destroy(localRecord);
		return false;
	}

	// Specify allocator for Vulkan to use
	VkAllocationCallbacks* allocator = NULL;

	if (czgConfig.allocLogPath) {
		size_t allocSize = sizeof(VkAllocationCallbacks);
		allocator = dyrecord_malloc(gpuRecord, allocSize);
		if CZ_NOEXPECT (!allocator) {dyrecord_destroy(localRecord); return false; }
		gpu->allocator = allocator;

		allocator->pUserData = &czgCallbackData;
		allocator->pfnAllocation = allocation_callback;
		allocator->pfnReallocation = reallocation_callback;
		allocator->pfnFree = free_callback;
		allocator->pfnInternalAllocation = internal_allocation_callback;
		allocator->pfnInternalFree = internal_free_callback;

		bres = init_alloc_logfile(czgConfig.allocLogPath);
		if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }
	}

#ifndef NDEBUG
	bres = init_debug_logfile(CZ_DEBUG_LOG_NAME);
	if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }
#endif

	VkDebugUtilsMessageSeverityFlagsEXT debugMessageSeverity = 0;
	debugMessageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
	debugMessageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
	debugMessageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
	debugMessageSeverity |= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

	VkDebugUtilsMessageTypeFlagsEXT debugMessageType = 0;
	debugMessageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT;
	debugMessageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
	debugMessageType |= VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

	VkDebugUtilsMessengerCreateInfoEXT debugMessengerInfo = {0};
	debugMessengerInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugMessengerInfo.messageSeverity = debugMessageSeverity;
	debugMessengerInfo.messageType = debugMessageType;
	debugMessengerInfo.pfnUserCallback = debug_callback;
	debugMessengerInfo.pUserData = &czgCallbackData;

	// Get instance layers
	uint32_t layerCount;
	VK_CALLR(vkEnumerateInstanceLayerProperties, &layerCount, NULL);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	VkLayerProperties* layersProps = NULL;

	if (layerCount) {
		size_t allocSize = layerCount * sizeof(VkLayerProperties);
		layersProps = dyrecord_malloc(localRecord, allocSize);
		if CZ_NOEXPECT (!layersProps) { dyrecord_destroy(localRecord); return false; }

		VK_CALLR(vkEnumerateInstanceLayerProperties, &layerCount, layersProps);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	}

	// Get instance extensions
	uint32_t extensionCount;
	VK_CALLR(vkEnumerateInstanceExtensionProperties, NULL, &extensionCount, NULL);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	VkExtensionProperties* extensionsProps = NULL;

	if (extensionCount) {
		size_t allocSize = extensionCount * sizeof(VkExtensionProperties);
		extensionsProps = dyrecord_malloc(localRecord, allocSize);
		if CZ_NOEXPECT (!extensionsProps) { dyrecord_destroy(localRecord); return false; }

		VK_CALLR(vkEnumerateInstanceExtensionProperties, NULL, &extensionCount, extensionsProps);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	}

	// Select layers
	size_t elmSize = sizeof(const char*);
	size_t elmCount = 4;

	DyArray enabledLayers = dyarray_create(elmSize, elmCount);
	if CZ_NOEXPECT (!enabledLayers) { dyrecord_destroy(localRecord); return false; }

	bres = dyrecord_add(localRecord, enabledLayers, dyarray_destroy_stub);
	if CZ_NOEXPECT (!bres) { dyarray_destroy(enabledLayers); dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < layerCount; i++) {
		const char* layerName = layersProps[i].layerName;

		if (
			(czgConfig.extensionLayers && (
				!strcmp(layerName, "VK_LAYER_KHRONOS_synchronization2") ||
				!strcmp(layerName, "VK_LAYER_KHRONOS_timeline_semaphore"))) ||
			(czgConfig.profileLayers && !strcmp(layerName, "VK_LAYER_KHRONOS_profiles")) ||
			(czgConfig.validationLayers && !strcmp(layerName, "VK_LAYER_KHRONOS_validation")))
		{
			dyarray_append(enabledLayers, &layerName);
		}
	}

	// Select extensions
	const void* nextChain = NULL;
	const void** next = &nextChain;

	bool usingPortabilityEnumeration = false;
	bool usingDebugUtils = false;

	elmSize = sizeof(const char*);
	elmCount = 2;

	DyArray enabledExtensions = dyarray_create(elmSize, elmCount);
	if CZ_NOEXPECT (!enabledExtensions) { dyrecord_destroy(localRecord); return false; }

	bres = dyrecord_add(localRecord, enabledExtensions, dyarray_destroy_stub);
	if CZ_NOEXPECT (!bres) { dyarray_destroy(enabledExtensions); dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < extensionCount; i++) {
		const char* extensionName = extensionsProps[i].extensionName;

		if (!strcmp(extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			(void) next; // Shut up warning for unused variable
			dyarray_append(enabledExtensions, &extensionName);
			usingPortabilityEnumeration = true;
		}

#ifndef NDEBUG
		else if (!strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			CZ_PNEXT_ADD(next, debugMessengerInfo);
			dyarray_append(enabledExtensions, &extensionName);
			usingDebugUtils = true;
		}
#endif
	}

	// Create instance
	VkApplicationInfo appInfo = {0};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = CZ_NAME;
	appInfo.applicationVersion = VK_MAKE_API_VERSION(0, CZ_VERSION_MAJOR, CZ_VERSION_MINOR, CZ_VERSION_PATCH);
	appInfo.apiVersion = VK_API_VERSION_1_4;

	uint32_t enabledLayerCount = (uint32_t) dyarray_size(enabledLayers);
	const char** enabledLayerNames = dyarray_raw(enabledLayers);

	uint32_t enabledExtensionCount = (uint32_t) dyarray_size(enabledExtensions);
	const char** enabledExtensionNames = dyarray_raw(enabledExtensions);

	VkInstanceCreateFlags instanceFlags = 0;
	if (usingPortabilityEnumeration) {
		instanceFlags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	}

	VkInstanceCreateInfo instanceInfo = {0};
	instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceInfo.pNext = nextChain;
	instanceInfo.flags = instanceFlags;
	instanceInfo.pApplicationInfo = &appInfo;
	instanceInfo.enabledLayerCount = enabledLayerCount;
	instanceInfo.ppEnabledLayerNames = enabledLayerNames;
	instanceInfo.enabledExtensionCount = enabledExtensionCount;
	instanceInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_DEFAULT) {
		printf("Enabled instance layers (%" PRIu32 "):\n", enabledLayerCount);
		for (uint32_t i = 0; i < enabledLayerCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledLayerNames[i]);
		}
		CZ_NEWLINE();

		printf("Enabled instance extensions (%" PRIu32 "):\n", enabledExtensionCount);
		for (uint32_t i = 0; i < enabledExtensionCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtensionNames[i]);
		}
		CZ_NEWLINE();
	}

	VkInstance instance;
	VK_CALLR(vkCreateInstance, &instanceInfo, allocator, &instance);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	volkLoadInstanceOnly(instance);

	if (usingDebugUtils) {
		VkDebugUtilsMessengerEXT messenger;
		VK_CALLR(vkCreateDebugUtilsMessengerEXT, instance, &debugMessengerInfo, allocator, &messenger);

		if (vkres == VK_SUCCESS) {
			gpu->debugMessenger = messenger;
		}
	}

	dyrecord_destroy(localRecord);
	return true;
}

bool select_device(struct Gpu* restrict gpu)
{
	VkResult vkres;
	size_t allocCount;
	size_t allocSize;

	VkInstance instance = volkGetLoadedInstance();
	if CZ_NOEXPECT (!instance) { return false; }

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	// Get physical devices
	uint32_t deviceCount;
	VK_CALLR(vkEnumeratePhysicalDevices, instance, &deviceCount, NULL);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	if CZ_NOEXPECT (!deviceCount) {
		log_error(stderr, "No physical devices are accessible to the Vulkan instance");
		dyrecord_destroy(localRecord);
		return false;
	}

	allocSize = deviceCount * sizeof(VkPhysicalDevice);
	VkPhysicalDevice* devices = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!devices) { dyrecord_destroy(localRecord); return false; }

	VK_CALLR(vkEnumeratePhysicalDevices, instance, &deviceCount, devices);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Get extensions of each device
	allocSize = deviceCount * sizeof(uint32_t);
	uint32_t* extensionCounts = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!extensionCounts) { dyrecord_destroy(localRecord); return false; }

	allocSize = deviceCount * sizeof(VkExtensionProperties*);
	VkExtensionProperties** devicesExtensionsProperties = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!devicesExtensionsProperties) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];
		VK_CALLR(vkEnumerateDeviceExtensionProperties, device, NULL, &extensionCounts[i], NULL);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

		allocCount = extensionCounts[i];
		allocSize = sizeof(VkExtensionProperties);

		devicesExtensionsProperties[i] = dyrecord_calloc(localRecord, allocCount, allocSize);
		if CZ_NOEXPECT (!devicesExtensionsProperties[i]) { dyrecord_destroy(localRecord); return false; }

		VK_CALLR(vkEnumerateDeviceExtensionProperties,
			device, NULL, &extensionCounts[i], devicesExtensionsProperties[i]);

		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	}

	// Get queue familes of each device
	allocSize = deviceCount * sizeof(uint32_t);
	uint32_t* familyCounts = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!familyCounts) { dyrecord_destroy(localRecord); return false; }

	allocSize = deviceCount * sizeof(VkQueueFamilyProperties2*);
	VkQueueFamilyProperties2** devicesFamiliesProperties = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!devicesFamiliesProperties) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		VkPhysicalDevice device = devices[i];
		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, device, &familyCounts[i], NULL);

		allocCount = familyCounts[i];
		allocSize = sizeof(VkQueueFamilyProperties2);

		devicesFamiliesProperties[i] = dyrecord_calloc(localRecord, allocCount, allocSize);
		if CZ_NOEXPECT (!devicesFamiliesProperties[i]) { dyrecord_destroy(localRecord); return false; }

		for (uint32_t j = 0; j < familyCounts[i]; j++) {
			devicesFamiliesProperties[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
		}

		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, device, &familyCounts[i], devicesFamiliesProperties[i]);
	}

	// Get memory properties of each device
	allocCount = deviceCount;
	allocSize = sizeof(VkPhysicalDeviceMemoryProperties2);

	VkPhysicalDeviceMemoryProperties2* devicesMemoryProperties = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!devicesMemoryProperties) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		devicesMemoryProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		VK_CALL(vkGetPhysicalDeviceMemoryProperties2, devices[i], &devicesMemoryProperties[i]);
	}

	// Get properties of each device
	allocCount = deviceCount;
	allocSize = sizeof(VkPhysicalDeviceProperties2);

	VkPhysicalDeviceProperties2* devicesProperties = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!devicesProperties) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		devicesProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		VK_CALL(vkGetPhysicalDeviceProperties2, devices[i], &devicesProperties[i]);
	}

	// Get features of each device
	allocCount = deviceCount;
	allocSize = sizeof(VkPhysicalDeviceFeatures2);

	VkPhysicalDeviceFeatures2* devicesFeatures = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!devicesFeatures) { dyrecord_destroy(localRecord); return false; }

	allocCount = deviceCount;
	allocSize = sizeof(VkPhysicalDevice16BitStorageFeatures);

	VkPhysicalDevice16BitStorageFeatures* devices16BitStorageFeatures = dyrecord_calloc(
		localRecord, allocCount, allocSize);

	if CZ_NOEXPECT (!devices16BitStorageFeatures) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < deviceCount; i++) {
		devices16BitStorageFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;

		devicesFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		devicesFeatures[i].pNext = &devices16BitStorageFeatures[i];

		VK_CALL(vkGetPhysicalDeviceFeatures2, devices[i], &devicesFeatures[i]);
	}

	uint32_t deviceIndex = UINT32_MAX;
	uint32_t bestScore = 0;

	bool using16BitStorage = false;
	bool usingMaintenance4 = false;
	bool usingMaintenance5 = false;
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
		uint32_t familyCount = familyCounts[i];
		uint32_t memoryTypeCount = devicesMemoryProperties[i].memoryProperties.memoryTypeCount;

		// Check properties
		bool hasVulkan11 = devicesProperties[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasVulkan12 = devicesProperties[i].properties.apiVersion >= VK_API_VERSION_1_2;
		bool hasVulkan13 = devicesProperties[i].properties.apiVersion >= VK_API_VERSION_1_3;
		bool hasVulkan14 = devicesProperties[i].properties.apiVersion >= VK_API_VERSION_1_4;

		// Check features
		bool hasShaderInt16 = devicesFeatures[i].features.shaderInt16;
		bool hasShaderInt64 = devicesFeatures[i].features.shaderInt64;

		bool hasStorageBuffer16BitAccess = devices16BitStorageFeatures[i].storageBuffer16BitAccess;

		// Check queue families' properties
		bool hasCompute = false;
		bool hasDedicatedCompute = false;
		bool hasDedicatedTransfer = false;

		for (uint32_t j = 0; j < familyCount; j++) {
			VkQueueFlags queueFlags = devicesFamiliesProperties[i][j].queueFamilyProperties.queueFlags;

			bool isCompute = queueFlags & VK_QUEUE_COMPUTE_BIT;
			bool isGraphics = queueFlags & VK_QUEUE_GRAPHICS_BIT;
			bool isTransfer = queueFlags & VK_QUEUE_TRANSFER_BIT;
			bool isDedicatedCompute = isCompute && !isGraphics;
			bool isDedicatedTransfer = isTransfer && !isCompute && !isGraphics;

			if (isCompute)           { hasCompute = true; }
			if (isDedicatedCompute)  { hasDedicatedCompute = true; }
			if (isDedicatedTransfer) { hasDedicatedTransfer = true; }
		}

		// Check memory properties
		bool hasDeviceNonHost = false;
		bool hasDeviceLocal = false;

		bool hasHostCachedNonCoherent = false;
		bool hasHostCached = false;
		bool hasHostNonCoherent = false;
		bool hasHostVisible = false;

		for (uint32_t j = 0; j < memoryTypeCount; j++) {
			VkMemoryPropertyFlags propFlags = devicesMemoryProperties[i].memoryProperties.memoryTypes[j].propertyFlags;

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

		// Check extensions
		bool hasCopyCommands2 = false;
		bool hasMaintenance4 = false;
		bool hasMaintenance5 = false;
		bool hasMaintenance6 = false;
		bool hasMaintenance7 = false;
		bool hasMaintenance8 = false;
		bool hasMaintenance9 = false;
		bool hasMapMemory2 = false;
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
			const char* extensionName = devicesExtensionsProperties[i][j].extensionName;

			if (!strcmp(extensionName, VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME))    { hasCopyCommands2 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME)) { hasMaintenance4 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_5_EXTENSION_NAME)) { hasMaintenance5 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_6_EXTENSION_NAME)) { hasMaintenance6 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_7_EXTENSION_NAME)) { hasMaintenance7 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_8_EXTENSION_NAME)) { hasMaintenance8 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_9_EXTENSION_NAME)) { hasMaintenance9 = true; }
			else if (!strcmp(extensionName, VK_KHR_MAP_MEMORY_2_EXTENSION_NAME))  { hasMapMemory2 = true; }
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

		// Score device
		uint32_t currentScore = 1;

		if (!hasVulkan11) { continue; }

		if (!hasDeviceLocal) { continue; }
		if (!hasHostVisible) { continue; }

		if (!hasCompute) { continue; }

		if (!hasCopyCommands2)     { continue; }
		if (!hasMaintenance6)      { continue; }
		if (!hasMapMemory2)        { continue; }
		if (!hasSynchronization2)  { continue; }
		if (!hasTimelineSemaphore) { continue; }

		if (hasVulkan12) { currentScore += 50; }
		if (hasVulkan13) { currentScore += 50; }
		if (hasVulkan14) { currentScore += 50; }

		if (czgConfig.preferInt16 && hasShaderInt16) { currentScore += 1000; }
		if (czgConfig.preferInt64 && hasShaderInt64) { currentScore += 1000; }

		if (hasDeviceNonHost) { currentScore += 50; }

		if (hasHostCachedNonCoherent) { currentScore += 1000; }
		else if (hasHostCached)       { currentScore += 500; }
		else if (hasHostNonCoherent)  { currentScore += 100; }

		if (hasDedicatedCompute)  { currentScore += 100; }
		if (hasDedicatedTransfer) { currentScore += 100; }

		if (hasMaintenance4)                 { currentScore += 10; }
		if (hasMaintenance5)                 { currentScore += 10; }
		if (hasMaintenance7)                 { currentScore += 10; }
		if (hasMaintenance8)                 { currentScore += 10; }
		if (hasMaintenance9)                 { currentScore += 10; }
		if (hasMemoryBudget)                 { currentScore += 10; }
		if (hasMemoryPriority)               { currentScore += 10; }
		if (hasPipelineCreationCacheControl) { currentScore += 10; }
		if (hasSpirv14)                      { currentScore += 10; }
		if (hasStorageBuffer16BitAccess)     { currentScore += 100; }
		if (hasSubgroupSizeControl)          { currentScore += 10; }

		if (czgConfig.capturePath && hasPipelineExecutableProperties) { currentScore += 10; }

		if (currentScore > bestScore) {
			bestScore = currentScore;
			deviceIndex = i;

			using16BitStorage = hasStorageBuffer16BitAccess;
			usingMaintenance4 = hasMaintenance4;
			usingMaintenance5 = hasMaintenance5;
			usingMaintenance7 = hasMaintenance7;
			usingMaintenance8 = hasMaintenance8;
			usingMaintenance9 = hasMaintenance9;
			usingMemoryBudget = hasMemoryBudget;
			usingMemoryPriority = hasMemoryPriority;
			usingPipelineCreationCacheControl = hasPipelineCreationCacheControl;
			usingPipelineExecutableProperties = czgConfig.capturePath && hasPipelineExecutableProperties;
			usingPortabilitySubset = hasPortabilitySubset;
			usingShaderInt16 = czgConfig.preferInt16 && hasShaderInt16;
			usingShaderInt64 = czgConfig.preferInt64 && hasShaderInt64;
			usingSpirv14 = hasSpirv14;
			usingSubgroupSizeControl = hasSubgroupSizeControl;
			usingVulkan12 = hasVulkan12;
			usingVulkan13 = hasVulkan13;
			usingVulkan14 = hasVulkan14;
		}
	}

	if (deviceIndex == UINT32_MAX) {
		log_error(
			stderr,
			"No physical device satisfies program requirements; "
			"see device_requirements.md for comprehensive physical device requirements");

		dyrecord_destroy(localRecord);
		return false;
	}

	const char* deviceName = devicesProperties[deviceIndex].properties.deviceName;
	uint32_t familyCount = familyCounts[deviceIndex];

	// Determine Vulkan & SPIR-V versions
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

	// Select queue families
	uint32_t computeFamilyIndex = 0;
	uint32_t transferFamilyIndex = 0;
	uint32_t computeQueueIndex = 0;
	uint32_t transferQueueIndex = 0;

	bool hasCompute = false;
	bool hasTransfer = false;
	bool hasDedicatedCompute = false;
	bool hasDedicatedTransfer = false;

	// TODO implement algorithm to choose queue indices too
	for (uint32_t i = 0; i < familyCount; i++) {
		VkQueueFlags queueFlags = devicesFamiliesProperties[deviceIndex][i].queueFamilyProperties.queueFlags;

		bool isCompute = queueFlags & VK_QUEUE_COMPUTE_BIT;
		bool isGraphics = queueFlags & VK_QUEUE_GRAPHICS_BIT;
		bool isTransfer = queueFlags & VK_QUEUE_TRANSFER_BIT;
		bool isDedicatedCompute = isCompute && !isGraphics;
		bool isDedicatedTransfer = isTransfer && !isCompute && !isGraphics;

		if (isCompute) {
			if (!hasDedicatedCompute && isDedicatedCompute) {
				computeFamilyIndex = i;
				hasDedicatedCompute = true;
				hasCompute = true;
			}
			else if (!hasCompute) {
				computeFamilyIndex = i;
				hasCompute = true;
			}
		}

		if (isTransfer) {
			if (!hasDedicatedTransfer && isDedicatedTransfer) {
				transferFamilyIndex = i;
				hasDedicatedTransfer = true;
				hasTransfer = true;
			}
			else if (!hasTransfer) {
				transferFamilyIndex = i;
				hasTransfer = true;
			}
		}
	}

	if (!hasTransfer) {
		transferFamilyIndex = computeFamilyIndex;
	}

	// TEMP remove when implemented algorithm for selecting queue indices
	if (computeFamilyIndex == transferFamilyIndex) {
		uint32_t familyIndex = computeFamilyIndex;
		uint32_t queueCount = devicesFamiliesProperties[deviceIndex][familyIndex].queueFamilyProperties.queueCount;

		if (queueCount > 1) {
			computeQueueIndex = 0;
			transferQueueIndex = 1;
		}
	}

	gpu->physicalDevice = devices[deviceIndex];

	gpu->computeFamilyIndex = computeFamilyIndex;
	gpu->transferFamilyIndex = transferFamilyIndex;
	gpu->computeQueueIndex = computeQueueIndex;
	gpu->transferQueueIndex = transferQueueIndex;

	gpu->vkVerMajor = vkVerMajor;
	gpu->vkVerMinor = vkVerMinor;
	gpu->spvVerMajor = spvVerMajor;
	gpu->spvVerMinor = spvVerMinor;

	gpu->using16BitStorage = using16BitStorage;
	gpu->usingMaintenance4 = usingMaintenance4;
	gpu->usingMaintenance5 = usingMaintenance5;
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

	if (czgConfig.queryBenchmarks) {
		gpu->computeFamilyTimestampValidBits =
			devicesFamiliesProperties[deviceIndex][computeFamilyIndex].queueFamilyProperties.timestampValidBits;

		gpu->transferFamilyTimestampValidBits =
			devicesFamiliesProperties[deviceIndex][transferFamilyIndex].queueFamilyProperties.timestampValidBits;

		gpu->timestampPeriod = devicesProperties[deviceIndex].properties.limits.timestampPeriod;
	}

	// Display info about selected device
	switch (czgConfig.outputLevel) {
	case CZ_OUTPUT_LEVEL_DEFAULT:
		printf(
			"Device: %s\n"
			"\tVulkan version:    %" PRIu32 ".%" PRIu32 "\n"
			"\tSPIR-V version:    %" PRIu32 ".%" PRIu32 "\n"
			"\tCompute QF index:  %" PRIu32 "\n"
			"\tTransfer QF index: %" PRIu32 "\n",
			deviceName,
			vkVerMajor,  vkVerMinor,
			spvVerMajor, spvVerMinor,
			computeFamilyIndex, transferFamilyIndex);

		if (czgConfig.preferInt16) {
			printf("\tshaderInt16:       %d\n", usingShaderInt16);
		}
		if (czgConfig.preferInt64) {
			printf("\tshaderInt64:       %d\n", usingShaderInt64);
		}

		CZ_NEWLINE();
		break;

	case CZ_OUTPUT_LEVEL_VERBOSE:
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
			deviceName, bestScore,
			vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor,
			computeFamilyIndex, transferFamilyIndex, computeQueueIndex, transferQueueIndex,
			usingMaintenance4, usingMaintenance5, usingMaintenance7, usingMaintenance8, usingMaintenance9,
			usingMemoryPriority, usingPipelineCreationCacheControl, usingPipelineExecutableProperties,
			usingShaderInt16, usingShaderInt64, using16BitStorage, usingSubgroupSizeControl);

		break;

	default:
		break;
	}

	dyrecord_destroy(localRecord);
	return true;
}

bool create_device(struct Gpu* restrict gpu)
{
	const VkAllocationCallbacks* allocator = gpu->allocator;

	VkPhysicalDevice physicalDevice = gpu->physicalDevice;

	uint32_t computeFamilyIndex = gpu->computeFamilyIndex;
	uint32_t transferFamilyIndex = gpu->transferFamilyIndex;
	uint32_t computeQueueIndex = gpu->computeQueueIndex;
	uint32_t transferQueueIndex = gpu->transferQueueIndex;

	uint32_t spvVerMinor = gpu->spvVerMinor;
	
	VkResult vkres;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	size_t elmSize = sizeof(const char*);
	size_t elmCount = 21;

	DyArray enabledExtensions = dyarray_create(elmSize, elmCount);
	if CZ_NOEXPECT (!enabledExtensions) { dyrecord_destroy(localRecord); return false; }

	bool bres = dyrecord_add(localRecord, enabledExtensions, dyarray_destroy_stub);
	if CZ_NOEXPECT (!bres) { dyarray_destroy(enabledExtensions); dyrecord_destroy(localRecord); return false; }

	// Required extensions
	const char* extensionName = VK_KHR_COPY_COMMANDS_2_EXTENSION_NAME;
	dyarray_append(enabledExtensions, &extensionName);

	extensionName = VK_KHR_MAINTENANCE_6_EXTENSION_NAME;
	dyarray_append(enabledExtensions, &extensionName);

	extensionName = VK_KHR_MAP_MEMORY_2_EXTENSION_NAME;
	dyarray_append(enabledExtensions, &extensionName);

	extensionName = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
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

	VkPhysicalDeviceFeatures2 deviceFeatures = {0};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.features.shaderInt64 = gpu->usingShaderInt64;
	deviceFeatures.features.shaderInt16 = gpu->usingShaderInt16;

	VkPhysicalDevice16BitStorageFeatures device16BitStorageFeatures = {0};
	device16BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
	device16BitStorageFeatures.storageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeatures deviceDynamicRenderingFeatures = {0};
	deviceDynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
	deviceDynamicRenderingFeatures.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceMaintenance4Features deviceMaintenance4Features = {0};
	deviceMaintenance4Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES;
	deviceMaintenance4Features.maintenance4 = VK_TRUE;

	VkPhysicalDeviceMaintenance5Features deviceMaintenance5Features = {0};
	deviceMaintenance5Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_5_FEATURES;
	deviceMaintenance5Features.maintenance5 = VK_TRUE;

	VkPhysicalDeviceMaintenance6Features deviceMaintenance6Features = {0};
	deviceMaintenance6Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_6_FEATURES;
	deviceMaintenance6Features.maintenance6 = VK_TRUE;

	VkPhysicalDeviceMaintenance7FeaturesKHR deviceMaintenance7Features = {0};
	deviceMaintenance7Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_7_FEATURES_KHR;
	deviceMaintenance7Features.maintenance7 = VK_TRUE;

	VkPhysicalDeviceMaintenance8FeaturesKHR deviceMaintenance8Features = {0};
	deviceMaintenance8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_8_FEATURES_KHR;
	deviceMaintenance8Features.maintenance8 = VK_TRUE;

	VkPhysicalDeviceMaintenance9FeaturesKHR deviceMaintenance9Features = {0};
	deviceMaintenance9Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_9_FEATURES_KHR;
	deviceMaintenance9Features.maintenance9 = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT deviceMemoryPriorityFeatures = {0};
	deviceMemoryPriorityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
	deviceMemoryPriorityFeatures.memoryPriority = VK_TRUE;

	VkPhysicalDevicePipelineCreationCacheControlFeatures devicePipelineCreationCacheControlFeatures = {0};
	devicePipelineCreationCacheControlFeatures.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES;
	devicePipelineCreationCacheControlFeatures.pipelineCreationCacheControl = VK_TRUE;

	VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR devicePipelineExecutablePropertiesFeatures = {0};
	devicePipelineExecutablePropertiesFeatures.sType =
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR;
	devicePipelineExecutablePropertiesFeatures.pipelineExecutableInfo = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeatures deviceSubgroupSizeControlFeatures = {0};
	deviceSubgroupSizeControlFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES;
	deviceSubgroupSizeControlFeatures.subgroupSizeControl = VK_TRUE;

	VkPhysicalDeviceSynchronization2Features deviceSynchronization2Features = {0};
	deviceSynchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
	deviceSynchronization2Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeatures deviceTimelineSemaphoreFeatures = {0};
	deviceTimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
	deviceTimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

	void** next = &deviceFeatures.pNext;

	CZ_PNEXT_ADD(next, deviceMaintenance6Features);
	CZ_PNEXT_ADD(next, deviceSynchronization2Features);
	CZ_PNEXT_ADD(next, deviceTimelineSemaphoreFeatures);

	if (gpu->using16BitStorage) {
		CZ_PNEXT_ADD(next, device16BitStorageFeatures);
	}
	if (gpu->usingMaintenance4) {
		CZ_PNEXT_ADD(next, deviceMaintenance4Features);
	}
	if (gpu->usingMaintenance5) {
		CZ_PNEXT_ADD(next, deviceDynamicRenderingFeatures);
		CZ_PNEXT_ADD(next, deviceMaintenance5Features);
	}
	if (gpu->usingMaintenance7) {
		CZ_PNEXT_ADD(next, deviceMaintenance7Features);
	}
	if (gpu->usingMaintenance8) {
		CZ_PNEXT_ADD(next, deviceMaintenance8Features);
	}
	if (gpu->usingMaintenance9) {
		CZ_PNEXT_ADD(next, deviceMaintenance9Features);
	}
	if (gpu->usingMemoryPriority) {
		CZ_PNEXT_ADD(next, deviceMemoryPriorityFeatures);
	}
	if (gpu->usingPipelineCreationCacheControl) {
		CZ_PNEXT_ADD(next, devicePipelineCreationCacheControlFeatures);
	}
	if (gpu->usingPipelineExecutableProperties) {
		CZ_PNEXT_ADD(next, devicePipelineExecutablePropertiesFeatures);
	}
	if (gpu->usingSubgroupSizeControl) {
		CZ_PNEXT_ADD(next, deviceSubgroupSizeControlFeatures);
	}

	// Create logical device
	float computeQueuePriorities[2];
	computeQueuePriorities[0] = 1;
	computeQueuePriorities[1] = 0;

	float transferQueuePriorities[1];
	transferQueuePriorities[0] = computeQueuePriorities[1];

	VkDeviceQueueCreateInfo queueCreateInfos[2] = {0};
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].queueFamilyIndex = computeFamilyIndex;
	queueCreateInfos[0].queueCount =
		computeFamilyIndex != transferFamilyIndex || computeQueueIndex == transferQueueIndex ? 1 : 2;
	queueCreateInfos[0].pQueuePriorities = computeQueuePriorities;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = transferFamilyIndex;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = transferQueuePriorities;

	uint32_t enabledExtensionCount = (uint32_t) dyarray_size(enabledExtensions);
	const char** enabledExtensionNames = dyarray_raw(enabledExtensions);

	VkDeviceCreateInfo deviceInfo = {0};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = &deviceFeatures;
	deviceInfo.queueCreateInfoCount = computeFamilyIndex == transferFamilyIndex ? 1 : 2;
	deviceInfo.pQueueCreateInfos = queueCreateInfos;
	deviceInfo.enabledExtensionCount = enabledExtensionCount;
	deviceInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_DEFAULT) {
		printf("Enabled device extensions (%" PRIu32 "):\n", enabledExtensionCount);
		for (uint32_t i = 0; i < enabledExtensionCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtensionNames[i]);
		}
		CZ_NEWLINE();
	}

	VkDevice device;
	VK_CALLR(vkCreateDevice, physicalDevice, &deviceInfo, allocator, &device);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->device = device;

	volkLoadDevice(device);

	// Get handles for compute & transfer queues
	VkDeviceQueueInfo2 computeQueueInfo = {0};
	computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	computeQueueInfo.queueFamilyIndex = computeFamilyIndex;
	computeQueueInfo.queueIndex = computeQueueIndex;

	VK_CALL(vkGetDeviceQueue2, device, &computeQueueInfo, &gpu->computeQueue);

	VkDeviceQueueInfo2 transferQueueInfo = {0};
	transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	transferQueueInfo.queueFamilyIndex = transferFamilyIndex;
	transferQueueInfo.queueIndex = transferQueueIndex;

	VK_CALL(vkGetDeviceQueue2, device, &transferQueueInfo, &gpu->transferQueue);

	dyrecord_destroy(localRecord);
	return true;
}

bool manage_memory(struct Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;
	VkDevice device = gpu->device;

	bool (*get_buffer_requirements)(VkDevice, VkDeviceSize, VkBufferUsageFlags, VkMemoryRequirements*) =
		gpu->usingMaintenance4 ? get_buffer_requirements_main4 : get_buffer_requirements_noext;

	// Get device properties
	VkPhysicalDeviceMaintenance4Properties deviceMaintenance4Properties = {0};
	deviceMaintenance4Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES;

	VkPhysicalDeviceMaintenance3Properties deviceMaintenance3Properties = {0};
	deviceMaintenance3Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
	deviceMaintenance3Properties.pNext = gpu->usingMaintenance4 ? &deviceMaintenance4Properties : NULL;

	VkPhysicalDeviceProperties2 deviceProperties = {0};
	deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	deviceProperties.pNext = &deviceMaintenance3Properties;

	VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevice, &deviceProperties);

	// Get memory properties
	VkPhysicalDeviceMemoryBudgetPropertiesEXT deviceBudgetProperties = {0};
	deviceBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;

	VkPhysicalDeviceMemoryProperties2 deviceMemoryProperties = {0};
	deviceMemoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	deviceMemoryProperties.pNext = gpu->usingMemoryBudget ? &deviceBudgetProperties : NULL;

	VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevice, &deviceMemoryProperties);

	VkDeviceSize maxMemorySize = deviceMaintenance3Properties.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize = gpu->usingMaintenance4 ? deviceMaintenance4Properties.maxBufferSize : maxMemorySize;

	uint32_t maxStorageBufferRange = deviceProperties.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryCount = deviceProperties.properties.limits.maxMemoryAllocationCount;
	uint32_t maxWorkgroupCount = deviceProperties.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxWorkgroupSize = deviceProperties.properties.limits.maxComputeWorkGroupSize[0];
	uint32_t memoryTypeCount = deviceMemoryProperties.memoryProperties.memoryTypeCount;

	// Get memoryTypeBits for wanted host visible & device local buffers
	VkBufferUsageFlags hostVisibleBufferUsage = 0;
	hostVisibleBufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	hostVisibleBufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VkMemoryRequirements hostVisibleMemoryRequirements;
	bool bres = get_buffer_requirements(device, sizeof(char), hostVisibleBufferUsage, &hostVisibleMemoryRequirements);
	if CZ_NOEXPECT (!bres) { return false; }

	VkBufferUsageFlags deviceLocalBufferUsage = 0;
	deviceLocalBufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	deviceLocalBufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	deviceLocalBufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	VkMemoryRequirements deviceLocalMemoryRequirements;
	bres = get_buffer_requirements(device, sizeof(char), deviceLocalBufferUsage, &deviceLocalMemoryRequirements);
	if CZ_NOEXPECT (!bres) { return false; }

	uint32_t deviceLocalMemoryTypeBits = deviceLocalMemoryRequirements.memoryTypeBits;
	uint32_t hostVisibleMemoryTypeBits = hostVisibleMemoryRequirements.memoryTypeBits;

	// Select memory heaps & types
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
		uint32_t heapIndex = deviceMemoryProperties.memoryProperties.memoryTypes[i].heapIndex;
		VkMemoryPropertyFlags propFlags = deviceMemoryProperties.memoryProperties.memoryTypes[i].propertyFlags;

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

	VkDeviceSize hostVisibleHeapBudget = deviceBudgetProperties.heapBudget[hostVisibleHeapIndex];
	VkDeviceSize deviceLocalHeapBudget = deviceBudgetProperties.heapBudget[deviceLocalHeapIndex];

	VkDeviceSize hostVisibleHeapSize = deviceMemoryProperties.memoryProperties.memoryHeaps[hostVisibleHeapIndex].size;
	VkDeviceSize deviceLocalHeapSize = deviceMemoryProperties.memoryProperties.memoryHeaps[deviceLocalHeapIndex].size;

	VkDeviceSize bytesPerHostVisibleHeap = gpu->usingMemoryBudget ? hostVisibleHeapBudget : hostVisibleHeapSize;
	VkDeviceSize bytesPerDeviceLocalHeap = gpu->usingMemoryBudget ? deviceLocalHeapBudget : deviceLocalHeapSize;

	VkDeviceSize bytesPerHeap = minu64(bytesPerHostVisibleHeap, bytesPerDeviceLocalHeap);
	bytesPerHeap = (VkDeviceSize) ((float) bytesPerHeap * czgConfig.maxMemory); // User-given limit on heap memory

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
	if CZ_NOEXPECT (!bres) { return false; }

	bres = get_buffer_requirements(device, bytesPerBuffer, deviceLocalBufferUsage, &deviceLocalMemoryRequirements);
	if CZ_NOEXPECT (!bres) { return false; }

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

	// Display info on planned memory usage
	switch (czgConfig.outputLevel) {
	case CZ_OUTPUT_LEVEL_DEFAULT:
		printf(
			"Memory information:\n"
			"\tHV memory type index:    %" PRIu32 "\n"
			"\tDL memory type index:    %" PRIu32 "\n"
			"\tWorkgroup size:          %" PRIu32 "\n"
			"\tWorkgroup count:         %" PRIu32 "\n"
			"\tValues per inout-buffer: %" PRIu32 "\n"
			"\tInout-buffers per heap:  %" PRIu32 "\n\n",
			hostVisibleTypeIndex, deviceLocalTypeIndex,
			workgroupSize, workgroupCount,
			valuesPerInout, inoutsPerHeap);

		break;

	case CZ_OUTPUT_LEVEL_VERBOSE:
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
			hasHostNonCoherent,
			hostVisibleHeapIndex, deviceLocalHeapIndex,
			hostVisibleTypeIndex, deviceLocalTypeIndex,
			workgroupSize, workgroupCount,
			valuesPerInout, inoutsPerBuffer, buffersPerHeap, valuesPerHeap);

		break;

	default:
		break;
	}

	return true;
}

bool create_buffers(struct Gpu* restrict gpu)
{
	DyRecord gpuRecord = gpu->allocRecord;

	const VkAllocationCallbacks* allocator = gpu->allocator;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerInout = gpu->bytesPerInout;
	VkDeviceSize bytesPerBuffer = gpu->bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleMemory = gpu->bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDeviceLocalMemory = gpu->bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout = gpu->valuesPerInout;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;
	uint32_t hostVisibleTypeIndex = gpu->hostVisibleTypeIndex;
	uint32_t deviceLocalTypeIndex = gpu->deviceLocalTypeIndex;

	VkResult vkres;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	// Create host visible buffers
	size_t allocCount = buffersPerHeap;
	size_t allocSize = sizeof(VkBuffer);

	VkBuffer* hostVisibleBuffers = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!hostVisibleBuffers) { dyrecord_destroy(localRecord); return false; }
	gpu->hostVisibleBuffers = hostVisibleBuffers;

	VkBufferUsageFlags hostVisibleBufferUsage = 0;
	hostVisibleBufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	hostVisibleBufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;

	VkBufferCreateInfo hostVisibleBufferInfo = {0};
	hostVisibleBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hostVisibleBufferInfo.size = bytesPerBuffer;
	hostVisibleBufferInfo.usage = hostVisibleBufferUsage;
	hostVisibleBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkBuffer hostVisibleBuffer;
		VK_CALLR(vkCreateBuffer, device, &hostVisibleBufferInfo, allocator, &hostVisibleBuffer);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		hostVisibleBuffers[i] = hostVisibleBuffer;
	}

	// Create device local buffers
	allocCount = buffersPerHeap;
	allocSize = sizeof(VkBuffer);

	VkBuffer* deviceLocalBuffers = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!deviceLocalBuffers) { dyrecord_destroy(localRecord); return false; }
	gpu->deviceLocalBuffers = deviceLocalBuffers;

	VkBufferUsageFlags deviceLocalBufferUsage = 0;
	deviceLocalBufferUsage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	deviceLocalBufferUsage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	deviceLocalBufferUsage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	VkBufferCreateInfo deviceLocalBufferInfo = {0};
	deviceLocalBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	deviceLocalBufferInfo.size = bytesPerBuffer;
	deviceLocalBufferInfo.usage = deviceLocalBufferUsage;
	deviceLocalBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkBuffer deviceLocalBuffer;
		VK_CALLR(vkCreateBuffer, device, &deviceLocalBufferInfo, allocator, &deviceLocalBuffer);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		deviceLocalBuffers[i] = deviceLocalBuffer;
	}

	// Create host visible device memories
	allocCount = buffersPerHeap;
	allocSize = sizeof(VkDeviceMemory);

	VkDeviceMemory* hostVisibleMemories = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!hostVisibleMemories) { dyrecord_destroy(localRecord); return false; }
	gpu->hostVisibleDeviceMemories = hostVisibleMemories;

	VkMemoryPriorityAllocateInfoEXT hostVisiblePriorityInfo = {0};
	hostVisiblePriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	hostVisiblePriorityInfo.priority = 0;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkMemoryDedicatedAllocateInfo hostVisibleDedicatedInfo = {0};
		hostVisibleDedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		hostVisibleDedicatedInfo.pNext = gpu->usingMemoryPriority ? &hostVisiblePriorityInfo : NULL;
		hostVisibleDedicatedInfo.buffer = hostVisibleBuffers[i];

		VkMemoryAllocateInfo hostVisibleAllocInfo = {0};
		hostVisibleAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		hostVisibleAllocInfo.pNext = &hostVisibleDedicatedInfo;
		hostVisibleAllocInfo.allocationSize = bytesPerHostVisibleMemory;
		hostVisibleAllocInfo.memoryTypeIndex = hostVisibleTypeIndex;

		VkDeviceMemory hostVisibleMemory;
		VK_CALLR(vkAllocateMemory, device, &hostVisibleAllocInfo, allocator, &hostVisibleMemory);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		hostVisibleMemories[i] = hostVisibleMemory;
	}

	// Create device local device memories
	allocCount = buffersPerHeap;
	allocSize = sizeof(VkDeviceMemory);

	VkDeviceMemory* deviceLocalMemories = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!deviceLocalMemories) { dyrecord_destroy(localRecord); return false; }
	gpu->deviceLocalDeviceMemories = deviceLocalMemories;

	VkMemoryPriorityAllocateInfoEXT deviceLocalPriorityInfo = {0};
	deviceLocalPriorityInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	deviceLocalPriorityInfo.priority = 1;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VkMemoryDedicatedAllocateInfo deviceLocalDedicatedInfo = {0};
		deviceLocalDedicatedInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
		deviceLocalDedicatedInfo.pNext = gpu->usingMemoryPriority ? &deviceLocalPriorityInfo : NULL;
		deviceLocalDedicatedInfo.buffer = deviceLocalBuffers[i];

		VkMemoryAllocateInfo deviceLocalAllocInfo = {0};
		deviceLocalAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		deviceLocalAllocInfo.pNext = &deviceLocalDedicatedInfo;
		deviceLocalAllocInfo.allocationSize = bytesPerDeviceLocalMemory;
		deviceLocalAllocInfo.memoryTypeIndex = deviceLocalTypeIndex;

		VkDeviceMemory deviceLocalMemory;
		VK_CALLR(vkAllocateMemory, device, &deviceLocalAllocInfo, allocator, &deviceLocalMemory);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		deviceLocalMemories[i] = deviceLocalMemory;
	}

	// Bind buffers and device memories
	allocCount = buffersPerHeap;
	allocSize = sizeof(VkBindBufferMemoryInfo[2]);

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!bindBufferMemoryInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		bindBufferMemoryInfos[i][0].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
		bindBufferMemoryInfos[i][0].buffer = hostVisibleBuffers[i];
		bindBufferMemoryInfos[i][0].memory = hostVisibleMemories[i];
		bindBufferMemoryInfos[i][0].memoryOffset = 0;

		bindBufferMemoryInfos[i][1].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
		bindBufferMemoryInfos[i][1].buffer = deviceLocalBuffers[i];
		bindBufferMemoryInfos[i][1].memory = deviceLocalMemories[i];
		bindBufferMemoryInfos[i][1].memoryOffset = 0;
	}

	uint32_t bindInfoCount = buffersPerHeap * CZ_COUNTOF(bindBufferMemoryInfos[0]);
	VkBindBufferMemoryInfo* bindInfos = (VkBindBufferMemoryInfo*) bindBufferMemoryInfos;

	VK_CALLR(vkBindBufferMemory2, device, bindInfoCount, bindInfos);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Map host visible device memories
	allocSize = inoutsPerHeap * sizeof(StartValue*);
	StartValue** mappedInBuffers = dyrecord_malloc(gpuRecord, allocSize);
	if CZ_NOEXPECT (!mappedInBuffers) { dyrecord_destroy(localRecord); return false; }
	gpu->mappedInBuffers = mappedInBuffers;

	allocSize = inoutsPerHeap * sizeof(StopTime*);
	StopTime** mappedOutBuffers = dyrecord_malloc(gpuRecord, allocSize);
	if CZ_NOEXPECT (!mappedOutBuffers) { dyrecord_destroy(localRecord); return false; }
	gpu->mappedOutBuffers = mappedOutBuffers;

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkMemoryMapInfo mapInfo = {0};
		mapInfo.sType = VK_STRUCTURE_TYPE_MEMORY_MAP_INFO;
		mapInfo.memory = hostVisibleMemories[i];
		mapInfo.size = bytesPerHostVisibleMemory;

		void* mappedMemory;
		VK_CALLR(vkMapMemory2KHR, device, &mapInfo, &mappedMemory);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

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

	dyrecord_destroy(localRecord);
	return true;
}

bool create_descriptors(struct Gpu* restrict gpu)
{
	DyRecord gpuRecord = gpu->allocRecord;

	const VkAllocationCallbacks* allocator = gpu->allocator;
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	VkResult vkres;
	size_t allocCount;
	size_t allocSize;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	// Create descriptor set layout (same layout for each set)
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
	descriptorSetLayoutInfo.bindingCount = CZ_COUNTOF(descriptorSetLayoutBindings);
	descriptorSetLayoutInfo.pBindings = descriptorSetLayoutBindings;

	VkDescriptorSetLayout descriptorSetLayout;
	VK_CALLR(vkCreateDescriptorSetLayout, device, &descriptorSetLayoutInfo, allocator, &descriptorSetLayout);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->descriptorSetLayout = descriptorSetLayout;

	// Create descriptor pool (all sets allocated from same pool)
	VkDescriptorPoolSize descriptorPoolSizes[1];
	descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[0].descriptorCount = inoutsPerHeap * 2;

	VkDescriptorPoolCreateInfo descriptorPoolInfo = {0};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.maxSets = inoutsPerHeap;
	descriptorPoolInfo.poolSizeCount = CZ_COUNTOF(descriptorPoolSizes);
	descriptorPoolInfo.pPoolSizes = descriptorPoolSizes;

	VkDescriptorPool descriptorPool;
	VK_CALLR(vkCreateDescriptorPool, device, &descriptorPoolInfo, allocator, &descriptorPool);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->descriptorPool = descriptorPool;

	// Allocate descriptor sets (one per inout-buffer)
	allocSize = inoutsPerHeap * sizeof(VkDescriptorSet);
	VkDescriptorSet* descriptorSets = dyrecord_malloc(gpuRecord, allocSize);
	if CZ_NOEXPECT (!descriptorSets) { dyrecord_destroy(localRecord); return false; }
	gpu->descriptorSets = descriptorSets;

	allocSize = inoutsPerHeap * sizeof(VkDescriptorSetLayout);
	VkDescriptorSetLayout* descriptorSetLayouts = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!descriptorSetLayouts) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		descriptorSetLayouts[i] = descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocInfo = {0};
	descriptorSetAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocInfo.descriptorPool = descriptorPool;
	descriptorSetAllocInfo.descriptorSetCount = inoutsPerHeap;
	descriptorSetAllocInfo.pSetLayouts = descriptorSetLayouts;

	VK_CALLR(vkAllocateDescriptorSets, device, &descriptorSetAllocInfo, descriptorSets);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Write to each descriptor set
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkWriteDescriptorSet);

	VkWriteDescriptorSet* writeDescriptorSets = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!writeDescriptorSets) { dyrecord_destroy(localRecord); return false; }

	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkDescriptorBufferInfo[2]);

	VkDescriptorBufferInfo (*descriptorBufferInfos)[2] = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!descriptorBufferInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			// Binding 0
			descriptorBufferInfos[j][0].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[j][0].offset = bytesPerInout * k;
			descriptorBufferInfos[j][0].range = bytesPerIn;

			// Binding 1
			descriptorBufferInfos[j][1].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[j][1].offset = bytesPerInout * k + bytesPerIn;
			descriptorBufferInfos[j][1].range = bytesPerOut;

			writeDescriptorSets[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[j].dstSet = descriptorSets[j];
			writeDescriptorSets[j].dstBinding = 0; // Start from this binding in the descriptor set
			writeDescriptorSets[j].dstArrayElement = 0; // Start from this descriptor in the binding
			writeDescriptorSets[j].descriptorCount = CZ_COUNTOF(descriptorBufferInfos[j]);
			writeDescriptorSets[j].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[j].pBufferInfo = descriptorBufferInfos[j];
		}
	}

	uint32_t writeCount = inoutsPerHeap;
	uint32_t copyCount = 0;
	VkCopyDescriptorSet* copyDescriptorSets = NULL;

	VK_CALL(vkUpdateDescriptorSets, device, writeCount, writeDescriptorSets, copyCount, copyDescriptorSets);

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

	dyrecord_destroy(localRecord);
	return true;
}

bool create_pipeline(struct Gpu* restrict gpu)
{
	const VkAllocationCallbacks* allocator = gpu->allocator;

	VkDevice device = gpu->device;
	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t workgroupSize = gpu->workgroupSize;

	uint32_t computeFamilyTimestampValidBits = gpu->computeFamilyTimestampValidBits;
	uint32_t transferFamilyTimestampValidBits = gpu->transferFamilyTimestampValidBits;

	uint32_t spvVerMajor = gpu->spvVerMajor;
	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

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
	enum CzEndianness endianness = get_endianness();

	sprintf(entryPointName, "main-%u-%lu", endianness, czgConfig.iterSize);

	if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_QUIET) {
		printf("Selected shader: %s\nSelected entry point: %s\n\n", shaderName, entryPointName);
	}

	size_t shaderSize;
	struct CzFileFlags shaderFileFlags = {0};
	shaderFileFlags.relativeToExe = true;

	enum CzResult czres = czFileSize(shaderName, &shaderSize, shaderFileFlags);
	if CZ_NOEXPECT (czres) {
		if (czres == CZ_RESULT_NO_FILE) {
			log_error(stderr, "Selected shader '%s' not found", shaderName);
		}
		dyrecord_destroy(localRecord);
		return false;
	}

	uint32_t* shaderCode = dyrecord_malloc(localRecord, shaderSize);
	if CZ_NOEXPECT (!shaderCode) { dyrecord_destroy(localRecord); return false; }

	size_t shaderOffset = 0;
	czres = czReadFile(shaderName, shaderCode, shaderSize, shaderOffset, shaderFileFlags);
	if CZ_NOEXPECT (czres) { dyrecord_destroy(localRecord); return false; }

	size_t cacheSize;
	struct CzFileFlags cacheFileFlags = {0};
	cacheFileFlags.relativeToExe = true;

	czres = czFileSize(CZ_PIPELINE_CACHE_NAME, &cacheSize, cacheFileFlags);
	if CZ_NOEXPECT (czres && czres != CZ_RESULT_NO_FILE) { dyrecord_destroy(localRecord); return false; }

	void* cacheData = NULL;
	if (czres == CZ_RESULT_SUCCESS) {
		cacheData = dyrecord_malloc(localRecord, cacheSize);
		if CZ_NOEXPECT (!cacheData) { dyrecord_destroy(localRecord); return false; }

		size_t cacheOffset = 0;
		czres = czReadFile(CZ_PIPELINE_CACHE_NAME, cacheData, cacheSize, cacheOffset, cacheFileFlags);
		if CZ_NOEXPECT (czres) { dyrecord_destroy(localRecord); return false; }
	}

	VkShaderModuleCreateInfo shaderInfo = {0};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderInfo.codeSize = shaderSize;
	shaderInfo.pCode = shaderCode;

	VkShaderModule shader = VK_NULL_HANDLE;
	if (!gpu->usingMaintenance5) {
		VK_CALLR(vkCreateShaderModule, device, &shaderInfo, allocator, &shader);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		gpu->shaderModule = shader;
	}

	VkPipelineCacheCreateFlags cacheFlags = 0;
	if (gpu->usingPipelineCreationCacheControl) {
		cacheFlags |= VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
	}

	VkPipelineCacheCreateInfo cacheInfo = {0};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	cacheInfo.flags = cacheFlags;
	cacheInfo.initialDataSize = cacheSize;
	cacheInfo.pInitialData = cacheData;

	VkPipelineCache cache;
	VK_CALLR(vkCreatePipelineCache, device, &cacheInfo, allocator, &cache);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->pipelineCache = cache;

	VkDescriptorSetLayout descriptorSetLayouts[1];
	descriptorSetLayouts[0] = descriptorSetLayout;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {0};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = CZ_COUNTOF(descriptorSetLayouts);
	pipelineLayoutInfo.pSetLayouts = descriptorSetLayouts;

	VkPipelineLayout pipelineLayout;
	VK_CALLR(vkCreatePipelineLayout, device, &pipelineLayoutInfo, allocator, &pipelineLayout);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->pipelineLayout = pipelineLayout;

	uint32_t specialisationData[1];
	specialisationData[0] = workgroupSize;

	VkSpecializationMapEntry specialisationMapEntries[1];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset = 0;
	specialisationMapEntries[0].size = sizeof(specialisationData[0]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = CZ_COUNTOF(specialisationMapEntries);
	specialisationInfo.pMapEntries = specialisationMapEntries;
	specialisationInfo.dataSize = sizeof(specialisationData);
	specialisationInfo.pData = specialisationData;

	VkPipelineShaderStageCreateFlags shaderStageFlags = 0;
	if (gpu->usingSubgroupSizeControl) {
		shaderStageFlags |= VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT;
	}

	VkPipelineShaderStageCreateInfo shaderStageInfo = {0};
	shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStageInfo.pNext = gpu->usingMaintenance5 ? &shaderInfo : NULL;
	shaderStageInfo.flags = shaderStageFlags;
	shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	shaderStageInfo.module = shader;
	shaderStageInfo.pName = entryPointName;
	shaderStageInfo.pSpecializationInfo = &specialisationInfo;

	VkPipelineCreateFlags pipelineFlags = 0;
	if (gpu->usingPipelineExecutableProperties) {
		pipelineFlags |= VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
		pipelineFlags |= VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR;
	}

	VkComputePipelineCreateInfo pipelineInfos[1] = {0};
	pipelineInfos[0].sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	pipelineInfos[0].flags = pipelineFlags;
	pipelineInfos[0].stage = shaderStageInfo;
	pipelineInfos[0].layout = pipelineLayout;

	VkPipeline pipeline;
	VK_CALLR(vkCreateComputePipelines, device, cache, CZ_COUNTOF(pipelineInfos), pipelineInfos, allocator, &pipeline);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->pipeline = pipeline;

	bool bres = save_pipeline_cache(device, cache, CZ_PIPELINE_CACHE_NAME);
	if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }

	if (computeFamilyTimestampValidBits || transferFamilyTimestampValidBits) {
		VkQueryPoolCreateInfo queryPoolInfo = {0};
		queryPoolInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolInfo.queryCount = inoutsPerHeap * 4; // Two per copy, two per dispatch

		VkQueryPool queryPool;
		VK_CALLR(vkCreateQueryPool, device, &queryPoolInfo, allocator, &queryPool);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		gpu->queryPool = queryPool;
	}

	if (gpu->usingPipelineExecutableProperties) {
		bres = capture_pipeline(device, pipeline);
		if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }
	}

	VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, allocator);
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	VK_CALL(vkDestroyPipelineCache, device, cache, allocator);
	gpu->pipelineCache = VK_NULL_HANDLE;

	if (!gpu->usingMaintenance5) {
		VK_CALL(vkDestroyShaderModule, device, shader, allocator);
		gpu->shaderModule = VK_NULL_HANDLE;
	}

	dyrecord_destroy(localRecord);
	return true;
}

static bool record_initial_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	const VkCopyBufferInfo2* inBufferCopyInfos,
	const VkDependencyInfo* dependencyInfo,
	uint32_t buffersPerHeap)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VK_CALLR(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if CZ_NOEXPECT (vkres) { return false; }

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL(vkCmdCopyBuffer2KHR, cmdBuffer, &inBufferCopyInfos[i]);
	}

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, dependencyInfo);

	VK_CALLR(vkEndCommandBuffer, cmdBuffer);
	if CZ_NOEXPECT (vkres) { return false; }

	return true;
}

static bool record_transfer_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	const VkCopyBufferInfo2* inBufferCopyInfo,
	const VkCopyBufferInfo2* outBufferCopyInfo,
	const VkDependencyInfo* dependencyInfos,
	VkQueryPool queryPool,
	uint32_t firstQuery,
	uint32_t timestampValidBits)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CALLR(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if CZ_NOEXPECT (vkres) { return false; }

	if (timestampValidBits) {
		// TODO Remove (vkCmdResetQueryPool not guaranteed to work on dedicated transfer queue families)
		uint32_t queryCount = 2;
		VK_CALL(vkCmdResetQueryPool, cmdBuffer, queryPool, firstQuery, queryCount);

		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
		uint32_t query = firstQuery;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	VK_CALL(vkCmdCopyBuffer2KHR, cmdBuffer, inBufferCopyInfo);
	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[0]);

	VK_CALL(vkCmdCopyBuffer2KHR, cmdBuffer, outBufferCopyInfo);
	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[1]);

	if (timestampValidBits) {
		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_COPY_BIT;
		uint32_t query = firstQuery + 1;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	VK_CALLR(vkEndCommandBuffer, cmdBuffer);
	if CZ_NOEXPECT (vkres) { return false; }

	return true;
}

static bool record_compute_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	VkPipeline pipeline,
	const VkBindDescriptorSetsInfo* bindDescriptorSetsInfo,
	const VkDependencyInfo* dependencyInfos,
	VkQueryPool queryPool,
	uint32_t firstQuery,
	uint32_t timestampValidBits,
	uint32_t workgroupCount)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {0};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	VK_CALLR(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if CZ_NOEXPECT (vkres) { return false; }

	if (timestampValidBits) {
		uint32_t queryCount = 2;
		VK_CALL(vkCmdResetQueryPool, cmdBuffer, queryPool, firstQuery, queryCount);

		VkPipelineStageFlags2 stage = VK_PIPELINE_STAGE_2_NONE;
		uint32_t query = firstQuery;

		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, stage, queryPool, query);
	}

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &dependencyInfos[0]);
	VK_CALL(vkCmdBindDescriptorSets2KHR, cmdBuffer, bindDescriptorSetsInfo);

	VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
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

	VK_CALLR(vkEndCommandBuffer, cmdBuffer);
	if CZ_NOEXPECT (vkres) { return false; }

	return true;
}

bool create_commands(struct Gpu* restrict gpu)
{
	DyRecord gpuRecord = gpu->allocRecord;

	const VkAllocationCallbacks* allocator = gpu->allocator;
	const VkBuffer* hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* descriptorSets  = gpu->descriptorSets;

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

	uint32_t computeFamilyIndex = gpu->computeFamilyIndex;
	uint32_t transferFamilyIndex = gpu->transferFamilyIndex;
	uint32_t computeFamilyTimestampValidBits = gpu->computeFamilyTimestampValidBits;
	uint32_t transferFamilyTimestampValidBits = gpu->transferFamilyTimestampValidBits;

	VkResult vkres;
	size_t allocCount;
	size_t allocSize;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	// Create initial command pool
	VkCommandPoolCreateInfo initialCmdPoolInfo = {0};
	initialCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	initialCmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	initialCmdPoolInfo.queueFamilyIndex = transferFamilyIndex;

	VkCommandPool initialCmdPool;
	VK_CALLR(vkCreateCommandPool, device, &initialCmdPoolInfo, allocator, &initialCmdPool);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->initialCmdPool = initialCmdPool;

	// Create compute command pool (all compute command buffers allocated from this pool)
	VkCommandPoolCreateInfo computeCmdPoolInfo = {0};
	computeCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computeCmdPoolInfo.queueFamilyIndex = computeFamilyIndex;

	VkCommandPool computeCmdPool;
	VK_CALLR(vkCreateCommandPool, device, &computeCmdPoolInfo, allocator, &computeCmdPool);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->computeCmdPool = computeCmdPool;

	// Create transfer command pool (all transfer command buffers allocated from this pool)
	VkCommandPoolCreateInfo transferCmdPoolInfo = {0};
	transferCmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferCmdPoolInfo.queueFamilyIndex = transferFamilyIndex;

	VkCommandPool transferCmdPool;
	VK_CALLR(vkCreateCommandPool, device, &transferCmdPoolInfo, allocator, &transferCmdPool);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->transferCmdPool = transferCmdPool;

	// Allocate initial command buffer
	VkCommandBufferAllocateInfo initialCmdBufferAllocInfo = {0};
	initialCmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	initialCmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	initialCmdBufferAllocInfo.commandPool = initialCmdPool;
	initialCmdBufferAllocInfo.commandBufferCount = 1;

	VkCommandBuffer initialCmdBuffer;
	VK_CALLR(vkAllocateCommandBuffers, device, &initialCmdBufferAllocInfo, &initialCmdBuffer);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	gpu->initialCmdBuffer = initialCmdBuffer;

	// Allocate compute command buffers (one per inout-buffer)
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkCommandBuffer);

	VkCommandBuffer* computeCmdBuffers = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeCmdBuffers) { dyrecord_destroy(localRecord); return false; }
	gpu->computeCmdBuffers = computeCmdBuffers;

	VkCommandBufferAllocateInfo computeCmdBufferAllocInfo = {0};
	computeCmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	computeCmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeCmdBufferAllocInfo.commandPool = computeCmdPool;
	computeCmdBufferAllocInfo.commandBufferCount = inoutsPerHeap;

	VK_CALLR(vkAllocateCommandBuffers, device, &computeCmdBufferAllocInfo, computeCmdBuffers);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Allocate transfer command buffers (one per inout-buffer)
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkCommandBuffer);

	VkCommandBuffer* transferCmdBuffers = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferCmdBuffers) { dyrecord_destroy(localRecord); return false; }
	gpu->transferCmdBuffers = transferCmdBuffers;

	VkCommandBufferAllocateInfo transferCmdBufferAllocInfo = {0};
	transferCmdBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transferCmdBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCmdBufferAllocInfo.commandPool = transferCmdPool;
	transferCmdBufferAllocInfo.commandBufferCount = inoutsPerHeap;

	VK_CALLR(vkAllocateCommandBuffers, device, &transferCmdBufferAllocInfo, transferCmdBuffers);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Specify in-buffer copy regions (same region layout per buffer)
	allocCount = inoutsPerBuffer;
	allocSize = sizeof(VkBufferCopy2);

	VkBufferCopy2* inBufferRegions = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!inBufferRegions) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerBuffer; i++) {
		inBufferRegions[i].sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		inBufferRegions[i].srcOffset = bytesPerInout * i;
		inBufferRegions[i].dstOffset = bytesPerInout * i;
		inBufferRegions[i].size = bytesPerIn;
	}

	// Specify out-buffer copy regions (same region layout per buffer)
	allocCount = inoutsPerBuffer;
	allocSize = sizeof(VkBufferCopy2);

	VkBufferCopy2* outBufferRegions = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!outBufferRegions) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerBuffer; i++) {
		outBufferRegions[i].sType = VK_STRUCTURE_TYPE_BUFFER_COPY_2;
		outBufferRegions[i].srcOffset = bytesPerInout * i + bytesPerIn;
		outBufferRegions[i].dstOffset = bytesPerInout * i + bytesPerIn;
		outBufferRegions[i].size = bytesPerOut;
	}

	// Specify initial buffer copies
	allocCount = buffersPerHeap;
	allocSize = sizeof(VkCopyBufferInfo2);

	VkCopyBufferInfo2* initialBufferCopyInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!initialBufferCopyInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		initialBufferCopyInfos[i].sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
		initialBufferCopyInfos[i].srcBuffer = hostVisibleBuffers[i];
		initialBufferCopyInfos[i].dstBuffer = deviceLocalBuffers[i];
		initialBufferCopyInfos[i].regionCount = inoutsPerBuffer;
		initialBufferCopyInfos[i].pRegions = inBufferRegions;
	}

	// Specify in-buffer copies
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkCopyBufferInfo2);

	VkCopyBufferInfo2* inBufferCopyInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!inBufferCopyInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer hostVisibleBuffer = hostVisibleBuffers[i];
		VkBuffer deviceLocalBuffer = deviceLocalBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			inBufferCopyInfos[j].sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
			inBufferCopyInfos[j].srcBuffer = hostVisibleBuffer;
			inBufferCopyInfos[j].dstBuffer = deviceLocalBuffer;
			inBufferCopyInfos[j].regionCount = 1;
			inBufferCopyInfos[j].pRegions = &inBufferRegions[k];
		}
	}

	// Specify out-buffer copies
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkCopyBufferInfo2);

	VkCopyBufferInfo2* outBufferCopyInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!outBufferCopyInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer hostVisibleBuffer = hostVisibleBuffers[i];
		VkBuffer deviceLocalBuffer = deviceLocalBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			outBufferCopyInfos[j].sType = VK_STRUCTURE_TYPE_COPY_BUFFER_INFO_2;
			outBufferCopyInfos[j].srcBuffer = deviceLocalBuffer;
			outBufferCopyInfos[j].dstBuffer = hostVisibleBuffer;
			outBufferCopyInfos[j].regionCount = 1;
			outBufferCopyInfos[j].pRegions = &outBufferRegions[k];
		}
	}

	// Specify descriptor set bindings
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkBindDescriptorSetsInfo);

	VkBindDescriptorSetsInfo* bindDescriptorSetsInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!bindDescriptorSetsInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		bindDescriptorSetsInfos[i].sType = VK_STRUCTURE_TYPE_BIND_DESCRIPTOR_SETS_INFO;
		bindDescriptorSetsInfos[i].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
		bindDescriptorSetsInfos[i].layout = pipelineLayout;
		bindDescriptorSetsInfos[i].firstSet = 0;
		bindDescriptorSetsInfos[i].descriptorSetCount = 1;
		bindDescriptorSetsInfos[i].pDescriptorSets = &descriptorSets[i];
	}

	// Specify buffer memory barriers for initial command buffer
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkBufferMemoryBarrier2);

	VkBufferMemoryBarrier2* initialBufferMemoryBarriers = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!initialBufferMemoryBarriers) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer deviceLocalBuffer = deviceLocalBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			initialBufferMemoryBarriers[j].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			initialBufferMemoryBarriers[j].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			initialBufferMemoryBarriers[j].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			initialBufferMemoryBarriers[j].srcQueueFamilyIndex = transferFamilyIndex;
			initialBufferMemoryBarriers[j].dstQueueFamilyIndex = computeFamilyIndex;
			initialBufferMemoryBarriers[j].buffer = deviceLocalBuffer;
			initialBufferMemoryBarriers[j].offset = bytesPerInout * k;
			initialBufferMemoryBarriers[j].size = bytesPerIn;
		}
	}

	// Specify buffer memory barriers for compute command buffer
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkBufferMemoryBarrier2[2]);

	VkBufferMemoryBarrier2 (*computeBufferMemoryBarriers)[2] = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeBufferMemoryBarriers) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer deviceLocalBuffer = deviceLocalBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			computeBufferMemoryBarriers[j][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			computeBufferMemoryBarriers[j][0].dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][0].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
			computeBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferFamilyIndex;
			computeBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeFamilyIndex;
			computeBufferMemoryBarriers[j][0].buffer = deviceLocalBuffer;
			computeBufferMemoryBarriers[j][0].offset = bytesPerInout * k;
			computeBufferMemoryBarriers[j][0].size = bytesPerIn;

			computeBufferMemoryBarriers[j][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			computeBufferMemoryBarriers[j][1].srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][1].srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			computeBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeFamilyIndex;
			computeBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferFamilyIndex;
			computeBufferMemoryBarriers[j][1].buffer = deviceLocalBuffer;
			computeBufferMemoryBarriers[j][1].offset = bytesPerInout * k + bytesPerIn;
			computeBufferMemoryBarriers[j][1].size = bytesPerOut;
		}
	}

	// Specify buffer memory barriers for transfer command buffer
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkBufferMemoryBarrier2[3]);

	VkBufferMemoryBarrier2 (*transferBufferMemoryBarriers)[3] = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferBufferMemoryBarriers) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VkBuffer hostVisibleBuffer = hostVisibleBuffers[i];
		VkBuffer deviceLocalBuffer = deviceLocalBuffers[i];

		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			transferBufferMemoryBarriers[j][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][0].srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferFamilyIndex;
			transferBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeFamilyIndex;
			transferBufferMemoryBarriers[j][0].buffer = deviceLocalBuffer;
			transferBufferMemoryBarriers[j][0].offset = bytesPerInout * k;
			transferBufferMemoryBarriers[j][0].size = bytesPerIn;

			transferBufferMemoryBarriers[j][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
			transferBufferMemoryBarriers[j][1].dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][1].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			transferBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeFamilyIndex;
			transferBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferFamilyIndex;
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
		}
	}

	// Specify dependency info for initial command buffer
	VkDependencyInfo initialDependencyInfo = {0};
	initialDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
	initialDependencyInfo.bufferMemoryBarrierCount = inoutsPerHeap;
	initialDependencyInfo.pBufferMemoryBarriers = initialBufferMemoryBarriers;

	// Specify dependency infos for compute command buffers
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkDependencyInfo[2]);

	VkDependencyInfo (*computeDependencyInfos)[2] = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeDependencyInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		computeDependencyInfos[i][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		computeDependencyInfos[i][0].bufferMemoryBarrierCount = 1;
		computeDependencyInfos[i][0].pBufferMemoryBarriers = &computeBufferMemoryBarriers[i][0];

		computeDependencyInfos[i][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		computeDependencyInfos[i][1].bufferMemoryBarrierCount = 1;
		computeDependencyInfos[i][1].pBufferMemoryBarriers = &computeBufferMemoryBarriers[i][1];
	}

	// Specify dependency infos for transfer command buffers
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkDependencyInfo[2]);

	VkDependencyInfo (*transferDependencyInfos)[2] = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferDependencyInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferDependencyInfos[i][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		transferDependencyInfos[i][0].bufferMemoryBarrierCount = 2;
		transferDependencyInfos[i][0].pBufferMemoryBarriers = &transferBufferMemoryBarriers[i][0];

		transferDependencyInfos[i][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		transferDependencyInfos[i][1].bufferMemoryBarrierCount = 1;
		transferDependencyInfos[i][1].pBufferMemoryBarriers = &transferBufferMemoryBarriers[i][2];
	}

	// Record initial command buffer
	bool bres = record_initial_cmdbuffer(
		initialCmdBuffer, initialBufferCopyInfos, &initialDependencyInfo, buffersPerHeap);

	if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }

	// Record compute command buffers
	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		uint32_t firstQuery = i * 4;
		bres = record_compute_cmdbuffer(
			computeCmdBuffers[i], pipeline, &bindDescriptorSetsInfos[i], computeDependencyInfos[i], queryPool,
			firstQuery, computeFamilyTimestampValidBits, workgroupCount);

		if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }
	}

	// Record transfer command buffers
	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		uint32_t firstQuery = i * 4 + 2;
		bres = record_transfer_cmdbuffer(
			transferCmdBuffers[i], &inBufferCopyInfos[i], &outBufferCopyInfos[i], transferDependencyInfos[i], queryPool,
			firstQuery, transferFamilyTimestampValidBits);

		if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }
	}

	// Create semaphores (one per inout-buffer)
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphore);

	VkSemaphore* semaphores = dyrecord_calloc(gpuRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!semaphores) { dyrecord_destroy(localRecord); return false; }
	gpu->semaphores = semaphores;

	VkSemaphoreTypeCreateInfo semaphoreTypeInfo = {0};
	semaphoreTypeInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	semaphoreTypeInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	semaphoreTypeInfo.initialValue = 0;

	VkSemaphoreCreateInfo semaphoreInfo = {0};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreInfo.pNext = &semaphoreTypeInfo;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VkSemaphore semaphore;
		VK_CALLR(vkCreateSemaphore, device, &semaphoreInfo, allocator, &semaphore);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
		semaphores[i] = semaphore;
	}

#ifndef NDEBUG
	set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) transferCmdPool, "Transfer");
	set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) computeCmdPool, "Compute");

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			char objectName[68];
			char specs[60];

			sprintf(
				specs,
				", Inout %" PRIu32 "/%" PRIu32 ", Buffer %" PRIu32 "/%" PRIu32,
				k + 1, inoutsPerBuffer, i + 1, buffersPerHeap);

			strcpy(objectName, "Compute");
			strcat(objectName, specs);

			set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) computeCmdBuffers[j], objectName);

			strcpy(objectName, "Transfer");
			strcat(objectName, specs);

			set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) transferCmdBuffers[j], objectName);
		}
	}
#endif

	VK_CALL(vkDestroyPipelineLayout, device, pipelineLayout, allocator);
	gpu->pipelineLayout = VK_NULL_HANDLE;

	dyrecord_destroy(localRecord);
	return true;
}

bool submit_commands(struct Gpu* restrict gpu)
{
	const VkAllocationCallbacks* allocator = gpu->allocator;
	const VkDeviceMemory* hostVisibleMemories = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer* computeCmdBuffers = gpu->computeCmdBuffers;
	const VkCommandBuffer* transferCmdBuffers = gpu->transferCmdBuffers;
	const VkSemaphore* semaphores = gpu->semaphores;

	StartValue* const* mappedInBuffers = gpu->mappedInBuffers;
	StopTime* const* mappedOutBuffers = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkQueue computeQueue = gpu->computeQueue;
	VkQueue transferQueue = gpu->transferQueue;

	VkCommandPool initialCmdPool = gpu->initialCmdPool;
	VkCommandBuffer initialCmdBuffer = gpu->initialCmdBuffer;

	VkQueryPool queryPool = gpu->queryPool;

	VkDeviceSize bytesPerIn = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t valuesPerInout = gpu->valuesPerInout;
	uint32_t valuesPerHeap = gpu->valuesPerHeap;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	uint32_t computeFamilyTimestampValidBits = gpu->computeFamilyTimestampValidBits;
	uint32_t transferFamilyTimestampValidBits = gpu->transferFamilyTimestampValidBits;

	double timestampPeriod = (double) gpu->timestampPeriod;
	bool hostNonCoherent = gpu->hostNonCoherent;

	VkResult vkres;
	size_t allocCount;
	size_t allocSize;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }
	
	// Create array of starting values with longest total stopping times
	size_t elmSize = sizeof(StartValue);
	size_t elmCount = 32;

	DyArray bestStartValues = dyarray_create(elmSize, elmCount);
	if CZ_NOEXPECT (!bestStartValues) { dyrecord_destroy(localRecord); return false; }

	bool bres = dyrecord_add(localRecord, bestStartValues, dyarray_destroy_stub);
	if CZ_NOEXPECT (!bres) { dyarray_destroy(bestStartValues); dyrecord_destroy(localRecord); return false; }

	// Create array of longest total stopping times found
	elmSize = sizeof(StopTime);
	elmCount = 32;

	DyArray bestStopTimes = dyarray_create(elmSize, elmCount);
	if CZ_NOEXPECT (!bestStopTimes) { dyrecord_destroy(localRecord); return false; }

	bres = dyrecord_add(localRecord, bestStopTimes, dyarray_destroy_stub);
	if CZ_NOEXPECT (!bres) { dyarray_destroy(bestStopTimes); dyrecord_destroy(localRecord); return false; }

	// Use progress file, if it exists
	struct Position position = {0};
	position.val0mod1off[0] = 1;
	position.curStartValue = 3;

	size_t fileSize;
	struct CzFileFlags fileFlags = {0};
	fileFlags.relativeToExe = true;

	enum CzResult czres = czFileSize(CZ_PROGRESS_FILE_NAME, &fileSize, fileFlags);
	if CZ_NOEXPECT (czres && czres != CZ_RESULT_NO_FILE) { dyrecord_destroy(localRecord); return false; }

	if (!czgConfig.restart && czres == CZ_RESULT_SUCCESS) {
		uint64_t val0mod1off0Upper, val0mod1off0Lower;
		uint64_t val0mod1off1Upper, val0mod1off1Lower;
		uint64_t val0mod1off2Upper, val0mod1off2Lower;
		uint64_t val1mod6off0Upper, val1mod6off0Lower;
		uint64_t val1mod6off1Upper, val1mod6off1Lower;
		uint64_t val1mod6off2Upper, val1mod6off2Lower;
		uint64_t curValueUpper, curValueLower;
		uint16_t bestTime;

		bres = read_text(
			CZ_PROGRESS_FILE_NAME,
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

		if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }

		position.val0mod1off[0] = CZ_UINT128(val0mod1off0Upper, val0mod1off0Lower);
		position.val0mod1off[1] = CZ_UINT128(val0mod1off1Upper, val0mod1off1Lower);
		position.val0mod1off[2] = CZ_UINT128(val0mod1off2Upper, val0mod1off2Lower);

		position.val1mod6off[0] = CZ_UINT128(val1mod6off0Upper, val1mod6off0Lower);
		position.val1mod6off[1] = CZ_UINT128(val1mod6off1Upper, val1mod6off1Lower);
		position.val1mod6off[2] = CZ_UINT128(val1mod6off2Upper, val1mod6off2Lower);

		position.curStartValue = CZ_UINT128(curValueUpper, curValueLower);
		position.bestStopTime = bestTime;
	}

	// Specify mapped memory ranges of host visible in-buffers
	VkMappedMemoryRange* inBuffersMappedRanges = NULL;

	if (hostNonCoherent) {
		allocCount = inoutsPerHeap;
		allocSize = sizeof(VkMappedMemoryRange);

		inBuffersMappedRanges = dyrecord_calloc(localRecord, allocCount, allocSize);
		if CZ_NOEXPECT (!inBuffersMappedRanges) { dyrecord_destroy(localRecord); return false; }

		for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
			for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
				inBuffersMappedRanges[j].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				inBuffersMappedRanges[j].memory = hostVisibleMemories[i];
				inBuffersMappedRanges[j].offset = bytesPerInout * k;
				inBuffersMappedRanges[j].size = bytesPerIn;
			}
		}
	}

	// Specify mapped memory ranges of host visible out-buffers
	VkMappedMemoryRange* outBuffersMappedRanges = NULL;

	if (hostNonCoherent) {
		allocCount = inoutsPerHeap;
		allocSize = sizeof(VkMappedMemoryRange);

		outBuffersMappedRanges = dyrecord_calloc(localRecord, allocCount, allocSize);
		if CZ_NOEXPECT (!outBuffersMappedRanges) { dyrecord_destroy(localRecord); return false; }

		for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
			for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
				outBuffersMappedRanges[j].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				outBuffersMappedRanges[j].memory = hostVisibleMemories[i];
				outBuffersMappedRanges[j].offset = bytesPerInout * k + bytesPerIn;
				outBuffersMappedRanges[j].size = bytesPerOut;
			}
		}
	}

	// Specify initial command buffer for submission
	VkCommandBufferSubmitInfo initialCmdBufferSubmitInfo = {0};
	initialCmdBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
	initialCmdBufferSubmitInfo.commandBuffer = initialCmdBuffer;

	// Specify transfer command buffers for submission
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkCommandBufferSubmitInfo);

	VkCommandBufferSubmitInfo* transferCmdBufferSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferCmdBufferSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferCmdBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		transferCmdBufferSubmitInfos[i].commandBuffer = transferCmdBuffers[i];
	}

	// Specify compute command buffers for submission
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkCommandBufferSubmitInfo);

	VkCommandBufferSubmitInfo* computeCmdBufferSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeCmdBufferSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		computeCmdBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		computeCmdBufferSubmitInfos[i].commandBuffer = computeCmdBuffers[i];
	}

	// Specify semaphore wait operations for submissions to transfer queue
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphoreSubmitInfo);

	VkSemaphoreSubmitInfo* transferWaitSemaphoreSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferWaitSemaphoreSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		transferWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferWaitSemaphoreSubmitInfos[i].value = 0;
		transferWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include acquire op
	}

	// Specify semaphore signal operations for submissions to transfer queue
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphoreSubmitInfo);

	VkSemaphoreSubmitInfo* transferSignalSemaphoreSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferSignalSemaphoreSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		transferSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferSignalSemaphoreSubmitInfos[i].value = 1;
		transferSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include release op
	}

	// Specify semaphore wait operations for submissions to compute queue
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphoreSubmitInfo);

	VkSemaphoreSubmitInfo* computeWaitSemaphoreSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeWaitSemaphoreSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		computeWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		computeWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeWaitSemaphoreSubmitInfos[i].value = 1;
		computeWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include acquire op
	}

	// Specify semaphore signal operations for submissions to compute queue
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphoreSubmitInfo);

	VkSemaphoreSubmitInfo* computeSignalSemaphoreSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeSignalSemaphoreSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		computeSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		computeSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeSignalSemaphoreSubmitInfos[i].value = 2;
		computeSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include release op
	}

	// Specify submission of initial command buffer
	VkSubmitInfo2 initialSubmitInfo = {0};
	initialSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
	initialSubmitInfo.commandBufferInfoCount = 1;
	initialSubmitInfo.pCommandBufferInfos = &initialCmdBufferSubmitInfo;
	initialSubmitInfo.signalSemaphoreInfoCount = inoutsPerHeap;
	initialSubmitInfo.pSignalSemaphoreInfos = transferSignalSemaphoreSubmitInfos;

	// Specify submissions to transfer queue
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSubmitInfo2);

	VkSubmitInfo2* transferSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		transferSubmitInfos[i].waitSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pWaitSemaphoreInfos = &transferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos[i].commandBufferInfoCount = 1;
		transferSubmitInfos[i].pCommandBufferInfos = &transferCmdBufferSubmitInfos[i];
		transferSubmitInfos[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pSignalSemaphoreInfos = &transferSignalSemaphoreSubmitInfos[i];
	}

	// Specify submissions to compute queue
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSubmitInfo2);

	VkSubmitInfo2* computeSubmitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeSubmitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		computeSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		computeSubmitInfos[i].waitSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pWaitSemaphoreInfos = &computeWaitSemaphoreSubmitInfos[i];
		computeSubmitInfos[i].commandBufferInfoCount = 1;
		computeSubmitInfos[i].pCommandBufferInfos = &computeCmdBufferSubmitInfos[i];
		computeSubmitInfos[i].signalSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pSignalSemaphoreInfos = &computeSignalSemaphoreSubmitInfos[i];
	}

	// Specify semaphore wait operations for transfer queue on host
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphoreWaitInfo);

	VkSemaphoreWaitInfo* transferSemaphoreWaitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!transferSemaphoreWaitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		transferSemaphoreWaitInfos[i].semaphoreCount = 1;
		transferSemaphoreWaitInfos[i].pSemaphores = &semaphores[i];
		transferSemaphoreWaitInfos[i].pValues = &transferSignalSemaphoreSubmitInfos[i].value;
	}

	// Specify semaphore wait operations for compute queue on host
	allocCount = inoutsPerHeap;
	allocSize = sizeof(VkSemaphoreWaitInfo);

	VkSemaphoreWaitInfo* computeSemaphoreWaitInfos = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!computeSemaphoreWaitInfos) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		computeSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		computeSemaphoreWaitInfos[i].semaphoreCount = 1;
		computeSemaphoreWaitInfos[i].pSemaphores = &semaphores[i];
		computeSemaphoreWaitInfos[i].pValues = &computeSignalSemaphoreSubmitInfos[i].value;
	}

	// Create array keeping track of initial tested starting value for each inout-buffer
	allocCount = inoutsPerHeap;
	allocSize = sizeof(StartValue);

	StartValue* testedValues = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!testedValues) { dyrecord_destroy(localRecord); return false; }

	// Create thread to wait for user input
	atomic_bool input;
	atomic_init(&input, false);

	pthread_t waitThread;
	int ires = pthread_create(&waitThread, NULL, wait_for_input, &input);
	if CZ_NOEXPECT (ires) { PCREATE_FAILURE(ires); dyrecord_destroy(localRecord); return false; }

	clock_t totalBmStart = clock();
	StartValue tested = position.curStartValue;

	// Write starting values to mapped in-buffers
	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		testedValues[i] = tested;
		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);
		tested += valuesPerInout * 4;
	}

	if (hostNonCoherent) {
		VK_CALLR(vkFlushMappedMemoryRanges, device, inoutsPerHeap, inBuffersMappedRanges);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	}

	// Initiate the first cycle
	uint32_t submitInfoCount = 1;
	VK_CALLR(vkQueueSubmit2KHR, transferQueue, submitInfoCount, &initialSubmitInfo, VK_NULL_HANDLE);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	submitInfoCount = inoutsPerHeap;
	VK_CALLR(vkQueueSubmit2KHR, computeQueue, submitInfoCount, computeSubmitInfos, VK_NULL_HANDLE);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		// Wait for transfers to complete execution
		uint64_t transferTimeout = UINT64_MAX;
		VK_CALLR(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[i], transferTimeout);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

		// Write starting values to mapped in-buffer
		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);

		// Update semaphore wait/signal values
		transferWaitSemaphoreSubmitInfos[i].value += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	if (hostNonCoherent) {
		VK_CALLR(vkFlushMappedMemoryRanges, device, inoutsPerHeap, inBuffersMappedRanges);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	}

	// Complete the first cycle (and initiate the second)
	submitInfoCount = inoutsPerHeap;
	VK_CALLR(vkQueueSubmit2KHR, transferQueue, submitInfoCount, transferSubmitInfos, VK_NULL_HANDLE);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Initial command buffer and pool are no longer needed
	VK_CALL(vkDestroyCommandPool, device, initialCmdPool, allocator);
	gpu->initialCmdPool = VK_NULL_HANDLE;

	StartValue total = 0;
	StartValue initialStartValue = position.curStartValue;

	// ===== Enter main loop =====
	for (uint64_t i = 0; i < czgConfig.maxLoops && !atomic_load(&input); i++) {
		clock_t mainLoopBmStart = clock();
		StartValue initialValue = position.curStartValue;

		double readBmTotal = 0;
		double writeBmTotal = 0;
		double waitComputeBmTotal = 0;
		double waitTransferBmTotal = 0;
		double computeBmTotal = 0;
		double transferBmTotal = 0;

		if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_SILENT) {
			printf("Loop #%" PRIu64 "\n", i + 1);
		}

		/*
		 * The following loop has two invocations of the vkGetQueryPoolResults function. On my Windows/Linux PC, these
		 * functions always return VK_SUCCESS. But on my Macbook, they very rarely return VK_NOT_READY. I'm yet to find
		 * a consistent pattern regarding when these failures occur, nor have I found a way to reliably replicate them.
		 * TODO Figure out what on Earth is going on here???
		 */
		for (uint32_t j = 0; j < inoutsPerHeap; j++) {
			double computeBmark = 0;
			double transferBmark = 0;

			// Wait for dispatch to complete execution
			clock_t waitComputeBmStart = clock();

			uint64_t computeTimeout = UINT64_MAX;
			VK_CALLR(vkWaitSemaphoresKHR, device, &computeSemaphoreWaitInfos[j], computeTimeout);
			if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

			clock_t waitComputeBmEnd = clock();

			// Calculate approx time taken for dispatch to execute
			if (computeFamilyTimestampValidBits) {
				uint32_t firstQuery = j * 4;
				uint32_t queryCount = 2;
				VkQueryResultFlags queryFlags = VK_QUERY_RESULT_64_BIT;

				uint64_t timestamps[2];
				VK_CALLR(vkGetQueryPoolResults,
					device, queryPool, firstQuery, queryCount, sizeof(timestamps), timestamps, sizeof(timestamps[0]),
					queryFlags);

				if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
				computeBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			// Update semaphore wait/signal values
			computeWaitSemaphoreSubmitInfos[j].value += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			// Resubmit compute command buffer for next cycle
			submitInfoCount = 1;
			VK_CALLR(vkQueueSubmit2KHR, computeQueue, submitInfoCount, &computeSubmitInfos[j], VK_NULL_HANDLE);
			if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

			// Wait for transfers to complete execution
			clock_t waitTransferBmStart = clock();

			uint64_t transferTimeout = UINT64_MAX;
			VK_CALLR(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[j], transferTimeout);
			if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

			clock_t waitTransferBmEnd = clock();

			// Calculate approx time taken for transfers to execute
			if (transferFamilyTimestampValidBits) {
				uint32_t firstQuery = j * 4 + 2;
				uint32_t queryCount = 2;
				VkQueryResultFlags queryFlags = VK_QUERY_RESULT_64_BIT;

				uint64_t timestamps[2];
				VK_CALLR(vkGetQueryPoolResults,
					device, queryPool, firstQuery, queryCount, sizeof(timestamps), timestamps, sizeof(timestamps[0]),
					queryFlags);

				if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
				transferBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (hostNonCoherent) {
				uint32_t rangeCount = 1;
				VK_CALLR(vkInvalidateMappedMemoryRanges, device, rangeCount, &outBuffersMappedRanges[j]);
				if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
			}

			// Read total stopping times from mapped out-buffer
			clock_t readBmStart = clock();
			read_outbuffer(mappedOutBuffers[j], &position, bestStartValues, bestStopTimes, valuesPerInout);
			clock_t readBmEnd = clock();

			// Write starting values to mapped in-buffer
			clock_t writeBmStart = clock();
			write_inbuffer(mappedInBuffers[j], &testedValues[j], valuesPerInout, valuesPerHeap);
			clock_t writeBmEnd = clock();

			if (hostNonCoherent) {
				uint32_t rangeCount = 1;
				VK_CALLR(vkFlushMappedMemoryRanges, device, rangeCount, &inBuffersMappedRanges[j]);
				if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
			}

			// Update semaphore wait/signal values
			transferWaitSemaphoreSubmitInfos[j].value += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			// Resubmit transfer command buffer for next cycle
			submitInfoCount = 1;
			VK_CALLR(vkQueueSubmit2KHR, transferQueue, submitInfoCount, &transferSubmitInfos[j], VK_NULL_HANDLE);
			if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return NULL; }

			// Calculate and display benchmarks for current inout-buffer
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

			if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_QUIET) {
				printf(
					"Inout-buffer %" PRIu32 "/%" PRIu32 "\n"
					"\tReading buffers:    %8.0fms\n"
					"\tWriting buffers:    %8.0fms\n"
					"\tCompute execution:  %8.0fms\n"
					"\tTransfer execution: %8.0fms\n"
					"\tIdle (compute):     %8.0fms\n"
					"\tIdle (transfer):    %8.0fms\n",
					j + 1, inoutsPerHeap,
					readBmark,        writeBmark,
					computeBmark,     transferBmark,
					waitComputeBmark, waitTransferBmark);
			}
		}

		// Calculate and display benchmarks for current loop iteration
		total += valuesPerHeap * 4;

		clock_t mainLoopBmEnd = clock();
		double mainLoopBmark = get_benchmark(mainLoopBmStart, mainLoopBmEnd);

		double readBmAvg = readBmTotal / (double) inoutsPerHeap;
		double writeBmAvg = writeBmTotal / (double) inoutsPerHeap;
		double computeBmAvg = computeBmTotal / (double) inoutsPerHeap;
		double transferBmAvg = transferBmTotal / (double) inoutsPerHeap;
		double waitComputeBmAvg = waitComputeBmTotal / (double) inoutsPerHeap;
		double waitTransferBmAvg = waitTransferBmTotal / (double) inoutsPerHeap;

		switch (czgConfig.outputLevel) {
		case CZ_OUTPUT_LEVEL_QUIET:
			printf(
				"Main loop: %.0fms\n"
				"Current value: 0x %016" PRIx64 " %016" PRIx64 "\n\n",
				mainLoopBmark,
				CZ_UINT128_UPPER(position.curStartValue - 3), CZ_UINT128_LOWER(position.curStartValue - 3));

			break;

		case CZ_OUTPUT_LEVEL_DEFAULT:
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
				readBmAvg,        writeBmAvg,
				computeBmAvg,     transferBmAvg,
				waitComputeBmAvg, waitTransferBmAvg,
				CZ_UINT128_UPPER(initialValue - 2),           CZ_UINT128_LOWER(initialValue - 2),
				CZ_UINT128_UPPER(position.curStartValue - 3), CZ_UINT128_LOWER(position.curStartValue - 3));

			break;

		case CZ_OUTPUT_LEVEL_VERBOSE:
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
				readBmTotal,        readBmAvg,        writeBmTotal,        writeBmAvg,
				computeBmTotal,     computeBmAvg,     transferBmTotal,     transferBmAvg,
				waitComputeBmTotal, waitComputeBmAvg, waitTransferBmTotal, waitTransferBmAvg,
				CZ_UINT128_UPPER(initialValue - 2),           CZ_UINT128_LOWER(initialValue - 2),
				CZ_UINT128_UPPER(position.curStartValue - 3), CZ_UINT128_LOWER(position.curStartValue - 3));

			break;

		default:
			break;
		}
	}
	CZ_NEWLINE();

	clock_t totalBmEnd = clock();
	double totalBmark = get_benchmark(totalBmStart, totalBmEnd);

	// Stop waiting thread
	if (atomic_load(&input)) {
		ires = pthread_join(waitThread, NULL);
		if CZ_NOEXPECT (ires) { PJOIN_FAILURE(ires); dyrecord_destroy(localRecord); return false; }
	}
	else {
		atomic_store(&input, true);
		ires = pthread_cancel(waitThread);
		if CZ_NOEXPECT (ires) { PCANCEL_FAILURE(ires); dyrecord_destroy(localRecord); return false; }
	}

	// Display results of calculations
	if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_SILENT) {
		printf(
			"Set of starting values tested: [0x %016" PRIx64 " %016" PRIx64 ", 0x %016" PRIx64 " %016" PRIx64 "]\n",
			CZ_UINT128_UPPER(initialStartValue - 2),      CZ_UINT128_LOWER(initialStartValue - 2),
			CZ_UINT128_UPPER(position.curStartValue - 3), CZ_UINT128_LOWER(position.curStartValue - 3));
	}

	size_t bestCount = dyarray_size(bestStartValues);

	if (bestCount) {
		printf(
			"New highest total stopping times (%zu):\n"
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
			i + 1, CZ_UINT128_UPPER(startValue), CZ_UINT128_LOWER(startValue), stopTime);
	}

	if (czgConfig.outputLevel > CZ_OUTPUT_LEVEL_SILENT) {
		double valuesPerSecond = (double) (1000 * total) / totalBmark;

		printf(
			"\n"
			"Time: %.3fms\n"
			"Speed: %.3f/s\n",
			totalBmark, valuesPerSecond);
	}

	// Write current position to progress file
	if (!czgConfig.restart) {
		bres = write_text(
			CZ_PROGRESS_FILE_NAME,
			"%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n"
			"%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n"
			"%016" PRIx64 " %016" PRIx64 "\n%04"  PRIx16,
			CZ_UINT128_UPPER(position.val0mod1off[0]), CZ_UINT128_LOWER(position.val0mod1off[0]),
			CZ_UINT128_UPPER(position.val0mod1off[1]), CZ_UINT128_LOWER(position.val0mod1off[1]),
			CZ_UINT128_UPPER(position.val0mod1off[2]), CZ_UINT128_LOWER(position.val0mod1off[2]),
			CZ_UINT128_UPPER(position.val1mod6off[0]), CZ_UINT128_LOWER(position.val1mod6off[0]),
			CZ_UINT128_UPPER(position.val1mod6off[1]), CZ_UINT128_LOWER(position.val1mod6off[1]),
			CZ_UINT128_UPPER(position.val1mod6off[2]), CZ_UINT128_LOWER(position.val1mod6off[2]),
			CZ_UINT128_UPPER(position.curStartValue),  CZ_UINT128_LOWER(position.curStartValue),
			position.bestStopTime);

		if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }
	}

	dyrecord_destroy(localRecord);
	return true;
}

bool destroy_gpu(struct Gpu* restrict gpu)
{
	VkInstance instance = volkGetLoadedInstance();

	const VkAllocationCallbacks* allocator = gpu->allocator;
	const VkBuffer* hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDeviceMemory* hostVisibleMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* deviceLocalMemories = gpu->deviceLocalDeviceMemories;
	const VkSemaphore* semaphores = gpu->semaphores;

	VkDevice device = gpu->device;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	VkResult vkres;

	if (device) {
		VK_CALL(vkDestroyShaderModule, device, gpu->shaderModule, allocator);
		VK_CALL(vkDestroyPipelineCache, device, gpu->pipelineCache, allocator);
		VK_CALL(vkDestroyPipelineLayout, device, gpu->pipelineLayout, allocator);
		VK_CALL(vkDestroyDescriptorSetLayout, device, gpu->descriptorSetLayout, allocator);

		// Make sure no command buffers are in the pending state
		VK_CALLR(vkDeviceWaitIdle, device);

		if (semaphores) {
			for (uint32_t i = 0; i < inoutsPerHeap; i++) {
				VK_CALL(vkDestroySemaphore, device, semaphores[i], allocator);
			}
		}

		VK_CALL(vkDestroyCommandPool, device, gpu->initialCmdPool, allocator);
		VK_CALL(vkDestroyCommandPool, device, gpu->computeCmdPool, allocator);
		VK_CALL(vkDestroyCommandPool, device, gpu->transferCmdPool, allocator);

		VK_CALL(vkDestroyPipeline, device, gpu->pipeline, allocator);
		VK_CALL(vkDestroyQueryPool, device, gpu->queryPool, allocator);
		VK_CALL(vkDestroyDescriptorPool, device, gpu->descriptorPool, allocator);

		if (hostVisibleBuffers) {
			for (uint32_t i = 0; i < buffersPerHeap; i++) {
				VK_CALL(vkDestroyBuffer, device, hostVisibleBuffers[i], allocator);
			}
		}
		if (deviceLocalBuffers) {
			for (uint32_t i = 0; i < buffersPerHeap; i++) {
				VK_CALL(vkDestroyBuffer, device, deviceLocalBuffers[i], allocator);
			}
		}
		if (hostVisibleMemories) {
			for (uint32_t i = 0; i < buffersPerHeap; i++) {
				VK_CALL(vkFreeMemory, device, hostVisibleMemories[i], allocator);
			}
		}
		if (deviceLocalMemories) {
			for (uint32_t i = 0; i < buffersPerHeap; i++) {
				VK_CALL(vkFreeMemory, device, deviceLocalMemories[i], allocator);
			}
		}

		VK_CALL(vkDestroyDevice, device, allocator);
	}

	if (instance) {
		if (vkDestroyDebugUtilsMessengerEXT) {
			VK_CALL(vkDestroyDebugUtilsMessengerEXT, instance, gpu->debugMessenger, allocator);
		}

		VK_CALL(vkDestroyInstance, instance, allocator);
	}

	volkFinalize();
	dyrecord_destroy(gpu->allocRecord);
	return true;
}

bool capture_pipeline(VkDevice device, VkPipeline pipeline)
{
	VkResult vkres;

	DyRecord localRecord = dyrecord_create();
	if CZ_NOEXPECT (!localRecord) { return false; }

	VkPipelineInfoKHR pipelineInfo = {0};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
	pipelineInfo.pipeline = pipeline;

	// Get pipeline executable properties
	uint32_t executableCount;
	VK_CALLR(vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, NULL);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	if CZ_NOEXPECT (!executableCount) {
		log_warning(stdout, "No pipeline executables available for capture");
		dyrecord_destroy(localRecord);
		return true;
	}

	size_t allocCount = executableCount;
	size_t allocSize = sizeof(VkPipelineExecutablePropertiesKHR);

	VkPipelineExecutablePropertiesKHR* executablesProperties = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!executablesProperties) { dyrecord_destroy(localRecord); return false; }

	VK_CALLR(vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, executablesProperties);
	if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

	// Specify pipeline executables to query
	allocCount = executableCount;
	allocSize = sizeof(VkPipelineExecutableInfoKHR);

	VkPipelineExecutableInfoKHR* executablesInfo = dyrecord_calloc(localRecord, allocCount, allocSize);
	if CZ_NOEXPECT (!executablesInfo) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < executableCount; i++) {
		executablesInfo[i].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
		executablesInfo[i].pipeline = pipeline;
		executablesInfo[i].executableIndex = i;
	}

	// Get statistics for each pipeline executable
	allocSize = executableCount * sizeof(uint32_t);
	uint32_t* statisticCounts = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!statisticCounts) { dyrecord_destroy(localRecord); return false; }

	allocSize = executableCount * sizeof(VkPipelineExecutableStatisticKHR*);
	VkPipelineExecutableStatisticKHR** executablesStatistics = dyrecord_malloc(localRecord, allocSize);
	if CZ_NOEXPECT (!executablesStatistics) { dyrecord_destroy(localRecord); return false; }

	for (uint32_t i = 0; i < executableCount; i++) {
		VK_CALLR(vkGetPipelineExecutableStatisticsKHR, device, &executablesInfo[i], &statisticCounts[i], NULL);
		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }

		allocCount = statisticCounts[i];
		allocSize = sizeof(VkPipelineExecutableStatisticKHR);

		executablesStatistics[i] = dyrecord_calloc(localRecord, allocCount, allocSize);
		if CZ_NOEXPECT (!executablesStatistics[i]) { dyrecord_destroy(localRecord); return false; }

		for (uint32_t j = 0; j < statisticCounts[i]; j++) {
			executablesStatistics[i][j].sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
		}

		VK_CALLR(vkGetPipelineExecutableStatisticsKHR,
			device, &executablesInfo[i], &statisticCounts[i], executablesStatistics[i]);

		if CZ_NOEXPECT (vkres) { dyrecord_destroy(localRecord); return false; }
	}

	// Construct message with info on each pipeline executable
	DyString message = dystring_create(1024);
	if CZ_NOEXPECT (!message) { dyrecord_destroy(localRecord); return false; }

	bool bres = dyrecord_add(localRecord, message, dystring_destroy_stub);
	if CZ_NOEXPECT (!bres) { dystring_destroy(message); dyrecord_destroy(localRecord); return false; }

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

	uint32_t statisticTotal = 0;

	for (uint32_t i = 0; i < executableCount; i++) {
		statisticTotal += statisticCounts[i];
	}

	// Write collected info to file
	const char* rawMessage = dystring_raw(message);
	bres = write_text(
		czgConfig.capturePath,
		"PIPELINE CAPTURE DATA\n"
		"\n"
		"Total # executables: %" PRIu32 "\n"
		"Total # statistics:  %" PRIu32 "\n"
		"\n"
		"%s",
		executableCount, statisticTotal, rawMessage);
	
	if CZ_NOEXPECT (!bres) { dyrecord_destroy(localRecord); return false; }

	dyrecord_destroy(localRecord);
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
	CZ_ASSUME(*firstStartValue % 8 == 3);
	CZ_ASSUME(valuesPerInout % 128 == 0);
	CZ_ASSUME(valuesPerInout != 0);

	StartValue startValue = *firstStartValue;

	for (uint32_t i = 0; i < valuesPerInout; i++) {
		mappedInBuffer[i] = startValue;
		startValue += 4;
	}

	*firstStartValue += valuesPerHeap * 4;
}

void read_outbuffer(
	const StopTime* restrict mappedOutBuffer,
	struct Position* restrict position,
	DyArray bestStartValues,
	DyArray bestStopTimes,
	uint32_t valuesPerInout)
{
	CZ_ASSUME(position->curStartValue % 8 == 3);
	CZ_ASSUME(valuesPerInout % 128 == 0);
	CZ_ASSUME(valuesPerInout != 0);

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
			for (uint32_t j = 2; j < CZ_COUNTOF(val0mod1off); j++) {
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
			for (uint32_t j = 1; j < CZ_COUNTOF(val0mod1off); j++) {
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
			for (uint32_t j = 3; j < CZ_COUNTOF(val0mod1off); j++) {
				if (val0mod1off[j - 2] || val0mod1off[j] * 4 != curValue) { continue; }

				val0mod1off[j - 2] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 5

		if (curValue % 6 == 1) {
			for (uint32_t j = 0; j < CZ_COUNTOF(val0mod1off); j++) {
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
			for (uint32_t j = 2; j < CZ_COUNTOF(val0mod1off); j++) {
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
			for (uint32_t j = 1; j < CZ_COUNTOF(val0mod1off); j++) {
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
			for (uint32_t j = 4; j < CZ_COUNTOF(val0mod1off); j++) {
				if (val0mod1off[j - 3] || val0mod1off[j] * 8 != curValue) { continue; }

				val0mod1off[j - 3] = curValue;
				break;
			}
		}

		curValue++; // curValue % 8 == 1

		for (uint32_t j = 0; j < CZ_COUNTOF(val0mod1off); j++) {
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
