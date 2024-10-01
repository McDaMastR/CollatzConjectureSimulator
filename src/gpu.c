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
#include "dyarray.h"

static void DyArray_destroy_stub(void* array)
{
	DyArray_destroy((DyArray*) array);
}

typedef struct DyData
{
	void* data;
	void (*free)(void*);
} DyData;

static void free_recursive(DyArray* array)
{
	size_t count = DyArray_size(array);

	for (size_t i = 0; i < count; i++) {
		DyData dyData;
		DyArray_get(array, &dyData, i);
		dyData.free(dyData.data);
	}

	DyArray_destroy(array);
}

bool create_instance(Gpu* gpu)
{
	BEGIN_FUNC

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 3);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	vkres = volkInitialize();
	if (vkres) {
		VKINIT_FAILURE(vkres)
		free_recursive(dyMem);
		return false;
	}

	uint32_t instApiVersion = volkGetInstanceVersion();
	if (instApiVersion == VK_API_VERSION_1_0) {
		VKVERS_FAILURE(instApiVersion)
		free_recursive(dyMem);
		return false;
	}

	if (LOG_VULKAN_ALLOCATIONS) {
		bool bres = init_alloc_logfile();
#ifndef NDEBUG
		if (!bres) {
			free_recursive(dyMem);
			return false;
		}
#endif
	}

#ifndef NDEBUG
	bool bres = init_debug_logfile();
	if (!bres) {
		free_recursive(dyMem);
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback = debug_callback;
	debugUtilsMessengerCreateInfo.pUserData       = &g_callbackData;
#endif

	uint32_t layerPropertyCount;
	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerPropertyCount, NULL)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEnumerateInstanceLayerProperties)
		free_recursive(dyMem);
		return false;
	}
#endif

	uint32_t extensionPropertyCount;
	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extensionPropertyCount, NULL)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEnumerateInstanceExtensionProperties)
		free_recursive(dyMem);
		return false;
	}
#endif

	size_t size =
		layerPropertyCount     * sizeof(VkLayerProperties) +
		extensionPropertyCount * sizeof(VkExtensionProperties);

	VkLayerProperties* layersProperties = (VkLayerProperties*) malloc(size);
#ifndef NDEBUG
	if (!layersProperties && size) {
		MALLOC_FAILURE(layersProperties, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {layersProperties, free};
	DyArray_append(dyMem, &dyData);

	VkExtensionProperties* extensionsProperties = (VkExtensionProperties*) (layersProperties + layerPropertyCount);

	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerPropertyCount, layersProperties)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEnumerateInstanceLayerProperties)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extensionPropertyCount, extensionsProperties)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEnumerateInstanceExtensionProperties)
		free_recursive(dyMem);
		return false;
	}
#endif

	DyArray* enabledLayers = DyArray_create(sizeof(const char*), 4);
#ifndef NDEBUG
	if (!enabledLayers) {
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {enabledLayers, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < layerPropertyCount; i++) {
		const char* layerName = layersProperties[i].layerName;

		if (EXTENSION_LAYERS && !strcmp(layerName, VK_KHR_SYNCHRONIZATION_2_LAYER_NAME))
			DyArray_append(enabledLayers, &layerName);

		else if (EXTENSION_LAYERS && !strcmp(layerName, VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME))
			DyArray_append(enabledLayers, &layerName);

		else if (PROFILE_LAYERS && !strcmp(layerName, VK_KHR_PROFILES_LAYER_NAME))
			DyArray_append(enabledLayers, &layerName);

		else if (VALIDATION_LAYERS && !strcmp(layerName, VK_KHR_VALIDATION_LAYER_NAME))
			DyArray_append(enabledLayers, &layerName);
	}

	const void*  nextChain = NULL;
	const void** next      = &nextChain;

	bool usingPortabilityEnumeration = false;
	bool usingDebugUtils             = false;

	DyArray* enabledExtensions = DyArray_create(sizeof(const char*), 2);
#ifndef NDEBUG
	if (!enabledExtensions) {
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {enabledExtensions, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < extensionPropertyCount; i++) {
		const char* extensionName = extensionsProperties[i].extensionName;

		if (!strcmp(extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			DyArray_append(enabledExtensions, &extensionName);
			usingPortabilityEnumeration = true;
		}

#ifndef NDEBUG
		else if (!strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			DyArray_append(enabledExtensions, &extensionName);
			usingDebugUtils = true;

			*next = &debugUtilsMessengerCreateInfo;
			next  = &debugUtilsMessengerCreateInfo.pNext;
		}
#endif
	}

	VkApplicationInfo applicationInfo = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	applicationInfo.pApplicationName = PROGRAM_NAME;
	applicationInfo.apiVersion       = VK_API_VERSION_1_3;

	uint32_t     enabledLayerCount = (uint32_t) DyArray_size(enabledLayers);
	const char** enabledLayerNames = (const char**) DyArray_raw(enabledLayers);

	uint32_t     enabledExtensionCount = (uint32_t) DyArray_size(enabledExtensions);
	const char** enabledExtensionNames = (const char**) DyArray_raw(enabledExtensions);

	VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instanceCreateInfo.pNext                   = nextChain;
	instanceCreateInfo.flags                   = usingPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceCreateInfo.pApplicationInfo        = &applicationInfo;
	instanceCreateInfo.enabledLayerCount       = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames     = enabledLayerNames;
	instanceCreateInfo.enabledExtensionCount   = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;

	printf("Enabled instance layers (%u):\n", enabledLayerCount);
	for (uint32_t i = 0; i < enabledLayerCount; i++)
		printf("\t%u) %s\n", i + 1, enabledLayerNames[i]);
	NEWLINE()

	printf("Enabled instance extensions (%u):\n", enabledExtensionCount);
	for (uint32_t i = 0; i < enabledExtensionCount; i++)
		printf("\t%u) %s\n", i + 1, enabledExtensionNames[i]);
	NEWLINE()

	VkInstance instance;
	VK_CALL_RES(vkCreateInstance, &instanceCreateInfo, g_allocator, &instance)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateInstance)
		free_recursive(dyMem);
		return false;
	}
#endif

	free_recursive(dyMem);
	volkLoadInstanceOnly(instance);

#ifndef NDEBUG
	if (usingDebugUtils) {
		VK_CALL_RES(vkCreateDebugUtilsMessengerEXT, instance, &debugUtilsMessengerCreateInfo, g_allocator, &gpu->debugUtilsMessenger)
		if (vkres)
			VULKAN_FAILURE(vkCreateDebugUtilsMessengerEXT)
	}
#endif

	END_FUNC
	return true;
}

bool select_device(Gpu* gpu)
{
	BEGIN_FUNC

	VkInstance instance = volkGetLoadedInstance();

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 2);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	uint32_t physicalDeviceCount;
	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &physicalDeviceCount, NULL)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEnumeratePhysicalDevices)
		free_recursive(dyMem);
		return false;
	}

	if (!physicalDeviceCount) {
		fputs("Runtime failure\nNo physical devices are accessible to the Vulkan instance\n\n", stderr);
		free_recursive(dyMem);
		return false;
	}
#endif

	size_t size =
		physicalDeviceCount *     sizeof(VkPhysicalDevice) +
		physicalDeviceCount *     sizeof(VkQueueFamilyProperties2*) +
		physicalDeviceCount *     sizeof(VkExtensionProperties*) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceMemoryProperties2) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceProperties2) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceFeatures2) +
		physicalDeviceCount *     sizeof(VkPhysicalDevice16BitStorageFeatures) +
		physicalDeviceCount *     sizeof(VkPhysicalDevice8BitStorageFeaturesKHR) +
		physicalDeviceCount * 2 * sizeof(uint32_t);

	VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*) malloc(size);
#ifndef NDEBUG
	if (!physicalDevices) {
		MALLOC_FAILURE(physicalDevices, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {physicalDevices, free};
	DyArray_append(dyMem, &dyData);

	VkQueueFamilyProperties2** queueFamiliesProperties2 = (VkQueueFamilyProperties2**) (physicalDevices          + physicalDeviceCount);
	VkExtensionProperties**    extensionsProperties     = (VkExtensionProperties**)    (queueFamiliesProperties2 + physicalDeviceCount);

	VkPhysicalDeviceMemoryProperties2* physicalDevicesMemoryProperties2 = (VkPhysicalDeviceMemoryProperties2*) (extensionsProperties             + physicalDeviceCount);
	VkPhysicalDeviceProperties2*       physicalDevicesProperties2       = (VkPhysicalDeviceProperties2*)       (physicalDevicesMemoryProperties2 + physicalDeviceCount);

	VkPhysicalDeviceFeatures2*              physicalDevicesFeatures2            = (VkPhysicalDeviceFeatures2*)              (physicalDevicesProperties2          + physicalDeviceCount);
	VkPhysicalDevice16BitStorageFeatures*   physicalDevices16BitStorageFeatures = (VkPhysicalDevice16BitStorageFeatures*)   (physicalDevicesFeatures2            + physicalDeviceCount);
	VkPhysicalDevice8BitStorageFeaturesKHR* physicalDevices8BitStorageFeatures  = (VkPhysicalDevice8BitStorageFeaturesKHR*) (physicalDevices16BitStorageFeatures + physicalDeviceCount);

	uint32_t* extensionPropertyCounts   = (uint32_t*) (physicalDevices8BitStorageFeatures + physicalDeviceCount);
	uint32_t* queueFamilyPropertyCounts = (uint32_t*) (extensionPropertyCounts            + physicalDeviceCount);

	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &physicalDeviceCount, physicalDevices)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEnumeratePhysicalDevices)
		free_recursive(dyMem);
		return false;
	}
#endif

	size_t queueFamilyTotal = 0;
	size_t extensionTotal   = 0;

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, physicalDevices[i], &queueFamilyPropertyCounts[i], NULL)

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, physicalDevices[i], NULL, &extensionPropertyCounts[i], NULL)
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkEnumerateDeviceExtensionProperties)
			free_recursive(dyMem);
			return false;
		}
#endif

		queueFamilyTotal += queueFamilyPropertyCounts[i];
		extensionTotal   += extensionPropertyCounts[i];
	}

	size =
		queueFamilyTotal * sizeof(VkQueueFamilyProperties2) +
		extensionTotal   * sizeof(VkExtensionProperties);

	queueFamiliesProperties2[0] = (VkQueueFamilyProperties2*) malloc(size);
#ifndef NDEBUG
	if (!queueFamiliesProperties2[0]) {
		MALLOC_FAILURE(queueFamiliesProperties2[0], size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {queueFamiliesProperties2[0], free};
	DyArray_append(dyMem, &dyData);

	extensionsProperties[0] = (VkExtensionProperties*) (queueFamiliesProperties2[0] + queueFamilyTotal);

	for (uint32_t i = 1; i < physicalDeviceCount; i++) {
		queueFamiliesProperties2[i] = queueFamiliesProperties2[i - 1] + queueFamilyPropertyCounts[i - 1];
		extensionsProperties[i]     = extensionsProperties[i - 1]     + extensionPropertyCounts[i - 1];
	}

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, physicalDevices[i], NULL, &extensionPropertyCounts[i], extensionsProperties[i])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkEnumerateDeviceExtensionProperties)
			free_recursive(dyMem);
			return false;
		}
#endif

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++)
			queueFamiliesProperties2[i][j] = (VkQueueFamilyProperties2) {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, physicalDevices[i], &queueFamilyPropertyCounts[i], queueFamiliesProperties2[i])

		physicalDevicesMemoryProperties2[i] = (VkPhysicalDeviceMemoryProperties2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevices[i], &physicalDevicesMemoryProperties2[i])

		physicalDevicesProperties2[i] = (VkPhysicalDeviceProperties2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevices[i], &physicalDevicesProperties2[i])

		physicalDevices8BitStorageFeatures[i] = (VkPhysicalDevice8BitStorageFeaturesKHR) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR};

		physicalDevices16BitStorageFeatures[i] = (VkPhysicalDevice16BitStorageFeatures) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
		physicalDevices16BitStorageFeatures[i].pNext = &physicalDevices8BitStorageFeatures[i];

		physicalDevicesFeatures2[i] = (VkPhysicalDeviceFeatures2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
		physicalDevicesFeatures2[i].pNext = &physicalDevices16BitStorageFeatures[i];

		VK_CALL(vkGetPhysicalDeviceFeatures2, physicalDevices[i], &physicalDevicesFeatures2[i])
	}

	uint32_t pdvIndex     = ~0U; // Physical device index
	uint32_t highestScore = 0;

	bool using8BitStorage                  = false;
	bool usingBufferDeviceAddress          = false;
	bool usingMaintenance4                 = false;
	bool usingMemoryBudget                 = false;
	bool usingMemoryPriority               = false;
	bool usingPipelineCreationCacheControl = false;
	bool usingPortabilitySubset            = false;
	bool usingShaderInt16                  = false;
	bool usingShaderInt64                  = false;
	bool usingSpirv14                      = false;
	bool usingSubgroupSizeControl          = false;
	bool usingVulkan12                     = false;
	bool usingVulkan13                     = false;

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		bool hasVulkan11 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasVulkan12 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_2;
		bool hasVulkan13 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_3;

		bool hasDiscreteGpu = physicalDevicesProperties2[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		bool hasShaderInt16 = physicalDevicesFeatures2[i].features.shaderInt16;
		bool hasShaderInt64 = physicalDevicesFeatures2[i].features.shaderInt64;

		bool hasStorageBuffer16BitAccess          = physicalDevices16BitStorageFeatures[i].storageBuffer16BitAccess;
		bool hasUniformAndStorageBuffer8BitAccess = physicalDevices8BitStorageFeatures[i].uniformAndStorageBuffer8BitAccess;

		bool hasDedicatedTransfer = false;
		bool hasDedicatedCompute  = false;
		bool hasCompute           = false;

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++) {
			VkQueueFlags queueFlags = queueFamiliesProperties2[i][j].queueFamilyProperties.queueFlags;

			bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
			bool isDedicatedCompute  = queueFlags == VK_QUEUE_COMPUTE_BIT;
			bool isCompute           = queueFlags & VK_QUEUE_COMPUTE_BIT;

			if (isDedicatedTransfer) hasDedicatedTransfer = true;
			if (isDedicatedCompute)  hasDedicatedCompute  = true;
			if (isCompute)           hasCompute           = true;
		}

		bool hasDeviceNonHost = false;
		bool hasDeviceLocal   = false;

		bool hasHostCachedNonCoherent = false;
		bool hasHostCached            = false;
		bool hasHostNonCoherent       = false;
		bool hasHostVisible           = false;

		for (uint32_t j = 0; j < physicalDevicesMemoryProperties2[i].memoryProperties.memoryTypeCount; j++) {
			VkMemoryPropertyFlags propertyFlags = physicalDevicesMemoryProperties2[i].memoryProperties.memoryTypes[j].propertyFlags;

			bool isDeviceLocal     = propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			bool isHostVisible     = propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			bool isHostCoherent    = propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			bool isHostCached      = propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			bool isLazilyAllocated = propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
			bool isProtected       = propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT;
			bool isDeviceCoherent  = propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD;

			if (isLazilyAllocated) continue;
			if (isProtected)       continue;
			if (isDeviceCoherent)  continue;

			if (isDeviceLocal) {
				hasDeviceLocal = true;
				if (!isHostVisible) hasDeviceNonHost = true;
			}

			if (isHostVisible) {
				hasHostVisible = true;
				if (isHostCached && !isHostCoherent) hasHostCachedNonCoherent = true;
				if (isHostCached)                    hasHostCached            = true;
				if (!isHostCoherent)                 hasHostNonCoherent       = true;
			}
		}

		bool has8BitStorage                  = false;
		bool hasBufferDeviceAddress          = false;
		bool hasMaintenance4                 = false;
		bool hasPortabilitySubset            = false;
		bool hasSpirv14                      = false;
		bool hasSynchronization2             = false;
		bool hasTimelineSemaphore            = false;
		bool hasMemoryBudget                 = false;
		bool hasMemoryPriority               = false;
		bool hasPipelineCreationCacheControl = false;
		bool hasSubgroupSizeControl          = false;

		for (uint32_t j = 0; j < extensionPropertyCounts[i]; j++) {
			const char* extensionName = extensionsProperties[i][j].extensionName;

			if      (!strcmp(extensionName, VK_KHR_8BIT_STORAGE_EXTENSION_NAME))                    has8BitStorage                  = true;
			else if (!strcmp(extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME))           hasBufferDeviceAddress          = true;
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))                   hasMaintenance4                 = true;
			else if (!strcmp(extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))              hasPortabilitySubset            = true;
			else if (!strcmp(extensionName, VK_KHR_SPIRV_1_4_EXTENSION_NAME))                       hasSpirv14                      = true;
			else if (!strcmp(extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))               hasSynchronization2             = true;
			else if (!strcmp(extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))              hasTimelineSemaphore            = true;
			else if (!strcmp(extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))                   hasMemoryBudget                 = true;
			else if (!strcmp(extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))                 hasMemoryPriority               = true;
			else if (!strcmp(extensionName, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME)) hasPipelineCreationCacheControl = true;
			else if (!strcmp(extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME))           hasSubgroupSizeControl          = true;
		}

		uint32_t currentScore = 1;

		if (!hasVulkan11)                 continue;
		if (!hasStorageBuffer16BitAccess) continue;

		if (!hasDeviceLocal) continue;
		if (!hasHostVisible) continue;

		if (!hasCompute) continue;

		if (!hasSynchronization2)  continue;
		if (!hasTimelineSemaphore) continue;

		if (hasVulkan12) currentScore += 50;
		if (hasVulkan13) currentScore += 50;

		if (hasDiscreteGpu) currentScore += 10000;

		if (hasShaderInt16) currentScore += 1000;
		if (hasShaderInt64) currentScore += 1000;

		if (hasDeviceNonHost) currentScore += 50;

		if (hasHostCachedNonCoherent) currentScore += 1000;
		else if (hasHostCached)       currentScore += 500;
		else if (hasHostNonCoherent)  currentScore += 100;

		if (hasDedicatedTransfer) currentScore += 100;
		if (hasDedicatedCompute)  currentScore += 100;

		if (hasBufferDeviceAddress)                                 currentScore += 10;
		if (hasMaintenance4)                                        currentScore += 10;
		if (hasMemoryBudget)                                        currentScore += 10;
		if (hasMemoryPriority)                                      currentScore += 10;
		if (hasPipelineCreationCacheControl)                        currentScore += 10;
		if (hasSpirv14)                                             currentScore += 10;
		if (hasSubgroupSizeControl)                                 currentScore += 10;
		if (has8BitStorage && hasUniformAndStorageBuffer8BitAccess) currentScore += 10;

		if (currentScore > highestScore) {
			highestScore = currentScore;
			pdvIndex     = i;

			using8BitStorage                  = has8BitStorage && hasUniformAndStorageBuffer8BitAccess;
			usingBufferDeviceAddress          = hasBufferDeviceAddress;
			usingMaintenance4                 = hasMaintenance4;
			usingMemoryBudget                 = hasMemoryBudget;
			usingMemoryPriority               = hasMemoryPriority;
			usingPipelineCreationCacheControl = hasPipelineCreationCacheControl;
			usingPortabilitySubset            = hasPortabilitySubset;
			usingShaderInt16                  = hasShaderInt16;
			usingShaderInt64                  = hasShaderInt64;
			usingSpirv14                      = hasSpirv14;
			usingSubgroupSizeControl          = hasSubgroupSizeControl;
			usingVulkan12                     = hasVulkan12;
			usingVulkan13                     = hasVulkan13;
		}
	}

	if (pdvIndex == ~0U) {
		fputs("Runtime failure\nNo physical device meets program requirements\nSee device_requirements.md for full physical device requirements\n\n", stderr);
		free_recursive(dyMem);
		return false;
	}

	uint32_t transferQueueFamilyIndex = ~0U;
	uint32_t computeQueueFamilyIndex  = ~0U;

	bool hasDedicatedTransfer = false;
	bool hasTransfer          = false;

	bool hasDedicatedCompute = false;
	bool hasCompute          = false;

	for (uint32_t i = 0; i < queueFamilyPropertyCounts[pdvIndex]; i++) {
		VkQueueFlags queueFlags = queueFamiliesProperties2[pdvIndex][i].queueFamilyProperties.queueFlags;

		bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
		bool isTransfer          = queueFlags & VK_QUEUE_TRANSFER_BIT;

		bool isDedicatedCompute = queueFlags == VK_QUEUE_COMPUTE_BIT;
		bool isCompute          = queueFlags & VK_QUEUE_COMPUTE_BIT;

		if (isTransfer) {
			if (isDedicatedTransfer && !hasDedicatedTransfer) {
				hasDedicatedTransfer     = true;
				hasTransfer              = true;
				transferQueueFamilyIndex = i;
			}

			else if (!hasTransfer) {
				hasTransfer              = true;
				transferQueueFamilyIndex = i;
			}
		}

		if (isCompute) {
			if (isDedicatedCompute && !hasDedicatedCompute) {
				hasDedicatedCompute     = true;
				hasCompute              = true;
				computeQueueFamilyIndex = i;
			}

			else if (!hasCompute) {
				hasCompute              = true;
				computeQueueFamilyIndex = i;
			}
		}
	}

	if (!hasTransfer)
		transferQueueFamilyIndex = computeQueueFamilyIndex;

	gpu->physicalDevice = physicalDevices[pdvIndex];

	gpu->transferQueueFamilyIndex = transferQueueFamilyIndex;
	gpu->computeQueueFamilyIndex  = computeQueueFamilyIndex;

	gpu->using8BitStorage                  = using8BitStorage;
	gpu->usingBufferDeviceAddress          = usingBufferDeviceAddress;
	gpu->usingMaintenance4                 = usingMaintenance4;
	gpu->usingMemoryBudget                 = usingMemoryBudget;
	gpu->usingMemoryPriority               = usingMemoryPriority;
	gpu->usingPipelineCreationCacheControl = usingPipelineCreationCacheControl;
	gpu->usingPortabilitySubset            = usingPortabilitySubset;
	gpu->usingShaderInt16                  = usingShaderInt16;
	gpu->usingShaderInt64                  = usingShaderInt64;
	gpu->usingSpirv14                      = usingSpirv14;
	gpu->usingSubgroupSizeControl          = usingSubgroupSizeControl;
	gpu->usingVulkan12                     = usingVulkan12;
	gpu->usingVulkan13                     = usingVulkan13;

	if (QUERY_BENCHMARKING) {
		gpu->transferQueueTimestampValidBits = queueFamiliesProperties2[pdvIndex][transferQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
		gpu->computeQueueTimestampValidBits  = queueFamiliesProperties2[pdvIndex][computeQueueFamilyIndex ].queueFamilyProperties.timestampValidBits;
		gpu->timestampPeriod                 = physicalDevicesProperties2[pdvIndex].properties.limits.timestampPeriod;
	}

	printf(
		"Device: %s\n"
		"\tScore:                             %u\n"
		"\tTransfer QF index:                 %u\n"
		"\tCompute QF index:                  %u\n"
		"\tSPIR-V 1.4:                        %d\n"
		"\tVulkan 1.2:                        %d\n"
		"\tVulkan 1.3:                        %d\n"
		"\tbufferDeviceAddress                %d\n"
		"\tmaintenance4                       %d\n"
		"\tmemoryPriority:                    %d\n"
		"\tpipelineCreationCacheControl:      %d\n"
		"\tshaderInt16:                       %d\n"
		"\tshaderInt64:                       %d\n"
		"\tsubgroupSizeControl:               %d\n"
		"\tuniformAndStorageBuffer8BitAccess: %d\n\n",
		physicalDevicesProperties2[pdvIndex].properties.deviceName,
		highestScore,
		transferQueueFamilyIndex,
		computeQueueFamilyIndex,
		usingSpirv14,
		usingVulkan12,
		usingVulkan13,
		usingBufferDeviceAddress,
		usingMaintenance4,
		usingMemoryPriority,
		usingPipelineCreationCacheControl,
		usingShaderInt16,
		usingShaderInt64,
		usingSubgroupSizeControl,
		using8BitStorage
	);

	free_recursive(dyMem);

	END_FUNC
	return true;
}

bool create_device(Gpu* gpu)
{
	BEGIN_FUNC

	VkPhysicalDevice physicalDevice = gpu->physicalDevice;

	uint32_t computeQueueFamilyIndex  = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 1);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	DyArray* enabledExtensions = DyArray_create(sizeof(const char*), 12);
#ifndef NDEBUG
	if (!enabledExtensions) {
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {enabledExtensions, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	const char* extensionName = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
	DyArray_append(enabledExtensions, &extensionName);

	extensionName = VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME;
	DyArray_append(enabledExtensions, &extensionName);

	if (VALIDATION_LAYERS && gpu->using8BitStorage) {
		extensionName = VK_KHR_8BIT_STORAGE_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (VALIDATION_LAYERS && gpu->usingBufferDeviceAddress) {
		extensionName = VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingMaintenance4) {
		extensionName = VK_KHR_MAINTENANCE_4_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingPortabilitySubset) {
		extensionName = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingSpirv14) {
		extensionName = VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);

		extensionName = VK_KHR_SPIRV_1_4_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingMemoryBudget) {
		extensionName = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingMemoryPriority) {
		extensionName = VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingPipelineCreationCacheControl) {
		extensionName = VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	if (gpu->usingSubgroupSizeControl) {
		extensionName = VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME;
		DyArray_append(enabledExtensions, &extensionName);
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {0};
	physicalDeviceFeatures.shaderInt64 = gpu->usingShaderInt64;
	physicalDeviceFeatures.shaderInt16 = gpu->usingShaderInt16;

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	physicalDeviceFeatures2.features = physicalDeviceFeatures;

	VkPhysicalDevice8BitStorageFeaturesKHR physicalDevice8BitStorageFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR};
	physicalDevice8BitStorageFeatures.storageBuffer8BitAccess           = VK_TRUE;
	physicalDevice8BitStorageFeatures.uniformAndStorageBuffer8BitAccess = VK_TRUE;

	VkPhysicalDevice16BitStorageFeatures physicalDevice16BitStorageFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
	physicalDevice16BitStorageFeatures.storageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeaturesKHR physicalDeviceBufferDeviceAddressFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};
	physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceMaintenance4FeaturesKHR physicalDeviceMaintenance4Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR};
	physicalDeviceMaintenance4Features.maintenance4 = VK_TRUE;

	VkPhysicalDeviceSynchronization2FeaturesKHR physicalDeviceSynchronization2Features = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR};
	physicalDeviceSynchronization2Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR physicalDeviceTimelineSemaphoreFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR};
	physicalDeviceTimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT physicalDeviceMemoryPriorityFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT};
	physicalDeviceMemoryPriorityFeatures.memoryPriority = VK_TRUE;

	VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT physicalDevicePipelineCreationCacheControlFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT};
	physicalDevicePipelineCreationCacheControlFeatures.pipelineCreationCacheControl = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeaturesEXT physicalDeviceSubgroupSizeControlFeatures = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT};
	physicalDeviceSubgroupSizeControlFeatures.subgroupSizeControl = VK_TRUE;

	void** next = &physicalDeviceFeatures2.pNext;

	*next = &physicalDevice16BitStorageFeatures;
	next  = &physicalDevice16BitStorageFeatures.pNext;

	*next = &physicalDeviceSynchronization2Features;
	next  = &physicalDeviceSynchronization2Features.pNext;

	*next = &physicalDeviceTimelineSemaphoreFeatures;
	next  = &physicalDeviceTimelineSemaphoreFeatures.pNext;

	if (VALIDATION_LAYERS && gpu->using8BitStorage) {
		*next = &physicalDevice8BitStorageFeatures;
		next  = &physicalDevice8BitStorageFeatures.pNext;
	}

	if (VALIDATION_LAYERS && gpu->usingBufferDeviceAddress) {
		*next = &physicalDeviceBufferDeviceAddressFeatures;
		next  = &physicalDeviceBufferDeviceAddressFeatures.pNext;
	}

	if (gpu->usingMaintenance4) {
		*next = &physicalDeviceMaintenance4Features;
		next  = &physicalDeviceMaintenance4Features.pNext;
	}

	if (gpu->usingMemoryPriority) {
		*next = &physicalDeviceMemoryPriorityFeatures;
		next  = &physicalDeviceMemoryPriorityFeatures.pNext;
	}

	if (gpu->usingPipelineCreationCacheControl) {
		*next = &physicalDevicePipelineCreationCacheControlFeatures;
		next  = &physicalDevicePipelineCreationCacheControlFeatures.pNext;
	}

	if (gpu->usingSubgroupSizeControl) {
		*next = &physicalDeviceSubgroupSizeControlFeatures;
		next  = &physicalDeviceSubgroupSizeControlFeatures.pNext;
	}

	// CPU spends more time waiting for compute operations than transfer operations
	// So compute queue has higher priority to potentially reduce this wait time
	float computeQueuePriority  = 1.f;
	float transferQueuePriority = 0.f;

	VkDeviceQueueCreateInfo queueCreateInfos[2];
	queueCreateInfos[0] = (VkDeviceQueueCreateInfo) {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfos[0].queueFamilyIndex = computeQueueFamilyIndex;
	queueCreateInfos[0].queueCount       = 1;
	queueCreateInfos[0].pQueuePriorities = &computeQueuePriority;

	queueCreateInfos[1] = (VkDeviceQueueCreateInfo) {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfos[1].queueFamilyIndex = transferQueueFamilyIndex;
	queueCreateInfos[1].queueCount       = 1;
	queueCreateInfos[1].pQueuePriorities = &transferQueuePriority;

	uint32_t     enabledExtensionCount = (uint32_t) DyArray_size(enabledExtensions);
	const char** enabledExtensionNames = (const char**) DyArray_raw(enabledExtensions);

	VkDeviceCreateInfo deviceCreateInfo = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	deviceCreateInfo.pNext                   = &physicalDeviceFeatures2;
	deviceCreateInfo.queueCreateInfoCount    = computeQueueFamilyIndex == transferQueueFamilyIndex ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount   = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;

	printf("Enabled device extensions (%u):\n", enabledExtensionCount);
	for (uint32_t i = 0; i < enabledExtensionCount; i++)
		printf("\t%u) %s\n", i + 1, enabledExtensionNames[i]);
	NEWLINE()

	VK_CALL_RES(vkCreateDevice, physicalDevice, &deviceCreateInfo, g_allocator, &gpu->device)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateDevice)
		free_recursive(dyMem);
		return false;
	}
#endif

	VkDevice device = gpu->device;

	free_recursive(dyMem);

	volkLoadDevice(device);

	VkDeviceQueueInfo2 transferDeviceQueueInfo2 = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
	transferDeviceQueueInfo2.queueFamilyIndex = transferQueueFamilyIndex;
	transferDeviceQueueInfo2.queueIndex       = 0;

	VkDeviceQueueInfo2 computeDeviceQueueInfo2 = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
	computeDeviceQueueInfo2.queueFamilyIndex = computeQueueFamilyIndex;
	computeDeviceQueueInfo2.queueIndex       = 0;

	VK_CALL(vkGetDeviceQueue2, device, &transferDeviceQueueInfo2, &gpu->transferQueue)
	VK_CALL(vkGetDeviceQueue2, device, &computeDeviceQueueInfo2,  &gpu->computeQueue)

#ifndef NDEBUG
	if(gpu->debugUtilsMessenger) {
		if (gpu->transferQueue != gpu->computeQueue) {
			set_debug_name(device, VK_OBJECT_TYPE_QUEUE, (uint64_t) gpu->transferQueue, "Transfer");
			set_debug_name(device, VK_OBJECT_TYPE_QUEUE, (uint64_t) gpu->computeQueue,  "Compute");
		}
		else
			set_debug_name(device, VK_OBJECT_TYPE_QUEUE, (uint64_t) gpu->transferQueue, "Transfer & Compute");
	}
#endif

	END_FUNC
	return true;
}

bool manage_memory(Gpu* gpu)
{
	BEGIN_FUNC

	VkPhysicalDevice physicalDevice = gpu->physicalDevice;
	VkDevice         device         = gpu->device;

	bool (*get_buffer_requirements)(VkDevice, VkDeviceSize, VkBufferUsageFlags, VkMemoryRequirements*) = gpu->usingMaintenance4 ? get_buffer_requirements_main4 : get_buffer_requirements_noext;

	VkPhysicalDeviceMaintenance4PropertiesKHR physicalDeviceMaintenance4Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR};

	VkPhysicalDeviceMaintenance3Properties physicalDeviceMaintenance3Properties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES};
	physicalDeviceMaintenance3Properties.pNext = gpu->usingMaintenance4 ? &physicalDeviceMaintenance4Properties : NULL;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	physicalDeviceProperties2.pNext = &physicalDeviceMaintenance3Properties;

	VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevice, &physicalDeviceProperties2)

	VkPhysicalDeviceMemoryBudgetPropertiesEXT physicalDeviceMemoryBudgetProperties = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};

	VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
	physicalDeviceMemoryProperties2.pNext = gpu->usingMemoryBudget ? &physicalDeviceMemoryBudgetProperties : NULL;

	VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevice, &physicalDeviceMemoryProperties2)

	VkDeviceSize maxMemoryAllocationSize = physicalDeviceMaintenance3Properties.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize           = gpu->usingMaintenance4 ? physicalDeviceMaintenance4Properties.maxBufferSize : maxMemoryAllocationSize;

	uint32_t maxStorageBufferRange    = physicalDeviceProperties2.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryAllocationCount = physicalDeviceProperties2.properties.limits.maxMemoryAllocationCount;
	uint32_t maxComputeWorkGroupCount = physicalDeviceProperties2.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxComputeWorkGroupSize  = physicalDeviceProperties2.properties.limits.maxComputeWorkGroupSize[0];
	uint32_t memoryTypeCount          = physicalDeviceMemoryProperties2.memoryProperties.memoryTypeCount;

	VkMemoryRequirements deviceLocalMemoryRequirements;
	VkMemoryRequirements hostVisibleMemoryRequirements;

	bool bres = get_buffer_requirements(device, sizeof(char), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &hostVisibleMemoryRequirements);
#ifndef NDEBUG
	if (!bres)
		return false;
#endif

	bres = get_buffer_requirements(device, sizeof(char), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &deviceLocalMemoryRequirements);
#ifndef NDEBUG
	if (!bres)
		return false;
#endif

	uint32_t deviceLocalMemoryTypeBits = deviceLocalMemoryRequirements.memoryTypeBits;
	uint32_t hostVisibleMemoryTypeBits = hostVisibleMemoryRequirements.memoryTypeBits;

	uint32_t deviceLocalHeapIndex = ~0U;
	uint32_t hostVisibleHeapIndex = ~0U;
	uint32_t deviceLocalTypeIndex = ~0U;
	uint32_t hostVisibleTypeIndex = ~0U;

	bool hasDeviceNonHost = false;
	bool hasDeviceLocal   = false;

	bool hasHostCachedNonCoherent = false;
	bool hasHostCached            = false;
	bool hasHostNonCoherent       = false;
	bool hasHostVisible           = false;

	for (uint32_t i = 0; i < memoryTypeCount; i++) {
		uint32_t memoryTypeBit = 1U << i;

		VkMemoryPropertyFlags propertyFlags = physicalDeviceMemoryProperties2.memoryProperties.memoryTypes[i].propertyFlags;
		uint32_t              heapIndex     = physicalDeviceMemoryProperties2.memoryProperties.memoryTypes[i].heapIndex;

		bool isDeviceLocal    = propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bool isHostVisible    = propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		bool isHostCoherent   = propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bool isHostCached     = propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		bool isProtected      = propertyFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT;
		bool isDeviceCoherent = propertyFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD;

		if (isProtected)      continue;
		if (isDeviceCoherent) continue;

		if (isDeviceLocal && (deviceLocalMemoryTypeBits & memoryTypeBit)) {
			if (!isHostVisible && !hasDeviceNonHost) {
				hasDeviceNonHost     = true;
				hasDeviceLocal       = true;
				deviceLocalHeapIndex = heapIndex;
				deviceLocalTypeIndex = i;
			}

			else if (!hasDeviceLocal) {
				hasDeviceLocal       = true;
				deviceLocalHeapIndex = heapIndex;
				deviceLocalTypeIndex = i;
			}
		}

		if (isHostVisible && (hostVisibleMemoryTypeBits & memoryTypeBit)) {
			if (isHostCached && !isHostCoherent && !hasHostCachedNonCoherent) {
				hasHostCachedNonCoherent = true;
				hasHostCached            = true;
				hasHostNonCoherent       = true;
				hasHostVisible           = true;
				hostVisibleHeapIndex     = heapIndex;
				hostVisibleTypeIndex     = i;
			}

			else if (isHostCached && !hasHostCached) {
				hasHostCached        = true;
				hasHostNonCoherent   = false;
				hasHostVisible       = true;
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
			}

			else if (!isHostCoherent && !hasHostCached && !hasHostNonCoherent) {
				hasHostNonCoherent   = true;
				hasHostVisible       = true;
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
			}

			else if (!hasHostVisible) {
				hasHostVisible       = true;
				hostVisibleHeapIndex = heapIndex;
				hostVisibleTypeIndex = i;
			}
		}
	}

	VkDeviceSize bytesPerHostVisibleHeap;
	VkDeviceSize bytesPerDeviceLocalHeap;

	if (gpu->usingMemoryBudget) {
		bytesPerHostVisibleHeap = physicalDeviceMemoryBudgetProperties.heapBudget[hostVisibleHeapIndex];
		bytesPerDeviceLocalHeap = physicalDeviceMemoryBudgetProperties.heapBudget[deviceLocalHeapIndex];
	}
	else {
		bytesPerHostVisibleHeap = physicalDeviceMemoryProperties2.memoryProperties.memoryHeaps[hostVisibleHeapIndex].size;
		bytesPerDeviceLocalHeap = physicalDeviceMemoryProperties2.memoryProperties.memoryHeaps[deviceLocalHeapIndex].size;
	}

	VkDeviceSize bytesPerHeap = bytesPerHostVisibleHeap < bytesPerDeviceLocalHeap ? bytesPerHostVisibleHeap : bytesPerDeviceLocalHeap;
	bytesPerHeap = (VkDeviceSize) (bytesPerHeap * MAX_HEAP_MEMORY);

	if (deviceLocalHeapIndex == hostVisibleHeapIndex)
		bytesPerHeap /= 2;

	VkDeviceSize bytesPerBuffer = maxMemoryAllocationSize < maxBufferSize ? maxMemoryAllocationSize : maxBufferSize;
	bytesPerBuffer = bytesPerHeap < bytesPerBuffer ? bytesPerHeap : bytesPerBuffer;

	uint32_t buffersPerHeap = (uint32_t) (bytesPerHeap / bytesPerBuffer);

	if (buffersPerHeap < maxMemoryAllocationCount && bytesPerHeap % bytesPerBuffer) {
		VkDeviceSize excessBytes = bytesPerBuffer - bytesPerHeap % bytesPerBuffer;

		buffersPerHeap++;
		bytesPerBuffer -= excessBytes / buffersPerHeap;

		if (excessBytes % buffersPerHeap)
			bytesPerBuffer--;
	}
	else if (buffersPerHeap > maxMemoryAllocationCount)
		buffersPerHeap = maxMemoryAllocationCount;

	uint32_t workgroupSize = floor_pow2(maxComputeWorkGroupSize);
	uint32_t workgroupCount = (uint32_t) (maxStorageBufferRange / (workgroupSize * sizeof(Value)));
	workgroupCount = maxComputeWorkGroupCount < workgroupCount ? maxComputeWorkGroupCount : workgroupCount;

	uint32_t     valuesPerInout  = workgroupSize * workgroupCount;
	VkDeviceSize bytesPerInout   = valuesPerInout * (sizeof(Value) + sizeof(Steps));
	uint32_t     inoutsPerBuffer = (uint32_t) (bytesPerBuffer / bytesPerInout);

	if (bytesPerBuffer % bytesPerInout > inoutsPerBuffer * workgroupSize * (sizeof(Value) + sizeof(Steps))) {
		uint32_t excessValues = valuesPerInout - (uint32_t) (bytesPerBuffer % bytesPerInout / (sizeof(Value) + sizeof(Steps)));

		inoutsPerBuffer++;
		valuesPerInout -= excessValues / inoutsPerBuffer;

		if (excessValues % inoutsPerBuffer)
			valuesPerInout--;

		valuesPerInout &= ~(workgroupSize - 1U);

		if (!valuesPerInout) {
			inoutsPerBuffer--;
			valuesPerInout = workgroupSize * workgroupCount;
		}

		workgroupCount = valuesPerInout / workgroupSize;
	}

	VkDeviceSize bytesPerIn  = valuesPerInout * sizeof(Value);
	VkDeviceSize bytesPerOut = valuesPerInout * sizeof(Steps);

	bytesPerInout  = bytesPerIn + bytesPerOut;
	bytesPerBuffer = bytesPerInout * inoutsPerBuffer;

	uint32_t valuesPerBuffer = valuesPerInout * inoutsPerBuffer;
	uint32_t valuesPerHeap   = valuesPerBuffer * buffersPerHeap;
	uint32_t inoutsPerHeap   = inoutsPerBuffer * buffersPerHeap;

	bres = get_buffer_requirements(device, bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &hostVisibleMemoryRequirements);
#ifndef NDEBUG
	if (!bres)
		return false;
#endif

	bres = get_buffer_requirements(device, bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, &deviceLocalMemoryRequirements);
#ifndef NDEBUG
	if (!bres)
		return false;
#endif

	VkDeviceSize bytesPerHostVisibleMemory = hostVisibleMemoryRequirements.size;
	VkDeviceSize bytesPerDeviceLocalMemory = deviceLocalMemoryRequirements.size;

	/*
		maxComputeWorkGroupSize is guaranteed to be at least 128
		workgroupSize is maxComputeWorkGroupSize rounded down to a power of 2
		=> workgroupSize is a multiple of 128

		valuesPerInout is a multiple of workgroupSize
		=> valuesPerInout is a multiple of 128

		bytesPerIn and bytesPerOut are multiples of valuesPerInout * 2
		=> bytesPerIn and bytesPerOut are multiples of 256

		nonCoherentAtomSize and minStorageBufferOffsetAlignment are guaranteed to be at most 256
		=> bytesPerIn and bytesPerOut are multiples of nonCoherentAtomSize and minStorageBufferOffsetAlignment
	*/

	gpu->bytesPerIn                = bytesPerIn;
	gpu->bytesPerOut               = bytesPerOut;
	gpu->bytesPerInout             = bytesPerInout;
	gpu->bytesPerBuffer            = bytesPerBuffer;
	gpu->bytesPerHostVisibleMemory = bytesPerHostVisibleMemory;
	gpu->bytesPerDeviceLocalMemory = bytesPerDeviceLocalMemory;

	gpu->valuesPerInout  = valuesPerInout;
	gpu->valuesPerBuffer = valuesPerBuffer;
	gpu->valuesPerHeap   = valuesPerHeap;
	gpu->inoutsPerBuffer = inoutsPerBuffer;
	gpu->inoutsPerHeap   = inoutsPerHeap;
	gpu->buffersPerHeap  = buffersPerHeap;

	gpu->workgroupCount = workgroupCount;
	gpu->workgroupSize  = workgroupSize;

	gpu->hostVisibleHeapIndex = hostVisibleHeapIndex;
	gpu->deviceLocalHeapIndex = deviceLocalHeapIndex;

	gpu->hostVisibleTypeIndex = hostVisibleTypeIndex;
	gpu->deviceLocalTypeIndex = deviceLocalTypeIndex;

	gpu->hostNonCoherent = hasHostNonCoherent;

	printf(
		"Memory information:\n"
		"\tHV memory heap index:     %u\n"
		"\tDL memory heap index:     %u\n"
		"\tHV memory type index:     %u\n"
		"\tDL memory type index:     %u\n"
		"\tHV non-coherent memory:   %d\n"
		"\tWorkgroup size:           %u\n"
		"\tWorkgroup count:          %u\n"
		"\tValues per inout-buffer:  %u\n"
		"\tInout-buffers per buffer: %u\n"
		"\tBuffers per heap:         %u\n"
		"\tValues per heap:          %u\n\n",
		hostVisibleHeapIndex,
		deviceLocalHeapIndex,
		hostVisibleTypeIndex,
		deviceLocalTypeIndex,
		hasHostNonCoherent,
		workgroupSize,
		workgroupCount,
		valuesPerInout,
		inoutsPerBuffer,
		buffersPerHeap,
		valuesPerHeap
	);

	size_t size =
		inoutsPerHeap      * sizeof(Value*) +
		inoutsPerHeap      * sizeof(Steps*) +
		buffersPerHeap * 2 * sizeof(VkBuffer) +
		buffersPerHeap * 2 * sizeof(VkDeviceMemory) +
		inoutsPerHeap      * sizeof(VkDescriptorSet) +
		inoutsPerHeap  * 2 * sizeof(VkCommandBuffer) +
		inoutsPerHeap      * sizeof(VkSemaphore);

	gpu->dynamicMemory = calloc(size, sizeof(char));
#ifndef NDEBUG
	if (!gpu->dynamicMemory) {
		CALLOC_FAILURE(gpu->dynamicMemory, size, sizeof(char))
		return false;
	}
#endif

	gpu->mappedInBuffers  = (Value**) gpu->dynamicMemory;
	gpu->mappedOutBuffers = (Steps**) (gpu->mappedInBuffers + inoutsPerHeap);

	gpu->hostVisibleBuffers = (VkBuffer*) (gpu->mappedOutBuffers   + inoutsPerHeap);
	gpu->deviceLocalBuffers = (VkBuffer*) (gpu->hostVisibleBuffers + buffersPerHeap);

	gpu->hostVisibleDeviceMemories = (VkDeviceMemory*) (gpu->deviceLocalBuffers        + buffersPerHeap);
	gpu->deviceLocalDeviceMemories = (VkDeviceMemory*) (gpu->hostVisibleDeviceMemories + buffersPerHeap);

	gpu->descriptorSets = (VkDescriptorSet*) (gpu->deviceLocalDeviceMemories + buffersPerHeap);

	gpu->transferCommandBuffers = (VkCommandBuffer*) (gpu->descriptorSets         + inoutsPerHeap);
	gpu->computeCommandBuffers  = (VkCommandBuffer*) (gpu->transferCommandBuffers + inoutsPerHeap);

	gpu->semaphores = (VkSemaphore*) (gpu->computeCommandBuffers + inoutsPerHeap);

	END_FUNC
	return true;
}

bool create_buffers(Gpu* gpu)
{
	BEGIN_FUNC

	VkDeviceMemory* restrict hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* restrict deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	VkBuffer*       restrict hostVisibleBuffers        = gpu->hostVisibleBuffers;
	VkBuffer*       restrict deviceLocalBuffers        = gpu->deviceLocalBuffers;
	Value**         restrict mappedInBuffers           = gpu->mappedInBuffers;
	Steps**         restrict mappedOutBuffers          = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerBuffer            = gpu->bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleMemory = gpu->bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDeviceLocalMemory = gpu->bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout       = gpu->valuesPerInout;
	uint32_t inoutsPerBuffer      = gpu->inoutsPerBuffer;
	uint32_t buffersPerHeap       = gpu->buffersPerHeap;
	uint32_t hostVisibleTypeIndex = gpu->hostVisibleTypeIndex;
	uint32_t deviceLocalTypeIndex = gpu->deviceLocalTypeIndex;

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 1);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	size_t size =
		buffersPerHeap * 2 * sizeof(VkMemoryAllocateInfo) +
		buffersPerHeap * 2 * sizeof(VkMemoryDedicatedAllocateInfo) +
		buffersPerHeap * 2 * sizeof(VkBindBufferMemoryInfo);

	VkMemoryAllocateInfo* hostVisibleMemoryAllocateInfos = (VkMemoryAllocateInfo*) malloc(size);
#ifndef NDEBUG
	if (!hostVisibleMemoryAllocateInfos) {
		MALLOC_FAILURE(hostVisibleMemoryAllocateInfos, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {hostVisibleMemoryAllocateInfos, free};
	DyArray_append(dyMem, &dyData);

	VkMemoryAllocateInfo* deviceLocalMemoryAllocateInfos = (VkMemoryAllocateInfo*) (hostVisibleMemoryAllocateInfos + buffersPerHeap);

	VkMemoryDedicatedAllocateInfo* hostVisibleMemoryDedicatedAllocateInfos = (VkMemoryDedicatedAllocateInfo*) (deviceLocalMemoryAllocateInfos          + buffersPerHeap);
	VkMemoryDedicatedAllocateInfo* deviceLocalMemoryDedicatedAllocateInfos = (VkMemoryDedicatedAllocateInfo*) (hostVisibleMemoryDedicatedAllocateInfos + buffersPerHeap);

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) (deviceLocalMemoryDedicatedAllocateInfos + buffersPerHeap);

	VkBufferCreateInfo hostVisibleBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	hostVisibleBufferCreateInfo.size        = bytesPerBuffer;
	hostVisibleBufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hostVisibleBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo deviceLocalBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	deviceLocalBufferCreateInfo.size        = bytesPerBuffer;
	deviceLocalBufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	deviceLocalBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkCreateBuffer, device, &hostVisibleBufferCreateInfo, g_allocator, &hostVisibleBuffers[i])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkCreateBuffer)
			free_recursive(dyMem);
			return false;
		}
#endif

		VK_CALL_RES(vkCreateBuffer, device, &deviceLocalBufferCreateInfo, g_allocator, &deviceLocalBuffers[i])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkCreateBuffer)
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	VkMemoryPriorityAllocateInfoEXT hostVisibleMemoryPriorityAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
	hostVisibleMemoryPriorityAllocateInfo.priority = 0.f;

	VkMemoryPriorityAllocateInfoEXT deviceLocalMemoryPriorityAllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
	deviceLocalMemoryPriorityAllocateInfo.priority = 1.f;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		hostVisibleMemoryDedicatedAllocateInfos[i] = (VkMemoryDedicatedAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		hostVisibleMemoryDedicatedAllocateInfos[i].pNext  = gpu->usingMemoryPriority ? &hostVisibleMemoryPriorityAllocateInfo : NULL;
		hostVisibleMemoryDedicatedAllocateInfos[i].buffer = hostVisibleBuffers[i];

		deviceLocalMemoryDedicatedAllocateInfos[i] = (VkMemoryDedicatedAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		deviceLocalMemoryDedicatedAllocateInfos[i].pNext  = gpu->usingMemoryPriority ? &deviceLocalMemoryPriorityAllocateInfo : NULL;
		deviceLocalMemoryDedicatedAllocateInfos[i].buffer = deviceLocalBuffers[i];

		hostVisibleMemoryAllocateInfos[i] = (VkMemoryAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		hostVisibleMemoryAllocateInfos[i].pNext           = &hostVisibleMemoryDedicatedAllocateInfos[i];
		hostVisibleMemoryAllocateInfos[i].allocationSize  = bytesPerHostVisibleMemory;
		hostVisibleMemoryAllocateInfos[i].memoryTypeIndex = hostVisibleTypeIndex;

		deviceLocalMemoryAllocateInfos[i] = (VkMemoryAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		deviceLocalMemoryAllocateInfos[i].pNext           = &deviceLocalMemoryDedicatedAllocateInfos[i];
		deviceLocalMemoryAllocateInfos[i].allocationSize  = bytesPerDeviceLocalMemory;
		deviceLocalMemoryAllocateInfos[i].memoryTypeIndex = deviceLocalTypeIndex;
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkAllocateMemory, device, &hostVisibleMemoryAllocateInfos[i], g_allocator, &hostVisibleDeviceMemories[i])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkAllocateMemory)
			free_recursive(dyMem);
			return false;
		}
#endif

		VK_CALL_RES(vkAllocateMemory, device, &deviceLocalMemoryAllocateInfos[i], g_allocator, &deviceLocalDeviceMemories[i])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkAllocateMemory)
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		bindBufferMemoryInfos[i][0] = (VkBindBufferMemoryInfo) {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
		bindBufferMemoryInfos[i][0].buffer = hostVisibleBuffers[i];
		bindBufferMemoryInfos[i][0].memory = hostVisibleDeviceMemories[i];

		bindBufferMemoryInfos[i][1] = (VkBindBufferMemoryInfo) {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
		bindBufferMemoryInfos[i][1].buffer = deviceLocalBuffers[i];
		bindBufferMemoryInfos[i][1].memory = deviceLocalDeviceMemories[i];
	}

	VK_CALL_RES(vkBindBufferMemory2, device, buffersPerHeap * 2, (VkBindBufferMemoryInfo*) bindBufferMemoryInfos)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkBindBufferMemory2)
		free_recursive(dyMem);
		return false;
	}
#endif

	uint32_t inoIndex = 0; // Inout-buffer index

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkMapMemory, device, hostVisibleDeviceMemories[i], 0, bytesPerHostVisibleMemory, 0, (void**) &mappedInBuffers[inoIndex])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkMapMemory)
			free_recursive(dyMem);
			return false;
		}
#endif

		mappedOutBuffers[inoIndex] = (Steps*) (mappedInBuffers[inoIndex] + valuesPerInout);
		inoIndex++;

		for (uint32_t j = 1; j < inoutsPerBuffer; j++) {
			mappedInBuffers[inoIndex]  = mappedInBuffers[inoIndex - 1]  + valuesPerInout + valuesPerInout * sizeof(Steps) / sizeof(Value);
			mappedOutBuffers[inoIndex] = mappedOutBuffers[inoIndex - 1] + valuesPerInout + valuesPerInout * sizeof(Value) / sizeof(Steps);
			inoIndex++;
		}
	}

	free_recursive(dyMem);

#ifndef NDEBUG
	if(gpu->debugUtilsMessenger) {
		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			char objectName[37];

			sprintf(objectName, "Host visible (%u/%u)", i + 1, buffersPerHeap);

			set_debug_name(device, VK_OBJECT_TYPE_BUFFER,        (uint64_t) hostVisibleBuffers[i],        objectName);
			set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) hostVisibleDeviceMemories[i], objectName);

			strcpy(objectName, "Device local");
			objectName[12] = ' '; // Remove '\0' from strcpy

			set_debug_name(device, VK_OBJECT_TYPE_BUFFER,        (uint64_t) deviceLocalBuffers[i],        objectName);
			set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) deviceLocalDeviceMemories[i], objectName);
		}
	}
#endif

	END_FUNC
	return true;
}

bool create_descriptors(Gpu* gpu)
{
	BEGIN_FUNC

	const VkBuffer* restrict deviceLocalBuffers = gpu->deviceLocalBuffers;

	VkDescriptorSet* restrict descriptorSets = gpu->descriptorSets;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerIn    = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut   = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap   = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap  = gpu->buffersPerHeap;

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 1);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	size_t size =
		inoutsPerHeap     * sizeof(VkDescriptorSetLayout) +
		inoutsPerHeap     * sizeof(VkWriteDescriptorSet) +
		inoutsPerHeap * 2 * sizeof(VkDescriptorBufferInfo);

	VkDescriptorSetLayout* descriptorSetLayouts = (VkDescriptorSetLayout*) malloc(size);
#ifndef NDEBUG
	if (!descriptorSetLayouts) {
		MALLOC_FAILURE(descriptorSetLayouts, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {descriptorSetLayouts, free};
	DyArray_append(dyMem, &dyData);

	VkWriteDescriptorSet* writeDescriptorSets = (VkWriteDescriptorSet*) (descriptorSetLayouts + inoutsPerHeap);

	VkDescriptorBufferInfo (*descriptorBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (writeDescriptorSets + inoutsPerHeap);

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2];
	descriptorSetLayoutBindings[0].binding            = 0;
	descriptorSetLayoutBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[0].descriptorCount    = 1;
	descriptorSetLayoutBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	descriptorSetLayoutBindings[0].pImmutableSamplers = NULL;

	descriptorSetLayoutBindings[1].binding            = 1;
	descriptorSetLayoutBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorSetLayoutBindings[1].descriptorCount    = 1;
	descriptorSetLayoutBindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	descriptorSetLayoutBindings[1].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descriptorSetLayoutCreateInfo.bindingCount = ARRAY_SIZE(descriptorSetLayoutBindings);
	descriptorSetLayoutCreateInfo.pBindings    = descriptorSetLayoutBindings;

	VkDescriptorPoolSize descriptorPoolSizes[1];
	descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[0].descriptorCount = inoutsPerHeap * 2;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	descriptorPoolCreateInfo.maxSets       = inoutsPerHeap;
	descriptorPoolCreateInfo.poolSizeCount = ARRAY_SIZE(descriptorPoolSizes);
	descriptorPoolCreateInfo.pPoolSizes    = descriptorPoolSizes;

	VK_CALL_RES(vkCreateDescriptorSetLayout, device, &descriptorSetLayoutCreateInfo, g_allocator, &gpu->descriptorSetLayout)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateDescriptorSetLayout)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkCreateDescriptorPool, device, &descriptorPoolCreateInfo, g_allocator, &gpu->descriptorPool)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateDescriptorPool)
		free_recursive(dyMem);
		return false;
	}
#endif

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool      descriptorPool      = gpu->descriptorPool;

	for (uint32_t i = 0; i < inoutsPerHeap; i++)
		descriptorSetLayouts[i] = descriptorSetLayout;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = inoutsPerHeap;
	descriptorSetAllocateInfo.pSetLayouts        = descriptorSetLayouts;

	VK_CALL_RES(vkAllocateDescriptorSets, device, &descriptorSetAllocateInfo, descriptorSets)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkAllocateDescriptorSets)
		free_recursive(dyMem);
		return false;
	}
#endif

	uint32_t inoIndex = 0; // Inout-buffer index

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutsPerBuffer; j++, inoIndex++) {
			descriptorBufferInfos[inoIndex][0].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[inoIndex][0].offset = bytesPerInout * j;
			descriptorBufferInfos[inoIndex][0].range  = bytesPerIn;

			descriptorBufferInfos[inoIndex][1].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[inoIndex][1].offset = bytesPerInout * j + bytesPerIn;
			descriptorBufferInfos[inoIndex][1].range  = bytesPerOut;

			writeDescriptorSets[inoIndex] = (VkWriteDescriptorSet) {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			writeDescriptorSets[inoIndex].dstSet          = descriptorSets[inoIndex];
			writeDescriptorSets[inoIndex].dstBinding      = 0;
			writeDescriptorSets[inoIndex].dstArrayElement = 0;
			writeDescriptorSets[inoIndex].descriptorCount = ARRAY_SIZE(descriptorBufferInfos[inoIndex]);
			writeDescriptorSets[inoIndex].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[inoIndex].pBufferInfo     = descriptorBufferInfos[inoIndex];
		}
	}

	VK_CALL(vkUpdateDescriptorSets, device, inoutsPerHeap, writeDescriptorSets, 0, NULL)

	free_recursive(dyMem);

#ifndef NDEBUG
	if(gpu->debugUtilsMessenger) {
		inoIndex = 0;

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			for (uint32_t j = 0; j < inoutsPerBuffer; j++, inoIndex++) {
				char objectName[58];

				sprintf(objectName, "Inout %u/%u, Buffer %u/%u", j + 1, inoutsPerBuffer, i + 1, buffersPerHeap);

				set_debug_name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descriptorSets[inoIndex], objectName);
			}
		}
	}
#endif

	END_FUNC
	return true;
}

bool create_pipeline(Gpu* gpu)
{
	BEGIN_FUNC

	VkDevice device = gpu->device;

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;

	uint32_t inoutsPerHeap                   = gpu->inoutsPerHeap;
	uint32_t workgroupSize                   = gpu->workgroupSize;
	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 2);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	char shaderName[23] = "./";

	if      (gpu->usingVulkan13) strcpy(shaderName, "v16/shader");
	else if (gpu->usingVulkan12) strcpy(shaderName, "v15/shader");
	else if (gpu->usingSpirv14)  strcpy(shaderName, "v14/shader");
	else                         strcpy(shaderName, "v13/shader");

	if (PREFER_INT16 && gpu->usingShaderInt16) strcat(shaderName, "_16");
	if (PREFER_INT64 && gpu->usingShaderInt64) strcat(shaderName, "_64");

	strcat(shaderName, ".spv");

	char entryPointName[8] = "main";

	if      (ITER_SIZE == 128) strcat(entryPointName, "128");
	else if (ITER_SIZE == 256) strcat(entryPointName, "256");

	printf("Selected shader: %s\nSelected entry point: %s\n\n", shaderName, entryPointName);

	size_t shaderSize;
	bool bres = file_size(shaderName, &shaderSize);
#ifndef NDEBUG
	if (!bres) {
		free_recursive(dyMem);
		return false;
	}
#endif

	size_t cacheSize;
	bres = file_size(PIPELINE_CACHE_NAME, &cacheSize);
#ifndef NDEBUG
	if (!bres) {
		free_recursive(dyMem);
		return false;
	}
#endif

	size_t size = shaderSize + cacheSize;

	uint32_t* shaderCode = (uint32_t*) malloc(size);
#ifndef NDEBUG
	if (!shaderCode) {
		MALLOC_FAILURE(shaderCode, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {shaderCode, free};
	DyArray_append(dyMem, &dyData);

	void* cacheData = (char*) shaderCode + shaderSize;

	bres = read_file(shaderName, shaderCode, shaderSize);
#ifndef NDEBUG
	if (!bres) {
		free_recursive(dyMem);
		return false;
	}
#endif

	if (cacheSize) {
		bres = read_file(PIPELINE_CACHE_NAME, cacheData, cacheSize);
#ifndef NDEBUG
		if (!bres) {
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	shaderModuleCreateInfo.codeSize = shaderSize;
	shaderModuleCreateInfo.pCode    = shaderCode;

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	pipelineCacheCreateInfo.flags = gpu->usingPipelineCreationCacheControl ? VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT : 0;
	pipelineCacheCreateInfo.initialDataSize = cacheSize;
	pipelineCacheCreateInfo.pInitialData    = cacheData;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts    = &descriptorSetLayout;

	VK_CALL_RES(vkCreateShaderModule, device, &shaderModuleCreateInfo, g_allocator, &gpu->shaderModule)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateShaderModule)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkCreatePipelineCache, device, &pipelineCacheCreateInfo, g_allocator, &gpu->pipelineCache)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreatePipelineCache)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkCreatePipelineLayout, device, &pipelineLayoutCreateInfo, g_allocator, &gpu->pipelineLayout)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreatePipelineLayout)
		free_recursive(dyMem);
		return false;
	}
#endif

	VkShaderModule   shaderModule   = gpu->shaderModule;
	VkPipelineCache  pipelineCache  = gpu->pipelineCache;
	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;

	VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, g_allocator)
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	uint32_t specialisationData[1];
	specialisationData[0] = workgroupSize;

	VkSpecializationMapEntry specialisationMapEntries[1];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset     = 0;
	specialisationMapEntries[0].size       = sizeof(specialisationData[0]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = ARRAY_SIZE(specialisationMapEntries);
	specialisationInfo.pMapEntries   = specialisationMapEntries;
	specialisationInfo.dataSize      = sizeof(specialisationData);
	specialisationInfo.pData         = specialisationData;

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	pipelineShaderStageCreateInfo.flags               = gpu->usingSubgroupSizeControl ? VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT : 0;
	pipelineShaderStageCreateInfo.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineShaderStageCreateInfo.module              = shaderModule;
	pipelineShaderStageCreateInfo.pName               = entryPointName;
	pipelineShaderStageCreateInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo computePipelineCreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	computePipelineCreateInfo.flags  = gpu->usingPipelineCreationCacheControl ? VK_PIPELINE_CREATE_EARLY_RETURN_ON_FAILURE_BIT_EXT : 0;
	computePipelineCreateInfo.stage  = pipelineShaderStageCreateInfo;
	computePipelineCreateInfo.layout = pipelineLayout;

	VK_CALL_RES(vkCreateComputePipelines, device, pipelineCache, 1, &computePipelineCreateInfo, g_allocator, &gpu->pipeline)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateComputePipelines)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL(vkDestroyShaderModule, device, shaderModule, g_allocator)
	gpu->shaderModule = VK_NULL_HANDLE;

	VK_CALL_RES(vkGetPipelineCacheData, device, pipelineCache, &cacheSize, NULL)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkGetPipelineCacheData)
		free_recursive(dyMem);
		return false;
	}
#endif

	cacheData = malloc(cacheSize);
#ifndef NDEBUG
	if (!cacheData) {
		MALLOC_FAILURE(cacheData, cacheSize)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {cacheData, free};
	DyArray_append(dyMem, &dyData);

	VK_CALL_RES(vkGetPipelineCacheData, device, pipelineCache, &cacheSize, cacheData)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkGetPipelineCacheData)
		free_recursive(dyMem);
		return false;
	}
#endif

	bres = write_file(PIPELINE_CACHE_NAME, cacheData, cacheSize);
#ifndef NDEBUG
	if (!bres) {
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL(vkDestroyPipelineCache, device, pipelineCache, g_allocator)
	gpu->pipelineCache = VK_NULL_HANDLE;

	if (transferQueueTimestampValidBits || computeQueueTimestampValidBits) {
		VkQueryPoolCreateInfo queryPoolCreateInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		queryPoolCreateInfo.queryType  = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount = inoutsPerHeap * 4;

		VK_CALL_RES(vkCreateQueryPool, device, &queryPoolCreateInfo, g_allocator, &gpu->queryPool)
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkCreateQueryPool)
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	free_recursive(dyMem);

	END_FUNC
	return true;
}

bool create_commands(Gpu* gpu)
{
	BEGIN_FUNC

	const VkBuffer*        restrict hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer*        restrict deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* restrict descriptorSets     = gpu->descriptorSets;

	VkCommandBuffer* restrict transferCommandBuffers = gpu->transferCommandBuffers;
	VkCommandBuffer* restrict computeCommandBuffers  = gpu->computeCommandBuffers;
	VkSemaphore*     restrict semaphores             = gpu->semaphores;

	VkDevice device = gpu->device;

	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;
	VkPipeline       pipeline       = gpu->pipeline;
	VkQueryPool      queryPool      = gpu->queryPool;

	VkDeviceSize bytesPerIn    = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut   = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap   = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap  = gpu->buffersPerHeap;
	uint32_t workgroupCount  = gpu->workgroupCount;

	uint32_t transferQueueFamilyIndex        = gpu->transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex         = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 1);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	size_t size =
		inoutsPerBuffer * 2 * sizeof(VkBufferCopy) +
		inoutsPerHeap   * 6 * sizeof(VkBufferMemoryBarrier2KHR) +
		inoutsPerHeap   * 4 * sizeof(VkDependencyInfoKHR);

	VkBufferCopy* inBufferCopies = (VkBufferCopy*) malloc(size);
#ifndef NDEBUG
	if (!inBufferCopies) {
		MALLOC_FAILURE(inBufferCopies, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {inBufferCopies, free};
	DyArray_append(dyMem, &dyData);

	VkBufferCopy* outBufferCopies = (VkBufferCopy*) (inBufferCopies + inoutsPerBuffer);

	VkBufferMemoryBarrier2KHR*  onetimeBufferMemoryBarriers2      = (VkBufferMemoryBarrier2KHR*)     (outBufferCopies               + inoutsPerBuffer);
	VkBufferMemoryBarrier2KHR (*transferBufferMemoryBarriers2)[3] = (VkBufferMemoryBarrier2KHR(*)[]) (onetimeBufferMemoryBarriers2  + inoutsPerHeap);
	VkBufferMemoryBarrier2KHR (*computeBufferMemoryBarriers2)[2]  = (VkBufferMemoryBarrier2KHR(*)[]) (transferBufferMemoryBarriers2 + inoutsPerHeap);

	VkDependencyInfoKHR (*transferDependencyInfos)[2] = (VkDependencyInfoKHR(*)[]) (computeBufferMemoryBarriers2 + inoutsPerHeap);
	VkDependencyInfoKHR (*computeDependencyInfos)[2]  = (VkDependencyInfoKHR(*)[]) (transferDependencyInfos      + inoutsPerHeap);

	VkCommandPoolCreateInfo onetimeCommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	onetimeCommandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	onetimeCommandPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo transferCommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	transferCommandPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo computeCommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	computeCommandPoolCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;

	VK_CALL_RES(vkCreateCommandPool, device, &onetimeCommandPoolCreateInfo, g_allocator, &gpu->onetimeCommandPool)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateCommandPool)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkCreateCommandPool, device, &transferCommandPoolCreateInfo, g_allocator, &gpu->transferCommandPool)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateCommandPool)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkCreateCommandPool, device, &computeCommandPoolCreateInfo, g_allocator, &gpu->computeCommandPool)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkCreateCommandPool)
		free_recursive(dyMem);
		return false;
	}
#endif

	VkCommandPool onetimeCommandPool  = gpu->onetimeCommandPool;
	VkCommandPool transferCommandPool = gpu->transferCommandPool;
	VkCommandPool computeCommandPool  = gpu->computeCommandPool;

	VkCommandBufferAllocateInfo onetimeCommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	onetimeCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	onetimeCommandBufferAllocateInfo.commandPool        = onetimeCommandPool;
	onetimeCommandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	transferCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandPool        = transferCommandPool;
	transferCommandBufferAllocateInfo.commandBufferCount = inoutsPerHeap;

	VkCommandBufferAllocateInfo computeCommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	computeCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeCommandBufferAllocateInfo.commandPool        = computeCommandPool;
	computeCommandBufferAllocateInfo.commandBufferCount = inoutsPerHeap;

	VK_CALL_RES(vkAllocateCommandBuffers, device, &onetimeCommandBufferAllocateInfo, &gpu->onetimeCommandBuffer)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkAllocateCommandBuffers)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkAllocateCommandBuffers, device, &transferCommandBufferAllocateInfo, transferCommandBuffers)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkAllocateCommandBuffers)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkAllocateCommandBuffers, device, &computeCommandBufferAllocateInfo, computeCommandBuffers)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkAllocateCommandBuffers)
		free_recursive(dyMem);
		return false;
	}
#endif

	VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;

	for (uint32_t i = 0; i < inoutsPerBuffer; i++) {
		inBufferCopies[i].srcOffset = bytesPerInout * i;
		inBufferCopies[i].dstOffset = bytesPerInout * i;
		inBufferCopies[i].size      = bytesPerIn;

		outBufferCopies[i].srcOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].dstOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].size      = bytesPerOut;
	}

	uint32_t inoIndex = 0; // Inout-buffer index

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutsPerBuffer; j++, inoIndex++) {
			onetimeBufferMemoryBarriers2[inoIndex] = (VkBufferMemoryBarrier2KHR) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			onetimeBufferMemoryBarriers2[inoIndex].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			onetimeBufferMemoryBarriers2[inoIndex].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
			onetimeBufferMemoryBarriers2[inoIndex].srcQueueFamilyIndex = transferQueueFamilyIndex;
			onetimeBufferMemoryBarriers2[inoIndex].dstQueueFamilyIndex = computeQueueFamilyIndex;
			onetimeBufferMemoryBarriers2[inoIndex].buffer              = deviceLocalBuffers[i];
			onetimeBufferMemoryBarriers2[inoIndex].offset              = bytesPerInout * j;
			onetimeBufferMemoryBarriers2[inoIndex].size                = bytesPerIn;

			transferBufferMemoryBarriers2[inoIndex][0] = (VkBufferMemoryBarrier2KHR) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			transferBufferMemoryBarriers2[inoIndex][0].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][0].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][0].buffer              = deviceLocalBuffers[i];
			transferBufferMemoryBarriers2[inoIndex][0].offset              = bytesPerInout * j;
			transferBufferMemoryBarriers2[inoIndex][0].size                = bytesPerIn;

			transferBufferMemoryBarriers2[inoIndex][1] = (VkBufferMemoryBarrier2KHR) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			transferBufferMemoryBarriers2[inoIndex][1].dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][1].dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][1].buffer              = deviceLocalBuffers[i];
			transferBufferMemoryBarriers2[inoIndex][1].offset              = bytesPerInout * j + bytesPerIn;
			transferBufferMemoryBarriers2[inoIndex][1].size                = bytesPerOut;

			transferBufferMemoryBarriers2[inoIndex][2] = (VkBufferMemoryBarrier2KHR) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			transferBufferMemoryBarriers2[inoIndex][2].srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][2].dstStageMask  = VK_PIPELINE_STAGE_2_HOST_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][2].dstAccessMask = VK_ACCESS_2_HOST_READ_BIT_KHR;
			transferBufferMemoryBarriers2[inoIndex][2].buffer        = hostVisibleBuffers[i];
			transferBufferMemoryBarriers2[inoIndex][2].offset        = bytesPerInout * j + bytesPerIn;
			transferBufferMemoryBarriers2[inoIndex][2].size          = bytesPerOut;

			computeBufferMemoryBarriers2[inoIndex][0] = (VkBufferMemoryBarrier2KHR) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			computeBufferMemoryBarriers2[inoIndex][0].dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
			computeBufferMemoryBarriers2[inoIndex][0].dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR;
			computeBufferMemoryBarriers2[inoIndex][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][0].buffer              = deviceLocalBuffers[i];
			computeBufferMemoryBarriers2[inoIndex][0].offset              = bytesPerInout * j;
			computeBufferMemoryBarriers2[inoIndex][0].size                = bytesPerIn;

			computeBufferMemoryBarriers2[inoIndex][1] = (VkBufferMemoryBarrier2KHR) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			computeBufferMemoryBarriers2[inoIndex][1].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
			computeBufferMemoryBarriers2[inoIndex][1].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR;
			computeBufferMemoryBarriers2[inoIndex][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][1].buffer              = deviceLocalBuffers[i];
			computeBufferMemoryBarriers2[inoIndex][1].offset              = bytesPerInout * j + bytesPerIn;
			computeBufferMemoryBarriers2[inoIndex][1].size                = bytesPerOut;

			transferDependencyInfos[inoIndex][0] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			transferDependencyInfos[inoIndex][0].bufferMemoryBarrierCount = 2;
			transferDependencyInfos[inoIndex][0].pBufferMemoryBarriers    = &transferBufferMemoryBarriers2[inoIndex][0];

			transferDependencyInfos[inoIndex][1] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			transferDependencyInfos[inoIndex][1].bufferMemoryBarrierCount = 1;
			transferDependencyInfos[inoIndex][1].pBufferMemoryBarriers    = &transferBufferMemoryBarriers2[inoIndex][2];

			computeDependencyInfos[inoIndex][0] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			computeDependencyInfos[inoIndex][0].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[inoIndex][0].pBufferMemoryBarriers    = &computeBufferMemoryBarriers2[inoIndex][0];

			computeDependencyInfos[inoIndex][1] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			computeDependencyInfos[inoIndex][1].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[inoIndex][1].pBufferMemoryBarriers    = &computeBufferMemoryBarriers2[inoIndex][1];
		}
	}

	VkDependencyInfoKHR onetimeDependencyInfo = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
	onetimeDependencyInfo.bufferMemoryBarrierCount = inoutsPerHeap;
	onetimeDependencyInfo.pBufferMemoryBarriers    = onetimeBufferMemoryBarriers2;

	VkCommandBufferBeginInfo onetimeCommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	onetimeCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCommandBufferBeginInfo transferCommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VkCommandBufferBeginInfo computeCommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VK_CALL_RES(vkBeginCommandBuffer, onetimeCommandBuffer, &onetimeCommandBufferBeginInfo)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkBeginCommandBuffer)
		free_recursive(dyMem);
		return false;
	}
#endif

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL(vkCmdCopyBuffer, onetimeCommandBuffer, hostVisibleBuffers[i], deviceLocalBuffers[i], inoutsPerBuffer, inBufferCopies)
	}

	if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
		VK_CALL(vkCmdPipelineBarrier2KHR, onetimeCommandBuffer, &onetimeDependencyInfo)
	}

	VK_CALL_RES(vkEndCommandBuffer, onetimeCommandBuffer)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkEndCommandBuffer)
		free_recursive(dyMem);
		return false;
	}
#endif

	inoIndex = 0;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutsPerBuffer; j++, inoIndex++) {
			VK_CALL_RES(vkBeginCommandBuffer, transferCommandBuffers[inoIndex], &transferCommandBufferBeginInfo)
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkBeginCommandBuffer)
				free_recursive(dyMem);
				return false;
			}
#endif

			if (transferQueueTimestampValidBits) {
				VK_CALL(vkCmdResetQueryPool, transferCommandBuffers[inoIndex], queryPool, inoIndex * 4, 2)
				VK_CALL(vkCmdWriteTimestamp2KHR, transferCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_NONE_KHR, queryPool, inoIndex * 4)
			}

			VK_CALL(vkCmdCopyBuffer, transferCommandBuffers[inoIndex], hostVisibleBuffers[i], deviceLocalBuffers[i], 1, &inBufferCopies[j])

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				VK_CALL(vkCmdPipelineBarrier2KHR, transferCommandBuffers[inoIndex], &transferDependencyInfos[inoIndex][0])
			}

			VK_CALL(vkCmdCopyBuffer, transferCommandBuffers[inoIndex], deviceLocalBuffers[i], hostVisibleBuffers[i], 1, &outBufferCopies[j])

			VK_CALL(vkCmdPipelineBarrier2KHR, transferCommandBuffers[inoIndex], &transferDependencyInfos[inoIndex][1])

			if (transferQueueTimestampValidBits) {
				VK_CALL(vkCmdWriteTimestamp2KHR, transferCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_COPY_BIT_KHR, queryPool, inoIndex * 4 + 1)
			}

			VK_CALL_RES(vkEndCommandBuffer, transferCommandBuffers[inoIndex])
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkEndCommandBuffer)
				free_recursive(dyMem);
				return false;
			}
#endif

			VK_CALL_RES(vkBeginCommandBuffer, computeCommandBuffers[inoIndex], &computeCommandBufferBeginInfo)
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkBeginCommandBuffer)
				free_recursive(dyMem);
				return false;
			}
#endif

			if (computeQueueTimestampValidBits) {
				VK_CALL(vkCmdResetQueryPool, computeCommandBuffers[inoIndex], queryPool, inoIndex * 4 + 2, 2)
				VK_CALL(vkCmdWriteTimestamp2KHR, computeCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_NONE_KHR, queryPool, inoIndex * 4 + 2)
			}

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				VK_CALL(vkCmdPipelineBarrier2KHR, computeCommandBuffers[inoIndex], &computeDependencyInfos[inoIndex][0])
			}

			VK_CALL(vkCmdBindDescriptorSets, computeCommandBuffers[inoIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[inoIndex], 0, NULL)
			VK_CALL(vkCmdBindPipeline, computeCommandBuffers[inoIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline)
			VK_CALL(vkCmdDispatchBase, computeCommandBuffers[inoIndex], 0, 0, 0, workgroupCount, 1, 1)

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				VK_CALL(vkCmdPipelineBarrier2KHR, computeCommandBuffers[inoIndex], &computeDependencyInfos[inoIndex][1])
			}

			if (computeQueueTimestampValidBits) {
				VK_CALL(vkCmdWriteTimestamp2KHR, computeCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR, queryPool, inoIndex * 4 + 3)
			}

			VK_CALL_RES(vkEndCommandBuffer, computeCommandBuffers[inoIndex])
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkEndCommandBuffer)
				free_recursive(dyMem);
				return false;
			}
#endif
		}
	}

	if (gpu->usingMaintenance4) {
		VK_CALL(vkDestroyPipelineLayout, device, pipelineLayout, g_allocator)
		gpu->pipelineLayout = VK_NULL_HANDLE;
	}

	VkSemaphoreTypeCreateInfoKHR semaphoreTypeCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR};
	semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
	semaphoreTypeCreateInfo.initialValue  = 0;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VK_CALL_RES(vkCreateSemaphore, device, &semaphoreCreateInfo, g_allocator, &semaphores[i])
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkCreateSemaphore)
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	free_recursive(dyMem);

#ifndef NDEBUG
	if(gpu->debugUtilsMessenger) {
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL,   (uint64_t) onetimeCommandPool,   "Onetime");
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) onetimeCommandBuffer, "Onetime");

		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) transferCommandPool, "Transfer");
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) computeCommandPool,  "Compute");

		inoIndex = 0;

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			for (uint32_t j = 0; j < inoutsPerBuffer; j++, inoIndex++) {
				char objectName[68];
				char specs[60];

				sprintf(specs, ", Inout %u/%u, Buffer %u/%u", j + 1, inoutsPerBuffer, i + 1, buffersPerHeap);

				strcpy(objectName, "Transfer");
				strcat(objectName, specs);

				set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) transferCommandBuffers[inoIndex], objectName);

				strcpy(objectName, "Compute");
				strcat(objectName, specs);

				set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) computeCommandBuffers[inoIndex], objectName);
			}
		}
	}
#endif

	END_FUNC
	return true;
}

bool submit_commands(Gpu* gpu)
{
	BEGIN_FUNC

	const VkDeviceMemory*  restrict hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer* restrict transferCommandBuffers    = gpu->transferCommandBuffers;
	const VkCommandBuffer* restrict computeCommandBuffers     = gpu->computeCommandBuffers;
	const VkSemaphore*     restrict semaphores                = gpu->semaphores;

	Value*       const* restrict mappedInBuffers  = gpu->mappedInBuffers;
	const Steps* const* restrict mappedOutBuffers = (const Steps* const*) gpu->mappedOutBuffers;

	VkDevice device        = gpu->device;
	VkQueue  transferQueue = gpu->transferQueue;
	VkQueue  computeQueue  = gpu->computeQueue;

	VkCommandPool   onetimeCommandPool   = gpu->onetimeCommandPool;
	VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;
	VkQueryPool     queryPool            = gpu->queryPool;

	VkDeviceSize bytesPerIn    = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut   = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t valuesPerInout  = gpu->valuesPerInout;
	uint32_t valuesPerHeap   = gpu->valuesPerHeap;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap   = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap  = gpu->buffersPerHeap;

	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	float timestampPeriod = gpu->timestampPeriod;
	bool  hostNonCoherent = gpu->hostNonCoherent;

	VkResult vkres;

	DyData dyData;
	DyArray* dyMem = DyArray_create(sizeof(DyData), 1);
#ifndef NDEBUG
	if (!dyMem)
		return false;
#endif

	size_t size =
		inoutsPerHeap     * sizeof(Value) +
		inoutsPerHeap * 2 * sizeof(VkMappedMemoryRange) +
		inoutsPerHeap * 2 * sizeof(VkSubmitInfo2KHR) +
		inoutsPerHeap * 2 * sizeof(VkCommandBufferSubmitInfoKHR) +
		inoutsPerHeap * 4 * sizeof(VkSemaphoreSubmitInfoKHR) +
		inoutsPerHeap * 2 * sizeof(VkSemaphoreWaitInfoKHR);

	Value* testedValues = (Value*) malloc(size);
#ifndef NDEBUG
	if (!testedValues) {
		MALLOC_FAILURE(testedValues, size)
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {testedValues, free};
	DyArray_append(dyMem, &dyData);

	VkMappedMemoryRange* hostVisibleInBuffersMappedMemoryRanges  = (VkMappedMemoryRange*) (testedValues                           + inoutsPerHeap);
	VkMappedMemoryRange* hostVisibleOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (hostVisibleInBuffersMappedMemoryRanges + inoutsPerHeap);

	VkSubmitInfo2KHR* transferSubmitInfos2 = (VkSubmitInfo2KHR*) (hostVisibleOutBuffersMappedMemoryRanges + inoutsPerHeap);
	VkSubmitInfo2KHR* computeSubmitInfos2  = (VkSubmitInfo2KHR*) (transferSubmitInfos2                    + inoutsPerHeap);

	VkCommandBufferSubmitInfoKHR* transferCommandBufferSubmitInfos = (VkCommandBufferSubmitInfoKHR*) (computeSubmitInfos2              + inoutsPerHeap);
	VkCommandBufferSubmitInfoKHR* computeCommandBufferSubmitInfos  = (VkCommandBufferSubmitInfoKHR*) (transferCommandBufferSubmitInfos + inoutsPerHeap);

	VkSemaphoreSubmitInfoKHR* transferWaitSemaphoreSubmitInfos   = (VkSemaphoreSubmitInfoKHR*) (computeCommandBufferSubmitInfos    + inoutsPerHeap);
	VkSemaphoreSubmitInfoKHR* transferSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (transferWaitSemaphoreSubmitInfos   + inoutsPerHeap);
	VkSemaphoreSubmitInfoKHR* computeWaitSemaphoreSubmitInfos    = (VkSemaphoreSubmitInfoKHR*) (transferSignalSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfoKHR* computeSignalSemaphoreSubmitInfos  = (VkSemaphoreSubmitInfoKHR*) (computeWaitSemaphoreSubmitInfos    + inoutsPerHeap);

	VkSemaphoreWaitInfoKHR* transferSemaphoreWaitInfos = (VkSemaphoreWaitInfoKHR*) (computeSignalSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreWaitInfoKHR* computeSemaphoreWaitInfos  = (VkSemaphoreWaitInfoKHR*) (transferSemaphoreWaitInfos        + inoutsPerHeap);

	DyArray* highestStepValues = DyArray_create(sizeof(Value), 64);
#ifndef NDEBUG
	if (!highestStepValues) {
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {highestStepValues, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	DyArray* highestStepCounts = DyArray_create(sizeof(Steps), 64);
#ifndef NDEBUG
	if (!highestStepCounts) {
		free_recursive(dyMem);
		return false;
	}
#endif

	dyData = (DyData) {highestStepCounts, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	uint32_t inoIndex = 0;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutsPerBuffer; j++, inoIndex++) {
			hostVisibleInBuffersMappedMemoryRanges[inoIndex] = (VkMappedMemoryRange) {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			hostVisibleInBuffersMappedMemoryRanges[inoIndex].memory = hostVisibleDeviceMemories[i];
			hostVisibleInBuffersMappedMemoryRanges[inoIndex].offset = bytesPerInout * j;
			hostVisibleInBuffersMappedMemoryRanges[inoIndex].size   = bytesPerIn;

			hostVisibleOutBuffersMappedMemoryRanges[inoIndex] = (VkMappedMemoryRange) {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			hostVisibleOutBuffersMappedMemoryRanges[inoIndex].memory = hostVisibleDeviceMemories[i];
			hostVisibleOutBuffersMappedMemoryRanges[inoIndex].offset = bytesPerInout * j + bytesPerIn;
			hostVisibleOutBuffersMappedMemoryRanges[inoIndex].size   = bytesPerOut;
		}
	}

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferCommandBufferSubmitInfos[i] = (VkCommandBufferSubmitInfoKHR) {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR};
		transferCommandBufferSubmitInfos[i].commandBuffer = transferCommandBuffers[i];

		computeCommandBufferSubmitInfos[i] = (VkCommandBufferSubmitInfoKHR) {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR};
		computeCommandBufferSubmitInfos[i].commandBuffer = computeCommandBuffers[i];

		transferWaitSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		transferWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferWaitSemaphoreSubmitInfos[i].value     = 0;
		transferWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include acquire operation

		transferSignalSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		transferSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferSignalSemaphoreSubmitInfos[i].value     = 1;
		transferSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include release operation

		computeWaitSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		computeWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeWaitSemaphoreSubmitInfos[i].value     = 1;
		computeWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include acquire operation

		computeSignalSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		computeSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeSignalSemaphoreSubmitInfos[i].value     = 2;
		computeSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include release operation

		transferSubmitInfos2[i] = (VkSubmitInfo2KHR) {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
		transferSubmitInfos2[i].waitSemaphoreInfoCount   = 1;
		transferSubmitInfos2[i].pWaitSemaphoreInfos      = &transferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos2[i].commandBufferInfoCount   = 1;
		transferSubmitInfos2[i].pCommandBufferInfos      = &transferCommandBufferSubmitInfos[i];
		transferSubmitInfos2[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos2[i].pSignalSemaphoreInfos    = &transferSignalSemaphoreSubmitInfos[i];

		computeSubmitInfos2[i] = (VkSubmitInfo2KHR) {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
		computeSubmitInfos2[i].waitSemaphoreInfoCount   = 1;
		computeSubmitInfos2[i].pWaitSemaphoreInfos      = &computeWaitSemaphoreSubmitInfos[i];
		computeSubmitInfos2[i].commandBufferInfoCount   = 1;
		computeSubmitInfos2[i].pCommandBufferInfos      = &computeCommandBufferSubmitInfos[i];
		computeSubmitInfos2[i].signalSemaphoreInfoCount = 1;
		computeSubmitInfos2[i].pSignalSemaphoreInfos    = &computeSignalSemaphoreSubmitInfos[i];

		transferSemaphoreWaitInfos[i] = (VkSemaphoreWaitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR};
		transferSemaphoreWaitInfos[i].semaphoreCount = 1;
		transferSemaphoreWaitInfos[i].pSemaphores    = &semaphores[i];
		transferSemaphoreWaitInfos[i].pValues        = &transferSignalSemaphoreSubmitInfos[i].value;

		computeSemaphoreWaitInfos[i] = (VkSemaphoreWaitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR};
		computeSemaphoreWaitInfos[i].semaphoreCount = 1;
		computeSemaphoreWaitInfos[i].pSemaphores    = &semaphores[i];
		computeSemaphoreWaitInfos[i].pValues        = &computeSignalSemaphoreSubmitInfos[i].value;
	}

	VkCommandBufferSubmitInfoKHR onetimeCommandBufferSubmitInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR};
	onetimeCommandBufferSubmitInfo.commandBuffer = onetimeCommandBuffer;

	VkSubmitInfo2KHR onetimeSubmitInfo2 = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
	onetimeSubmitInfo2.commandBufferInfoCount   = 1;
	onetimeSubmitInfo2.pCommandBufferInfos      = &onetimeCommandBufferSubmitInfo;
	onetimeSubmitInfo2.signalSemaphoreInfoCount = inoutsPerHeap;
	onetimeSubmitInfo2.pSignalSemaphoreInfos    = transferSignalSemaphoreSubmitInfos;

	SET_128BIT_INT(testedValues[0], MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM)

	for (uint32_t i = 1; i < inoutsPerHeap; i++)
		testedValues[i] = testedValues[i - 1] + valuesPerInout * 2;

	for (uint32_t i = 0; i < inoutsPerHeap; i++)
		writeInBuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hostVisibleInBuffersMappedMemoryRanges)
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkFlushMappedMemoryRanges)
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	clock_t bmarkStart = clock();

	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, 1, &onetimeSubmitInfo2, VK_NULL_HANDLE)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkQueueSubmit2KHR)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, inoutsPerHeap, computeSubmitInfos2, VK_NULL_HANDLE)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkQueueSubmit2KHR)
		free_recursive(dyMem);
		return false;
	}
#endif

	atomic_bool input;
	atomic_init(&input, false);

	pthread_t waitThread;
	int ires = pthread_create(&waitThread, NULL, wait_for_input, &input);
#ifndef NDEBUG
	if (ires) {
		PCREATE_FAILURE(ires, &waitThread, NULL, NULL)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[0], ~0ULL)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkWaitSemaphoresKHR)
		free_recursive(dyMem);
		return false;
	}
#endif

	VK_CALL(vkDestroyCommandPool, device, onetimeCommandPool, g_allocator)
	gpu->onetimeCommandPool = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < inoutsPerHeap; i++)
		writeInBuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hostVisibleInBuffersMappedMemoryRanges)
#ifndef NDEBUG
		if (vkres) {
			VULKAN_FAILURE(vkFlushMappedMemoryRanges)
			free_recursive(dyMem);
			return false;
		}
#endif
	}

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferWaitSemaphoreSubmitInfos[i].value   += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, inoutsPerHeap, transferSubmitInfos2, VK_NULL_HANDLE)
#ifndef NDEBUG
	if (vkres) {
		VULKAN_FAILURE(vkQueueSubmit2KHR)
		free_recursive(dyMem);
		return false;
	}
#endif

	Value tested;
	Value prev;
	SET_128BIT_INT(tested, MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM)
	SET_128BIT_INT(prev,   MAX_STEP_VALUE_TOP, MAX_STEP_VALUE_BOTTOM)

	Value total   = 0;
	Steps longest = MAX_STEP_COUNT;

	// ===== Enter main loop =====
	for (uint64_t i = 0; !atomic_load(&input); i++) {
		clock_t mainLoopBmarkStart = clock();
		Value   initialValue       = tested;

		float readBmarkTotal                  = 0.f;
		float writeBmarkTotal                 = 0.f;
		float waitComputeSemaphoreBmarkTotal  = 0.f;
		float waitTransferSemaphoreBmarkTotal = 0.f;
		float computeBmarkTotal               = 0.f;
		float transferBmarkTotal              = 0.f;

		printf("Benchmarks #%llu\n", i + 1);

		for (uint32_t j = 0; j < inoutsPerHeap; j++) {
			uint64_t timestamps[2];
			float computeBmark  = 0.f;
			float transferBmark = 0.f;

			clock_t waitComputeSemaphoreBmarkStart = clock();
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &computeSemaphoreWaitInfos[j], ~0ULL)
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkWaitSemaphoresKHR)
				free_recursive(dyMem);
				return false;
			}
#endif
			clock_t waitComputeSemaphoreBmarkEnd = clock();

			if (computeQueueTimestampValidBits) {
				VK_CALL_RES(vkGetQueryPoolResults, device, queryPool, j * 4 + 2, 2, sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT)
#ifndef NDEBUG
				if (vkres) {
					VULKAN_FAILURE(vkGetQueryPoolResults)
					free_recursive(dyMem);
					return false;
				}
#endif
				computeBmark = (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			computeWaitSemaphoreSubmitInfos[j].value   += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, 1, &computeSubmitInfos2[j], VK_NULL_HANDLE)
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkQueueSubmit2KHR)
				free_recursive(dyMem);
				return false;
			}
#endif

			clock_t waitTransferSemaphoreBmarkStart = clock();
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[j], ~0ULL)
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkWaitSemaphoresKHR)
				free_recursive(dyMem);
				return false;
			}
#endif
			clock_t waitTransferSemaphoreBmarkEnd = clock();

			if (transferQueueTimestampValidBits) {
				VK_CALL_RES(vkGetQueryPoolResults, device, queryPool, j * 4, 2, sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT)
#ifndef NDEBUG
				if (vkres) {
					VULKAN_FAILURE(vkGetQueryPoolResults)
					free_recursive(dyMem);
					return false;
				}
#endif
				transferBmark = (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (hostNonCoherent) {
				VK_CALL_RES(vkInvalidateMappedMemoryRanges, device, 1, &hostVisibleOutBuffersMappedMemoryRanges[j])
#ifndef NDEBUG
				if (vkres) {
					VULKAN_FAILURE(vkInvalidateMappedMemoryRanges)
					free_recursive(dyMem);
					return false;
				}
#endif
			}

			clock_t readBmarkStart = clock();
			readOutBuffer(mappedOutBuffers[j], &tested, &prev, &longest, highestStepValues, highestStepCounts, valuesPerInout);
			clock_t readBmarkEnd = clock();

			clock_t writeBmarkStart = clock();
			writeInBuffer(mappedInBuffers[j], &testedValues[j], valuesPerInout, valuesPerHeap);
			clock_t writeBmarkEnd = clock();

			if (hostNonCoherent) {
				VK_CALL_RES(vkFlushMappedMemoryRanges, device, 1, &hostVisibleInBuffersMappedMemoryRanges[j])
#ifndef NDEBUG
				if (vkres) {
					VULKAN_FAILURE(vkFlushMappedMemoryRanges)
					free_recursive(dyMem);
					return false;
				}
#endif
			}

			transferWaitSemaphoreSubmitInfos[j].value   += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, 1, &transferSubmitInfos2[j], VK_NULL_HANDLE)
#ifndef NDEBUG
			if (vkres) {
				VULKAN_FAILURE(vkQueueSubmit2KHR)
				free_recursive(dyMem);
				return NULL;
			}
#endif

			float readBmark                  = get_benchmark(readBmarkStart, readBmarkEnd);
			float writeBmark                 = get_benchmark(writeBmarkStart, writeBmarkEnd);
			float waitComputeSemaphoreBmark  = get_benchmark(waitComputeSemaphoreBmarkStart, waitComputeSemaphoreBmarkEnd);
			float waitTransferSemaphoreBmark = get_benchmark(waitTransferSemaphoreBmarkStart, waitTransferSemaphoreBmarkEnd);

			readBmarkTotal                  += readBmark;
			writeBmarkTotal                 += writeBmark;
			computeBmarkTotal               += computeBmark;
			transferBmarkTotal              += transferBmark;
			waitComputeSemaphoreBmarkTotal  += waitComputeSemaphoreBmark;
			waitTransferSemaphoreBmarkTotal += waitTransferSemaphoreBmark;

			printf(
				"\tInout-buffer %u/%u\n"
				"\t\tReading buffers:      %5.0fms\n"
				"\t\tWriting buffers:      %5.0fms\n"
				"\t\tCompute execution:    %5.0fms\n"
				"\t\tTransfer execution:   %5.0fms\n"
				"\t\tWaiting for compute:  %5.0fms\n"
				"\t\tWaiting for transfer: %5.0fms\n",
				j + 1, inoutsPerHeap,
				(double) readBmark,                 (double) writeBmark,
				(double) computeBmark,              (double) transferBmark,
				(double) waitComputeSemaphoreBmark, (double) waitTransferSemaphoreBmark
			);
		}

		total += valuesPerHeap;

		clock_t mainLoopBmarkEnd = clock();
		float   mainLoopBmark    = get_benchmark(mainLoopBmarkStart, mainLoopBmarkEnd);

		printf(
			"\tMain loop: %.0fms\n"
			"\tReading buffers:      (total) %5.0fms, (avg) %5.0fms\n"
			"\tWriting buffers:      (total) %5.0fms, (avg) %5.0fms\n"
			"\tCompute execution:    (total) %5.0fms, (avg) %5.0fms\n"
			"\tTransfer execution:   (total) %5.0fms, (avg) %5.0fms\n"
			"\tWaiting for compute:  (total) %5.0fms, (avg) %5.0fms\n"
			"\tWaiting for transfer: (total) %5.0fms, (avg) %5.0fms\n"
			"\tInitial value: 0x %016llx %016llx\n"
			"\tFinal value:   0x %016llx %016llx\n\n",
			(double) mainLoopBmark,
			(double) readBmarkTotal,                  (double) (readBmarkTotal  / inoutsPerHeap),
			(double) writeBmarkTotal,                 (double) (writeBmarkTotal / inoutsPerHeap),
			(double) computeBmarkTotal,               (double) (computeBmarkTotal  / inoutsPerHeap),
			(double) transferBmarkTotal,              (double) (transferBmarkTotal / inoutsPerHeap),
			(double) waitComputeSemaphoreBmarkTotal,  (double) (waitComputeSemaphoreBmarkTotal  / inoutsPerHeap),
			(double) waitTransferSemaphoreBmarkTotal, (double) (waitTransferSemaphoreBmarkTotal / inoutsPerHeap),
			TOP_128BIT_INT(initialValue), BOTTOM_128BIT_INT(initialValue),
			TOP_128BIT_INT(tested - 2),   BOTTOM_128BIT_INT(tested - 2)
		);
	}
	NEWLINE()

	clock_t bmarkEnd = clock();
	float   bmark    = get_benchmark(bmarkStart, bmarkEnd);

	size_t count = DyArray_size(highestStepValues);

	printf(
		"Set of starting values tested: [0x %016llx %016llx, 0x %016llx %016llx]\n"
		"Continue on: 0x %016llx %016llx\n"
		"Highest step counts (%zu):\n",
		MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM,
		TOP_128BIT_INT(tested - 2), BOTTOM_128BIT_INT(tested - 2),
		TOP_128BIT_INT(tested),     BOTTOM_128BIT_INT(tested),
		count
	);

	for (uint32_t i = 0; i < count; i++) {
		Value value;
		Steps steps;

		DyArray_get(highestStepValues, &value, i);
		DyArray_get(highestStepCounts, &steps, i);

		printf(
			"\t%u)\tsteps(0x %016llx %016llx) = %hu\n",
			i + 1, TOP_128BIT_INT(value), BOTTOM_128BIT_INT(value), steps
		);
	}
	NEWLINE()

	printf("Time: %.0fms\nSpeed: %.0f/s\n", (double) bmark, 1000 * total / (double) bmark);

	ires = pthread_join(waitThread, NULL);
#ifndef NDEBUG
	if (ires) {
		PJOIN_FAILURE(ires, waitThread, NULL)
		free_recursive(dyMem);
		return false;
	}
#endif

	free_recursive(dyMem);

	END_FUNC
	return true;
}

bool destroy_gpu(Gpu* gpu)
{
	BEGIN_FUNC

	VkInstance instance = volkGetLoadedInstance();

	const VkBuffer*       restrict hostVisibleBuffers        = gpu->hostVisibleBuffers;
	const VkBuffer*       restrict deviceLocalBuffers        = gpu->deviceLocalBuffers;
	const VkDeviceMemory* restrict hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* restrict deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	const VkSemaphore*    restrict semaphores                = gpu->semaphores;

	VkDebugUtilsMessengerEXT debugUtilsMessenger = gpu->debugUtilsMessenger;

	VkDevice device = gpu->device;

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool      descriptorPool      = gpu->descriptorPool;
	VkShaderModule        shaderModule        = gpu->shaderModule;
	VkPipelineCache       pipelineCache       = gpu->pipelineCache;
	VkPipelineLayout      pipelineLayout      = gpu->pipelineLayout;
	VkPipeline            pipeline            = gpu->pipeline;
	VkCommandPool         onetimeCommandPool  = gpu->onetimeCommandPool;
	VkCommandPool         transferCommandPool = gpu->transferCommandPool;
	VkCommandPool         computeCommandPool  = gpu->computeCommandPool;
	VkQueryPool           queryPool           = gpu->queryPool;

	uint32_t inoutsPerHeap  = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	void* dynamicMemory = gpu->dynamicMemory;

	VkResult vkres;

	if(device) {
		VK_CALL(vkDestroyShaderModule,        device, shaderModule,        g_allocator)
		VK_CALL(vkDestroyPipelineCache,       device, pipelineCache,       g_allocator)
		VK_CALL(vkDestroyPipelineLayout,      device, pipelineLayout,      g_allocator)
		VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, g_allocator)

		// Make sure no command buffers are in the pending state
		VK_CALL_RES(vkDeviceWaitIdle, device)
#ifndef NDEBUG
		if (vkres)
			VULKAN_FAILURE(vkDeviceWaitIdle)
#endif

		for (uint32_t i = 0; i < inoutsPerHeap; i++)
			VK_CALL(vkDestroySemaphore, device, semaphores[i], g_allocator)

		VK_CALL(vkDestroyCommandPool, device, onetimeCommandPool,  g_allocator)
		VK_CALL(vkDestroyCommandPool, device, computeCommandPool,  g_allocator)
		VK_CALL(vkDestroyCommandPool, device, transferCommandPool, g_allocator)

		VK_CALL(vkDestroyPipeline,       device, pipeline,       g_allocator)
		VK_CALL(vkDestroyQueryPool,      device, queryPool,      g_allocator)
		VK_CALL(vkDestroyDescriptorPool, device, descriptorPool, g_allocator)

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			VK_CALL(vkDestroyBuffer, device, hostVisibleBuffers[i], g_allocator)
			VK_CALL(vkDestroyBuffer, device, deviceLocalBuffers[i], g_allocator)

			VK_CALL(vkFreeMemory, device, hostVisibleDeviceMemories[i], g_allocator)
			VK_CALL(vkFreeMemory, device, deviceLocalDeviceMemories[i], g_allocator)
		}

		VK_CALL(vkDestroyDevice, device, g_allocator)
	}

	if(instance) {
#ifndef NDEBUG
		VK_CALL(vkDestroyDebugUtilsMessengerEXT, instance, debugUtilsMessenger, g_allocator)
#endif

		VK_CALL(vkDestroyInstance, instance, g_allocator)
	}

	free(dynamicMemory);
	volkFinalize();

	END_FUNC
	return true;
}

void* wait_for_input(void* ptr)
{
	puts("Calculating... press enter/return to stop\n");
	getchar();
	puts("Stopping...\n");

	atomic_bool* input = (atomic_bool*) ptr;
	atomic_store(input, true);

	return NULL;
}

void writeInBuffer(Value* restrict mappedInBuffer, Value* restrict firstValue, uint32_t valuesPerInoutBuffer, uint32_t valuesPerHeap)
{
	Value value = *firstValue;

	for (uint32_t i = 0; i < valuesPerInoutBuffer; i++, value += 2)
		mappedInBuffer[i] = value;

	*firstValue += valuesPerHeap * 2;
}

void readOutBuffer(
	const Steps* restrict mappedOutBuffer,
	Value*   restrict firstValue,
	Value*   restrict prev,
	Steps*   restrict longest,
	DyArray* restrict highestStepValues,
	DyArray* restrict highestStepCounts,
	uint32_t valuesPerInoutBuffer
)
{
	Value value      = *firstValue - 2;
	Value value0mod1 = *prev;
	Steps steps0mod1 = *longest;

	for (uint32_t i = 0; i < valuesPerInoutBuffer; i++) {
		Steps steps = steps0mod1 + 1;
		value++;

		if (value == value0mod1 * 2) {
			value0mod1 = value;
			steps0mod1 = steps;

			DyArray_append(highestStepValues, &value);
			DyArray_append(highestStepCounts, &steps);
		}

		steps = mappedOutBuffer[i];
		value++;

		if (steps > steps0mod1) {
			value0mod1 = value;
			steps0mod1 = steps;

			DyArray_append(highestStepValues, &value);
			DyArray_append(highestStepCounts, &steps);
		}
	}

	*firstValue = value + 2;
	*prev       = value0mod1;
	*longest    = steps0mod1;
}
