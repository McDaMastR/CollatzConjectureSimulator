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

#include <string.h>

static VkDeviceSize min_DeviceSize(const VkDeviceSize a, const VkDeviceSize b)
{
	if (a < b)
		return a;
	return b;
}

static uint32_t min_uint32(const uint32_t a, const uint32_t b)
{
	if (a < b)
		return a;
	return b;
}

static float get_benchmark(const clock_t start, const clock_t end)
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
static void free_CreateInstanceData(const CreateInstanceData_t data)
{
	free(data.properties);
}

bool create_instance(Gpu_t* const gpu)
{
	BEGIN_FUNC

	CreateInstanceData_t data = {0};
	VkResult result;
	bool initResult;

	result = volkInitialize();
	if (result != VK_SUCCESS) {
		VINIT_FAILURE()
		free_CreateInstanceData(data);
		return false;
	}

	const uint32_t appApiVersion = VK_API_VERSION_1_2;
	const uint32_t instApiVersion = volkGetInstanceVersion();

	if (VK_API_VERSION_VARIANT(instApiVersion) != VK_API_VERSION_VARIANT(appApiVersion) ||
		VK_API_VERSION_MAJOR(instApiVersion)    < VK_API_VERSION_MAJOR(appApiVersion)   ||
		VK_API_VERSION_MINOR(instApiVersion)    < VK_API_VERSION_MINOR(appApiVersion)) {
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
#endif // NDEBUG

	VkAllocationCallbacks allocationCallbacks;
	allocationCallbacks.pUserData             = &gpu->allocationCallbackCounts;
	allocationCallbacks.pfnAllocation         = allocation_callback;
	allocationCallbacks.pfnReallocation       = reallocation_callback;
	allocationCallbacks.pfnFree               = free_callback;
	allocationCallbacks.pfnInternalAllocation = internal_allocation_callback;
	allocationCallbacks.pfnInternalFree       = internal_free_callback;

	gpu->allocationCallbacks = allocationCallbacks;
	gpu->allocator = &gpu->allocationCallbacks;
#endif // LOG_VULKAN_ALLOCATIONS
	const VkAllocationCallbacks* const allocator = gpu->allocator;

#ifndef NDEBUG
	GET_INIT_RESULT(init_debug_logfile())
	if (!initResult) {
		free_CreateInstanceData(data);
		return false;
	}

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo;
	debugUtilsMessengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debugUtilsMessengerCreateInfo.pNext = NULL;
	debugUtilsMessengerCreateInfo.flags = 0;
	debugUtilsMessengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback = debug_callback;
	debugUtilsMessengerCreateInfo.pUserData = &gpu->debugCallbackCount;
#endif // NDEBUG

	uint32_t layerPropertyCount;
	GET_RESULT(vkEnumerateInstanceLayerProperties(&layerPropertyCount, NULL))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEnumerateInstanceLayerProperties, 2, 'p', &layerPropertyCount, 'p', NULL)
		free_CreateInstanceData(data);
		return false;
	}
#endif // NDEBUG

	uint32_t extensionPropertyCount;
	GET_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertyCount, NULL))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEnumerateInstanceExtensionProperties, 3, 'p', NULL, 'p', &extensionPropertyCount, 'p', NULL)
		free_CreateInstanceData(data);
		return false;
	}
#endif // NDEBUG

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
#endif // NDEBUG

	VkLayerProperties*     const layersProperties     = (VkLayerProperties*) data.properties;
	VkExtensionProperties* const extensionsProperties = (VkExtensionProperties*) (layersProperties + layerPropertyCount);

	GET_RESULT(vkEnumerateInstanceLayerProperties(&layerPropertyCount, layersProperties))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEnumerateInstanceLayerProperties, 2, 'p', &layerPropertyCount, 'p', layersProperties)
		free_CreateInstanceData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkEnumerateInstanceExtensionProperties(NULL, &extensionPropertyCount, extensionsProperties))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEnumerateInstanceExtensionProperties, 3, 'p', NULL, 'p', &extensionPropertyCount, 'p', extensionsProperties)
		free_CreateInstanceData(data);
		return false;
	}
#endif // NDEBUG

	uint32_t enabledLayerCount = 0;
	const char* enabledLayers[1];
	for (uint32_t i = 0; i < layerPropertyCount; i++) {
#ifndef NDEBUG
		if (strcmp(layersProperties[i].layerName, VK_LAYER_KHRONOS_VALIDATION_LAYER_NAME) == 0) {
			enabledLayers[enabledLayerCount] = layersProperties[i].layerName;
			enabledLayerCount++;
			continue;
		}
#endif // NDEBUG
	}

	const void* pNextChain = NULL;
	const void** next = &pNextChain;

	uint32_t enabledExtensionCount = 0;
	const char* enabledExtensions[2];

	bool khrPortabilityEnumeration = false;
	bool extDebugUtils = false;
	for (uint32_t i = 0; i < extensionPropertyCount; i++) {
		if (strcmp(extensionsProperties[i].extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME) == 0) {
			khrPortabilityEnumeration = true;
			enabledExtensions[enabledExtensionCount] = extensionsProperties[i].extensionName;
			enabledExtensionCount++;
			continue;
		}

#ifndef NDEBUG
		if (strcmp(extensionsProperties[i].extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
			extDebugUtils = true;
			enabledExtensions[enabledExtensionCount] = extensionsProperties[i].extensionName;
			enabledExtensionCount++;

			*next = &debugUtilsMessengerCreateInfo;
			next = &debugUtilsMessengerCreateInfo.pNext;
			continue;
		}
#endif // NDEBUG
	}

	VkApplicationInfo applicationInfo;
	applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	applicationInfo.pNext = NULL;
	applicationInfo.pApplicationName = PROGRAM_NAME;
	applicationInfo.applicationVersion = 0;
	applicationInfo.pEngineName = NULL;
	applicationInfo.engineVersion = 0;
	applicationInfo.apiVersion = appApiVersion;

	VkInstanceCreateInfo instanceCreateInfo;
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pNext = pNextChain;
	instanceCreateInfo.flags = khrPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.enabledLayerCount = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames = enabledLayers;
	instanceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions;

	printf("Enabled instance layers (%u):\n", enabledLayerCount);
	for (uint32_t i = 0; i < enabledLayerCount; i++)
		printf("\tLayer %u: %s\n", i + 1, enabledLayers[i]);
	NEWLINE

	printf("Enabled instance extensions (%u):\n", enabledExtensionCount);
	for (uint32_t i = 0; i < enabledExtensionCount; i++)
		printf("\tExtension %u: %s\n", i + 1, enabledExtensions[i]);
	NEWLINE

	VkInstance instance;
	GET_RESULT(vkCreateInstance(&instanceCreateInfo, allocator, &instance))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateInstance, 3, 'p', &instanceCreateInfo, 'p', allocator, 'p', &instance)
		free_CreateInstanceData(data);
		return false;
	}
#endif // NDEBUG

	volkLoadInstanceOnly(instance);
	free_CreateInstanceData(data);

#ifndef NDEBUG
	if (extDebugUtils) {
		debugUtilsMessengerCreateInfo.pNext = NULL;
		GET_RESULT(vkCreateDebugUtilsMessengerEXT(instance, &debugUtilsMessengerCreateInfo, allocator, &gpu->debugMessenger))
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkCreateDebugUtilsMessengerEXT, 4, 'p', instance, 'p', &debugUtilsMessengerCreateInfo, 'p', allocator, 'p', &gpu->debugMessenger)
			return false;
		}
	}
#endif // NDEBUG

	END_FUNC
	return true;
}

typedef struct CreateDeviceData
{
	void* devices;
	void* properties;
} CreateDeviceData_t;

static void free_CreateDeviceData(const CreateDeviceData_t data)
{
	free(data.devices);
	free(data.properties);
}

bool create_device(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkInstance instance = volkGetLoadedInstance();
	const VkAllocationCallbacks* const allocator = gpu->allocator;

	CreateDeviceData_t data = {0};
	VkResult result;

	uint32_t physicalDeviceCount;
	GET_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEnumeratePhysicalDevices, 3, 'p', instance, 'p', &physicalDeviceCount, 'p', NULL)
		free_CreateDeviceData(data);
		return false;
	}

	if (!physicalDeviceCount) {
		fprintf(stderr,
			"Device failure at line %d\n"
			"Function call 'vkEnumeratePhysicalDevices' returned *pPhysicalDeviceCount = %u\n\n",
			__LINE__, physicalDeviceCount
		);

		free_CreateDeviceData(data);
		return false;
	}
#endif // NDEBUG

	size_t size =
		physicalDeviceCount *     sizeof(VkPhysicalDevice) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceProperties2) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceFeatures2) +
		physicalDeviceCount *     sizeof(VkPhysicalDevice16BitStorageFeatures) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceMaintenance4FeaturesKHR) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceMemoryProperties2) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceMemoryPriorityFeaturesEXT) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceSynchronization2FeaturesKHR) +
		physicalDeviceCount *     sizeof(VkPhysicalDeviceTimelineSemaphoreFeatures) +
		physicalDeviceCount *     sizeof(VkExtensionProperties*) +
		physicalDeviceCount *     sizeof(VkQueueFamilyProperties2*) +
		physicalDeviceCount * 2 * sizeof(uint32_t) +
		physicalDeviceCount * 5 * sizeof(bool);

	data.devices = malloc(size);
#ifndef NDEBUG
	if (!data.devices) {
		MALLOC_FAILURE(data.devices)
		free_CreateDeviceData(data);
		return false;
	}
#endif // NDEBUG

	VkPhysicalDevice* const physicalDevices = (VkPhysicalDevice*) data.devices;

	VkExtensionProperties**    const extensionsProperties    = (VkExtensionProperties**)    (physicalDevices      + physicalDeviceCount);
	VkQueueFamilyProperties2** const queueFamiliesProperties = (VkQueueFamilyProperties2**) (extensionsProperties + physicalDeviceCount);

	VkPhysicalDeviceProperties2* const physicalDevicesProperties = (VkPhysicalDeviceProperties2*) (queueFamiliesProperties   + physicalDeviceCount);
	VkPhysicalDeviceFeatures2*   const physicalDevicesFeatures   = (VkPhysicalDeviceFeatures2*)   (physicalDevicesProperties + physicalDeviceCount);

	VkPhysicalDevice16BitStorageFeatures*    const physicalDevices16BitStorageFeatures = (VkPhysicalDevice16BitStorageFeatures*)    (physicalDevicesFeatures             + physicalDeviceCount);
	VkPhysicalDeviceMaintenance4FeaturesKHR* const physicalDevicesMaintenance4Features = (VkPhysicalDeviceMaintenance4FeaturesKHR*) (physicalDevices16BitStorageFeatures + physicalDeviceCount);

	VkPhysicalDeviceMemoryProperties2*         const physicalDevicesMemoryProperties       = (VkPhysicalDeviceMemoryProperties2*)         (physicalDevicesMaintenance4Features + physicalDeviceCount);
	VkPhysicalDeviceMemoryPriorityFeaturesEXT* const physicalDevicesMemoryPriorityFeatures = (VkPhysicalDeviceMemoryPriorityFeaturesEXT*) (physicalDevicesMemoryProperties     + physicalDeviceCount);

	VkPhysicalDeviceSynchronization2FeaturesKHR* const physicalDevicesSynchronization2Features  = (VkPhysicalDeviceSynchronization2FeaturesKHR*) (physicalDevicesMemoryPriorityFeatures   + physicalDeviceCount);
	VkPhysicalDeviceTimelineSemaphoreFeatures*   const physicalDevicesTimelineSemaphoreFeatures = (VkPhysicalDeviceTimelineSemaphoreFeatures*)   (physicalDevicesSynchronization2Features + physicalDeviceCount);

	uint32_t* const extensionPropertyCounts   = (uint32_t*) (physicalDevicesTimelineSemaphoreFeatures + physicalDeviceCount);
	uint32_t* const queueFamilyPropertyCounts = (uint32_t*) (extensionPropertyCounts                  + physicalDeviceCount);

	bool* const haveMaintenance4      = (bool*) (queueFamilyPropertyCounts + physicalDeviceCount);
	bool* const haveSynchronization2  = (bool*) (haveMaintenance4          + physicalDeviceCount);
	bool* const havePortabilitySubset = (bool*) (haveSynchronization2      + physicalDeviceCount);
	bool* const haveMemoryBudget      = (bool*) (havePortabilitySubset     + physicalDeviceCount);
	bool* const haveMemoryPriority    = (bool*) (haveMemoryBudget          + physicalDeviceCount);

	GET_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEnumeratePhysicalDevices, 3, 'p', instance, 'p', &physicalDeviceCount, 'p', physicalDevices)
		free_CreateDeviceData(data);
		return false;
	}
#endif // NDEBUG

	memset(haveMaintenance4,      0, physicalDeviceCount * sizeof(bool));
	memset(haveSynchronization2,  0, physicalDeviceCount * sizeof(bool));
	memset(havePortabilitySubset, 0, physicalDeviceCount * sizeof(bool));
	memset(haveMemoryBudget,      0, physicalDeviceCount * sizeof(bool));
	memset(haveMemoryPriority,    0, physicalDeviceCount * sizeof(bool));

	size_t extensionTotal = 0;
	size_t queueFamilyTotal = 0;
	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCounts[i], NULL);
		GET_RESULT(vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &extensionPropertyCounts[i], NULL))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkEnumerateDeviceExtensionProperties, 4, 'p', physicalDevices[i], 'p', NULL, 'p', &extensionPropertyCounts[i], 'p', NULL)
			free_CreateDeviceData(data);
			return false;
		}
#endif // NDEBUG

		queueFamilyTotal += queueFamilyPropertyCounts[i];
		extensionTotal += extensionPropertyCounts[i];
	}

	size =
		queueFamilyTotal * sizeof(VkQueueFamilyProperties2) +
		extensionTotal   * sizeof(VkExtensionProperties);

	data.properties = malloc(size);
#ifndef NDEBUG
	if (!data.properties) {
		MALLOC_FAILURE(data.properties)
		free_CreateDeviceData(data);
		return false;
	}
#endif // NDEBUG

	queueFamiliesProperties[0] = (VkQueueFamilyProperties2*) data.properties;
	extensionsProperties[0]    = (VkExtensionProperties*) (queueFamiliesProperties[0] + queueFamilyTotal);

	for (uint32_t i = 1; i < physicalDeviceCount; i++) {
		extensionsProperties[i] = extensionsProperties[i - 1] + extensionPropertyCounts[i - 1];
		queueFamiliesProperties[i] = queueFamiliesProperties[i - 1] + queueFamilyPropertyCounts[i - 1];
	}

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		GET_RESULT(vkEnumerateDeviceExtensionProperties(physicalDevices[i], NULL, &extensionPropertyCounts[i], extensionsProperties[i]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkEnumerateDeviceExtensionProperties, 4, 'p', physicalDevices[i], 'p', NULL, 'p', &extensionPropertyCounts[i], 'p', extensionsProperties[i])
			free_CreateDeviceData(data);
			return false;
		}
#endif // NDEBUG

		for (uint32_t j = 0; j < extensionPropertyCounts[i]; j++) {
			const char* const extensionName = extensionsProperties[i][j].extensionName;

			if (strcmp(extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME) == 0) {
				haveMaintenance4[i] = true;
				continue;
			}

			if (strcmp(extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME) == 0) {
				haveSynchronization2[i] = true;
				continue;
			}

			if (strcmp(extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME) == 0) {
				havePortabilitySubset[i] = true;
				continue;
			}

			if (strcmp(extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME) == 0) {
				haveMemoryBudget[i] = true;
				continue;
			}

			if (strcmp(extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME) == 0) {
				haveMemoryPriority[i] = true;
				continue;
			}
		}

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++) {
			queueFamiliesProperties[i][j].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2;
			queueFamiliesProperties[i][j].pNext = NULL;
		}

		vkGetPhysicalDeviceQueueFamilyProperties2(physicalDevices[i], &queueFamilyPropertyCounts[i], queueFamiliesProperties[i]);

		physicalDevicesProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		physicalDevicesProperties[i].pNext = NULL;

		vkGetPhysicalDeviceProperties2(physicalDevices[i], &physicalDevicesProperties[i]);

		physicalDevicesMemoryProperties[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
		physicalDevicesMemoryProperties[i].pNext = NULL;

		vkGetPhysicalDeviceMemoryProperties2(physicalDevices[i], &physicalDevicesMemoryProperties[i]);

		physicalDevicesFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
		void** next = &physicalDevicesFeatures[i].pNext;

		physicalDevices16BitStorageFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES;
		*next = &physicalDevices16BitStorageFeatures[i];
		next = &physicalDevices16BitStorageFeatures[i].pNext;

		physicalDevicesTimelineSemaphoreFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES;
		*next = &physicalDevicesTimelineSemaphoreFeatures[i];
		next = &physicalDevicesTimelineSemaphoreFeatures[i].pNext;

		if (haveMaintenance4[i]) {
			physicalDevicesMaintenance4Features[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR;
			*next = &physicalDevicesMaintenance4Features[i];
			next = &physicalDevicesMaintenance4Features[i].pNext;
		}

		if (haveSynchronization2[i]) {
			physicalDevicesSynchronization2Features[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
			*next = &physicalDevicesSynchronization2Features[i];
			next = &physicalDevicesSynchronization2Features[i].pNext;
		}

		if (haveMemoryPriority[i]) {
			physicalDevicesMemoryPriorityFeatures[i].sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT;
			*next = &physicalDevicesMemoryPriorityFeatures[i];
			next = &physicalDevicesMemoryPriorityFeatures[i].pNext;
		}

		*next = NULL;
		vkGetPhysicalDeviceFeatures2(physicalDevices[i], &physicalDevicesFeatures[i]);
	}

	// Choose physical device
	uint32_t physicalDeviceIndex = UINT32_MAX;
	uint32_t highestScore = 0;

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		if (VK_API_VERSION_MINOR(physicalDevicesProperties[i].properties.apiVersion) < 2) continue;
		if (physicalDevicesFeatures[i].features.shaderInt16                 == VK_FALSE) continue;
		if (physicalDevices16BitStorageFeatures[i].storageBuffer16BitAccess == VK_FALSE) continue;
		if (!haveMaintenance4[i]    ) continue;
		if (!haveSynchronization2[i]) continue;

		bool hasDedicatedDeviceLocal = false;
		bool hasHostCachedNonCoherent = false;
		bool hasHostCached = false;
		bool hasHostNonCoherent = false;

		bool hasDedicatedTransfer = false;
		bool hasDedicatedCompute = false;

		for (uint32_t j = 0; j < physicalDevicesMemoryProperties[i].memoryProperties.memoryTypeCount; j++) {
			const VkMemoryPropertyFlags propertyFlags = physicalDevicesMemoryProperties[i].memoryProperties.memoryTypes[j].propertyFlags;

			if (propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && !(propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))
				hasDedicatedDeviceLocal = true;

			if (propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT && !(propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				hasHostCachedNonCoherent = true;

			if (propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
				hasHostCached = true;

			if (!(propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))
				hasHostNonCoherent = true;
		}

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++) {
			const VkQueueFlags queueFlags = queueFamiliesProperties[i][j].queueFamilyProperties.queueFlags;

			if (queueFlags == VK_QUEUE_TRANSFER_BIT)
				hasDedicatedTransfer = true;

			if (queueFlags == VK_QUEUE_COMPUTE_BIT)
				hasDedicatedCompute = true;
		}

		uint32_t currentScore = 1;

		if (physicalDevicesProperties[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			currentScore += 10000;

		if (hasDedicatedDeviceLocal)
			currentScore += 1000;
		if (hasHostCachedNonCoherent)
			currentScore += 1000;
		else if (hasHostCached)
			currentScore += 500;
		else if (hasHostNonCoherent)
			currentScore += 100;

		if (hasDedicatedTransfer)
			currentScore += 100;
		if (hasDedicatedCompute)
			currentScore += 50;

		if (haveMemoryBudget[i])
			currentScore += 10;
		if (haveMemoryPriority[i])
			currentScore += 10;

		if (currentScore > highestScore)
			physicalDeviceIndex = i;
	}

	if (physicalDeviceIndex == UINT32_MAX) {
		fprintf(stderr, "Vulkan failure\nNo physical device meets requirements of program\n\n");
		free_CreateDeviceData(data);
		return false;
	}

	const VkPhysicalDevice physicalDevice = physicalDevices[physicalDeviceIndex];
	gpu->physicalDevice = physicalDevice;

	// Enable available extensions
	uint32_t enabledExtensionCount = 0;
	const char* enabledExtensions[5];

	enabledExtensions[enabledExtensionCount] = VK_KHR_MAINTENANCE_4_EXTENSION_NAME;
	enabledExtensionCount++;

	enabledExtensions[enabledExtensionCount] = VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
	enabledExtensionCount++;

	if (havePortabilitySubset[physicalDeviceIndex]) {
		enabledExtensions[enabledExtensionCount] = VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	if (haveMemoryBudget[physicalDeviceIndex]) {
		enabledExtensions[enabledExtensionCount] = VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	if (haveMemoryPriority[physicalDeviceIndex]) {
		enabledExtensions[enabledExtensionCount] = VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME;
		enabledExtensionCount++;
	}

	// Disable unnecessary features
	physicalDevicesFeatures[physicalDeviceIndex].features.robustBufferAccess = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.fullDrawIndexUint32 = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.imageCubeArray = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.independentBlend = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.geometryShader = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.tessellationShader = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sampleRateShading = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.dualSrcBlend = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.logicOp = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.multiDrawIndirect = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.drawIndirectFirstInstance = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.depthClamp = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.depthBiasClamp = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.fillModeNonSolid = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.depthBounds = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.wideLines = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.largePoints = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.alphaToOne = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.multiViewport = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.samplerAnisotropy = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.textureCompressionETC2 = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.textureCompressionASTC_LDR = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.textureCompressionBC = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.occlusionQueryPrecise = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.pipelineStatisticsQuery = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.vertexPipelineStoresAndAtomics = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.fragmentStoresAndAtomics = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderTessellationAndGeometryPointSize = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderImageGatherExtended = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderStorageImageExtendedFormats = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderStorageImageMultisample = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderStorageImageReadWithoutFormat = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderStorageImageWriteWithoutFormat = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderUniformBufferArrayDynamicIndexing = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderSampledImageArrayDynamicIndexing = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderStorageBufferArrayDynamicIndexing = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderStorageImageArrayDynamicIndexing = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderClipDistance = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderCullDistance = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderFloat64 = VK_FALSE;
	// physicalDevicesFeatures[physicalDeviceIndex].features.shaderInt64; // Enable if available
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderInt16 = VK_TRUE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderResourceResidency = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.shaderResourceMinLod = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseBinding = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidencyBuffer = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidencyImage2D = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidencyImage3D = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidency2Samples = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidency4Samples = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidency8Samples = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidency16Samples = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.sparseResidencyAliased = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.variableMultisampleRate = VK_FALSE;
	physicalDevicesFeatures[physicalDeviceIndex].features.inheritedQueries = VK_FALSE;

	physicalDevices16BitStorageFeatures[physicalDeviceIndex].storageBuffer16BitAccess = VK_TRUE;
	physicalDevices16BitStorageFeatures[physicalDeviceIndex].uniformAndStorageBuffer16BitAccess = VK_FALSE;
	physicalDevices16BitStorageFeatures[physicalDeviceIndex].storagePushConstant16 = VK_FALSE;
	physicalDevices16BitStorageFeatures[physicalDeviceIndex].storageInputOutput16 = VK_FALSE;

	physicalDevicesMaintenance4Features[physicalDeviceIndex].maintenance4 = VK_TRUE;

	physicalDevicesSynchronization2Features[physicalDeviceIndex].synchronization2 = VK_TRUE;

	physicalDevicesTimelineSemaphoreFeatures[physicalDeviceIndex].timelineSemaphore = VK_TRUE;

	if (haveMemoryPriority[physicalDeviceIndex])
		physicalDevicesMemoryPriorityFeatures[physicalDeviceIndex].memoryPriority = VK_TRUE;

	// Choose memory types and heaps
	bool hasDedicatedDeviceLocal = false;
	bool hasDeviceLocal = false;
	bool hasHostCachedNonCoherent = false;
	bool hasHostCached = false;
	bool hasHostNonCoherent = false;
	bool hasHostVisible = false;

	const uint32_t memoryTypeCount = physicalDevicesMemoryProperties[physicalDeviceIndex].memoryProperties.memoryTypeCount;
	for (uint32_t i = 0; i < memoryTypeCount; i++) {
		const VkMemoryType memoryType = physicalDevicesMemoryProperties[physicalDeviceIndex].memoryProperties.memoryTypes[i];

		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && !(memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) && !hasDedicatedDeviceLocal) {
			hasDedicatedDeviceLocal = true;
			hasDeviceLocal = true;
			gpu->deviceLocalMemoryTypeIndex = i;
			gpu->deviceLocalMemoryHeapIndex = memoryType.heapIndex;
		}

		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && !hasDeviceLocal) {
			hasDeviceLocal = true;
			gpu->deviceLocalMemoryTypeIndex = i;
			gpu->deviceLocalMemoryHeapIndex = memoryType.heapIndex;
		}

		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT && !(memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && !hasHostCachedNonCoherent) {
			hasHostCachedNonCoherent = true;
			hasHostCached = true;
			hasHostNonCoherent = true;
			hasHostVisible = true;
			gpu->hostVisibleMemoryTypeIndex = i;
			gpu->hostVisibleMemoryHeapIndex = memoryType.heapIndex;
		}

		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT && !hasHostCached) {
			hasHostCached = true;
			hasHostNonCoherent = true;
			hasHostVisible = true;
			gpu->hostVisibleMemoryTypeIndex = i;
			gpu->hostVisibleMemoryHeapIndex = memoryType.heapIndex;
		}

		if (!(memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) && !hasHostNonCoherent) {
			hasHostNonCoherent = true;
			hasHostVisible = true;
			gpu->hostVisibleMemoryTypeIndex = i;
			gpu->hostVisibleMemoryHeapIndex = memoryType.heapIndex;
		}

		if (memoryType.propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT && !hasHostVisible) {
			hasHostVisible = true;
			gpu->hostVisibleMemoryTypeIndex = i;
			gpu->hostVisibleMemoryHeapIndex = memoryType.heapIndex;
		}
	}

	// Choose queue families
	const uint32_t queueFamilyPropertyCount = queueFamilyPropertyCounts[physicalDeviceIndex];
	bool hasDedicatedCompute = false;
	bool hasCompute = false;
	bool hasDedicatedTransfer = false;
	bool hasTransfer = false;

	for (uint32_t i = 0; i < queueFamilyPropertyCount; i++) {
		const VkQueueFlags queueFlags = queueFamiliesProperties[physicalDeviceIndex][i].queueFamilyProperties.queueFlags;

		if (queueFlags == VK_QUEUE_COMPUTE_BIT && !hasDedicatedCompute) {
			hasDedicatedCompute = true;
			hasCompute = true;
			gpu->computeQueueFamilyIndex = i;
		}

		if (queueFlags & VK_QUEUE_COMPUTE_BIT && !hasCompute) {
			hasCompute = true;
			gpu->computeQueueFamilyIndex = i;
		}

		if (queueFlags == VK_QUEUE_TRANSFER_BIT && !hasDedicatedTransfer) {
			hasDedicatedTransfer = true;
			hasTransfer = true;
			gpu->transferQueueFamilyIndex = i;
		}

		if (queueFlags & VK_QUEUE_TRANSFER_BIT && !hasTransfer) {
			hasTransfer = true;
			gpu->transferQueueFamilyIndex = i;
		}
	}

	if (!hasTransfer)
		gpu->transferQueueFamilyIndex = gpu->computeQueueFamilyIndex;

	const uint32_t computeQueueFamilyIndex = gpu->computeQueueFamilyIndex;
	const uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;

	gpu->usingShaderInt64 = physicalDevicesFeatures[physicalDeviceIndex].features.shaderInt64;
	gpu->usingMemoryBudget = haveMemoryBudget[physicalDeviceIndex];
	gpu->usingMemoryPriority = haveMemoryPriority[physicalDeviceIndex];

#if QUERY_BENCHMARKING
	gpu->transferQueueTimestampValidBits = queueFamiliesProperties[physicalDeviceIndex][transferQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
	gpu->computeQueueTimestampValidBits  = queueFamiliesProperties[physicalDeviceIndex][computeQueueFamilyIndex ].queueFamilyProperties.timestampValidBits;
	gpu->timestampPeriod = physicalDevicesProperties[physicalDeviceIndex].properties.limits.timestampPeriod;
#endif // QUERY_BENCHMARKING

	// CPU spends much more time waiting for compute operations than transfer operations
	// So compute queue has higher priority to potentially reduce this wait time
	const float computeQueuePriority = 1.0f;
	const float transferQueuePriority = 0.0f;

	VkDeviceQueueCreateInfo queueCreateInfos[2];
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].pNext = NULL;
	queueCreateInfos[0].flags = 0;
	queueCreateInfos[0].queueFamilyIndex = computeQueueFamilyIndex;
	queueCreateInfos[0].queueCount = 1;
	queueCreateInfos[0].pQueuePriorities = &computeQueuePriority;

	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].pNext = NULL;
	queueCreateInfos[1].flags = 0;
	queueCreateInfos[1].queueFamilyIndex = transferQueueFamilyIndex;
	queueCreateInfos[1].queueCount = 1;
	queueCreateInfos[1].pQueuePriorities = &transferQueuePriority;

	VkDeviceCreateInfo deviceCreateInfo;
	deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceCreateInfo.pNext = &physicalDevicesFeatures[physicalDeviceIndex];
	deviceCreateInfo.flags = 0;
	deviceCreateInfo.queueCreateInfoCount = computeQueueFamilyIndex == transferQueueFamilyIndex ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = NULL;
	deviceCreateInfo.enabledExtensionCount = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensions;
	deviceCreateInfo.pEnabledFeatures = NULL;

	printf(
		"Device: %s\n"
		"\tTransfer QF index: %u\n"
		"\tCompute QF index:  %u\n"
		"\tDL type index:     %u\n"
		"\tHV type index:     %u\n"
		"\tDL heap index:     %u\n"
		"\tHV heap index:     %u\n"
		"\tshaderInt64:       %d\n"
		"\tmemoryPriority:    %d\n\n",
		physicalDevicesProperties[physicalDeviceIndex].properties.deviceName,
		transferQueueFamilyIndex,
		computeQueueFamilyIndex,
		gpu->deviceLocalMemoryTypeIndex,
		gpu->hostVisibleMemoryTypeIndex,
		gpu->deviceLocalMemoryHeapIndex,
		gpu->hostVisibleMemoryHeapIndex,
		gpu->usingShaderInt64,
		gpu->usingMemoryPriority
	);

	printf("Enabled device extensions (%u):\n", enabledExtensionCount);
	for (uint32_t i = 0; i < enabledExtensionCount; i++)
		printf("\tExtension %u: %s\n", i + 1, enabledExtensions[i]);
	NEWLINE

	VkDevice device;
	GET_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, allocator, &device))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateDevice, 4, 'p', physicalDevice, 'p', &deviceCreateInfo, 'p', allocator, 'p', &device)
		free_CreateDeviceData(data);
		return false;
	}
#endif // NDEBUG

	volkLoadDevice(device);
	free_CreateDeviceData(data);

	VkDeviceQueueInfo2 transferQueueInfo;
	transferQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	transferQueueInfo.pNext = NULL;
	transferQueueInfo.flags = 0;
	transferQueueInfo.queueFamilyIndex = transferQueueFamilyIndex;
	transferQueueInfo.queueIndex = 0;

	VkDeviceQueueInfo2 computeQueueInfo;
	computeQueueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2;
	computeQueueInfo.pNext = NULL;
	computeQueueInfo.flags = 0;
	computeQueueInfo.queueFamilyIndex = computeQueueFamilyIndex;
	computeQueueInfo.queueIndex = 0;

	vkGetDeviceQueue2(device, &transferQueueInfo, &gpu->transferQueue);
	vkGetDeviceQueue2(device, &computeQueueInfo, &gpu->computeQueue);

#ifndef NDEBUG
	if(gpu->debugMessenger) {
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;
		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_QUEUE;

		if (gpu->transferQueue == gpu->computeQueue) {
			debugUtilsObjectNameInfo.objectHandle = (uint64_t) gpu->transferQueue;
			debugUtilsObjectNameInfo.pObjectName = "Transfer & compute queue";
			SET_DEBUG_NAME()
		}
		else {
			debugUtilsObjectNameInfo.objectHandle = (uint64_t) gpu->transferQueue;
			debugUtilsObjectNameInfo.pObjectName = "Transfer queue";
			SET_DEBUG_NAME()

			debugUtilsObjectNameInfo.objectHandle = (uint64_t) gpu->computeQueue;
			debugUtilsObjectNameInfo.pObjectName = "Compute queue";
			SET_DEBUG_NAME()
		}
	}
#endif // NDEBUG

	END_FUNC
	return true;
}

bool manage_memory(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkDevice device = volkGetLoadedDevice();
	const VkPhysicalDevice physicalDevice = gpu->physicalDevice;
	const uint32_t hostVisibleMemoryHeapIndex = gpu->hostVisibleMemoryHeapIndex;
	const uint32_t deviceLocalMemoryHeapIndex = gpu->deviceLocalMemoryHeapIndex;
	const bool usingMemoryBudget = gpu->usingMemoryBudget;

	VkPhysicalDeviceMaintenance3Properties physicalDeviceMaintenance3Properties;
	physicalDeviceMaintenance3Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES;
	physicalDeviceMaintenance3Properties.pNext = NULL;

	VkPhysicalDeviceMaintenance4PropertiesKHR physicalDeviceMaintenance4Properties;
	physicalDeviceMaintenance4Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR;
	physicalDeviceMaintenance4Properties.pNext = &physicalDeviceMaintenance3Properties;

	VkPhysicalDeviceProperties2 physicalDeviceProperties;
	physicalDeviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
	physicalDeviceProperties.pNext = &physicalDeviceMaintenance4Properties;

	vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT physicalDeviceMemoryBudgetProperties;
	physicalDeviceMemoryBudgetProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT;
	physicalDeviceMemoryBudgetProperties.pNext = NULL;

	VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties;
	physicalDeviceMemoryProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2;
	physicalDeviceMemoryProperties.pNext = usingMemoryBudget ? &physicalDeviceMemoryBudgetProperties : NULL;

	vkGetPhysicalDeviceMemoryProperties2(physicalDevice, &physicalDeviceMemoryProperties);

	const VkDeviceSize inBufferOffsetAlignment = physicalDeviceProperties.properties.limits.minStorageBufferOffsetAlignment;
	const VkDeviceSize maxMemoryAllocationSize = physicalDeviceMaintenance3Properties.maxMemoryAllocationSize;
	const VkDeviceSize maxBufferSize = physicalDeviceMaintenance4Properties.maxBufferSize;
	const uint32_t maxOutBufferRange = physicalDeviceProperties.properties.limits.maxStorageBufferRange;
	const uint32_t maxMemoryAllocationCount = physicalDeviceProperties.properties.limits.maxMemoryAllocationCount;
	const uint32_t maxComputeWorkGroupCount = physicalDeviceProperties.properties.limits.maxComputeWorkGroupCount[0];
	const uint32_t maxComputeWorkGroupSize = physicalDeviceProperties.properties.limits.maxComputeWorkGroupSize[0];

#if IN_BUFFER_TYPE == 1
	const VkDeviceSize outBufferOffsetAlignment = physicalDeviceProperties.properties.limits.minStorageBufferOffsetAlignment;
	const uint32_t maxInBufferRange = physicalDeviceProperties.properties.limits.maxStorageBufferRange;
#elif IN_BUFFER_TYPE == 2
	const VkDeviceSize outBufferOffsetAlignment = physicalDeviceProperties.properties.limits.minUniformBufferOffsetAlignment;
	const uint32_t maxInBufferRange = physicalDeviceProperties.properties.limits.maxUniformBufferRange;
#endif // IN_BUFFER_TYPE

	VkDeviceSize bytesPerHostVisibleHeap;
	VkDeviceSize bytesPerDeviceLocalHeap;
	if (usingMemoryBudget) {
		bytesPerHostVisibleHeap = physicalDeviceMemoryBudgetProperties.heapBudget[hostVisibleMemoryHeapIndex];
		bytesPerDeviceLocalHeap = physicalDeviceMemoryBudgetProperties.heapBudget[deviceLocalMemoryHeapIndex];
	}
	else {
		bytesPerHostVisibleHeap = physicalDeviceMemoryProperties.memoryProperties.memoryHeaps[hostVisibleMemoryHeapIndex].size;
		bytesPerDeviceLocalHeap = physicalDeviceMemoryProperties.memoryProperties.memoryHeaps[deviceLocalMemoryHeapIndex].size;
	}

	VkDeviceSize bytesPerHeap = min_DeviceSize(bytesPerHostVisibleHeap, bytesPerDeviceLocalHeap);
	bytesPerHeap /= deviceLocalMemoryHeapIndex == hostVisibleMemoryHeapIndex ? 2 : 1;
	bytesPerHeap *= MAX_HEAP_MEMORY;

	VkDeviceSize bytesPerDeviceMemory = min_DeviceSize(maxMemoryAllocationSize, bytesPerHeap);
	uint32_t deviceMemoriesPerHeap = bytesPerHeap / bytesPerDeviceMemory;

	if (deviceMemoriesPerHeap < maxMemoryAllocationCount && bytesPerHeap % bytesPerDeviceMemory) {
		const VkDeviceSize excessBytes = bytesPerDeviceMemory - bytesPerHeap % bytesPerDeviceMemory;
		deviceMemoriesPerHeap++;
		bytesPerDeviceMemory -= excessBytes / deviceMemoriesPerHeap;
		bytesPerDeviceMemory -= excessBytes % deviceMemoriesPerHeap ? 1 : 0;
	}
	else if (deviceMemoriesPerHeap > maxMemoryAllocationCount)
		deviceMemoriesPerHeap = maxMemoryAllocationCount;

	VkDeviceSize bytesPerBuffer = min_DeviceSize(maxBufferSize, bytesPerDeviceMemory);
	uint32_t buffersPerDeviceMemory = bytesPerDeviceMemory / bytesPerBuffer;

	if (bytesPerDeviceMemory % bytesPerBuffer) {
		const VkDeviceSize excessBytes = bytesPerBuffer - bytesPerDeviceMemory % bytesPerBuffer;
		buffersPerDeviceMemory++;
		bytesPerBuffer -= excessBytes / buffersPerDeviceMemory;
		bytesPerBuffer -= excessBytes % buffersPerDeviceMemory ? 1 : 0;
	}

	const uint32_t valuesPerInBuffer = maxInBufferRange / sizeof(value_t);
	const uint32_t valuesPerOutBuffer = maxOutBufferRange / sizeof(step_t);
	uint32_t valuesPerInoutBuffer = min_uint32(valuesPerInBuffer, valuesPerOutBuffer);
	uint32_t computeWorkGroupSize = maxComputeWorkGroupSize;
	uint32_t computeWorkGroupCount = min_uint32(maxComputeWorkGroupCount, valuesPerInoutBuffer / maxComputeWorkGroupSize);

	valuesPerInoutBuffer = computeWorkGroupSize * computeWorkGroupCount;
	VkDeviceSize bytesPerInoutBuffer = valuesPerInoutBuffer * (sizeof(value_t) + sizeof(step_t));
	uint32_t inoutBuffersPerBuffer = bytesPerBuffer / bytesPerInoutBuffer;

	if (bytesPerBuffer % bytesPerInoutBuffer > computeWorkGroupSize * (sizeof(value_t) + sizeof(step_t))) {
		const uint32_t excessValues = valuesPerInoutBuffer - bytesPerBuffer % bytesPerInoutBuffer / (sizeof(value_t) + sizeof(step_t));
		inoutBuffersPerBuffer++;
		valuesPerInoutBuffer -= excessValues / inoutBuffersPerBuffer;
		valuesPerInoutBuffer -= excessValues % inoutBuffersPerBuffer ? 1 : 0;
		valuesPerInoutBuffer -= valuesPerInoutBuffer % computeWorkGroupSize;
		computeWorkGroupCount = valuesPerInoutBuffer / computeWorkGroupSize;
	}

	VkDeviceSize bytesPerInBuffer  = valuesPerInoutBuffer * sizeof(value_t);
	VkDeviceSize bytesPerOutBuffer = valuesPerInoutBuffer * sizeof(step_t);
	VkDeviceSize inBufferAlignment  = (inBufferOffsetAlignment  - bytesPerInBuffer  % inBufferOffsetAlignment ) % inBufferOffsetAlignment;
	VkDeviceSize outBufferAlignment = (outBufferOffsetAlignment - bytesPerOutBuffer % outBufferOffsetAlignment) % outBufferOffsetAlignment;

	VkDeviceSize bytesPerHostVisibleInoutBuffer = bytesPerInBuffer + bytesPerOutBuffer;
	VkDeviceSize bytesPerDeviceLocalInoutBuffer = bytesPerInBuffer + bytesPerOutBuffer + inBufferAlignment + outBufferAlignment;
	VkDeviceSize bytesPerHostVisibleBuffer = bytesPerHostVisibleInoutBuffer * inoutBuffersPerBuffer;
	VkDeviceSize bytesPerDeviceLocalBuffer = bytesPerDeviceLocalInoutBuffer * inoutBuffersPerBuffer;

	VkBufferCreateInfo hostVisibleBufferCreateInfo;
	hostVisibleBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hostVisibleBufferCreateInfo.pNext = NULL;
	hostVisibleBufferCreateInfo.flags = 0;
	hostVisibleBufferCreateInfo.size = bytesPerHostVisibleBuffer;
	hostVisibleBufferCreateInfo.usage = HOST_VISIBLE_BUFFER_USAGE;
	hostVisibleBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	hostVisibleBufferCreateInfo.queueFamilyIndexCount = 0;
	hostVisibleBufferCreateInfo.pQueueFamilyIndices = NULL;

	VkBufferCreateInfo deviceLocalBufferCreateInfo;
	deviceLocalBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	deviceLocalBufferCreateInfo.pNext = NULL;
	deviceLocalBufferCreateInfo.flags = 0;
	deviceLocalBufferCreateInfo.size = bytesPerDeviceLocalBuffer;
	deviceLocalBufferCreateInfo.usage = DEVICE_LOCAL_BUFFER_USAGE;
	deviceLocalBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	deviceLocalBufferCreateInfo.queueFamilyIndexCount = 0;
	deviceLocalBufferCreateInfo.pQueueFamilyIndices = NULL;

	VkDeviceBufferMemoryRequirementsKHR hostVisibleDevicebufferMemoryRequirements;
	hostVisibleDevicebufferMemoryRequirements.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS_KHR;
	hostVisibleDevicebufferMemoryRequirements.pNext = NULL;
	hostVisibleDevicebufferMemoryRequirements.pCreateInfo = &hostVisibleBufferCreateInfo;

	VkDeviceBufferMemoryRequirementsKHR deviceLocalDevicebufferMemoryRequirements;
	deviceLocalDevicebufferMemoryRequirements.sType = VK_STRUCTURE_TYPE_DEVICE_BUFFER_MEMORY_REQUIREMENTS_KHR;
	deviceLocalDevicebufferMemoryRequirements.pNext = NULL;
	deviceLocalDevicebufferMemoryRequirements.pCreateInfo = &deviceLocalBufferCreateInfo;

	VkMemoryRequirements2 hostVisibleMemoryRequirements;
	hostVisibleMemoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	hostVisibleMemoryRequirements.pNext = NULL;

	VkMemoryRequirements2 deviceLocalMemoryRequirements;
	deviceLocalMemoryRequirements.sType = VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2;
	deviceLocalMemoryRequirements.pNext = NULL;

	vkGetDeviceBufferMemoryRequirementsKHR(device, &hostVisibleDevicebufferMemoryRequirements, &hostVisibleMemoryRequirements);
	vkGetDeviceBufferMemoryRequirementsKHR(device, &deviceLocalDevicebufferMemoryRequirements, &deviceLocalMemoryRequirements);

	VkDeviceSize hostVisibleBufferAlignment = hostVisibleMemoryRequirements.memoryRequirements.alignment;
	VkDeviceSize deviceLocalBufferAlignment = deviceLocalMemoryRequirements.memoryRequirements.alignment;
	hostVisibleBufferAlignment = (hostVisibleBufferAlignment - bytesPerHostVisibleBuffer % hostVisibleBufferAlignment) % hostVisibleBufferAlignment;
	deviceLocalBufferAlignment = (deviceLocalBufferAlignment - bytesPerDeviceLocalBuffer % deviceLocalBufferAlignment) % deviceLocalBufferAlignment;

	bytesPerHostVisibleBuffer += hostVisibleBufferAlignment;
	bytesPerDeviceLocalBuffer += deviceLocalBufferAlignment;
	VkDeviceSize bytesPerHostVisibleDeviceMemory = bytesPerHostVisibleBuffer * buffersPerDeviceMemory;
	VkDeviceSize bytesPerDeviceLocalDeviceMemory = bytesPerDeviceLocalBuffer * buffersPerDeviceMemory;
	bytesPerHostVisibleHeap = bytesPerHostVisibleDeviceMemory * deviceMemoriesPerHeap;
	bytesPerDeviceLocalHeap = bytesPerDeviceLocalDeviceMemory * deviceMemoriesPerHeap;

	const uint32_t valuesPerBuffer = valuesPerInoutBuffer * inoutBuffersPerBuffer;
	const uint32_t valuesPerDeviceMemory = valuesPerBuffer * buffersPerDeviceMemory;
	const uint32_t valuesPerHeap = valuesPerDeviceMemory * deviceMemoriesPerHeap;
	const uint32_t inoutBuffersPerDeviceMemory = inoutBuffersPerBuffer * buffersPerDeviceMemory;
	const uint32_t inoutBuffersPerHeap = inoutBuffersPerDeviceMemory * deviceMemoriesPerHeap;
	const uint32_t buffersPerHeap = buffersPerDeviceMemory * deviceMemoriesPerHeap;

	gpu->inBufferAlignment  = inBufferAlignment;
	gpu->outBufferAlignment = outBufferAlignment;
	gpu->hostVisibleBufferAlignment = hostVisibleBufferAlignment;
	gpu->deviceLocalBufferAlignment = deviceLocalBufferAlignment;
	gpu->computeWorkGroupCount = computeWorkGroupCount;
	gpu->computeWorkGroupSize  = computeWorkGroupSize;

	gpu->bytesPerInBuffer  = bytesPerInBuffer;
	gpu->bytesPerOutBuffer = bytesPerOutBuffer;
	gpu->bytesPerHostVisibleInoutBuffer = bytesPerHostVisibleInoutBuffer;
	gpu->bytesPerDeviceLocalInoutBuffer = bytesPerDeviceLocalInoutBuffer;
	gpu->bytesPerHostVisibleBuffer = bytesPerHostVisibleBuffer;
	gpu->bytesPerDeviceLocalBuffer = bytesPerDeviceLocalBuffer;
	gpu->bytesPerHostVisibleDeviceMemory = bytesPerHostVisibleDeviceMemory;
	gpu->bytesPerDeviceLocalDeviceMemory = bytesPerDeviceLocalDeviceMemory;
	gpu->bytesPerHostVisibleHeap = bytesPerHostVisibleHeap;
	gpu->bytesPerDeviceLocalHeap = bytesPerDeviceLocalHeap;

	gpu->valuesPerInoutBuffer = valuesPerInoutBuffer;
	gpu->valuesPerBuffer = valuesPerBuffer;
	gpu->valuesPerDeviceMemory = valuesPerDeviceMemory;
	gpu->valuesPerHeap = valuesPerHeap;
	gpu->inoutBuffersPerBuffer = inoutBuffersPerBuffer;
	gpu->inoutBuffersPerDeviceMemory = inoutBuffersPerDeviceMemory;
	gpu->inoutBuffersPerHeap = inoutBuffersPerHeap;
	gpu->buffersPerDeviceMemory = buffersPerDeviceMemory;
	gpu->buffersPerHeap = buffersPerHeap;
	gpu->deviceMemoriesPerHeap = deviceMemoriesPerHeap;

	printf(
		"Memory information:\n"
		"\tIn-buffer alignment:       0x%llx\n"
		"\tOut-buffer alignment:      0x%llx\n"
		"\tHV-buffer alignment:       0x%llx\n"
		"\tDL-buffer alignment:       0x%llx\n"
		"\tCompute workgroup size:    %u\n"
		"\tCompute workgroup count:   %u\n"
		"\tValues per inout-buffer:   %u\n"
		"\tInout-buffers per buffer:  %u\n"
		"\tBuffers per device memory: %u\n"
		"\tDevice memories per heap:  %u\n"
		"\tValues per heap:           %u\n\n",
		inBufferAlignment,
		outBufferAlignment,
		hostVisibleBufferAlignment,
		deviceLocalBufferAlignment,
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

	gpu->dynamicMemory = calloc((size_t) 1, size);
#ifndef NDEBUG
	if (!gpu->dynamicMemory) {
		CALLOC_FAILURE(gpu->dynamicMemory, 1, size)
		return false;
	}
#endif // NDEBUG

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

static void free_CreateBuffersData(const CreateBuffersData_t data)
{
	free(data.memory);
}

bool create_buffers(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkDevice device = volkGetLoadedDevice();
	const uint32_t hostVisibleMemoryTypeIndex = gpu->hostVisibleMemoryTypeIndex;
	const uint32_t deviceLocalMemoryTypeIndex = gpu->deviceLocalMemoryTypeIndex;
	VkDeviceMemory* const hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* const deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	const VkDeviceSize hostVisibleBufferAlignment = gpu->hostVisibleBufferAlignment;
	VkBuffer* const hostVisibleBuffers = gpu->hostVisibleBuffers;
	VkBuffer* const deviceLocalBuffers = gpu->deviceLocalBuffers;
	value_t** const mappedHostVisibleInBuffers = gpu->mappedHostVisibleInBuffers;
	step_t** const mappedHostVisibleOutBuffers = gpu->mappedHostVisibleOutBuffers;

	const VkDeviceSize bytesPerHostVisibleBuffer = gpu->bytesPerHostVisibleBuffer;
	const VkDeviceSize bytesPerDeviceLocalBuffer = gpu->bytesPerDeviceLocalBuffer;
	const VkDeviceSize bytesPerHostVisibleDeviceMemory = gpu->bytesPerHostVisibleDeviceMemory;
	const VkDeviceSize bytesPerDeviceLocalDeviceMemory = gpu->bytesPerDeviceLocalDeviceMemory;
	const uint32_t valuesPerInoutBuffer = gpu->valuesPerInoutBuffer;
	const uint32_t inoutBuffersPerBuffer = gpu->inoutBuffersPerBuffer;
	const uint32_t buffersPerDeviceMemory = gpu->buffersPerDeviceMemory;
	const uint32_t buffersPerHeap = gpu->buffersPerHeap;
	const uint32_t deviceMemoriesPerHeap = gpu->deviceMemoriesPerHeap;
	const bool usingMemoryPriority = gpu->usingMemoryPriority;
	const VkAllocationCallbacks* const allocator = gpu->allocator;

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
#endif // NDEBUG

	VkBindBufferMemoryInfo (*const bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) data.memory;

	VkBufferCreateInfo hostVisibleBufferCreateInfo;
	hostVisibleBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	hostVisibleBufferCreateInfo.pNext = NULL;
	hostVisibleBufferCreateInfo.flags = 0;
	hostVisibleBufferCreateInfo.size = bytesPerHostVisibleBuffer;
	hostVisibleBufferCreateInfo.usage = HOST_VISIBLE_BUFFER_USAGE;
	hostVisibleBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	hostVisibleBufferCreateInfo.queueFamilyIndexCount = 0;
	hostVisibleBufferCreateInfo.pQueueFamilyIndices = NULL;

	VkBufferCreateInfo deviceLocalBufferCreateInfo;
	deviceLocalBufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	deviceLocalBufferCreateInfo.pNext = NULL;
	deviceLocalBufferCreateInfo.flags = 0;
	deviceLocalBufferCreateInfo.size = bytesPerDeviceLocalBuffer;
	deviceLocalBufferCreateInfo.usage = DEVICE_LOCAL_BUFFER_USAGE;
	deviceLocalBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	deviceLocalBufferCreateInfo.queueFamilyIndexCount = 0;
	deviceLocalBufferCreateInfo.pQueueFamilyIndices = NULL;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		GET_RESULT(vkCreateBuffer(device, &hostVisibleBufferCreateInfo, allocator, &hostVisibleBuffers[i]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkCreateBuffer, 4, 'p', device, 'p', &hostVisibleBufferCreateInfo, 'p', allocator, 'p', &hostVisibleBuffers[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif // NDEBUG

		GET_RESULT(vkCreateBuffer(device, &deviceLocalBufferCreateInfo, allocator, &deviceLocalBuffers[i]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkCreateBuffer, 4, 'p', device, 'p', &deviceLocalBufferCreateInfo, 'p', allocator, 'p', &deviceLocalBuffers[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif // NDEBUG
	}

	VkMemoryPriorityAllocateInfoEXT hostVisibleMemoryPriorityAllocateInfo;
	hostVisibleMemoryPriorityAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	hostVisibleMemoryPriorityAllocateInfo.pNext = NULL;
	hostVisibleMemoryPriorityAllocateInfo.priority = 0.0f;

	VkMemoryPriorityAllocateInfoEXT deviceLocalMemoryPriorityAllocateInfo;
	deviceLocalMemoryPriorityAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT;
	deviceLocalMemoryPriorityAllocateInfo.pNext = NULL;
	deviceLocalMemoryPriorityAllocateInfo.priority = 1.0f;

	VkMemoryAllocateInfo hostVisibleMemoryAllocateInfo;
	hostVisibleMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	hostVisibleMemoryAllocateInfo.pNext = usingMemoryPriority ? &hostVisibleMemoryPriorityAllocateInfo : NULL;
	hostVisibleMemoryAllocateInfo.allocationSize = bytesPerHostVisibleDeviceMemory;
	hostVisibleMemoryAllocateInfo.memoryTypeIndex = hostVisibleMemoryTypeIndex;

	VkMemoryAllocateInfo deviceLocalMemoryAllocateInfo;
	deviceLocalMemoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	deviceLocalMemoryAllocateInfo.pNext = usingMemoryPriority ? &deviceLocalMemoryPriorityAllocateInfo : NULL;
	deviceLocalMemoryAllocateInfo.allocationSize = bytesPerDeviceLocalDeviceMemory;
	deviceLocalMemoryAllocateInfo.memoryTypeIndex = deviceLocalMemoryTypeIndex;

	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		GET_RESULT(vkAllocateMemory(device, &hostVisibleMemoryAllocateInfo, allocator, &hostVisibleDeviceMemories[i]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkAllocateMemory, 4, 'p', device, 'p', &hostVisibleMemoryAllocateInfo, 'p', allocator, 'p', &hostVisibleDeviceMemories[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif // NDEBUG

		GET_RESULT(vkAllocateMemory(device, &deviceLocalMemoryAllocateInfo, allocator, &deviceLocalDeviceMemories[i]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkAllocateMemory, 4, 'p', device, 'p', &deviceLocalMemoryAllocateInfo, 'p', allocator, 'p', &deviceLocalDeviceMemories[i])
			free_CreateBuffersData(data);
			return false;
		}
#endif // NDEBUG
	}

	uint32_t bufIndex = 0; // Buffer index
	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		for (uint32_t j = 0; j < buffersPerDeviceMemory; j++, bufIndex++) {
			bindBufferMemoryInfos[bufIndex][0].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
			bindBufferMemoryInfos[bufIndex][0].pNext = NULL;
			bindBufferMemoryInfos[bufIndex][0].buffer = hostVisibleBuffers[bufIndex];
			bindBufferMemoryInfos[bufIndex][0].memory = hostVisibleDeviceMemories[i];
			bindBufferMemoryInfos[bufIndex][0].memoryOffset = bytesPerHostVisibleBuffer * j;

			bindBufferMemoryInfos[bufIndex][1].sType = VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO;
			bindBufferMemoryInfos[bufIndex][1].pNext = NULL;
			bindBufferMemoryInfos[bufIndex][1].buffer = deviceLocalBuffers[bufIndex];
			bindBufferMemoryInfos[bufIndex][1].memory = deviceLocalDeviceMemories[i];
			bindBufferMemoryInfos[bufIndex][1].memoryOffset = bytesPerDeviceLocalBuffer * j;
		}
	}

	GET_RESULT(vkBindBufferMemory2(device, 2 * buffersPerHeap, (const VkBindBufferMemoryInfo*) bindBufferMemoryInfos))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkBindBufferMemory2, 3, 'p', device, 'u', 2 * buffersPerHeap, 'p', bindBufferMemoryInfos)
		free_CreateBuffersData(data);
		return false;
	}
#endif // NDEBUG

	uint32_t inoIndex = 0; // Inout-buffer index
	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		GET_RESULT(vkMapMemory(device, hostVisibleDeviceMemories[i], (VkDeviceSize) 0, bytesPerHostVisibleDeviceMemory, 0, (void**) &mappedHostVisibleInBuffers[inoIndex]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkMapMemory, 6, 'p', device, 'p', hostVisibleDeviceMemories[i], 'u', 0, 'u', bytesPerHostVisibleDeviceMemory, 'u', 0, 'p', (void**) &mappedHostVisibleInBuffers[inoIndex])
			free_CreateBuffersData(data);
			return false;
		}
#endif // NDEBUG
		mappedHostVisibleOutBuffers[inoIndex] = (step_t*) (mappedHostVisibleInBuffers[inoIndex] + valuesPerInoutBuffer);
		inoIndex++;

		for (uint32_t j = 1; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			mappedHostVisibleInBuffers [inoIndex] = (value_t*) (mappedHostVisibleOutBuffers[inoIndex - 1] + valuesPerInoutBuffer);
			mappedHostVisibleOutBuffers[inoIndex] = (step_t*)  (mappedHostVisibleInBuffers [inoIndex]     + valuesPerInoutBuffer);
		}

		for (uint32_t j = 1; j < buffersPerDeviceMemory; j++) {
			mappedHostVisibleInBuffers [inoIndex] = (value_t*) ((char*) (mappedHostVisibleOutBuffers[inoIndex - 1] + valuesPerInoutBuffer) + hostVisibleBufferAlignment);
			mappedHostVisibleOutBuffers[inoIndex] = (step_t*)  (mappedHostVisibleInBuffers[inoIndex] + valuesPerInoutBuffer);
			inoIndex++;

			for (uint32_t k = 1; k < inoutBuffersPerBuffer; k++, inoIndex++) {
				mappedHostVisibleInBuffers [inoIndex] = (value_t*) (mappedHostVisibleOutBuffers[inoIndex - 1] + valuesPerInoutBuffer);
				mappedHostVisibleOutBuffers[inoIndex] = (step_t*)  (mappedHostVisibleInBuffers [inoIndex]     + valuesPerInoutBuffer);
			}
		}
	}

	free_CreateBuffersData(data);

#ifndef NDEBUG
	if(gpu->debugMessenger) {
		char objectName[80];
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;

		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_BUFFER;
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

				strcpy(objectName, "Device local"); // strlen("Host visible") == strlen("Device local")
				objectName[12] = ' '; // Remove '\0' from strcpy

				debugUtilsObjectNameInfo.objectHandle = (uint64_t) deviceLocalBuffers[bufIndex];
				SET_DEBUG_NAME()
			}
		}

		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_DEVICE_MEMORY;
		debugUtilsObjectNameInfo.pObjectName = objectName;

		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			sprintf(objectName,
				"Host visible device memory %u/%u",
				i + 1, deviceMemoriesPerHeap
			);

			debugUtilsObjectNameInfo.objectHandle = (uint64_t) hostVisibleDeviceMemories[i];
			SET_DEBUG_NAME()

			strcpy(objectName, "Device local"); // strlen("Host visible") == strlen("Device local")
			objectName[12] = ' '; // Remove '\0' from strcpy

			debugUtilsObjectNameInfo.objectHandle = (uint64_t) deviceLocalDeviceMemories[i];
			SET_DEBUG_NAME()
		}
	}
#endif // NDEBUG

	END_FUNC
	return true;
}

typedef struct CreateDescriptorsData
{
	void* memory;
} CreateDescriptorsData_t;

static void free_CreateDescriptorsData(const CreateDescriptorsData_t data)
{
	free(data.memory);
}

bool create_descriptors(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkDevice device = volkGetLoadedDevice();
	const VkBuffer* const deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDeviceSize inBufferAlignment = gpu->inBufferAlignment;
	VkDescriptorSet* const descriptorSets = gpu->descriptorSets;

	const VkDeviceSize bytesPerInBuffer = gpu->bytesPerInBuffer;
	const VkDeviceSize bytesPerOutBuffer = gpu->bytesPerOutBuffer;
	const VkDeviceSize bytesPerDeviceLocalInoutBuffer = gpu->bytesPerDeviceLocalInoutBuffer;
	const uint32_t inoutBuffersPerBuffer = gpu->inoutBuffersPerBuffer;
	const uint32_t inoutBuffersPerHeap = gpu->inoutBuffersPerHeap;
	const uint32_t buffersPerHeap = gpu->buffersPerHeap;

	const uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	const uint32_t computeQueueTimestampValidBits = gpu->computeQueueTimestampValidBits;
	const VkAllocationCallbacks* const allocator = gpu->allocator;

	CreateDescriptorsData_t data = {0};
	VkResult result;

	size_t size =
		inoutBuffersPerHeap     * sizeof(VkDescriptorSetLayout) +  // Descriptor set layouts
		inoutBuffersPerHeap * 2 * sizeof(VkDescriptorBufferInfo) + // In-buffer & out-buffer descriptor buffer information
		inoutBuffersPerHeap * 2 * sizeof(VkWriteDescriptorSet);    // In-buffer & out-buffer descriptor set information

	data.memory = malloc(size);
#ifndef NDEBUG
	if (!data.memory) {
		MALLOC_FAILURE(data.memory)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif // NDEBUG

	VkDescriptorSetLayout   *const descriptorSetLayouts      = (VkDescriptorSetLayout*) data.memory;
	VkDescriptorBufferInfo (*const descriptorBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (descriptorSetLayouts  + inoutBuffersPerHeap);
	VkWriteDescriptorSet   (*const writeDescriptorSets)[2]   = (VkWriteDescriptorSet(*)[])   (descriptorBufferInfos + inoutBuffersPerHeap);

	VkDescriptorSetLayoutBinding descriptorSetLayoutBindings[2];
	descriptorSetLayoutBindings[0].binding = 0;
	descriptorSetLayoutBindings[0].descriptorType = IN_BUFFER_DESCRIPTOR_TYPE;
	descriptorSetLayoutBindings[0].descriptorCount = 1;
	descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	descriptorSetLayoutBindings[0].pImmutableSamplers = NULL;

	descriptorSetLayoutBindings[1].binding = 1;
	descriptorSetLayoutBindings[1].descriptorType = OUT_BUFFER_DESCRIPTOR_TYPE;
	descriptorSetLayoutBindings[1].descriptorCount = 1;
	descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
	descriptorSetLayoutBindings[1].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo;
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.pNext = NULL;
	descriptorSetLayoutCreateInfo.flags = 0;
	descriptorSetLayoutCreateInfo.bindingCount = sizeof(descriptorSetLayoutBindings) / sizeof(descriptorSetLayoutBindings[0]);
	descriptorSetLayoutCreateInfo.pBindings = descriptorSetLayoutBindings;

	VkDescriptorPoolSize descriptorPoolSizes[2];
	descriptorPoolSizes[0].type = IN_BUFFER_DESCRIPTOR_TYPE;
	descriptorPoolSizes[0].descriptorCount = inoutBuffersPerHeap;

	descriptorPoolSizes[1].type = OUT_BUFFER_DESCRIPTOR_TYPE;
	descriptorPoolSizes[1].descriptorCount = inoutBuffersPerHeap;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = NULL;
	descriptorPoolCreateInfo.flags = 0;
	descriptorPoolCreateInfo.maxSets = inoutBuffersPerHeap;
	descriptorPoolCreateInfo.poolSizeCount = sizeof(descriptorPoolSizes) / sizeof(descriptorPoolSizes[0]);
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;

	GET_RESULT(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, allocator, &gpu->descriptorSetLayout))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateDescriptorSetLayout, 4, 'p', device, 'p', &descriptorSetLayoutCreateInfo, 'p', allocator, 'p', &gpu->descriptorSetLayout)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, allocator, &gpu->descriptorPool))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateDescriptorPool, 4, 'p', device, 'p', &descriptorPoolCreateInfo, 'p', allocator, 'p', &gpu->descriptorPool)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif // NDEBUG

	const VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	const VkDescriptorPool descriptorPool = gpu->descriptorPool;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		descriptorSetLayouts[i] = descriptorSetLayout;
	
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.pNext = NULL;
	descriptorSetAllocateInfo.descriptorPool = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = inoutBuffersPerHeap;
	descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts;

	GET_RESULT(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, descriptorSets))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkAllocateDescriptorSets, 3, 'p', device, 'p', &descriptorSetAllocateInfo, 'p', descriptorSets)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif // NDEBUG

	uint32_t inoIndex = 0; // Inout-buffer index
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			descriptorBufferInfos[inoIndex][0].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[inoIndex][0].offset = bytesPerDeviceLocalInoutBuffer * j;
			descriptorBufferInfos[inoIndex][0].range = bytesPerInBuffer;

			descriptorBufferInfos[inoIndex][1].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[inoIndex][1].offset = bytesPerInBuffer + inBufferAlignment + bytesPerDeviceLocalInoutBuffer * j;
			descriptorBufferInfos[inoIndex][1].range = bytesPerOutBuffer;

			writeDescriptorSets[inoIndex][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[inoIndex][0].pNext = NULL;
			writeDescriptorSets[inoIndex][0].dstSet = descriptorSets[inoIndex];
			writeDescriptorSets[inoIndex][0].dstBinding = 0;
			writeDescriptorSets[inoIndex][0].dstArrayElement = 0;
			writeDescriptorSets[inoIndex][0].descriptorCount = 1;
			writeDescriptorSets[inoIndex][0].descriptorType = IN_BUFFER_DESCRIPTOR_TYPE;
			writeDescriptorSets[inoIndex][0].pImageInfo = NULL;
			writeDescriptorSets[inoIndex][0].pBufferInfo = &descriptorBufferInfos[inoIndex][0];
			writeDescriptorSets[inoIndex][0].pTexelBufferView = NULL;

			writeDescriptorSets[inoIndex][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			writeDescriptorSets[inoIndex][1].pNext = NULL;
			writeDescriptorSets[inoIndex][1].dstSet = descriptorSets[inoIndex];
			writeDescriptorSets[inoIndex][1].dstBinding = 1;
			writeDescriptorSets[inoIndex][1].dstArrayElement = 0;
			writeDescriptorSets[inoIndex][1].descriptorCount = 1;
			writeDescriptorSets[inoIndex][1].descriptorType = OUT_BUFFER_DESCRIPTOR_TYPE;
			writeDescriptorSets[inoIndex][1].pImageInfo = NULL;
			writeDescriptorSets[inoIndex][1].pBufferInfo = &descriptorBufferInfos[inoIndex][1];
			writeDescriptorSets[inoIndex][1].pTexelBufferView = NULL;
		}
	}

	vkUpdateDescriptorSets(device, 2 * inoutBuffersPerHeap, (const VkWriteDescriptorSet*) writeDescriptorSets, 0, NULL);

	VkQueryPoolCreateInfo queryPoolCreateInfo;
	queryPoolCreateInfo.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	queryPoolCreateInfo.pNext = NULL;
	queryPoolCreateInfo.flags = 0;
	queryPoolCreateInfo.queryType = VK_QUERY_TYPE_TIMESTAMP;
	queryPoolCreateInfo.queryCount = 4 * inoutBuffersPerHeap;
	queryPoolCreateInfo.pipelineStatistics = 0;

	if (transferQueueTimestampValidBits || computeQueueTimestampValidBits)
		GET_RESULT(vkCreateQueryPool(device, &queryPoolCreateInfo, allocator, &gpu->queryPool))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateQueryPool, 4, 'p', device, 'p', &queryPoolCreateInfo, 'p', allocator, 'p', &gpu->queryPool)
		free_CreateDescriptorsData(data);
		return false;
	}
#endif // NDEBUG

	free_CreateDescriptorsData(data);

#ifndef NDEBUG
	if(gpu->debugMessenger) {
		const uint32_t buffersPerDeviceMemory = gpu->buffersPerDeviceMemory;
		const uint32_t deviceMemoriesPerHeap = gpu->deviceMemoriesPerHeap;

		char objectName[122];
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;
		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_DESCRIPTOR_SET;
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
#endif // NDEBUG

	END_FUNC
	return true;
}

typedef struct CreatePipelineData
{
	void* shaderCode;
	void* pipelineCache;
} CreatePipelineData_t;

static void free_CreatePipelineData(const CreatePipelineData_t data)
{
	free(data.shaderCode);
	free(data.pipelineCache);
}

bool create_pipeline(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkDevice device = volkGetLoadedDevice();
	const VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;

	const uint32_t valuesPerInoutBuffer = gpu->valuesPerInoutBuffer;
	const uint32_t computeWorkGroupSize = gpu->computeWorkGroupSize;
	const bool usingShaderInt64 = gpu->usingShaderInt64;
	const VkAllocationCallbacks* const allocator = gpu->allocator;

	CreatePipelineData_t data = {0};
	VkResult result;
	int fileResult;
	size_t readResult;

	// Get shader code from file, pre-compiled into SPIR-V
	const char* const shaderName = usingShaderInt64 ? SHADER64_NAME : SHADER32_NAME;
	FILE* file = fopen(shaderName, "rb");
#ifndef NDEBUG
	if (!file) {
		FOPEN_FAILURE(shaderName, "rb")
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	GET_FILE_RESULT(fseek(file, 0, SEEK_END))
#ifndef NDEBUG
	if (fileResult) {
		FSEEK_FAILURE(0, SEEK_END)
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	long fileSize = ftell(file);
#ifndef NDEBUG
	if (fileSize == -1) {
		FTELL_FAILURE();
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}

	if (fileSize % 4) {
		fputs("Shader failure: SPIR-V file size isn't a multiple of 4\n\n", stderr);
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	rewind(file);

	size_t size = (size_t) fileSize;
	data.shaderCode = malloc(size);
#ifndef NDEBUG
	if (!data.shaderCode) {
		MALLOC_FAILURE(data.shaderCode)
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	const size_t shaderSize = (size_t) fileSize;
	uint32_t* const shaderCode = (uint32_t*) data.shaderCode;

	GET_READ_RESULT(fread(shaderCode, (size_t) 1, shaderSize, file))
#ifndef NDEBUG
	if (readResult != shaderSize) {
		FREAD_FAILURE(shaderCode, 1, shaderSize)
		fclose(file);
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	GET_FILE_RESULT(fclose(file))
#ifndef NDEBUG
	if (fileResult) {
		FCLOSE_FAILURE()
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	// Get pipeline cache from file, if it exists
	size_t cacheSize = 0;
	void* cacheData = NULL;
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
#endif // NDEBUG

		fileSize = ftell(file);
#ifndef NDEBUG
		if (fileSize == -1) {
			FTELL_FAILURE();
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif // NDEBUG

		rewind(file);

		size = (size_t) fileSize;
		data.pipelineCache = malloc(size);
#ifndef NDEBUG
		if (!data.pipelineCache) {
			MALLOC_FAILURE(data.pipelineCache)
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif // NDEBUG

		cacheSize = (size_t) fileSize;
		cacheData = data.pipelineCache;

		GET_READ_RESULT(fread(cacheData, (size_t) 1, cacheSize, file))
#ifndef NDEBUG
		if (readResult != cacheSize) {
			FREAD_FAILURE(cacheData, 1, cacheSize)
			fclose(file);
			free_CreatePipelineData(data);
			return false;
		}
#endif // NDEBUG

		GET_FILE_RESULT(fclose(file))
#ifndef NDEBUG
		if (fileResult) {
			FCLOSE_FAILURE()
			free_CreatePipelineData(data);
			return false;
		}
#endif // NDEBUG
	}

	VkShaderModuleCreateInfo shaderModuleCreateInfo;
	shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shaderModuleCreateInfo.pNext = NULL;
	shaderModuleCreateInfo.flags = 0;
	shaderModuleCreateInfo.codeSize = shaderSize;
	shaderModuleCreateInfo.pCode = shaderCode;

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo;
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.pNext = NULL;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = cacheSize;
	pipelineCacheCreateInfo.pInitialData = cacheData;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.pNext = NULL;
	pipelineLayoutCreateInfo.flags = 0;
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pipelineLayoutCreateInfo.pPushConstantRanges = NULL;

	GET_RESULT(vkCreateShaderModule(device, &shaderModuleCreateInfo, allocator, &gpu->shaderModule))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateShaderModule, 4, 'p', device, 'p', &shaderModuleCreateInfo, 'p', allocator, 'p', &gpu->shaderModule)
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, allocator, &gpu->pipelineCache))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreatePipelineCache, 4, 'p', device, 'p', &pipelineCacheCreateInfo, 'p', allocator, 'p', &gpu->pipelineCache)
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, allocator, &gpu->pipelineLayout))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreatePipelineLayout, 4, 'p', device, 'p', &pipelineLayoutCreateInfo, 'p', allocator, 'p', &gpu->pipelineLayout)
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	const VkShaderModule shaderModule = gpu->shaderModule;
	const VkPipelineCache pipelineCache = gpu->pipelineCache;
	const VkPipelineLayout pipelineLayout = gpu->pipelineLayout;

	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, allocator);
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	uint32_t specialisationData[2];
	specialisationData[0] = computeWorkGroupSize;
	specialisationData[1] = valuesPerInoutBuffer;

	VkSpecializationMapEntry specialisationMapEntries[2];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset = 0;
	specialisationMapEntries[0].size = sizeof(specialisationData[0]);
	specialisationMapEntries[1].constantID = 1;
	specialisationMapEntries[1].offset = sizeof(specialisationData[0]);
	specialisationMapEntries[1].size = sizeof(specialisationData[1]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = sizeof(specialisationMapEntries) / sizeof(specialisationMapEntries[0]);
	specialisationInfo.pMapEntries = specialisationMapEntries;
	specialisationInfo.dataSize = sizeof(specialisationData);
	specialisationInfo.pData = specialisationData;

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo;
	pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	pipelineShaderStageCreateInfo.pNext = NULL;
	pipelineShaderStageCreateInfo.flags = 0;
	pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineShaderStageCreateInfo.module = shaderModule;
	pipelineShaderStageCreateInfo.pName = "main";
	pipelineShaderStageCreateInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo computePipelineCreateInfo;
	computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
	computePipelineCreateInfo.pNext = NULL;
	computePipelineCreateInfo.flags = 0;
	computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
	computePipelineCreateInfo.layout = pipelineLayout;
	computePipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
	computePipelineCreateInfo.basePipelineIndex = 0;

	GET_RESULT(vkCreateComputePipelines(device, pipelineCache, 1, &computePipelineCreateInfo, allocator, &gpu->pipeline))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateComputePipelines, 6, 'p', device, 'p', pipelineCache, 'u', 1, 'p', &computePipelineCreateInfo, 'p', allocator, 'p', &gpu->pipeline)
		free_CreatePipelineData(data);
		return false;
	}
#endif // NDEBUG

	vkDestroyShaderModule(device, shaderModule, allocator);
	gpu->shaderModule = VK_NULL_HANDLE;

	free_CreatePipelineData(data);

	END_FUNC
	return true;
}

typedef struct CreateCommandsData
{
	void* memory;
} CreateCommandsData_t;

static void free_CreateCommandsData(const CreateCommandsData_t data)
{
	free(data.memory);
}

bool create_commands(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkDevice device = volkGetLoadedDevice();
	const uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;
	const uint32_t computeQueueFamilyIndex = gpu->computeQueueFamilyIndex;
	const VkDeviceSize inBufferAlignment = gpu->inBufferAlignment;
	const VkBuffer* const hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* const deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* const descriptorSets = gpu->descriptorSets;
	const VkPipelineLayout pipelineLayout = gpu->pipelineLayout;
	const VkPipeline pipeline = gpu->pipeline;
	VkCommandBuffer* const transferCommandBuffers = gpu->transferCommandBuffers;
	VkCommandBuffer* const computeCommandBuffers  = gpu->computeCommandBuffers;
	VkSemaphore* const semaphores = gpu->semaphores;

	const VkDeviceSize bytesPerInBuffer = gpu->bytesPerInBuffer;
	const VkDeviceSize bytesPerOutBuffer = gpu->bytesPerOutBuffer;
	const VkDeviceSize bytesPerHostVisibleInoutBuffer = gpu->bytesPerHostVisibleInoutBuffer;
	const VkDeviceSize bytesPerDeviceLocalInoutBuffer = gpu->bytesPerDeviceLocalInoutBuffer;
	const uint32_t inoutBuffersPerBuffer = gpu->inoutBuffersPerBuffer;
	const uint32_t inoutBuffersPerHeap = gpu->inoutBuffersPerHeap;
	const uint32_t buffersPerHeap = gpu->buffersPerHeap;
	const uint32_t computeWorkGroupCount = gpu->computeWorkGroupCount;

	const uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	const uint32_t computeQueueTimestampValidBits = gpu->computeQueueTimestampValidBits;
	const VkQueryPool queryPool = gpu->queryPool;
	const VkAllocationCallbacks* const allocator = gpu->allocator;

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
#endif // NDEBUG

	VkBufferCopy* const inBufferCopies  = (VkBufferCopy*) data.memory;
	VkBufferCopy* const outBufferCopies = (VkBufferCopy*) (inBufferCopies + inoutBuffersPerBuffer);

	VkBufferMemoryBarrier2KHR  *const onetimeBufferMemoryBarriers      = (VkBufferMemoryBarrier2KHR*)     (outBufferCopies              + inoutBuffersPerBuffer);
	VkBufferMemoryBarrier2KHR (*const transferBufferMemoryBarriers)[3] = (VkBufferMemoryBarrier2KHR(*)[]) (onetimeBufferMemoryBarriers  + inoutBuffersPerHeap);
	VkBufferMemoryBarrier2KHR (*const computeBufferMemoryBarriers)[2]  = (VkBufferMemoryBarrier2KHR(*)[]) (transferBufferMemoryBarriers + inoutBuffersPerHeap);

	VkDependencyInfoKHR (*const transferDependencyInfos)[2] = (VkDependencyInfoKHR(*)[]) (computeBufferMemoryBarriers + inoutBuffersPerHeap);
	VkDependencyInfoKHR (*const computeDependencyInfos)[2]  = (VkDependencyInfoKHR(*)[]) (transferDependencyInfos     + inoutBuffersPerHeap);

	VkCommandPoolCreateInfo onetimeCommandPoolCreateInfo;
	onetimeCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	onetimeCommandPoolCreateInfo.pNext = NULL;
	onetimeCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	onetimeCommandPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo transferCommandPoolCreateInfo;
	transferCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transferCommandPoolCreateInfo.pNext = NULL;
	transferCommandPoolCreateInfo.flags = 0;
	transferCommandPoolCreateInfo.queueFamilyIndex = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo computeCommandPoolCreateInfo;
	computeCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	computeCommandPoolCreateInfo.pNext = NULL;
	computeCommandPoolCreateInfo.flags = 0;
	computeCommandPoolCreateInfo.queueFamilyIndex = computeQueueFamilyIndex;

	GET_RESULT(vkCreateCommandPool(device, &onetimeCommandPoolCreateInfo, allocator, &gpu->onetimeCommandPool))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateCommandPool, 4, 'p', device, 'p', &onetimeCommandPoolCreateInfo, 'p', allocator, 'p', &gpu->onetimeCommandPool)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkCreateCommandPool(device, &transferCommandPoolCreateInfo, allocator, &gpu->transferCommandPool))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateCommandPool, 4, 'p', device, 'p', &transferCommandPoolCreateInfo, 'p', allocator, 'p', &gpu->transferCommandPool)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkCreateCommandPool(device, &computeCommandPoolCreateInfo, allocator, &gpu->computeCommandPool))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateCommandPool, 4, 'p', device, 'p', &computeCommandPoolCreateInfo, 'p', allocator, 'p', &gpu->computeCommandPool)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	const VkCommandPool onetimeCommandPool = gpu->onetimeCommandPool;
	const VkCommandPool transferCommandPool = gpu->transferCommandPool;
	const VkCommandPool computeCommandPool = gpu->computeCommandPool;

	VkCommandBufferAllocateInfo onetimeCommandBufferAllocateInfo;
	onetimeCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	onetimeCommandBufferAllocateInfo.pNext = NULL;
	onetimeCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	onetimeCommandBufferAllocateInfo.commandPool = onetimeCommandPool;
	onetimeCommandBufferAllocateInfo.commandBufferCount = 1;

	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo;
	transferCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	transferCommandBufferAllocateInfo.pNext = NULL;
	transferCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandPool = transferCommandPool;
	transferCommandBufferAllocateInfo.commandBufferCount = inoutBuffersPerHeap;

	VkCommandBufferAllocateInfo computeCommandBufferAllocateInfo;
	computeCommandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	computeCommandBufferAllocateInfo.pNext = NULL;
	computeCommandBufferAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeCommandBufferAllocateInfo.commandPool = computeCommandPool;
	computeCommandBufferAllocateInfo.commandBufferCount = inoutBuffersPerHeap;

	GET_RESULT(vkAllocateCommandBuffers(device, &onetimeCommandBufferAllocateInfo, &gpu->onetimeCommandBuffer))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkAllocateCommandBuffers, 3, 'p', device, 'p', &onetimeCommandBufferAllocateInfo, 'p', &gpu->onetimeCommandBuffer)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkAllocateCommandBuffers(device, &transferCommandBufferAllocateInfo, transferCommandBuffers))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkAllocateCommandBuffers, 3, 'p', device, 'p', &transferCommandBufferAllocateInfo, 'p', transferCommandBuffers)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkAllocateCommandBuffers(device, &computeCommandBufferAllocateInfo, computeCommandBuffers))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkAllocateCommandBuffers, 3, 'p', device, 'p', &computeCommandBufferAllocateInfo, 'p', computeCommandBuffers)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	const VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;

	for (uint32_t i = 0; i < inoutBuffersPerBuffer; i++) {
		inBufferCopies[i].srcOffset = bytesPerHostVisibleInoutBuffer * i;
		inBufferCopies[i].dstOffset = bytesPerDeviceLocalInoutBuffer * i;
		inBufferCopies[i].size = bytesPerInBuffer;

		outBufferCopies[i].srcOffset = bytesPerInBuffer + inBufferAlignment + bytesPerDeviceLocalInoutBuffer * i;
		outBufferCopies[i].dstOffset = bytesPerInBuffer + bytesPerHostVisibleInoutBuffer * i;
		outBufferCopies[i].size = bytesPerOutBuffer;
	}

	uint32_t inoIndex = 0; // Inout-buffer index
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			// Availability operation: (vkCmdCopyBuffer; DL-in) -> device domain
			// Release operation: DL-in -> compute QF
			onetimeBufferMemoryBarriers[inoIndex].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			onetimeBufferMemoryBarriers[inoIndex].pNext = NULL;
			onetimeBufferMemoryBarriers[inoIndex].srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
			onetimeBufferMemoryBarriers[inoIndex].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			onetimeBufferMemoryBarriers[inoIndex].dstStageMask  = VK_PIPELINE_STAGE_2_NONE;
			onetimeBufferMemoryBarriers[inoIndex].dstAccessMask = VK_ACCESS_2_NONE;
			onetimeBufferMemoryBarriers[inoIndex].srcQueueFamilyIndex = transferQueueFamilyIndex;
			onetimeBufferMemoryBarriers[inoIndex].dstQueueFamilyIndex = computeQueueFamilyIndex;
			onetimeBufferMemoryBarriers[inoIndex].buffer = deviceLocalBuffers[i];
			onetimeBufferMemoryBarriers[inoIndex].offset = bytesPerDeviceLocalInoutBuffer * j;
			onetimeBufferMemoryBarriers[inoIndex].size = bytesPerInBuffer;

			// Availability operation: (vkCmdCopyBuffer; DL-in) -> device domain
			// Release operation: DL-in -> compute QF
			transferBufferMemoryBarriers[inoIndex][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			transferBufferMemoryBarriers[inoIndex][0].pNext = NULL;
			transferBufferMemoryBarriers[inoIndex][0].srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[inoIndex][0].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[inoIndex][0].dstStageMask  = VK_PIPELINE_STAGE_2_NONE;
			transferBufferMemoryBarriers[inoIndex][0].dstAccessMask = VK_ACCESS_2_NONE;
			transferBufferMemoryBarriers[inoIndex][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers[inoIndex][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers[inoIndex][0].buffer = deviceLocalBuffers[i];
			transferBufferMemoryBarriers[inoIndex][0].offset = bytesPerDeviceLocalInoutBuffer * j;
			transferBufferMemoryBarriers[inoIndex][0].size = bytesPerInBuffer;

			// Aquire operation: compute QF -> DL-out
			// Visibility operation: device domain -> (vkCmdCopyBuffer; DL-out)
			transferBufferMemoryBarriers[inoIndex][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			transferBufferMemoryBarriers[inoIndex][1].pNext = NULL;
			transferBufferMemoryBarriers[inoIndex][1].srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
			transferBufferMemoryBarriers[inoIndex][1].srcAccessMask = VK_ACCESS_2_NONE;
			transferBufferMemoryBarriers[inoIndex][1].dstStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[inoIndex][1].dstAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
			transferBufferMemoryBarriers[inoIndex][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers[inoIndex][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers[inoIndex][1].buffer = deviceLocalBuffers[i];
			transferBufferMemoryBarriers[inoIndex][1].offset = bytesPerInBuffer + inBufferAlignment + bytesPerDeviceLocalInoutBuffer * j;
			transferBufferMemoryBarriers[inoIndex][1].size = bytesPerOutBuffer;

			// Availability operation: (vkCmdCopyBuffer; HV-out) -> device domain
			// Memory domain operation: device domain -> host domain
			transferBufferMemoryBarriers[inoIndex][2].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			transferBufferMemoryBarriers[inoIndex][2].pNext = NULL;
			transferBufferMemoryBarriers[inoIndex][2].srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[inoIndex][2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[inoIndex][2].dstStageMask  = VK_PIPELINE_STAGE_2_HOST_BIT;
			transferBufferMemoryBarriers[inoIndex][2].dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
			transferBufferMemoryBarriers[inoIndex][2].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			transferBufferMemoryBarriers[inoIndex][2].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			transferBufferMemoryBarriers[inoIndex][2].buffer = hostVisibleBuffers[i];
			transferBufferMemoryBarriers[inoIndex][2].offset = bytesPerInBuffer + bytesPerHostVisibleInoutBuffer * j;
			transferBufferMemoryBarriers[inoIndex][2].size = bytesPerOutBuffer;

			// Aquire operation: transfer QF -> DL-in
			// Visibility operation: device domain -> (vkCmdDispatch; DL-in)
			computeBufferMemoryBarriers[inoIndex][0].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			computeBufferMemoryBarriers[inoIndex][0].pNext = NULL;
			computeBufferMemoryBarriers[inoIndex][0].srcStageMask  = VK_PIPELINE_STAGE_2_NONE;
			computeBufferMemoryBarriers[inoIndex][0].srcAccessMask = VK_ACCESS_2_NONE;
			computeBufferMemoryBarriers[inoIndex][0].dstStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
#if IN_BUFFER_TYPE == 1
			computeBufferMemoryBarriers[inoIndex][0].dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
#elif IN_BUFFER_TYPE == 2
			computeBufferMemoryBarriers[inoIndex][0].dstAccessMask = VK_ACCESS_2_UNIFORM_READ_BIT;
#endif // IN_BUFFER_TYPE
			computeBufferMemoryBarriers[inoIndex][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers[inoIndex][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers[inoIndex][0].buffer = deviceLocalBuffers[i];
			computeBufferMemoryBarriers[inoIndex][0].offset = bytesPerDeviceLocalInoutBuffer * j;
			computeBufferMemoryBarriers[inoIndex][0].size = bytesPerInBuffer;

			// Availability operation: (vkCmdDispatch; DL-out) -> device domain
			// Release operation: DL-out -> transfer QF
			computeBufferMemoryBarriers[inoIndex][1].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR;
			computeBufferMemoryBarriers[inoIndex][1].pNext = NULL;
			computeBufferMemoryBarriers[inoIndex][1].srcStageMask  = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[inoIndex][1].srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			computeBufferMemoryBarriers[inoIndex][1].dstStageMask  = VK_PIPELINE_STAGE_2_NONE;
			computeBufferMemoryBarriers[inoIndex][1].dstAccessMask = VK_ACCESS_2_NONE;
			computeBufferMemoryBarriers[inoIndex][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers[inoIndex][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers[inoIndex][1].buffer = deviceLocalBuffers[i];
			computeBufferMemoryBarriers[inoIndex][1].offset = bytesPerInBuffer + inBufferAlignment + bytesPerDeviceLocalInoutBuffer * j;
			computeBufferMemoryBarriers[inoIndex][1].size = bytesPerOutBuffer;

			transferDependencyInfos[inoIndex][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			transferDependencyInfos[inoIndex][0].pNext = NULL;
			transferDependencyInfos[inoIndex][0].dependencyFlags = 0;
			transferDependencyInfos[inoIndex][0].memoryBarrierCount = 0;
			transferDependencyInfos[inoIndex][0].pMemoryBarriers = NULL;
			transferDependencyInfos[inoIndex][0].bufferMemoryBarrierCount = 2;
			transferDependencyInfos[inoIndex][0].pBufferMemoryBarriers = &transferBufferMemoryBarriers[inoIndex][0];
			transferDependencyInfos[inoIndex][0].imageMemoryBarrierCount = 0;
			transferDependencyInfos[inoIndex][0].pImageMemoryBarriers = NULL;

			transferDependencyInfos[inoIndex][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			transferDependencyInfos[inoIndex][1].pNext = NULL;
			transferDependencyInfos[inoIndex][1].dependencyFlags = 0;
			transferDependencyInfos[inoIndex][1].memoryBarrierCount = 0;
			transferDependencyInfos[inoIndex][1].pMemoryBarriers = NULL;
			transferDependencyInfos[inoIndex][1].bufferMemoryBarrierCount = 1;
			transferDependencyInfos[inoIndex][1].pBufferMemoryBarriers = &transferBufferMemoryBarriers[inoIndex][2];
			transferDependencyInfos[inoIndex][1].imageMemoryBarrierCount = 0;
			transferDependencyInfos[inoIndex][1].pImageMemoryBarriers = NULL;

			computeDependencyInfos[inoIndex][0].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			computeDependencyInfos[inoIndex][0].pNext = NULL;
			computeDependencyInfos[inoIndex][0].dependencyFlags = 0;
			computeDependencyInfos[inoIndex][0].memoryBarrierCount = 0;
			computeDependencyInfos[inoIndex][0].pMemoryBarriers = NULL;
			computeDependencyInfos[inoIndex][0].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[inoIndex][0].pBufferMemoryBarriers = &computeBufferMemoryBarriers[inoIndex][0];
			computeDependencyInfos[inoIndex][0].imageMemoryBarrierCount = 0;
			computeDependencyInfos[inoIndex][0].pImageMemoryBarriers = NULL;

			computeDependencyInfos[inoIndex][1].sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
			computeDependencyInfos[inoIndex][1].pNext = NULL;
			computeDependencyInfos[inoIndex][1].dependencyFlags = 0;
			computeDependencyInfos[inoIndex][1].memoryBarrierCount = 0;
			computeDependencyInfos[inoIndex][1].pMemoryBarriers = NULL;
			computeDependencyInfos[inoIndex][1].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[inoIndex][1].pBufferMemoryBarriers = &computeBufferMemoryBarriers[inoIndex][1];
			computeDependencyInfos[inoIndex][1].imageMemoryBarrierCount = 0;
			computeDependencyInfos[inoIndex][1].pImageMemoryBarriers = NULL;
		}
	}

	VkDependencyInfoKHR onetimeDependencyInfo;
	onetimeDependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR;
	onetimeDependencyInfo.pNext = NULL;
	onetimeDependencyInfo.dependencyFlags = 0;
	onetimeDependencyInfo.memoryBarrierCount = 0;
	onetimeDependencyInfo.pMemoryBarriers = NULL;
	onetimeDependencyInfo.bufferMemoryBarrierCount = inoutBuffersPerHeap;
	onetimeDependencyInfo.pBufferMemoryBarriers = onetimeBufferMemoryBarriers;
	onetimeDependencyInfo.imageMemoryBarrierCount = 0;
	onetimeDependencyInfo.pImageMemoryBarriers = NULL;

	VkCommandBufferBeginInfo onetimeCommandBufferBeginInfo;
	onetimeCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	onetimeCommandBufferBeginInfo.pNext = NULL;
	onetimeCommandBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	onetimeCommandBufferBeginInfo.pInheritanceInfo = NULL;

	VkCommandBufferBeginInfo transferCommandBufferBeginInfo;
	transferCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	transferCommandBufferBeginInfo.pNext = NULL;
	transferCommandBufferBeginInfo.flags = 0;
	transferCommandBufferBeginInfo.pInheritanceInfo = NULL;

	VkCommandBufferBeginInfo computeCommandBufferBeginInfo;
	computeCommandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	computeCommandBufferBeginInfo.pNext = NULL;
	computeCommandBufferBeginInfo.flags = 0;
	computeCommandBufferBeginInfo.pInheritanceInfo = NULL;

	GET_RESULT(vkBeginCommandBuffer(onetimeCommandBuffer, &onetimeCommandBufferBeginInfo))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkBeginCommandBuffer, 2, 'p', onetimeCommandBuffer, 'p', &onetimeCommandBufferBeginInfo)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		vkCmdCopyBuffer(
			onetimeCommandBuffer,
			hostVisibleBuffers[i],
			deviceLocalBuffers[i],
			inoutBuffersPerBuffer,
			inBufferCopies
		);
	}

	vkCmdPipelineBarrier2KHR(
		onetimeCommandBuffer,
		&onetimeDependencyInfo
	);

	GET_RESULT(vkEndCommandBuffer(onetimeCommandBuffer))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkEndCommandBuffer, 1, 'p', onetimeCommandBuffer)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	inoIndex = 0;
	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		for (uint32_t j = 0; j < inoutBuffersPerBuffer; j++, inoIndex++) {
			GET_RESULT(vkBeginCommandBuffer(transferCommandBuffers[inoIndex], &transferCommandBufferBeginInfo))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkBeginCommandBuffer, 2, 'p', transferCommandBuffers[inoIndex], 'p', &transferCommandBufferBeginInfo)
				free_CreateCommandsData(data);
				return false;
			}
#endif // NDEBUG

			if (transferQueueTimestampValidBits) {
				vkCmdResetQueryPool(
					transferCommandBuffers[inoIndex],
					queryPool,
					4 * inoIndex,
					2
				);

				vkCmdWriteTimestamp2KHR(
					transferCommandBuffers[inoIndex],
					VK_PIPELINE_STAGE_2_NONE,
					queryPool,
					4 * inoIndex
				);
			}

			vkCmdCopyBuffer(
				transferCommandBuffers[inoIndex],
				hostVisibleBuffers[i],
				deviceLocalBuffers[i],
				1,
				&inBufferCopies[j]
			);

			vkCmdPipelineBarrier2KHR(
				transferCommandBuffers[inoIndex],
				&transferDependencyInfos[inoIndex][0]
			);

			vkCmdCopyBuffer(
				transferCommandBuffers[inoIndex],
				deviceLocalBuffers[i],
				hostVisibleBuffers[i],
				1,
				&outBufferCopies[j]
			);

			vkCmdPipelineBarrier2KHR(
				transferCommandBuffers[inoIndex],
				&transferDependencyInfos[inoIndex][1]
			);

			if (transferQueueTimestampValidBits) {
				vkCmdWriteTimestamp2KHR(
					transferCommandBuffers[inoIndex],
					VK_PIPELINE_STAGE_2_COPY_BIT,
					queryPool,
					4 * inoIndex + 1
				);
			}

			GET_RESULT(vkEndCommandBuffer(transferCommandBuffers[inoIndex]))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkEndCommandBuffer, 1, 'p', transferCommandBuffers[inoIndex])
				free_CreateCommandsData(data);
				return false;
			}
#endif // NDEBUG

			GET_RESULT(vkBeginCommandBuffer(computeCommandBuffers[inoIndex], &computeCommandBufferBeginInfo))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkBeginCommandBuffer, 2, 'p', computeCommandBuffers[inoIndex], 'p', &computeCommandBufferBeginInfo)
				free_CreateCommandsData(data);
				return false;
			}
#endif // NDEBUG

			if (computeQueueTimestampValidBits) {
				vkCmdResetQueryPool(
					computeCommandBuffers[inoIndex],
					queryPool,
					4 * inoIndex + 2,
					2
				);

				vkCmdWriteTimestamp2KHR(
					computeCommandBuffers[inoIndex],
					VK_PIPELINE_STAGE_2_NONE,
					queryPool,
					4 * inoIndex + 2
				);
			}

			vkCmdPipelineBarrier2KHR(
				computeCommandBuffers[inoIndex],
				&computeDependencyInfos[inoIndex][0]
			);

			vkCmdBindPipeline(
				computeCommandBuffers[inoIndex],
				VK_PIPELINE_BIND_POINT_COMPUTE,
				pipeline
			);

			vkCmdBindDescriptorSets(
				computeCommandBuffers[inoIndex],
				VK_PIPELINE_BIND_POINT_COMPUTE,
				pipelineLayout,
				0,
				1,
				&descriptorSets[inoIndex],
				0,
				NULL
			);

			vkCmdDispatch(
				computeCommandBuffers[inoIndex],
				computeWorkGroupCount,
				1,
				1
			);

			vkCmdPipelineBarrier2KHR(
				computeCommandBuffers[inoIndex],
				&computeDependencyInfos[inoIndex][1]
			);

			if (computeQueueTimestampValidBits) {
				vkCmdWriteTimestamp2KHR(
					computeCommandBuffers[inoIndex],
					VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
					queryPool,
					4 * inoIndex + 3
				);
			}

			GET_RESULT(vkEndCommandBuffer(computeCommandBuffers[inoIndex]))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkEndCommandBuffer, 1, 'p', computeCommandBuffers[inoIndex])
				free_CreateCommandsData(data);
				return false;
			}
#endif // NDEBUG
		}
	}

	vkDestroyPipelineLayout(device, pipelineLayout, allocator);
	gpu->pipelineLayout = VK_NULL_HANDLE;

	VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo;
	semaphoreTypeCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
	semaphoreTypeCreateInfo.pNext = NULL;
	semaphoreTypeCreateInfo.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;
	semaphoreTypeCreateInfo.initialValue = 0;

	VkSemaphoreCreateInfo semaphoreCreateInfo;
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = &semaphoreTypeCreateInfo;
	semaphoreCreateInfo.flags = 0;

	GET_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, allocator, &gpu->onetimeSemaphore))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkCreateSemaphore, 4, 'p', device, 'p', &semaphoreCreateInfo, 'p', allocator, 'p', &gpu->onetimeSemaphore)
		free_CreateCommandsData(data);
		return false;
	}
#endif // NDEBUG

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++) {
		GET_RESULT(vkCreateSemaphore(device, &semaphoreCreateInfo, allocator, &semaphores[i]))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkCreateSemaphore, 4, 'p', device, 'p', &semaphoreCreateInfo, 'p', allocator, 'p', &semaphores[i])
			free_CreateCommandsData(data);
			return false;
		}
#endif // NDEBUG
	}

	free_CreateCommandsData(data);

#ifndef NDEBUG
	if(gpu->debugMessenger) {
		const uint32_t buffersPerDeviceMemory = gpu->buffersPerDeviceMemory;
		const uint32_t deviceMemoriesPerHeap = gpu->deviceMemoriesPerHeap;

		char objectName[134];
		VkDebugUtilsObjectNameInfoEXT debugUtilsObjectNameInfo;
		debugUtilsObjectNameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
		debugUtilsObjectNameInfo.pNext = NULL;

		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_COMMAND_POOL;
		debugUtilsObjectNameInfo.objectHandle = (uint64_t) onetimeCommandPool;
		debugUtilsObjectNameInfo.pObjectName = "Onetime command pool";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectHandle = (uint64_t) transferCommandPool;
		debugUtilsObjectNameInfo.pObjectName = "Transfer command pool";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectHandle = (uint64_t) computeCommandPool;
		debugUtilsObjectNameInfo.pObjectName = "Compute command pool";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
		debugUtilsObjectNameInfo.objectHandle = (uint64_t) onetimeCommandBuffer;
		debugUtilsObjectNameInfo.pObjectName = "Onetime command buffer";
		SET_DEBUG_NAME()

		debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
		debugUtilsObjectNameInfo.objectHandle = (uint64_t) gpu->onetimeSemaphore;
		debugUtilsObjectNameInfo.pObjectName = "Onetime semaphore";
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

					debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_COMMAND_BUFFER;
					debugUtilsObjectNameInfo.objectHandle = (uint64_t) transferCommandBuffers[inoIndex];
					SET_DEBUG_NAME()

					strcpy(objectName, "Compute command buffer ");
					strcat(objectName, specs);

					debugUtilsObjectNameInfo.objectHandle = (uint64_t) computeCommandBuffers[inoIndex];
					SET_DEBUG_NAME()

					strcpy(objectName, "Transfer-compute semaphore ");
					strcat(objectName, specs);

					debugUtilsObjectNameInfo.objectType = VK_OBJECT_TYPE_SEMAPHORE;
					debugUtilsObjectNameInfo.objectHandle = (uint64_t) semaphores[inoIndex];
					SET_DEBUG_NAME()
				}
			}
		}
	}
#endif // NDEBUG

	END_FUNC
	return true;
}

#if END_ON == 1
typedef struct WaitForInputData
{
	volatile bool input;
	pthread_mutex_t mutex;
} WaitForInputData_t;

static void* wait_for_input(void* const ptr)
{
	WaitForInputData_t* const data = (WaitForInputData_t*) ptr;
	int threadResult;

	puts("Calculating... press enter/return to stop\n");
	getchar();
	puts("Stopping...\n");

	GET_THRD_RESULT(pthread_mutex_lock(&data->mutex))
#ifndef NDEBUG
	if (threadResult) {
		PLOCK_FAILURE(&data->mutex)
		return NULL;
	}
#endif // NDEBUG

	data->input = true;

	GET_THRD_RESULT(pthread_mutex_unlock(&data->mutex))
#ifndef NDEBUG
	if (threadResult) {
		PUNLOCK_FAILURE(&data->mutex)
		return NULL;
	}
#endif // NDEBUG

	return ptr;
}

static bool has_input(const volatile bool* const input, pthread_mutex_t mutex)
{
	int threadResult;

	GET_THRD_RESULT(pthread_mutex_lock(&mutex))
#ifndef NDEBUG
	if (threadResult) {
		PLOCK_FAILURE(&mutex)
		return false;
	}
#endif // NDEBUG

	bool value = *input;

	GET_THRD_RESULT(pthread_mutex_unlock(&mutex))
#ifndef NDEBUG
	if (threadResult) {
		PUNLOCK_FAILURE(&mutex)
		return false;
	}
#endif // NDEBUG

	return value;
}
#endif // END_ON

static void writeInBuffer(
	value_t* const mappedHostVisibleInBuffer,
	value_t* const firstValue,
	const uint32_t valuesPerInoutBuffer,
	const uint32_t valuesPerHeap
)
{
	value_t value = *firstValue;
	for (uint32_t i = 0; i < valuesPerInoutBuffer; i++, value += 2)
		mappedHostVisibleInBuffer[i] = value;

	*firstValue += valuesPerHeap * 2;
}

static void readOutBuffer(
	const step_t* const mappedHostVisibleOutBuffer,
	value_t* const firstValue,
	value_t* const highestStepValues,
	step_t* const highestStepCounts,
	step_t* const longest,
	step_t* const count,
	value_t* const prev,
	const uint32_t valuesPerInoutBuffer
)
{
	value_t value      = *firstValue;
	step_t curCount    = *count;

	value_t value0mod1 = *prev;
	step_t  steps0mod1 = *longest;

	for (uint32_t i = 0; i < valuesPerInoutBuffer; i++, value += 2) {
		const step_t steps = mappedHostVisibleOutBuffer[i];

		if (value > 2 * value0mod1) {
			value0mod1 = 2 * value0mod1;
			steps0mod1 = steps0mod1 + 1;
			highestStepValues[curCount] = value0mod1;
			highestStepCounts[curCount] = steps0mod1;
			curCount++;
		}

		if (steps > steps0mod1) {
			value0mod1 = value;
			steps0mod1 = steps;
			highestStepValues[curCount] = value0mod1;
			highestStepCounts[curCount] = steps0mod1;
			curCount++;
		}
	}

	*firstValue = value;
	*prev       = value0mod1;
	*longest    = steps0mod1;
	*count      = curCount;
}

typedef struct SubmitCommandsData
{
	void* memory;
} SubmitCommandsData_t;

static void free_SubmitCommandsData(const SubmitCommandsData_t data)
{
	free(data.memory);
}

bool submit_commands(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkDevice device = volkGetLoadedDevice();
	const VkQueue transferQueue = gpu->transferQueue;
	const VkQueue computeQueue = gpu->computeQueue;
	const VkDeviceMemory* const hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceSize hostVisibleBufferAlignment = gpu->hostVisibleBufferAlignment;
	value_t* const* const mappedHostVisibleInBuffers = (value_t* const*) gpu->mappedHostVisibleInBuffers;
	const step_t* const* const mappedHostVisibleOutBuffers = (const step_t* const*) gpu->mappedHostVisibleOutBuffers;
	const VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;
	const VkCommandBuffer* const transferCommandBuffers = gpu->transferCommandBuffers;
	const VkCommandBuffer* const computeCommandBuffers = gpu->computeCommandBuffers;
	const VkCommandPool onetimeCommandPool = gpu->onetimeCommandPool;
	const VkSemaphore onetimeSemaphore = gpu->onetimeSemaphore;
	const VkSemaphore* const semaphores = gpu->semaphores;

	const VkDeviceSize bytesPerInBuffer = gpu->bytesPerInBuffer;
	const VkDeviceSize bytesPerOutBuffer = gpu->bytesPerOutBuffer;
	const uint32_t valuesPerInoutBuffer = gpu->valuesPerInoutBuffer;
	const uint32_t valuesPerHeap = gpu->valuesPerHeap;
	const uint32_t inoutBuffersPerBuffer = gpu->inoutBuffersPerBuffer;
	const uint32_t inoutBuffersPerHeap = gpu->inoutBuffersPerHeap;
	const uint32_t buffersPerDeviceMemory = gpu->buffersPerDeviceMemory;
	const uint32_t deviceMemoriesPerHeap = gpu->deviceMemoriesPerHeap;

	const uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	const uint32_t computeQueueTimestampValidBits = gpu->computeQueueTimestampValidBits;
	const float timestampPeriod = gpu->timestampPeriod;
	const VkQueryPool queryPool = gpu->queryPool;
	const VkAllocationCallbacks* const allocator = gpu->allocator;

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
		inoutBuffersPerHeap * sizeof(VkSemaphoreWaitInfo) +          // Transfer semaphore wait information
		inoutBuffersPerHeap * sizeof(VkSemaphoreWaitInfo);           // Compute semaphore wait information

	data.memory = malloc(size);
#ifndef NDEBUG
	if (!data.memory) {
		MALLOC_FAILURE(data.memory)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	value_t* const testedValues = (value_t*) data.memory;

	VkMappedMemoryRange* const hostVisibleInBuffersMappedMemoryRanges  = (VkMappedMemoryRange*) (testedValues                           + inoutBuffersPerHeap);
	VkMappedMemoryRange* const hostVisibleOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (hostVisibleInBuffersMappedMemoryRanges + inoutBuffersPerHeap);

	VkSubmitInfo2KHR* const transferSubmitInfos = (VkSubmitInfo2KHR*) (hostVisibleOutBuffersMappedMemoryRanges + inoutBuffersPerHeap);
	VkSubmitInfo2KHR* const computeSubmitInfos  = (VkSubmitInfo2KHR*) (transferSubmitInfos                     + inoutBuffersPerHeap);

	VkCommandBufferSubmitInfoKHR* const transferCommandBufferSubmitInfos = (VkCommandBufferSubmitInfoKHR*) (computeSubmitInfos               + inoutBuffersPerHeap);
	VkCommandBufferSubmitInfoKHR* const computeCommandBufferSubmitInfos  = (VkCommandBufferSubmitInfoKHR*) (transferCommandBufferSubmitInfos + inoutBuffersPerHeap);

	VkSemaphoreSubmitInfoKHR* const transferToComputeSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (computeCommandBufferSubmitInfos             + inoutBuffersPerHeap);
	VkSemaphoreSubmitInfoKHR* const transferToComputeWaitSemaphoreSubmitInfos   = (VkSemaphoreSubmitInfoKHR*) (transferToComputeSignalSemaphoreSubmitInfos + inoutBuffersPerHeap);
	VkSemaphoreSubmitInfoKHR* const computeToTransferSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (transferToComputeWaitSemaphoreSubmitInfos   + inoutBuffersPerHeap);
	VkSemaphoreSubmitInfoKHR* const computeToTransferWaitSemaphoreSubmitInfos   = (VkSemaphoreSubmitInfoKHR*) (computeToTransferSignalSemaphoreSubmitInfos + inoutBuffersPerHeap);

	VkSemaphoreWaitInfo* const transferSemaphoreWaitInfos = (VkSemaphoreWaitInfo*) (computeToTransferWaitSemaphoreSubmitInfos + inoutBuffersPerHeap);
	VkSemaphoreWaitInfo* const computeSemaphoreWaitInfos  = (VkSemaphoreWaitInfo*) (transferSemaphoreWaitInfos                + inoutBuffersPerHeap);

	const clock_t bmarkStart = clock();

#if END_ON == 1
	int threadResult;
	pthread_mutex_t inputMutex;
	GET_THRD_RESULT(pthread_mutex_init(&inputMutex, NULL))
#ifndef NDEBUG
	if (threadResult) {
		PINIT_FAILURE(&inputMutex, NULL)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	WaitForInputData_t inputData;
	inputData.mutex = inputMutex;
	inputData.input = false;

	pthread_t waitThread;
	GET_THRD_RESULT(pthread_create(&waitThread, NULL, wait_for_input, &inputData))
#ifndef NDEBUG
	if (threadResult) {
		PCREATE_FAILURE(&waitThread, NULL, wait_for_input, &inputData)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG
#endif // END_ON

	uint32_t inoIndex = 0;
	for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
		VkDeviceSize offset = 0;

		for (uint32_t j = 0; j < buffersPerDeviceMemory; j++, offset += hostVisibleBufferAlignment) {
			for (uint32_t k = 0; k < inoutBuffersPerBuffer; k++, inoIndex++) {
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].pNext = NULL;
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].memory = hostVisibleDeviceMemories[i];
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].offset = offset;
				hostVisibleInBuffersMappedMemoryRanges[inoIndex].size = bytesPerInBuffer;
				offset += bytesPerInBuffer;

				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].pNext = NULL;
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].memory = hostVisibleDeviceMemories[i];
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].offset = offset;
				hostVisibleOutBuffersMappedMemoryRanges[inoIndex].size = bytesPerOutBuffer;
				offset += bytesPerOutBuffer;
			}
		}
	}

	VkCommandBufferSubmitInfoKHR onetimeCommandBufferSubmitInfo;
	onetimeCommandBufferSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
	onetimeCommandBufferSubmitInfo.pNext = NULL;
	onetimeCommandBufferSubmitInfo.commandBuffer = onetimeCommandBuffer;
	onetimeCommandBufferSubmitInfo.deviceMask = 0;

	VkSemaphoreSubmitInfoKHR onetimeToComputeSignalSemaphoreSubmitInfo;
	onetimeToComputeSignalSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
	onetimeToComputeSignalSemaphoreSubmitInfo.pNext = NULL;
	onetimeToComputeSignalSemaphoreSubmitInfo.semaphore = onetimeSemaphore;
	onetimeToComputeSignalSemaphoreSubmitInfo.value = 1;
	onetimeToComputeSignalSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Needs to include release operation
	onetimeToComputeSignalSemaphoreSubmitInfo.deviceIndex = 0;

	VkSemaphoreSubmitInfoKHR onetimeToComputeWaitSemaphoreSubmitInfo;
	onetimeToComputeWaitSemaphoreSubmitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
	onetimeToComputeWaitSemaphoreSubmitInfo.pNext = NULL;
	onetimeToComputeWaitSemaphoreSubmitInfo.semaphore = onetimeSemaphore;
	onetimeToComputeWaitSemaphoreSubmitInfo.value = 1;
	onetimeToComputeWaitSemaphoreSubmitInfo.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Needs to include aquire operation
	onetimeToComputeWaitSemaphoreSubmitInfo.deviceIndex = 0;

	VkSubmitInfo2KHR onetimeSubmitInfo;
	onetimeSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
	onetimeSubmitInfo.pNext = NULL;
	onetimeSubmitInfo.flags = 0;
	onetimeSubmitInfo.waitSemaphoreInfoCount = 0;
	onetimeSubmitInfo.pWaitSemaphoreInfos = NULL;
	onetimeSubmitInfo.commandBufferInfoCount = 1;
	onetimeSubmitInfo.pCommandBufferInfos = &onetimeCommandBufferSubmitInfo;
	onetimeSubmitInfo.signalSemaphoreInfoCount = 1;
	onetimeSubmitInfo.pSignalSemaphoreInfos = &onetimeToComputeSignalSemaphoreSubmitInfo;

	VkSemaphoreWaitInfo onetimeSemaphoreWaitInfo;
	onetimeSemaphoreWaitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
	onetimeSemaphoreWaitInfo.pNext = NULL;
	onetimeSemaphoreWaitInfo.flags = 0;
	onetimeSemaphoreWaitInfo.semaphoreCount = 1;
	onetimeSemaphoreWaitInfo.pSemaphores = &onetimeSemaphore;
	onetimeSemaphoreWaitInfo.pValues = &onetimeToComputeSignalSemaphoreSubmitInfo.value;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++) {
		computeCommandBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
		computeCommandBufferSubmitInfos[i].pNext = NULL;
		computeCommandBufferSubmitInfos[i].commandBuffer = computeCommandBuffers[i];
		computeCommandBufferSubmitInfos[i].deviceMask = 0;

		transferCommandBufferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR;
		transferCommandBufferSubmitInfos[i].pNext = NULL;
		transferCommandBufferSubmitInfos[i].commandBuffer = transferCommandBuffers[i];
		transferCommandBufferSubmitInfos[i].deviceMask = 0;

		computeToTransferSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		computeToTransferSignalSemaphoreSubmitInfos[i].pNext = NULL;
		computeToTransferSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeToTransferSignalSemaphoreSubmitInfos[i].value = 1;
		computeToTransferSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		computeToTransferSignalSemaphoreSubmitInfos[i].deviceIndex = 0;

		computeToTransferWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		computeToTransferWaitSemaphoreSubmitInfos[i].pNext = NULL;
		computeToTransferWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeToTransferWaitSemaphoreSubmitInfos[i].value = 1;
		computeToTransferWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		computeToTransferWaitSemaphoreSubmitInfos[i].deviceIndex = 0;

		transferToComputeSignalSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		transferToComputeSignalSemaphoreSubmitInfos[i].pNext = NULL;
		transferToComputeSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferToComputeSignalSemaphoreSubmitInfos[i].value = 2;
		transferToComputeSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		transferToComputeSignalSemaphoreSubmitInfos[i].deviceIndex = 0;

		transferToComputeWaitSemaphoreSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR;
		transferToComputeWaitSemaphoreSubmitInfos[i].pNext = NULL;
		transferToComputeWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferToComputeWaitSemaphoreSubmitInfos[i].value = 2;
		transferToComputeWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
		transferToComputeWaitSemaphoreSubmitInfos[i].deviceIndex = 0;

		computeSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
		computeSubmitInfos[i].pNext = NULL;
		computeSubmitInfos[i].flags = 0;
		computeSubmitInfos[i].waitSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pWaitSemaphoreInfos = &onetimeToComputeWaitSemaphoreSubmitInfo;
		computeSubmitInfos[i].commandBufferInfoCount = 1;
		computeSubmitInfos[i].pCommandBufferInfos = &computeCommandBufferSubmitInfos[i];
		computeSubmitInfos[i].signalSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pSignalSemaphoreInfos = &computeToTransferSignalSemaphoreSubmitInfos[i];

		transferSubmitInfos[i].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR;
		transferSubmitInfos[i].pNext = NULL;
		transferSubmitInfos[i].flags = 0;
		transferSubmitInfos[i].waitSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pWaitSemaphoreInfos = &computeToTransferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos[i].commandBufferInfoCount = 1;
		transferSubmitInfos[i].pCommandBufferInfos = &transferCommandBufferSubmitInfos[i];
		transferSubmitInfos[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pSignalSemaphoreInfos = &transferToComputeSignalSemaphoreSubmitInfos[i];

		computeSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		computeSemaphoreWaitInfos[i].pNext = NULL;
		computeSemaphoreWaitInfos[i].flags = 0;
		computeSemaphoreWaitInfos[i].semaphoreCount = 1;
		computeSemaphoreWaitInfos[i].pSemaphores = &semaphores[i];
		computeSemaphoreWaitInfos[i].pValues = &computeToTransferSignalSemaphoreSubmitInfos[i].value;

		transferSemaphoreWaitInfos[i].sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
		transferSemaphoreWaitInfos[i].pNext = NULL;
		transferSemaphoreWaitInfos[i].flags = 0;
		transferSemaphoreWaitInfos[i].semaphoreCount = 1;
		transferSemaphoreWaitInfos[i].pSemaphores = &semaphores[i];
		transferSemaphoreWaitInfos[i].pValues = &transferToComputeSignalSemaphoreSubmitInfos[i].value;
	}

	SET_MIN_TEST_VALUE(testedValues[0])
	for (uint32_t i = 1; i < inoutBuffersPerHeap; i++)
		testedValues[i] = testedValues[i - 1] + 2 * valuesPerInoutBuffer;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		writeInBuffer(mappedHostVisibleInBuffers[i], &testedValues[i], valuesPerInoutBuffer, valuesPerHeap);

	GET_RESULT(vkFlushMappedMemoryRanges(device, inoutBuffersPerHeap, hostVisibleInBuffersMappedMemoryRanges))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkFlushMappedMemoryRanges, 3, 'p', device, 'u', inoutBuffersPerHeap, 'p', hostVisibleInBuffersMappedMemoryRanges)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkQueueSubmit2KHR(transferQueue, 1, &onetimeSubmitInfo, VK_NULL_HANDLE))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', transferQueue, 'u', 1, 'p', &onetimeSubmitInfo, 'p', VK_NULL_HANDLE)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkQueueSubmit2KHR(computeQueue, inoutBuffersPerHeap, computeSubmitInfos, VK_NULL_HANDLE))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', computeQueue, 'u', inoutBuffersPerHeap, 'p', computeSubmitInfos, 'p', VK_NULL_HANDLE)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		computeSubmitInfos[i].pWaitSemaphoreInfos = &transferToComputeWaitSemaphoreSubmitInfos[i];

	GET_RESULT(vkWaitSemaphores(device, &onetimeSemaphoreWaitInfo, UINT64_MAX))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkWaitSemaphores, 3, 'p', device, 'p', &onetimeSemaphoreWaitInfo, 'u', UINT64_MAX)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	vkDestroyCommandPool(device, onetimeCommandPool, allocator);
	gpu->onetimeCommandPool = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
		writeInBuffer(mappedHostVisibleInBuffers[i], &testedValues[i], valuesPerInoutBuffer, valuesPerHeap);

	GET_RESULT(vkFlushMappedMemoryRanges(device, inoutBuffersPerHeap, hostVisibleInBuffersMappedMemoryRanges))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkFlushMappedMemoryRanges, 3, 'p', device, 'u', inoutBuffersPerHeap, 'p', hostVisibleInBuffersMappedMemoryRanges)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	GET_RESULT(vkQueueSubmit2KHR(transferQueue, inoutBuffersPerHeap, transferSubmitInfos, VK_NULL_HANDLE))
#ifndef NDEBUG
	if (result != VK_SUCCESS) {
		VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', transferQueue, 'u', inoutBuffersPerHeap, 'p', transferSubmitInfos, 'p', VK_NULL_HANDLE)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG

	value_t tested;
	SET_MIN_TEST_VALUE(tested)
	value_t prev;
	SET_MAX_STEP_VALUE(prev)

	value_t num = 0;
	step_t count = 0;
	step_t longest = MAX_STEP_COUNT;

	value_t highestStepValues[256];
	step_t highestStepCounts[256];

	// ===== Enter main loop =====
#if END_ON == 1
	for (uint64_t i = 0; !has_input(&inputData.input, inputMutex); i++) {
#elif END_ON == 2
	for (uint64_t i = 0; i < 50; i++) {
#elif END_ON == 3
	for (uint64_t i = 0; !count; i++) {
#endif // END_ON

		const clock_t mainLoopBmarkStart = clock();
		const value_t initialValue = tested;

		float readBmarkTotal = 0.0F;
		float writeBmarkTotal = 0.0F;
		float waitComputeSemaphoreBmarkTotal = 0.0F;
		float waitTransferSemaphoreBmarkTotal = 0.0F;
		float computeBmarkTotal = 0.0F;
		float transferBmarkTotal = 0.0F;

		printf("Benchmarks #%llu\n", i + 1);

		for (uint32_t j = 0; j < inoutBuffersPerHeap; j++) {
			uint64_t timestamps[2];
			float computeBmark = 0.0F;
			float transferBmark = 0.0F;

			const clock_t waitComputeSemaphoreBmarkStart = clock();
			GET_RESULT(vkWaitSemaphores(device, &computeSemaphoreWaitInfos[j], UINT64_MAX))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkWaitSemaphores, 3, 'p', device, 'p', &computeSemaphoreWaitInfos[j], 'u', UINT64_MAX)
				free_SubmitCommandsData(data);
				return false;
			}
#endif // NDEBUG
			const clock_t waitComputeSemaphoreBmarkEnd = clock();

			if (computeQueueTimestampValidBits) {
				GET_RESULT(vkGetQueryPoolResults(device, queryPool, 4 * j + 2, 2, sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT))
#ifndef NDEBUG
				if (result != VK_SUCCESS) {
					VULKAN_FAILURE(vkGetQueryPoolResults, 8, 'p', device, 'p', queryPool, 'u', 4 * j + 2, 'u', 2, 'u', sizeof(timestamps), 'p', timestamps, 'u', sizeof(timestamps[0]), 'x', VK_QUERY_RESULT_64_BIT)
					free_SubmitCommandsData(data);
					return false;
				}
#endif // NDEBUG
				computeBmark = (float) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000.0F;
			}

			computeToTransferSignalSemaphoreSubmitInfos[j].value += 2;
			GET_RESULT(vkQueueSubmit2KHR(computeQueue, 1, &computeSubmitInfos[j], VK_NULL_HANDLE))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', computeQueue, 'u', 1, 'p', &computeSubmitInfos[j], 'p', VK_NULL_HANDLE)
				free_SubmitCommandsData(data);
				return false;
			}
#endif // NDEBUG
			transferToComputeWaitSemaphoreSubmitInfos[j].value += 2;

			const clock_t waitTransferSemaphoreBmarkStart = clock();
			GET_RESULT(vkWaitSemaphores(device, &transferSemaphoreWaitInfos[j], UINT64_MAX))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkWaitSemaphores, 3, 'p', device, 'p', &transferSemaphoreWaitInfos[j], 'u', UINT64_MAX)
				free_SubmitCommandsData(data);
				return false;
			}
#endif // NDEBUG
			const clock_t waitTransferSemaphoreBmarkEnd = clock();

			if (transferQueueTimestampValidBits) {
				GET_RESULT(vkGetQueryPoolResults(device, queryPool, 4 * j, 2, sizeof(timestamps), timestamps, sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT))
#ifndef NDEBUG
				if (result != VK_SUCCESS) {
					VULKAN_FAILURE(vkGetQueryPoolResults, 8, 'p', device, 'p', queryPool, 'u', 4 * j, 'u', 2, 'u', sizeof(timestamps), 'p', timestamps, 'u', sizeof(timestamps[0]), 'x', VK_QUERY_RESULT_64_BIT)
					free_SubmitCommandsData(data);
					return false;
				}
#endif // NDEBUG
				transferBmark = (float) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000.0F;
			}

			const clock_t writeBmarkStart = clock();
			writeInBuffer(mappedHostVisibleInBuffers[j], &testedValues[j], valuesPerInoutBuffer, valuesPerHeap);
			const clock_t writeBmarkEnd = clock();

			GET_RESULT(vkFlushMappedMemoryRanges(device, 1, &hostVisibleInBuffersMappedMemoryRanges[j]))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkFlushMappedMemoryRanges, 3, 'p', device, 'u', 1, 'p', &hostVisibleInBuffersMappedMemoryRanges[j])
				free_SubmitCommandsData(data);
				return false;
			}
#endif // NDEBUG

			GET_RESULT(vkInvalidateMappedMemoryRanges(device, 1, &hostVisibleOutBuffersMappedMemoryRanges[j]))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkInvalidateMappedMemoryRanges, 3, 'p', device, 'u', 1, 'p', &hostVisibleOutBuffersMappedMemoryRanges[j])
				free_SubmitCommandsData(data);
				return false;
			}
#endif // NDEBUG

			const clock_t readBmarkStart = clock();
			readOutBuffer(mappedHostVisibleOutBuffers[j], &tested, highestStepValues, highestStepCounts, &longest, &count, &prev, valuesPerInoutBuffer);
			const clock_t readBmarkEnd = clock();

			computeToTransferWaitSemaphoreSubmitInfos[j].value += 2;
			transferToComputeSignalSemaphoreSubmitInfos[j].value += 2;
			GET_RESULT(vkQueueSubmit2KHR(transferQueue, 1, &transferSubmitInfos[j], VK_NULL_HANDLE))
#ifndef NDEBUG
			if (result != VK_SUCCESS) {
				VULKAN_FAILURE(vkQueueSubmit2KHR, 4, 'p', transferQueue, 'u', 1, 'p', &transferSubmitInfos[j], 'p', VK_NULL_HANDLE)
				return NULL;
			}
#endif // NDEBUG

			const float readBmark = get_benchmark(readBmarkStart, readBmarkEnd);
			const float writeBmark = get_benchmark(writeBmarkStart, writeBmarkEnd);
			const float waitComputeSemaphoreBmark = get_benchmark(waitComputeSemaphoreBmarkStart, waitComputeSemaphoreBmarkEnd);
			const float waitTransferSemaphoreBmark = get_benchmark(waitTransferSemaphoreBmarkStart, waitTransferSemaphoreBmarkEnd);

			readBmarkTotal += readBmark;
			writeBmarkTotal += writeBmark;
			computeBmarkTotal += computeBmark;
			transferBmarkTotal += transferBmark;
			waitComputeSemaphoreBmarkTotal += waitComputeSemaphoreBmark;
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
				(double) readBmark, (double) writeBmark,
				(double) computeBmark, (double) transferBmark,
				(double) waitComputeSemaphoreBmark, (double) waitTransferSemaphoreBmark
			);
		}

		num += valuesPerHeap;
		const clock_t mainLoopBmarkEnd = clock();
		const float mainLoopBmark = get_benchmark(mainLoopBmarkStart, mainLoopBmarkEnd);

		printf(
			"\tMain loop: %.0fms\n"
			"\tReading buffers:      (total) %4.0fms, (avg) %6.1fms\n"
			"\tWriting buffers:      (total) %4.0fms, (avg) %6.1fms\n"
			"\tCompute execution:    (total) %4.0fms, (avg) %6.1fms\n"
			"\tTransfer execution:   (total) %4.0fms, (avg) %6.1fms\n"
			"\tWaiting for compute:  (total) %4.0fms, (avg) %6.1fms\n"
			"\tWaiting for transfer: (total) %4.0fms, (avg) %6.1fms\n"
			"\tInitial value: 0X %016llX %016llX\n"
			"\tFinal value:   0X %016llX %016llX\n\n",
			(double) mainLoopBmark,
			(double) readBmarkTotal,  (double) (readBmarkTotal  / inoutBuffersPerHeap),
			(double) writeBmarkTotal, (double) (writeBmarkTotal / inoutBuffersPerHeap),
			(double) computeBmarkTotal,  (double) (computeBmarkTotal  / inoutBuffersPerHeap),
			(double) transferBmarkTotal, (double) (transferBmarkTotal / inoutBuffersPerHeap),
			(double) waitComputeSemaphoreBmarkTotal,  (double) (waitComputeSemaphoreBmarkTotal  / inoutBuffersPerHeap),
			(double) waitTransferSemaphoreBmarkTotal, (double) (waitTransferSemaphoreBmarkTotal / inoutBuffersPerHeap),
			GET_TOP_128BIT_INT(initialValue), GET_BOTTOM_128BIT_INT(initialValue),
			GET_TOP_128BIT_INT(tested - 2),   GET_BOTTOM_128BIT_INT(tested - 2)
		);
	}
	NEWLINE

	const clock_t bmarkEnd = clock();
	const float bmark = get_benchmark(bmarkStart, bmarkEnd);

	printf(
		"Set of starting values tested: [0X %016llX %016llX, 0X %016llX %016llX]\n"
		"Continue on: 0X %016llX %016llX\n"
		"Highest step counts (%hu):\n",
		MIN_TEST_VALUE_TOP, MIN_TEST_VALUE_BOTTOM,
		GET_TOP_128BIT_INT(tested - 2), GET_BOTTOM_128BIT_INT(tested - 2),
		GET_TOP_128BIT_INT(tested),     GET_BOTTOM_128BIT_INT(tested),
		count
	);

	for (uint32_t i = 0; i < count; i++) {
		printf(
			"\t%u) steps(0X %016llX %016llX) = %hu\n",
			i + 1, GET_TOP_128BIT_INT(highestStepValues[i]), GET_BOTTOM_128BIT_INT(highestStepValues[i]), highestStepCounts[i]
		);
	}
	NEWLINE

	printf(
		"Time: %.0fms\n"
		"Speed: %.0f/s\n",
		(double) bmark, 1000 * num / (double) bmark
	);

#if END_ON == 1
	if (inputData.input) {
		void* threadReturn;
		GET_THRD_RESULT(pthread_join(waitThread, &threadReturn))
#ifndef NDEBUG
		if (threadResult || !threadReturn) {
			PJOIN_FAILURE(waitThread, &threadReturn)
			free_SubmitCommandsData(data);
			return false;
		}
#endif // NDEBUG
	}
	else {
		GET_THRD_RESULT(pthread_cancel(waitThread))
#ifndef NDEBUG
		if (threadResult) {
			PCANCEL_FAILURE(waitThread)
			free_SubmitCommandsData(data);
			return false;
		}
#endif // NDEBUG
	}

	GET_THRD_RESULT(pthread_mutex_destroy(&inputMutex))
#ifndef NDEBUG
	if (threadResult) {
		PDESTROY_FAILURE(&inputMutex)
		free_SubmitCommandsData(data);
		return false;
	}
#endif // NDEBUG
#endif // END_ON

	free_SubmitCommandsData(data);

	END_FUNC
	return true;
}

bool destroy_gpu(Gpu_t* const gpu)
{
	BEGIN_FUNC

	const VkInstance instance = volkGetLoadedInstance();
	const VkDevice device = volkGetLoadedDevice();
	const VkDeviceMemory* const hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* const deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	const VkBuffer* const hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer* const deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	const VkDescriptorPool descriptorPool = gpu->descriptorPool;
	const VkShaderModule shaderModule = gpu->shaderModule;
	const VkPipelineCache pipelineCache = gpu->pipelineCache;
	const VkPipelineLayout pipelineLayout = gpu->pipelineLayout;
	const VkPipeline pipeline = gpu->pipeline;
	const VkCommandPool onetimeCommandPool = gpu->onetimeCommandPool;
	const VkCommandPool transferCommandPool = gpu->transferCommandPool;
	const VkCommandPool computeCommandPool = gpu->computeCommandPool;
	const VkSemaphore onetimeSemaphore = gpu->onetimeSemaphore;
	const VkSemaphore* const semaphores = gpu->semaphores;
	const VkQueryPool queryPool = gpu->queryPool;
	void* const dynamicMemory = gpu->dynamicMemory;

	const uint32_t inoutBuffersPerHeap = gpu->inoutBuffersPerHeap;
	const uint32_t buffersPerHeap = gpu->buffersPerHeap;
	const uint32_t deviceMemoriesPerHeap = gpu->deviceMemoriesPerHeap;

	const VkAllocationCallbacks* const allocator = gpu->allocator;
	VkResult result;
	size_t writeResult;
	int fileResult;

	if(pipelineCache) {
		size_t size;
		GET_RESULT(vkGetPipelineCacheData(device, pipelineCache, &size, NULL))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkGetPipelineCacheData, 4, 'p', device, 'p', pipelineCache, 'p', &size, 'p', NULL)
			return false;
		}
#endif // NDEBUG

		void* const cache = malloc(size);
#ifndef NDEBUG
		if (!cache) {
			MALLOC_FAILURE(cache)
			return false;
		}
#endif // NDEBUG

		GET_RESULT(vkGetPipelineCacheData(device, pipelineCache, &size, cache))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkGetPipelineCacheData, 4, 'p', device, 'p', pipelineCache, 'p', &size, 'p', cache)
			free(cache);
			return false;
		}
#endif // NDEBUG

		FILE* const file = fopen(PIPELINE_CACHE_NAME, "wb");
#ifndef NDEBUG
		if (!file) {
			FOPEN_FAILURE(PIPELINE_CACHE_NAME, "wb")
			free(cache);
			return false;
		}
#endif // NDEBUG

		GET_WRITE_RESULT(fwrite(cache, (size_t) 1, size, file))
#ifndef NDEBUG
		if (writeResult != size) {
			FWRITE_FAILURE(cache, 1, size)
			fclose(file);
			free(cache);
			return false;
		}
#endif // NDEBUG

		GET_FILE_RESULT(fclose(file))
#ifndef NDEBUG
		if (fileResult) {
			FCLOSE_FAILURE()
			free(cache);
			return false;
		}
#endif // NDEBUG

		vkDestroyPipelineCache(device, pipelineCache, allocator);
		free(cache);
	}

	if(device) {
		vkDestroyDescriptorSetLayout(device, descriptorSetLayout, allocator);
		vkDestroyShaderModule(device, shaderModule, allocator);
		vkDestroyPipelineLayout(device, pipelineLayout, allocator);

		// Make sure no command buffers are in the pending state
		GET_RESULT(vkDeviceWaitIdle(device))
#ifndef NDEBUG
		if (result != VK_SUCCESS) {
			VULKAN_FAILURE(vkDeviceWaitIdle, 1, 'p', device)
			return false;
		}
#endif // NDEBUG

		vkDestroySemaphore(device, onetimeSemaphore, allocator);
		for (uint32_t i = 0; i < inoutBuffersPerHeap; i++)
			vkDestroySemaphore(device, semaphores[i], allocator);

		vkDestroyCommandPool(device, onetimeCommandPool, allocator);
		vkDestroyCommandPool(device, computeCommandPool, allocator);
		vkDestroyCommandPool(device, transferCommandPool, allocator);

		vkDestroyPipeline(device, pipeline, allocator);
		vkDestroyQueryPool(device, queryPool, allocator);
		vkDestroyDescriptorPool(device, descriptorPool, allocator);

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			vkDestroyBuffer(device, hostVisibleBuffers[i], allocator);
			vkDestroyBuffer(device, deviceLocalBuffers[i], allocator);
		}

		for (uint32_t i = 0; i < deviceMemoriesPerHeap; i++) {
			vkFreeMemory(device, hostVisibleDeviceMemories[i], allocator);
			vkFreeMemory(device, deviceLocalDeviceMemories[i], allocator);
		}

		vkDestroyDevice(device, allocator);
	}

#ifndef NDEBUG
	if(gpu->debugMessenger) {
		vkDestroyDebugUtilsMessengerEXT(instance, gpu->debugMessenger, allocator);
	}
#endif // NDEBUG

	if(instance) {
		vkDestroyInstance(instance, allocator);
	}

	volkFinalize();
	free(dynamicMemory);

	END_FUNC
	return true;
}
