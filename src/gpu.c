/* 
 * Copyright (C) 2024 Seth McDonald <seth.i.mcdonald@gmail.com>
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
#include "util.h"


static void print_version(void)
{
	printf(
		"%s %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n%s\n%s\n\n",
		PROGRAM_NAME, PROGRAM_VER_MAJOR, PROGRAM_VER_MINOR, PROGRAM_VER_PATCH, PROGRAM_COPYRIGHT, PROGRAM_LICENCE);
}

static void print_help(void)
{
	printf(
		"Usage: %s [options]\n"
		"\n"
		"Options:\n"
		"  --version                   Output version and licence information, then terminate.\n"
		"  --help                      Output this hopefully helpful CLI overview, then terminate.\n"
		"\n"
		"  --silent                    Output no diagnostic information.\n"
		"  --quiet                     Output minimal diagnostic information.\n"
		"  --verbose                   Output maximal diagnostic information.\n"
		"\n"
		"  --int16                     Prefer shaders using 16-bit integers where appropriate.\n"
		"  --int64                     Prefer shaders using 64-bit integers where appropriate.\n"
		"\n"
		"  --ext-layers                Enable the Khronos extension layers, if present.\n"
		"  --profile-layers            Enable the Khronos profiles layer, if present.\n"
		"  --validation                Enable the Khronos validation layer, if present.\n"
		"\n"
		"  --restart                   Restart the simulation. Do not overwrite previous progress.\n"
		"  --no-query-benchmarks       Do not benchmark Vulkan commands via queries.\n"
		"  --log-allocations           Log all memory allocations performed by Vulkan to %s.\n"
		"  --capture-pipelines         Output pipeline data captured via %s, if present.\n"
		"\n"
		"  --iter-size <value>         Bit precision of the iterating value in shaders. Must be 128 or 256.\n"
		"  --max-loops <value>         Maximum number of iterations of the main loop. Must be a non-negative integer.\n"
		"  --max-memory <value>        Maximum proportion of available GPU heap memory to be allocated. Must be within "
			"(0, 1].\n",
		PROGRAM_EXE, ALLOC_LOG_NAME, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);
}

static void DyArray_destroy_stub(void* restrict array)
{
	DyArray_destroy((DyArray) array);
}

typedef struct DyData
{
	void* data;
	void (*free)(void*);
} DyData;

static void free_recursive(restrict DyArray array)
{
	size_t count = DyArray_size(array);

	for (size_t i = 0; i < count; i++) {
		DyData dyData;
		DyArray_get(array, &dyData, i);
		dyData.free(dyData.data);
	}

	DyArray_destroy(array);
}


bool parse_cmdline(Gpu* restrict gpu, int argc, char** restrict argv)
{
	*gpu = (Gpu) {0};

	gpu->outputLevel = CLI_OUTPUT_DEFAULT;

	gpu->iterSize  = 128;
	gpu->maxLoops  = UINT64_MAX;
	gpu->maxMemory = .4f;

	gpu->preferInt16 = false;
	gpu->preferInt64 = false;

	gpu->extensionLayers  = false;
	gpu->profileLayers    = false;
	gpu->validationLayers = false;

	gpu->restartCount      = false;
	gpu->queryBenchmarking = true;
	gpu->logAllocations    = false;
	gpu->capturePipelines  = false;

	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];
		char* endChar;

		if (!strcmp(arg, "--version")) { print_version(); return false; }
		if (!strcmp(arg, "--help"))    { print_help();    return false; }

		if (!strcmp(arg, "--silent"))  { gpu->outputLevel = CLI_OUTPUT_SILENT;  continue; }
		if (!strcmp(arg, "--quiet"))   { gpu->outputLevel = CLI_OUTPUT_QUIET;   continue; }
		if (!strcmp(arg, "--verbose")) { gpu->outputLevel = CLI_OUTPUT_VERBOSE; continue; }

		if (!strcmp(arg, "--int16")) { gpu->preferInt16 = true; continue; }
		if (!strcmp(arg, "--int64")) { gpu->preferInt64 = true; continue; }

		if (!strcmp(arg, "--ext-layers"))     { gpu->extensionLayers  = true; continue; }
		if (!strcmp(arg, "--profile-layers")) { gpu->profileLayers    = true; continue; }
		if (!strcmp(arg, "--validation"))     { gpu->validationLayers = true; continue; }

		if (!strcmp(arg, "--restart"))             { gpu->restartCount      = true;  continue; }
		if (!strcmp(arg, "--no-query-benchmarks")) { gpu->queryBenchmarking = false; continue; }
		if (!strcmp(arg, "--log-allocations"))     { gpu->logAllocations    = true;  continue; }
		if (!strcmp(arg, "--capture-pipelines"))   { gpu->capturePipelines  = true;  continue; }

		if (!strcmp(arg, "--iter-size")) {
			if (i + 1 == argc) { printf("Warning: ignoring incomplete argument %d '%s'\n", i, arg); continue; }

			i++;
			arg = argv[i];

			unsigned long iterSize = strtoul(arg, &endChar, 0);

			if (*endChar != '\0') {
				printf("Warning: partially interpreting argument %d '%s' as %lu\n", i, arg, iterSize); }
			if (iterSize != 128 && iterSize != 256) {
				printf("Warning: ignoring invalid argument %d '%s'\n", i, arg); continue; }

			gpu->iterSize = (uint32_t) iterSize;
			continue;
		}

		if (!strcmp(arg, "--max-loops")) {
			if (i + 1 == argc) { printf("Warning: ignoring incomplete argument %d '%s'\n", i, arg); continue; }

			i++;
			arg = argv[i];

			unsigned long long maxLoops = strtoull(arg, &endChar, 0);

			if (*endChar != '\0') {
				printf("Warning: partially interpreting argument %d '%s' as %llu\n", i, arg, maxLoops); }

			gpu->maxLoops = (uint64_t) maxLoops;
			continue;
		}

		if (!strcmp(arg, "--max-memory")) {
			if (i + 1 == argc) { printf("Warning: ignoring incomplete argument %d '%s'\n", i, arg); continue; }

			i++;
			arg = argv[i];

			float maxMemory = strtof(arg, &endChar);

			if (*endChar != '\0') {
				printf("Warning: partially interpreting argument %d '%s' as %f\n", i, arg, (double) maxMemory); }
			if (maxMemory <= 0 || 1 < maxMemory) {
				printf("Warning: ignoring invalid argument %d '%s'\n", i, arg); continue; }

			gpu->maxMemory = maxMemory;
			continue;
		}

		printf("Warning: ignoring unknown argument %d '%s'\n", i, arg);
	}

	NEWLINE();

	return true;
}

bool create_instance(Gpu* restrict gpu)
{
	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 3);
	if EXPECT_FALSE (!dyMem) return false;

	vkres = volkInitialize();
	if EXPECT_FALSE (vkres) { VKINIT_FAILURE(vkres); free_recursive(dyMem); return false; }

	uint32_t instApiVersion = volkGetInstanceVersion();
	if EXPECT_FALSE (instApiVersion == VK_API_VERSION_1_0) {
		VKVERS_FAILURE(instApiVersion); free_recursive(dyMem); return false; }

	if (gpu->logAllocations) {
		g_allocator = &g_allocationCallbacks;

		bool bres = init_alloc_logfile();
		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

#ifndef NDEBUG
	bool bres = init_debug_logfile();
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
		VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	debugUtilsMessengerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debugUtilsMessengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debugUtilsMessengerCreateInfo.pfnUserCallback = debug_callback;
	debugUtilsMessengerCreateInfo.pUserData       = &g_callbackData;
#endif

	uint32_t layerPropertyCount;
	uint32_t extensionPropertyCount;

	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerPropertyCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extensionPropertyCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	size_t size =
		layerPropertyCount * sizeof(VkLayerProperties) + extensionPropertyCount * sizeof(VkExtensionProperties);

	void* p = malloc(size);
	if EXPECT_FALSE (!p && size) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkLayerProperties*     layersProperties     = (VkLayerProperties*) p;
	VkExtensionProperties* extensionsProperties = (VkExtensionProperties*) (layersProperties + layerPropertyCount);

	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerPropertyCount, layersProperties);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extensionPropertyCount, extensionsProperties);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	DyArray enabledLayers = DyArray_create(sizeof(const char*), 4);
	if EXPECT_FALSE (!enabledLayers) { free_recursive(dyMem); return false; }

	dyData = (DyData) {enabledLayers, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < layerPropertyCount; i++) {
		const char* layerName = layersProperties[i].layerName;

		if (
			(gpu->extensionLayers && (
				!strcmp(layerName, VK_KHR_SYNCHRONIZATION_2_LAYER_NAME) ||
				!strcmp(layerName, VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME))) ||
			(gpu->profileLayers && !strcmp(layerName, VK_KHR_PROFILES_LAYER_NAME)) ||
			(gpu->validationLayers && !strcmp(layerName, VK_KHR_VALIDATION_LAYER_NAME)))
		{
			DyArray_append(enabledLayers, &layerName);
			continue;
		}
	}

	const void*  nextChain = NULL;
	const void** next      = &nextChain;

	bool usingPortabilityEnumeration = false;
	bool usingDebugUtils             = false;

	DyArray enabledExtensions = DyArray_create(sizeof(const char*), 2);
	if EXPECT_FALSE (!enabledExtensions) { free_recursive(dyMem); return false; }

	dyData = (DyData) {enabledExtensions, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < extensionPropertyCount; i++) {
		const char* extensionName = extensionsProperties[i].extensionName;

		if (!strcmp(extensionName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			DyArray_append(enabledExtensions, &extensionName);
			usingPortabilityEnumeration = true;
			continue;
		}

#ifndef NDEBUG
		if (!strcmp(extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			PNEXT_ADD(next, debugUtilsMessengerCreateInfo);
			DyArray_append(enabledExtensions, &extensionName);
			usingDebugUtils = true;
			continue;
		}
#endif
	}

	VkApplicationInfo applicationInfo  = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	applicationInfo.pApplicationName   = PROGRAM_NAME;
	applicationInfo.applicationVersion = PROGRAM_VERSION;
	applicationInfo.apiVersion         = VK_API_VERSION_1_3;

	uint32_t     enabledLayerCount = (uint32_t) DyArray_size(enabledLayers);
	const char** enabledLayerNames = (const char**) DyArray_raw(enabledLayers);

	uint32_t     enabledExtensionCount = (uint32_t) DyArray_size(enabledExtensions);
	const char** enabledExtensionNames = (const char**) DyArray_raw(enabledExtensions);

	VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instanceCreateInfo.pNext                = nextChain;
	instanceCreateInfo.flags = usingPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceCreateInfo.pApplicationInfo        = &applicationInfo;
	instanceCreateInfo.enabledLayerCount       = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames     = enabledLayerNames;
	instanceCreateInfo.enabledExtensionCount   = enabledExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if (gpu->outputLevel > CLI_OUTPUT_DEFAULT) {
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
	VK_CALL_RES(vkCreateInstance, &instanceCreateInfo, g_allocator, &instance);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	free_recursive(dyMem);
	volkLoadInstanceOnly(instance);

#ifndef NDEBUG
	if (usingDebugUtils) {
		VK_CALL_RES(
			vkCreateDebugUtilsMessengerEXT, instance, &debugUtilsMessengerCreateInfo, g_allocator,
			&gpu->debugUtilsMessenger);
	}
#endif

	return true;
}

bool select_device(Gpu* restrict gpu)
{
	VkInstance instance = volkGetLoadedInstance();

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 2);
	if EXPECT_FALSE (!dyMem) return false;

	uint32_t physicalDeviceCount;
	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &physicalDeviceCount, NULL);

	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	if EXPECT_FALSE (!physicalDeviceCount) {
		fputs("Runtime failure\nNo physical devices are accessible to the Vulkan instance\n\n", stderr);
		free_recursive(dyMem);
		return false;
	}

	size_t size =
		physicalDeviceCount * sizeof(VkPhysicalDevice) +
		physicalDeviceCount * sizeof(VkQueueFamilyProperties2*) +
		physicalDeviceCount * sizeof(VkExtensionProperties*) +
		physicalDeviceCount * sizeof(VkPhysicalDeviceMemoryProperties2) +
		physicalDeviceCount * sizeof(VkPhysicalDeviceProperties2) +
		physicalDeviceCount * sizeof(VkPhysicalDeviceFeatures2) +
		physicalDeviceCount * sizeof(VkPhysicalDevice16BitStorageFeatures) +
		physicalDeviceCount * sizeof(VkPhysicalDevice8BitStorageFeaturesKHR) +
		physicalDeviceCount * sizeof(uint32_t) * 2;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkPhysicalDevice* physicalDevices = (VkPhysicalDevice*) p;

	VkQueueFamilyProperties2** queueFamiliesProperties2 = (VkQueueFamilyProperties2**) (
		physicalDevices + physicalDeviceCount);
	VkExtensionProperties** extensionsProperties = (VkExtensionProperties**) (
		queueFamiliesProperties2 + physicalDeviceCount);

	VkPhysicalDeviceMemoryProperties2* physicalDevicesMemoryProperties2 = (VkPhysicalDeviceMemoryProperties2*) (
		extensionsProperties + physicalDeviceCount);
	VkPhysicalDeviceProperties2* physicalDevicesProperties2 = (VkPhysicalDeviceProperties2*) (
		physicalDevicesMemoryProperties2 + physicalDeviceCount);

	VkPhysicalDeviceFeatures2* physicalDevicesFeatures2 = (VkPhysicalDeviceFeatures2*) (
		physicalDevicesProperties2 + physicalDeviceCount);
	VkPhysicalDevice16BitStorageFeatures* physicalDevices16BitStorageFeatures =
		(VkPhysicalDevice16BitStorageFeatures*) (physicalDevicesFeatures2 + physicalDeviceCount);
	VkPhysicalDevice8BitStorageFeaturesKHR* physicalDevices8BitStorageFeatures =
		(VkPhysicalDevice8BitStorageFeaturesKHR*) (physicalDevices16BitStorageFeatures + physicalDeviceCount);

	uint32_t* extensionPropertyCounts   = (uint32_t*) (physicalDevices8BitStorageFeatures + physicalDeviceCount);
	uint32_t* queueFamilyPropertyCounts = (uint32_t*) (extensionPropertyCounts + physicalDeviceCount);

	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &physicalDeviceCount, physicalDevices);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	size_t queueFamilyTotal = 0;
	size_t extensionTotal   = 0;

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, physicalDevices[i], &queueFamilyPropertyCounts[i], NULL);

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, physicalDevices[i], NULL, &extensionPropertyCounts[i], NULL);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		queueFamilyTotal += queueFamilyPropertyCounts[i];
		extensionTotal   += extensionPropertyCounts[i];
	}

	size = queueFamilyTotal * sizeof(VkQueueFamilyProperties2) + extensionTotal * sizeof(VkExtensionProperties);

	p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	queueFamiliesProperties2[0] = (VkQueueFamilyProperties2*) p;
	extensionsProperties[0]     = (VkExtensionProperties*) (queueFamiliesProperties2[0] + queueFamilyTotal);

	for (uint32_t i = 1; i < physicalDeviceCount; i++) {
		queueFamiliesProperties2[i] = queueFamiliesProperties2[i - 1] + queueFamilyPropertyCounts[i - 1];
		extensionsProperties[i]     = extensionsProperties[i - 1] + extensionPropertyCounts[i - 1];
	}

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		VK_CALL_RES(
			vkEnumerateDeviceExtensionProperties, physicalDevices[i], NULL, &extensionPropertyCounts[i],
			extensionsProperties[i]);

		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		for (uint32_t j = 0; j < queueFamilyPropertyCounts[i]; j++) {
			queueFamiliesProperties2[i][j] = (VkQueueFamilyProperties2) {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};
		}

		VK_CALL(
			vkGetPhysicalDeviceQueueFamilyProperties2, physicalDevices[i], &queueFamilyPropertyCounts[i],
			queueFamiliesProperties2[i]);

		physicalDevicesMemoryProperties2[i] = (VkPhysicalDeviceMemoryProperties2) {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevices[i], &physicalDevicesMemoryProperties2[i]);

		physicalDevicesProperties2[i] = (VkPhysicalDeviceProperties2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevices[i], &physicalDevicesProperties2[i]);

		physicalDevices8BitStorageFeatures[i] = (VkPhysicalDevice8BitStorageFeaturesKHR) {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR};

		physicalDevices16BitStorageFeatures[i] = (VkPhysicalDevice16BitStorageFeatures) {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
		physicalDevices16BitStorageFeatures[i].pNext = &physicalDevices8BitStorageFeatures[i];

		physicalDevicesFeatures2[i]       = (VkPhysicalDeviceFeatures2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
		physicalDevicesFeatures2[i].pNext = &physicalDevices16BitStorageFeatures[i];

		VK_CALL(vkGetPhysicalDeviceFeatures2, physicalDevices[i], &physicalDevicesFeatures2[i]);
	}

	uint32_t devIndex     = UINT32_MAX; // Physical device index
	uint32_t highestScore = 0;

	bool using8BitStorage                  = false;
	bool using16BitStorage                 = false;
	bool usingBufferDeviceAddress          = false;
	bool usingMaintenance4                 = false;
	bool usingMemoryBudget                 = false;
	bool usingMemoryPriority               = false;
	bool usingPipelineCreationCacheControl = false;
	bool usingPipelineExecutableProperties = false;
	bool usingPortabilitySubset            = false;
	bool usingShaderInt16                  = false;
	bool usingShaderInt64                  = false;
	bool usingSpirv14                      = false;
	bool usingSubgroupSizeControl          = false;
	bool usingVulkan12                     = false;
	bool usingVulkan13                     = false;

	for (uint32_t i = 0; i < physicalDeviceCount; i++) {
		uint32_t extensionCount   = extensionPropertyCounts[i];
		uint32_t queueFamilyCount = queueFamilyPropertyCounts[i];
		uint32_t memoryTypeCount  = physicalDevicesMemoryProperties2[i].memoryProperties.memoryTypeCount;

		bool hasVulkan11 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasVulkan12 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_2;
		bool hasVulkan13 = physicalDevicesProperties2[i].properties.apiVersion >= VK_API_VERSION_1_3;

		bool hasDiscreteGpu =
			physicalDevicesProperties2[i].properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;

		bool hasShaderInt16 = physicalDevicesFeatures2[i].features.shaderInt16;
		bool hasShaderInt64 = physicalDevicesFeatures2[i].features.shaderInt64;

		bool hasUniformAndStorageBuffer8BitAccess =
			physicalDevices8BitStorageFeatures[i].uniformAndStorageBuffer8BitAccess;
		bool hasStorageBuffer16BitAccess = physicalDevices16BitStorageFeatures[i].storageBuffer16BitAccess;

		bool hasDedicatedTransfer = false;
		bool hasDedicatedCompute  = false;
		bool hasCompute           = false;

		for (uint32_t j = 0; j < queueFamilyCount; j++) {
			VkQueueFlags queueFlags = queueFamiliesProperties2[i][j].queueFamilyProperties.queueFlags;

			bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
			bool isDedicatedCompute  = queueFlags == VK_QUEUE_COMPUTE_BIT;
			bool isCompute           = queueFlags & VK_QUEUE_COMPUTE_BIT;

			if (isDedicatedTransfer) { hasDedicatedTransfer = true; }
			if (isDedicatedCompute)  { hasDedicatedCompute  = true; }
			if (isCompute)           { hasCompute           = true; }
		}

		bool hasDeviceNonHost = false;
		bool hasDeviceLocal   = false;

		bool hasHostCachedNonCoherent = false;
		bool hasHostCached            = false;
		bool hasHostNonCoherent       = false;
		bool hasHostVisible           = false;

		for (uint32_t j = 0; j < memoryTypeCount; j++) {
			VkMemoryPropertyFlags propertyFlags =
				physicalDevicesMemoryProperties2[i].memoryProperties.memoryTypes[j].propertyFlags;

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
				if (!isHostVisible) { hasDeviceNonHost = true; }
			}

			if (isHostVisible) {
				hasHostVisible = true;
				if (isHostCached && !isHostCoherent) { hasHostCachedNonCoherent = true; }
				if (isHostCached)                    { hasHostCached            = true; }
				if (!isHostCoherent)                 { hasHostNonCoherent       = true; }
			}
		}

		bool has8BitStorage                  = false;
		bool hasBufferDeviceAddress          = false;
		bool hasMaintenance4                 = false;
		bool hasPipelineExecutableProperties = false;
		bool hasPortabilitySubset            = false;
		bool hasSpirv14                      = false;
		bool hasSynchronization2             = false;
		bool hasTimelineSemaphore            = false;
		bool hasMemoryBudget                 = false;
		bool hasMemoryPriority               = false;
		bool hasPipelineCreationCacheControl = false;
		bool hasSubgroupSizeControl          = false;

		for (uint32_t j = 0; j < extensionCount; j++) {
			const char* extensionName = extensionsProperties[i][j].extensionName;

			if      (!strcmp(extensionName, VK_KHR_8BIT_STORAGE_EXTENSION_NAME)) { has8BitStorage = true; }
			else if (!strcmp(extensionName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) {
				hasBufferDeviceAddress = true; }
			else if (!strcmp(extensionName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME)) { hasMaintenance4 = true; }
			else if (!strcmp(extensionName, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME)) {
				hasPipelineExecutableProperties = true; }
			else if (!strcmp(extensionName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) { hasPortabilitySubset = true; }
			else if (!strcmp(extensionName, VK_KHR_SPIRV_1_4_EXTENSION_NAME))          { hasSpirv14           = true; }
			else if (!strcmp(extensionName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))  { hasSynchronization2  = true; }
			else if (!strcmp(extensionName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) { hasTimelineSemaphore = true; }
			else if (!strcmp(extensionName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))      { hasMemoryBudget      = true; }
			else if (!strcmp(extensionName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))    { hasMemoryPriority    = true; }
			else if (!strcmp(extensionName, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME)) {
				hasPipelineCreationCacheControl = true; }
			else if (!strcmp(extensionName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) {
				hasSubgroupSizeControl = true; }
		}

		uint32_t currentScore = 1;

		if (!hasVulkan11) continue;

		if (!hasDeviceLocal) continue;
		if (!hasHostVisible) continue;

		if (!hasCompute) continue;

		if (!hasSynchronization2)  continue;
		if (!hasTimelineSemaphore) continue;

		if (hasVulkan12) { currentScore += 50; }
		if (hasVulkan13) { currentScore += 50; }

		if (hasDiscreteGpu) { currentScore += 10000; }

		if (gpu->preferInt16 && hasShaderInt16) { currentScore += 1000; }
		if (gpu->preferInt64 && hasShaderInt64) { currentScore += 1000; }

		if (hasDeviceNonHost) { currentScore += 50; }

		if      (hasHostCachedNonCoherent) { currentScore += 1000; }
		else if (hasHostCached)            { currentScore += 500;  }
		else if (hasHostNonCoherent)       { currentScore += 100;  }

		if (hasDedicatedTransfer) { currentScore += 100; }
		if (hasDedicatedCompute)  { currentScore += 100; }

		if (hasMaintenance4)                 { currentScore += 10;  }
		if (hasMemoryBudget)                 { currentScore += 10;  }
		if (hasMemoryPriority)               { currentScore += 10;  }
		if (hasPipelineCreationCacheControl) { currentScore += 10;  }
		if (hasSpirv14)                      { currentScore += 10;  }
		if (hasStorageBuffer16BitAccess)     { currentScore += 100; }
		if (hasSubgroupSizeControl)          { currentScore += 10;  }

		if (gpu->validationLayers && has8BitStorage && hasUniformAndStorageBuffer8BitAccess) { currentScore += 10; }
		if (gpu->validationLayers && hasBufferDeviceAddress)                                 { currentScore += 10; }
		if (gpu->capturePipelines && hasPipelineExecutableProperties)                        { currentScore += 10; }

		if (currentScore > highestScore) {
			highestScore = currentScore;
			devIndex     = i;

			using8BitStorage = gpu->validationLayers && has8BitStorage && hasUniformAndStorageBuffer8BitAccess;
			using16BitStorage                 = hasStorageBuffer16BitAccess;
			usingBufferDeviceAddress          = gpu->validationLayers && hasBufferDeviceAddress;
			usingMaintenance4                 = hasMaintenance4;
			usingMemoryBudget                 = hasMemoryBudget;
			usingMemoryPriority               = hasMemoryPriority;
			usingPipelineCreationCacheControl = hasPipelineCreationCacheControl;
			usingPipelineExecutableProperties = gpu->capturePipelines && hasPipelineExecutableProperties;
			usingPortabilitySubset            = hasPortabilitySubset;
			usingShaderInt16                  = gpu->preferInt16 && hasShaderInt16;
			usingShaderInt64                  = gpu->preferInt64 && hasShaderInt64;
			usingSpirv14                      = hasSpirv14;
			usingSubgroupSizeControl          = hasSubgroupSizeControl;
			usingVulkan12                     = hasVulkan12;
			usingVulkan13                     = hasVulkan13;
		}
	}

	if (devIndex == UINT32_MAX) {
		fputs(
			"Runtime failure\n"
			"No physical device meets program requirements\n"
			"See device_requirements.md for full physical device requirements\n\n",
			stderr);
		free_recursive(dyMem);
		return false;
	}

	const char* deviceName = physicalDevicesProperties2[devIndex].properties.deviceName;

	uint32_t queueFamilyCount = queueFamilyPropertyCounts[devIndex];

	uint32_t vkVerMajor = 1;
	uint32_t vkVerMinor;
	uint32_t spvVerMajor = 1;
	uint32_t spvVerMinor;

	if      (usingVulkan13) { vkVerMinor = 3; spvVerMinor = 6; }
	else if (usingVulkan12) { vkVerMinor = 2; spvVerMinor = 5; }
	else if (usingSpirv14)  { vkVerMinor = 1; spvVerMinor = 4; }
	else                    { vkVerMinor = 1; spvVerMinor = 3; }

	uint32_t transferQueueFamilyIndex = 0;
	uint32_t computeQueueFamilyIndex  = 0;

	bool hasDedicatedTransfer = false;
	bool hasTransfer          = false;

	bool hasDedicatedCompute = false;
	bool hasCompute          = false;

	for (uint32_t i = 0; i < queueFamilyCount; i++) {
		VkQueueFlags queueFlags = queueFamiliesProperties2[devIndex][i].queueFamilyProperties.queueFlags;

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

	if (!hasTransfer) { transferQueueFamilyIndex = computeQueueFamilyIndex; }

	gpu->physicalDevice = physicalDevices[devIndex];

	gpu->transferQueueFamilyIndex = transferQueueFamilyIndex;
	gpu->computeQueueFamilyIndex  = computeQueueFamilyIndex;

	gpu->vkVerMajor  = vkVerMajor;
	gpu->vkVerMinor  = vkVerMinor;
	gpu->spvVerMajor = spvVerMajor;
	gpu->spvVerMinor = spvVerMinor;

	gpu->using8BitStorage                  = using8BitStorage;
	gpu->using16BitStorage                 = using16BitStorage;
	gpu->usingBufferDeviceAddress          = usingBufferDeviceAddress;
	gpu->usingMaintenance4                 = usingMaintenance4;
	gpu->usingMemoryBudget                 = usingMemoryBudget;
	gpu->usingMemoryPriority               = usingMemoryPriority;
	gpu->usingPipelineCreationCacheControl = usingPipelineCreationCacheControl;
	gpu->usingPipelineExecutableProperties = usingPipelineExecutableProperties;
	gpu->usingPortabilitySubset            = usingPortabilitySubset;
	gpu->usingShaderInt16                  = usingShaderInt16;
	gpu->usingShaderInt64                  = usingShaderInt64;
	gpu->usingSubgroupSizeControl          = usingSubgroupSizeControl;

	if (gpu->queryBenchmarking) {
		gpu->transferQueueTimestampValidBits =
			queueFamiliesProperties2[devIndex][transferQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
		gpu->computeQueueTimestampValidBits =
			queueFamiliesProperties2[devIndex][computeQueueFamilyIndex].queueFamilyProperties.timestampValidBits;
		gpu->timestampPeriod = physicalDevicesProperties2[devIndex].properties.limits.timestampPeriod;
	}

	switch (gpu->outputLevel) {
		case CLI_OUTPUT_SILENT: break;
		case CLI_OUTPUT_QUIET:  break;
		case CLI_OUTPUT_DEFAULT:
			printf(
				"Device: %s\n"
				"\tVulkan version:    %" PRIu32 ".%" PRIu32 "\n"
				"\tSPIR-V version:    %" PRIu32 ".%" PRIu32 "\n"
				"\tTransfer QF index: %" PRIu32 "\n"
				"\tCompute QF index:  %" PRIu32 "\n",
				deviceName,
				vkVerMajor,  vkVerMinor,
				spvVerMajor, spvVerMinor,
				transferQueueFamilyIndex,
				computeQueueFamilyIndex);
			if (gpu->preferInt16) { printf("\tshaderInt16:       %d\n", usingShaderInt16); }
			if (gpu->preferInt64) { printf("\tshaderInt64:       %d\n", usingShaderInt64); }
			NEWLINE();
			break;
		case CLI_OUTPUT_VERBOSE:
			printf(
				"Device: %s\n"
				"\tScore:                             %" PRIu32 "\n"
				"\tVulkan version:                    %" PRIu32 ".%" PRIu32 "\n"
				"\tSPIR-V version:                    %" PRIu32 ".%" PRIu32 "\n"
				"\tTransfer QF index:                 %" PRIu32 "\n"
				"\tCompute QF index:                  %" PRIu32 "\n"
				"\tbufferDeviceAddress                %d\n"
				"\tmaintenance4                       %d\n"
				"\tmemoryPriority:                    %d\n"
				"\tpipelineCreationCacheControl:      %d\n"
				"\tpipelineExecutableProperties:      %d\n"
				"\tshaderInt16:                       %d\n"
				"\tshaderInt64:                       %d\n"
				"\tstorageBuffer16BitAccess:          %d\n"
				"\tsubgroupSizeControl:               %d\n"
				"\tuniformAndStorageBuffer8BitAccess: %d\n\n",
				deviceName,
				highestScore,
				vkVerMajor,  vkVerMinor,
				spvVerMajor, spvVerMinor,
				transferQueueFamilyIndex,
				computeQueueFamilyIndex,
				usingBufferDeviceAddress,
				usingMaintenance4,
				usingMemoryPriority,
				usingPipelineCreationCacheControl,
				usingPipelineExecutableProperties,
				usingShaderInt16,
				usingShaderInt64,
				using16BitStorage,
				usingSubgroupSizeControl,
				using8BitStorage);
			break;
		default: break;
	}

	free_recursive(dyMem);

	return true;
}

bool create_device(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;

	uint32_t computeQueueFamilyIndex  = gpu->computeQueueFamilyIndex;
	uint32_t transferQueueFamilyIndex = gpu->transferQueueFamilyIndex;

	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 1);
	if EXPECT_FALSE (!dyMem) return false;

	DyArray enabledExtensions = DyArray_create(sizeof(const char*), 13);
	if EXPECT_FALSE (!enabledExtensions) { free_recursive(dyMem); return false; }

	dyData = (DyData) {enabledExtensions, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	DyArray_append(enabledExtensions, &VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	DyArray_append(enabledExtensions, &VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

	if (gpu->using8BitStorage)       { DyArray_append(enabledExtensions, &VK_KHR_8BIT_STORAGE_EXTENSION_NAME);       }
	if (gpu->usingMaintenance4)      { DyArray_append(enabledExtensions, &VK_KHR_MAINTENANCE_4_EXTENSION_NAME);      }
	if (gpu->usingPortabilitySubset) { DyArray_append(enabledExtensions, &VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME); }
	if (gpu->usingMemoryBudget)      { DyArray_append(enabledExtensions, &VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);      }
	if (gpu->usingMemoryPriority)    { DyArray_append(enabledExtensions, &VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);    }

	if (gpu->usingBufferDeviceAddress) {
		DyArray_append(enabledExtensions, &VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME); }
	if (gpu->usingPipelineExecutableProperties) {
		DyArray_append(enabledExtensions, &VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME); }
	if (gpu->usingPipelineCreationCacheControl) {
		DyArray_append(enabledExtensions, &VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME); }
	if (gpu->usingSubgroupSizeControl) {
		DyArray_append(enabledExtensions, &VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME); }

	if (spvVerMinor >= 4) {
		DyArray_append(enabledExtensions, &VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		DyArray_append(enabledExtensions, &VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	}

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {VK_FALSE};
	physicalDeviceFeatures.shaderInt64              = gpu->usingShaderInt64;
	physicalDeviceFeatures.shaderInt16              = gpu->usingShaderInt16;

	VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	physicalDeviceFeatures2.features                  = physicalDeviceFeatures;

	VkPhysicalDevice16BitStorageFeatures physicalDevice16BitStorageFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
	physicalDevice16BitStorageFeatures.storageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDevice8BitStorageFeaturesKHR physicalDevice8BitStorageFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES_KHR};
	physicalDevice8BitStorageFeatures.storageBuffer8BitAccess           = VK_TRUE;
	physicalDevice8BitStorageFeatures.uniformAndStorageBuffer8BitAccess = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeaturesKHR physicalDeviceBufferDeviceAddressFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR};
	physicalDeviceBufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceMaintenance4FeaturesKHR physicalDeviceMaintenance4Features = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES_KHR};
	physicalDeviceMaintenance4Features.maintenance4 = VK_TRUE;

	VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR physicalDevicePipelineExecutablePropertiesFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR};
	physicalDevicePipelineExecutablePropertiesFeatures.pipelineExecutableInfo = VK_TRUE;

	VkPhysicalDeviceSynchronization2FeaturesKHR physicalDeviceSynchronization2Features = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR};
	physicalDeviceSynchronization2Features.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeaturesKHR physicalDeviceTimelineSemaphoreFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES_KHR};
	physicalDeviceTimelineSemaphoreFeatures.timelineSemaphore = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT physicalDeviceMemoryPriorityFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT};
	physicalDeviceMemoryPriorityFeatures.memoryPriority = VK_TRUE;

	VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT physicalDevicePipelineCreationCacheControlFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES_EXT};
	physicalDevicePipelineCreationCacheControlFeatures.pipelineCreationCacheControl = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeaturesEXT physicalDeviceSubgroupSizeControlFeatures = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES_EXT};
	physicalDeviceSubgroupSizeControlFeatures.subgroupSizeControl = VK_TRUE;

	void** next = &physicalDeviceFeatures2.pNext;

	PNEXT_ADD(next, physicalDeviceSynchronization2Features);
	PNEXT_ADD(next, physicalDeviceTimelineSemaphoreFeatures);

	if (gpu->using8BitStorage)                  { PNEXT_ADD(next, physicalDevice8BitStorageFeatures);                  }
	if (gpu->using16BitStorage)                 { PNEXT_ADD(next, physicalDevice16BitStorageFeatures);                 }
	if (gpu->usingBufferDeviceAddress)          { PNEXT_ADD(next, physicalDeviceBufferDeviceAddressFeatures);          }
	if (gpu->usingMaintenance4)                 { PNEXT_ADD(next, physicalDeviceMaintenance4Features);                 }
	if (gpu->usingMemoryPriority)               { PNEXT_ADD(next, physicalDeviceMemoryPriorityFeatures);               }
	if (gpu->usingPipelineCreationCacheControl) { PNEXT_ADD(next, physicalDevicePipelineCreationCacheControlFeatures); }
	if (gpu->usingPipelineExecutableProperties) { PNEXT_ADD(next, physicalDevicePipelineExecutablePropertiesFeatures); }
	if (gpu->usingSubgroupSizeControl)          { PNEXT_ADD(next, physicalDeviceSubgroupSizeControlFeatures);          }

	// CPU spends more time waiting for compute operations than transfer operations
	// So compute queue has higher priority to potentially reduce this wait time
	float computeQueuePriority  = 1;
	float transferQueuePriority = 0;

	VkDeviceQueueCreateInfo queueCreateInfos[2];
	queueCreateInfos[0]                  = (VkDeviceQueueCreateInfo) {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfos[0].queueFamilyIndex = computeQueueFamilyIndex;
	queueCreateInfos[0].queueCount       = 1;
	queueCreateInfos[0].pQueuePriorities = &computeQueuePriority;

	queueCreateInfos[1]                  = (VkDeviceQueueCreateInfo) {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfos[1].queueFamilyIndex = transferQueueFamilyIndex;
	queueCreateInfos[1].queueCount       = 1;
	queueCreateInfos[1].pQueuePriorities = &transferQueuePriority;

	uint32_t     enabledExtensionCount = (uint32_t) DyArray_size(enabledExtensions);
	const char** enabledExtensionNames = (const char**) DyArray_raw(enabledExtensions);

	VkDeviceCreateInfo deviceCreateInfo      = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	deviceCreateInfo.pNext                   = &physicalDeviceFeatures2;
	deviceCreateInfo.queueCreateInfoCount    = computeQueueFamilyIndex == transferQueueFamilyIndex ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount   = enabledExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtensionNames;

	if (gpu->outputLevel > CLI_OUTPUT_DEFAULT) {
		printf("Enabled device extensions (%" PRIu32 "):\n", enabledExtensionCount);
		for (uint32_t i = 0; i < enabledExtensionCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtensionNames[i]);
		}
		NEWLINE();
	}

	VK_CALL_RES(vkCreateDevice, physicalDevice, &deviceCreateInfo, g_allocator, &gpu->device);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkDevice device = gpu->device;

	free_recursive(dyMem);

	volkLoadDevice(device);

	VkDeviceQueueInfo2 transferDeviceQueueInfo2 = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
	transferDeviceQueueInfo2.queueFamilyIndex   = transferQueueFamilyIndex;
	transferDeviceQueueInfo2.queueIndex         = 0;

	VkDeviceQueueInfo2 computeDeviceQueueInfo2 = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
	computeDeviceQueueInfo2.queueFamilyIndex   = computeQueueFamilyIndex;
	computeDeviceQueueInfo2.queueIndex         = 0;

	VK_CALL(vkGetDeviceQueue2, device, &transferDeviceQueueInfo2, &gpu->transferQueue);
	VK_CALL(vkGetDeviceQueue2, device, &computeDeviceQueueInfo2,  &gpu->computeQueue);

#ifndef NDEBUG
	if (gpu->debugUtilsMessenger) {
		if (gpu->transferQueue != gpu->computeQueue) {
			set_debug_name(device, VK_OBJECT_TYPE_QUEUE, (uint64_t) gpu->transferQueue, "Transfer");
			set_debug_name(device, VK_OBJECT_TYPE_QUEUE, (uint64_t) gpu->computeQueue,  "Compute");
		}
		else { set_debug_name(device, VK_OBJECT_TYPE_QUEUE, (uint64_t) gpu->transferQueue, "Transfer & Compute"); }
	}
#endif

	return true;
}

bool manage_memory(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;
	VkDevice         device         = gpu->device;

	bool (*get_buffer_requirements)(VkDevice, VkDeviceSize, VkBufferUsageFlags, VkMemoryRequirements*) =
		gpu->usingMaintenance4 ? get_buffer_requirements_main4 : get_buffer_requirements_noext;

	VkPhysicalDeviceMaintenance4PropertiesKHR physicalDeviceMaintenance4Properties = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES_KHR};

	VkPhysicalDeviceMaintenance3Properties physicalDeviceMaintenance3Properties = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES};
	physicalDeviceMaintenance3Properties.pNext = gpu->usingMaintenance4 ? &physicalDeviceMaintenance4Properties : NULL;

	VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	physicalDeviceProperties2.pNext                       = &physicalDeviceMaintenance3Properties;

	VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevice, &physicalDeviceProperties2);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT physicalDeviceMemoryBudgetProperties = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};

	VkPhysicalDeviceMemoryProperties2 physicalDeviceMemoryProperties2 = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
	physicalDeviceMemoryProperties2.pNext = gpu->usingMemoryBudget ? &physicalDeviceMemoryBudgetProperties : NULL;

	VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevice, &physicalDeviceMemoryProperties2);

	VkDeviceSize maxMemoryAllocationSize = physicalDeviceMaintenance3Properties.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize =
		gpu->usingMaintenance4 ? physicalDeviceMaintenance4Properties.maxBufferSize : maxMemoryAllocationSize;

	uint32_t maxStorageBufferRange    = physicalDeviceProperties2.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryAllocationCount = physicalDeviceProperties2.properties.limits.maxMemoryAllocationCount;
	uint32_t maxComputeWorkGroupCount = physicalDeviceProperties2.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxComputeWorkGroupSize  = physicalDeviceProperties2.properties.limits.maxComputeWorkGroupSize[0];
	uint32_t memoryTypeCount          = physicalDeviceMemoryProperties2.memoryProperties.memoryTypeCount;

	VkMemoryRequirements deviceLocalMemoryRequirements;
	VkMemoryRequirements hostVisibleMemoryRequirements;

	bool bres = get_buffer_requirements(
		device, sizeof(char), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		&hostVisibleMemoryRequirements);

	if EXPECT_FALSE (!bres) return false;

	bres = get_buffer_requirements(
		device, sizeof(char),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		&deviceLocalMemoryRequirements);

	if EXPECT_FALSE (!bres) return false;

	uint32_t deviceLocalMemoryTypeBits = deviceLocalMemoryRequirements.memoryTypeBits;
	uint32_t hostVisibleMemoryTypeBits = hostVisibleMemoryRequirements.memoryTypeBits;

	uint32_t deviceLocalHeapIndex = 0;
	uint32_t hostVisibleHeapIndex = 0;
	uint32_t deviceLocalTypeIndex = 0;
	uint32_t hostVisibleTypeIndex = 0;

	bool hasDeviceNonHost = false;
	bool hasDeviceLocal   = false;

	bool hasHostCachedNonCoherent = false;
	bool hasHostCached            = false;
	bool hasHostNonCoherent       = false;
	bool hasHostVisible           = false;

	for (uint32_t i = 0; i < memoryTypeCount; i++) {
		uint32_t memoryTypeBit = 1U << i;

		VkMemoryPropertyFlags propertyFlags =
			physicalDeviceMemoryProperties2.memoryProperties.memoryTypes[i].propertyFlags;
		uint32_t heapIndex = physicalDeviceMemoryProperties2.memoryProperties.memoryTypes[i].heapIndex;

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

	VkDeviceSize hostVisibleHeapBudget = physicalDeviceMemoryBudgetProperties.heapBudget[hostVisibleHeapIndex];
	VkDeviceSize deviceLocalHeapBudget = physicalDeviceMemoryBudgetProperties.heapBudget[deviceLocalHeapIndex];

	VkDeviceSize hostVisibleHeapSize =
		physicalDeviceMemoryProperties2.memoryProperties.memoryHeaps[hostVisibleHeapIndex].size;
	VkDeviceSize deviceLocalHeapSize =
		physicalDeviceMemoryProperties2.memoryProperties.memoryHeaps[deviceLocalHeapIndex].size;

	VkDeviceSize bytesPerHostVisibleHeap = gpu->usingMemoryBudget ? hostVisibleHeapBudget : hostVisibleHeapSize;
	VkDeviceSize bytesPerDeviceLocalHeap = gpu->usingMemoryBudget ? deviceLocalHeapBudget : deviceLocalHeapSize;

	VkDeviceSize bytesPerHeap = minu64(bytesPerHostVisibleHeap, bytesPerDeviceLocalHeap);
	bytesPerHeap              = (VkDeviceSize) ((float) bytesPerHeap * gpu->maxMemory);

	if (deviceLocalHeapIndex == hostVisibleHeapIndex) { bytesPerHeap /= 2; }

	VkDeviceSize bytesPerBuffer = minu64v(3, maxMemoryAllocationSize, maxBufferSize, bytesPerHeap);
	uint32_t     buffersPerHeap = (uint32_t) (bytesPerHeap / bytesPerBuffer);

	if (buffersPerHeap < maxMemoryAllocationCount && bytesPerHeap % bytesPerBuffer) {
		VkDeviceSize excessBytes = bytesPerBuffer - bytesPerHeap % bytesPerBuffer;

		buffersPerHeap++;
		bytesPerBuffer -= excessBytes / buffersPerHeap;

		if (excessBytes % buffersPerHeap) { bytesPerBuffer--; }
	}
	else if (buffersPerHeap > maxMemoryAllocationCount) { buffersPerHeap = maxMemoryAllocationCount; }

	uint32_t workgroupSize  = floor_pow2(maxComputeWorkGroupSize);
	uint32_t workgroupCount = minu32(
		maxComputeWorkGroupCount, (uint32_t) (maxStorageBufferRange / (workgroupSize * sizeof(Value))));

	uint32_t     valuesPerInout  = workgroupSize * workgroupCount;
	VkDeviceSize bytesPerInout   = valuesPerInout * (sizeof(Value) + sizeof(Count));
	uint32_t     inoutsPerBuffer = (uint32_t) (bytesPerBuffer / bytesPerInout);

	if (bytesPerBuffer % bytesPerInout > inoutsPerBuffer * workgroupSize * (sizeof(Value) + sizeof(Count))) {
		uint32_t excessValues =
			valuesPerInout - (uint32_t) (bytesPerBuffer % bytesPerInout / (sizeof(Value) + sizeof(Count)));

		inoutsPerBuffer++;
		valuesPerInout -= excessValues / inoutsPerBuffer;

		if (excessValues % inoutsPerBuffer) { valuesPerInout--; }

		valuesPerInout &= ~(workgroupSize - 1U);

		if (!valuesPerInout) {
			inoutsPerBuffer--;
			valuesPerInout = workgroupSize * workgroupCount;
		}

		workgroupCount = valuesPerInout / workgroupSize;
	}

	VkDeviceSize bytesPerIn  = valuesPerInout * sizeof(Value);
	VkDeviceSize bytesPerOut = valuesPerInout * sizeof(Count);

	bytesPerInout  = bytesPerIn + bytesPerOut;
	bytesPerBuffer = bytesPerInout * inoutsPerBuffer;

	uint32_t valuesPerBuffer = valuesPerInout * inoutsPerBuffer;
	uint32_t valuesPerHeap   = valuesPerBuffer * buffersPerHeap;
	uint32_t inoutsPerHeap   = inoutsPerBuffer * buffersPerHeap;

	bres = get_buffer_requirements(
		device, bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		&hostVisibleMemoryRequirements);

	if EXPECT_FALSE (!bres) return false;

	bres = get_buffer_requirements(
		device, bytesPerBuffer,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		&deviceLocalMemoryRequirements);

	if EXPECT_FALSE (!bres) return false;

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

	gpu->workgroupSize  = workgroupSize;
	gpu->workgroupCount = workgroupCount;

	gpu->hostVisibleHeapIndex = hostVisibleHeapIndex;
	gpu->deviceLocalHeapIndex = deviceLocalHeapIndex;

	gpu->hostVisibleTypeIndex = hostVisibleTypeIndex;
	gpu->deviceLocalTypeIndex = deviceLocalTypeIndex;

	gpu->hostNonCoherent = hasHostNonCoherent;

	switch (gpu->outputLevel) {
		case CLI_OUTPUT_SILENT: break;
		case CLI_OUTPUT_QUIET:  break;
		case CLI_OUTPUT_DEFAULT:
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
		case CLI_OUTPUT_VERBOSE:
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
				valuesPerInout, inoutsPerBuffer,
				buffersPerHeap, valuesPerHeap);
			break;
		default: break;
	}

	size_t size =
		inoutsPerHeap  * sizeof(Value*) +
		inoutsPerHeap  * sizeof(Count*) +
		buffersPerHeap * sizeof(VkBuffer) * 2 +
		buffersPerHeap * sizeof(VkDeviceMemory) * 2 +
		inoutsPerHeap  * sizeof(VkDescriptorSet) +
		inoutsPerHeap  * sizeof(VkCommandBuffer) * 2 +
		inoutsPerHeap  * sizeof(VkSemaphore);

	gpu->dynamicMemory = calloc(size, sizeof(char));
	if EXPECT_FALSE (!gpu->dynamicMemory) { CALLOC_FAILURE(gpu->dynamicMemory, size, sizeof(char)); return false; }

	gpu->mappedInBuffers  = (Value**) gpu->dynamicMemory;
	gpu->mappedOutBuffers = (Count**) (gpu->mappedInBuffers + inoutsPerHeap);

	gpu->hostVisibleBuffers = (VkBuffer*) (gpu->mappedOutBuffers + inoutsPerHeap);
	gpu->deviceLocalBuffers = (VkBuffer*) (gpu->hostVisibleBuffers + buffersPerHeap);

	gpu->hostVisibleDeviceMemories = (VkDeviceMemory*) (gpu->deviceLocalBuffers + buffersPerHeap);
	gpu->deviceLocalDeviceMemories = (VkDeviceMemory*) (gpu->hostVisibleDeviceMemories + buffersPerHeap);

	gpu->descriptorSets = (VkDescriptorSet*) (gpu->deviceLocalDeviceMemories + buffersPerHeap);

	gpu->transferCommandBuffers = (VkCommandBuffer*) (gpu->descriptorSets + inoutsPerHeap);
	gpu->computeCommandBuffers  = (VkCommandBuffer*) (gpu->transferCommandBuffers + inoutsPerHeap);

	gpu->semaphores = (VkSemaphore*) (gpu->computeCommandBuffers + inoutsPerHeap);

	return true;
}

bool create_buffers(Gpu* restrict gpu)
{
	VkBuffer*       hostVisibleBuffers        = gpu->hostVisibleBuffers;
	VkBuffer*       deviceLocalBuffers        = gpu->deviceLocalBuffers;
	VkDeviceMemory* hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	Value**         mappedInBuffers           = gpu->mappedInBuffers;
	Count**         mappedOutBuffers          = gpu->mappedOutBuffers;

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
	DyArray dyMem = DyArray_create(sizeof(DyData), 1);
	if EXPECT_FALSE (!dyMem) return false;

	size_t size =
		buffersPerHeap * sizeof(VkMemoryAllocateInfo) * 2 +
		buffersPerHeap * sizeof(VkMemoryDedicatedAllocateInfo) * 2 +
		buffersPerHeap * sizeof(VkBindBufferMemoryInfo) * 2;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkMemoryAllocateInfo* hostVisibleMemoryAllocateInfos = (VkMemoryAllocateInfo*) p;
	VkMemoryAllocateInfo* deviceLocalMemoryAllocateInfos = (VkMemoryAllocateInfo*) (
		hostVisibleMemoryAllocateInfos + buffersPerHeap);

	VkMemoryDedicatedAllocateInfo* hostVisibleMemoryDedicatedAllocateInfos = (VkMemoryDedicatedAllocateInfo*) (
		deviceLocalMemoryAllocateInfos + buffersPerHeap);
	VkMemoryDedicatedAllocateInfo* deviceLocalMemoryDedicatedAllocateInfos = (VkMemoryDedicatedAllocateInfo*) (
		hostVisibleMemoryDedicatedAllocateInfos + buffersPerHeap);

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) (
		deviceLocalMemoryDedicatedAllocateInfos + buffersPerHeap);

	VkBufferCreateInfo hostVisibleBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	hostVisibleBufferCreateInfo.size               = bytesPerBuffer;
	hostVisibleBufferCreateInfo.usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hostVisibleBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo deviceLocalBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	deviceLocalBufferCreateInfo.size               = bytesPerBuffer;
	deviceLocalBufferCreateInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	deviceLocalBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkCreateBuffer, device, &hostVisibleBufferCreateInfo, g_allocator, &hostVisibleBuffers[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		VK_CALL_RES(vkCreateBuffer, device, &deviceLocalBufferCreateInfo, g_allocator, &deviceLocalBuffers[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	VkMemoryPriorityAllocateInfoEXT hostVisibleMemoryPriorityAllocateInfo = {
		VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
	hostVisibleMemoryPriorityAllocateInfo.priority = 0;

	VkMemoryPriorityAllocateInfoEXT deviceLocalMemoryPriorityAllocateInfo = {
		VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
	deviceLocalMemoryPriorityAllocateInfo.priority = 1;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		hostVisibleMemoryDedicatedAllocateInfos[i] = (VkMemoryDedicatedAllocateInfo) {
			VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		hostVisibleMemoryDedicatedAllocateInfos[i].pNext =
			gpu->usingMemoryPriority ? &hostVisibleMemoryPriorityAllocateInfo : NULL;
		hostVisibleMemoryDedicatedAllocateInfos[i].buffer = hostVisibleBuffers[i];

		deviceLocalMemoryDedicatedAllocateInfos[i] = (VkMemoryDedicatedAllocateInfo) {
			VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		deviceLocalMemoryDedicatedAllocateInfos[i].pNext =
			gpu->usingMemoryPriority ? &deviceLocalMemoryPriorityAllocateInfo : NULL;
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
		VK_CALL_RES(
			vkAllocateMemory, device, &hostVisibleMemoryAllocateInfos[i], g_allocator, &hostVisibleDeviceMemories[i]);

		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		VK_CALL_RES(
			vkAllocateMemory, device, &deviceLocalMemoryAllocateInfos[i], g_allocator, &deviceLocalDeviceMemories[i]);

		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		bindBufferMemoryInfos[i][0]        = (VkBindBufferMemoryInfo) {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
		bindBufferMemoryInfos[i][0].buffer = hostVisibleBuffers[i];
		bindBufferMemoryInfos[i][0].memory = hostVisibleDeviceMemories[i];

		bindBufferMemoryInfos[i][1]        = (VkBindBufferMemoryInfo) {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
		bindBufferMemoryInfos[i][1].buffer = deviceLocalBuffers[i];
		bindBufferMemoryInfos[i][1].memory = deviceLocalDeviceMemories[i];
	}

	VK_CALL_RES(vkBindBufferMemory2, device, buffersPerHeap * 2, (VkBindBufferMemoryInfo*) bindBufferMemoryInfos);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(
			vkMapMemory, device, hostVisibleDeviceMemories[i], 0, bytesPerHostVisibleMemory, 0,
			(void**) &mappedInBuffers[j]);

		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		mappedOutBuffers[j] = (Count*) (mappedInBuffers[j] + valuesPerInout);
		j++;

		for (uint32_t k = 1; k < inoutsPerBuffer; j++, k++) {
			mappedInBuffers[j] =
				mappedInBuffers[j - 1] + valuesPerInout + valuesPerInout * sizeof(Count) / sizeof(Value);
			mappedOutBuffers[j] =
				mappedOutBuffers[j - 1] + valuesPerInout + valuesPerInout * sizeof(Value) / sizeof(Count);
		}
	}

	free_recursive(dyMem);

#ifndef NDEBUG
	if (gpu->debugUtilsMessenger) {
		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			char objectName[37];

			sprintf(objectName, "Host visible (%" PRIu32 "/%" PRIu32 ")", i + 1, buffersPerHeap);

			set_debug_name(device, VK_OBJECT_TYPE_BUFFER,        (uint64_t) hostVisibleBuffers[i],        objectName);
			set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) hostVisibleDeviceMemories[i], objectName);

			strcpy(objectName, "Device local");
			objectName[12] = ' '; // Remove '\0' from strcpy

			set_debug_name(device, VK_OBJECT_TYPE_BUFFER,        (uint64_t) deviceLocalBuffers[i],        objectName);
			set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) deviceLocalDeviceMemories[i], objectName);
		}
	}
#endif

	return true;
}

bool create_descriptors(Gpu* restrict gpu)
{
	const VkBuffer* deviceLocalBuffers = gpu->deviceLocalBuffers;

	VkDescriptorSet* descriptorSets = gpu->descriptorSets;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerIn    = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut   = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap   = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap  = gpu->buffersPerHeap;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 1);
	if EXPECT_FALSE (!dyMem) return false;

	size_t size =
		inoutsPerHeap * sizeof(VkDescriptorSetLayout) +
		inoutsPerHeap * sizeof(VkWriteDescriptorSet) +
		inoutsPerHeap * sizeof(VkDescriptorBufferInfo) * 2;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkDescriptorSetLayout* descriptorSetLayouts = (VkDescriptorSetLayout*) p;
	VkWriteDescriptorSet*  writeDescriptorSets  = (VkWriteDescriptorSet*) (descriptorSetLayouts + inoutsPerHeap);

	VkDescriptorBufferInfo (*descriptorBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (
		writeDescriptorSets + inoutsPerHeap);

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

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descriptorSetLayoutCreateInfo.bindingCount = ARR_SIZE(descriptorSetLayoutBindings);
	descriptorSetLayoutCreateInfo.pBindings    = descriptorSetLayoutBindings;

	VkDescriptorPoolSize descriptorPoolSizes[1];
	descriptorPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorPoolSizes[0].descriptorCount = inoutsPerHeap * 2;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	descriptorPoolCreateInfo.maxSets                    = inoutsPerHeap;
	descriptorPoolCreateInfo.poolSizeCount              = ARR_SIZE(descriptorPoolSizes);
	descriptorPoolCreateInfo.pPoolSizes                 = descriptorPoolSizes;

	VK_CALL_RES(
		vkCreateDescriptorSetLayout, device, &descriptorSetLayoutCreateInfo, g_allocator, &gpu->descriptorSetLayout);

	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreateDescriptorPool, device, &descriptorPoolCreateInfo, g_allocator, &gpu->descriptorPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool      descriptorPool      = gpu->descriptorPool;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		descriptorSetLayouts[i] = descriptorSetLayout;
	}

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	descriptorSetAllocateInfo.descriptorPool              = descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount          = inoutsPerHeap;
	descriptorSetAllocateInfo.pSetLayouts                 = descriptorSetLayouts;

	VK_CALL_RES(vkAllocateDescriptorSets, device, &descriptorSetAllocateInfo, descriptorSets);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			descriptorBufferInfos[j][0].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[j][0].offset = bytesPerInout * k;
			descriptorBufferInfos[j][0].range  = bytesPerIn;

			descriptorBufferInfos[j][1].buffer = deviceLocalBuffers[i];
			descriptorBufferInfos[j][1].offset = bytesPerInout * k + bytesPerIn;
			descriptorBufferInfos[j][1].range  = bytesPerOut;

			writeDescriptorSets[j]                 = (VkWriteDescriptorSet) {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			writeDescriptorSets[j].dstSet          = descriptorSets[j];
			writeDescriptorSets[j].dstBinding      = 0;
			writeDescriptorSets[j].dstArrayElement = 0;
			writeDescriptorSets[j].descriptorCount = ARR_SIZE(descriptorBufferInfos[j]);
			writeDescriptorSets[j].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescriptorSets[j].pBufferInfo     = descriptorBufferInfos[j];
		}
	}

	VK_CALL(vkUpdateDescriptorSets, device, inoutsPerHeap, writeDescriptorSets, 0, NULL);

	free_recursive(dyMem);

#ifndef NDEBUG
	if (gpu->debugUtilsMessenger) {
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
	}
#endif

	return true;
}

bool create_pipeline(Gpu* restrict gpu)
{
	VkDevice device = gpu->device;

	VkDescriptorSetLayout descriptorSetLayout = gpu->descriptorSetLayout;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t workgroupSize = gpu->workgroupSize;

	uint32_t transferQueueTimestampValidBits = gpu->transferQueueTimestampValidBits;
	uint32_t computeQueueTimestampValidBits  = gpu->computeQueueTimestampValidBits;

	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 2);
	if EXPECT_FALSE (!dyMem) return false;

	char shaderName[34] = "./";

	if      (spvVerMinor == 6) { strcat(shaderName, "v16/"); }
	else if (spvVerMinor == 5) { strcat(shaderName, "v15/"); }
	else if (spvVerMinor == 4) { strcat(shaderName, "v14/"); }
	else if (spvVerMinor == 3) { strcat(shaderName, "v13/"); }

	strcat(shaderName, "spirv");

	if (gpu->using16BitStorage) { strcat(shaderName, "-sto16"); }
	if (gpu->usingShaderInt16)  { strcat(shaderName, "-int16"); }
	if (gpu->usingShaderInt64)  { strcat(shaderName, "-int64"); }

	strcat(shaderName, ".spv");

	char entryPointName[11] = "main";

	switch (get_endianness()) {
		case ENDIANNESS_BIG:    strcat(entryPointName, "_0"); break;
		case ENDIANNESS_LITTLE: strcat(entryPointName, "_1"); break;
		default: break;
	}

	switch (gpu->iterSize) {
		case 128: strcat(entryPointName, "_128"); break;
		case 256: strcat(entryPointName, "_256"); break;
		default: break;
	}

	if (gpu->outputLevel > CLI_OUTPUT_QUIET) {
		printf("Selected shader: %s\nSelected entry point: %s\n\n", shaderName, entryPointName); }

	size_t shaderSize;
	size_t cacheSize;

	bool bres = file_size(shaderName, &shaderSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	bres = file_size(PIPELINE_CACHE_NAME, &cacheSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	size_t size = shaderSize + cacheSize;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	uint32_t* shaderCode = (uint32_t*) p;
	void*     cacheData  = (char*) shaderCode + shaderSize;

	bres = read_file(shaderName, shaderCode, shaderSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	if (cacheSize) {
		bres = read_file(PIPELINE_CACHE_NAME, cacheData, cacheSize);
		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

	VkShaderModuleCreateInfo shaderModuleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	shaderModuleCreateInfo.codeSize                 = shaderSize;
	shaderModuleCreateInfo.pCode                    = shaderCode;

	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	pipelineCacheCreateInfo.flags =
		gpu->usingPipelineCreationCacheControl ? VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT : 0;
	pipelineCacheCreateInfo.initialDataSize = cacheSize;
	pipelineCacheCreateInfo.pInitialData    = cacheData;

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	pipelineLayoutCreateInfo.setLayoutCount             = 1;
	pipelineLayoutCreateInfo.pSetLayouts                = &descriptorSetLayout;

	VK_CALL_RES(vkCreateShaderModule, device, &shaderModuleCreateInfo, g_allocator, &gpu->shaderModule);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreatePipelineCache, device, &pipelineCacheCreateInfo, g_allocator, &gpu->pipelineCache);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreatePipelineLayout, device, &pipelineLayoutCreateInfo, g_allocator, &gpu->pipelineLayout);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkShaderModule   shaderModule   = gpu->shaderModule;
	VkPipelineCache  pipelineCache  = gpu->pipelineCache;
	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;

	VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, g_allocator);
	gpu->descriptorSetLayout = VK_NULL_HANDLE;

	uint32_t specialisationData[1];
	specialisationData[0] = workgroupSize;

	VkSpecializationMapEntry specialisationMapEntries[1];
	specialisationMapEntries[0].constantID = 0;
	specialisationMapEntries[0].offset     = 0;
	specialisationMapEntries[0].size       = sizeof(specialisationData[0]);

	VkSpecializationInfo specialisationInfo;
	specialisationInfo.mapEntryCount = ARR_SIZE(specialisationMapEntries);
	specialisationInfo.pMapEntries   = specialisationMapEntries;
	specialisationInfo.dataSize      = sizeof(specialisationData);
	specialisationInfo.pData         = specialisationData;

	VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo = {
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	pipelineShaderStageCreateInfo.flags =
		gpu->usingSubgroupSizeControl ? VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT : 0;
	pipelineShaderStageCreateInfo.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
	pipelineShaderStageCreateInfo.module              = shaderModule;
	pipelineShaderStageCreateInfo.pName               = entryPointName;
	pipelineShaderStageCreateInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo computePipelineCreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	computePipelineCreateInfo.flags =
		gpu->usingPipelineExecutableProperties ?
		VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR | VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR :
		0;
	computePipelineCreateInfo.stage  = pipelineShaderStageCreateInfo;
	computePipelineCreateInfo.layout = pipelineLayout;

	VK_CALL_RES(
		vkCreateComputePipelines, device, pipelineCache, 1, &computePipelineCreateInfo, g_allocator, &gpu->pipeline);

	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL(vkDestroyShaderModule, device, shaderModule, g_allocator);
	gpu->shaderModule = VK_NULL_HANDLE;

	VK_CALL_RES(vkGetPipelineCacheData, device, pipelineCache, &cacheSize, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	p = malloc(cacheSize);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, cacheSize); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	cacheData = p;

	VK_CALL_RES(vkGetPipelineCacheData, device, pipelineCache, &cacheSize, cacheData);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	bres = write_file(PIPELINE_CACHE_NAME, cacheData, cacheSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	VK_CALL(vkDestroyPipelineCache, device, pipelineCache, g_allocator);
	gpu->pipelineCache = VK_NULL_HANDLE;

	if (transferQueueTimestampValidBits || computeQueueTimestampValidBits) {
		VkQueryPoolCreateInfo queryPoolCreateInfo = {VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO};
		queryPoolCreateInfo.queryType             = VK_QUERY_TYPE_TIMESTAMP;
		queryPoolCreateInfo.queryCount            = inoutsPerHeap * 4;

		VK_CALL_RES(vkCreateQueryPool, device, &queryPoolCreateInfo, g_allocator, &gpu->queryPool);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	// TEMP probably will move to own function
	if (!gpu->usingPipelineExecutableProperties) {
		free_recursive(dyMem);
		return true;
	}

	VkPipeline pipeline = gpu->pipeline;

	VkPipelineInfoKHR pipelineInfo = {VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR};
	pipelineInfo.pipeline          = pipeline;

	uint32_t executableCount = 1;

	VkPipelineExecutablePropertiesKHR pipelineExecutableProperties = {
		VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR};

	VK_CALL_RES(
		vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, &pipelineExecutableProperties);

	if EXPECT_FALSE (vkres || executableCount != 1) { free_recursive(dyMem); return false; }

	printf(
		"Pipeline Executable Properties:\n"
		"\tStage         = %s\n"
		"\tName          = %s\n"
		"\tDescription   = %s\n"
		"\tSubgroup Size = %" PRIu32 "\n",
		string_VkShaderStageFlagBits((VkShaderStageFlagBits) pipelineExecutableProperties.stages),
		pipelineExecutableProperties.name,
		pipelineExecutableProperties.description,
		pipelineExecutableProperties.subgroupSize);

	VkPipelineExecutableInfoKHR pipelineExecutableInfo = {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR};
	pipelineExecutableInfo.pipeline                    = pipeline;
	pipelineExecutableInfo.executableIndex             = 0;

	uint32_t statisticCount;

	VK_CALL_RES(vkGetPipelineExecutableStatisticsKHR, device, &pipelineExecutableInfo, &statisticCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	size = statisticCount * sizeof(VkPipelineExecutableStatisticKHR);

	p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkPipelineExecutableStatisticKHR* pipelineExecutableStatistics = (VkPipelineExecutableStatisticKHR*) p;

	for (uint32_t i = 0; i < statisticCount; i++) {
		pipelineExecutableStatistics[i] = (VkPipelineExecutableStatisticKHR) {
			VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR};
	}

	VK_CALL_RES(
		vkGetPipelineExecutableStatisticsKHR, device, &pipelineExecutableInfo, &statisticCount,
		pipelineExecutableStatistics);

	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < statisticCount; i++) {
		const char* name        = pipelineExecutableStatistics[i].name;
		const char* description = pipelineExecutableStatistics[i].description;

		VkPipelineExecutableStatisticFormatKHR format = pipelineExecutableStatistics[i].format;
		VkPipelineExecutableStatisticValueKHR  value  = pipelineExecutableStatistics[i].value;

		printf(
			"Pipeline Statistic #%" PRIu32 "\n"
			"\tName        = %s\n"
			"\tDescription = %s\n"
			"\tValue       = ",
			i + 1, name, description);

		switch (format) {
			case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:  printf("%" PRIu32 "\n", value.b32); break;
			case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:   printf("%" PRId64 "\n", value.i64); break;
			case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:  printf("%" PRIu64 "\n", value.u64); break;
			case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR: printf("%f\n",          value.f64); break;
			default: break;
		}
	}
	NEWLINE();

	free_recursive(dyMem);

	return true;
}

bool create_commands(Gpu* restrict gpu)
{
	const VkBuffer*        hostVisibleBuffers = gpu->hostVisibleBuffers;
	const VkBuffer*        deviceLocalBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* descriptorSets     = gpu->descriptorSets;

	VkCommandBuffer* transferCommandBuffers = gpu->transferCommandBuffers;
	VkCommandBuffer* computeCommandBuffers  = gpu->computeCommandBuffers;
	VkSemaphore*     semaphores             = gpu->semaphores;

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
	DyArray dyMem = DyArray_create(sizeof(DyData), 1);
	if EXPECT_FALSE (!dyMem) return false;

	size_t size =
		inoutsPerBuffer * sizeof(VkBufferCopy) * 2 +
		inoutsPerHeap   * sizeof(VkBufferMemoryBarrier2KHR) * 6 +
		inoutsPerHeap   * sizeof(VkDependencyInfoKHR) * 4;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkBufferCopy* inBufferCopies  = (VkBufferCopy*) p;
	VkBufferCopy* outBufferCopies = (VkBufferCopy*) (inBufferCopies + inoutsPerBuffer);

	VkBufferMemoryBarrier2KHR* onetimeBufferMemoryBarriers2 = (VkBufferMemoryBarrier2KHR*) (
		outBufferCopies + inoutsPerBuffer);
	VkBufferMemoryBarrier2KHR (*transferBufferMemoryBarriers2)[3] = (VkBufferMemoryBarrier2KHR(*)[]) (
		onetimeBufferMemoryBarriers2 + inoutsPerHeap);
	VkBufferMemoryBarrier2KHR (*computeBufferMemoryBarriers2)[2] = (VkBufferMemoryBarrier2KHR(*)[]) (
		transferBufferMemoryBarriers2 + inoutsPerHeap);

	VkDependencyInfoKHR (*transferDependencyInfos)[2] = (VkDependencyInfoKHR(*)[]) (
		computeBufferMemoryBarriers2 + inoutsPerHeap);
	VkDependencyInfoKHR (*computeDependencyInfos)[2] = (VkDependencyInfoKHR(*)[]) (
		transferDependencyInfos + inoutsPerHeap);

	VkCommandPoolCreateInfo onetimeCommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	onetimeCommandPoolCreateInfo.flags                   = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	onetimeCommandPoolCreateInfo.queueFamilyIndex        = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo transferCommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	transferCommandPoolCreateInfo.queueFamilyIndex        = transferQueueFamilyIndex;

	VkCommandPoolCreateInfo computeCommandPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	computeCommandPoolCreateInfo.queueFamilyIndex        = computeQueueFamilyIndex;

	VK_CALL_RES(vkCreateCommandPool, device, &onetimeCommandPoolCreateInfo, g_allocator, &gpu->onetimeCommandPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreateCommandPool, device, &transferCommandPoolCreateInfo, g_allocator, &gpu->transferCommandPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreateCommandPool, device, &computeCommandPoolCreateInfo, g_allocator, &gpu->computeCommandPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkCommandPool onetimeCommandPool  = gpu->onetimeCommandPool;
	VkCommandPool transferCommandPool = gpu->transferCommandPool;
	VkCommandPool computeCommandPool  = gpu->computeCommandPool;

	VkCommandBufferAllocateInfo onetimeCommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	onetimeCommandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	onetimeCommandBufferAllocateInfo.commandPool                 = onetimeCommandPool;
	onetimeCommandBufferAllocateInfo.commandBufferCount          = 1;

	VkCommandBufferAllocateInfo transferCommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	transferCommandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCommandBufferAllocateInfo.commandPool                 = transferCommandPool;
	transferCommandBufferAllocateInfo.commandBufferCount          = inoutsPerHeap;

	VkCommandBufferAllocateInfo computeCommandBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	computeCommandBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeCommandBufferAllocateInfo.commandPool                 = computeCommandPool;
	computeCommandBufferAllocateInfo.commandBufferCount          = inoutsPerHeap;

	VK_CALL_RES(vkAllocateCommandBuffers, device, &onetimeCommandBufferAllocateInfo, &gpu->onetimeCommandBuffer);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkAllocateCommandBuffers, device, &transferCommandBufferAllocateInfo, transferCommandBuffers);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkAllocateCommandBuffers, device, &computeCommandBufferAllocateInfo, computeCommandBuffers);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;

	for (uint32_t i = 0; i < inoutsPerBuffer; i++) {
		inBufferCopies[i].srcOffset = bytesPerInout * i;
		inBufferCopies[i].dstOffset = bytesPerInout * i;
		inBufferCopies[i].size      = bytesPerIn;

		outBufferCopies[i].srcOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].dstOffset = bytesPerInout * i + bytesPerIn;
		outBufferCopies[i].size      = bytesPerOut;
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			onetimeBufferMemoryBarriers2[j] = (VkBufferMemoryBarrier2KHR) {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			onetimeBufferMemoryBarriers2[j].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			onetimeBufferMemoryBarriers2[j].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
			onetimeBufferMemoryBarriers2[j].srcQueueFamilyIndex = transferQueueFamilyIndex;
			onetimeBufferMemoryBarriers2[j].dstQueueFamilyIndex = computeQueueFamilyIndex;
			onetimeBufferMemoryBarriers2[j].buffer              = deviceLocalBuffers[i];
			onetimeBufferMemoryBarriers2[j].offset              = bytesPerInout * k;
			onetimeBufferMemoryBarriers2[j].size                = bytesPerIn;

			transferBufferMemoryBarriers2[j][0] = (VkBufferMemoryBarrier2KHR) {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			transferBufferMemoryBarriers2[j][0].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			transferBufferMemoryBarriers2[j][0].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
			transferBufferMemoryBarriers2[j][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers2[j][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers2[j][0].buffer              = deviceLocalBuffers[i];
			transferBufferMemoryBarriers2[j][0].offset              = bytesPerInout * k;
			transferBufferMemoryBarriers2[j][0].size                = bytesPerIn;

			transferBufferMemoryBarriers2[j][1] = (VkBufferMemoryBarrier2KHR) {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			transferBufferMemoryBarriers2[j][1].dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			transferBufferMemoryBarriers2[j][1].dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT_KHR;
			transferBufferMemoryBarriers2[j][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			transferBufferMemoryBarriers2[j][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			transferBufferMemoryBarriers2[j][1].buffer              = deviceLocalBuffers[i];
			transferBufferMemoryBarriers2[j][1].offset              = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers2[j][1].size                = bytesPerOut;

			transferBufferMemoryBarriers2[j][2] = (VkBufferMemoryBarrier2KHR) {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			transferBufferMemoryBarriers2[j][2].srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT_KHR;
			transferBufferMemoryBarriers2[j][2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT_KHR;
			transferBufferMemoryBarriers2[j][2].dstStageMask  = VK_PIPELINE_STAGE_2_HOST_BIT_KHR;
			transferBufferMemoryBarriers2[j][2].dstAccessMask = VK_ACCESS_2_HOST_READ_BIT_KHR;
			transferBufferMemoryBarriers2[j][2].buffer        = hostVisibleBuffers[i];
			transferBufferMemoryBarriers2[j][2].offset        = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers2[j][2].size          = bytesPerOut;

			computeBufferMemoryBarriers2[j][0] = (VkBufferMemoryBarrier2KHR) {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			computeBufferMemoryBarriers2[j][0].dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
			computeBufferMemoryBarriers2[j][0].dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT_KHR;
			computeBufferMemoryBarriers2[j][0].srcQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers2[j][0].dstQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers2[j][0].buffer              = deviceLocalBuffers[i];
			computeBufferMemoryBarriers2[j][0].offset              = bytesPerInout * k;
			computeBufferMemoryBarriers2[j][0].size                = bytesPerIn;

			computeBufferMemoryBarriers2[j][1] = (VkBufferMemoryBarrier2KHR) {
				VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2_KHR};
			computeBufferMemoryBarriers2[j][1].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR;
			computeBufferMemoryBarriers2[j][1].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT_KHR;
			computeBufferMemoryBarriers2[j][1].srcQueueFamilyIndex = computeQueueFamilyIndex;
			computeBufferMemoryBarriers2[j][1].dstQueueFamilyIndex = transferQueueFamilyIndex;
			computeBufferMemoryBarriers2[j][1].buffer              = deviceLocalBuffers[i];
			computeBufferMemoryBarriers2[j][1].offset              = bytesPerInout * k + bytesPerIn;
			computeBufferMemoryBarriers2[j][1].size                = bytesPerOut;

			transferDependencyInfos[j][0] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			transferDependencyInfos[j][0].bufferMemoryBarrierCount = 2;
			transferDependencyInfos[j][0].pBufferMemoryBarriers    = &transferBufferMemoryBarriers2[j][0];

			transferDependencyInfos[j][1] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			transferDependencyInfos[j][1].bufferMemoryBarrierCount = 1;
			transferDependencyInfos[j][1].pBufferMemoryBarriers    = &transferBufferMemoryBarriers2[j][2];

			computeDependencyInfos[j][0] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			computeDependencyInfos[j][0].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[j][0].pBufferMemoryBarriers    = &computeBufferMemoryBarriers2[j][0];

			computeDependencyInfos[j][1] = (VkDependencyInfoKHR) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
			computeDependencyInfos[j][1].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[j][1].pBufferMemoryBarriers    = &computeBufferMemoryBarriers2[j][1];
		}
	}

	VkDependencyInfoKHR onetimeDependencyInfo      = {VK_STRUCTURE_TYPE_DEPENDENCY_INFO_KHR};
	onetimeDependencyInfo.bufferMemoryBarrierCount = inoutsPerHeap;
	onetimeDependencyInfo.pBufferMemoryBarriers    = onetimeBufferMemoryBarriers2;

	VkCommandBufferBeginInfo onetimeCommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
	onetimeCommandBufferBeginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	VkCommandBufferBeginInfo transferCommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VkCommandBufferBeginInfo computeCommandBufferBeginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VK_CALL_RES(vkBeginCommandBuffer, onetimeCommandBuffer, &onetimeCommandBufferBeginInfo);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL(
			vkCmdCopyBuffer, onetimeCommandBuffer, hostVisibleBuffers[i], deviceLocalBuffers[i], inoutsPerBuffer,
			inBufferCopies);
	}

	if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
		VK_CALL(vkCmdPipelineBarrier2KHR, onetimeCommandBuffer, &onetimeDependencyInfo); }

	VK_CALL_RES(vkEndCommandBuffer, onetimeCommandBuffer);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			VK_CALL_RES(vkBeginCommandBuffer, transferCommandBuffers[j], &transferCommandBufferBeginInfo);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			if (transferQueueTimestampValidBits) {
				VK_CALL(vkCmdResetQueryPool, transferCommandBuffers[j], queryPool, j * 4, 2);
				VK_CALL(
					vkCmdWriteTimestamp2KHR, transferCommandBuffers[j], VK_PIPELINE_STAGE_2_NONE_KHR, queryPool, j * 4);
			}

			VK_CALL(
				vkCmdCopyBuffer, transferCommandBuffers[j], hostVisibleBuffers[i], deviceLocalBuffers[i], 1,
				&inBufferCopies[k]);

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				VK_CALL(vkCmdPipelineBarrier2KHR, transferCommandBuffers[j], &transferDependencyInfos[j][0]); }

			VK_CALL(
				vkCmdCopyBuffer, transferCommandBuffers[j], deviceLocalBuffers[i], hostVisibleBuffers[i], 1,
				&outBufferCopies[k]);

			VK_CALL(vkCmdPipelineBarrier2KHR, transferCommandBuffers[j], &transferDependencyInfos[j][1]);

			if (transferQueueTimestampValidBits) {
				VK_CALL(
					vkCmdWriteTimestamp2KHR, transferCommandBuffers[j], VK_PIPELINE_STAGE_2_COPY_BIT_KHR, queryPool,
					j * 4 + 1);
			}

			VK_CALL_RES(vkEndCommandBuffer, transferCommandBuffers[j]);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			VK_CALL_RES(vkBeginCommandBuffer, computeCommandBuffers[j], &computeCommandBufferBeginInfo);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			if (computeQueueTimestampValidBits) {
				VK_CALL(vkCmdResetQueryPool, computeCommandBuffers[j], queryPool, j * 4 + 2, 2);
				VK_CALL(
					vkCmdWriteTimestamp2KHR, computeCommandBuffers[j], VK_PIPELINE_STAGE_2_NONE_KHR, queryPool,
					j * 4 + 2);
			}

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				VK_CALL(vkCmdPipelineBarrier2KHR, computeCommandBuffers[j], &computeDependencyInfos[j][0]); }

			VK_CALL(
				vkCmdBindDescriptorSets, computeCommandBuffers[j], VK_PIPELINE_BIND_POINT_COMPUTE, pipelineLayout, 0, 1,
				&descriptorSets[j], 0, NULL);
			VK_CALL(vkCmdBindPipeline, computeCommandBuffers[j], VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
			VK_CALL(vkCmdDispatchBase, computeCommandBuffers[j], 0, 0, 0, workgroupCount, 1, 1);

			if (transferQueueFamilyIndex != computeQueueFamilyIndex) {
				VK_CALL(vkCmdPipelineBarrier2KHR, computeCommandBuffers[j], &computeDependencyInfos[j][1]); }

			if (computeQueueTimestampValidBits) {
				VK_CALL(
					vkCmdWriteTimestamp2KHR, computeCommandBuffers[j], VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT_KHR,
					queryPool, j * 4 + 3);
			}

			VK_CALL_RES(vkEndCommandBuffer, computeCommandBuffers[j]);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
		}
	}

	if (gpu->usingMaintenance4) {
		VK_CALL(vkDestroyPipelineLayout, device, pipelineLayout, g_allocator);
		gpu->pipelineLayout = VK_NULL_HANDLE;
	}

	VkSemaphoreTypeCreateInfoKHR semaphoreTypeCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO_KHR};
	semaphoreTypeCreateInfo.semaphoreType                = VK_SEMAPHORE_TYPE_TIMELINE_KHR;
	semaphoreTypeCreateInfo.initialValue                 = 0;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	semaphoreCreateInfo.pNext                 = &semaphoreTypeCreateInfo;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VK_CALL_RES(vkCreateSemaphore, device, &semaphoreCreateInfo, g_allocator, &semaphores[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	free_recursive(dyMem);

#ifndef NDEBUG
	if (gpu->debugUtilsMessenger) {
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL,   (uint64_t) onetimeCommandPool,   "Onetime");
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) onetimeCommandBuffer, "Onetime");

		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) transferCommandPool, "Transfer");
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) computeCommandPool,  "Compute");

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

				set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) transferCommandBuffers[j], objectName);

				strcpy(objectName, "Compute");
				strcat(objectName, specs);

				set_debug_name(device, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t) computeCommandBuffers[j], objectName);
			}
		}
	}
#endif

	return true;
}

bool submit_commands(Gpu* restrict gpu)
{
	const VkDeviceMemory*  hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer* transferCommandBuffers    = gpu->transferCommandBuffers;
	const VkCommandBuffer* computeCommandBuffers     = gpu->computeCommandBuffers;
	const VkSemaphore*     semaphores                = gpu->semaphores;

	Value*       const* mappedInBuffers  = gpu->mappedInBuffers;
	const Count* const* mappedOutBuffers = (const Count* const*) gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkQueue transferQueue = gpu->transferQueue;
	VkQueue computeQueue  = gpu->computeQueue;

	VkQueryPool queryPool = gpu->queryPool;

	VkCommandPool   onetimeCommandPool   = gpu->onetimeCommandPool;
	VkCommandBuffer onetimeCommandBuffer = gpu->onetimeCommandBuffer;

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

	CliOutput outputLevel = gpu->outputLevel;
	uint64_t  maxLoops    = gpu->maxLoops;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 3);
	if EXPECT_FALSE (!dyMem) return false;

	size_t size =
		inoutsPerHeap * sizeof(Value) +
		inoutsPerHeap * sizeof(VkMappedMemoryRange) * 2 +
		inoutsPerHeap * sizeof(VkSubmitInfo2KHR) * 2 +
		inoutsPerHeap * sizeof(VkCommandBufferSubmitInfoKHR) * 2 +
		inoutsPerHeap * sizeof(VkSemaphoreSubmitInfoKHR) * 4 +
		inoutsPerHeap * sizeof(VkSemaphoreWaitInfoKHR) * 2;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	Value* testedValues = (Value*) p;

	VkMappedMemoryRange* hostVisibleInBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (testedValues + inoutsPerHeap);
	VkMappedMemoryRange* hostVisibleOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (
		hostVisibleInBuffersMappedMemoryRanges + inoutsPerHeap);

	VkSubmitInfo2KHR* transferSubmitInfos2 = (VkSubmitInfo2KHR*) (
		hostVisibleOutBuffersMappedMemoryRanges + inoutsPerHeap);
	VkSubmitInfo2KHR* computeSubmitInfos2 = (VkSubmitInfo2KHR*) (transferSubmitInfos2 + inoutsPerHeap);

	VkCommandBufferSubmitInfoKHR* transferCommandBufferSubmitInfos = (VkCommandBufferSubmitInfoKHR*) (
		computeSubmitInfos2 + inoutsPerHeap);
	VkCommandBufferSubmitInfoKHR* computeCommandBufferSubmitInfos = (VkCommandBufferSubmitInfoKHR*) (
		transferCommandBufferSubmitInfos + inoutsPerHeap);

	VkSemaphoreSubmitInfoKHR* transferWaitSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (
		computeCommandBufferSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfoKHR* transferSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (
		transferWaitSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfoKHR* computeWaitSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (
		transferSignalSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreSubmitInfoKHR* computeSignalSemaphoreSubmitInfos = (VkSemaphoreSubmitInfoKHR*) (
		computeWaitSemaphoreSubmitInfos + inoutsPerHeap);

	VkSemaphoreWaitInfoKHR* transferSemaphoreWaitInfos = (VkSemaphoreWaitInfoKHR*) (
		computeSignalSemaphoreSubmitInfos + inoutsPerHeap);
	VkSemaphoreWaitInfoKHR* computeSemaphoreWaitInfos = (VkSemaphoreWaitInfoKHR*) (
		transferSemaphoreWaitInfos + inoutsPerHeap);

	DyArray highestStepValues = DyArray_create(sizeof(Value), 64);
	if EXPECT_FALSE (!highestStepValues) { free_recursive(dyMem); return false; }

	dyData = (DyData) {highestStepValues, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	DyArray highestStepCounts = DyArray_create(sizeof(Count), 64);
	if EXPECT_FALSE (!highestStepCounts) { free_recursive(dyMem); return false; }

	dyData = (DyData) {highestStepCounts, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	size_t fileSize;
	bool bres = file_size(PROGRESS_NAME, &fileSize);
	if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

	ValueInfo prevValues = {0};

	if (!gpu->restartCount && fileSize) {
		uint64_t val0mod1off0Upper, val0mod1off0Lower;
		uint64_t val0mod1off1Upper, val0mod1off1Lower;
		uint64_t val0mod1off2Upper, val0mod1off2Lower;
		uint64_t val1mod6off0Upper, val1mod6off0Lower;
		uint64_t val1mod6off1Upper, val1mod6off1Lower;
		uint64_t val1mod6off2Upper, val1mod6off2Lower;
		uint64_t curValueUpper, curValueLower;
		uint16_t curCount;

		bres = read_text(
			PROGRESS_NAME,
			"%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n"
			"%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n%" SCNx64 " %" SCNx64 "\n"
			"%" SCNx64 " %" SCNx64 "\n%" SCNx16,
			&val0mod1off0Upper, &val0mod1off0Lower,
			&val0mod1off1Upper, &val0mod1off1Lower,
			&val0mod1off2Upper, &val0mod1off2Lower,
			&val1mod6off0Upper, &val1mod6off0Lower,
			&val1mod6off1Upper, &val1mod6off1Lower,
			&val1mod6off2Upper, &val1mod6off2Lower,
			&curValueUpper, &curValueLower, &curCount);

		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

		prevValues.val0mod1off[0] = INT128(val0mod1off0Upper, val0mod1off0Lower);
		prevValues.val0mod1off[1] = INT128(val0mod1off1Upper, val0mod1off1Lower);
		prevValues.val0mod1off[2] = INT128(val0mod1off2Upper, val0mod1off2Lower);

		prevValues.val1mod6off[0] = INT128(val1mod6off0Upper, val1mod6off0Lower);
		prevValues.val1mod6off[1] = INT128(val1mod6off1Upper, val1mod6off1Lower);
		prevValues.val1mod6off[2] = INT128(val1mod6off2Upper, val1mod6off2Lower);

		prevValues.curValue = INT128(curValueUpper, curValueLower);
		prevValues.curCount = curCount;
	}
	else {
		prevValues.val0mod1off[0] = 1;
		prevValues.curValue       = 3;
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			hostVisibleInBuffersMappedMemoryRanges[j] = (VkMappedMemoryRange) {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			hostVisibleInBuffersMappedMemoryRanges[j].memory = hostVisibleDeviceMemories[i];
			hostVisibleInBuffersMappedMemoryRanges[j].offset = bytesPerInout * k;
			hostVisibleInBuffersMappedMemoryRanges[j].size   = bytesPerIn;

			hostVisibleOutBuffersMappedMemoryRanges[j] = (VkMappedMemoryRange) {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			hostVisibleOutBuffersMappedMemoryRanges[j].memory = hostVisibleDeviceMemories[i];
			hostVisibleOutBuffersMappedMemoryRanges[j].offset = bytesPerInout * k + bytesPerIn;
			hostVisibleOutBuffersMappedMemoryRanges[j].size   = bytesPerOut;
		}
	}

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferCommandBufferSubmitInfos[i] = (VkCommandBufferSubmitInfoKHR) {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR};
		transferCommandBufferSubmitInfos[i].commandBuffer = transferCommandBuffers[i];

		computeCommandBufferSubmitInfos[i] = (VkCommandBufferSubmitInfoKHR) {
			VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO_KHR};
		computeCommandBufferSubmitInfos[i].commandBuffer = computeCommandBuffers[i];

		transferWaitSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		transferWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferWaitSemaphoreSubmitInfos[i].value     = 0;
		transferWaitSemaphoreSubmitInfos[i].stageMask =
			VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include acquire operation

		transferSignalSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {
			VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		transferSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferSignalSemaphoreSubmitInfos[i].value     = 1;
		transferSignalSemaphoreSubmitInfos[i].stageMask =
			VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include release operation

		computeWaitSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		computeWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeWaitSemaphoreSubmitInfos[i].value     = 1;
		computeWaitSemaphoreSubmitInfos[i].stageMask =
			VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include acquire operation

		computeSignalSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfoKHR) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO_KHR};
		computeSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeSignalSemaphoreSubmitInfos[i].value     = 2;
		computeSignalSemaphoreSubmitInfos[i].stageMask =
			VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT_KHR; // Need to include release operation

		transferSubmitInfos2[i]                          = (VkSubmitInfo2KHR) {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
		transferSubmitInfos2[i].waitSemaphoreInfoCount   = 1;
		transferSubmitInfos2[i].pWaitSemaphoreInfos      = &transferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos2[i].commandBufferInfoCount   = 1;
		transferSubmitInfos2[i].pCommandBufferInfos      = &transferCommandBufferSubmitInfos[i];
		transferSubmitInfos2[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos2[i].pSignalSemaphoreInfos    = &transferSignalSemaphoreSubmitInfos[i];

		computeSubmitInfos2[i]                          = (VkSubmitInfo2KHR) {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
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
	onetimeCommandBufferSubmitInfo.commandBuffer                = onetimeCommandBuffer;

	VkSubmitInfo2KHR onetimeSubmitInfo2         = {VK_STRUCTURE_TYPE_SUBMIT_INFO_2_KHR};
	onetimeSubmitInfo2.commandBufferInfoCount   = 1;
	onetimeSubmitInfo2.pCommandBufferInfos      = &onetimeCommandBufferSubmitInfo;
	onetimeSubmitInfo2.signalSemaphoreInfoCount = inoutsPerHeap;
	onetimeSubmitInfo2.pSignalSemaphoreInfos    = transferSignalSemaphoreSubmitInfos;

	clock_t bmarkStart = clock();

	Value tested = prevValues.curValue;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		testedValues[i] = tested;
		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);
		tested += valuesPerInout * 4;
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hostVisibleInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, 1, &onetimeSubmitInfo2, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, inoutsPerHeap, computeSubmitInfos2, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	atomic_bool input;
	atomic_init(&input, false);

	pthread_t waitThread;
	int ires = pthread_create(&waitThread, NULL, wait_for_input, &input);
	if EXPECT_FALSE (ires) { PCREATE_FAILURE(ires, &waitThread, NULL); free_recursive(dyMem); return false; }

	VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[0], UINT64_MAX);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL(vkDestroyCommandPool, device, onetimeCommandPool, g_allocator);
	gpu->onetimeCommandPool = VK_NULL_HANDLE;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hostVisibleInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferWaitSemaphoreSubmitInfos[i].value   += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, inoutsPerHeap, transferSubmitInfos2, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	Value total      = 0;
	Value startValue = prevValues.curValue;

	// ===== Enter main loop =====
	for (uint64_t i = 0; i < maxLoops && !atomic_load(&input); i++) {
		clock_t mainLoopBmarkStart = clock();

		Value initialValue = prevValues.curValue;

		float readBmarkTotal         = 0;
		float writeBmarkTotal        = 0;
		float waitComputeBmarkTotal  = 0;
		float waitTransferBmarkTotal = 0;
		float computeBmarkTotal      = 0;
		float transferBmarkTotal     = 0;

		if (outputLevel > CLI_OUTPUT_SILENT) { printf("Loop #%" PRIu64 "\n", i + 1); }

		for (uint32_t j = 0; j < inoutsPerHeap; j++) {
			uint64_t timestamps[2];

			float computeBmark  = 0;
			float transferBmark = 0;

			clock_t waitComputeBmarkStart = clock();
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &computeSemaphoreWaitInfos[j], UINT64_MAX);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			clock_t waitComputeBmarkEnd = clock();

			if (computeQueueTimestampValidBits) {
				VK_CALL_RES(
					vkGetQueryPoolResults, device, queryPool, j * 4 + 2, 2, sizeof(timestamps), timestamps,
					sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT);

				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

				computeBmark = (float) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			computeWaitSemaphoreSubmitInfos[j].value   += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, 1, &computeSubmitInfos2[j], VK_NULL_HANDLE);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			clock_t waitTransferBmarkStart = clock();
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[j], UINT64_MAX);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			clock_t waitTransferBmarkEnd = clock();

			if (transferQueueTimestampValidBits) {
				VK_CALL_RES(
					vkGetQueryPoolResults, device, queryPool, j * 4, 2, sizeof(timestamps), timestamps,
					sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT);

				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

				transferBmark = (float) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (hostNonCoherent) {
				VK_CALL_RES(vkInvalidateMappedMemoryRanges, device, 1, &hostVisibleOutBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			}

			clock_t readBmarkStart = clock();
			read_outbuffer(mappedOutBuffers[j], &prevValues, highestStepValues, highestStepCounts, valuesPerInout);
			clock_t readBmarkEnd = clock();

			clock_t writeBmarkStart = clock();
			write_inbuffer(mappedInBuffers[j], &testedValues[j], valuesPerInout, valuesPerHeap);
			clock_t writeBmarkEnd = clock();

			if (hostNonCoherent) {
				VK_CALL_RES(vkFlushMappedMemoryRanges, device, 1, &hostVisibleInBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			}

			transferWaitSemaphoreSubmitInfos[j].value   += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, 1, &transferSubmitInfos2[j], VK_NULL_HANDLE);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return NULL; }

			float readBmark         = get_benchmark(readBmarkStart, readBmarkEnd);
			float writeBmark        = get_benchmark(writeBmarkStart, writeBmarkEnd);
			float waitComputeBmark  = get_benchmark(waitComputeBmarkStart, waitComputeBmarkEnd);
			float waitTransferBmark = get_benchmark(waitTransferBmarkStart, waitTransferBmarkEnd);

			readBmarkTotal         += readBmark;
			writeBmarkTotal        += writeBmark;
			computeBmarkTotal      += computeBmark;
			transferBmarkTotal     += transferBmark;
			waitComputeBmarkTotal  += waitComputeBmark;
			waitTransferBmarkTotal += waitTransferBmark;

			if (outputLevel > CLI_OUTPUT_QUIET) {
				printf(
					"Inout-buffer %" PRIu32 "/%" PRIu32 "\n"
					"\tReading buffers:    %8.0fms\n"
					"\tWriting buffers:    %8.0fms\n"
					"\tCompute execution:  %8.0fms\n"
					"\tTransfer execution: %8.0fms\n"
					"\tIdle (compute):     %8.0fms\n"
					"\tIdle (transfer):    %8.0fms\n",
					j + 1, inoutsPerHeap,
					(double) readBmark,        (double) writeBmark,
					(double) computeBmark,     (double) transferBmark,
					(double) waitComputeBmark, (double) waitTransferBmark);
			}
		}

		total += valuesPerHeap * 4;

		clock_t mainLoopBmarkEnd = clock();
		float   mainLoopBmark    = get_benchmark(mainLoopBmarkStart, mainLoopBmarkEnd);

		Value currentValue = prevValues.curValue;

		switch (outputLevel) {
			case CLI_OUTPUT_SILENT: break;
			case CLI_OUTPUT_QUIET:
				printf(
					"Main loop: %.0fms\n"
					"Current value: 0x %016" PRIx64 " %016" PRIx64 "\n\n",
					(double) mainLoopBmark,
					INT128_UPPER(currentValue - 3), INT128_LOWER(currentValue - 3));
				break;
			case CLI_OUTPUT_DEFAULT:
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
					(double) mainLoopBmark,
					(double) (readBmarkTotal  / (float) inoutsPerHeap),
					(double) (writeBmarkTotal / (float) inoutsPerHeap),
					(double) (computeBmarkTotal  / (float) inoutsPerHeap),
					(double) (transferBmarkTotal / (float) inoutsPerHeap),
					(double) (waitComputeBmarkTotal  / (float) inoutsPerHeap),
					(double) (waitTransferBmarkTotal / (float) inoutsPerHeap),
					INT128_UPPER(initialValue - 2), INT128_LOWER(initialValue - 2),
					INT128_UPPER(currentValue - 3), INT128_LOWER(currentValue - 3));
				break;
			case CLI_OUTPUT_VERBOSE:
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
					(double) mainLoopBmark,
					(double) readBmarkTotal,  (double) (readBmarkTotal  / (float) inoutsPerHeap),
					(double) writeBmarkTotal, (double) (writeBmarkTotal / (float) inoutsPerHeap),
					(double) computeBmarkTotal,  (double) (computeBmarkTotal  / (float) inoutsPerHeap),
					(double) transferBmarkTotal, (double) (transferBmarkTotal / (float) inoutsPerHeap),
					(double) waitComputeBmarkTotal,  (double) (waitComputeBmarkTotal  / (float) inoutsPerHeap),
					(double) waitTransferBmarkTotal, (double) (waitTransferBmarkTotal / (float) inoutsPerHeap),
					INT128_UPPER(initialValue - 2), INT128_LOWER(initialValue - 2),
					INT128_UPPER(currentValue - 3), INT128_LOWER(currentValue - 3));
				break;
			default: break;
		}
	}
	NEWLINE();

	clock_t bmarkEnd = clock();
	float   bmark    = get_benchmark(bmarkStart, bmarkEnd);

	uint32_t count = (uint32_t) DyArray_size(highestStepValues);

	Value endValue = prevValues.curValue;

	if (outputLevel > CLI_OUTPUT_SILENT) {
		printf(
			"Set of starting values tested: [0x %016" PRIx64 " %016" PRIx64 ", 0x %016" PRIx64 " %016" PRIx64 "]\n",
			INT128_UPPER(startValue - 2), INT128_LOWER(startValue - 2),
			INT128_UPPER(endValue - 3),   INT128_LOWER(endValue - 3));
	}

	if (count) {
		printf(
			"New highest step counts (%" PRIu32 "):\n"
			"|   #   |   Starting value (hexadecimal)    | Step count |\n",
			count);
	}

	for (uint32_t i = 0; i < count; i++) {
		Value value;
		Count steps;

		DyArray_get(highestStepValues, &value, i);
		DyArray_get(highestStepCounts, &steps, i);

		printf(
			"| %5" PRIu32 " | %016" PRIx64 " %016" PRIx64 " | %10" PRIu16 " |\n",
			i + 1, INT128_UPPER(value), INT128_LOWER(value), steps);
	}

	if (outputLevel > CLI_OUTPUT_QUIET) {
		printf("\nTime: %.0fms\nSpeed: %.0f/s\n", (double) bmark, (double) (1000 * total) / (double) bmark); }

	ires = pthread_join(waitThread, NULL);
	if EXPECT_FALSE (ires) { PJOIN_FAILURE(ires, waitThread, NULL); free_recursive(dyMem); return false; }

	if (!gpu->restartCount) {
		bres = write_text(
			PROGRESS_NAME,
			"%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n"
			"%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n%016" PRIx64 " %016" PRIx64 "\n"
			"%016" PRIx64 " %016" PRIx64 "\n%04"  PRIx16,
			INT128_UPPER(prevValues.val0mod1off[0]), INT128_LOWER(prevValues.val0mod1off[0]),
			INT128_UPPER(prevValues.val0mod1off[1]), INT128_LOWER(prevValues.val0mod1off[1]),
			INT128_UPPER(prevValues.val0mod1off[2]), INT128_LOWER(prevValues.val0mod1off[2]),
			INT128_UPPER(prevValues.val1mod6off[0]), INT128_LOWER(prevValues.val1mod6off[0]),
			INT128_UPPER(prevValues.val1mod6off[1]), INT128_LOWER(prevValues.val1mod6off[1]),
			INT128_UPPER(prevValues.val1mod6off[2]), INT128_LOWER(prevValues.val1mod6off[2]),
			INT128_UPPER(prevValues.curValue), INT128_LOWER(prevValues.curValue), prevValues.curCount);

		if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
	}

	free_recursive(dyMem);

	return true;
}

bool destroy_gpu(Gpu* restrict gpu)
{
	VkInstance instance = volkGetLoadedInstance();

	const VkBuffer*       hostVisibleBuffers        = gpu->hostVisibleBuffers;
	const VkBuffer*       deviceLocalBuffers        = gpu->deviceLocalBuffers;
	const VkDeviceMemory* hostVisibleDeviceMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* deviceLocalDeviceMemories = gpu->deviceLocalDeviceMemories;
	const VkSemaphore*    semaphores                = gpu->semaphores;

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

	VkResult vkres;

	if (device) {
		VK_CALL(vkDestroyShaderModule,        device, shaderModule,        g_allocator);
		VK_CALL(vkDestroyPipelineCache,       device, pipelineCache,       g_allocator);
		VK_CALL(vkDestroyPipelineLayout,      device, pipelineLayout,      g_allocator);
		VK_CALL(vkDestroyDescriptorSetLayout, device, descriptorSetLayout, g_allocator);

		// Make sure no command buffers are in the pending state
		VK_CALL_RES(vkDeviceWaitIdle, device);

		for (uint32_t i = 0; i < inoutsPerHeap; i++) {
			VK_CALL(vkDestroySemaphore, device, semaphores[i], g_allocator);
		}

		VK_CALL(vkDestroyCommandPool, device, onetimeCommandPool,  g_allocator);
		VK_CALL(vkDestroyCommandPool, device, computeCommandPool,  g_allocator);
		VK_CALL(vkDestroyCommandPool, device, transferCommandPool, g_allocator);

		VK_CALL(vkDestroyPipeline,       device, pipeline,       g_allocator);
		VK_CALL(vkDestroyQueryPool,      device, queryPool,      g_allocator);
		VK_CALL(vkDestroyDescriptorPool, device, descriptorPool, g_allocator);

		for (uint32_t i = 0; i < buffersPerHeap; i++) {
			VK_CALL(vkDestroyBuffer, device, hostVisibleBuffers[i], g_allocator);
			VK_CALL(vkDestroyBuffer, device, deviceLocalBuffers[i], g_allocator);

			VK_CALL(vkFreeMemory, device, hostVisibleDeviceMemories[i], g_allocator);
			VK_CALL(vkFreeMemory, device, deviceLocalDeviceMemories[i], g_allocator);
		}

		VK_CALL(vkDestroyDevice, device, g_allocator);
	}

	if (instance) {
#ifndef NDEBUG
		VK_CALL(vkDestroyDebugUtilsMessengerEXT, instance, debugUtilsMessenger, g_allocator);
#endif
		VK_CALL(vkDestroyInstance, instance, g_allocator);
	}

	free(gpu->dynamicMemory);
	volkFinalize();

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

void write_inbuffer(
	Value* restrict mappedInBuffer, Value* restrict firstValue, uint32_t valuesPerInout, uint32_t valuesPerHeap)
{
	ASSUME(*firstValue % 8 == 3);
	ASSUME(valuesPerInout % 128 == 0);
	ASSUME(valuesPerInout != 0);

	Value value = *firstValue;

	for (uint32_t i = 0; i < valuesPerInout; i++) {
		mappedInBuffer[i] = value;
		value += 4;
	}

	*firstValue += valuesPerHeap * 4;
}

void read_outbuffer(
	const Count* restrict mappedOutBuffer,
	ValueInfo* restrict prevValues,
	restrict DyArray highestStepValues,
	restrict DyArray highestStepCounts,
	uint32_t valuesPerInout)
{
	ASSUME(prevValues->curValue % 8 == 3);
	ASSUME(valuesPerInout % 128 == 0);
	ASSUME(valuesPerInout != 0);

	Value val0mod1off[3];
	Value val1mod6off[3];

	memcpy(val0mod1off, prevValues->val0mod1off, sizeof(val0mod1off));
	memcpy(val1mod6off, prevValues->val1mod6off, sizeof(val1mod6off));

	Value value = prevValues->curValue - 2;
	Count count = prevValues->curCount;

	for (uint32_t i = 0; i < valuesPerInout;) {
		value++; // value % 8 == 2

		if (value == val0mod1off[0] * 2) {
			new_high(
				&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, highestStepValues, highestStepCounts);
		}
		else {
			for (uint32_t j = 2; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 1] || value != val0mod1off[j] * 2) continue;

				val0mod1off[j - 1] = value;

				break;
			}
		}

		value++; // value % 8 == 3

		if (mappedOutBuffer[i] > count) {
			new_high(
				&value, &count, mappedOutBuffer[i], val0mod1off, val1mod6off, highestStepValues, highestStepCounts);
		}
		else if (mappedOutBuffer[i] == count && !val1mod6off[0] && value % 6 == 1) { val1mod6off[0] = value; }
		else {
			for (uint32_t j = 1; j < ARR_SIZE(val0mod1off); j++) {
				if (mappedOutBuffer[i] != count - j) continue;

				if (!val0mod1off[j])                   { val0mod1off[j] = value; }
				if (!val1mod6off[j] && value % 6 == 1) { val1mod6off[j] = value; }

				break;
			}
		}

		value++; // value % 8 == 4

		if (value == val0mod1off[1] * 4) {
			new_high(
				&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, highestStepValues, highestStepCounts);
		}
		else {
			for (uint32_t j = 3; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 2] || value != val0mod1off[j] * 4) continue;

				val0mod1off[j - 2] = value;

				break;
			}
		}

		value++; // value % 8 == 5

		if (value % 6 == 1) {
			for (uint32_t j = 0; j < ARR_SIZE(val0mod1off); j++) {
				if (val1mod6off[j] || value - 1 != val0mod1off[j]) continue;

				val1mod6off[j] = value;

				break;
			}
		}

		i++;

		value++; // value % 8 == 6

		if (value == val0mod1off[0] * 2) {
			new_high(
				&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, highestStepValues, highestStepCounts);
		}
		else {
			for (uint32_t j = 2; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 1] || value != val0mod1off[j] * 2) continue;

				val0mod1off[j - 1] = value;

				break;
			}
		}

		value++; // value % 8 == 7

		if (mappedOutBuffer[i] > count) {
			new_high(
				&value, &count, mappedOutBuffer[i], val0mod1off, val1mod6off, highestStepValues, highestStepCounts);
		}
		else if (mappedOutBuffer[i] == count && !val1mod6off[0] && value % 6 == 1) { val1mod6off[0] = value; }
		else {
			for (uint32_t j = 1; j < ARR_SIZE(val0mod1off); j++) {
				if (mappedOutBuffer[i] != count - j) continue;

				if (!val0mod1off[j])                   { val0mod1off[j] = value; }
				if (!val1mod6off[j] && value % 6 == 1) { val1mod6off[j] = value; }
			
				break;
			}
		}

		value++; // value % 8 == 0

		if (value == val0mod1off[2] * 8) {
			new_high(
				&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, highestStepValues, highestStepCounts);
		}
		else {
			for (uint32_t j = 4; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 3] || value != val0mod1off[j] * 8) continue;

				val0mod1off[j - 3] = value;

				break;
			}
		}

		value++; // value % 8 == 1

		for (uint32_t j = 0; j < ARR_SIZE(val0mod1off); j++) {
			if (!val1mod6off[j] || (value - 1) / 4 != (val1mod6off[j] - 1) / 3) continue;

			new_high(
				&value, &count, (Count) (count - j + 3), val0mod1off, val1mod6off, highestStepValues,
				highestStepCounts);

			break;
		}

		i++;
	}

	memcpy(prevValues->val0mod1off, val0mod1off, sizeof(val0mod1off));
	memcpy(prevValues->val1mod6off, val1mod6off, sizeof(val1mod6off));

	prevValues->curValue = value + 2;
	prevValues->curCount = count;
}

void new_high(
	const Value* restrict value,
	Count* restrict count,
	Count newCount,
	Value* restrict val0mod1off,
	Value* restrict val1mod6off,
	restrict DyArray highestStepValues,
	restrict DyArray highestStepCounts)
{
	uint32_t oldCount = *count;
	uint32_t difCount = minu32(newCount - oldCount, 3);

	*count = newCount;

	memmove(val0mod1off + difCount, val0mod1off, sizeof(Value) * (3 - difCount));
	memmove(val1mod6off + difCount, val1mod6off, sizeof(Value) * (3 - difCount));

	memset(val0mod1off + 1, 0, sizeof(Value) * (difCount - 1));
	memset(val1mod6off + 1, 0, sizeof(Value) * (difCount - 1));

	val0mod1off[0] = *value;
	val1mod6off[0] = *value % 6 == 1 ? *value : 0;

	DyArray_append(highestStepValues, value);
	DyArray_append(highestStepCounts, count);
}
