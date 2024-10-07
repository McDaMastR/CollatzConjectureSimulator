/* 
 * Copyright (C) 2024  Seth McDonald <seth.i.mcdonald@gmail.com>
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


// Check compiler supports necessary features
#ifdef __STDC_NO_ATOMICS__
	#error "Compiler must support C11 atomic types and operations"
#endif

#ifndef __SIZEOF_INT128__
	#error "Compiler must support GNU C 128-bit integer extension"
#elif __SIZEOF_INT128__ != 16
	#error "Target must support 128-bit integers"
#endif

// Check which optional builtins and attributes the compiler supports
#ifndef __has_builtin
	#define __has_builtin(x) 0
#endif

#ifndef __has_attribute
	#define __has_attribute(x) 0
#endif

#if __has_builtin(__builtin_assume)
	#define ASSUME(x) __builtin_assume (x)
#elif __has_attribute(assume)
	#define ASSUME(x) __attribute__ (( assume (x) ))
#else
	#define ASSUME(x)
#endif

#if __has_builtin(__builtin_expect)
	#define EXPECT_TRUE(x)  __builtin_expect ((bool) (x), true)
	#define EXPECT_FALSE(x) __builtin_expect ((bool) (x), false)
#else
	#define EXPECT_TRUE(x)  x
	#define EXPECT_FALSE(x) x
#endif

#if __has_attribute(cold)
	#define COLD_FUNC __attribute__ (( cold ))
#else
	#define COLD_FUNC
#endif

#if __has_attribute(hot)
	#define HOT_FUNC __attribute__ (( hot ))
#else
	#define HOT_FUNC
#endif

#if __has_attribute(const)
	#define CONST_FUNC __attribute__ (( const ))
#else
	#define CONST_FUNC
#endif

#if __has_attribute(pure)
	#define PURE_FUNC __attribute__ (( pure ))
#else
	#define PURE_FUNC
#endif

#if __has_attribute(malloc)
	#define MALLOC_FUNC __attribute__ (( malloc ))
#else
	#define MALLOC_FUNC
#endif

#if __has_attribute(malloc) && !defined(__clang__)
	#define FREE_FUNC(func, index) __attribute__ (( malloc (func, index) ))
#else
	#define FREE_FUNC(func, index)
#endif

#if __has_attribute(nonnull)
	#define NONNULL_ARGS(...) __attribute__ (( nonnull (__VA_ARGS__) ))
#else
	#define NONNULL_ARGS(...)
#endif

#if __has_attribute(returns_nonnull)
	#define NONNULL_RET __attribute__ (( returns_nonnull ))
#else
	#define NONNULL_RET
#endif

#if __has_attribute(warn_unused_result)
	#define WARN_UNUSED_RET __attribute__ (( warn_unused_result ))
#else
	#define WARN_UNUSED_RET
#endif

#if __has_attribute(null_terminated_string_arg)
	#define NULTSTR_ARG(index) __attribute__ (( null_terminated_string_arg (index) ))
#else
	#define NULTSTR_ARG(index)
#endif

#if __has_attribute(access)
	#define NO_ACCESS(index) __attribute__ (( access (none,       index) ))
	#define RE_ACCESS(index) __attribute__ (( access (read_only,  index) ))
	#define WR_ACCESS(index) __attribute__ (( access (write_only, index) ))
	#define RW_ACCESS(index) __attribute__ (( access (read_write, index) ))
#else
	#define NO_ACCESS(index)
	#define RE_ACCESS(index)
	#define WR_ACCESS(index)
	#define RW_ACCESS(index)
#endif

// Helper macros
#define NEWLINE() putchar('\n');

#define ARR_SIZE(arr) (sizeof(arr) / sizeof(*(arr)))

#define INT128_UPPER(val)    ((uint64_t) ((val) >> 64))
#define INT128_LOWER(val)    ((uint64_t) ((val) & ~0ULL))
#define INT128(upper, lower) ((Value) (upper) << 64 | (Value) (lower))


// Datatypes
typedef unsigned __int128 Value;

typedef uint16_t Steps;


// Configuration constants
extern const uint64_t MIN_TEST_VALUE_UPPER;
extern const uint64_t MIN_TEST_VALUE_LOWER;
extern const Value    MIN_TEST_VALUE;

extern const uint64_t MAX_STEP_VALUE_UPPER;
extern const uint64_t MAX_STEP_VALUE_LOWER;
extern const Value    MAX_STEP_VALUE;

extern const Steps MAX_STEP_COUNT;

extern const float MAX_HEAP_MEMORY;

extern const bool QUERY_BENCHMARKING;
extern const bool LOG_VULKAN_ALLOCATIONS;

extern const bool EXTENSION_LAYERS;
extern const bool PROFILE_LAYERS;
extern const bool VALIDATION_LAYERS;

extern const bool PREFER_INT16;
extern const bool PREFER_INT64;

extern const int ITER_SIZE;

// String constants
extern const char* const PROGRAM_NAME;

extern const char* const DEBUG_LOG_NAME;
extern const char* const ALLOC_LOG_NAME;
extern const char* const PIPELINE_CACHE_NAME;

extern const char* const VK_KHR_PROFILES_LAYER_NAME;
extern const char* const VK_KHR_VALIDATION_LAYER_NAME;
extern const char* const VK_KHR_SYNCHRONIZATION_2_LAYER_NAME;
extern const char* const VK_KHR_TIMELINE_SEMAPHORE_LAYER_NAME;

// Runtime constants
extern const VkAllocationCallbacks g_allocationCallbacks;
extern const VkAllocationCallbacks* const g_allocator;

// Miscellaneous constants
extern const float MS_PER_CLOCK;
