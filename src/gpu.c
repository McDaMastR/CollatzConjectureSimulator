/* 
 * Collatz Conjecture Simulator
 * Copyright (C) 2024  Seth Isaiah McDonald <seth.i.mcdonald@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "defs.h"

#include <stdatomic.h>
#include <string.h>

static VkInstance       g_instance       = VK_NULL_HANDLE;
static VkPhysicalDevice g_physicalDevice = VK_NULL_HANDLE;
static VkDevice         g_device         = VK_NULL_HANDLE;

static VkQueue g_transferQueue = VK_NULL_HANDLE;
static VkQueue g_computeQueue  = VK_NULL_HANDLE;

#ifndef NDEBUG
static VkDebugUtilsMessengerEXT g_debugMessenger     = VK_NULL_HANDLE;
static uint64_t                 g_debugCallbackCount = 0;
#endif

static VkAllocationCallbacks* g_allocator = NULL;
#if LOG_VULKAN_ALLOCATIONS
static VkAllocationCallbacks      g_allocationCallbacks      = {0};
static AllocationCallbackCounts_t g_allocationCallbackCounts = {0};
#endif

static float get_benchmark(clock_t start, clock_t end)
{
	return (end - start) * MS_PER_CLOCK;
}

// "*Data" structures hold pointers to dynamically allocated memory
// whose lifetimes are encapsulated in the corresponding function
typedef struct CreateInstanceData
{
	void* properties;
} CreateInstanceData_t;

// "free_*Data" functions free all dynamically allocated memory
// given in the corresponding "*Data" structure
static void free_CreateInstanceData(CreateInstanceData_t data)
{
	free(data.properties);
}

bool create_instance(void)
{
	BEGIN_FUNC

	CreateInstanceData_t data = {0};
	bool initResult;

	VkResult result = volkInitialize();
	if (result) {
		VINIT_FAILURE()
		free_CreateInstanceData(data);
		return false;
	}

	uint32_t appApiVersion  = VK_API_VERSION_1_1;
	uint32_t instApiVersion = volkGetInstanceVersion();
	if (instApiVersion < appApiVersion) {
		VINSTVERS_FAILURE(instApiVersion)
		free_CreateInstanceData(data);
		return false;
	}

#if LOG_VULKAN_ALLOCATIONS
	GET_INIT_RESULT(init_alloc_logfile())
#ifndef NDEBUG
	if (!initResult) {
		free_CreateInstanceData(data);
		return false;
	}
#endif

	g_allocationCallbacks.pUserData             = &g_allocationCallbackCounts;
	g_allocationCallbacks.pfnAllocation         = allocation_callback;
	g_allocationCallbacks.pfnReallocation       = reallocation_callback;
	g_allocationCallbacks.pfnFree               = free_callback;
	g_allocationCallbacks.pfnInternalAllocation = internal_allocation_callback;
	g_allocationCallbacks.pfnInternalFree       = internal_free_callback;

	g_allocator = &g_allocationCallbacks;
#endif

#ifndef NDEBUG
	GET_INIT_RESULT(init_debug_logfile())
	if (!initResult) {
		free_CreateInstanceData(data);
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
	debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsMessengerCreateInfo.pNext = NULL;
	debugUtilsMessengerCreateInfo.flags           = 0;
	debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType     = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback = debug_callback;
	debugUtilsMessengerCreateInfo.pUserData       = &g_debugCallbackCount;
#endif

	uint32_t layerPropertyCount;
	GET_RESULT(vkEnumerateInstanceLayerProperties(&layerPropertyCount, NULL))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEnumerateInstanceLayerProperties, 2, 'p', &layerPropertyCount, 'p', NULL)
		free_CreateInstanceData(data);
		return false;
	}
#endif

	uint32_t extensionPropertyCount;
	GET_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertyCount, NULL))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEnumerateInstanceExtensionProperties, 3, 'p', NULL, 'p', &extensionPropertyCount, 'p', NULL)
		free_CreateInstanceData(data);
		return false;
	}
#endif

	size_t size =
		layerPropertyCount     * sizeof(VkLayerProperties) +
		extensionPropertyCount * sizeof(VkExtensionProperties);

	data.properties = malloc(size);
#ifndef NDEBUG
	if (!data.properties && size) {
		MALLOC_FAILURE(data.properties)
		free_CreateInstanceData(data);
		return false;
	}
#endif

	VkLayerProperties*     layersProperties     = (VkLayerProperties*) data.properties;
	VkExtensionProperties* extensionsProperties = (VkExtensionProperties*) (layersProperties + layerPropertyCount);

	GET_RESULT(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layersProperties))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEnumerateInstanceLayerProperties, 2, 'p', &layerPropertyCount, 'p', layersProperties)
		free_CreateInstanceData(data);
		return false;
	}
#endif

	GET_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertyCount, extensionsProperties))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEnumerateInstanceExtensionProperties, 3, 'p', NULL, 'p', &extensionPropertyCount, 'p', extensionsProperties)
		free_CreateInstanceData(data);
		return false;
	}
#endif

	uint32_t enabledLayerCount = 0;
	const char* enabledLayers[2];
	for (uint32_t i = 0; i < layerPropertyCount; i++) {
		const char* layerName = layersProperties[i].layerName;

#if EXTENSION_LAYERS
		if (!strcmp(layerName, VK_KHR_SYNCHRONIZATION_2_LAYER_NAME)) {
			enabledLayers[enabledLayerCount] = layersProperties[i].layerName;
			enabledLayerCount++;
			continue;
		}
#endif

#if VALIDATION_LAYERS
		if (!strcmp(layerName, VK_KHR_VALIDATION_LAYER_NAME)) {
			enabledLayers[enabledLayerCount] = layersProperties[i].layerName;
			enabledLayerCount++;
			continue;
		}
#endif
	}

	const void*  nextChain = NULL;
	const void** next      = &nextChain;

	bool khrPortabilityEnumeration = false;
	bool extDebugUtils             = false;

	uint32_t enabledExtensionCount = 0;
	const char* enabledExtensions[2];
	for (uint32_t i = 0; i < extensionPropertyCount; i++) {
		const char* extensionName = extensionsProperties[i].extensionName;

		if (!strcmp(extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			khrPortabilityEnumeration = true;
			enabledExtensions[enabledExtensionCount] = extensionsProperties[i].extensionName;
			enabledExtensionCount++;
		}

#ifndef NDEBUG
		else if (!strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			extDebugUtils = true;
			enabledExtensions[enabledExtensionCount] = extensionsProperties[i].extensionName;
			enabledExtensionCount++;

			*next = &debugUtilsMessengerCreateInfo;
			next  = &debugUtilsMessengerCreateInfo.pNext;
		}
#endif
	}

	VkApplicationInfo applicationInfo;
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = NULL;
	applicationInfo.pApplicationName   = PROGRAM_NAME;
	applicationInfo.applicationVersion = 0;
	applicationInfo.pEngineName        = NULL;
	applicationInfo.engineVersion      = 0;
	applicationInfo.apiVersion         = appApiVersion;

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = nextChain;
	instanceCreateInfo.flags                   = khrPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceCreateInfo.pApplicationInfo        = &applicationInfo;
	instanceCreateInfo.enabledLayerCount       = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames     = enabledLayers;
	instanceCreateInfo.enabledExtensionCount   = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

	printf("Enabled instance layers (%u):\n", enabledLayerCount);
	for (uint32_t i = 0; i < enabledLayerCount; i++)
		printf("\t%u) %s\n", i + 1, enabledLayers[i]);
	NEWLINE

	printf("Enabled instance extensions (%u):\n", enabledExtensionCount);
	for (uint32_t i = 0; i < enabledExtensionCount; i++)
		printf("\t%u) %s\n", i + 1, enabledExtensions[i]);
	NEWLINE

	GET_RESULT(vkCreateInstance(&instanceCreateInfo, g_allocator, &g_instance))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateInstance, 3, 'p', &instanceCreateInfo, 'p', g_allocator, 'p', &g_instance)
		free_CreateInstanceData(data);
		return false;
	}
#endif

	volkLoadInstanceOnly(g_instance);
	free_CreateInstanceData(data);

#ifndef NDEBUG
	if (extDebugUtils) {
		debugUtilsMessengerCreateInfo.pNext = NULL;
		GET_RESULT(vkCreateDebugUtilsMessengerEXT(g_instance, &debugUtilsMessengerCreateInfo, g_allocator, &g_debugMessenger))
		if (result) {
			VULKAN_FAILURE(vkCreateDebugUtilsMessengerEXT, 4, 'p', g_instance, 'p', &debugUtilsMessengerCreateInfo, 'p', g_allocator, 'p', &g_debugMessenger)
			return false;
		}
	}
#endif

	END_FUNC
	return true;
}

typedef struct SelectDeviceData
{
	void* devices;
	void* properties;
} SelectDeviceData_t;

static void free_SelectDeviceData(SelectDeviceData_t data)
{
	free(data.devices);
	free(data.properties);
}

bool select_device(Gpu_t* gpu)
{
	BEGIN_FUNC

	SelectDeviceData_t data = {0};
	VkResult result;

	uint32_t physicalDeviceCount;
	GET_RESULT(vkEnumeratePhysicalDevices(g_instance, &physicalDeviceCount, NULL))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEnumeratePhysicalDevices, 3, 'p', g_instance, 'p', &physicalDeviceCount, 'p', NULL)
		free_SelectDeviceData(data);
		return false;
	}

	if (!physicalDeviceCount) {
		clock_t time = PROGRAM_TIME;
		fprintf(stderr,
			"Vulkan failure at line %d (%ld ms)\n"
			"Function call 'vkEnumeratePhysicalDevices' returned *pPhysicalDeviceCount = %u\n\n",
			__LINE__, time, physicalDeviceCount
		);

		free_SelectDeviceData(data);
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
		physicalDeviceCount * 2 * sizeof(uint32_t);

	data.devices = malloc(size);
#ifndef NDEBUG
	if (!data.devices) {
		MALLOC_FAILURE(data.devices)
		free_SelectDeviceData(data);
		return false;
	}
#endif

	VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*) data.devices;

	VkQueueFamilyProperties2** queueFamiliesProperties2 = (VkQueueFamilyProperties2**) (physicalDevices          + physicalDeviceCount);
	VkExtensionProperties**    extensionsProperties     = (VkExtensionProperties**)    (queueFamiliesProperties2 + physicalDeviceCount);

	VkPhysicalDeviceMemoryProperties2* physicalDevicesMemoryProperties2 = (VkPhysicalDeviceMemoryProperties2*) (extensionsProperties             + physicalDeviceCount);
	VkPhysicalDeviceProperties2*       physicalDevicesProperties2       = (VkPhysicalDeviceProperties2*)       (physicalDevicesMemoryProperties2 + physicalDeviceCount);

	VkPhysicalDeviceFeatures2*            physicalDevicesFeatures2            = (VkPhysicalDeviceFeatures2*)            (physicalDevicesProperties2 + physicalDeviceCount);
	VkPhysicalDevice16BitStorageFeatures* physicalDevices16BitStorageFeatures = (VkPhysicalDevice16BitStorageFeatures*) (physicalDevicesFeatures2   + physicalDeviceCount);

	uint32_t* extensionPropertyCounts   = (uint32_t*) (physicalDevices16BitStorageFeatures + physicalDeviceCount);
	uint32_t* queueFamilyPropertyCounts = (uint32_t*) (extensionPropertyCounts             + physicalDeviceCount);

	GET_RESULT(vkEnumeratePhysicalDevices(g_instance, &physicalDeviceCount, physicalDevices))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEnumeratePhysicalDevices, 3, 'p', g_instance, 'p', &physicalDeviceCount, 'p', physicalDevices)
		free_SelectDeviceData(data);
		return false;
	}
#endif

	size_t queueFamilyTotal = 0;
	size_t extensionTotal   = 0;
	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCounts[i], NULL);
		GET_RESULT(vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &extensionPropertyCounts[i], NULL))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkEnumerateDeviceExtensionProperties, 4, 'p', physicalDevices[i], 'p', NULL, 'p', &extensionPropertyCounts[i], 'p', NULL)
			free_SelectDeviceData(data);
			return false;
		}
#endif

		queueFamilyTotal += queueFamilyPropertyCounts[i];
		extensionTotal   += extensionPropertyCounts[i];
	}

	size =
		queueFamilyTotal * sizeof(VkQueueFamilyProperties2) +
		extensionTotal   * sizeof(VkExtensionProperties);

	data.properties = malloc(size);
#ifndef NDEBUG
	if (!data.properties) {
		MALLOC_FAILURE(data.properties)
		free_SelectDeviceData(data);
		return false;
	}
#endif

	queueFamiliesProperties2[0] = (VkQueueFamilyProperties2*) data.properties;
	extensionsProperties[0]     = (VkExtensionProperties*) (queueFamiliesProperties2[0] + queueFamilyTotal);

	for (uint32_t i = 1; i < physicalDeviceCount; i++) {
		queueFamiliesProperties2[i] = queueFamiliesProperties2[i - 1] + queueFamilyPropertyCounts[i - 1];
		extensionsProperties[i]     = extensionsProperties[i - 1]     + extensionPropertyCounts[i - 1];
	}

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		GET_RESULT(vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &extensionPropertyCounts[i], extensionsProperties[i]))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkEnumerateDeviceExtensionProperties, 4, 'p', physicalDevices[i], 'p', NULL, 'p', &extensionPropertyCounts[i], 'p', extensionsProperties[i])
			free_SelectDeviceData(data);
			return false;
		}
#endif

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++) {
			queueFamiliesProperties2[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			queueFamiliesProperties2[i][j].pNext = NULL;
		}

		vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCounts[i], queueFamiliesProperties2[i]);

		physicalDevicesMemoryProperties2[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		physicalDevicesMemoryProperties2[i].pNext = NULL;

		vkGetPhysicalDeviceMemoryProperties2(physicalDevices[i], &physicalDevicesMemoryProperties2[i]);

		physicalDevicesProperties2[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDevicesProperties2[i].pNext = NULL;

		vkGetPhysicalDeviceProperties2(physicalDevices[i], &physicalDevicesProperties2[i]);

		physicalDevices16BitStorageFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
		physicalDevices16BitStorageFeatures[i].pNext = NULL;

		physicalDevicesFeatures2[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		physicalDevicesFeatures2[i].pNext = &physicalDevices16BitStorageFeatures[i];

		vkGetPhysicalDeviceFeatures2(physicalDevices[i], &physicalDevicesFeatures2[i]);
	}

	uint32_t pdvIndex     = ~0U; // Physical device index
	uint32_t highestScore = 0;

	bool usingShaderInt16         = false;
	bool usingShaderInt64         = false;
	bool usingMaintenance4        = false;
	bool usingMemoryBudget        = false;
	bool usingMemoryPriority      = false;
	bool usingSubgroupSizeControl = false;
	bool usingPortabilitySubset   = false;

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		bool hasApiVersion11 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasDiscreteGpu  = physicalDevicesProperties2[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		bool hasShaderInt16              = physicalDevicesFeatures2[i].features.shaderInt16;
		bool hasShaderInt64              = physicalDevicesFeatures2[i].features.shaderInt64;
		bool hasStorageBuffer16BitAccess = physicalDevices16BitStorageFeatures[i].storageBuffer16BitAccess;

		bool hasCompute           = false;
		bool hasDedicatedCompute  = false;
		bool hasDedicatedTransfer = false;

		bool hasDedicatedDeviceLocal  = false;
		bool hasHostCachedNonCoherent = false;
		bool hasHostCached            = false;
		bool hasHostNonCoherent       = false;

		bool hasMaintenance4        = false;
		bool hasSynchronization2    = false;
		bool hasTimelineSemaphore   = false;
		bool hasMemoryBudget        = false;
		bool hasMemoryPriority      = false;
		bool hasSubgroupSizeControl = false;
		bool hasPortabilitySubset   = false;

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++) {
			VkQueueFlags queueFlags = queueFamiliesProperties2[i][j].queueFamilyProperties.queueFlags;

			bool isCompute           = queueFlags &  VK_QUEUE_COMPUTE_BIT;
			bool isDedicatedCompute  = queueFlags == VK_QUEUE_COMPUTE_BIT;
			bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;

			if (isCompute)           hasCompute           = true;
			if (isDedicatedCompute)  hasDedicatedCompute  = true;
			if (isDedicatedTransfer) hasDedicatedTransfer = true;
		}

		for (uint32_t j = 0; j < physicalDevicesMemoryProperties2[i].memoryProperties.memoryTypeCount; j++) {
			VkMemoryPropertyFlags propertyFlags = physicalDevicesMemoryProperties2[i].memoryProperties.memoryTypes[j].propertyFlags;

			bool isDeviceLocal  = propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			bool isHostCached   = propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			bool isHostCoherent = propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			bool isHostVisible  = propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

			if (isDeviceLocal && !isHostVisible) hasDedicatedDeviceLocal  = true;
			if (isHostCached && !isHostCoherent) hasHostCachedNonCoherent = true;
			if (isHostCached)                    hasHostCached            = true;
			if (!isHostCoherent)                 hasHostNonCoherent       = true;
		}

		for (uint32_t j = 0; j < extensionPropertyCounts[i]; j++) {
			const char* extensionName = extensionsProperties[i][j].extensionName;

			if      (!strcmp(extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))         hasMaintenance4        = true;
			else if (!strcmp(extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))     hasSynchronization2    = true;
			else if (!strcmp(extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME))    hasTimelineSemaphore   = true;
			else if (!strcmp(extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))         hasMemoryBudget        = true;
			else if (!strcmp(extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))       hasMemoryPriority      = true;
			else if (!strcmp(extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) hasSubgroupSizeControl = true;
			else if (!strcmp(extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME))    hasPortabilitySubset   = true;
		}

		uint32_t currentScore = 1;

		if (!hasApiVersion11)             continue;
		if (!hasStorageBuffer16BitAccess) continue;
		if (!hasCompute)                  continue;

		if (!hasSynchronization2)  continue;
		if (!hasTimelineSemaphore) continue;

		if (hasDiscreteGpu) currentScore += 10000;

		if (hasShaderInt16) currentScore += 1000;
		if (hasShaderInt64) currentScore += 1000;

		if (hasDedicatedDeviceLocal)  currentScore += 1000;
		if (hasHostCachedNonCoherent) currentScore += 1000;
		else if (hasHostCached)       currentScore += 500;
		else if (hasHostNonCoherent)  currentScore += 100;

		if (hasDedicatedTransfer) currentScore += 100;
		if (hasDedicatedCompute)  currentScore += 100;

		if (hasMaintenance4)        currentScore += 10;
		if (hasMemoryBudget)        currentScore += 10;
		if (hasMemoryPriority)      currentScore += 10;
		if (hasSubgroupSizeControl) currentScore += 10;

		if (currentScore > highestScore) {
			highestScore = currentScore;
			pdvIndex     = i;

			usingShaderInt16         = hasShaderInt16;
			usingShaderInt64         = hasShaderInt64;
			usingMaintenance4        = hasMaintenance4;
			usingMemoryBudget        = hasMemoryBudget;
			usingMemoryPriority      = hasMemoryPriority;
			usingSubgroupSizeControl = hasSubgroupSizeControl;
			usingPortabilitySubset   = hasPortabilitySubset;
		}
	}

	if (pdvIndex == ~0U) {
		fprintf(stderr, "Vulkan failure\nNo physical device meets requirements of program\n\n");
		free_SelectDeviceData(data);
		return false;
	}

	uint32_t computeQueueFamilyIndex  = ~0U;
	uint32_t transferQueueFamilyIndex = ~0U;

	bool hasDedicatedCompute  = false;
	bool hasCompute           = false;
	bool hasDedicatedTransfer = false;
	bool hasTransfer          = false;

	for (uint32_t i = 0; i < queueFamilyPropertyCounts[pdvIndex]; i++) {
		VkQueueFlags queueFlags = queueFamiliesProperties2[pdvIndex][i].queueFamilyProperties.queueFlags;

		bool isDedicatedCompute  = queueFlags == VK_QUEUE_COMPUTE_BIT;
		bool isCompute           = queueFlags &  VK_QUEUE_COMPUTE_BIT;
		bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
		bool isTransfer          = queueFlags &  VK_QUEUE_TRANSFER_BIT;

		if (isDedicatedCompute && !hasDedicatedCompute) {
			hasDedicatedCompute = true;
			hasCompute          = true;
			computeQueueFamilyIndex = i;
		}

		else if (isCompute && !hasCompute) {
			hasCompute = true;
			computeQueueFamilyIndex = i;
		}

		if (isDedicatedTransfer && !hasDedicatedTransfer) {
			hasDedicatedTransfer = true;
			hasTransfer          = true;
			transferQueueFamilyIndex = i;
		}

		else if (isTransfer && !hasTransfer) {
			hasTransfer = true;
			transferQueueFamilyIndex = i;
		}
	}

	if (!hasTransfer) transferQueueFamilyIndex = computeQueueFamilyIndex;

	uint32_t hostVisibleMemoryHeapIndex = ~0U;
	uint32_t hostVisibleMemoryTypeIndex = ~0U;
	uint32_t deviceLocalMemoryHeapIndex = ~0U;
	uint32_t deviceLocalMemoryTypeIndex = ~0U;

	bool hasDedicatedDeviceLocal  = false;
	bool hasDeviceLocal           = false;
	bool hasHostCachedNonCoherent = false;
	bool hasHostCached            = false;
	bool hasHostNonCoherent       = false;
	bool hasHostVisible           = false;

	for (uint32_t i = 0; i < physicalDevicesMemoryProperties2[pdvIndex].memoryProperties.memoryTypeCount; i++) {
		VkMemoryPropertyFlags propertyFlags = physicalDevicesMemoryProperties2[pdvIndex].memoryProperties.memoryTypes[i].propertyFlags;
		uint32_t              heapIndex     = physicalDevicesMemoryProperties2[pdvIndex].memoryProperties.memoryTypes[i].heapIndex;

		bool isDeviceLocal  = propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bool isHostCached   = propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		bool isHostCoherent = propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bool isHostVisible  = propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

		if (isDeviceLocal && !isHostVisible && !hasDedicatedDeviceLocal) {
			hasDedicatedDeviceLocal = true;
			hasDeviceLocal          = true;
			deviceLocalMemoryTypeIndex = i;
			deviceLocalMemoryHeapIndex = heapIndex;
		}

		else if (isDeviceLocal && !hasDeviceLocal) {
			hasDeviceLocal = true;
			deviceLocalMemoryTypeIndex = i;
			deviceLocalMemoryHeapIndex = heapIndex;
		}

		if (isHostCached && !isHostCoherent && !hasHostCachedNonCoherent) {
			hasHostCachedNonCoherent = true;
			hasHostCached            = true;
			hasHostNonCoherent       = true;
			hasHostVisible           = true;
			hostVisibleMemoryTypeIndex = i;
			hostVisibleMemoryHeapIndex = heapIndex;
		}

		else if (isHostCached && !hasHostCached) {
			hasHostCached      = true;
			hasHostNonCoherent = false;
			hasHostVisible     = true;
			hostVisibleMemoryTypeIndex = i;
			hostVisibleMemoryHeapIndex = heapIndex;
		}

		else if (!isHostCoherent && !hasHostCached && !hasHostNonCoherent) {
			hasHostCached      = false;
			hasHostNonCoherent = true;
			hasHostVisible     = true;
			hostVisibleMemoryTypeIndex = i;
			hostVisibleMemoryHeapIndex = heapIndex;
		}

		else if (isHostVisible && !hasHostVisible) {
			hasHostVisible = true;
			hostVisibleMemoryTypeIndex = i;
			hostVisibleMemoryHeapIndex = heapIndex;
		}
	}

	g_physicalDevice = physicalDevices[pdvIndex];

	gpu->computeQueueFamilyIndex  = computeQueueFamilyIndex;
	gpu->transferQueueFamilyIndex = transferQueueFamilyIndex;

	gpu->hostVisibleMemoryHeapIndex = hostVisibleMemoryHeapIndex;
	gpu->hostVisibleMemoryTypeIndex = hostVisibleMemoryTypeIndex;
	gpu->deviceLocalMemoryHeapIndex = deviceLocalMemoryHeapIndex;
	gpu->deviceLocalMemoryTypeIndex = deviceLocalMemoryTypeIndex;

	gpu->usingShaderInt16         = usingShaderInt16;
	gpu->usingShaderInt64         = usingShaderInt64;
	gpu->usingMaintenance4        = usingMaintenance4;
	gpu->usingMemoryBudget        = usingMemoryBudget;
	gpu->usingMemoryPriority      = usingMemoryPriority;
	gpu->usingSubgroupSizeControl = usingSubgroupSizeControl;
	gpu->usingPortabilitySubset   = usingPortabilitySubset;
	gpu->usingNonCoherent         = hasHostNonCoherent;

#if QUERY_BENCHMARKING
	gpu->transferQueueTimestampValidBits = queueFamiliesProperties2[pdvIndex][transferQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
	gpu->computeQueueTimestampValidBits  = queueFamiliesProperties2[pdvIndex][computeQueueFamilyIndex ].queueFamilyProperties.timestampValidBits;
	gpu->timestampPeriod                 = physicalDevicesProperties2[pdvIndex].properties.limits.timestampPeriod;
#endif

	printf(
		"Selected device: %s\n"
		"\tScore:               %u\n"
		"\tTransfer QF index:   %u\n"
		"\tCompute QF index:    %u\n"
		"\tDL type index:       %u\n"
		"\tHV type index:       %u\n"
		"\tDL heap index:       %u\n"
		"\tHV heap index:       %u\n"
		"\tshaderInt16:         %d\n"
		"\tshaderInt64:         %d\n"
		"\tmaintenance4         %d\n"
		"\tmemoryPriority:      %d\n"
		"\tsubgroupSizeControl: %d\n\n",
		physicalDevicesProperties2[pdvIndex].properties.deviceName,
		highestScore,
		transferQueueFamilyIndex,
		computeQueueFamilyIndex,
		deviceLocalMemoryTypeIndex,
		hostVisibleMemoryTypeIndex,
		deviceLocalMemoryHeapIndex,
		hostVisibleMemoryHeapIndex,
		usingShaderInt16,
		usingShaderInt64,
		usingMaintenance4,
		usingMemoryPriority,
		usingSubgroupSizeControl
	);

	free_SelectDeviceData(data);

	END_FUNC
	return true;
}

bool create_device(Gpu_t* gpu)
{
	BEGIN_FUNC

	uint32_t computeQueueFamilyIndex  = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;

	bool usingShaderInt16         = gpu->usingShaderInt16;
	bool usingShaderInt64         = gpu->usingShaderInt64;
	bool usingMaintenance4        = gpu->usingMaintenance4;
	bool usingMemoryBudget        = gpu->usingMemoryBudget;
	bool usingMemoryPriority      = gpu->usingMemoryPriority;
	bool usingSubgroupSizeControl = gpu->usingSubgroupSizeControl;
	bool usingPortabilitySubset   = gpu->usingPortabilitySubset;

	VkResult result;

	uint32_t enabledExtensionCount = 0;
	const char* enabledExtensions[7];

	enabledExtensions[enabledExtensionCount] = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
	enabledExtensionCount++;

	enabledExtensions[enabledExtensionCount] = VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME;
	enabledExtensionCount++;

	if (usingMaintenance4) {
		enabledExtensions[enabledExtensionCount] = VK_KHR_MAINTENANCE_4_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	if (usingMemoryBudget) {
		enabledExtensions[enabledExtensionCount] = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	if (usingMemoryPriority) {
		enabledExtensions[enabledExtensionCount] = VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	if (usingSubgroupSizeControl) {
		enabledExtensions[enabledExtensionCount] = VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	if (usingPortabilitySubset) {
		enabledExtensions[enabledExtensionCount] = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2;
	physicalDeviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	physicalDeviceFeatures2.pNext = NULL;
	physicalDeviceFeatures2.features.robustBufferAccess                      = VK_FALSE;
	physicalDeviceFeatures2.features.fullDrawIndexUint32                     = VK_FALSE;
	physicalDeviceFeatures2.features.imageCubeArray                          = VK_FALSE;
	physicalDeviceFeatures2.features.independentBlend                        = VK_FALSE;
	physicalDeviceFeatures2.features.geometryShader                          = VK_FALSE;
	physicalDeviceFeatures2.features.tessellationShader                      = VK_FALSE;
	physicalDeviceFeatures2.features.sampleRateShading                       = VK_FALSE;
	physicalDeviceFeatures2.features.dualSrcBlend                            = VK_FALSE;
	physicalDeviceFeatures2.features.logicOp                                 = VK_FALSE;
	physicalDeviceFeatures2.features.multiDrawIndirect                       = VK_FALSE;
	physicalDeviceFeatures2.features.drawIndirectFirstInstance               = VK_FALSE;
	physicalDeviceFeatures2.features.depthClamp                              = VK_FALSE;
	physicalDeviceFeatures2.features.depthBiasClamp                          = VK_FALSE;
	physicalDeviceFeatures2.features.fillModeNonSolid                        = VK_FALSE;
	physicalDeviceFeatures2.features.depthBounds                             = VK_FALSE;
	physicalDeviceFeatures2.features.wideLines                               = VK_FALSE;
	physicalDeviceFeatures2.features.largePoints                             = VK_FALSE;
	physicalDeviceFeatures2.features.alphaToOne                              = VK_FALSE;
	physicalDeviceFeatures2.features.multiViewport                           = VK_FALSE;
	physicalDeviceFeatures2.features.samplerAnisotropy                       = VK_FALSE;
	physicalDeviceFeatures2.features.textureCompressionETC2                  = VK_FALSE;
	physicalDeviceFeatures2.features.textureCompressionASTC_LDR              = VK_FALSE;
	physicalDeviceFeatures2.features.textureCompressionBC                    = VK_FALSE;
	physicalDeviceFeatures2.features.occlusionQueryPrecise                   = VK_FALSE;
	physicalDeviceFeatures2.features.pipelineStatisticsQuery                 = VK_FALSE;
	physicalDeviceFeatures2.features.vertexPipelineStoresAndAtomics          = VK_FALSE;
	physicalDeviceFeatures2.features.fragmentStoresAndAtomics                = VK_FALSE;
	physicalDeviceFeatures2.features.shaderTessellationAndGeometryPointSize  = VK_FALSE;
	physicalDeviceFeatures2.features.shaderImageGatherExtended               = VK_FALSE;
	physicalDeviceFeatures2.features.shaderStorageImageExtendedFormats       = VK_FALSE;
	physicalDeviceFeatures2.features.shaderStorageImageMultisample           = VK_FALSE;
	physicalDeviceFeatures2.features.shaderStorageImageReadWithoutFormat     = VK_FALSE;
	physicalDeviceFeatures2.features.shaderStorageImageWriteWithoutFormat    = VK_FALSE;
	physicalDeviceFeatures2.features.shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
	physicalDeviceFeatures2.features.shaderSampledImageArrayDynamicIndexing  = VK_FALSE;
	physicalDeviceFeatures2.features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
	physicalDeviceFeatures2.features.shaderStorageImageArrayDynamicIndexing  = VK_FALSE;
	physicalDeviceFeatures2.features.shaderClipDistance                      = VK_FALSE;
	physicalDeviceFeatures2.features.shaderCullDistance                      = VK_FALSE;
	physicalDeviceFeatures2.features.shaderFloat64                           = VK_FALSE;
	physicalDeviceFeatures2.features.shaderInt64                             = usingShaderInt64;
	physicalDeviceFeatures2.features.shaderInt16                             = usingShaderInt16;
	physicalDeviceFeatures2.features.shaderResourceResidency                 = VK_FALSE;
	physicalDeviceFeatures2.features.shaderResourceMinLod                    = VK_FALSE;
	physicalDeviceFeatures2.features.sparseBinding                           = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidencyBuffer                   = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidencyImage2D                  = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidencyImage3D                  = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidency2Samples                 = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidency4Samples                 = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidency8Samples                 = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidency16Samples                = VK_FALSE;
	physicalDeviceFeatures2.features.sparseResidencyAliased                  = VK_FALSE;
	physicalDeviceFeatures2.features.variableMultisampleRate                 = VK_FALSE;
	physicalDeviceFeatures2.features.inheritedQueries                        = VK_FALSE;

	VkPhysicalDevice16BitStorageFeatures physicalDevice16BitStorageFeatures;
	physicalDevice16BitStorageFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
	physicalDevice16BitStorageFeatures.pNext = NULL;
	physicalDevice16BitStorageFeatures.storageBuffer16BitAccess           = VK_TRUE;
	physicalDevice16BitStorageFeatures.uniformAndStorageBuffer16BitAccess = VK_FALSE;
	physicalDevice16BitStorageFeatures.storagePushConstant16              = VK_FALSE;
	physicalDevice16BitStorageFeatures.storageInputOutput16               = VK_FALSE;

	VkPhysicalDeviceSynchronization2FeaturesKHR physicalDeviceSynchronization2Features;
	physicalDeviceSynchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
	physicalDeviceSynchronization2Features.pNext = NULL;
	physicalDeviceSynchronization2Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR physicalDeviceTimelineSemaphoreFeatures;
	physicalDeviceTimelineSemaphoreFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR;
	physicalDeviceTimelineSemaphoreFeatures.pNext = NULL;
	physicalDeviceTimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

	VkPhysicalDeviceMaintenance4FeaturesKHR physicalDeviceMaintenance4Features;
	physicalDeviceMaintenance4Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR;
	physicalDeviceMaintenance4Features.pNext = NULL;
	physicalDeviceMaintenance4Features.maintenance4 = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT physicalDeviceMemoryPriorityFeatures;
	physicalDeviceMemoryPriorityFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
	physicalDeviceMemoryPriorityFeatures.pNext = NULL;
	physicalDeviceMemoryPriorityFeatures.memoryPriority = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeaturesEXT physicalDeviceSubgroupSizeControlFeatures;
	physicalDeviceSubgroupSizeControlFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT;
	physicalDeviceSubgroupSizeControlFeatures.pNext = NULL;
	physicalDeviceSubgroupSizeControlFeatures.subgroupSizeControl  = VK_TRUE;
	physicalDeviceSubgroupSizeControlFeatures.computeFullSubgroups = VK_FALSE;

	void** next = &physicalDeviceFeatures2.pNext;

	*next = &physicalDevice16BitStorageFeatures;
	next  = &physicalDevice16BitStorageFeatures.pNext;

	*next = &physicalDeviceSynchronization2Features;
	next  = &physicalDeviceSynchronization2Features.pNext;

	*next = &physicalDeviceTimelineSemaphoreFeatures;
	next  = &physicalDeviceTimelineSemaphoreFeatures.pNext;

	if (usingMaintenance4) {
		*next = &physicalDeviceMaintenance4Features;
		next  = &physicalDeviceMaintenance4Features.pNext;
	}

	if (usingMemoryPriority) {
		*next = &physicalDeviceMemoryPriorityFeatures;
		next  = &physicalDeviceMemoryPriorityFeatures.pNext;
	}

	if (usingSubgroupSizeControl) {
		*next = &physicalDeviceSubgroupSizeControlFeatures;
		next  = &physicalDeviceSubgroupSizeControlFeatures.pNext;
	}

	// CPU spends more time waiting for compute operations than transfer operations
	// So compute queue has higher priority to potentially reduce this wait time
	float computeQueuePriority  = 1.0f;
	float transferQueuePriority = 0.0f;

	VkDeviceQueueCreateInfo queueCreateInfos[2];
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = NULL;
	queueCreateInfos[0].flags            = 0;
	queueCreateInfos[0].queueFamilyIndex = computeQueueFamilyIndex;
	queueCreateInfos[0].queueCount       = 1;
	queueCreateInfos[0].pQueuePriorities = &computeQueuePriority;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].pNext = NULL;
	queueCreateInfos[1].flags            = 0;
	queueCreateInfos[1].queueFamilyIndex = transferQueueFamilyIndex;
	queueCreateInfos[1].queueCount       = 1;
	queueCreateInfos[1].pQueuePriorities = &transferQueuePriority;

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &physicalDeviceFeatures2;
	deviceCreateInfo.flags                   = 0;
	deviceCreateInfo.queueCreateInfoCount    = computeQueueFamilyIndex == transferQueueFamilyIndex ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount       = 0;
	deviceCreateInfo.ppEnabledLayerNames     = NULL;
	deviceCreateInfo.enabledExtensionCount   = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
	deviceCreateInfo.pEnabledFeatures        = NULL;

	printf("Enabled device extensions (%u):\n", enabledExtensionCount);
	for (uint32_t i = 0; i < enabledExtensionCount; i++)
		printf("\t%u) %s\n", i + 1, enabledExtensions[i]);
	NEWLINE

	GET_RESULT(vkCreateDevice(g_physicalDevice, &deviceCreateInfo, g_allocator, &g_device))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateDevice, 4, 'p', g_physicalDevice, 'p', &deviceCreateInfo, 'p', g_allocator, 'p', &g_device)
		return false;
	}
#endif

	volkLoadDevice(g_device);

	VkDeviceQueueInfo2 transferDeviceQueueInfo2;
	transferDeviceQueueInfo2.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	transferDeviceQueueInfo2.pNext = NULL;
	transferDeviceQueueInfo2.flags            = 0;
	transferDeviceQueueInfo2.queueFamilyIndex = transferQueueFamilyIndex;
	transferDeviceQueueInfo2.queueIndex       = 0;

	VkDeviceQueueInfo2 computeDeviceQueueInfo2;
	computeDeviceQueueInfo2.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	computeDeviceQueueInfo2.pNext = NULL;
	computeDeviceQueueInfo2.flags            = 0;
	computeDeviceQueueInfo2.queueFamilyIndex = computeQueueFamilyIndex;
	computeDeviceQueueInfo2.queueIndex       = 0;

	vkGetDeviceQueue2(g_device, &transferDeviceQueueInfo2, &g_transferQueue);
	vkGetDeviceQueue2(g_device, &computeDeviceQueueInfo2,  &g_computeQueue);

#ifndef NDEBUG
	if(g_debugMessenger) {
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;
		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_QUEUE;

		if (g_transferQueue == g_computeQueue) {
			debugUtilsObjectNameInfo.objectHandle = (uint64_t) g_transferQueue;
			debugUtilsObjectNameInfo.pObjectName  = "Transfer & compute queue";
			SET_DEBUG_NAME()
		}
		else {
			debugUtilsObjectNameInfo.objectHandle = (uint64_t) g_transferQueue;
			debugUtilsObjectNameInfo.pObjectName  = "Transfer queue";
			SET_DEBUG_NAME()

			debugUtilsObjectNameInfo.objectHandle = (uint64_t) g_computeQueue;
			debugUtilsObjectNameInfo.pObjectName  = "Compute queue";
			SET_DEBUG_NAME()
		}
	}
#endif

	END_FUNC
	return true;
}

static VkMemoryRequirements get_buffer_requirements_noext(VkDeviceSize size, VkBufferUsageFlags usage)
{
	VkResult result;

	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags                 = 0;
	bufferCreateInfo.size                  = size;
	bufferCreateInfo.usage                 = usage;
	bufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices   = NULL;

	VkBuffer buffer;
	GET_RESULT(vkCreateBuffer(g_device, &bufferCreateInfo, g_allocator, &buffer))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkCreateBuffer, 4, 'p', g_device, 'p', &bufferCreateInfo, 'p', g_allocator, 'p', &buffer)
			return (VkMemoryRequirements) {0};
		}
#endif

	VkBufferMemoryRequirementsInfo2 bufferMemoryRequirementsInfo2;
	bufferMemoryRequirementsInfo2.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_REQUIREMENTS_INFO_2;
	bufferMemoryRequirementsInfo2.pNext = NULL;
	bufferMemoryRequirementsInfo2.buffer = buffer;

	VkMemoryRequirements2 memoryRequirements2;
	memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	memoryRequirements2.pNext = NULL;

	vkGetBufferMemoryRequirements2(g_device, &bufferMemoryRequirementsInfo2, &memoryRequirements2);

	vkDestroyBuffer(g_device, buffer, g_allocator);

	return memoryRequirements2.memoryRequirements;
}

static VkMemoryRequirements get_buffer_requirements_main4(VkDeviceSize size, VkBufferUsageFlags usage)
{
	VkBufferCreateInfo bufferCreateInfo;
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.pNext = NULL;
	bufferCreateInfo.flags                 = 0;
	bufferCreateInfo.size                  = size;
	bufferCreateInfo.usage                 = usage;
	bufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	bufferCreateInfo.queueFamilyIndexCount = 0;
	bufferCreateInfo.pQueueFamilyIndices   = NULL;

	VkDeviceBufferMemoryRequirementsKHR deviceBufferMemoryRequirements;
	deviceBufferMemoryRequirements.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS_KHR;
	deviceBufferMemoryRequirements.pNext = NULL;
	deviceBufferMemoryRequirements.pCreateInfo = &bufferCreateInfo;

	VkMemoryRequirements2 memoryRequirements2;
	memoryRequirements2.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	memoryRequirements2.pNext = NULL;

	vkGetDeviceBufferMemoryRequirementsKHR(g_device, &deviceBufferMemoryRequirements, &memoryRequirements2);

	return memoryRequirements2.memoryRequirements;
}

bool manage_memory(Gpu_t* gpu)
{
	BEGIN_FUNC

	uint32_t hostVisibleMemoryHeapIndex = gpu->hostVisibleMemoryHeapIndex;
	uint32_t deviceLocalMemoryHeapIndex = gpu->deviceLocalMemoryHeapIndex;

	bool usingMaintenance4 = gpu->usingMaintenance4;
	bool usingMemoryBudget = gpu->usingMemoryBudget;
	bool usingNonCoherent  = gpu->usingNonCoherent;

	VkPhysicalDeviceMaintenance4PropertiesKHR physicalDeviceMaintenance4Properties;
	physicalDeviceMaintenance4Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR;
	physicalDeviceMaintenance4Properties.pNext = NULL;

	VkPhysicalDeviceMaintenance3Properties physicalDeviceMaintenance3Properties;
	physicalDeviceMaintenance3Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
	physicalDeviceMaintenance3Properties.pNext = usingMaintenance4 ? &physicalDeviceMaintenance4Properties : NULL;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2;
	physicalDeviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physicalDeviceProperties2.pNext = &physicalDeviceMaintenance3Properties;

	vkGetPhysicalDeviceProperties2(g_physicalDevice, &physicalDeviceProperties2);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT physicalDeviceMemoryBudgetProperties;
	physicalDeviceMemoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
	physicalDeviceMemoryBudgetProperties.pNext = NULL;

	VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties2;
	physicalDeviceMemoryProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	physicalDeviceMemoryProperties2.pNext = usingMemoryBudget ? &physicalDeviceMemoryBudgetProperties : NULL;

	vkGetPhysicalDeviceMemoryProperties2(g_physicalDevice, &physicalDeviceMemoryProperties2);

	VkDeviceSize minStorageBufferOffsetAlignment = physicalDeviceProperties2.properties.limits.minStorageBufferOffsetAlignment;
	VkDeviceSize nonCoherentAtomSize             = physicalDeviceProperties2.properties.limits.nonCoherentAtomSize;

	VkDeviceSize maxMemoryAllocationSize = physicalDeviceMaintenance3Properties.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize           = usingMaintenance4 ? physicalDeviceMaintenance4Properties.maxBufferSize : maxMemoryAllocationSize;

	uint32_t maxStorageBufferRange    = physicalDeviceProperties2.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryAllocationCount = physicalDeviceProperties2.properties.limits.maxMemoryAllocationCount;
	uint32_t maxComputeWorkGroupCount = physicalDeviceProperties2.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxComputeWorkGroupSize  = physicalDeviceProperties2.properties.limits.maxComputeWorkGroupSize[0];

	VkDeviceSize bytesPerHostVisibleHeap;
	VkDeviceSize bytesPerDeviceLocalHeap;

	if (usingMemoryBudget) {
		bytesPerHostVisibleHeap = physicalDeviceMemoryBudgetProperties.heapBudget[hostVisibleMemoryHeapIndex];
		bytesPerDeviceLocalHeap = physicalDeviceMemoryBudgetProperties.heapBudget[deviceLocalMemoryHeapIndex];
	}
	else {
		bytesPerHostVisibleHeap = physicalDeviceMemoryProperties2.memoryProperties.memoryHeaps[hostVisibleMemoryHeapIndex].size;
		bytesPerDeviceLocalHeap = physicalDeviceMemoryProperties2.memoryProperties.memoryHeaps[deviceLocalMemoryHeapIndex].size;
	}

	VkDeviceSize bytesPerHeap  = bytesPerHostVisibleHeap < bytesPerDeviceLocalHeap ? bytesPerHostVisibleHeap : bytesPerDeviceLocalHeap;
	bytesPerHeap              /= deviceLocalMemoryHeapIndex == hostVisibleMemoryHeapIndex ? 2 : 1;
	bytesPerHeap              *= MAX_HEAP_MEMORY;

	VkDeviceSize bytesPerDeviceMemory  = maxMemoryAllocationSize < bytesPerHeap ? maxMemoryAllocationSize : bytesPerHeap;
	uint32_t     deviceMemoriesPerHeap = bytesPerHeap / bytesPerDeviceMemory;

	if (deviceMemoriesPerHeap < maxMemoryAllocationCount && bytesPerHeap % bytesPerDeviceMemory) {
		VkDeviceSize excessBytes = bytesPerDeviceMemory - bytesPerHeap % bytesPerDeviceMemory;
		deviceMemoriesPerHeap++;
		bytesPerDeviceMemory -= excessBytes / deviceMemoriesPerHeap;
		bytesPerDeviceMemory -= excessBytes % deviceMemoriesPerHeap ? 1 : 0;
	}
	else if (deviceMemoriesPerHeap > maxMemoryAllocationCount)
		deviceMemoriesPerHeap = maxMemoryAllocationCount;

	VkDeviceSize bytesPerBuffer         = maxBufferSize < bytesPerDeviceMemory ? maxBufferSize : bytesPerDeviceMemory;
	uint32_t     buffersPerDeviceMemory = bytesPerDeviceMemory / bytesPerBuffer;

	if (bytesPerDeviceMemory % bytesPerBuffer) {
		VkDeviceSize excessBytes = bytesPerBuffer - bytesPerDeviceMemory % bytesPerBuffer;
		buffersPerDeviceMemory++;
		bytesPerBuffer -= excessBytes / buffersPerDeviceMemory;
		bytesPerBuffer -= excessBytes % buffersPerDeviceMemory ? 1 : 0;
	}

	uint32_t valuesPerInoutBuffer  = maxStorageBufferRange / sizeof(value_t);
	uint32_t computeWorkGroupSize  = maxComputeWorkGroupSize;
	uint32_t computeWorkGroupCount = valuesPerInoutBuffer / computeWorkGroupSize;
	computeWorkGroupCount = maxComputeWorkGroupCount < computeWorkGroupCount ? maxComputeWorkGroupCount : computeWorkGroupCount;

	valuesPerInoutBuffer               = computeWorkGroupSize * computeWorkGroupCount;
	VkDeviceSize bytesPerInoutBuffer   = valuesPerInoutBuffer * (sizeof(value_t) + sizeof(step_t));
	uint32_t     inoutBuffersPerBuffer = bytesPerBuffer / bytesPerInoutBuffer;

	if (bytesPerBuffer % bytesPerInoutBuffer > computeWorkGroupSize * (sizeof(value_t) + sizeof(step_t))) {
		uint32_t excessValues = valuesPerInoutBuffer - bytesPerBuffer % bytesPerInoutBuffer / (sizeof(value_t) + sizeof(step_t));
		inoutBuffersPerBuffer++;
		valuesPerInoutBuffer -= excessValues / inoutBuffersPerBuffer;
		valuesPerInoutBuffer -= excessValues % inoutBuffersPerBuffer ? 1 : 0;
		valuesPerInoutBuffer -= valuesPerInoutBuffer % computeWorkGroupSize;
		computeWorkGroupCount = valuesPerInoutBuffer / computeWorkGroupSize;
	}

	VkDeviceSize inoutBufferAlignment = usingNonCoherent && nonCoherentAtomSize > minStorageBufferOffsetAlignment ? nonCoherentAtomSize : minStorageBufferOffsetAlignment;

	valuesPerInoutBuffer -= valuesPerInoutBuffer % (inoutBufferAlignment / sizeof(step_t));

	VkDeviceSize bytesPerInBuffer  = valuesPerInoutBuffer * sizeof(value_t);
	VkDeviceSize bytesPerOutBuffer = valuesPerInoutBuffer * sizeof(step_t);

	bytesPerInoutBuffer = bytesPerInBuffer + bytesPerOutBuffer;
	bytesPerBuffer      = bytesPerInoutBuffer * inoutBuffersPerBuffer;

	VkMemoryRequirements hostVisibleMemoryRequirements;
	VkMemoryRequirements deviceLocalMemoryRequirements;

	if (usingMaintenance4) {
		hostVisibleMemoryRequirements = get_buffer_requirements_main4(bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
		deviceLocalMemoryRequirements = get_buffer_requirements_main4(bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	}
	else {
		hostVisibleMemoryRequirements = get_buffer_requirements_noext(bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
#ifndef NDEBUG
		if (hostVisibleMemoryRequirements.size == 0) return false;
#endif

		deviceLocalMemoryRequirements = get_buffer_requirements_noext(bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
#ifndef NDEBUG
		if (deviceLocalMemoryRequirements.size == 0) return false;
#endif
	}

	VkDeviceSize bytesPerHostVisibleBuffer = hostVisibleMemoryRequirements.size;
	VkDeviceSize bytesPerDeviceLocalBuffer = deviceLocalMemoryRequirements.size;

	VkDeviceSize hostVisibleBufferAlignment = hostVisibleMemoryRequirements.alignment;
	VkDeviceSize deviceLocalBufferAlignment = deviceLocalMemoryRequirements.alignment;

	VkDeviceSize hostVisibleBufferPadding = (hostVisibleBufferAlignment - (bytesPerHostVisibleBuffer & (hostVisibleBufferAlignment - 1))) & (hostVisibleBufferAlignment - 1);
	VkDeviceSize deviceLocalBufferPadding = (deviceLocalBufferAlignment - (bytesPerDeviceLocalBuffer & (deviceLocalBufferAlignment - 1))) & (deviceLocalBufferAlignment - 1);

	bytesPerHostVisibleBuffer += hostVisibleBufferPadding;
	bytesPerDeviceLocalBuffer += deviceLocalBufferPadding;

	VkDeviceSize bytesPerHostVisibleDeviceMemory = bytesPerHostVisibleBuffer * buffersPerDeviceMemory - hostVisibleBufferPadding;
	VkDeviceSize bytesPerDeviceLocalDeviceMemory = bytesPerDeviceLocalBuffer * buffersPerDeviceMemory - deviceLocalBufferPadding;

	bytesPerHostVisibleHeap = bytesPerHostVisibleDeviceMemory * deviceMemoriesPerHeap;
	bytesPerDeviceLocalHeap = bytesPerDeviceLocalDeviceMemory * deviceMemoriesPerHeap;

	uint32_t valuesPerBuffer             = valuesPerInoutBuffer        * inoutBuffersPerBuffer;
	uint32_t valuesPerDeviceMemory       = valuesPerBuffer             * buffersPerDeviceMemory;
	uint32_t valuesPerHeap               = valuesPerDeviceMemory       * deviceMemoriesPerHeap;
	uint32_t inoutBuffersPerDeviceMemory = inoutBuffersPerBuffer       * buffersPerDeviceMemory;
	uint32_t inoutBuffersPerHeap         = inoutBuffersPerDeviceMemory * deviceMemoriesPerHeap;
	uint32_t buffersPerHeap              = buffersPerDeviceMemory      * deviceMemoriesPerHeap;

	gpu->bytesPerInBuffer                = bytesPerInBuffer;
	gpu->bytesPerOutBuffer               = bytesPerOutBuffer;
	gpu->bytesPerInoutBuffer             = bytesPerInoutBuffer;
	gpu->bytesPerBuffer                  = bytesPerBuffer;
	gpu->bytesPerHostVisibleBuffer       = bytesPerHostVisibleBuffer;
	gpu->bytesPerDeviceLocalBuffer       = bytesPerDeviceLocalBuffer;
	gpu->bytesPerHostVisibleDeviceMemory = bytesPerHostVisibleDeviceMemory;
	gpu->bytesPerDeviceLocalDeviceMemory = bytesPerDeviceLocalDeviceMemory;

	gpu->valuesPerInoutBuffer        = valuesPerInoutBuffer;
	gpu->valuesPerBuffer             = valuesPerBuffer;
	gpu->valuesPerDeviceMemory       = valuesPerDeviceMemory;
	gpu->valuesPerHeap               = valuesPerHeap;
	gpu->inoutBuffersPerBuffer       = inoutBuffersPerBuffer;
	gpu->inoutBuffersPerDeviceMemory = inoutBuffersPerDeviceMemory;
	gpu->inoutBuffersPerHeap         = inoutBuffersPerHeap;
	gpu->buffersPerDeviceMemory      = buffersPerDeviceMemory;
	gpu->buffersPerHeap              = buffersPerHeap;
	gpu->deviceMemoriesPerHeap       = deviceMemoriesPerHeap;

	gpu->computeWorkGroupCount = computeWorkGroupCount;
	gpu->computeWorkGroupSize  = computeWorkGroupSize;

	printf(
		"Memory information:\n"
		"\tHV-buffer padding:         0x%llx\n"
		"\tDL-buffer padding:         0x%llx\n"
		"\tCompute workgroup size:    %u\n"
		"\tCompute workgroup count:   %u\n"
		"\tValues per inout-buffer:   %u\n"
		"\tInout-buffers per buffer:  %u\n"
		"\tBuffers per device memory: %u\n"
		"\tDevice memories per heap:  %u\n"
		"\tValues per heap:           %u\n\n",
		hostVisibleBufferPadding,
		deviceLocalBufferPadding,
		computeWorkGroupSize,
		computeWorkGroupCount,
		valuesPerInoutBuffer,
		inoutBuffersPerBuffer,
		buffersPerDeviceMemory,
		deviceMemoriesPerHeap,
		valuesPerHeap
	);

	size_t size =
		inoutBuffersPerHeap   * sizeof(value_t*) +        // Mapped host visible in-buffers
		inoutBuffersPerHeap   * sizeof(step_t*) +         // Mapped host visible out-buffers
		buffersPerHeap        * sizeof(VkBuffer) +        // Host visible buffers
		buffersPerHeap        * sizeof(VkBuffer) +        // Device local buffers
		deviceMemoriesPerHeap * sizeof(VkDeviceMemory) +  // Host visible device memories
		deviceMemoriesPerHeap * sizeof(VkDeviceMemory) +  // Device local device memories
		inoutBuffersPerHeap   * sizeof(VkDescriptorSet) + // Descriptor sets
		inoutBuffersPerHeap   * sizeof(VkCommandBuffer) + // Transfer command buffers
		inoutBuffersPerHeap   * sizeof(VkCommandBuffer) + // Compute command buffers
		inoutBuffersPerHeap   * sizeof(VkSemaphore);      // Semaphores

	gpu->dynamicMemory = calloc(1, size);
#ifndef NDEBUG
	if (!gpu->dynamicMemory) {
		CALLOC_FAILURE(gpu->dynamicMemory, 1, size)
		return false;
	}
#endif

	gpu->mappedHostVisibleInBuffers  = (value_t**) gpu->dynamicMemory;
	gpu->mappedHostVisibleOutBuffers = (step_t**) (gpu->mappedHostVisibleInBuffers + inoutBuffersPerHeap);

	gpu->hostVisibleBuffers = (VkBuffer*) (gpu->mappedHostVisibleOutBuffers + inoutBuffersPerHeap);
	gpu->deviceLocalBuffers = (VkBuffer*) (gpu->hostVisibleBuffers          + buffersPerHeap);

	gpu->hostVisibleDeviceMemories = (VkDeviceMemory*) (gpu->deviceLocalBuffers        + buffersPerHeap);
	gpu->deviceLocalDeviceMemories = (VkDeviceMemory*) (gpu->hostVisibleDeviceMemories + deviceMemoriesPerHeap);

	gpu->descriptorSets = (VkDescriptorSet*) (gpu->deviceLocalDeviceMemories + deviceMemoriesPerHeap);

	gpu->transferCommandBuffers = (VkCommandBuffer*) (gpu->descriptorSets         + inoutBuffersPerHeap);
	gpu->computeCommandBuffers  = (VkCommandBuffer*) (gpu->transferCommandBuffers + inoutBuffersPerHeap);

	gpu->semaphores = (VkSemaphore*) (gpu->computeCommandBuffers + inoutBuffersPerHeap);

	END_FUNC
	return true;
}

typedef struct CreateBuffersData
{
	void* memory;
} CreateBuffersData_t;

static void free_CreateBuffersData(CreateBuffersData_t data)
{
	free(data.memory);
}

bool create_buffers(Gpu_t* gpu)
{
	BEGIN_FUNC

	VkDeviceMemory* restrict hostVisibleDeviceMemories   = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* restrict deviceLocalDeviceMemories   = gpu->deviceLocalDeviceMemories;
	VkBuffer*       restrict hostVisibleBuffers          = gpu->hostVisibleBuffers;
	VkBuffer*       restrict deviceLocalBuffers          = gpu->deviceLocalBuffers;
	value_t**       restrict mappedHostVisibleInBuffers  = gpu->mappedHostVisibleInBuffers;
	step_t**        restrict mappedHostVisibleOutBuffers = gpu->mappedHostVisibleOutBuffers;

	VkDeviceSize bytesPerInoutBuffer             = gpu->bytesPerInoutBuffer;
	VkDeviceSize bytesPerBuffer                  = gpu->bytesPerBuffer;
	VkDeviceSize bytesPerHostVisibleBuffer       = gpu->bytesPerHostVisibleBuffer;
	VkDeviceSize bytesPerDeviceLocalBuffer       = gpu->bytesPerDeviceLocalBuffer;
	VkDeviceSize bytesPerHostVisibleDeviceMemory = gpu->bytesPerHostVisibleDeviceMemory;
	VkDeviceSize bytesPerDeviceLocalDeviceMemory = gpu->bytesPerDeviceLocalDeviceMemory;

	uint32_t valuesPerInoutBuffer       = gpu->valuesPerInoutBuffer;
	uint32_t inoutBuffersPerBuffer      = gpu->inoutBuffersPerBuffer;
	uint32_t buffersPerDeviceMemory     = gpu->buffersPerDeviceMemory;
	uint32_t buffersPerHeap             = gpu->buffersPerHeap;
	uint32_t deviceMemoriesPerHeap      = gpu->deviceMemoriesPerHeap;
	uint32_t hostVisibleMemoryTypeIndex = gpu->hostVisibleMemoryTypeIndex;
	uint32_t deviceLocalMemoryTypeIndex = gpu->deviceLocalMemoryTypeIndex;

	bool usingMemoryPriority = gpu->usingMemoryPriority;

	CreateBuffersData_t data = {0};
	VkResult result;

	size_t size = buffersPerHeap * 2 * sizeof(VkBindBufferMemoryInfo);
	data.memory = malloc(size);
#ifndef NDEBUG
	if (!data.memory) {
		MALLOC_FAILURE(data.memory)
		free_CreateBuffersData(data);
		return false;
	}
#endif

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) data.memory;

	VkBufferCreateInfo hostVisibleBufferCreateInfo;
	hostVisibleBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hostVisibleBufferCreateInfo.pNext = NULL;
	hostVisibleBufferCreateInfo.flags                 = 0;
	hostVisibleBufferCreateInfo.size                  = bytesPerBuffer;
	hostVisibleBufferCreateInfo.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hostVisibleBufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	hostVisibleBufferCreateInfo.queueFamilyIndexCount = 0;
	hostVisibleBufferCreateInfo.pQueueFamilyIndices   = NULL;

	VkBufferCreateInfo deviceLocalBufferCreateInfo;
	deviceLocalBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	deviceLocalBufferCreateInfo.pNext = NULL;
	deviceLocalBufferCreateInfo.flags                 = 0;
	deviceLocalBufferCreateInfo.size                  = bytesPerBuffer;
	deviceLocalBufferCreateInfo.usage                 = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	deviceLocalBufferCreateInfo.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
	deviceLocalBufferCreateInfo.queueFamilyIndexCount = 0;
	deviceLocalBufferCreateInfo.pQueueFamilyIndices   = NULL;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		GET_RESULT(vkCreateBuffer(g_device, &hostVisibleBufferCreateInfo, g_allocator, &hostVisibleBuffers[i]))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkCreateBuffer, 4, 'p', g_device, 'p', &hostVisibleBufferCreateInfo, 'p', g_allocator, 'p', &hostVisibleBuffers[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif

		GET_RESULT(vkCreateBuffer(g_device, &deviceLocalBufferCreateInfo, g_allocator, &deviceLocalBuffers[i]))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkCreateBuffer, 4, 'p', g_device, 'p', &deviceLocalBufferCreateInfo, 'p', g_allocator, 'p', &deviceLocalBuffers[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif
	}

	VkMemoryAllocateInfo hostVisibleMemoryAllocateInfo;
	hostVisibleMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	hostVisibleMemoryAllocateInfo.pNext = NULL;
	hostVisibleMemoryAllocateInfo.allocationSize  = bytesPerHostVisibleDeviceMemory;
	hostVisibleMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;

	VkMemoryAllocateInfo deviceLocalMemoryAllocateInfo;
	deviceLocalMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	deviceLocalMemoryAllocateInfo.pNext = NULL;
	deviceLocalMemoryAllocateInfo.allocationSize  = bytesPerDeviceLocalDeviceMemory;
	deviceLocalMemoryAllocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

	VkMemoryDedicatedAllocateInfo hostVisibleMemoryDedicatedAllocateInfo;
	hostVisibleMemoryDedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	hostVisibleMemoryDedicatedAllocateInfo.pNext = NULL;
	hostVisibleMemoryDedicatedAllocateInfo.image  = VK_NULL_HANDLE;
	hostVisibleMemoryDedicatedAllocateInfo.buffer = VK_NULL_HANDLE;

	VkMemoryDedicatedAllocateInfo deviceLocalMemoryDedicatedAllocateInfo;
	deviceLocalMemoryDedicatedAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO;
	deviceLocalMemoryDedicatedAllocateInfo.pNext = NULL;
	deviceLocalMemoryDedicatedAllocateInfo.image  = VK_NULL_HANDLE;
	deviceLocalMemoryDedicatedAllocateInfo.buffer = VK_NULL_HANDLE;

	VkMemoryPriorityAllocateInfoEXT hostVisibleMemoryPriorityAllocateInfo;
	hostVisibleMemoryPriorityAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	hostVisibleMemoryPriorityAllocateInfo.pNext = NULL;
	hostVisibleMemoryPriorityAllocateInfo.priority = 0.0f;

	VkMemoryPriorityAllocateInfoEXT deviceLocalMemoryPriorityAllocateInfo;
	deviceLocalMemoryPriorityAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	deviceLocalMemoryPriorityAllocateInfo.pNext = NULL;
	deviceLocalMemoryPriorityAllocateInfo.priority = 1.0f;

	const void** hostVisibleNext = &hostVisibleMemoryAllocateInfo.pNext;
	const void** deviceLocalNext = &deviceLocalMemoryAllocateInfo.pNext;

	if (buffersPerDeviceMemory == 1) {
		*hostVisibleNext = &hostVisibleMemoryDedicatedAllocateInfo;
		hostVisibleNext  = &hostVisibleMemoryDedicatedAllocateInfo.pNext;

		*deviceLocalNext = &deviceLocalMemoryDedicatedAllocateInfo;
		deviceLocalNext  = &deviceLocalMemoryDedicatedAllocateInfo.pNext;
	}

	if (usingMemoryPriority) {
		*hostVisibleNext = &hostVisibleMemoryPriorityAllocateInfo;
		hostVisibleNext  = &hostVisibleMemoryPriorityAllocateInfo.pNext;

		*deviceLocalNext = &deviceLocalMemoryPriorityAllocateInfo;
		deviceLocalNext  = &deviceLocalMemoryPriorityAllocateInfo.pNext;
	}

	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		hostVisibleMemoryDedicatedAllocateInfo.buffer = hostVisibleBuffers[i];
		deviceLocalMemoryDedicatedAllocateInfo.buffer = deviceLocalBuffers[i];

		GET_RESULT(vkAllocateMemory(g_device, &hostVisibleMemoryAllocateInfo, g_allocator, &hostVisibleDeviceMemories[i]))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkAllocateMemory, 4, 'p', g_device, 'p', &hostVisibleMemoryAllocateInfo, 'p', g_allocator, 'p', &hostVisibleDeviceMemories[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif

		GET_RESULT(vkAllocateMemory(g_device, &deviceLocalMemoryAllocateInfo, g_allocator, &deviceLocalDeviceMemories[i]))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkAllocateMemory, 4, 'p', g_device, 'p', &deviceLocalMemoryAllocateInfo, 'p', g_allocator, 'p', &deviceLocalDeviceMemories[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif
	}

	uint32_t bufIndex = 0; // Buffer index
	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		for (uint32_t j = 0; j < buffersPerDeviceMemory; j++, bufIndex++) {
			bindBufferMemoryInfos[bufIndex][0].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
			bindBufferMemoryInfos[bufIndex][0].pNext = NULL;
			bindBufferMemoryInfos[bufIndex][0].buffer       = hostVisibleBuffers[bufIndex];
			bindBufferMemoryInfos[bufIndex][0].memory       = hostVisibleDeviceMemories[i];
			bindBufferMemoryInfos[bufIndex][0].memoryOffset = bytesPerHostVisibleBuffer * j;

			bindBufferMemoryInfos[bufIndex][1].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
			bindBufferMemoryInfos[bufIndex][1].pNext = NULL;
			bindBufferMemoryInfos[bufIndex][1].buffer       = deviceLocalBuffers[bufIndex];
			bindBufferMemoryInfos[bufIndex][1].memory       = deviceLocalDeviceMemories[i];
			bindBufferMemoryInfos[bufIndex][1].memoryOffset = bytesPerDeviceLocalBuffer * j;
		}
	}

	GET_RESULT(vkBindBufferMemory2(g_device, buffersPerHeap * 2, (VkBindBufferMemoryInfo*) bindBufferMemoryInfos))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkBindBufferMemory2, 3, 'p', g_device, 'u', buffersPerHeap * 2, 'p', bindBufferMemoryInfos)
		free_CreateBuffersData(data);
		return false;
	}
#endif

	uint32_t inoIndex = 0; // Inout-buffer index
	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		for (uint32_t j = 0; j < buffersPerDeviceMemory; j++) {
			for (uint32_t k = 0; k < inoutBuffersPerBuffer; k++, inoIndex++) {
				GET_RESULT(vkMapMemory(g_device, hostVisibleDeviceMemories[i], bytesPerHostVisibleBuffer * j + bytesPerInoutBuffer * k, bytesPerInoutBuffer, 0, (void**) &mappedHostVisibleInBuffers[inoIndex]))
#ifndef NDEBUG
				if (result) {
					VULKAN_FAILURE(vkMapMemory, 6, 'p', g_device, 'p', hostVisibleDeviceMemories[i], 'u', bytesPerHostVisibleBuffer * j + bytesPerInoutBuffer * k, 'u', bytesPerInoutBuffer, 'u', 0, 'p', (void**) &mappedHostVisibleInBuffers[inoIndex])
					free_CreateBuffersData(data);
					return false;
				}
#endif
				mappedHostVisibleOutBuffers[inoIndex] = (step_t*) (mappedHostVisibleInBuffers[inoIndex] + valuesPerInoutBuffer);
			}
		}
	}

	free_CreateBuffersData(data);

#ifndef NDEBUG
	if(g_debugMessenger) {
		char objectName[80];
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;
		debugUtilsObjectNameInfo.objectType  = VK_OBJECT_TYPE_BUFFER;
		debugUtilsObjectNameInfo.pObjectName = objectName;

		bufIndex = 0;
		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			for (uint32_t j = 0; j < buffersPerDeviceMemory; j++, bufIndex++) {
				sprintf(objectName,
					"Host visible buffer %u/%u (Device memory %u/%u)",
					j + 1, buffersPerDeviceMemory, i + 1, deviceMemoriesPerHeap
				);

				debugUtilsObjectNameInfo.objectHandle = (uint64_t) hostVisibleBuffers[bufIndex];
				SET_DEBUG_NAME()

				strcpy(objectName, "Device local");
				objectName[12] = ' '; // Remove '\0' from strcpy

				debugUtilsObjectNameInfo.objectHandle = (uint64_t) deviceLocalBuffers[bufIndex];
				SET_DEBUG_NAME()
			}
		}

		debugUtilsObjectNameInfo.objectType  = VK_OBJECT_TYPE_DEVICE_MEMORY;
		debugUtilsObjectNameInfo.pObjectName = objectName;

		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			sprintf(objectName,
				"Host visible device memory %u/%u",
				i + 1, deviceMemoriesPerHeap
			);

			debugUtilsObjectNameInfo.objectHandle = (uint64_t) hostVisibleDeviceMemories[i];
			SET_DEBUG_NAME()

			strcpy(objectName, "Device local");
			objectName[12] = ' '; // Remove '\0' from strcpy

			debugUtilsObjectNameInfo.objectHandle = (uint64_t) deviceLocalDeviceMemories[i];
			SET_DEBUG_NAME()
		}
	}
#endif

	END_FUNC
	return true;
}

typedef struct CreateDescriptorsData
{
	void* memory;
} CreateDescriptorsData_t;

static void free_CreateDescriptorsData(CreateDescriptorsData_t data)
{
	free(data.memory);
}

bool create_descriptors(Gpu_t* gpu)
{
	BEGIN_FUNC

	const VkBuffer* restrict deviceLocalBuffers = gpu->deviceLocalBuffers;

	VkDeviceSize bytesPerInBuffer    = gpu->bytesPerInBuffer;
	VkDeviceSize bytesPerOutBuffer   = gpu->bytesPerOutBuffer;
	VkDeviceSize bytesPerInoutBuffer = gpu->bytesPerInoutBuffer;

	uint32_t inoutBuffersPerBuffer           = gpu->inoutBuffersPerBuffer;
	uint32_t inoutBuffersPerHeap             = gpu->inoutBuffersPerHeap;
	uint32_t buffersPerDeviceMemory          = gpu->buffersPerDeviceMemory;
	uint32_t buffersPerHeap                  = gpu->buffersPerHeap;
	uint32_t deviceMemoriesPerHeap           = gpu->deviceMemoriesPerHeap;
	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	CreateDescriptorsData_t data = {0};
	VkResult result;

	size_t size =
		inoutBuffersPerHeap     * sizeof(VkDescriptorSetLayout) + // Descriptor set layouts
		inoutBuffersPerHeap     * sizeof(VkWriteDescriptorSet) +  // In-buffer & out-buffer descriptor set information
		inoutBuffersPerHeap * 2 * sizeof(VkDescriptorBufferInfo); // In-buffer & out-buffer descriptor buffer information

	data.memory = malloc(size);
#ifndef NDEBUG
	if (!data.memory) {
		MALLOC_FAILURE(data.memory)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif

	VkDescriptorSetLayout*   descriptorSetLayouts      = (VkDescriptorSetLayout*) data.memory;
	VkWriteDescriptorSet*    writeDescriptorSets       = (VkWriteDescriptorSet*)       (descriptorSetLayouts + inoutBuffersPerHeap);
	VkDescriptorBufferInfo (*descriptorBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (writeDescriptorSets  + inoutBuffersPerHeap);

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

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = NULL;
	descriptorSetLayoutCreateInfo.flags        = 0;
	descriptorSetLayoutCreateInfo.bindingCount = sizeof(descriptorSetLayoutBindings) / sizeof(descriptorSetLayoutBindings[0]);
	descriptorSetLayoutCreateInfo.pBindings    = descriptorSetLayoutBindings;

	VkDescriptorPoolSize descriptorPoolSizes[1];
	descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[0].descriptorCount = inoutBuffersPerHeap * 2;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = NULL;
	descriptorPoolCreateInfo.flags         = 0;
	descriptorPoolCreateInfo.maxSets       = inoutBuffersPerHeap;
	descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSizes) / sizeof(descriptorPoolSizes[0]);
	descriptorPoolCreateInfo.pPoolSizes    = descriptorPoolSizes;

	GET_RESULT(vkCreateDescriptorSetLayout(g_device, &descriptorSetLayoutCreateInfo, g_allocator, &gpu->descriptorSetLayout))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateDescriptorSetLayout, 4, 'p', g_device, 'p', &descriptorSetLayoutCreateInfo, 'p', g_allocator, 'p', &gpu->descriptorSetLayout)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif

	GET_RESULT(vkCreateDescriptorPool(g_device, &descriptorPoolCreateInfo, g_allocator, &gpu->descriptorPool))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateDescriptorPool, 4, 'p', g_device, 'p', &descriptorPoolCreateInfo, 'p', g_allocator, 'p', &gpu->descriptorPool)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool      descriptorPool      = gpu->descriptorPool;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		descriptorSetLayouts[i] = descriptorSetLayout;

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = NULL;
	descriptorSetAllocateInfo.descriptorPool     = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = inoutBuffersPerHeap;
	descriptorSetAllocateInfo.pSetLayouts        = descriptorSetLayouts;

	GET_RESULT(vkAllocateDescriptorSets(g_device, &descriptorSetAllocateInfo, gpu->descriptorSets))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkAllocateDescriptorSets, 3, 'p', g_device, 'p', &descriptorSetAllocateInfo, 'p', gpu->descriptorSets)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif

	VkDescriptorSet* descriptorSets = gpu->descriptorSets;

	uint32_t inoIndex = 0; // Inout-buffer index
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			descriptorBufferInfos[inoIndex][0].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[inoIndex][0].offset = bytesPerInoutBuffer * j;
			descriptorBufferInfos[inoIndex][0].range  = bytesPerInBuffer;

			descriptorBufferInfos[inoIndex][1].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[inoIndex][1].offset = bytesPerInBuffer + bytesPerInoutBuffer * j;
			descriptorBufferInfos[inoIndex][1].range  = bytesPerOutBuffer;

			writeDescriptorSets[inoIndex].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[inoIndex].pNext = NULL;
			writeDescriptorSets[inoIndex].dstSet           = descriptorSets[inoIndex];
			writeDescriptorSets[inoIndex].dstBinding       = 0;
			writeDescriptorSets[inoIndex].dstArrayElement  = 0;
			writeDescriptorSets[inoIndex].descriptorCount  = sizeof(descriptorBufferInfos[inoIndex]) / sizeof(descriptorBufferInfos[inoIndex][0]);
			writeDescriptorSets[inoIndex].descriptorType   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[inoIndex].pImageInfo       = NULL;
			writeDescriptorSets[inoIndex].pBufferInfo      = descriptorBufferInfos[inoIndex];
			writeDescriptorSets[inoIndex].pTexelBufferView = NULL;
		}
	}

	vkUpdateDescriptorSets(g_device, inoutBuffersPerHeap, writeDescriptorSets, 0, NULL);

	if (transferQueueTimestampValidBits || computeQueueTimestampValidBits) {
		VkQueryPoolCreateInfo queryPoolCreateInfo;
		queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
		queryPoolCreateInfo.pNext = NULL;
		queryPoolCreateInfo.flags              = 0;
		queryPoolCreateInfo.queryType          = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount         = inoutBuffersPerHeap * 4;
		queryPoolCreateInfo.pipelineStatistics = 0;

		GET_RESULT(vkCreateQueryPool(g_device, &queryPoolCreateInfo, g_allocator, &gpu->queryPool))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkCreateQueryPool, 4, 'p', g_device, 'p', &queryPoolCreateInfo, 'p', g_allocator, 'p', &gpu->queryPool)
			free_CreateDescriptorsData(data);
			return false;
		}
#endif
	}

	free_CreateDescriptorsData(data);

#ifndef NDEBUG
	if(g_debugMessenger) {
		char objectName[122];
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;
		debugUtilsObjectNameInfo.objectType  = VK_OBJECT_TYPE_DESCRIPTOR_SET;
		debugUtilsObjectNameInfo.pObjectName = objectName;

		inoIndex = 0;
		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			for (uint32_t j = 0; j < buffersPerDeviceMemory; j++) {
				for (uint32_t k = 0; k < inoutBuffersPerBuffer; k++, inoIndex++) {
					sprintf(objectName,
						"Descriptor set (Inout-buffer: %u/%u, Buffer: %u/%u, Device memory: %u/%u)",
						k + 1, inoutBuffersPerBuffer, j + 1, buffersPerDeviceMemory, i + 1, deviceMemoriesPerHeap
					);

					debugUtilsObjectNameInfo.objectHandle = (uint64_t) descriptorSets[inoIndex];
					SET_DEBUG_NAME()
				}
			}
		}
	}
#endif

	END_FUNC
	return true;
}

typedef struct CreatePipelineData
{
	void* shaderCode;
	void* pipelineCache;
} CreatePipelineData_t;

static void free_CreatePipelineData(CreatePipelineData_t data)
{
	free(data.shaderCode);
	free(data.pipelineCache);
}

bool create_pipeline(Gpu_t* gpu)
{
	BEGIN_FUNC

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;

	uint32_t computeWorkGroupSize = gpu->computeWorkGroupSize;

	bool usingShaderInt16         = gpu->usingShaderInt16;
	bool usingShaderInt64         = gpu->usingShaderInt64;
	bool usingSubgroupSizeControl = gpu->usingSubgroupSizeControl;

	CreatePipelineData_t data = {0};
	VkResult result;
	size_t   readResult;
	int      fileResult;

	const char* shaderName;
	if (usingShaderInt16 && usingShaderInt64) shaderName = SHADER_16_64_NAME;
	else if (usingShaderInt16)                shaderName = SHADER_16_NAME;
	else if (usingShaderInt64)                shaderName = SHADER_64_NAME;
	else                                      shaderName = SHADER_NOEXT_NAME;

	// Get shader code from file, pre-compiled into SPIR-V
	FILE* file = fopen(shaderName, "rb");
#ifndef NDEBUG
	if (!file) {
		FOPEN_FAILURE(shaderName, "rb")
		free_CreatePipelineData(data);
		return false;
	}
#endif

	GET_FILE_RESULT(fseek(file, 0, SEEK_END))
#ifndef NDEBUG
	if (fileResult) {
		FSEEK_FAILURE(0, SEEK_END)
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif

	long fileSize = ftell(file);
#ifndef NDEBUG
	if (fileSize == -1) {
		FTELL_FAILURE();
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}

	if (fileSize % 4) {
		fputs("Vulkan failure: SPIR-V file size isn't a multiple of 4\n\n", stderr);
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif

	rewind(file);

	size_t size     = (size_t) fileSize;
	data.shaderCode = malloc(size);
#ifndef NDEBUG
	if (!data.shaderCode) {
		MALLOC_FAILURE(data.shaderCode)
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif

	size_t    shaderSize = (size_t)    fileSize;
	uint32_t* shaderCode = (uint32_t*) data.shaderCode;

	GET_READ_RESULT(fread(shaderCode, 1, shaderSize, file))
#ifndef NDEBUG
	if (readResult != shaderSize) {
		FREAD_FAILURE(shaderCode, 1, shaderSize)
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif

	fclose(file);

	// Get pipeline cache from file, if it exists
	size_t cacheSize = 0;
	void*  cacheData = NULL;
	file = fopen(PIPELINE_CACHE_NAME, "rb");

	if (file) {
		GET_FILE_RESULT(fseek(file, 0, SEEK_END))
#ifndef NDEBUG
		if (fileResult) {
			FSEEK_FAILURE(0, SEEK_END)
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif

		fileSize = ftell(file);
#ifndef NDEBUG
		if (fileSize == -1) {
			FTELL_FAILURE();
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif

		rewind(file);

		size               = (size_t) fileSize;
		data.pipelineCache = malloc(size);
#ifndef NDEBUG
		if (!data.pipelineCache) {
			MALLOC_FAILURE(data.pipelineCache)
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif

		cacheSize = (size_t) fileSize;
		cacheData = data.pipelineCache;

		GET_READ_RESULT(fread(cacheData, 1, cacheSize, file))
#ifndef NDEBUG
		if (readResult != cacheSize) {
			FREAD_FAILURE(cacheData, 1, cacheSize)
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif

		fclose(file);
	}

	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = NULL;
	shaderModuleCreateInfo.flags    = 0;
	shaderModuleCreateInfo.codeSize = shaderSize;
	shaderModuleCreateInfo.pCode    = shaderCode;

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = NULL;
	pipelineCacheCreateInfo.flags           = 0;
	pipelineCacheCreateInfo.initialDataSize = cacheSize;
	pipelineCacheCreateInfo.pInitialData    = cacheData;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags                  = 0;
	pipelineLayoutCreateInfo.setLayoutCount         = 1;
	pipelineLayoutCreateInfo.pSetLayouts            = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges    = NULL;

	GET_RESULT(vkCreateShaderModule(g_device, &shaderModuleCreateInfo, g_allocator, &gpu->shaderModule))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateShaderModule, 4, 'p', g_device, 'p', &shaderModuleCreateInfo, 'p', g_allocator, 'p', &gpu->shaderModule)
		free_CreatePipelineData(data);
		return false;
	}
#endif

	GET_RESULT(vkCreatePipelineCache(g_device, &pipelineCacheCreateInfo, g_allocator, &gpu->pipelineCache))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreatePipelineCache, 4, 'p', g_device, 'p', &pipelineCacheCreateInfo, 'p', g_allocator, 'p', &gpu->pipelineCache)
		free_CreatePipelineData(data);
		return false;
	}
#endif

	GET_RESULT(vkCreatePipelineLayout(g_device, &pipelineLayoutCreateInfo, g_allocator, &gpu->pipelineLayout))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreatePipelineLayout, 4, 'p', g_device, 'p', &pipelineLayoutCreateInfo, 'p', g_allocator, 'p', &gpu->pipelineLayout)
		free_CreatePipelineData(data);
		return false;
	}
#endif

	VkShaderModule   shaderModule   = gpu->shaderModule;
	VkPipelineCache  pipelineCache  = gpu->pipelineCache;
	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;

	vkDestroyDescriptorSetLayout(g_device, descriptorSetLayout, g_allocator);
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	uint32_t specialisationData[1];
	specialisationData[0] = computeWorkGroupSize;

	VkSpecializationMapEntry specialisationMapEntries[1];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset     = 0;
	specialisationMapEntries[0].size       = sizeof(specialisationData[0]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = sizeof(specialisationMapEntries) / sizeof(specialisationMapEntries[0]);
	specialisationInfo.pMapEntries   = specialisationMapEntries;
	specialisationInfo.dataSize      = sizeof(specialisationData);
	specialisationInfo.pData         = specialisationData;

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
	pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineShaderStageCreateInfo.pNext = NULL;
	pipelineShaderStageCreateInfo.flags               = usingSubgroupSizeControl ? VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT : 0;
	pipelineShaderStageCreateInfo.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineShaderStageCreateInfo.module              = shaderModule;
	pipelineShaderStageCreateInfo.pName               = "main";
	pipelineShaderStageCreateInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo computePipelineCreateInfo;
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = NULL;
	computePipelineCreateInfo.flags              = 0;
	computePipelineCreateInfo.stage              = pipelineShaderStageCreateInfo;
	computePipelineCreateInfo.layout             = pipelineLayout;
	computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	computePipelineCreateInfo.basePipelineIndex  = 0;

	GET_RESULT(vkCreateComputePipelines(g_device, pipelineCache, 1, &computePipelineCreateInfo, g_allocator, &gpu->pipeline))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateComputePipelines, 6, 'p', g_device, 'p', pipelineCache, 'u', 1, 'p', &computePipelineCreateInfo, 'p', g_allocator, 'p', &gpu->pipeline)
		free_CreatePipelineData(data);
		return false;
	}
#endif

	vkDestroyShaderModule(g_device, shaderModule, g_allocator);
	gpu->shaderModule = VK_NULL_HANDLE;

	free_CreatePipelineData(data);

	END_FUNC
	return true;
}

typedef struct CreateCommandsData
{
	void* memory;
} CreateCommandsData_t;

static void free_CreateCommandsData(CreateCommandsData_t data)
{
	free(data.memory);
}

bool create_commands(Gpu_t* gpu)
{
	BEGIN_FUNC

	const VkBuffer*        restrict hostVisibleBuffers     = gpu->hostVisibleBuffers;
	const VkBuffer*        restrict deviceLocalBuffers     = gpu->deviceLocalBuffers;
	const VkDescriptorSet* restrict descriptorSets         = gpu->descriptorSets;
	VkCommandBuffer*       restrict transferCommandBuffers = gpu->transferCommandBuffers;
	VkCommandBuffer*       restrict computeCommandBuffers  = gpu->computeCommandBuffers;
	VkSemaphore*           restrict semaphores             = gpu->semaphores;

	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;
	VkPipeline       pipeline       = gpu->pipeline;
	VkQueryPool      queryPool      = gpu->queryPool;

	VkDeviceSize bytesPerInBuffer    = gpu->bytesPerInBuffer;
	VkDeviceSize bytesPerOutBuffer   = gpu->bytesPerOutBuffer;
	VkDeviceSize bytesPerInoutBuffer = gpu->bytesPerInoutBuffer;

	uint32_t inoutBuffersPerBuffer           = gpu->inoutBuffersPerBuffer;
	uint32_t inoutBuffersPerHeap             = gpu->inoutBuffersPerHeap;
	uint32_t buffersPerDeviceMemory          = gpu->buffersPerDeviceMemory;
	uint32_t buffersPerHeap                  = gpu->buffersPerHeap;
	uint32_t deviceMemoriesPerHeap           = gpu->deviceMemoriesPerHeap;
	uint32_t computeWorkGroupCount           = gpu->computeWorkGroupCount;
	uint32_t transferQueueFamilyIndex        = gpu->transferQueueFamilyIndex;
	uint32_t computeQueueFamilyIndex         = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	bool usingMaintenance4 = gpu->usingMaintenance4;

	CreateCommandsData_t data = {0};
	VkResult result;

	size_t size =
		inoutBuffersPerBuffer   * sizeof(VkBufferCopy) +              // In-buffer copy information
		inoutBuffersPerBuffer   * sizeof(VkBufferCopy) +              // Out-buffer copy information
		inoutBuffersPerHeap     * sizeof(VkBufferMemoryBarrier2KHR) + // Onetime buffer memory barriers
		inoutBuffersPerHeap * 3 * sizeof(VkBufferMemoryBarrier2KHR) + // Transfer buffer memory barriers
		inoutBuffersPerHeap * 2 * sizeof(VkBufferMemoryBarrier2KHR) + // Compute buffer memory barriers
		inoutBuffersPerHeap * 2 * sizeof(VkDependencyInfoKHR) +       // Transfer dependency information
		inoutBuffersPerHeap * 2 * sizeof(VkDependencyInfoKHR);        // Compute dependency information

	data.memory = malloc(size);
#ifndef NDEBUG
	if (!data.memory) {
		MALLOC_FAILURE(data.memory)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	VkBufferCopy* inBufferCopies  = (VkBufferCopy*) data.memory;
	VkBufferCopy* outBufferCopies = (VkBufferCopy*) (inBufferCopies + inoutBuffersPerBuffer);

	VkBufferMemoryBarrier2KHR*  onetimeBufferMemoryBarriers2      = (VkBufferMemoryBarrier2KHR*)     (outBufferCopies               + inoutBuffersPerBuffer);
	VkBufferMemoryBarrier2KHR (*transferBufferMemoryBarriers2)[3] = (VkBufferMemoryBarrier2KHR(*)[]) (onetimeBufferMemoryBarriers2  + inoutBuffersPerHeap);
	VkBufferMemoryBarrier2KHR (*computeBufferMemoryBarriers2)[2]  = (VkBufferMemoryBarrier2KHR(*)[]) (transferBufferMemoryBarriers2 + inoutBuffersPerHeap);

	VkDependencyInfoKHR (*transferDependencyInfos)[2] = (VkDependencyInfoKHR(*)[]) (computeBufferMemoryBarriers2 + inoutBuffersPerHeap);
	VkDependencyInfoKHR (*computeDependencyInfos)[2]  = (VkDependencyInfoKHR(*)[]) (transferDependencyInfos      + inoutBuffersPerHeap);

	VkCommandPoolCreateInfo onetimeCommandPoolCreateInfo;
	onetimeCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	onetimeCommandPoolCreateInfo.pNext = NULL;
	onetimeCommandPoolCreateInfo.flags            = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	onetimeCommandPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo transferCommandPoolCreateInfo;
	transferCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferCommandPoolCreateInfo.pNext = NULL;
	transferCommandPoolCreateInfo.flags            = 0;
	transferCommandPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo computeCommandPoolCreateInfo;
	computeCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computeCommandPoolCreateInfo.pNext = NULL;
	computeCommandPoolCreateInfo.flags            = 0;
	computeCommandPoolCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;

	GET_RESULT(vkCreateCommandPool(g_device, &onetimeCommandPoolCreateInfo, g_allocator, &gpu->onetimeCommandPool))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateCommandPool, 4, 'p', g_device, 'p', &onetimeCommandPoolCreateInfo, 'p', g_allocator, 'p', &gpu->onetimeCommandPool)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	GET_RESULT(vkCreateCommandPool(g_device, &transferCommandPoolCreateInfo, g_allocator, &gpu->transferCommandPool))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateCommandPool, 4, 'p', g_device, 'p', &transferCommandPoolCreateInfo, 'p', g_allocator, 'p', &gpu->transferCommandPool)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	GET_RESULT(vkCreateCommandPool(g_device, &computeCommandPoolCreateInfo, g_allocator, &gpu->computeCommandPool))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkCreateCommandPool, 4, 'p', g_device, 'p', &computeCommandPoolCreateInfo, 'p', g_allocator, 'p', &gpu->computeCommandPool)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	VkCommandPool onetimeCommandPool  = gpu->onetimeCommandPool;
	VkCommandPool transferCommandPool = gpu->transferCommandPool;
	VkCommandPool computeCommandPool  = gpu->computeCommandPool;

	VkCommandBufferAllocateInfo onetimeCommandBufferAllocateInfo;
	onetimeCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	onetimeCommandBufferAllocateInfo.pNext = NULL;
	onetimeCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	onetimeCommandBufferAllocateInfo.commandPool        = onetimeCommandPool;
	onetimeCommandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo;
	transferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transferCommandBufferAllocateInfo.pNext = NULL;
	transferCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandPool        = transferCommandPool;
	transferCommandBufferAllocateInfo.commandBufferCount = inoutBuffersPerHeap;

	VkCommandBufferAllocateInfo computeCommandBufferAllocateInfo;
	computeCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	computeCommandBufferAllocateInfo.pNext = NULL;
	computeCommandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeCommandBufferAllocateInfo.commandPool        = computeCommandPool;
	computeCommandBufferAllocateInfo.commandBufferCount = inoutBuffersPerHeap;

	GET_RESULT(vkAllocateCommandBuffers(g_device, &onetimeCommandBufferAllocateInfo, &gpu->onetimeCommandBuffer))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkAllocateCommandBuffers, 3, 'p', g_device, 'p', &onetimeCommandBufferAllocateInfo, 'p', &gpu->onetimeCommandBuffer)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	GET_RESULT(vkAllocateCommandBuffers(g_device, &transferCommandBufferAllocateInfo, transferCommandBuffers))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkAllocateCommandBuffers, 3, 'p', g_device, 'p', &transferCommandBufferAllocateInfo, 'p', transferCommandBuffers)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	GET_RESULT(vkAllocateCommandBuffers(g_device, &computeCommandBufferAllocateInfo, computeCommandBuffers))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkAllocateCommandBuffers, 3, 'p', g_device, 'p', &computeCommandBufferAllocateInfo, 'p', computeCommandBuffers)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;

	for (uint32_t i = 0; i < inoutBuffersPerBuffer; i++) {
		inBufferCopies[i].srcOffset = bytesPerInoutBuffer * i;
		inBufferCopies[i].dstOffset = bytesPerInoutBuffer * i;
		inBufferCopies[i].size      = bytesPerInBuffer;

		outBufferCopies[i].srcOffset = bytesPerInoutBuffer * i + bytesPerInBuffer;
		outBufferCopies[i].dstOffset = bytesPerInoutBuffer * i + bytesPerInBuffer;
		outBufferCopies[i].size      = bytesPerOutBuffer;
	}

	uint32_t inoIndex = 0; // Inout-buffer index
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			onetimeBufferMemoryBarriers2[inoIndex].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			onetimeBufferMemoryBarriers2[inoIndex].pNext = NULL;
			onetimeBufferMemoryBarriers2[inoIndex].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
			onetimeBufferMemoryBarriers2[inoIndex].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			onetimeBufferMemoryBarriers2[inoIndex].dstStageMask        = VK_PIPELINE_STAGE_2_NONE;
			onetimeBufferMemoryBarriers2[inoIndex].dstAccessMask       = VK_ACCESS_2_NONE;
			onetimeBufferMemoryBarriers2[inoIndex].srcQueueFamilyIndex = transferQueueFamilyIndex;
			onetimeBufferMemoryBarriers2[inoIndex].dstQueueFamilyIndex = computeQueueFamilyIndex;
			onetimeBufferMemoryBarriers2[inoIndex].buffer              = deviceLocalBuffers[i];
			onetimeBufferMemoryBarriers2[inoIndex].offset              = bytesPerInoutBuffer * j;
			onetimeBufferMemoryBarriers2[inoIndex].size                = bytesPerInBuffer;

			transferBufferMemoryBarriers2[inoIndex][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			transferBufferMemoryBarriers2[inoIndex][0].pNext = NULL;
			transferBufferMemoryBarriers2[inoIndex][0].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers2[inoIndex][0].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers2[inoIndex][0].dstStageMask        = VK_PIPELINE_STAGE_2_NONE;
			transferBufferMemoryBarriers2[inoIndex][0].dstAccessMask       = VK_ACCESS_2_NONE;
			transferBufferMemoryBarriers2[inoIndex][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][0].buffer              = deviceLocalBuffers[i];
			transferBufferMemoryBarriers2[inoIndex][0].offset              = bytesPerInoutBuffer * j;
			transferBufferMemoryBarriers2[inoIndex][0].size                = bytesPerInBuffer;

			transferBufferMemoryBarriers2[inoIndex][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			transferBufferMemoryBarriers2[inoIndex][1].pNext = NULL;
			transferBufferMemoryBarriers2[inoIndex][1].srcStageMask        = VK_PIPELINE_STAGE_2_NONE;
			transferBufferMemoryBarriers2[inoIndex][1].srcAccessMask       = VK_ACCESS_2_NONE;
			transferBufferMemoryBarriers2[inoIndex][1].dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers2[inoIndex][1].dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT;
			transferBufferMemoryBarriers2[inoIndex][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers2[inoIndex][1].buffer              = deviceLocalBuffers[i];
			transferBufferMemoryBarriers2[inoIndex][1].offset              = bytesPerInoutBuffer * j + bytesPerInBuffer;
			transferBufferMemoryBarriers2[inoIndex][1].size                = bytesPerOutBuffer;

			transferBufferMemoryBarriers2[inoIndex][2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			transferBufferMemoryBarriers2[inoIndex][2].pNext = NULL;
			transferBufferMemoryBarriers2[inoIndex][2].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers2[inoIndex][2].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers2[inoIndex][2].dstStageMask        = VK_PIPELINE_STAGE_2_HOST_BIT;
			transferBufferMemoryBarriers2[inoIndex][2].dstAccessMask       = VK_ACCESS_2_HOST_READ_BIT;
			transferBufferMemoryBarriers2[inoIndex][2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			transferBufferMemoryBarriers2[inoIndex][2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			transferBufferMemoryBarriers2[inoIndex][2].buffer              = hostVisibleBuffers[i];
			transferBufferMemoryBarriers2[inoIndex][2].offset              = bytesPerInoutBuffer * j + bytesPerInBuffer;
			transferBufferMemoryBarriers2[inoIndex][2].size                = bytesPerOutBuffer;

			computeBufferMemoryBarriers2[inoIndex][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			computeBufferMemoryBarriers2[inoIndex][0].pNext = NULL;
			computeBufferMemoryBarriers2[inoIndex][0].srcStageMask        = VK_PIPELINE_STAGE_2_NONE;
			computeBufferMemoryBarriers2[inoIndex][0].srcAccessMask       = VK_ACCESS_2_NONE;
			computeBufferMemoryBarriers2[inoIndex][0].dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers2[inoIndex][0].dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
			computeBufferMemoryBarriers2[inoIndex][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][0].buffer              = deviceLocalBuffers[i];
			computeBufferMemoryBarriers2[inoIndex][0].offset              = bytesPerInoutBuffer * j;
			computeBufferMemoryBarriers2[inoIndex][0].size                = bytesPerInBuffer;

			computeBufferMemoryBarriers2[inoIndex][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			computeBufferMemoryBarriers2[inoIndex][1].pNext = NULL;
			computeBufferMemoryBarriers2[inoIndex][1].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers2[inoIndex][1].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			computeBufferMemoryBarriers2[inoIndex][1].dstStageMask        = VK_PIPELINE_STAGE_2_NONE;
			computeBufferMemoryBarriers2[inoIndex][1].dstAccessMask       = VK_ACCESS_2_NONE;
			computeBufferMemoryBarriers2[inoIndex][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers2[inoIndex][1].buffer              = deviceLocalBuffers[i];
			computeBufferMemoryBarriers2[inoIndex][1].offset              = bytesPerInoutBuffer * j + bytesPerInBuffer;
			computeBufferMemoryBarriers2[inoIndex][1].size                = bytesPerOutBuffer;

			transferDependencyInfos[inoIndex][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			transferDependencyInfos[inoIndex][0].pNext = NULL;
			transferDependencyInfos[inoIndex][0].dependencyFlags          = 0;
			transferDependencyInfos[inoIndex][0].memoryBarrierCount       = 0;
			transferDependencyInfos[inoIndex][0].pMemoryBarriers          = NULL;
			transferDependencyInfos[inoIndex][0].bufferMemoryBarrierCount = 2;
			transferDependencyInfos[inoIndex][0].pBufferMemoryBarriers    = &transferBufferMemoryBarriers2[inoIndex][0];
			transferDependencyInfos[inoIndex][0].imageMemoryBarrierCount  = 0;
			transferDependencyInfos[inoIndex][0].pImageMemoryBarriers     = NULL;

			transferDependencyInfos[inoIndex][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			transferDependencyInfos[inoIndex][1].pNext = NULL;
			transferDependencyInfos[inoIndex][1].dependencyFlags          = 0;
			transferDependencyInfos[inoIndex][1].memoryBarrierCount       = 0;
			transferDependencyInfos[inoIndex][1].pMemoryBarriers          = NULL;
			transferDependencyInfos[inoIndex][1].bufferMemoryBarrierCount = 1;
			transferDependencyInfos[inoIndex][1].pBufferMemoryBarriers    = &transferBufferMemoryBarriers2[inoIndex][2];
			transferDependencyInfos[inoIndex][1].imageMemoryBarrierCount  = 0;
			transferDependencyInfos[inoIndex][1].pImageMemoryBarriers     = NULL;

			computeDependencyInfos[inoIndex][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			computeDependencyInfos[inoIndex][0].pNext = NULL;
			computeDependencyInfos[inoIndex][0].dependencyFlags          = 0;
			computeDependencyInfos[inoIndex][0].memoryBarrierCount       = 0;
			computeDependencyInfos[inoIndex][0].pMemoryBarriers          = NULL;
			computeDependencyInfos[inoIndex][0].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[inoIndex][0].pBufferMemoryBarriers    = &computeBufferMemoryBarriers2[inoIndex][0];
			computeDependencyInfos[inoIndex][0].imageMemoryBarrierCount  = 0;
			computeDependencyInfos[inoIndex][0].pImageMemoryBarriers     = NULL;

			computeDependencyInfos[inoIndex][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			computeDependencyInfos[inoIndex][1].pNext = NULL;
			computeDependencyInfos[inoIndex][1].dependencyFlags          = 0;
			computeDependencyInfos[inoIndex][1].memoryBarrierCount       = 0;
			computeDependencyInfos[inoIndex][1].pMemoryBarriers          = NULL;
			computeDependencyInfos[inoIndex][1].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[inoIndex][1].pBufferMemoryBarriers    = &computeBufferMemoryBarriers2[inoIndex][1];
			computeDependencyInfos[inoIndex][1].imageMemoryBarrierCount  = 0;
			computeDependencyInfos[inoIndex][1].pImageMemoryBarriers     = NULL;
		}
	}

	VkDependencyInfoKHR onetimeDependencyInfo;
	onetimeDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
	onetimeDependencyInfo.pNext = NULL;
	onetimeDependencyInfo.dependencyFlags          = 0;
	onetimeDependencyInfo.memoryBarrierCount       = 0;
	onetimeDependencyInfo.pMemoryBarriers          = NULL;
	onetimeDependencyInfo.bufferMemoryBarrierCount = inoutBuffersPerHeap;
	onetimeDependencyInfo.pBufferMemoryBarriers    = onetimeBufferMemoryBarriers2;
	onetimeDependencyInfo.imageMemoryBarrierCount  = 0;
	onetimeDependencyInfo.pImageMemoryBarriers     = NULL;

	VkCommandBufferBeginInfo onetimeCommandBufferBeginInfo;
	onetimeCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	onetimeCommandBufferBeginInfo.pNext = NULL;
	onetimeCommandBufferBeginInfo.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	onetimeCommandBufferBeginInfo.pInheritanceInfo = NULL;

	VkCommandBufferBeginInfo transferCommandBufferBeginInfo;
	transferCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	transferCommandBufferBeginInfo.pNext = NULL;
	transferCommandBufferBeginInfo.flags            = 0;
	transferCommandBufferBeginInfo.pInheritanceInfo = NULL;

	VkCommandBufferBeginInfo computeCommandBufferBeginInfo;
	computeCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	computeCommandBufferBeginInfo.pNext = NULL;
	computeCommandBufferBeginInfo.flags            = 0;
	computeCommandBufferBeginInfo.pInheritanceInfo = NULL;

	GET_RESULT(vkBeginCommandBuffer(onetimeCommandBuffer, &onetimeCommandBufferBeginInfo))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkBeginCommandBuffer, 2, 'p', onetimeCommandBuffer, 'p', &onetimeCommandBufferBeginInfo)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		vkCmdCopyBuffer(onetimeCommandBuffer, hostVisibleBuffers[i], deviceLocalBuffers[i], inoutBuffersPerBuffer, inBufferCopies);
	}

	if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
		vkCmdPipelineBarrier2KHR(onetimeCommandBuffer, &onetimeDependencyInfo);
	}

	GET_RESULT(vkEndCommandBuffer(onetimeCommandBuffer))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkEndCommandBuffer, 1, 'p', onetimeCommandBuffer)
		free_CreateCommandsData(data);
		return false;
	}
#endif

	inoIndex = 0;
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			GET_RESULT(vkBeginCommandBuffer(transferCommandBuffers[inoIndex], &transferCommandBufferBeginInfo))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkBeginCommandBuffer, 2, 'p', transferCommandBuffers[inoIndex], 'p', &transferCommandBufferBeginInfo)
				free_CreateCommandsData(data);
				return false;
			}
#endif

			if (transferQueueTimestampValidBits) {
				vkCmdResetQueryPool(transferCommandBuffers[inoIndex], queryPool, inoIndex * 4, 2);
				vkCmdWriteTimestamp2KHR(transferCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_NONE, queryPool, inoIndex * 4);
			}

			vkCmdCopyBuffer(transferCommandBuffers[inoIndex], hostVisibleBuffers[i], deviceLocalBuffers[i], 1, &inBufferCopies[j]);

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				vkCmdPipelineBarrier2KHR(transferCommandBuffers[inoIndex], &transferDependencyInfos[inoIndex][0]);
			}

			vkCmdCopyBuffer(transferCommandBuffers[inoIndex], deviceLocalBuffers[i], hostVisibleBuffers[i], 1, &outBufferCopies[j]);

			vkCmdPipelineBarrier2KHR(transferCommandBuffers[inoIndex], &transferDependencyInfos[inoIndex][1]);

			if (transferQueueTimestampValidBits) {
				vkCmdWriteTimestamp2KHR(transferCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_COPY_BIT, queryPool, inoIndex * 4 + 1);
			}

			GET_RESULT(vkEndCommandBuffer(transferCommandBuffers[inoIndex]))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkEndCommandBuffer, 1, 'p', transferCommandBuffers[inoIndex])
				free_CreateCommandsData(data);
				return false;
			}
#endif

			GET_RESULT(vkBeginCommandBuffer(computeCommandBuffers[inoIndex], &computeCommandBufferBeginInfo))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkBeginCommandBuffer, 2, 'p', computeCommandBuffers[inoIndex], 'p', &computeCommandBufferBeginInfo)
				free_CreateCommandsData(data);
				return false;
			}
#endif

			if (computeQueueTimestampValidBits) {
				vkCmdResetQueryPool(computeCommandBuffers[inoIndex], queryPool, inoIndex * 4 + 2, 2);
				vkCmdWriteTimestamp2KHR(computeCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_NONE, queryPool, inoIndex * 4 + 2);
			}

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				vkCmdPipelineBarrier2KHR(computeCommandBuffers[inoIndex], &computeDependencyInfos[inoIndex][0]);
			}

			vkCmdBindPipeline(computeCommandBuffers[inoIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			vkCmdBindDescriptorSets(computeCommandBuffers[inoIndex], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1, &descriptorSets[inoIndex], 0, NULL);
			vkCmdDispatchBase(computeCommandBuffers[inoIndex], 0, 0, 0, computeWorkGroupCount, 1, 1);

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				vkCmdPipelineBarrier2KHR(computeCommandBuffers[inoIndex], &computeDependencyInfos[inoIndex][1]);
			}

			if (computeQueueTimestampValidBits) {
				vkCmdWriteTimestamp2KHR(computeCommandBuffers[inoIndex], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, queryPool, inoIndex * 4 + 3);
			}

			GET_RESULT(vkEndCommandBuffer(computeCommandBuffers[inoIndex]))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkEndCommandBuffer, 1, 'p', computeCommandBuffers[inoIndex])
				free_CreateCommandsData(data);
				return false;
			}
#endif
		}
	}

	if (usingMaintenance4) {
		vkDestroyPipelineLayout(g_device, pipelineLayout, g_allocator);
		gpu->pipelineLayout = VK_NULL_HANDLE;
	}

	VkSemaphoreTypeCreateInfoKHR semaphoreTypeCreateInfo;
	semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR;
	semaphoreTypeCreateInfo.pNext = NULL;
	semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
	semaphoreTypeCreateInfo.initialValue  = 0;

	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;
	semaphoreCreateInfo.flags = 0;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++) {
		GET_RESULT(vkCreateSemaphore(g_device, &semaphoreCreateInfo, g_allocator, &semaphores[i]))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkCreateSemaphore, 4, 'p', g_device, 'p', &semaphoreCreateInfo, 'p', g_allocator, 'p', &semaphores[i])
			free_CreateCommandsData(data);
			return false;
		}
#endif
	}

	free_CreateCommandsData(data);

#ifndef NDEBUG
	if(g_debugMessenger) {
		char objectName[134];
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;
		debugUtilsObjectNameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_POOL;
		debugUtilsObjectNameInfo.objectHandle = (uint64_t) onetimeCommandPool;
		debugUtilsObjectNameInfo.pObjectName  = "Onetime command pool";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectHandle = (uint64_t) transferCommandPool;
		debugUtilsObjectNameInfo.pObjectName  = "Transfer command pool";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectHandle = (uint64_t) computeCommandPool;
		debugUtilsObjectNameInfo.pObjectName  = "Compute command pool";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
		debugUtilsObjectNameInfo.objectHandle = (uint64_t) onetimeCommandBuffer;
		debugUtilsObjectNameInfo.pObjectName  = "Onetime command buffer";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.pObjectName = objectName;

		inoIndex = 0;
		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			for (uint32_t j = 0; j < buffersPerDeviceMemory; j++) {
				for (uint32_t k = 0; k < inoutBuffersPerBuffer; k++, inoIndex++) {
					char specs[107];
					sprintf(specs,
						"(Inout-buffer: %u/%u, Buffer: %u/%u, Device memory: %u/%u)",
						k + 1, inoutBuffersPerBuffer, j + 1, buffersPerDeviceMemory, i + 1, deviceMemoriesPerHeap
					);

					strcpy(objectName, "Transfer command buffer ");
					strcat(objectName, specs);

					debugUtilsObjectNameInfo.objectType   = VK_OBJECT_TYPE_COMMAND_BUFFER;
					debugUtilsObjectNameInfo.objectHandle = (uint64_t) transferCommandBuffers[inoIndex];
					SET_DEBUG_NAME()

					strcpy(objectName, "Compute command buffer ");
					strcat(objectName, specs);

					debugUtilsObjectNameInfo.objectHandle = (uint64_t) computeCommandBuffers[inoIndex];
					SET_DEBUG_NAME()

					strcpy(objectName, "Transfer-compute semaphore ");
					strcat(objectName, specs);

					debugUtilsObjectNameInfo.objectType   = VK_OBJECT_TYPE_SEMAPHORE;
					debugUtilsObjectNameInfo.objectHandle = (uint64_t) semaphores[inoIndex];
					SET_DEBUG_NAME()
				}
			}
		}
	}
#endif

	END_FUNC
	return true;
}

#if END_ON == 1
static void* wait_for_input(void* ptr)
{
	puts("Calculating... press enter/return to stop\n");
	getchar();
	puts("Stopping...\n");

	atomic_bool* input = (atomic_bool*) ptr;
	atomic_store_explicit(input, true, memory_order_release);

	return NULL;
}
#endif

static void writeInBuffer(
	value_t* restrict mappedHostVisibleInBuffer,
	value_t* restrict firstValue,
	uint32_t valuesPerInoutBuffer,
	uint32_t valuesPerHeap
)
{
	value_t value = *firstValue;

	for (uint32_t i = 0; i < valuesPerInoutBuffer; i++, value += 2)
		mappedHostVisibleInBuffer[i] = value;

	*firstValue += valuesPerHeap * 2;
}

static void readOutBuffer(
	const volatile step_t* restrict mappedHostVisibleOutBuffer,
	value_t* restrict firstValue,
	value_t* restrict highestStepValues,
	step_t*  restrict highestStepCounts,
	step_t*  restrict longest,
	step_t*  restrict count,
	value_t* restrict prev,
	uint32_t valuesPerInoutBuffer
)
{
	value_t value    = *firstValue - 2;
	step_t  curCount = *count;

	value_t value0mod1 = *prev;
	step_t  steps0mod1 = *longest;

	for (uint32_t i = 0; i < valuesPerInoutBuffer; i++) {
		step_t steps = steps0mod1 + 1;
		value++;

		if (value == value0mod1 * 2) {
			value0mod1 = value;
			steps0mod1 = steps;
			highestStepValues[curCount] = value;
			highestStepCounts[curCount] = steps;
			curCount++;
		}

		steps = mappedHostVisibleOutBuffer[i];
		value++;

		if (steps > steps0mod1) {
			value0mod1 = value;
			steps0mod1 = steps;
			highestStepValues[curCount] = value;
			highestStepCounts[curCount] = steps;
			curCount++;
		}
	}

	*firstValue = value + 2;
	*count      = curCount;

	*prev       = value0mod1;
	*longest    = steps0mod1;
}

typedef struct SubmitCommandsData
{
	void* memory;
} SubmitCommandsData_t;

static void free_SubmitCommandsData(SubmitCommandsData_t data)
{
	free(data.memory);
}

bool submit_commands(Gpu_t* gpu)
{
	BEGIN_FUNC

	const VkDeviceMemory*         restrict hostVisibleDeviceMemories   = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer*        restrict transferCommandBuffers      = gpu->transferCommandBuffers;
	const VkCommandBuffer*        restrict computeCommandBuffers       = gpu->computeCommandBuffers;
	const VkSemaphore*            restrict semaphores                  = gpu->semaphores;
	value_t* const*               restrict mappedHostVisibleInBuffers  = gpu->mappedHostVisibleInBuffers;
	const volatile step_t* const* restrict mappedHostVisibleOutBuffers = (const volatile step_t* const*) gpu->mappedHostVisibleOutBuffers;

	VkCommandPool   onetimeCommandPool   = gpu->onetimeCommandPool;
	VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;
	VkQueryPool     queryPool            = gpu->queryPool;

	VkDeviceSize bytesPerInBuffer          = gpu->bytesPerInBuffer;
	VkDeviceSize bytesPerOutBuffer         = gpu->bytesPerOutBuffer;
	VkDeviceSize bytesPerInoutBuffer       = gpu->bytesPerInoutBuffer;
	VkDeviceSize bytesPerHostVisibleBuffer = gpu->bytesPerHostVisibleBuffer;

	uint32_t valuesPerInoutBuffer            = gpu->valuesPerInoutBuffer;
	uint32_t valuesPerHeap                   = gpu->valuesPerHeap;
	uint32_t inoutBuffersPerBuffer           = gpu->inoutBuffersPerBuffer;
	uint32_t inoutBuffersPerHeap             = gpu->inoutBuffersPerHeap;
	uint32_t buffersPerDeviceMemory          = gpu->buffersPerDeviceMemory;
	uint32_t deviceMemoriesPerHeap           = gpu->deviceMemoriesPerHeap;
	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	float timestampPeriod = gpu->timestampPeriod;
	bool usingNonCoherent = gpu->usingNonCoherent;

	SubmitCommandsData_t data = {0};
	VkResult result;

	size_t size =
		inoutBuffersPerHeap * sizeof(value_t) +                      // Tested values
		inoutBuffersPerHeap * sizeof(VkMappedMemoryRange) +          // In-buffer mapped memory ranges
		inoutBuffersPerHeap * sizeof(VkMappedMemoryRange) +          // Out-buffer mapped memory ranges
		inoutBuffersPerHeap * sizeof(VkSubmitInfo2KHR) +             // Transfer submission information
		inoutBuffersPerHeap * sizeof(VkSubmitInfo2KHR) +             // Compute submission information
		inoutBuffersPerHeap * sizeof(VkCommandBufferSubmitInfoKHR) + // Transfer command buffer submission information
		inoutBuffersPerHeap * sizeof(VkCommandBufferSubmitInfoKHR) + // Compute command buffer submission information
		inoutBuffersPerHeap * sizeof(VkSemaphoreSubmitInfoKHR) +     // Transfer-to-compute signal semaphore submission information
		inoutBuffersPerHeap * sizeof(VkSemaphoreSubmitInfoKHR) +     // Transfer-to-compute wait semaphore submission information
		inoutBuffersPerHeap * sizeof(VkSemaphoreSubmitInfoKHR) +     // Compute-to-transfer signal semaphore submission information
		inoutBuffersPerHeap * sizeof(VkSemaphoreSubmitInfoKHR) +     // Compute-to-transfer wait semaphore submission information
		inoutBuffersPerHeap * sizeof(VkSemaphoreWaitInfoKHR) +       // Transfer semaphore wait information
		inoutBuffersPerHeap * sizeof(VkSemaphoreWaitInfoKHR);        // Compute semaphore wait information

	data.memory = malloc(size);
#ifndef NDEBUG
	if (!data.memory) {
		MALLOC_FAILURE(data.memory)
		free_SubmitCommandsData(data);
		return false;
	}
#endif

	value_t* testedValues = (value_t*) data.memory;

	VkMappedMemoryRange* hostVisibleInBuffersMappedMemoryRanges  = (VkMappedMemoryRange*) (testedValues                           + inoutBuffersPerHeap);
	VkMappedMemoryRange* hostVisibleOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (hostVisibleInBuffersMappedMemoryRanges + inoutBuffersPerHeap);

	VkSubmitInfo2KHR* transferSubmitInfos2 = (VkSubmitInfo2KHR*) (hostVisibleOutBuffersMappedMemoryRanges + inoutBuffersPerHeap);
	VkSubmitInfo2KHR* computeSubmitInfos2  = (VkSubmitInfo2KHR*) (transferSubmitInfos2                    + inoutBuffersPerHeap);

	VkCommandBufferSubmitInfoKHR* transferCommandBufferSubmitInfos = (VkCommandBufferSubmitInfoKHR*) (computeSubmitInfos2              + inoutBuffersPerHeap);
	VkCommandBufferSubmitInfoKHR* computeCommandBufferSubmitInfos  = (VkCommandBufferSubmitInfoKHR*) (transferCommandBufferSubmitInfos + inoutBuffersPerHeap);

	VkSemaphoreSubmitInfoKHR* transferWaitSemaphoreSubmitInfos   = (VkSemaphoreSubmitInfoKHR*) (computeCommandBufferSubmitInfos    + inoutBuffersPerHeap);
	VkSemaphoreSubmitInfoKHR* transferSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (transferWaitSemaphoreSubmitInfos   + inoutBuffersPerHeap);
	VkSemaphoreSubmitInfoKHR* computeWaitSemaphoreSubmitInfos    = (VkSemaphoreSubmitInfoKHR*) (transferSignalSemaphoreSubmitInfos + inoutBuffersPerHeap);
	VkSemaphoreSubmitInfoKHR* computeSignalSemaphoreSubmitInfos  = (VkSemaphoreSubmitInfoKHR*) (computeWaitSemaphoreSubmitInfos    + inoutBuffersPerHeap);

	VkSemaphoreWaitInfoKHR* transferSemaphoreWaitInfos = (VkSemaphoreWaitInfoKHR*) (computeSignalSemaphoreSubmitInfos + inoutBuffersPerHeap);
	VkSemaphoreWaitInfoKHR* computeSemaphoreWaitInfos  = (VkSemaphoreWaitInfoKHR*) (transferSemaphoreWaitInfos        + inoutBuffersPerHeap);

	clock_t bmarkStart = clock();

	uint32_t inoIndex = 0;
	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		for (uint32_t j = 0; j < buffersPerDeviceMemory; j++) {
			for (uint32_t k = 0; k < inoutBuffersPerBuffer; k++, inoIndex++) {
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].pNext = NULL;
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].memory = hostVisibleDeviceMemories[i];
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].offset = bytesPerHostVisibleBuffer * j + bytesPerInoutBuffer * k;
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].size   = bytesPerInBuffer;

				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].pNext = NULL;
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].memory = hostVisibleDeviceMemories[i];
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].offset = bytesPerHostVisibleBuffer * j + bytesPerInoutBuffer * k + bytesPerInBuffer;
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].size   = bytesPerOutBuffer;
			}
		}
	}

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++) {
		transferCommandBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
		transferCommandBufferSubmitInfos[i].pNext = NULL;
		transferCommandBufferSubmitInfos[i].commandBuffer = transferCommandBuffers[i];
		transferCommandBufferSubmitInfos[i].deviceMask    = 0;

		computeCommandBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
		computeCommandBufferSubmitInfos[i].pNext = NULL;
		computeCommandBufferSubmitInfos[i].commandBuffer = computeCommandBuffers[i];
		computeCommandBufferSubmitInfos[i].deviceMask    = 0;

		transferWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		transferWaitSemaphoreSubmitInfos[i].pNext = NULL;
		transferWaitSemaphoreSubmitInfos[i].semaphore   = semaphores[i];
		transferWaitSemaphoreSubmitInfos[i].value       = 0;
		transferWaitSemaphoreSubmitInfos[i].stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		transferWaitSemaphoreSubmitInfos[i].deviceIndex = 0;

		transferSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		transferSignalSemaphoreSubmitInfos[i].pNext = NULL;
		transferSignalSemaphoreSubmitInfos[i].semaphore   = semaphores[i];
		transferSignalSemaphoreSubmitInfos[i].value       = 1;
		transferSignalSemaphoreSubmitInfos[i].stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		transferSignalSemaphoreSubmitInfos[i].deviceIndex = 0;

		computeWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		computeWaitSemaphoreSubmitInfos[i].pNext = NULL;
		computeWaitSemaphoreSubmitInfos[i].semaphore   = semaphores[i];
		computeWaitSemaphoreSubmitInfos[i].value       = 1;
		computeWaitSemaphoreSubmitInfos[i].stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		computeWaitSemaphoreSubmitInfos[i].deviceIndex = 0;

		computeSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		computeSignalSemaphoreSubmitInfos[i].pNext = NULL;
		computeSignalSemaphoreSubmitInfos[i].semaphore   = semaphores[i];
		computeSignalSemaphoreSubmitInfos[i].value       = 2;
		computeSignalSemaphoreSubmitInfos[i].stageMask   = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		computeSignalSemaphoreSubmitInfos[i].deviceIndex = 0;

		transferSubmitInfos2[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
		transferSubmitInfos2[i].pNext = NULL;
		transferSubmitInfos2[i].flags                    = 0;
		transferSubmitInfos2[i].waitSemaphoreInfoCount   = 1;
		transferSubmitInfos2[i].pWaitSemaphoreInfos      = &transferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos2[i].commandBufferInfoCount   = 1;
		transferSubmitInfos2[i].pCommandBufferInfos      = &transferCommandBufferSubmitInfos[i];
		transferSubmitInfos2[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos2[i].pSignalSemaphoreInfos    = &transferSignalSemaphoreSubmitInfos[i];

		computeSubmitInfos2[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
		computeSubmitInfos2[i].pNext = NULL;
		computeSubmitInfos2[i].flags                    = 0;
		computeSubmitInfos2[i].waitSemaphoreInfoCount   = 1;
		computeSubmitInfos2[i].pWaitSemaphoreInfos      = &computeWaitSemaphoreSubmitInfos[i];
		computeSubmitInfos2[i].commandBufferInfoCount   = 1;
		computeSubmitInfos2[i].pCommandBufferInfos      = &computeCommandBufferSubmitInfos[i];
		computeSubmitInfos2[i].signalSemaphoreInfoCount = 1;
		computeSubmitInfos2[i].pSignalSemaphoreInfos    = &computeSignalSemaphoreSubmitInfos[i];

		transferSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;
		transferSemaphoreWaitInfos[i].pNext = NULL;
		transferSemaphoreWaitInfos[i].flags          = 0;
		transferSemaphoreWaitInfos[i].semaphoreCount = 1;
		transferSemaphoreWaitInfos[i].pSemaphores    = &semaphores[i];
		transferSemaphoreWaitInfos[i].pValues        = &transferSignalSemaphoreSubmitInfos[i].value;

		computeSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO_KHR;
		computeSemaphoreWaitInfos[i].pNext = NULL;
		computeSemaphoreWaitInfos[i].flags          = 0;
		computeSemaphoreWaitInfos[i].semaphoreCount = 1;
		computeSemaphoreWaitInfos[i].pSemaphores    = &semaphores[i];
		computeSemaphoreWaitInfos[i].pValues        = &computeSignalSemaphoreSubmitInfos[i].value;
	}

	VkCommandBufferSubmitInfoKHR onetimeCommandBufferSubmitInfo;
	onetimeCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
	onetimeCommandBufferSubmitInfo.pNext = NULL;
	onetimeCommandBufferSubmitInfo.commandBuffer = onetimeCommandBuffer;
	onetimeCommandBufferSubmitInfo.deviceMask    = 0;

	VkSubmitInfo2KHR onetimeSubmitInfo2;
	onetimeSubmitInfo2.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
	onetimeSubmitInfo2.pNext = NULL;
	onetimeSubmitInfo2.flags                    = 0;
	onetimeSubmitInfo2.waitSemaphoreInfoCount   = 0;
	onetimeSubmitInfo2.pWaitSemaphoreInfos      = NULL;
	onetimeSubmitInfo2.commandBufferInfoCount   = 1;
	onetimeSubmitInfo2.pCommandBufferInfos      = &onetimeCommandBufferSubmitInfo;
	onetimeSubmitInfo2.signalSemaphoreInfoCount = inoutBuffersPerHeap;
	onetimeSubmitInfo2.pSignalSemaphoreInfos    = transferSignalSemaphoreSubmitInfos;

	SET_MIN_TEST_VALUE(testedValues[0])
	for (uint32_t i = 1; i < inoutBuffersPerHeap; i++)
		testedValues[i] = testedValues[i - 1] + valuesPerInoutBuffer * 2;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		writeInBuffer(mappedHostVisibleInBuffers[i], &testedValues[i], valuesPerInoutBuffer, valuesPerHeap);

	if (usingNonCoherent) {
		GET_RESULT(vkFlushMappedMemoryRanges(g_device, inoutBuffersPerHeap, hostVisibleInBuffersMappedMemoryRanges))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkFlushMappedMemoryRanges, 3, 'p', g_device, 'u', inoutBuffersPerHeap, 'p', hostVisibleInBuffersMappedMemoryRanges)
			free_SubmitCommandsData(data);
			return false;
		}
#endif
	}

	GET_RESULT(vkQueueSubmit2KHR(g_transferQueue, 1, &onetimeSubmitInfo2, VK_NULL_HANDLE))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', g_transferQueue, 'u', 1, 'p', &onetimeSubmitInfo2, 'p', VK_NULL_HANDLE)
		free_SubmitCommandsData(data);
		return false;
	}
#endif

	GET_RESULT(vkQueueSubmit2KHR(g_computeQueue, inoutBuffersPerHeap, computeSubmitInfos2, VK_NULL_HANDLE))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', g_computeQueue, 'u', inoutBuffersPerHeap, 'p', computeSubmitInfos2, 'p', VK_NULL_HANDLE)
		free_SubmitCommandsData(data);
		return false;
	}
#endif

#if END_ON == 1
	atomic_bool input;
	atomic_init(&input, false);

	int threadResult;
	pthread_t waitThread;
	GET_THRD_RESULT(pthread_create(&waitThread, NULL, wait_for_input, &input))
#ifndef NDEBUG
	if (threadResult) {
		PCREATE_FAILURE(&waitThread, NULL, wait_for_input, NULL)
		free_SubmitCommandsData(data);
		return false;
	}
#endif
#endif

	GET_RESULT(vkWaitSemaphoresKHR(g_device, &transferSemaphoreWaitInfos[0], ~0ULL))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkWaitSemaphoresKHR, 3, 'p', g_device, 'p', &transferSemaphoreWaitInfos[0], 'u', ~0ULL)
		free_SubmitCommandsData(data);
		return false;
	}
#endif

	vkDestroyCommandPool(g_device, onetimeCommandPool, g_allocator);
	gpu->onetimeCommandPool = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		writeInBuffer(mappedHostVisibleInBuffers[i], &testedValues[i], valuesPerInoutBuffer, valuesPerHeap);

	if (usingNonCoherent) {
		GET_RESULT(vkFlushMappedMemoryRanges(g_device, inoutBuffersPerHeap, hostVisibleInBuffersMappedMemoryRanges))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkFlushMappedMemoryRanges, 3, 'p', g_device, 'u', inoutBuffersPerHeap, 'p', hostVisibleInBuffersMappedMemoryRanges)
			free_SubmitCommandsData(data);
			return false;
		}
#endif
	}

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++) {
		transferWaitSemaphoreSubmitInfos[i].value   += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	GET_RESULT(vkQueueSubmit2KHR(g_transferQueue, inoutBuffersPerHeap, transferSubmitInfos2, VK_NULL_HANDLE))
#ifndef NDEBUG
	if (result) {
		VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', g_transferQueue, 'u', inoutBuffersPerHeap, 'p', transferSubmitInfos2, 'p', VK_NULL_HANDLE)
		free_SubmitCommandsData(data);
		return false;
	}
#endif

	value_t tested;
	value_t prev;
	SET_MIN_TEST_VALUE(tested)
	SET_MAX_STEP_VALUE(prev)

	value_t num = 0;
	step_t count = 0;
	step_t longest = MAX_STEP_COUNT;

	value_t highestStepValues[256];
	step_t  highestStepCounts[256];

	// ===== Enter main loop =====
#if END_ON == 1
	for (uint64_t i = 0; !atomic_load_explicit(&input, memory_order_acquire); i++) {
#elif END_ON == 2
	for (uint64_t i = 0; i < 30; i++) {
#elif END_ON == 3
	for (uint64_t i = 0; !count; i++) {
#endif

		clock_t mainLoopBmarkStart = clock();
		value_t initialValue = tested;

		float readBmarkTotal                  = 0.0f;
		float writeBmarkTotal                 = 0.0f;
		float waitComputeSemaphoreBmarkTotal  = 0.0f;
		float waitTransferSemaphoreBmarkTotal = 0.0f;
		float computeBmarkTotal               = 0.0f;
		float transferBmarkTotal              = 0.0f;

		printf("Benchmarks #%llu\n", i + 1);

		for (uint32_t j = 0; j < inoutBuffersPerHeap; j++) {
			uint64_t timestamps[2];
			float computeBmark  = 0.0f;
			float transferBmark = 0.0f;

			clock_t waitComputeSemaphoreBmarkStart = clock();
			GET_RESULT(vkWaitSemaphoresKHR(g_device, &computeSemaphoreWaitInfos[j], ~0ULL))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkWaitSemaphoresKHR, 3, 'p', g_device, 'p', &computeSemaphoreWaitInfos[j], 'u', ~0ULL)
				free_SubmitCommandsData(data);
				return false;
			}
#endif
			clock_t waitComputeSemaphoreBmarkEnd = clock();

			if (computeQueueTimestampValidBits) {
				GET_RESULT(vkGetQueryPoolResults(g_device, queryPool, j * 4 + 2, 2, sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT))
#ifndef NDEBUG
				if (result) {
					VULKAN_FAILURE(vkGetQueryPoolResults, 8, 'p', g_device, 'p', queryPool, 'u', j * 4 + 2, 'u', 2, 'u', sizeof(timestamps), 'p', timestamps, 'u', sizeof(timestamps[0]), 'x', VK_QUERY_RESULT_64_BIT)
					free_SubmitCommandsData(data);
					return false;
				}
#endif
				computeBmark = (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			computeWaitSemaphoreSubmitInfos[j].value   += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			GET_RESULT(vkQueueSubmit2KHR(g_computeQueue, 1, &computeSubmitInfos2[j], VK_NULL_HANDLE))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', g_computeQueue, 'u', 1, 'p', &computeSubmitInfos2[j], 'p', VK_NULL_HANDLE)
				free_SubmitCommandsData(data);
				return false;
			}
#endif

			clock_t waitTransferSemaphoreBmarkStart = clock();
			GET_RESULT(vkWaitSemaphoresKHR(g_device, &transferSemaphoreWaitInfos[j], ~0ULL))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkWaitSemaphoresKHR, 3, 'p', g_device, 'p', &transferSemaphoreWaitInfos[j], 'u', ~0ULL)
				free_SubmitCommandsData(data);
				return false;
			}
#endif
			clock_t waitTransferSemaphoreBmarkEnd = clock();

			if (transferQueueTimestampValidBits) {
				GET_RESULT(vkGetQueryPoolResults(g_device, queryPool, j * 4, 2, sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT))
#ifndef NDEBUG
				if (result) {
					VULKAN_FAILURE(vkGetQueryPoolResults, 8, 'p', g_device, 'p', queryPool, 'u', j * 4, 'u', 2, 'u', sizeof(timestamps), 'p', timestamps, 'u', sizeof(timestamps[0]), 'x', VK_QUERY_RESULT_64_BIT)
					free_SubmitCommandsData(data);
					return false;
				}
#endif
				transferBmark = (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (usingNonCoherent) {
				GET_RESULT(vkInvalidateMappedMemoryRanges(g_device, 1, &hostVisibleOutBuffersMappedMemoryRanges[j]))
#ifndef NDEBUG
				if (result) {
					VULKAN_FAILURE(vkInvalidateMappedMemoryRanges, 3, 'p', g_device, 'u', 1, 'p', &hostVisibleOutBuffersMappedMemoryRanges[j])
					free_SubmitCommandsData(data);
					return false;
				}
#endif
			}

			clock_t readBmarkStart = clock();
			readOutBuffer(mappedHostVisibleOutBuffers[j], &tested, highestStepValues, highestStepCounts, &longest, &count, &prev, valuesPerInoutBuffer);
			clock_t readBmarkEnd = clock();

			clock_t writeBmarkStart = clock();
			writeInBuffer(mappedHostVisibleInBuffers[j], &testedValues[j], valuesPerInoutBuffer, valuesPerHeap);
			clock_t writeBmarkEnd = clock();

			if (usingNonCoherent) {
				GET_RESULT(vkFlushMappedMemoryRanges(g_device, 1, &hostVisibleInBuffersMappedMemoryRanges[j]))
#ifndef NDEBUG
				if (result) {
					VULKAN_FAILURE(vkFlushMappedMemoryRanges, 3, 'p', g_device, 'u', 1, 'p', &hostVisibleInBuffersMappedMemoryRanges[j])
					free_SubmitCommandsData(data);
					return false;
				}
#endif
			}

			transferWaitSemaphoreSubmitInfos[j].value   += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			GET_RESULT(vkQueueSubmit2KHR(g_transferQueue, 1, &transferSubmitInfos2[j], VK_NULL_HANDLE))
#ifndef NDEBUG
			if (result) {
				VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', g_transferQueue, 'u', 1, 'p', &transferSubmitInfos2[j], 'p', VK_NULL_HANDLE)
				free_SubmitCommandsData(data);
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
				"\tInout-buffer %u/%u:\n"
				"\t\tReading buffers:      %4.0fms\n"
				"\t\tWriting buffers:      %4.0fms\n"
				"\t\tCompute execution:    %4.0fms\n"
				"\t\tTransfer execution:   %4.0fms\n"
				"\t\tWaiting for compute:  %4.0fms\n"
				"\t\tWaiting for transfer: %4.0fms\n",
				j + 1, inoutBuffersPerHeap,
				(double) readBmark,                 (double) writeBmark,
				(double) computeBmark,              (double) transferBmark,
				(double) waitComputeSemaphoreBmark, (double) waitTransferSemaphoreBmark
			);
		}

		num += valuesPerHeap;
		clock_t mainLoopBmarkEnd = clock();
		float mainLoopBmark      = get_benchmark(mainLoopBmarkStart, mainLoopBmarkEnd);

		printf(
			"\tMain loop: %.0fms\n"
			"\tReading buffers:      (total) %4.0fms, (avg) %6.1fms\n"
			"\tWriting buffers:      (total) %4.0fms, (avg) %6.1fms\n"
			"\tCompute execution:    (total) %4.0fms, (avg) %6.1fms\n"
			"\tTransfer execution:   (total) %4.0fms, (avg) %6.1fms\n"
			"\tWaiting for compute:  (total) %4.0fms, (avg) %6.1fms\n"
			"\tWaiting for transfer: (total) %4.0fms, (avg) %6.1fms\n"
			"\tInitial value: 0x %016llx %016llx\n"
			"\tFinal value:   0x %016llx %016llx\n\n",
			(double) mainLoopBmark,
			(double) readBmarkTotal,                  (double) (readBmarkTotal  / inoutBuffersPerHeap),
			(double) writeBmarkTotal,                 (double) (writeBmarkTotal / inoutBuffersPerHeap),
			(double) computeBmarkTotal,               (double) (computeBmarkTotal  / inoutBuffersPerHeap),
			(double) transferBmarkTotal,              (double) (transferBmarkTotal / inoutBuffersPerHeap),
			(double) waitComputeSemaphoreBmarkTotal,  (double) (waitComputeSemaphoreBmarkTotal  / inoutBuffersPerHeap),
			(double) waitTransferSemaphoreBmarkTotal, (double) (waitTransferSemaphoreBmarkTotal / inoutBuffersPerHeap),
			TOP_128BIT_INT(initialValue), BOTTOM_128BIT_INT(initialValue),
			TOP_128BIT_INT(tested - 2),   BOTTOM_128BIT_INT(tested - 2)
		);
	}
	NEWLINE

	clock_t bmarkEnd = clock();
	float bmark      = get_benchmark(bmarkStart, bmarkEnd);

	printf(
		"Set of starting values tested: [0x %016llx %016llx, 0x %016llx %016llx]\n"
		"Continue on: 0x %016llx %016llx\n"
		"Highest step counts (%hu):\n",
		MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM,
		TOP_128BIT_INT(tested - 2), BOTTOM_128BIT_INT(tested - 2),
		TOP_128BIT_INT(tested),     BOTTOM_128BIT_INT(tested),
		count
	);

	for (uint32_t i = 0; i < count; i++) {
		printf(
			"\t%u)\tsteps(0x %016llx %016llx) = %hu\n",
			i + 1, TOP_128BIT_INT(highestStepValues[i]), BOTTOM_128BIT_INT(highestStepValues[i]), highestStepCounts[i]
		);
	}
	NEWLINE

	printf(
		"Time: %.0fms\n"
		"Speed: %.0f/s\n",
		(double) bmark, 1000 * num / (double) bmark
	);

#if END_ON == 1
	GET_THRD_RESULT(pthread_join(waitThread, NULL))
#ifndef NDEBUG
	if (threadResult) {
		PJOIN_FAILURE(waitThread, NULL)
		free_SubmitCommandsData(data);
		return false;
	}
#endif
#endif

	free_SubmitCommandsData(data);

	END_FUNC
	return true;
}

bool destroy_gpu(Gpu_t* gpu)
{
	BEGIN_FUNC

	const VkDeviceMemory* restrict hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* restrict deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	const VkBuffer*       restrict hostVisibleBuffers        = gpu->hostVisibleBuffers;
	const VkBuffer*       restrict deviceLocalBuffers        = gpu->deviceLocalBuffers;
	const VkSemaphore*    restrict semaphores                = gpu->semaphores;

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

	uint32_t inoutBuffersPerHeap   = gpu->inoutBuffersPerHeap;
	uint32_t buffersPerHeap        = gpu->buffersPerHeap;
	uint32_t deviceMemoriesPerHeap = gpu->deviceMemoriesPerHeap;

	void* dynamicMemory = gpu->dynamicMemory;

	VkResult result;
	size_t writeResult;

	if(pipelineCache) {
		size_t size;
		GET_RESULT(vkGetPipelineCacheData(g_device, pipelineCache, &size, NULL))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkGetPipelineCacheData, 4, 'p', g_device, 'p', pipelineCache, 'p', &size, 'p', NULL)
			return false;
		}
#endif

		void* cache = malloc(size);
#ifndef NDEBUG
		if (!cache) {
			MALLOC_FAILURE(cache)
			return false;
		}
#endif

		GET_RESULT(vkGetPipelineCacheData(g_device, pipelineCache, &size, cache))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkGetPipelineCacheData, 4, 'p', g_device, 'p', pipelineCache, 'p', &size, 'p', cache)
			free(cache);
			return false;
		}
#endif

		FILE* file = fopen(PIPELINE_CACHE_NAME, "wb");
#ifndef NDEBUG
		if (!file) {
			FOPEN_FAILURE(PIPELINE_CACHE_NAME, "wb")
			free(cache);
			return false;
		}
#endif

		GET_WRITE_RESULT(fwrite(cache, 1, size, file))
#ifndef NDEBUG
		if (writeResult != size) {
			FWRITE_FAILURE(cache, 1, size)
			fclose(file);
			free(cache);
			return false;
		}
#endif

		fclose(file);
		free(cache);

		vkDestroyPipelineCache(g_device, pipelineCache, g_allocator);
	}

	if(g_device) {
		vkDestroyDescriptorSetLayout(g_device, descriptorSetLayout, g_allocator);
		vkDestroyShaderModule(g_device, shaderModule, g_allocator);
		vkDestroyPipelineLayout(g_device, pipelineLayout, g_allocator);

		// Make sure no command buffers are in the pending state
		GET_RESULT(vkDeviceWaitIdle(g_device))
#ifndef NDEBUG
		if (result) {
			VULKAN_FAILURE(vkDeviceWaitIdle, 1, 'p', g_device)
			return false;
		}
#endif

		for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
			vkDestroySemaphore(g_device, semaphores[i], g_allocator);

		vkDestroyCommandPool(g_device, onetimeCommandPool, g_allocator);
		vkDestroyCommandPool(g_device, computeCommandPool, g_allocator);
		vkDestroyCommandPool(g_device, transferCommandPool, g_allocator);

		vkDestroyPipeline(g_device, pipeline, g_allocator);
		vkDestroyQueryPool(g_device, queryPool, g_allocator);
		vkDestroyDescriptorPool(g_device, descriptorPool, g_allocator);

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			vkDestroyBuffer(g_device, hostVisibleBuffers[i], g_allocator);
			vkDestroyBuffer(g_device, deviceLocalBuffers[i], g_allocator);
		}

		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			vkFreeMemory(g_device, hostVisibleDeviceMemories[i], g_allocator);
			vkFreeMemory(g_device, deviceLocalDeviceMemories[i], g_allocator);
		}

		vkDestroyDevice(g_device, g_allocator);
	}

	if(g_instance) {
#ifndef NDEBUG
		vkDestroyDebugUtilsMessengerEXT(g_instance, g_debugMessenger, g_allocator);
#endif

		vkDestroyInstance(g_instance, g_allocator);
	}

	volkFinalize();
	free(dynamicMemory);

	END_FUNC
	return true;
}
