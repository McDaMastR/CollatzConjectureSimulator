# Vulkan Synchronisation

A description of the Vulkan synchronisation that occurs within Collatz Conjecture Simulator.

## Legend

| Memory Operation | Source             | Destination        |
| ---------------- | ------------------ | ------------------ |
| read             | memory range       |                    |
| write            |                    | memory range       |
| availability     | (agent; reference) | memory domain      |
| memory domain    | memory domain      | memory domain      |
| visibility       | memory domain      | (agent; reference) |
| release          | memory range       |                    |
| acquire          |                    | memory range       |

## Main thread

| Memory Operation | Source                | Destination                        | Function                       |
| ---------------- | --------------------- | ---------------------------------- | ------------------------------ |
| visibility       | host domain           | (host threads; HV-out)             | vkInvalidateMappedMemoryRanges |
| read             | HV-out                |                                    | readOutBuffer                  |
| write            |                       | HV-in                              | writeInBuffer                  |
| availability     | (host threads; HV-in) | host domain                        | vkFlushMappedMemoryRanges      |
| memory domain    | host domain           | device domain                      | vkQueueSubmit2KHR              |
| visibility       | device domain         | (device agents; device references) |                                |

## Transfer command buffer

| Memory Operation | Source                    | Destination               | Function                 |
| ---------------- | ------------------------- | ------------------------- | ------------------------ |
| read             | HV-in                     |                           | vkCmdCopyBuffer          |
| write            |                           | DL-in                     |                          |
| availability     | (copy operations; DL-in)  | device domain             | vkCmdPipelineBarrier2KHR |
| release          | DL-in                     |                           |                          |
| acquire          |                           | DL-out                    |                          |
| visibility       | device domain             | (copy operations; DL-out) |                          |
| read             | DL-out                    |                           | vkCmdCopyBuffer          |
| write            |                           | HV-out                    |                          |
| availability     | (copy operations; HV-out) | device domain             | vkCmdPipelineBarrier2KHR |
| memory domain    | device domain             | host domain               |                          |

## Compute command buffer

| Memory Operation | Source                        | Destination                  | Function                 |
| ---------------- | ----------------------------- | ---------------------------- | ------------------------ |
| acquire          |                               | DL-in                        | vkCmdPipelineBarrier2KHR |
| visibility       | device domain                 | (dispatch operations; DL-in) |                          |
| read             | DL-in                         |                              | vkCmdDispatchBase        |
| write            |                               | DL-out                       |                          |
| availability     | (dispatch operations; DL-out) | device domain                | vkCmdPipelineBarrier2KHR |
| release          | DL-out                        |                              |                          |
