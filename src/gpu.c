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

	VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo = {VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT};
	messengerCreateInfo.messageSeverity =
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	messengerCreateInfo.messageType =
		VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	messengerCreateInfo.pfnUserCallback = debug_callback;
	messengerCreateInfo.pUserData       = &g_callbackData;
#endif

	uint32_t layerCount;
	uint32_t extCount;

	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	size_t size = layerCount * sizeof(VkLayerProperties) + extCount * sizeof(VkExtensionProperties);

	void* p = malloc(size);
	if EXPECT_FALSE (!p && size) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkLayerProperties*     layersProps = (VkLayerProperties*) p;
	VkExtensionProperties* extsProps   = (VkExtensionProperties*) (layersProps + layerCount);

	VK_CALL_RES(vkEnumerateInstanceLayerProperties, &layerCount, layersProps);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkEnumerateInstanceExtensionProperties, NULL, &extCount, extsProps);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	DyArray enabledLayers = DyArray_create(sizeof(const char*), 4);
	if EXPECT_FALSE (!enabledLayers) { free_recursive(dyMem); return false; }

	dyData = (DyData) {enabledLayers, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < layerCount; i++) {
		const char* layerName = layersProps[i].layerName;

		if (
			(gpu->extensionLayers && (
				!strcmp(layerName, VK_KHR_SYNCHRONIZATION_2_LAYER_NAME) ||
				!strcmp(layerName, VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME))) ||
			(gpu->profileLayers && !strcmp(layerName, VK_KHR_PROFILES_LAYER_NAME)) ||
			(gpu->validationLayers && !strcmp(layerName, VK_KHR_VALIDATION_LAYER_NAME)))
		{
			DyArray_append(enabledLayers, &layerName);
		}
	}

	const void*  nextChain = NULL;
	const void** next      = &nextChain;

	bool usingPortabilityEnumeration = false;
	bool usingDebugUtils             = false;

	DyArray enabledExts = DyArray_create(sizeof(const char*), 2);
	if EXPECT_FALSE (!enabledExts) { free_recursive(dyMem); return false; }

	dyData = (DyData) {enabledExts, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	for (uint32_t i = 0; i < extCount; i++) {
		const char* extName = extsProps[i].extensionName;

		if (!strcmp(extName, VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME)) {
			DyArray_append(enabledExts, &extName);
			usingPortabilityEnumeration = true;
			continue;
		}

#ifndef NDEBUG
		if (!strcmp(extName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME)) {
			PNEXT_ADD(next, messengerCreateInfo);
			DyArray_append(enabledExts, &extName);
			usingDebugUtils = true;
			continue;
		}
#endif
	}

	VkApplicationInfo appInfo  = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
	appInfo.pApplicationName   = PROGRAM_NAME;
	appInfo.applicationVersion = PROGRAM_VERSION;
	appInfo.apiVersion         = VK_API_VERSION_1_3;

	uint32_t     enabledLayerCount = (uint32_t) DyArray_size(enabledLayers);
	const char** enabledLayerNames = (const char**) DyArray_raw(enabledLayers);

	uint32_t     enabledExtCount = (uint32_t) DyArray_size(enabledExts);
	const char** enabledExtNames = (const char**) DyArray_raw(enabledExts);

	VkInstanceCreateInfo instanceCreateInfo = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
	instanceCreateInfo.pNext                = nextChain;
	instanceCreateInfo.flags = usingPortabilityEnumeration ? VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR : 0;
	instanceCreateInfo.pApplicationInfo        = &appInfo;
	instanceCreateInfo.enabledLayerCount       = enabledLayerCount;
	instanceCreateInfo.ppEnabledLayerNames     = enabledLayerNames;
	instanceCreateInfo.enabledExtensionCount   = enabledExtCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtNames;

	if (gpu->outputLevel > CLI_OUTPUT_DEFAULT) {
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

	free_recursive(dyMem);
	volkLoadInstanceOnly(instance);

#ifndef NDEBUG
	if (usingDebugUtils) {
		VK_CALL_RES(
			vkCreateDebugUtilsMessengerEXT, instance, &messengerCreateInfo, g_allocator, &gpu->debugUtilsMessenger);
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

	uint32_t deviceCount;
	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &deviceCount, NULL);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	if EXPECT_FALSE (!deviceCount) {
		fputs("Runtime failure\nNo physical devices are accessible to the Vulkan instance\n\n", stderr);
		free_recursive(dyMem);
		return false;
	}

	size_t size =
		deviceCount * sizeof(VkPhysicalDevice) +
		deviceCount * sizeof(VkQueueFamilyProperties2*) +
		deviceCount * sizeof(VkExtensionProperties*) +
		deviceCount * sizeof(VkPhysicalDeviceMemoryProperties2) +
		deviceCount * sizeof(VkPhysicalDeviceProperties2) +
		deviceCount * sizeof(VkPhysicalDeviceFeatures2) +
		deviceCount * sizeof(VkPhysicalDevice16BitStorageFeatures) +
		deviceCount * sizeof(VkPhysicalDevice8BitStorageFeatures) +
		deviceCount * sizeof(uint32_t) * 2;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkPhysicalDevice* devices = (VkPhysicalDevice*) p;

	VkQueueFamilyProperties2** qfsProps  = (VkQueueFamilyProperties2**) (devices + deviceCount);
	VkExtensionProperties**    extsProps = (VkExtensionProperties**) (qfsProps + deviceCount);

	VkPhysicalDeviceMemoryProperties2* devsMemoryProps = (VkPhysicalDeviceMemoryProperties2*) (extsProps + deviceCount);
	VkPhysicalDeviceProperties2*       devsProps       = (VkPhysicalDeviceProperties2*) (devsMemoryProps + deviceCount);

	VkPhysicalDeviceFeatures2* devsFeats = (VkPhysicalDeviceFeatures2*) (devsProps + deviceCount);
	VkPhysicalDevice16BitStorageFeatures* devs16BitStorageFeats = (VkPhysicalDevice16BitStorageFeatures*) (
		devsFeats + deviceCount);
	VkPhysicalDevice8BitStorageFeatures* devs8BitStorageFeats = (VkPhysicalDevice8BitStorageFeatures*) (
		devs16BitStorageFeats + deviceCount);

	uint32_t* extCounts = (uint32_t*) (devs8BitStorageFeats + deviceCount);
	uint32_t* qfCounts  = (uint32_t*) (extCounts + deviceCount);

	VK_CALL_RES(vkEnumeratePhysicalDevices, instance, &deviceCount, devices);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	size_t qfTotal  = 0;
	size_t extTotal = 0;

	for (uint32_t i = 0; i < deviceCount; i++) {
		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, devices[i], &qfCounts[i], NULL);

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, devices[i], NULL, &extCounts[i], NULL);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		qfTotal  += qfCounts[i];
		extTotal += extCounts[i];
	}

	size = qfTotal * sizeof(VkQueueFamilyProperties2) + extTotal * sizeof(VkExtensionProperties);

	p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	qfsProps[0]  = (VkQueueFamilyProperties2*) p;
	extsProps[0] = (VkExtensionProperties*) (qfsProps[0] + qfTotal);

	for (uint32_t i = 1; i < deviceCount; i++) {
		qfsProps[i]  = qfsProps[i - 1] + qfCounts[i - 1];
		extsProps[i] = extsProps[i - 1] + extCounts[i - 1];
	}

	for (uint32_t i = 0; i < deviceCount; i++) {
		uint32_t qfCount = qfCounts[i];

		VK_CALL_RES(vkEnumerateDeviceExtensionProperties, devices[i], NULL, &extCounts[i], extsProps[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		for (uint32_t j = 0; j < qfCount; j++) {
			qfsProps[i][j] = (VkQueueFamilyProperties2) {VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2};
		}

		VK_CALL(vkGetPhysicalDeviceQueueFamilyProperties2, devices[i], &qfCounts[i], qfsProps[i]);

		devsMemoryProps[i] = (VkPhysicalDeviceMemoryProperties2) {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceMemoryProperties2, devices[i], &devsMemoryProps[i]);

		devsProps[i] = (VkPhysicalDeviceProperties2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};

		VK_CALL(vkGetPhysicalDeviceProperties2, devices[i], &devsProps[i]);

		devs8BitStorageFeats[i] = (VkPhysicalDevice8BitStorageFeatures) {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES};

		devs16BitStorageFeats[i] = (VkPhysicalDevice16BitStorageFeatures) {
			VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
		devs16BitStorageFeats[i].pNext = &devs8BitStorageFeats[i];

		devsFeats[i]       = (VkPhysicalDeviceFeatures2) {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
		devsFeats[i].pNext = &devs16BitStorageFeats[i];

		VK_CALL(vkGetPhysicalDeviceFeatures2, devices[i], &devsFeats[i]);
	}

	uint32_t devIndex  = UINT32_MAX;
	uint32_t bestScore = 0;

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
	bool usingVulkan14                     = false;

	for (uint32_t i = 0; i < deviceCount; i++) {
		uint32_t extCount        = extCounts[i];
		uint32_t qfCount         = qfCounts[i];
		uint32_t memoryTypeCount = devsMemoryProps[i].memoryProperties.memoryTypeCount;

		bool hasVulkan11 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_1;
		bool hasVulkan12 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_2;
		bool hasVulkan13 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_3;
		bool hasVulkan14 = devsProps[i].properties.apiVersion >= VK_API_VERSION_1_4;

		bool hasShaderInt16 = devsFeats[i].features.shaderInt16;
		bool hasShaderInt64 = devsFeats[i].features.shaderInt64;

		bool hasUniformAndStorageBuffer8BitAccess = devs8BitStorageFeats[i].uniformAndStorageBuffer8BitAccess;
		bool hasStorageBuffer16BitAccess          = devs16BitStorageFeats[i].storageBuffer16BitAccess;

		bool hasDedicatedTransfer = false;
		bool hasDedicatedCompute  = false;
		bool hasCompute           = false;

		for (uint32_t j = 0; j < qfCount; j++) {
			VkQueueFlags queueFlags = qfsProps[i][j].queueFamilyProperties.queueFlags;

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
			VkMemoryPropertyFlags propFlags = devsMemoryProps[i].memoryProperties.memoryTypes[j].propertyFlags;

			bool isDeviceLocal     = propFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			bool isHostVisible     = propFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
			bool isHostCoherent    = propFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			bool isHostCached      = propFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
			bool isLazilyAllocated = propFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
			bool isProtected       = propFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT;
			bool isDeviceCoherent  = propFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD;

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

		for (uint32_t j = 0; j < extCount; j++) {
			const char* extName = extsProps[i][j].extensionName;

			if      (!strcmp(extName, VK_KHR_8BIT_STORAGE_EXTENSION_NAME))          { has8BitStorage         = true; }
			else if (!strcmp(extName, VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME)) { hasBufferDeviceAddress = true; }
			else if (!strcmp(extName, VK_KHR_MAINTENANCE_4_EXTENSION_NAME))         { hasMaintenance4        = true; }
			else if (!strcmp(extName, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME)) {
				hasPipelineExecutableProperties = true; }
			else if (!strcmp(extName, VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME)) { hasPortabilitySubset = true; }
			else if (!strcmp(extName, VK_KHR_SPIRV_1_4_EXTENSION_NAME))          { hasSpirv14           = true; }
			else if (!strcmp(extName, VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME))  { hasSynchronization2  = true; }
			else if (!strcmp(extName, VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME)) { hasTimelineSemaphore = true; }
			else if (!strcmp(extName, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME))      { hasMemoryBudget      = true; }
			else if (!strcmp(extName, VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME))    { hasMemoryPriority    = true; }
			else if (!strcmp(extName, VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME)) {
				hasPipelineCreationCacheControl = true; }
			else if (!strcmp(extName, VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME)) { hasSubgroupSizeControl = true; }
		}

		uint32_t currScore = 1;

		if (!hasVulkan11) continue;

		if (!hasDeviceLocal) continue;
		if (!hasHostVisible) continue;

		if (!hasCompute) continue;

		if (!hasSynchronization2)  continue;
		if (!hasTimelineSemaphore) continue;

		if (hasVulkan12) { currScore += 50; }
		if (hasVulkan13) { currScore += 50; }
		if (hasVulkan14) { currScore += 50; }

		if (gpu->preferInt16 && hasShaderInt16) { currScore += 1000; }
		if (gpu->preferInt64 && hasShaderInt64) { currScore += 1000; }

		if (hasDeviceNonHost) { currScore += 50; }

		if      (hasHostCachedNonCoherent) { currScore += 1000; }
		else if (hasHostCached)            { currScore += 500;  }
		else if (hasHostNonCoherent)       { currScore += 100;  }

		if (hasDedicatedTransfer) { currScore += 100; }
		if (hasDedicatedCompute)  { currScore += 100; }

		if (hasMaintenance4)                 { currScore += 10;  }
		if (hasMemoryBudget)                 { currScore += 10;  }
		if (hasMemoryPriority)               { currScore += 10;  }
		if (hasPipelineCreationCacheControl) { currScore += 10;  }
		if (hasSpirv14)                      { currScore += 10;  }
		if (hasStorageBuffer16BitAccess)     { currScore += 100; }
		if (hasSubgroupSizeControl)          { currScore += 10;  }

		if (gpu->validationLayers && has8BitStorage && hasUniformAndStorageBuffer8BitAccess) { currScore += 10; }
		if (gpu->validationLayers && hasBufferDeviceAddress)                                 { currScore += 10; }
		if (gpu->capturePipelines && hasPipelineExecutableProperties)                        { currScore += 10; }

		if (currScore > bestScore) {
			bestScore = currScore;
			devIndex  = i;

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
			usingVulkan14                     = hasVulkan14;
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

	const char* deviceName = devsProps[devIndex].properties.deviceName;

	uint32_t qfCount = qfCounts[devIndex];

	uint32_t vkVerMajor = 1;
	uint32_t vkVerMinor;
	uint32_t spvVerMajor = 1;
	uint32_t spvVerMinor;

	if      (usingVulkan14) { vkVerMinor = 4; spvVerMinor = 6; }
	else if (usingVulkan13) { vkVerMinor = 3; spvVerMinor = 6; }
	else if (usingVulkan12) { vkVerMinor = 2; spvVerMinor = 5; }
	else if (usingSpirv14)  { vkVerMinor = 1; spvVerMinor = 4; }
	else                    { vkVerMinor = 1; spvVerMinor = 3; }

	uint32_t transferQfIndex = 0;
	uint32_t computeQfIndex  = 0;

	bool hasDedicatedTransfer = false;
	bool hasTransfer          = false;

	bool hasDedicatedCompute = false;
	bool hasCompute          = false;

	for (uint32_t i = 0; i < qfCount; i++) {
		VkQueueFlags queueFlags = qfsProps[devIndex][i].queueFamilyProperties.queueFlags;

		bool isDedicatedTransfer = queueFlags == VK_QUEUE_TRANSFER_BIT;
		bool isTransfer          = queueFlags & VK_QUEUE_TRANSFER_BIT;

		bool isDedicatedCompute = queueFlags == VK_QUEUE_COMPUTE_BIT;
		bool isCompute          = queueFlags & VK_QUEUE_COMPUTE_BIT;

		if (isTransfer) {
			if (isDedicatedTransfer && !hasDedicatedTransfer) {
				hasDedicatedTransfer = true;
				hasTransfer          = true;
				transferQfIndex      = i;
			}

			else if (!hasTransfer) {
				hasTransfer     = true;
				transferQfIndex = i;
			}
		}

		if (isCompute) {
			if (isDedicatedCompute && !hasDedicatedCompute) {
				hasDedicatedCompute = true;
				hasCompute          = true;
				computeQfIndex      = i;
			}

			else if (!hasCompute) {
				hasCompute     = true;
				computeQfIndex = i;
			}
		}
	}

	if (!hasTransfer) { transferQfIndex = computeQfIndex; }

	gpu->physicalDevice = devices[devIndex];

	gpu->transferQueueFamilyIndex = transferQfIndex;
	gpu->computeQueueFamilyIndex  = computeQfIndex;

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
		gpu->transferQueueFamilyTimestampValidBits =
			qfsProps[devIndex][transferQfIndex].queueFamilyProperties.timestampValidBits;
		gpu->computeQueueFamilyTimestampValidBits =
			qfsProps[devIndex][computeQfIndex].queueFamilyProperties.timestampValidBits;
		gpu->timestampPeriod = devsProps[devIndex].properties.limits.timestampPeriod;
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
				deviceName, vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor, transferQfIndex, computeQfIndex);
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
				deviceName, bestScore, vkVerMajor, vkVerMinor, spvVerMajor, spvVerMinor,
				transferQfIndex, computeQfIndex,
				usingBufferDeviceAddress, usingMaintenance4, usingMemoryPriority, usingPipelineCreationCacheControl,
				usingPipelineExecutableProperties, usingShaderInt16, usingShaderInt64, using16BitStorage,
				usingSubgroupSizeControl, using8BitStorage);
			break;
		default: break;
	}

	free_recursive(dyMem);

	return true;
}

bool create_device(Gpu* restrict gpu)
{
	VkPhysicalDevice physicalDevice = gpu->physicalDevice;

	uint32_t transferQfIndex = gpu->transferQueueFamilyIndex;
	uint32_t computeQfIndex  = gpu->computeQueueFamilyIndex;

	uint32_t spvVerMinor = gpu->spvVerMinor;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 1);
	if EXPECT_FALSE (!dyMem) return false;

	DyArray enabledExts = DyArray_create(sizeof(const char*), 13);
	if EXPECT_FALSE (!enabledExts) { free_recursive(dyMem); return false; }

	dyData = (DyData) {enabledExts, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	DyArray_append(enabledExts, &VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
	DyArray_append(enabledExts, &VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME);

	if (gpu->using8BitStorage)         { DyArray_append(enabledExts, &VK_KHR_8BIT_STORAGE_EXTENSION_NAME);          }
	if (gpu->usingBufferDeviceAddress) { DyArray_append(enabledExts, &VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME); }
	if (gpu->usingMaintenance4)        { DyArray_append(enabledExts, &VK_KHR_MAINTENANCE_4_EXTENSION_NAME);         }
	if (gpu->usingPortabilitySubset)   { DyArray_append(enabledExts, &VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME);    }
	if (gpu->usingMemoryBudget)        { DyArray_append(enabledExts, &VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);         }
	if (gpu->usingMemoryPriority)      { DyArray_append(enabledExts, &VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME);       }
	if (gpu->usingSubgroupSizeControl) { DyArray_append(enabledExts, &VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME); }

	if (gpu->usingPipelineExecutableProperties) {
		DyArray_append(enabledExts, &VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME); }
	if (gpu->usingPipelineCreationCacheControl) {
		DyArray_append(enabledExts, &VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME); }

	if (spvVerMinor >= 4) {
		DyArray_append(enabledExts, &VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME);
		DyArray_append(enabledExts, &VK_KHR_SPIRV_1_4_EXTENSION_NAME);
	}

	VkPhysicalDeviceFeatures generalFeats = {VK_FALSE};
	generalFeats.shaderInt64              = gpu->usingShaderInt64;
	generalFeats.shaderInt16              = gpu->usingShaderInt16;

	VkPhysicalDeviceFeatures2 devFeats = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};
	devFeats.features                  = generalFeats;

	VkPhysicalDevice16BitStorageFeatures dev16BitStorageFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_16BIT_STORAGE_FEATURES};
	dev16BitStorageFeats.storageBuffer16BitAccess = VK_TRUE;

	VkPhysicalDevice8BitStorageFeatures dev8BitStorageFeats = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_8BIT_STORAGE_FEATURES};
	dev8BitStorageFeats.storageBuffer8BitAccess             = VK_TRUE;
	dev8BitStorageFeats.uniformAndStorageBuffer8BitAccess   = VK_TRUE;

	VkPhysicalDeviceBufferDeviceAddressFeatures devBufferDeviceAddressFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES};
	devBufferDeviceAddressFeats.bufferDeviceAddress = VK_TRUE;

	VkPhysicalDeviceMaintenance4Features devMaintenance4Feats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_FEATURES};
	devMaintenance4Feats.maintenance4 = VK_TRUE;

	VkPhysicalDevicePipelineExecutablePropertiesFeaturesKHR devPipelineExecutablePropertiesFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_EXECUTABLE_PROPERTIES_FEATURES_KHR};
	devPipelineExecutablePropertiesFeats.pipelineExecutableInfo = VK_TRUE;

	VkPhysicalDeviceSynchronization2Features devSynchronization2Feats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES};
	devSynchronization2Feats.synchronization2 = VK_TRUE;

	VkPhysicalDeviceTimelineSemaphoreFeatures devTimelineSemaphoreFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TIMELINE_SEMAPHORE_FEATURES};
	devTimelineSemaphoreFeats.timelineSemaphore = VK_TRUE;

	VkPhysicalDeviceMemoryPriorityFeaturesEXT devMemoryPriorityFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PRIORITY_FEATURES_EXT};
	devMemoryPriorityFeats.memoryPriority = VK_TRUE;

	VkPhysicalDevicePipelineCreationCacheControlFeatures devPipelineCreationCacheControlFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PIPELINE_CREATION_CACHE_CONTROL_FEATURES};
	devPipelineCreationCacheControlFeats.pipelineCreationCacheControl = VK_TRUE;

	VkPhysicalDeviceSubgroupSizeControlFeatures devSubgroupSizeControlFeats = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SUBGROUP_SIZE_CONTROL_FEATURES};
	devSubgroupSizeControlFeats.subgroupSizeControl = VK_TRUE;

	void** next = &devFeats.pNext;

	PNEXT_ADD(next, devSynchronization2Feats);
	PNEXT_ADD(next, devTimelineSemaphoreFeats);

	if (gpu->using8BitStorage)                  { PNEXT_ADD(next, dev8BitStorageFeats);                  }
	if (gpu->using16BitStorage)                 { PNEXT_ADD(next, dev16BitStorageFeats);                 }
	if (gpu->usingBufferDeviceAddress)          { PNEXT_ADD(next, devBufferDeviceAddressFeats);          }
	if (gpu->usingMaintenance4)                 { PNEXT_ADD(next, devMaintenance4Feats);                 }
	if (gpu->usingMemoryPriority)               { PNEXT_ADD(next, devMemoryPriorityFeats);               }
	if (gpu->usingPipelineCreationCacheControl) { PNEXT_ADD(next, devPipelineCreationCacheControlFeats); }
	if (gpu->usingPipelineExecutableProperties) { PNEXT_ADD(next, devPipelineExecutablePropertiesFeats); }
	if (gpu->usingSubgroupSizeControl)          { PNEXT_ADD(next, devSubgroupSizeControlFeats);          }

	float computeQueuePriority  = 1;
	float transferQueuePriority = 0;

	VkDeviceQueueCreateInfo queueCreateInfos[2];
	queueCreateInfos[0]                  = (VkDeviceQueueCreateInfo) {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfos[0].queueFamilyIndex = computeQfIndex;
	queueCreateInfos[0].queueCount       = 1;
	queueCreateInfos[0].pQueuePriorities = &computeQueuePriority;

	queueCreateInfos[1]                  = (VkDeviceQueueCreateInfo) {VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
	queueCreateInfos[1].queueFamilyIndex = transferQfIndex;
	queueCreateInfos[1].queueCount       = 1;
	queueCreateInfos[1].pQueuePriorities = &transferQueuePriority;

	uint32_t     enabledExtCount = (uint32_t) DyArray_size(enabledExts);
	const char** enabledExtNames = (const char**) DyArray_raw(enabledExts);

	VkDeviceCreateInfo deviceCreateInfo      = {VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
	deviceCreateInfo.pNext                   = &devFeats;
	deviceCreateInfo.queueCreateInfoCount    = computeQfIndex == transferQfIndex ? 1 : 2;
	deviceCreateInfo.pQueueCreateInfos       = queueCreateInfos;
	deviceCreateInfo.enabledExtensionCount   = enabledExtCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledExtNames;

	if (gpu->outputLevel > CLI_OUTPUT_DEFAULT) {
		printf("Enabled device extensions (%" PRIu32 "):\n", enabledExtCount);
		for (uint32_t i = 0; i < enabledExtCount; i++) {
			printf("\t%" PRIu32 ") %s\n", i + 1, enabledExtNames[i]);
		}
		NEWLINE();
	}

	VK_CALL_RES(vkCreateDevice, physicalDevice, &deviceCreateInfo, g_allocator, &gpu->device);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkDevice device = gpu->device;

	free_recursive(dyMem);

	volkLoadDevice(device);

	VkDeviceQueueInfo2 transferQueueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
	transferQueueInfo.queueFamilyIndex   = transferQfIndex;
	transferQueueInfo.queueIndex         = 0;

	VkDeviceQueueInfo2 computeQueueInfo = {VK_STRUCTURE_TYPE_DEVICE_QUEUE_INFO_2};
	computeQueueInfo.queueFamilyIndex   = computeQfIndex;
	computeQueueInfo.queueIndex         = 0;

	VK_CALL(vkGetDeviceQueue2, device, &transferQueueInfo, &gpu->transferQueue);
	VK_CALL(vkGetDeviceQueue2, device, &computeQueueInfo,  &gpu->computeQueue);

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

	VkPhysicalDeviceMaintenance4Properties devMaintenance4Props = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_4_PROPERTIES};

	VkPhysicalDeviceMaintenance3Properties devMaintenance3Props = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MAINTENANCE_3_PROPERTIES};
	devMaintenance3Props.pNext = gpu->usingMaintenance4 ? &devMaintenance4Props : NULL;

	VkPhysicalDeviceProperties2 devProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
	devProps.pNext                       = &devMaintenance3Props;

	VK_CALL(vkGetPhysicalDeviceProperties2, physicalDevice, &devProps);

	VkPhysicalDeviceMemoryBudgetPropertiesEXT devMemoryBudgetProps = {
		VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_BUDGET_PROPERTIES_EXT};

	VkPhysicalDeviceMemoryProperties2 devMemoryProps = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MEMORY_PROPERTIES_2};
	devMemoryProps.pNext                             = gpu->usingMemoryBudget ? &devMemoryBudgetProps : NULL;

	VK_CALL(vkGetPhysicalDeviceMemoryProperties2, physicalDevice, &devMemoryProps);

	VkDeviceSize maxMemorySize = devMaintenance3Props.maxMemoryAllocationSize;
	VkDeviceSize maxBufferSize = gpu->usingMaintenance4 ? devMaintenance4Props.maxBufferSize : maxMemorySize;

	uint32_t maxStorageBufferRange = devProps.properties.limits.maxStorageBufferRange;
	uint32_t maxMemoryCount        = devProps.properties.limits.maxMemoryAllocationCount;
	uint32_t maxWorkgroupCount     = devProps.properties.limits.maxComputeWorkGroupCount[0];
	uint32_t maxWorkgroupSize      = devProps.properties.limits.maxComputeWorkGroupSize[0];
	uint32_t memoryTypeCount       = devMemoryProps.memoryProperties.memoryTypeCount;

	VkMemoryRequirements dlRequirements; // dl = device local
	VkMemoryRequirements hvRequirements; // hv = host visible

	bool bres = get_buffer_requirements(
		device, sizeof(char), VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &hvRequirements);

	if EXPECT_FALSE (!bres) return false;

	bres = get_buffer_requirements(
		device, sizeof(char),
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		&dlRequirements);

	if EXPECT_FALSE (!bres) return false;

	uint32_t dlMemoryTypeBits = dlRequirements.memoryTypeBits;
	uint32_t hvMemoryTypeBits = hvRequirements.memoryTypeBits;

	uint32_t dlHeapIndex = 0;
	uint32_t hvHeapIndex = 0;
	uint32_t dlTypeIndex = 0;
	uint32_t hvTypeIndex = 0;

	bool hasDeviceNonHost = false;
	bool hasDeviceLocal   = false;

	bool hasHostCachedNonCoherent = false;
	bool hasHostCached            = false;
	bool hasHostNonCoherent       = false;
	bool hasHostVisible           = false;

	for (uint32_t i = 0; i < memoryTypeCount; i++) {
		uint32_t memoryTypeBit = 1U << i;

		VkMemoryPropertyFlags propFlags = devMemoryProps.memoryProperties.memoryTypes[i].propertyFlags;
		uint32_t              heapIndex = devMemoryProps.memoryProperties.memoryTypes[i].heapIndex;

		bool isDeviceLocal    = propFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		bool isHostVisible    = propFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
		bool isHostCoherent   = propFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		bool isHostCached     = propFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
		bool isProtected      = propFlags & VK_MEMORY_PROPERTY_PROTECTED_BIT;
		bool isDeviceCoherent = propFlags & VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD;

		if (isProtected)      continue;
		if (isDeviceCoherent) continue;

		if (isDeviceLocal && (dlMemoryTypeBits & memoryTypeBit)) {
			if (!isHostVisible && !hasDeviceNonHost) {
				hasDeviceNonHost = true;
				hasDeviceLocal   = true;
				dlHeapIndex      = heapIndex;
				dlTypeIndex      = i;
			}
			else if (!hasDeviceLocal) {
				hasDeviceLocal = true;
				dlHeapIndex    = heapIndex;
				dlTypeIndex    = i;
			}
		}

		if (isHostVisible && (hvMemoryTypeBits & memoryTypeBit)) {
			if (isHostCached && !isHostCoherent && !hasHostCachedNonCoherent) {
				hasHostCachedNonCoherent = true;
				hasHostCached            = true;
				hasHostNonCoherent       = true;
				hasHostVisible           = true;
				hvHeapIndex              = heapIndex;
				hvTypeIndex              = i;
			}
			else if (isHostCached && !hasHostCached) {
				hasHostCached        = true;
				hasHostNonCoherent   = false;
				hasHostVisible       = true;
				hvHeapIndex          = heapIndex;
				hvTypeIndex          = i;
			}
			else if (!isHostCoherent && !hasHostCached && !hasHostNonCoherent) {
				hasHostNonCoherent   = true;
				hasHostVisible       = true;
				hvHeapIndex          = heapIndex;
				hvTypeIndex          = i;
			}
			else if (!hasHostVisible) {
				hasHostVisible       = true;
				hvHeapIndex          = heapIndex;
				hvTypeIndex          = i;
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
	bytesPerHeap              = (VkDeviceSize) ((float) bytesPerHeap * gpu->maxMemory);

	if (dlHeapIndex == hvHeapIndex) { bytesPerHeap /= 2; }

	VkDeviceSize bytesPerBuffer = minu64v(3, maxMemorySize, maxBufferSize, bytesPerHeap);
	uint32_t     buffersPerHeap = (uint32_t) (bytesPerHeap / bytesPerBuffer);

	if (buffersPerHeap < maxMemoryCount && bytesPerHeap % bytesPerBuffer) {
		VkDeviceSize excessBytes = bytesPerBuffer - bytesPerHeap % bytesPerBuffer;

		buffersPerHeap++;
		bytesPerBuffer -= excessBytes / buffersPerHeap;

		if (excessBytes % buffersPerHeap) { bytesPerBuffer--; }
	}
	else if (buffersPerHeap > maxMemoryCount) { buffersPerHeap = maxMemoryCount; }

	uint32_t workgroupSize  = floor_pow2(maxWorkgroupSize);
	uint32_t workgroupCount = minu32(
		maxWorkgroupCount, (uint32_t) (maxStorageBufferRange / (workgroupSize * sizeof(Value))));

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
		device, bytesPerBuffer, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, &hvRequirements);

	if EXPECT_FALSE (!bres) return false;

	bres = get_buffer_requirements(
		device, bytesPerBuffer,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		&dlRequirements);

	if EXPECT_FALSE (!bres) return false;

	VkDeviceSize bytesPerHvMemory = hvRequirements.size;
	VkDeviceSize bytesPerDlMemory = dlRequirements.size;

	/*
		maxWorkgroupSize is guaranteed to be at least 128
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
	gpu->bytesPerHostVisibleMemory = bytesPerHvMemory;
	gpu->bytesPerDeviceLocalMemory = bytesPerDlMemory;

	gpu->valuesPerInout  = valuesPerInout;
	gpu->valuesPerBuffer = valuesPerBuffer;
	gpu->valuesPerHeap   = valuesPerHeap;
	gpu->inoutsPerBuffer = inoutsPerBuffer;
	gpu->inoutsPerHeap   = inoutsPerHeap;
	gpu->buffersPerHeap  = buffersPerHeap;

	gpu->workgroupSize  = workgroupSize;
	gpu->workgroupCount = workgroupCount;

	gpu->hostVisibleHeapIndex = hvHeapIndex;
	gpu->deviceLocalHeapIndex = dlHeapIndex;

	gpu->hostVisibleTypeIndex = hvTypeIndex;
	gpu->deviceLocalTypeIndex = dlTypeIndex;

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
				hvTypeIndex, dlTypeIndex, workgroupSize, workgroupCount, valuesPerInout, inoutsPerHeap);
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
				hasHostNonCoherent, hvHeapIndex, dlHeapIndex, hvTypeIndex, dlTypeIndex,
				workgroupSize, workgroupCount, valuesPerInout, inoutsPerBuffer, buffersPerHeap, valuesPerHeap);
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
	VkBuffer*       hvBuffers        = gpu->hostVisibleBuffers;
	VkBuffer*       dlBuffers        = gpu->deviceLocalBuffers;
	VkDeviceMemory* hvMemories       = gpu->hostVisibleDeviceMemories;
	VkDeviceMemory* dlMemories       = gpu->deviceLocalDeviceMemories;
	Value**         mappedInBuffers  = gpu->mappedInBuffers;
	Count**         mappedOutBuffers = gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkDeviceSize bytesPerBuffer   = gpu->bytesPerBuffer;
	VkDeviceSize bytesPerHvMemory = gpu->bytesPerHostVisibleMemory;
	VkDeviceSize bytesPerDlMemory = gpu->bytesPerDeviceLocalMemory;

	uint32_t valuesPerInout  = gpu->valuesPerInout;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t buffersPerHeap  = gpu->buffersPerHeap;
	uint32_t hvTypeIndex     = gpu->hostVisibleTypeIndex;
	uint32_t dlTypeIndex     = gpu->deviceLocalTypeIndex;

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

	VkMemoryAllocateInfo* hvAllocateInfos = (VkMemoryAllocateInfo*) p;
	VkMemoryAllocateInfo* dlAllocateInfos = (VkMemoryAllocateInfo*) (hvAllocateInfos + buffersPerHeap);

	VkMemoryDedicatedAllocateInfo* hvDedicatedInfos = (VkMemoryDedicatedAllocateInfo*) (
		dlAllocateInfos + buffersPerHeap);
	VkMemoryDedicatedAllocateInfo* dlDedicatedInfos = (VkMemoryDedicatedAllocateInfo*) (
		hvDedicatedInfos + buffersPerHeap);

	VkBindBufferMemoryInfo (*bindBufferMemoryInfos)[2] = (VkBindBufferMemoryInfo(*)[]) (
		dlDedicatedInfos + buffersPerHeap);

	VkBufferCreateInfo hvBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	hvBufferCreateInfo.size               = bytesPerBuffer;
	hvBufferCreateInfo.usage              = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	hvBufferCreateInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

	VkBufferCreateInfo dlBufferCreateInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
	dlBufferCreateInfo.size               = bytesPerBuffer;
	dlBufferCreateInfo.usage =
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	dlBufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkCreateBuffer, device, &hvBufferCreateInfo, g_allocator, &hvBuffers[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		VK_CALL_RES(vkCreateBuffer, device, &dlBufferCreateInfo, g_allocator, &dlBuffers[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	VkMemoryPriorityAllocateInfoEXT hvPriorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
	hvPriorityInfo.priority                        = 0;

	VkMemoryPriorityAllocateInfoEXT dlPriorityInfo = {VK_STRUCTURE_TYPE_MEMORY_PRIORITY_ALLOCATE_INFO_EXT};
	dlPriorityInfo.priority                        = 1;

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		hvDedicatedInfos[i]        = (VkMemoryDedicatedAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		hvDedicatedInfos[i].pNext  = gpu->usingMemoryPriority ? &hvPriorityInfo : NULL;
		hvDedicatedInfos[i].buffer = hvBuffers[i];

		dlDedicatedInfos[i]        = (VkMemoryDedicatedAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO};
		dlDedicatedInfos[i].pNext  = gpu->usingMemoryPriority ? &dlPriorityInfo : NULL;
		dlDedicatedInfos[i].buffer = dlBuffers[i];

		hvAllocateInfos[i]                 = (VkMemoryAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		hvAllocateInfos[i].pNext           = &hvDedicatedInfos[i];
		hvAllocateInfos[i].allocationSize  = bytesPerHvMemory;
		hvAllocateInfos[i].memoryTypeIndex = hvTypeIndex;

		dlAllocateInfos[i]                 = (VkMemoryAllocateInfo) {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
		dlAllocateInfos[i].pNext           = &dlDedicatedInfos[i];
		dlAllocateInfos[i].allocationSize  = bytesPerDlMemory;
		dlAllocateInfos[i].memoryTypeIndex = dlTypeIndex;
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkAllocateMemory, device, &hvAllocateInfos[i], g_allocator, &hvMemories[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		VK_CALL_RES(vkAllocateMemory, device, &dlAllocateInfos[i], g_allocator, &dlMemories[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	for (uint32_t i = 0; i < buffersPerHeap; i++) {
		bindBufferMemoryInfos[i][0]        = (VkBindBufferMemoryInfo) {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
		bindBufferMemoryInfos[i][0].buffer = hvBuffers[i];
		bindBufferMemoryInfos[i][0].memory = hvMemories[i];

		bindBufferMemoryInfos[i][1]        = (VkBindBufferMemoryInfo) {VK_STRUCTURE_TYPE_BIND_BUFFER_MEMORY_INFO};
		bindBufferMemoryInfos[i][1].buffer = dlBuffers[i];
		bindBufferMemoryInfos[i][1].memory = dlMemories[i];
	}

	VK_CALL_RES(vkBindBufferMemory2, device, buffersPerHeap * 2, (VkBindBufferMemoryInfo*) bindBufferMemoryInfos);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		VK_CALL_RES(vkMapMemory, device, hvMemories[i], 0, bytesPerHvMemory, 0, (void**) &mappedInBuffers[j]);
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

			set_debug_name(device, VK_OBJECT_TYPE_BUFFER,        (uint64_t) hvBuffers[i],  objectName);
			set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) hvMemories[i], objectName);

			strcpy(objectName, "Device local");
			objectName[12] = ' '; // Remove '\0' from strcpy

			set_debug_name(device, VK_OBJECT_TYPE_BUFFER,        (uint64_t) dlBuffers[i],  objectName);
			set_debug_name(device, VK_OBJECT_TYPE_DEVICE_MEMORY, (uint64_t) dlMemories[i], objectName);
		}
	}
#endif

	return true;
}

bool create_descriptors(Gpu* restrict gpu)
{
	const VkBuffer* dlBuffers = gpu->deviceLocalBuffers;

	VkDescriptorSet* descSets = gpu->descriptorSets;

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

	VkDescriptorSetLayout* descSetLayouts = (VkDescriptorSetLayout*) p;
	VkWriteDescriptorSet*  writeDescSets  = (VkWriteDescriptorSet*) (descSetLayouts + inoutsPerHeap);

	VkDescriptorBufferInfo (*descBufferInfos)[2] = (VkDescriptorBufferInfo(*)[]) (writeDescSets + inoutsPerHeap);

	VkDescriptorSetLayoutBinding descSetLayoutBindings[2];
	descSetLayoutBindings[0].binding            = 0;
	descSetLayoutBindings[0].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descSetLayoutBindings[0].descriptorCount    = 1;
	descSetLayoutBindings[0].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	descSetLayoutBindings[0].pImmutableSamplers = NULL;

	descSetLayoutBindings[1].binding            = 1;
	descSetLayoutBindings[1].descriptorType     = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descSetLayoutBindings[1].descriptorCount    = 1;
	descSetLayoutBindings[1].stageFlags         = VK_SHADER_STAGE_COMPUTE_BIT;
	descSetLayoutBindings[1].pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descSetLayoutCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
	descSetLayoutCreateInfo.bindingCount                    = ARR_SIZE(descSetLayoutBindings);
	descSetLayoutCreateInfo.pBindings                       = descSetLayoutBindings;

	VkDescriptorPoolSize descPoolSizes[1];
	descPoolSizes[0].type            = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descPoolSizes[0].descriptorCount = inoutsPerHeap * 2;

	VkDescriptorPoolCreateInfo descPoolCreateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
	descPoolCreateInfo.maxSets                    = inoutsPerHeap;
	descPoolCreateInfo.poolSizeCount              = ARR_SIZE(descPoolSizes);
	descPoolCreateInfo.pPoolSizes                 = descPoolSizes;

	VK_CALL_RES(vkCreateDescriptorSetLayout, device, &descSetLayoutCreateInfo, g_allocator, &gpu->descriptorSetLayout);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreateDescriptorPool, device, &descPoolCreateInfo, g_allocator, &gpu->descriptorPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkDescriptorSetLayout descSetLayout = gpu->descriptorSetLayout;
	VkDescriptorPool      descPool      = gpu->descriptorPool;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		descSetLayouts[i] = descSetLayout;
	}

	VkDescriptorSetAllocateInfo descSetAllocateInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
	descSetAllocateInfo.descriptorPool              = descPool;
	descSetAllocateInfo.descriptorSetCount          = inoutsPerHeap;
	descSetAllocateInfo.pSetLayouts                 = descSetLayouts;

	VK_CALL_RES(vkAllocateDescriptorSets, device, &descSetAllocateInfo, descSets);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			descBufferInfos[j][0].buffer = dlBuffers[i];
			descBufferInfos[j][0].offset = bytesPerInout * k;
			descBufferInfos[j][0].range  = bytesPerIn;

			descBufferInfos[j][1].buffer = dlBuffers[i];
			descBufferInfos[j][1].offset = bytesPerInout * k + bytesPerIn;
			descBufferInfos[j][1].range  = bytesPerOut;

			writeDescSets[j]                 = (VkWriteDescriptorSet) {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
			writeDescSets[j].dstSet          = descSets[j];
			writeDescSets[j].dstBinding      = 0;
			writeDescSets[j].dstArrayElement = 0;
			writeDescSets[j].descriptorCount = ARR_SIZE(descBufferInfos[j]);
			writeDescSets[j].descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			writeDescSets[j].pBufferInfo     = descBufferInfos[j];
		}
	}

	VK_CALL(vkUpdateDescriptorSets, device, inoutsPerHeap, writeDescSets, 0, NULL);

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

				set_debug_name(device, VK_OBJECT_TYPE_DESCRIPTOR_SET, (uint64_t) descSets[j], objectName);
			}
		}
	}
#endif

	return true;
}

bool create_pipeline(Gpu* restrict gpu)
{
	VkDevice device = gpu->device;

	VkDescriptorSetLayout descSetLayout = gpu->descriptorSetLayout;

	uint32_t inoutsPerHeap = gpu->inoutsPerHeap;
	uint32_t workgroupSize = gpu->workgroupSize;

	uint32_t transferQfTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;
	uint32_t computeQfTimestampValidBits  = gpu->computeQueueFamilyTimestampValidBits;

	uint32_t spvVerMajor = gpu->spvVerMajor;
	uint32_t spvVerMinor = gpu->spvVerMinor;
	uint32_t iterSize    = gpu->iterSize;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 2);
	if EXPECT_FALSE (!dyMem) return false;

	char shaderName[52];
	char entryPointName[27];

	sprintf(
		shaderName,
		"./v%" PRIu32 "%" PRIu32 "/spirv%s%s%s.spv",
		spvVerMajor, spvVerMinor,
		gpu->using16BitStorage ? "-sto16" : "",
		gpu->usingShaderInt16  ? "-int16" : "",
		gpu->usingShaderInt64  ? "-int64" : "");

	sprintf(entryPointName, "main-%" PRIu32 "-%" PRIu32, get_endianness(), iterSize);

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

	VkShaderModuleCreateInfo moduleCreateInfo = {VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};
	moduleCreateInfo.codeSize                 = shaderSize;
	moduleCreateInfo.pCode                    = shaderCode;

	VkPipelineCacheCreateInfo cacheCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO};
	cacheCreateInfo.flags =
		gpu->usingPipelineCreationCacheControl ? VK_PIPELINE_CACHE_CREATE_EXTERNALLY_SYNCHRONIZED_BIT : 0;
	cacheCreateInfo.initialDataSize = cacheSize;
	cacheCreateInfo.pInitialData    = cacheData;

	VkPipelineLayoutCreateInfo layoutCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
	layoutCreateInfo.setLayoutCount             = 1;
	layoutCreateInfo.pSetLayouts                = &descSetLayout;

	VK_CALL_RES(vkCreateShaderModule, device, &moduleCreateInfo, g_allocator, &gpu->shaderModule);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreatePipelineCache, device, &cacheCreateInfo, g_allocator, &gpu->pipelineCache);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreatePipelineLayout, device, &layoutCreateInfo, g_allocator, &gpu->pipelineLayout);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkShaderModule   shaderModule   = gpu->shaderModule;
	VkPipelineCache  pipelineCache  = gpu->pipelineCache;
	VkPipelineLayout pipelineLayout = gpu->pipelineLayout;

	VK_CALL(vkDestroyDescriptorSetLayout, device, descSetLayout, g_allocator);
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

	VkPipelineShaderStageCreateInfo stageCreateInfo = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	stageCreateInfo.flags =
		gpu->usingSubgroupSizeControl ? VK_PIPELINE_SHADER_STAGE_CREATE_ALLOW_VARYING_SUBGROUP_SIZE_BIT_EXT : 0;
	stageCreateInfo.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
	stageCreateInfo.module              = shaderModule;
	stageCreateInfo.pName               = entryPointName;
	stageCreateInfo.pSpecializationInfo = &specialisationInfo;

	VkComputePipelineCreateInfo pipelineCreateInfo = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	pipelineCreateInfo.flags =
		gpu->usingPipelineExecutableProperties ?
		VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR | VK_PIPELINE_CREATE_CAPTURE_INTERNAL_REPRESENTATIONS_BIT_KHR :
		0;
	pipelineCreateInfo.stage  = stageCreateInfo;
	pipelineCreateInfo.layout = pipelineLayout;

	VK_CALL_RES(vkCreateComputePipelines, device, pipelineCache, 1, &pipelineCreateInfo, g_allocator, &gpu->pipeline);
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

	if (transferQfTimestampValidBits || computeQfTimestampValidBits) {
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

	VkPipelineExecutablePropertiesKHR pipelineExecutableProps = {VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR};

	VK_CALL_RES(
		vkGetPipelineExecutablePropertiesKHR, device, &pipelineInfo, &executableCount, &pipelineExecutableProps);

	if EXPECT_FALSE (vkres || executableCount != 1) { free_recursive(dyMem); return false; }

	printf(
		"Pipeline Executable Properties:\n"
		"\tStage         = %s\n"
		"\tName          = %s\n"
		"\tDescription   = %s\n"
		"\tSubgroup Size = %" PRIu32 "\n",
		string_VkShaderStageFlagBits((VkShaderStageFlagBits) pipelineExecutableProps.stages),
		pipelineExecutableProps.name, pipelineExecutableProps.description, pipelineExecutableProps.subgroupSize);

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

static bool record_transfer_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	VkBuffer hvBuffer,
	VkBuffer dlBuffer,
	const VkBufferCopy* restrict inBufferRegion,
	const VkBufferCopy* restrict outBufferRegion,
	const VkDependencyInfo* restrict depInfos,
	VkQueryPool queryPool,
	uint32_t firstQuery,
	uint32_t timestampValidBits)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VK_CALL_RES(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if EXPECT_FALSE (vkres) return false;

	if (timestampValidBits) {
		VK_CALL(vkCmdResetQueryPool, cmdBuffer, queryPool, firstQuery, 2);
		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, VK_PIPELINE_STAGE_2_NONE, queryPool, firstQuery);
	}

	VK_CALL(vkCmdCopyBuffer, cmdBuffer, hvBuffer, dlBuffer, 1, inBufferRegion);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[0]);
	
	VK_CALL(vkCmdCopyBuffer, cmdBuffer, dlBuffer, hvBuffer, 1, outBufferRegion);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[1]);

	if (timestampValidBits) {
		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, VK_PIPELINE_STAGE_2_COPY_BIT, queryPool, firstQuery + 1); }

	VK_CALL_RES(vkEndCommandBuffer, cmdBuffer);
	if EXPECT_FALSE (vkres) return false;

	return true;
}

static bool record_compute_cmdbuffer(
	VkCommandBuffer cmdBuffer,
	VkPipeline pipeline,
	VkPipelineLayout layout,
	const VkDescriptorSet* restrict descSet,
	const VkDependencyInfo* restrict depInfos,
	VkQueryPool queryPool,
	uint32_t firstQuery,
	uint32_t timestampValidBits,
	uint32_t workgroupCount)
{
	VkResult vkres;

	VkCommandBufferBeginInfo beginInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};

	VK_CALL_RES(vkBeginCommandBuffer, cmdBuffer, &beginInfo);
	if EXPECT_FALSE (vkres) return false;

	if (timestampValidBits) {
		VK_CALL(vkCmdResetQueryPool, cmdBuffer, queryPool, firstQuery, 2);
		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, VK_PIPELINE_STAGE_2_NONE, queryPool, firstQuery);
	}

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[0]);

	VK_CALL(vkCmdBindDescriptorSets, cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, layout, 0, 1, descSet, 0, NULL);

	VK_CALL(vkCmdBindPipeline, cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);

	VK_CALL(vkCmdDispatchBase, cmdBuffer, 0, 0, 0, workgroupCount, 1, 1);

	VK_CALL(vkCmdPipelineBarrier2KHR, cmdBuffer, &depInfos[1]);

	if (timestampValidBits) {
		VK_CALL(vkCmdWriteTimestamp2KHR, cmdBuffer, VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT, queryPool, firstQuery + 1);
	}

	VK_CALL_RES(vkEndCommandBuffer, cmdBuffer);
	if EXPECT_FALSE (vkres) return false;

	return true;
}

bool create_commands(Gpu* restrict gpu)
{
	const VkBuffer*        hvBuffers = gpu->hostVisibleBuffers;
	const VkBuffer*        dlBuffers = gpu->deviceLocalBuffers;
	const VkDescriptorSet* descSets  = gpu->descriptorSets;

	VkCommandBuffer* transferCmdBuffers = gpu->transferCommandBuffers;
	VkCommandBuffer* computeCmdBuffers  = gpu->computeCommandBuffers;
	VkSemaphore*     semaphores         = gpu->semaphores;

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

	uint32_t transferQfIndex              = gpu->transferQueueFamilyIndex;
	uint32_t computeQfIndex               = gpu->computeQueueFamilyIndex;
	uint32_t transferQfTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;
	uint32_t computeQfTimestampValidBits  = gpu->computeQueueFamilyTimestampValidBits;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 1);
	if EXPECT_FALSE (!dyMem) return false;

	size_t size =
		inoutsPerBuffer * sizeof(VkBufferCopy) * 2 +
		inoutsPerHeap   * sizeof(VkBufferMemoryBarrier2) * 5 +
		inoutsPerHeap   * sizeof(VkDependencyInfo) * 4;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	VkBufferCopy* inBufferCopies  = (VkBufferCopy*) p;
	VkBufferCopy* outBufferCopies = (VkBufferCopy*) (inBufferCopies + inoutsPerBuffer);

	VkBufferMemoryBarrier2 (*transferBufferMemoryBarriers)[3] = (VkBufferMemoryBarrier2(*)[]) (
		outBufferCopies + inoutsPerBuffer);
	VkBufferMemoryBarrier2 (*computeBufferMemoryBarriers)[2] = (VkBufferMemoryBarrier2(*)[]) (
		transferBufferMemoryBarriers + inoutsPerHeap);

	VkDependencyInfo (*transferDependencyInfos)[2] = (VkDependencyInfo(*)[]) (
		computeBufferMemoryBarriers + inoutsPerHeap);
	VkDependencyInfo (*computeDependencyInfos)[2] = (VkDependencyInfo(*)[]) (transferDependencyInfos + inoutsPerHeap);

	VkCommandPoolCreateInfo transferCmdPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	transferCmdPoolCreateInfo.queueFamilyIndex        = transferQfIndex;

	VkCommandPoolCreateInfo computeCmdPoolCreateInfo = {VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO};
	computeCmdPoolCreateInfo.queueFamilyIndex        = computeQfIndex;

	VK_CALL_RES(vkCreateCommandPool, device, &transferCmdPoolCreateInfo, g_allocator, &gpu->transferCommandPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkCreateCommandPool, device, &computeCmdPoolCreateInfo, g_allocator, &gpu->computeCommandPool);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VkCommandPool transferCmdPool = gpu->transferCommandPool;
	VkCommandPool computeCmdPool  = gpu->computeCommandPool;

	VkCommandBufferAllocateInfo transferCmdBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	transferCmdBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	transferCmdBufferAllocateInfo.commandPool                 = transferCmdPool;
	transferCmdBufferAllocateInfo.commandBufferCount          = inoutsPerHeap;

	VkCommandBufferAllocateInfo computeCmdBufferAllocateInfo = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO};
	computeCmdBufferAllocateInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeCmdBufferAllocateInfo.commandPool                 = computeCmdPool;
	computeCmdBufferAllocateInfo.commandBufferCount          = inoutsPerHeap;

	VK_CALL_RES(vkAllocateCommandBuffers, device, &transferCmdBufferAllocateInfo, transferCmdBuffers);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkAllocateCommandBuffers, device, &computeCmdBufferAllocateInfo, computeCmdBuffers);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

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
			transferBufferMemoryBarriers[j][0] = (VkBufferMemoryBarrier2) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
			transferBufferMemoryBarriers[j][0].srcStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][0].srcAccessMask       = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferQfIndex;
			transferBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeQfIndex;
			transferBufferMemoryBarriers[j][0].buffer              = dlBuffers[i];
			transferBufferMemoryBarriers[j][0].offset              = bytesPerInout * k;
			transferBufferMemoryBarriers[j][0].size                = bytesPerIn;

			transferBufferMemoryBarriers[j][1] = (VkBufferMemoryBarrier2) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
			transferBufferMemoryBarriers[j][1].dstStageMask        = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][1].dstAccessMask       = VK_ACCESS_2_TRANSFER_READ_BIT;
			transferBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeQfIndex;
			transferBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferQfIndex;
			transferBufferMemoryBarriers[j][1].buffer              = dlBuffers[i];
			transferBufferMemoryBarriers[j][1].offset              = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers[j][1].size                = bytesPerOut;

			transferBufferMemoryBarriers[j][2] = (VkBufferMemoryBarrier2) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
			transferBufferMemoryBarriers[j][2].srcStageMask  = VK_PIPELINE_STAGE_2_COPY_BIT;
			transferBufferMemoryBarriers[j][2].srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
			transferBufferMemoryBarriers[j][2].dstStageMask  = VK_PIPELINE_STAGE_2_HOST_BIT;
			transferBufferMemoryBarriers[j][2].dstAccessMask = VK_ACCESS_2_HOST_READ_BIT;
			transferBufferMemoryBarriers[j][2].buffer        = hvBuffers[i];
			transferBufferMemoryBarriers[j][2].offset        = bytesPerInout * k + bytesPerIn;
			transferBufferMemoryBarriers[j][2].size          = bytesPerOut;

			computeBufferMemoryBarriers[j][0] = (VkBufferMemoryBarrier2) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
			computeBufferMemoryBarriers[j][0].dstStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][0].dstAccessMask       = VK_ACCESS_2_SHADER_STORAGE_READ_BIT;
			computeBufferMemoryBarriers[j][0].srcQueueFamilyIndex = transferQfIndex;
			computeBufferMemoryBarriers[j][0].dstQueueFamilyIndex = computeQfIndex;
			computeBufferMemoryBarriers[j][0].buffer              = dlBuffers[i];
			computeBufferMemoryBarriers[j][0].offset              = bytesPerInout * k;
			computeBufferMemoryBarriers[j][0].size                = bytesPerIn;

			computeBufferMemoryBarriers[j][1] = (VkBufferMemoryBarrier2) {VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2};
			computeBufferMemoryBarriers[j][1].srcStageMask        = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
			computeBufferMemoryBarriers[j][1].srcAccessMask       = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
			computeBufferMemoryBarriers[j][1].srcQueueFamilyIndex = computeQfIndex;
			computeBufferMemoryBarriers[j][1].dstQueueFamilyIndex = transferQfIndex;
			computeBufferMemoryBarriers[j][1].buffer              = dlBuffers[i];
			computeBufferMemoryBarriers[j][1].offset              = bytesPerInout * k + bytesPerIn;
			computeBufferMemoryBarriers[j][1].size                = bytesPerOut;

			transferDependencyInfos[j][0] = (VkDependencyInfo) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			transferDependencyInfos[j][0].bufferMemoryBarrierCount = 2;
			transferDependencyInfos[j][0].pBufferMemoryBarriers    = &transferBufferMemoryBarriers[j][0];

			transferDependencyInfos[j][1] = (VkDependencyInfo) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			transferDependencyInfos[j][1].bufferMemoryBarrierCount = 1;
			transferDependencyInfos[j][1].pBufferMemoryBarriers    = &transferBufferMemoryBarriers[j][2];

			computeDependencyInfos[j][0] = (VkDependencyInfo) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			computeDependencyInfos[j][0].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[j][0].pBufferMemoryBarriers    = &computeBufferMemoryBarriers[j][0];

			computeDependencyInfos[j][1] = (VkDependencyInfo) {VK_STRUCTURE_TYPE_DEPENDENCY_INFO};
			computeDependencyInfos[j][1].bufferMemoryBarrierCount = 1;
			computeDependencyInfos[j][1].pBufferMemoryBarriers    = &computeBufferMemoryBarriers[j][1];
		}
	}

	for (uint32_t i = 0, j = 0; i < buffersPerHeap; i++) {
		for (uint32_t k = 0; k < inoutsPerBuffer; j++, k++) {
			bool bres = record_transfer_cmdbuffer(
				transferCmdBuffers[j], hvBuffers[i], dlBuffers[i], &inBufferCopies[k], &outBufferCopies[k],
				transferDependencyInfos[j], queryPool, j * 4, transferQfTimestampValidBits);

			if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }

			bres = record_compute_cmdbuffer(
				computeCmdBuffers[j], pipeline, pipelineLayout, &descSets[j], computeDependencyInfos[j], queryPool,
				j * 4 + 2, computeQfTimestampValidBits, workgroupCount);

			if EXPECT_FALSE (!bres) { free_recursive(dyMem); return false; }
		}
	}

	if (gpu->usingMaintenance4) {
		VK_CALL(vkDestroyPipelineLayout, device, pipelineLayout, g_allocator);
		gpu->pipelineLayout = VK_NULL_HANDLE;
	}

	VkSemaphoreTypeCreateInfo semaphoreTypeCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO};
	semaphoreTypeCreateInfo.semaphoreType             = VK_SEMAPHORE_TYPE_TIMELINE;
	semaphoreTypeCreateInfo.initialValue              = 0;

	VkSemaphoreCreateInfo semaphoreCreateInfo = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
	semaphoreCreateInfo.pNext                 = &semaphoreTypeCreateInfo;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VK_CALL_RES(vkCreateSemaphore, device, &semaphoreCreateInfo, g_allocator, &semaphores[i]);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	free_recursive(dyMem);

#ifndef NDEBUG
	if (gpu->debugUtilsMessenger) {
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) transferCmdPool, "Transfer");
		set_debug_name(device, VK_OBJECT_TYPE_COMMAND_POOL, (uint64_t) computeCmdPool,  "Compute");

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
	}
#endif

	return true;
}

bool submit_commands(Gpu* restrict gpu)
{
	const VkDeviceMemory*  hvMemories         = gpu->hostVisibleDeviceMemories;
	const VkCommandBuffer* transferCmdBuffers = gpu->transferCommandBuffers;
	const VkCommandBuffer* computeCmdBuffers  = gpu->computeCommandBuffers;
	const VkSemaphore*     semaphores         = gpu->semaphores;

	Value*       const* mappedInBuffers  = gpu->mappedInBuffers;
	const Count* const* mappedOutBuffers = (const Count* const*) gpu->mappedOutBuffers;

	VkDevice device = gpu->device;

	VkQueue transferQueue = gpu->transferQueue;
	VkQueue computeQueue  = gpu->computeQueue;

	VkQueryPool queryPool = gpu->queryPool;

	VkDeviceSize bytesPerIn    = gpu->bytesPerIn;
	VkDeviceSize bytesPerOut   = gpu->bytesPerOut;
	VkDeviceSize bytesPerInout = gpu->bytesPerInout;

	uint32_t valuesPerInout  = gpu->valuesPerInout;
	uint32_t valuesPerHeap   = gpu->valuesPerHeap;
	uint32_t inoutsPerBuffer = gpu->inoutsPerBuffer;
	uint32_t inoutsPerHeap   = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap  = gpu->buffersPerHeap;

	uint32_t transferQfTimestampValidBits = gpu->transferQueueFamilyTimestampValidBits;
	uint32_t computeQfTimestampValidBits  = gpu->computeQueueFamilyTimestampValidBits;

	double timestampPeriod = (double) gpu->timestampPeriod;
	bool   hostNonCoherent = gpu->hostNonCoherent;

	CliOutput outputLevel = gpu->outputLevel;
	uint64_t  maxLoops    = gpu->maxLoops;

	VkResult vkres;

	DyData dyData;
	DyArray dyMem = DyArray_create(sizeof(DyData), 3);
	if EXPECT_FALSE (!dyMem) return false;

	size_t size =
		inoutsPerHeap * sizeof(Value) +
		inoutsPerHeap * sizeof(VkMappedMemoryRange) * 2 +
		inoutsPerHeap * sizeof(VkSubmitInfo2) * 2 +
		inoutsPerHeap * sizeof(VkCommandBufferSubmitInfo) * 2 +
		inoutsPerHeap * sizeof(VkSemaphoreSubmitInfo) * 4 +
		inoutsPerHeap * sizeof(VkSemaphoreWaitInfo) * 2;

	void* p = malloc(size);
	if EXPECT_FALSE (!p) { MALLOC_FAILURE(p, size); free_recursive(dyMem); return false; }

	dyData = (DyData) {p, free};
	DyArray_append(dyMem, &dyData);

	Value* testedValues = (Value*) p;

	VkMappedMemoryRange* hvInBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (testedValues + inoutsPerHeap);
	VkMappedMemoryRange* hvOutBuffersMappedMemoryRanges = (VkMappedMemoryRange*) (
		hvInBuffersMappedMemoryRanges + inoutsPerHeap);

	VkSubmitInfo2* transferSubmitInfos = (VkSubmitInfo2*) (hvOutBuffersMappedMemoryRanges + inoutsPerHeap);
	VkSubmitInfo2* computeSubmitInfos  = (VkSubmitInfo2*) (transferSubmitInfos + inoutsPerHeap);

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

	DyArray bestValues = DyArray_create(sizeof(Value), 64);
	if EXPECT_FALSE (!bestValues) { free_recursive(dyMem); return false; }

	dyData = (DyData) {bestValues, DyArray_destroy_stub};
	DyArray_append(dyMem, &dyData);

	DyArray bestCounts = DyArray_create(sizeof(Count), 64);
	if EXPECT_FALSE (!bestCounts) { free_recursive(dyMem); return false; }

	dyData = (DyData) {bestCounts, DyArray_destroy_stub};
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
			hvInBuffersMappedMemoryRanges[j]        = (VkMappedMemoryRange) {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			hvInBuffersMappedMemoryRanges[j].memory = hvMemories[i];
			hvInBuffersMappedMemoryRanges[j].offset = bytesPerInout * k;
			hvInBuffersMappedMemoryRanges[j].size   = bytesPerIn;

			hvOutBuffersMappedMemoryRanges[j]        = (VkMappedMemoryRange) {VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE};
			hvOutBuffersMappedMemoryRanges[j].memory = hvMemories[i];
			hvOutBuffersMappedMemoryRanges[j].offset = bytesPerInout * k + bytesPerIn;
			hvOutBuffersMappedMemoryRanges[j].size   = bytesPerOut;
		}
	}

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		transferCmdBufferSubmitInfos[i] = (VkCommandBufferSubmitInfo) {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
		transferCmdBufferSubmitInfos[i].commandBuffer = transferCmdBuffers[i];

		computeCmdBufferSubmitInfos[i] = (VkCommandBufferSubmitInfo) {VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO};
		computeCmdBufferSubmitInfos[i].commandBuffer = computeCmdBuffers[i];

		transferWaitSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfo) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
		transferWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferWaitSemaphoreSubmitInfos[i].value     = 0;
		transferWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include acquire op

		transferSignalSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfo) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
		transferSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		transferSignalSemaphoreSubmitInfos[i].value     = 1;
		transferSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include release op

		computeWaitSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfo) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
		computeWaitSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeWaitSemaphoreSubmitInfos[i].value     = 1;
		computeWaitSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include acquire op

		computeSignalSemaphoreSubmitInfos[i] = (VkSemaphoreSubmitInfo) {VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO};
		computeSignalSemaphoreSubmitInfos[i].semaphore = semaphores[i];
		computeSignalSemaphoreSubmitInfos[i].value     = 2;
		computeSignalSemaphoreSubmitInfos[i].stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT; // Include release op

		transferSubmitInfos[i]                          = (VkSubmitInfo2) {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
		transferSubmitInfos[i].waitSemaphoreInfoCount   = 1;
		transferSubmitInfos[i].pWaitSemaphoreInfos      = &transferWaitSemaphoreSubmitInfos[i];
		transferSubmitInfos[i].commandBufferInfoCount   = 1;
		transferSubmitInfos[i].pCommandBufferInfos      = &transferCmdBufferSubmitInfos[i];
		transferSubmitInfos[i].signalSemaphoreInfoCount = 1;
		transferSubmitInfos[i].pSignalSemaphoreInfos    = &transferSignalSemaphoreSubmitInfos[i];

		computeSubmitInfos[i]                          = (VkSubmitInfo2) {VK_STRUCTURE_TYPE_SUBMIT_INFO_2};
		computeSubmitInfos[i].waitSemaphoreInfoCount   = 1;
		computeSubmitInfos[i].pWaitSemaphoreInfos      = &computeWaitSemaphoreSubmitInfos[i];
		computeSubmitInfos[i].commandBufferInfoCount   = 1;
		computeSubmitInfos[i].pCommandBufferInfos      = &computeCmdBufferSubmitInfos[i];
		computeSubmitInfos[i].signalSemaphoreInfoCount = 1;
		computeSubmitInfos[i].pSignalSemaphoreInfos    = &computeSignalSemaphoreSubmitInfos[i];

		transferSemaphoreWaitInfos[i]                = (VkSemaphoreWaitInfo) {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
		transferSemaphoreWaitInfos[i].semaphoreCount = 1;
		transferSemaphoreWaitInfos[i].pSemaphores    = &semaphores[i];
		transferSemaphoreWaitInfos[i].pValues        = &transferSignalSemaphoreSubmitInfos[i].value;

		computeSemaphoreWaitInfos[i]                = (VkSemaphoreWaitInfo) {VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO};
		computeSemaphoreWaitInfos[i].semaphoreCount = 1;
		computeSemaphoreWaitInfos[i].pSemaphores    = &semaphores[i];
		computeSemaphoreWaitInfos[i].pValues        = &computeSignalSemaphoreSubmitInfos[i].value;
	}

	clock_t totalBmStart = clock();

	Value tested = prevValues.curValue;

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		testedValues[i] = tested;
		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);
		tested += valuesPerInout * 4;
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hvInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, inoutsPerHeap, transferSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, inoutsPerHeap, computeSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	atomic_bool input;
	atomic_init(&input, false);

	pthread_t waitThread;
	int ires = pthread_create(&waitThread, NULL, wait_for_input, &input);
	if EXPECT_FALSE (ires) { PCREATE_FAILURE(ires); free_recursive(dyMem); return false; }

	for (uint32_t i = 0; i < inoutsPerHeap; i++) {
		VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[i], UINT64_MAX);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

		write_inbuffer(mappedInBuffers[i], &testedValues[i], valuesPerInout, valuesPerHeap);

		transferWaitSemaphoreSubmitInfos[i].value   += 2;
		transferSignalSemaphoreSubmitInfos[i].value += 2;
	}

	if (hostNonCoherent) {
		VK_CALL_RES(vkFlushMappedMemoryRanges, device, inoutsPerHeap, hvInBuffersMappedMemoryRanges);
		if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
	}

	VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, inoutsPerHeap, transferSubmitInfos, VK_NULL_HANDLE);
	if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

	Value total      = 0;
	Value startValue = prevValues.curValue;

	// ===== Enter main loop =====
	for (uint64_t i = 0; i < maxLoops && !atomic_load(&input); i++) {
		clock_t mainLoopBmStart = clock();

		Value initialValue = prevValues.curValue;

		double readBmTotal         = 0;
		double writeBmTotal        = 0;
		double waitComputeBmTotal  = 0;
		double waitTransferBmTotal = 0;
		double computeBmTotal      = 0;
		double transferBmTotal     = 0;

		if (outputLevel > CLI_OUTPUT_SILENT) { printf("Loop #%" PRIu64 "\n", i + 1); }

		for (uint32_t j = 0; j < inoutsPerHeap; j++) {
			uint64_t timestamps[2];

			double computeBmark  = 0;
			double transferBmark = 0;

			clock_t waitComputeBmStart = clock();
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &computeSemaphoreWaitInfos[j], UINT64_MAX);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			clock_t waitComputeBmEnd = clock();

			if (computeQfTimestampValidBits) {
				VK_CALL_RES(
					vkGetQueryPoolResults, device, queryPool, j * 4 + 2, 2, sizeof(timestamps), timestamps,
					sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT);

				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

				computeBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			computeWaitSemaphoreSubmitInfos[j].value   += 2;
			computeSignalSemaphoreSubmitInfos[j].value += 2;

			VK_CALL_RES(vkQueueSubmit2KHR, computeQueue, 1, &computeSubmitInfos[j], VK_NULL_HANDLE);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

			clock_t waitTransferBmStart = clock();
			VK_CALL_RES(vkWaitSemaphoresKHR, device, &transferSemaphoreWaitInfos[j], UINT64_MAX);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			clock_t waitTransferBmEnd = clock();

			if (transferQfTimestampValidBits) {
				VK_CALL_RES(
					vkGetQueryPoolResults, device, queryPool, j * 4, 2, sizeof(timestamps), timestamps,
					sizeof(timestamps[0]), VK_QUERY_RESULT_64_BIT);

				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }

				transferBmark = (double) (timestamps[1] - timestamps[0]) * timestampPeriod / 1000000;
			}

			if (hostNonCoherent) {
				VK_CALL_RES(vkInvalidateMappedMemoryRanges, device, 1, &hvOutBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			}

			clock_t readBmStart = clock();
			read_outbuffer(mappedOutBuffers[j], &prevValues, bestValues, bestCounts, valuesPerInout);
			clock_t readBmEnd = clock();

			clock_t writeBmStart = clock();
			write_inbuffer(mappedInBuffers[j], &testedValues[j], valuesPerInout, valuesPerHeap);
			clock_t writeBmEnd = clock();

			if (hostNonCoherent) {
				VK_CALL_RES(vkFlushMappedMemoryRanges, device, 1, &hvInBuffersMappedMemoryRanges[j]);
				if EXPECT_FALSE (vkres) { free_recursive(dyMem); return false; }
			}

			transferWaitSemaphoreSubmitInfos[j].value   += 2;
			transferSignalSemaphoreSubmitInfos[j].value += 2;

			VK_CALL_RES(vkQueueSubmit2KHR, transferQueue, 1, &transferSubmitInfos[j], VK_NULL_HANDLE);
			if EXPECT_FALSE (vkres) { free_recursive(dyMem); return NULL; }

			double readBmark         = get_benchmark(readBmStart, readBmEnd);
			double writeBmark        = get_benchmark(writeBmStart, writeBmEnd);
			double waitComputeBmark  = get_benchmark(waitComputeBmStart, waitComputeBmEnd);
			double waitTransferBmark = get_benchmark(waitTransferBmStart, waitTransferBmEnd);

			readBmTotal         += readBmark;
			writeBmTotal        += writeBmark;
			computeBmTotal      += computeBmark;
			transferBmTotal     += transferBmark;
			waitComputeBmTotal  += waitComputeBmark;
			waitTransferBmTotal += waitTransferBmark;

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
					readBmark, writeBmark, computeBmark, transferBmark, waitComputeBmark, waitTransferBmark);
			}
		}

		total += valuesPerHeap * 4;

		clock_t mainLoopBmEnd = clock();
		double  mainLoopBmark = get_benchmark(mainLoopBmStart, mainLoopBmEnd);

		Value currentValue = prevValues.curValue;

		switch (outputLevel) {
			case CLI_OUTPUT_SILENT: break;
			case CLI_OUTPUT_QUIET:
				printf(
					"Main loop: %.0fms\n"
					"Current value: 0x %016" PRIx64 " %016" PRIx64 "\n\n",
					mainLoopBmark, INT128_UPPER(currentValue - 3), INT128_LOWER(currentValue - 3));
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
					mainLoopBmark,
					readBmTotal / (double) inoutsPerHeap,        writeBmTotal / (double) inoutsPerHeap,
					computeBmTotal / (double) inoutsPerHeap,     transferBmTotal / (double) inoutsPerHeap,
					waitComputeBmTotal / (double) inoutsPerHeap, waitTransferBmTotal / (double) inoutsPerHeap,
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
					mainLoopBmark,
					readBmTotal,  readBmTotal / (double) inoutsPerHeap,
					writeBmTotal, writeBmTotal / (double) inoutsPerHeap,
					computeBmTotal,  computeBmTotal / (double) inoutsPerHeap,
					transferBmTotal, transferBmTotal / (double) inoutsPerHeap,
					waitComputeBmTotal,  waitComputeBmTotal / (double) inoutsPerHeap,
					waitTransferBmTotal, waitTransferBmTotal / (double) inoutsPerHeap,
					INT128_UPPER(initialValue - 2), INT128_LOWER(initialValue - 2),
					INT128_UPPER(currentValue - 3), INT128_LOWER(currentValue - 3));
				break;
			default: break;
		}
	}
	NEWLINE();

	clock_t totalBmEnd = clock();
	double  totalBmark = get_benchmark(totalBmStart, totalBmEnd);

	if (atomic_load(&input)) {
		ires = pthread_join(waitThread, NULL);
		if EXPECT_FALSE (ires) { PJOIN_FAILURE(ires); free_recursive(dyMem); return false; }
	}
	else { atomic_store(&input, true); }

	uint32_t count = (uint32_t) DyArray_size(bestValues);

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

		DyArray_get(bestValues, &value, i);
		DyArray_get(bestCounts, &steps, i);

		printf(
			"| %5" PRIu32 " | %016" PRIx64 " %016" PRIx64 " | %10" PRIu16 " |\n",
			i + 1, INT128_UPPER(value), INT128_LOWER(value), steps);
	}

	if (outputLevel > CLI_OUTPUT_QUIET) {
		printf(
			"\n"
			"Time: %.3fms\n"
			"Speed: %.3f/s\n",
			totalBmark, (double) (1000 * total) / totalBmark);
	}

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

	const VkBuffer*       hvBuffers  = gpu->hostVisibleBuffers;
	const VkBuffer*       dlBuffers  = gpu->deviceLocalBuffers;
	const VkDeviceMemory* hvMemories = gpu->hostVisibleDeviceMemories;
	const VkDeviceMemory* dlMemories = gpu->deviceLocalDeviceMemories;
	const VkSemaphore*    semaphores = gpu->semaphores;

	VkDebugUtilsMessengerEXT debugUtilsMessenger = gpu->debugUtilsMessenger;

	VkDevice device = gpu->device;

	VkDescriptorSetLayout descSetLayout   = gpu->descriptorSetLayout;
	VkDescriptorPool      descPool        = gpu->descriptorPool;
	VkShaderModule        shaderModule    = gpu->shaderModule;
	VkPipelineCache       pipelineCache   = gpu->pipelineCache;
	VkPipelineLayout      pipelineLayout  = gpu->pipelineLayout;
	VkPipeline            pipeline        = gpu->pipeline;
	VkCommandPool         computeCmdPool  = gpu->computeCommandPool;
	VkCommandPool         transferCmdPool = gpu->transferCommandPool;
	VkQueryPool           queryPool       = gpu->queryPool;

	uint32_t inoutsPerHeap  = gpu->inoutsPerHeap;
	uint32_t buffersPerHeap = gpu->buffersPerHeap;

	VkResult vkres;

	if (device) {
		VK_CALL(vkDestroyShaderModule,        device, shaderModule,   g_allocator);
		VK_CALL(vkDestroyPipelineCache,       device, pipelineCache,  g_allocator);
		VK_CALL(vkDestroyPipelineLayout,      device, pipelineLayout, g_allocator);
		VK_CALL(vkDestroyDescriptorSetLayout, device, descSetLayout,  g_allocator);

		// Make sure no command buffers are in the pending state
		VK_CALL_RES(vkDeviceWaitIdle, device);

		for (uint32_t i = 0; i < inoutsPerHeap; i++) {
			VK_CALL(vkDestroySemaphore, device, semaphores[i], g_allocator);
		}

		VK_CALL(vkDestroyCommandPool, device, computeCmdPool,  g_allocator);
		VK_CALL(vkDestroyCommandPool, device, transferCmdPool, g_allocator);

		VK_CALL(vkDestroyPipeline,       device, pipeline,  g_allocator);
		VK_CALL(vkDestroyQueryPool,      device, queryPool, g_allocator);
		VK_CALL(vkDestroyDescriptorPool, device, descPool,  g_allocator);

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
	atomic_bool* input = (atomic_bool*) ptr;

	puts("Calculating... press enter/return to stop\n");
	getchar();

	if (!atomic_load(input)) {
		puts("Stopping...\n");
		atomic_store(input, true);
	}

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
	restrict DyArray bestValues,
	restrict DyArray bestCounts,
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
			new_high(&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, bestValues, bestCounts); }
		else {
			for (uint32_t j = 2; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 1] || value != val0mod1off[j] * 2) continue;

				val0mod1off[j - 1] = value;

				break;
			}
		}

		value++; // value % 8 == 3

		if (mappedOutBuffer[i] > count) {
			new_high(&value, &count, mappedOutBuffer[i], val0mod1off, val1mod6off, bestValues, bestCounts); }
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
			new_high(&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, bestValues, bestCounts); }
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
			new_high(&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, bestValues, bestCounts); }
		else {
			for (uint32_t j = 2; j < ARR_SIZE(val0mod1off); j++) {
				if (val0mod1off[j - 1] || value != val0mod1off[j] * 2) continue;

				val0mod1off[j - 1] = value;

				break;
			}
		}

		value++; // value % 8 == 7

		if (mappedOutBuffer[i] > count) {
			new_high(&value, &count, mappedOutBuffer[i], val0mod1off, val1mod6off, bestValues, bestCounts); }
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
			new_high(&value, &count, (Count) (count + 1), val0mod1off, val1mod6off, bestValues, bestCounts); }
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

			new_high(&value, &count, (Count) (count - j + 3), val0mod1off, val1mod6off, bestValues, bestCounts);

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
	restrict DyArray bestValues,
	restrict DyArray bestCounts)
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

	DyArray_append(bestValues, value);
	DyArray_append(bestCounts, count);
}
