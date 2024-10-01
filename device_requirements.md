# GPU Requirements

The full requirements of the selected `VkPhysicalDevice` object.

## Extensions

- `VK_KHR_8bit_storage` (optional)
- `VK_KHR_buffer_device_address` (optional)
- `VK_KHR_maintenance4` (optional)
- `VK_KHR_synchronization2`
- `VK_KHR_timeline_semaphore`
- `VK_EXT_memory_budget` (optional)
- `VK_EXT_memory_priority` (optional)
- `VK_EXT_pipeline_creation_cache_control` (optional)
- `VK_EXT_subgroup_size_control` (optional)

## Features

`VkPhysicalDeviceFeatures`

- `shaderInt16` (optional)
- `shaderInt64` (optional)

`VkPhysicalDevice16BitStorageFeatures`

- `storageBuffer16BitAccess`

`VkPhysicalDevice8BitStorageFeaturesKHR`

- `storageBuffer8BitAccess` (optional, guaranteed by `VK_KHR_8bit_storage`)
- `uniformAndStorageBuffer8BitAccess` (optional)

`VkPhysicalDeviceBufferDeviceAddressFeaturesKHR`

- `bufferDeviceAddress` (optional, guaranteed by `VK_KHR_buffer_device_address`)

`VkPhysicalDeviceMaintenance4FeaturesKHR`

- `maintenance4` (optional, guaranteed by `VK_KHR_maintenance4`)

`VkPhysicalDeviceSynchronization2FeaturesKHR`

- `synchronization2` (guaranteed by `VK_KHR_synchronization2`)

`VkPhysicalDeviceTimelineSemaphoreFeaturesKHR`

- `timelineSemaphore` (guaranteed by `VK_KHR_timeline_semaphore`)

`VkPhysicalDeviceMemoryPriorityFeaturesEXT`

- `memoryPriority` (optional, guaranteed by `VK_EXT_memory_priority`)

`VkPhysicalDevicePipelineCreationCacheControlFeaturesEXT`

- `pipelineCreationCacheControl` (optional, guaranteed by `VK_EXT_pipeline_creation_cache_control`)

`VkPhysicalDeviceSubgroupSizeControlFeaturesEXT`

- `subgroupSizeControl` (optional, guaranteed by `VK_EXT_subgroup_size_control`)

## Properties

`VkPhysicalDeviceProperties`

- `apiVersion` >= `VK_API_VERSION_1_1`

`VkPhysicalDeviceLimits`

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

`VkPhysicalDeviceMaintenance3Properties`

- `maxPerSetDescriptors` >= 2 (guaranteed >= 1 024)
- `maxMemoryAllocationSize` >= 18 (guaranteed >= 1 073 741 824)

`VkPhysicalDeviceTimelineSemaphoreProperties`

- `maxTimelineSemaphoreValueDifference` >= 2 (guaranteed >= 2 147 483 647)

`VkPhysicalDeviceMaintenance4PropertiesKHR`

- `maxBufferSize` >= 18 (guaranteed >= 1 073 741 824)

## Memory properties

### Device local memory

`VkMemoryType`

- `propertyFlags` includes `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` (guaranteed)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` (optional)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` (optional, guaranteed by no `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_CACHED_BIT` (optional, guaranteed by no `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT`
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_PROTECTED_BIT`
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD`
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD` (guaranteed by no `VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD`)

`VkMemoryHeap`

- `flags` includes `VK_MEMORY_HEAP_DEVICE_LOCAL_BIT` (guaranteed by `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`)

### Host visible memory

`VkMemoryType`

- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT` (optional)
- `propertyFlags` includes `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` (guaranteed)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` (optional)
- `propertyFlags` includes `VK_MEMORY_PROPERTY_HOST_CACHED_BIT` (optional)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT` (guaranteed by `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_PROTECTED_BIT` (guaranteed by `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT`)
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD`
- `propertyFlags` not includes `VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD` (guaranteed by no `VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD`)

`VkMemoryHeap`

- `flags` not includes `VK_MEMORY_HEAP_DEVICE_LOCAL_BIT` (optional, guaranteed by no `VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT`)

## Queue family properties

### Compute queue family

`VkQueueFamilyProperties`

- `queueFlags` includes `VK_QUEUE_COMPUTE_BIT`
- `queueCount` >= 1 (guaranteed)

### Transfer queue family

`VkQueueFamilyProperties`

- `queueFlags` includes `VK_QUEUE_TRANSFER_BIT` (guaranteed by `VK_QUEUE_COMPUTE_BIT`)
- `queueCount` >= 1 (guaranteed)
