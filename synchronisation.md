# Vulkan Synchronisation

A description of the Vulkan synchronisation within the program.

## Main thread

| Memory Operation | Source                        | Destination                        | Function                       |
| ---------------- | ----------------------------- | ---------------------------------- | ------------------------------ |
| visibility       | host domain                   | (host threads; HV-out)             | vkInvalidateMappedMemoryRanges |
| read             | HV-out                        |                                    | readOutBuffer                  |
| write            |                               | HV-in                              | writeInBuffer                  |
| availability     | (host threads; HV-in)         | host domain                        | vkFlushMappedMemoryRanges      |
| memory domain    | host domain                   | device domain                      | vkQueueSubmit2KHR              |
| visibility       | device domain                 | (device agents; device references) |                                |

## Onetime command buffer

| Memory Operation | Source                        | Destination                        | Function                       |
| ---------------- | ----------------------------- | ---------------------------------- | ------------------------------ |
| read             | HV-in                         |                                    | vkCmdCopyBuffer                |
| write            |                               | DL-in                              |                                |
| availability     | (copy operations; DL-in)      | device domain                      | vkCmdPipelineBarrier2KHR       |
| release          | DL-in                         |                                    |                                |

## Transfer command buffer

| Memory Operation | Source                        | Destination                        | Function                       |
| ---------------- | ----------------------------- | ---------------------------------- | ------------------------------ |
| read             | HV-in                         |                                    | vkCmdCopyBuffer                |
| write            |                               | DL-in                              |                                |
| availability     | (copy operations; DL-in)      | device domain                      | vkCmdPipelineBarrier2KHR       |
| release          | DL-in                         |                                    |                                |
| aquire           |                               | DL-out                             |                                |
| visibility       | device domain                 | (copy operations; DL-out)          |                                |
| read             | DL-out                        |                                    | vkCmdCopyBuffer                |
| write            |                               | HV-out                             |                                |
| availability     | (copy operations; HV-out)     | device domain                      | vkCmdPipelineBarrier2KHR       |
| memory domain    | device domain                 | host domain                        |                                |

## Compute command buffer

| Memory Operation | Source                        | Destination                        | Function                       |
| ---------------- | ----------------------------- | ---------------------------------- | ------------------------------ |
| aquire           |                               | DL-in                              | vkCmdPipelineBarrier2KHR       |
| visibility       | device domain                 | (dispatch operations; DL-in)       |                                |
| read             | DL-in                         |                                    | vkCmdDispatch                  |
| write            |                               | DL-out                             |                                |
| availability     | (dispatch operations; DL-out) | device domain                      | vkCmdPipelineBarrier2KHR       |
| release          | DL-out                        |                                    |                                |
