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

$
f(n) =
 \begin{cases}
  n/2  & \text{if } n \equiv 0 \pmod 2\\
  3n+1 & \text{if } n \equiv 1 \pmod 2
 \end{cases}
$

The Collatz function can be applied recursively, meaning given an initial input $n$ and resultant
output $f(n)$, this first output can be used as an input, resulting in a second output $f(f(n))$.
This second output can again be used as an input, resulting in a third output $f^3(n)$. And so on.

By applying the Collatz function recursively, the sequence of successive inputs and outputs will
form a _Collatz sequence_. If a Collatz sequence includes the value $1$, then the number of
elements in the sequence from the starting value to the first instance of the value $1$ is the
_total stopping time_ of that Collatz sequence. That is, given a starting value $n$ and total
stopping time $k$, $f^k(n)=1$.

The Collatz Conjecture states that for all positive integer starting values $n$, finite recursive
application of the Collatz function will eventually result in the value $1$. Using mathematical
logic:

$\forall n \in \mathbb{Z}_{> 0}, \exists k \in \mathbb{Z}_{\geq 0} : f^k(n)=1$

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

- C99
  - `__int128`
  - Little endian
- CMake 3.21
- pthreads
- Vulkan 1.2
  - `VK_EXT_debug_utils` (recommended for debug)
  - `VK_LAYER_KHRONOS_validation` (recommended for debug)
  - `VK_KHR_maintenance4`
  - `VK_KHR_synchronization2`
  - `VK_EXT_memory_budget` (recommended)
  - `VK_EXT_memory_priority` (recommended)
  - `VkPhysicalDeviceFeatures::shaderInt16`
  - `VkPhysicalDeviceFeatures::shaderInt64` (recommended)
  - `VkPhysicalDevice16BitStorageFeatures::storageBuffer16BitAccess`
- shaderc
  - glslc

## Building and Running

The program is built via CMake. It uses `find_package` to locate the Vulkan and pthreads libraries,
and `find_program` to locate the glslc executable. The compute shaders are added as a custom
target and are compiled into SPIR-V when building.

To generate the build system for the program, navigate the terminal to the project directory and
execute the following command.

    cmake -S . -B build

A `build` directory will be created containing the build system. To build the program, execute the
following command.

    cmake --build build

A `bin` directory will be created containing the compiled compute shaders and program executable.
To run the program, execute `CollatzConjectureSimulator.exe` from within the `bin` directory. If
not executed inside the `bin` directory, the program will be unable to locate the compiled shaders.

If in debug, a `log.txt` file will be created during execution containing any potentially notable
callbacks from the Vulkan API via the `VK_EXT_debug_utils` extension. After execution, a
`pipeline_cache.bin` file will be created containing the data from a `VkPipelineCache` object. This
file will be read by the program if run again.

## Inout-buffers

To facilitate this use of the GPU, _inout-buffers_ are used. Inout-buffers are ranges of GPU memory
within `VkBuffer` objects and consist of an _in-buffer_ and _out-buffer_. In-buffers are storage
buffers or uniform buffers and contain an array of 128-bit unsigned integer starting values.
Out-buffers are storage buffers and contain an array of 16-bit unsigned integer total stopping
times (step counts).

The main loop consists of the CPU writing starting values to in-buffers; the GPU reading starting
values from in-buffers, iterating through Collatz sequences, and writing step counts to
out-buffers; and the CPU reading steps counts from out-buffers. The number of inout-buffers is
dependent on the system's specifications. There are one or more inout-buffers per `VkBuffer`
object, one or more `VkBuffer` objects per `VkDeviceMemory` object, and two or more
`VkDeviceMemory` objects.

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

CPU -> HV-in -> DL-in -> GPU -> DL-out -> HV-out -> CPU
