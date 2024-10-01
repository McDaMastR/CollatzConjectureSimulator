# Collatz Conjecture Simulator

A C program to efficiently determine the total stopping time of the Collatz sequence of any 128-bit
integer starting value, and output all starting values $n$ whose total stopping time is the greatest
of all integers in the interval $[1, n]$.

## The Collatz Conjecture

The [Collatz Conjecture](https://en.wikipedia.org/wiki/Collatz_conjecture) is a famous unsolved
mathematical problem. It regards the _Collatz function_, a mathematical function which takes a
positive integer input $n$ and gives a positive integer output $f(n)$. If the input is even, the
function returns half the input. If the input is odd, the function returns triple the input, plus
one.

```math
f(n) =
 \begin{cases}
  n/2  & \text{if } n \equiv 0 \pmod 2\\
  3n+1 & \text{if } n \equiv 1 \pmod 2
 \end{cases}
```

The Collatz function can be applied recursively, meaning given an initial input $n$ and resultant
output $f(n)$, this first output can be used as an input, resulting in a second output $f(f(n))$.
This second output can again be used as an input, resulting in a third output $f^3(n)$. And so on.

By applying the Collatz function recursively, the sequence of successive inputs and outputs will
form a _Collatz sequence_. If a Collatz sequence includes the value $1$, then the number of
elements in the sequence from the starting value to the first instance of the value $1$ is the
_total stopping time_. That is, given a starting value $n$ and total stopping time $k$, $f^k(n) =
1$.

The Collatz Conjecture states that for all positive integer starting values $n$, finite recursive
application of the Collatz function will eventually result in the value $1$. Using mathematical
logic:

```math
\forall n \in \mathbb{Z}_{> 0}, \exists k \in \mathbb{Z}_{\geq 0} : f^k(n) = 1
```

## The Simulation

This program aims to find the Collatz sequences with the greatest total stopping times. That is,
positive integer values $n$ such that of the set of integers in the interval $[1, n]$, the starting
value with the greatest total stopping time is $n$. Total stopping times are calculated by
iterating through Collatz sequences and counting each step (application of the Collatz function)
until a value of $1$ is found.

Due to every iteration through a Collatz sequence being computationally independent of any other,
it is possible to calculate the total stopping times of multiple starting values simultaneously. As
such, the program uses the GPU to iterate through multiple Collatz sequences in parallel. The GPU
is accessed via the Vulkan API, and uses compute shaders to perform the iterations. The program is
primarily written in C, and the shaders are written in GLSL.

## Program Requirements

The system requirements that must be met for the program to build and run correctly. The full
requirements of the GPU are given in [device_requirements.md](device_requirements.md).

- [Little endian](https://en.wikipedia.org/wiki/Endianness)
- [CMake](https://cmake.org) 3.21
- [pthreads](https://en.wikipedia.org/wiki/Pthreads)
- [glslang](https://github.com/KhronosGroup/glslang)
- [SPIR-V Tools](https://github.com/KhronosGroup/SPIRV-Tools)
  - `spirv-link`
  - `spirv-opt`
  - `spirv-dis`
- [C](https://en.wikipedia.org/wiki/C_(programming_language))11
  - `_Atomic`
  - `__int128`
- [Vulkan](https://www.vulkan.org) 1.1
  - `storageBuffer16BitAccess`
  - `synchronization2`
  - `timelineSemaphore`

## Building and Running

The program is built via CMake. To generate the build system, navigate the terminal to the project
directory and execute the following command.

```bash
cmake -S . -B build
```

Several options can be specified to customise the build system. `CMAKE_BUILD_TYPE` can be set to
Debug or Release to specify a debug or release build variant, respectively. If not set, it defaults
to Debug. `DEBUG_SHADERS` is a boolean specifying whether to include debug information in generated
SPIR-V, and defaults to OFF. `OPTIMISE_SHADERS` is a boolean specifying whether to optimise
generated SPIR-V using `spirv-opt`, and defaults to ON. `USING_DISASSEMBLER` is a boolean
specifying whether to disassemble generated SPIR-V using `spirv-dis`, and defaults to OFF.

Once the above command has finished, a `build` directory will have been created containing the
build system. To now build the program, execute the following command.

```bash
cmake --build build
```

To specify a debug or release build, add `--config Debug` or `--config Release`, respectively, to
the command. By default, only the executable will be built. To instead build the SPIR-V, add
`--target Shaders`.

The above command will create a `bin` directory containing the SPIR-V and executable. If built in
debug, the executable will be named `CollatzSim-Debug`. Otherwise, it will be named `CollatzSim`.
The executable must be run from within the `bin` directory, else the program will be unable to
locate the generated SPIR-V.

During the program's execution, a `pipeline_cache.bin` file will be created containing the data
from a `VkPipelineCache` object. This file will be read by the program if run again. If in debug,
a `debug.log` file will be created containing all debug callbacks from the Vulkan API via a
`VkDebugUtilsMessengerEXT` object, if `VK_EXT_debug_utils` is present. If logging Vulkan
allocations, an `alloc.log` file will be created containing all allocation callbacks from the
Vulkan API via a `VkAllocationCallbacks` object.

## Inout-buffers

To facilitate this use of the GPU, _inout-buffers_ are used. Inout-buffers are ranges of GPU memory
within `VkBuffer` objects and consist of an _in-buffer_ and _out-buffer_. In-buffers are shader
storage buffer objects (SSBOs) and contain an array of 128-bit unsigned integer starting values.
Out-buffers are also SSBOs and contain an array of 16-bit unsigned integer total stopping times
(step counts).

The main loop consists of the CPU writing starting values to in-buffers; the GPU reading starting
values from in-buffers, iterating through Collatz sequences, and writing step counts to
out-buffers; and the CPU reading steps counts from out-buffers. The number of inout-buffers is
dependent on the system's specifications. There are one or more inout-buffers per `VkBuffer`
object, one `VkBuffer` object per `VkDeviceMemory` object, and two or more `VkDeviceMemory`
objects.

The program attempts to minimise the time spent idle by the CPU and GPU due to one waiting for the
other to complete execution. Such as the GPU waiting for starting values, or the CPU waiting for
step counts. This is done by having an even number of `VkDeviceMemory` objects, where half contain
memory close to the GPU (device local memory), and half contain memory visible to both the CPU and
GPU (host visible memory). There are therefore four types of memory ranges: host visible in-buffers
(HV-in), host visible out-buffers (HV-out), device local in-buffers (DL-in), and device local
out-buffers (DL-out).

Rather than the CPU and GPU taking turns executing, both processors spend time running in parallel.
The CPU reads and writes host visible inout-buffers, and the GPU reads and writes device local
inout-buffers, simultaneously. Starting values are written to HV-in, copied from HV-in to DL-in,
and read from DL-in. Step counts are written to DL-out, copied from DL-out to HV-out, and read from
HV-out.

<p align="center">CPU -> HV-in -> DL-in -> GPU -> DL-out -> HV-out -> CPU</p>

## Program configurations

The program defines various global constants in [config.c](src/config.c) whose values can be
altered to configure the behaviour of the program.

`MIN_TEST_VALUE_TOP` and `MIN_TEST_VALUE_BOTTOM` are the upper and lower 64 bits, respectively, of
the 128-bit starting value that will be tested first. Subsequent tested starting values will
linearly increase from there onwards.

`MAX_STEP_VALUE_TOP` and `MAX_STEP_VALUE_BOTTOM` are the upper and lower 64 bits, respectively, of
the 128-bit starting value with the current highest step count. That is, in the set of integers
from 1 to `MIN_TEST_VALUE`, the starting value with the highest step count is `MAX_STEP_VALUE`.

`MAX_STEP_COUNT` is the step count of the starting value `MAX_STEP_VALUE`. By configuring
`MIN_TEST_VALUE`, `MAX_STEP_VALUE`, and `MAX_STEP_COUNT`, the program can resume testing starting
values from where it last ended.

`MAX_HEAP_MEMORY` is a floating-point value within the interval $(0, 1)$, describing the maximum
proportion of available memory in a `VkMemoryHeap` that can be allocated via `vkAllocateMemory`.
For example, a value of 0.8 means at most 80% of available memory in any GPU memory heap will be
allocated for inout-buffers. If `VK_EXT_memory_budget` is present, _available memory_ refers to the
`VkPhysicalDeviceMemoryBudgetPropertiesEXT::heapBudget` of a memory heap. Elsewise, it refers to
the corresponding `VkMemoryHeap::size`. By default, `MAX_HEAP_MEMORY` is set to 0.4.

`QUERY_BENCHMARKING` is a boolean describing whether Vulkan commands will be benchmarked via
queries. If true, the `vkCmdCopyBuffer` and `vkCmdDispatchBase` commands will be benchmarked. By
default, it is set to true.

`LOG_VULKAN_ALLOCATIONS` is a boolean describing whether memory allocations performed by the Vulkan
API will be logged via a `VkAllocationCallbacks` object. If true, performance may be significantly
reduced. By default, it is set to false.

`EXTENSION_LAYERS` is a boolean describing whether the Khronos
[extension layers](https://github.com/KhronosGroup/Vulkan-ExtensionLayer)
`VK_LAYER_KHRONOS_synchronization2` and `VK_LAYER_KHRONOS_timeline_semaphore` will be enabled, if
present. This should be set to true if either `VK_KHR_synchronization2` or
`VK_KHR_timeline_semaphore` are not present. By default, it is set to false.

`PROFILE_LAYERS` is a boolean describing whether the Khronos
[profiles layer](https://github.com/KhronosGroup/Vulkan-Profiles) `VK_LAYER_KHRONOS_profiles` will
be enabled, if present. By default, it is set to false.

`VALIDATION_LAYERS` is a boolean describing whether the Khronos
[validation layer](https://github.com/KhronosGroup/Vulkan-ValidationLayers)
`VK_LAYER_KHRONOS_validation` will be enabled, if present. By default, it is set to false.

`PREFER_INT16` is a boolean describing whether SPIR-V modules which utilise the `Int16` capability
will be prioritised, if the `shaderInt16` feature is enabled. By default, it is set to false.

`PREFER_INT64` is a boolean describing whether SPIR-V modules which utilise the `Int64` capability
will be prioritised, if the `shaderInt64` feature is enabled. By default, it is set to false.

`ITER_SIZE` is an integer describing the integer size to emulate when iterating through Collatz
sequences. The possible values are 128 and 256, corresponding to 128-bit and 256-bit integer sizes,
respectively. By default, it is set to 128.
