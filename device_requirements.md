# GPU Requirements

The full set of optional and required functionality of the selected `VkPhysicalDevice` object.

Because much of the desired GPU functionality is optional, particularly many extensions and features, any functionality
that is required will be marked with (\*). If all the desired functionality specified by a struct is required, then the
struct itself will be marked with (\*) rather than each listed element and quality.

Additionally, many requirements are already guaranteed to be satisfied by the core API, corresponding extensions, or
other related functionality. Such requirements may be annotated with this guarantee. Some functionality is desired
solely as a dependency for a different functionality and is not further used by the application. Such functionality may
be annotated with this dependency. Note that for functionalities A and B, A requiring B implies B is guaranteed by A.

## Extensions

- `VK_KHR_create_renderpass2` (required by `VK_KHR_depth_stencil_resolve`)
- `VK_KHR_depth_stencil_resolve` (required by `VK_KHR_dynamic_rendering`)
- `VK_KHR_dynamic_rendering` (required by `VK_KHR_maintenance5`)
- `VK_KHR_maintenance4`
- `VK_KHR_maintenance5`
- `VK_KHR_maintenance6`
- `VK_KHR_maintenance7`
- `VK_KHR_maintenance8`
- `VK_KHR_maintenance9`
- `VK_KHR_shader_float_controls` (required by `VK_KHR_spirv_1_4`)
- `VK_KHR_spirv_1_4`
- `VK_KHR_synchronization2` (\*)
- `VK_KHR_timeline_semaphore`(\*)
- `VK_EXT_memory_budget`
- `VK_EXT_memory_priority`
- `VK_EXT_pipeline_creation_cache_control`
- `VK_EXT_subgroup_size_control`

## Features

`VkPhysicalDeviceFeatures`

- `shaderInt16`
- `shaderInt64`

`VkPhysicalDevice16BitStorageFeatures`

- `storageBuffer16BitAccess`

`VkPhysicalDeviceDynamicRenderingFeaturesKHR`

- `dynamicRendering` (required by `VK_KHR_dynamic_rendering`)

`VkPhysicalDeviceMaintenance4FeaturesKHR`

- `maintenance4` (guaranteed by `VK_KHR_maintenance4`)

`VkPhysicalDeviceMaintenance5FeaturesKHR`

- `maintenance5` (guaranteed by `VK_KHR_maintenance5`)

`VkPhysicalDeviceMaintenance6FeaturesKHR`

- `maintenance6` (guaranteed by `VK_KHR_maintenance6`)

`VkPhysicalDeviceMaintenance7FeaturesKHR`

- `maintenance7` (guaranteed by `VK_KHR_maintenance7`)

`VkPhysicalDeviceMaintenance8FeaturesKHR`

- `maintenance8` (guaranteed by `VK_KHR_maintenance8`)

`VkPhysicalDeviceMaintenance9FeaturesKHR`

- `maintenance9` (guaranteed by `VK_KHR_maintenance9`)

`VkPhysicalDeviceSynchronization2FeaturesKHR` (\*)

- `synchronization2` (guaranteed by `VK_KHR_synchronization2`)

`VkPhysicalDeviceTimelineSemaphoreFeaturesKHR` (\*)

- `timelineSemaphore` (guaranteed by `VK_KHR_timeline_semaphore`)

`VkPhysicalDeviceMemoryPriorityFeaturesEXT`

- `memoryPriority` (guaranteed by `VK_EXT_memory_priority`)

`VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT`

- `pipelineCreationCacheControl` (guaranteed by `VK_EXT_pipeline_creation_cache_control`)

`VkPhysicalDeviceSubgroupSizeControlFeaturesEXT`

- `subgroupSizeControl` (guaranteed by `VK_EXT_subgroup_size_control`)

## Properties

`VkPhysicalDeviceProperties` (\*)

- `apiVersion` >= `VK_API_VERSION_1_1`

`VkPhysicalDeviceLimits` (\*)

- `maxStorageBufferRange` >= 16 (guaranteed >= 134 217 728)
- `maxMemoryAllocationCount` >= 2 (guaranteed >= 4 096)
- `maxBoundDescriptorSets` >= 1 (guaranteed >= 4)
- `maxPerStageDescriptorStorageBuffers` >= 2 (guaranteed >= 4)
- `maxPerStageResources` >= 2 (guaranteed >= 128)
- `maxDescriptorSetStorageBuffers` >= 2 (guaranteed >= 24)
- `maxComputeWorkGroupCount[0]` >= 1 (guaranteed >= 65 535)
- `maxComputeWorkGroupCount[1]` >= 1 (guaranteed >= 65 535)
- `maxComputeWorkGroupCount[2]` >= 1 (guaranteed >= 65 535)
- `maxComputeWorkGroupInvocations` >= 1 (guaranteed >= 128)
- `maxComputeWorkGroupSize[0]` >= 1 (guaranteed >= 128)
- `maxComputeWorkGroupSize[1]` >= 1 (guaranteed >= 128)
- `maxComputeWorkGroupSize[2]` >= 1 (guaranteed >= 64)

`VkPhysicalDeviceMaintenance3Properties` (\*)

- `maxPerSetDescriptors` >= 2 (guaranteed >= 1 024)
- `maxMemoryAllocationSize` >= 18 (guaranteed >= 1 073 741 824)

`VkPhysicalDeviceMaintenance4PropertiesKHR` (\*)

- `maxBufferSize` >= 18 (guaranteed >= 1 073 741 824)

`VkPhysicalDeviceTimelineSemaphorePropertiesKHR` (\*)

- `maxTimelineSemaphoreValueDifference` >= 2 (guaranteed >= 2 147 483 647)

## Memory properties

### Device local memory

`VkMemoryType`

- `propertyFlags` includes `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` (\*) (guaranteed)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` (guaranteed by no
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_CACHED_BIT` (guaranteed by no
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT` (\*)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_PROTECTED_BIT` (\*)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD` (\*)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD` (\*) (guaranteed by no
`VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD`)

`VkMemoryHeap` (\*)

- `flags` includes `VK_MEMORY_HEAP_DEVICE_LOCAL_BIT` (guaranteed by `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`)

### Host visible memory

`VkMemoryType`

- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`
- `propertyFlags` includes `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` (\*) (guaranteed)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT`
- `propertyFlags` includes `VK_MEMORY_PROPERTY_HOST_CACHED_BIT`
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT` (\*) (guaranteed by
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_PROTECTED_BIT` (\*) (guaranteed by
`VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD` (\*)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD` (\*) (guaranteed by no
`VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD`)

`VkMemoryHeap`

- `flags` not includes `VK_MEMORY_HEAP_DEVICE_LOCAL_BIT` (guaranteed by no `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`)

## Queue family properties

### Compute queue family

`VkQueueFamilyProperties` (\*)

- `queueFlags` includes `VK_QUEUE_COMPUTE_BIT`
- `queueCount` >= 1 (guaranteed)

### Transfer queue family

`VkQueueFamilyProperties` (\*)

- `queueFlags` includes `VK_QUEUE_TRANSFER_BIT` (guaranteed by `VK_QUEUE_COMPUTE_BIT`)
- `queueCount` >= 1 (guaranteed)
