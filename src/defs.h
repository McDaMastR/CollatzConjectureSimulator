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

#pragma once


// Check support for necessary features

#ifdef __STDC_NO_ATOMICS__
	#error "Compiler must support C11 atomic types and operations"
#endif

#ifndef __SIZEOF_INT128__
	#error "Compiler must support GNU C 128-bit integer extension"
#elif __SIZEOF_INT128__ != 16
	#error "Target must support 128-bit integers"
#endif


// Check support for optional builtins and attributes

#ifdef __has_builtin
	#define has_builtin(x) __has_builtin(__builtin_##x)
#else
	#define has_builtin(x) 0
#endif

#ifdef __has_attribute
	#define has_attribute(x) __has_attribute(x)
#else
	#define has_attribute(x) 0
#endif

#if has_builtin(assume)
	#define ASSUME(x) __builtin_assume (x)
#elif has_attribute(assume)
	#define ASSUME(x) __attribute__ (( assume (x) ))
#elif defined(_MSC_VER)
	#define ASSUME(x) __assume (x)
#else
	#define ASSUME(x)
#endif

#if has_builtin(expect)
	#define EXPECT_TRUE(x)  ( __builtin_expect ((bool) (x), true)  )
	#define EXPECT_FALSE(x) ( __builtin_expect ((bool) (x), false) )
#else
	#define EXPECT_TRUE(x)  (x)
	#define EXPECT_FALSE(x) (x)
#endif

#if has_attribute(hot)
	#define HOT_FUNC __attribute__ (( hot ))
#else
	#define HOT_FUNC
#endif

#if has_attribute(cold)
	#define COLD_FUNC __attribute__ (( cold ))
#else
	#define COLD_FUNC
#endif

#if has_attribute(const)
	#define CONST_FUNC __attribute__ (( const ))
#else
	#define CONST_FUNC
#endif

#if has_attribute(pure)
	#define PURE_FUNC __attribute__ (( pure ))
#else
	#define PURE_FUNC
#endif

#if has_attribute(malloc)
	#define MALLOC_FUNC __attribute__ (( malloc ))
#else
	#define MALLOC_FUNC
#endif

#if has_attribute(malloc) && !defined(__clang__)
	#define FREE_FUNC(func, arg) __attribute__ (( malloc (func, arg) ))
#else
	#define FREE_FUNC(func, arg)
#endif

#if has_attribute(format)
	#ifdef __clang__
		#define PRINTF_FUNC(fmt, args) __attribute__ (( format (printf, fmt, args) ))
		#define SCANF_FUNC(fmt, args)  __attribute__ (( format (scanf,  fmt, args) ))
	#else
		#define PRINTF_FUNC(fmt, args) __attribute__ (( format (gnu_printf, fmt, args) ))
		#define SCANF_FUNC(fmt, args)  __attribute__ (( format (gnu_scanf,  fmt, args) ))
	#endif
#else
	#define PRINTF_FUNC(fmt, varg)
	#define SCANF_FUNC(fmt, varg)
#endif

#if has_attribute(nonnull)
	#define NONNULL_ARGS(...) __attribute__ (( nonnull (__VA_ARGS__) ))
	#define NONNULL_ARGS_ALL  __attribute__ (( nonnull ))
#else
	#define NONNULL_ARGS(...)
	#define NONNULL_ARGS_ALL
#endif

#if has_attribute(alloc_size)
	#define ALLOC_ARG(arg)         __attribute__ (( alloc_size (arg) ))
	#define ALLOC_ARGS(arg1, arg2) __attribute__ (( alloc_size (arg1, arg2) ))
#else
	#define ALLOC_ARG(arg)
	#define ALLOC_ARGS(arg1, arg2)
#endif

#if has_attribute(alloc_align)
	#define ALIGN_ARG(arg) __attribute__ (( alloc_align (arg) ))
#else
	#define ALIGN_ARG(arg)
#endif

#if has_attribute(null_terminated_string_arg)
	#define NULTSTR_ARG(arg) __attribute__ (( null_terminated_string_arg (arg) ))
#else
	#define NULTSTR_ARG(arg)
#endif

#if has_attribute(access)
	#define NO_ACCESS(arg) __attribute__ (( access (none,       arg) ))
	#define RE_ACCESS(arg) __attribute__ (( access (read_only,  arg) ))
	#define WR_ACCESS(arg) __attribute__ (( access (write_only, arg) ))
	#define RW_ACCESS(arg) __attribute__ (( access (read_write, arg) ))
#else
	#define NO_ACCESS(arg)
	#define RE_ACCESS(arg)
	#define WR_ACCESS(arg)
	#define RW_ACCESS(arg)
#endif

#if has_attribute(returns_nonnull)
	#define NONNULL_RET __attribute__ (( returns_nonnull ))
#else
	#define NONNULL_RET
#endif

#if has_attribute(warn_unused_result)
	#define USE_RET __attribute__ (( warn_unused_result ))
#else
	#define USE_RET
#endif


// Check support for nonstandard predefined macros

#ifdef __FILE_NAME__
	#define FILE_NAME __FILE_NAME__
#else
	#define FILE_NAME __FILE__
#endif


// ANSI escape codes regarding Select Graphic Rendition

#define SGR_RESET      "\e[m"
#define SGR_BOLD       "\e[1m"
#define SGR_FAINT      "\e[2m"
#define SGR_ITALIC     "\e[3m"
#define SGR_UNDERLINE  "\e[4m"
#define SGR_SLOW_BLINK "\e[5m"
#define SGR_FAST_BLINK "\e[6m"
#define SGR_INVERT     "\e[7m"
#define SGR_CONCEAL    "\e[8m"
#define SGR_STRIKE     "\e[9m"

#define SGR_FG_BLACK   "\e[30m"
#define SGR_FG_RED     "\e[31m"
#define SGR_FG_GREEN   "\e[32m"
#define SGR_FG_YELLOW  "\e[33m"
#define SGR_FG_BLUE    "\e[34m"
#define SGR_FG_MAGENTA "\e[35m"
#define SGR_FG_CYAN    "\e[36m"
#define SGR_FG_WHITE   "\e[37m"
#define SGR_FG_DEFAULT "\e[39m"

#define SGR_BG_BLACK   "\e[40m"
#define SGR_BG_RED     "\e[41m"
#define SGR_BG_GREEN   "\e[42m"
#define SGR_BG_YELLOW  "\e[43m"
#define SGR_BG_BLUE    "\e[44m"
#define SGR_BG_MAGENTA "\e[45m"
#define SGR_BG_CYAN    "\e[46m"
#define SGR_BG_WHITE   "\e[47m"
#define SGR_BG_DEFAULT "\e[49m"


// Helper macros

#define NEWLINE() putchar('\n')

#define ARR_SIZE(a) ( sizeof(a) / sizeof(*(a)) )

#define INT128_UPPER(x) ( (uint64_t) ((x) >> 64)   )
#define INT128_LOWER(x) ( (uint64_t) ((x) & ~0ULL) )

#define INT128(upper, lower) ( (Value) (upper) << 64 | (Value) (lower) )

#define PNEXT_ADD(p, s) do { *p = &s; p = &s.pNext; } while (0)


// Undefine select Vulkan string macros (redefined as constants below)

#undef VK_KHR_8BIT_STORAGE_EXTENSION_NAME
#undef VK_KHR_16BIT_STORAGE_EXTENSION_NAME
#undef VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME
#undef VK_KHR_MAINTENANCE_4_EXTENSION_NAME
#undef VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME
#undef VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME
#undef VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME
#undef VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME
#undef VK_KHR_SPIRV_1_4_EXTENSION_NAME
#undef VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME
#undef VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME

#undef VK_EXT_DEBUG_UTILS_EXTENSION_NAME
#undef VK_EXT_MEMORY_BUDGET_EXTENSION_NAME
#undef VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME
#undef VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME
#undef VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME


// Datatypes

typedef enum Endianness
{
	ENDIANNESS_BIG    = 0,
	ENDIANNESS_LITTLE = 1
} Endianness;

typedef enum OutputLevel
{
	OUTPUT_LEVEL_SILENT  = 0,
	OUTPUT_LEVEL_QUIET   = 1,
	OUTPUT_LEVEL_DEFAULT = 2,
	OUTPUT_LEVEL_VERBOSE = 3
} OutputLevel;

typedef enum ColourLevel
{
	COLOUR_LEVEL_NONE = 0,
	COLOUR_LEVEL_TTY  = 1,
	COLOUR_LEVEL_ALL  = 2
} ColourLevel;

typedef struct ProgramConfig
{
	OutputLevel outputLevel;
	ColourLevel colourLevel;

	unsigned long      iterSize;
	unsigned long long maxLoops;
	float              maxMemory;

	bool preferInt16;
	bool preferInt64;

	bool extensionLayers;
	bool profileLayers;
	bool validationLayers;

	bool restartCount;
	bool queryBenchmarking;
	bool logAllocations;
	bool capturePipelines;
} ProgramConfig;

typedef unsigned __int128 Value;

typedef uint16_t Count;


// Global constants

extern const char* const PROGRAM_NAME;
extern const char* const PROGRAM_EXE;
extern const char* const PROGRAM_COPYRIGHT;
extern const char* const PROGRAM_LICENCE;

extern const char* const DEBUG_LOG_NAME;
extern const char* const ALLOC_LOG_NAME;
extern const char* const PIPELINE_CACHE_NAME;
extern const char* const PROGRESS_FILE_NAME;
extern const char* const CAPTURE_FILE_NAME;

extern const char* const VK_KHR_PROFILES_LAYER_NAME;
extern const char* const VK_KHR_VALIDATION_LAYER_NAME;
extern const char* const VK_KHR_SYNCHRONIZATION_2_LAYER_NAME;
extern const char* const VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME;

extern const char* const VK_KHR_8BIT_STORAGE_EXTENSION_NAME;
extern const char* const VK_KHR_16BIT_STORAGE_EXTENSION_NAME;
extern const char* const VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME;
extern const char* const VK_KHR_MAINTENANCE_4_EXTENSION_NAME;
extern const char* const VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME;
extern const char* const VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME;
extern const char* const VK_KHR_PORTABILITY_SUBSET_EXTENSION_NAME;
extern const char* const VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME;
extern const char* const VK_KHR_SPIRV_1_4_EXTENSION_NAME;
extern const char* const VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME;
extern const char* const VK_KHR_TIMELINE_SEMAPHORE_EXTENSION_NAME;

extern const char* const VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
extern const char* const VK_EXT_MEMORY_BUDGET_EXTENSION_NAME;
extern const char* const VK_EXT_MEMORY_PRIORITY_EXTENSION_NAME;
extern const char* const VK_EXT_PIPELINE_CREATION_CACHE_CONTROL_EXTENSION_NAME;
extern const char* const VK_EXT_SUBGROUP_SIZE_CONTROL_EXTENSION_NAME;

extern const uint32_t PROGRAM_VERSION;
extern const uint32_t PROGRAM_VER_MAJOR;
extern const uint32_t PROGRAM_VER_MINOR;
extern const uint32_t PROGRAM_VER_PATCH;

extern const double MS_PER_CLOCK;

extern const VkAllocationCallbacks  g_allocationCallbacks;
extern const VkAllocationCallbacks* g_allocator;

extern ProgramConfig g_config;
