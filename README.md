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

- [C](https://en.wikipedia.org/wiki/C_(programming_language))11
  - `_Atomic`
  - `__int128`
  - Little endian
- [CMake](https://cmake.org) 3.21
- [pthreads](https://en.wikipedia.org/wiki/Pthreads)
- [glslc](https://github.com/google/shaderc)
- [Vulkan](https://www.vulkan.org) 1.1
  - `storageBuffer16BitAccess`
  - `synchronization2`
  - `timelineSemaphore`

## Building and Running

The program is built via CMake. To generate the build system, navigate the terminal to the project
directory and execute the following command. To specify a debug or release build system, add
`-DCMAKE_BUILD_TYPE=Debug` or `-DCMAKE_BUILD_TYPE=Release`, respectively.

```text
cmake -S . -B build
```

A `build` directory will be created containing the build system. To build the program, execute the
following command. To specify a debug or release build, add `--config Debug` or `--config Release`,
respectively.

```text
cmake --build build
```

A `bin` directory will be created containing the compiled compute shaders and program executable.
To run the program, execute `CollatzConjectureSimulator.exe` from within the `bin` directory. If
not executed inside the `bin` directory, the program will be unable to locate the compiled shaders.

If in debug, a `debug_log.txt` file will be created during execution containing all debug callbacks
from the Vulkan API via a `VkDebugUtilsMessengerEXT` object, if `VK_EXT_debug_utils` is present. If
logging Vulkan allocations, an `alloc_log.txt` file will be created during execution containing all
allocation callbacks from the Vulkan API via a `VkAllocationCallbacks` object. During execution, a
`pipeline_cache.bin` file will be created containing the data from a `VkPipelineCache` object. This
file will be read by the program if run again.

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

## Preprocessor configurations

The program defines various preprocessor macros in [defs.h](src/defs.h) whose definitions can be
changed to configure the behaviour of the program.

`MIN_TEST_VALUE_TOP` and `MIN_TEST_VALUE_BOTTOM` are the upper and lower 64 bits, respectively, of
the 128-bit starting value the program will test first. Subsequent tested starting values will
linearly increase from there onwards.

`MAX_STEP_VALUE_TOP` and `MAX_STEP_VALUE_BOTTOM` are the upper and lower 64 bits, respectively, of
the 128-bit starting value with the current highest step count. That is, in the set of integers
from 1 to `MIN_TEST_VALUE`, the starting value with the highest step count is `MAX_STEP_VALUE`.

`MAX_STEP_COUNT` is the step count of the starting value `MAX_STEP_VALUE`. By configuring
`MIN_TEST_VALUE`, `MAX_STEP_VALUE`, and `MAX_STEP_COUNT`, the program can resume testing starting
values from exactly where it last ended.

`MAX_HEAP_MEMORY` is a floating-point value within the interval $(0, 1)$, describing the maximum
proportion of available memory in a `VkMemoryHeap` the program can allocate via `vkAllocateMemory`.
For example, a value of 0.8f means at most 80% of available memory in any GPU memory heap will be
allocated for inout-buffers. If `VK_EXT_memory_budget` is present, _available memory_ refers to the
`VkPhysicalDeviceMemoryBudgetPropertiesEXT::heapBudget` of a memory heap. Elsewise, it refers to
the corresponding `VkMemoryHeap::size`.

`QUERY_BENCHMARKING` is a boolean value describing whether or not the program will benchmark Vulkan
commands via queries. If 1, the `vkCmdCopyBuffer` and `vkCmdDispatchBase` commands will be
benchmarked.

`LOG_VULKAN_ALLOCATIONS` is a boolean value describing whether or not the program will log memory
allocations performed by the Vulkan API via a `VkAllocationCallbacks` object.

`EXTENSION_LAYERS` is a boolean value describing whether or not the Khronos
[extension layers](https://github.com/KhronosGroup/Vulkan-ExtensionLayer) will be enabled, if
present. This includes `VK_LAYER_KHRONOS_synchronization2` and
`VK_LAYER_KHRONOS_timeline_semaphore`. This value should be 1 if either `VK_KHR_synchronization2`
or `VK_KHR_timeline_semaphore` are not present.

`PROFILE_LAYERS` is a boolean value describing whether or not the Khronos
[profiles layer](https://github.com/KhronosGroup/Vulkan-Profiles) will be enabled, namely
`VK_LAYER_KHRONOS_profiles`.

`VALIDATION_LAYERS` is a boolean value describing whether or not the Khronos
[validation layer](https://github.com/KhronosGroup/Vulkan-ValidationLayers) will be enabled, namely
`VK_LAYER_KHRONOS_validation`.

`END_ON` is an integer value describing when the program will terminate. If 1, the program will end
on user input, namely when either of the __enter__ or __return__ keys are pressed. If 2, the
program will end when the main loop has performed a particular number of loops, such as 20 or
10 000. If 3, the program will end when a new starting value is found to have the highest step
count so far.
