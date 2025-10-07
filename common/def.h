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

// Check support for optional attributes and builtins

#ifdef __has_attribute
	#define CZ_HAS_ATTRIBUTE(x) __has_attribute(x)
#else
	#define CZ_HAS_ATTRIBUTE(x) 0
#endif

#ifdef __has_builtin
	#define CZ_HAS_BUILTIN(x) __has_builtin(__builtin_##x)
#else
	#define CZ_HAS_BUILTIN(x) 0
#endif

#ifdef __has_include
	#define CZ_HAS_INCLUDE(x) __has_include(<x>)
#else
	#define CZ_HAS_INCLUDE(x) 0
#endif

#if CZ_HAS_BUILTIN(assume)
	#define CZ_ASSUME(x) __builtin_assume(x)
#elif CZ_HAS_ATTRIBUTE(assume)
	#define CZ_ASSUME(x) __attribute__ (( assume(x) ))
#elif defined(_MSC_VER)
	#define CZ_ASSUME(x) __assume(x)
#else
	#define CZ_ASSUME(x)
#endif

#if CZ_HAS_BUILTIN(expect)
	#define CZ_EXPECT(x)   ( __builtin_expect((x) ? 1 : 0, 1) )
	#define CZ_NOEXPECT(x) ( __builtin_expect((x) ? 1 : 0, 0) )
#else
	#define CZ_EXPECT(x)   (x)
	#define CZ_NOEXPECT(x) (x)
#endif

#if CZ_HAS_ATTRIBUTE(hot)
	#define CZ_HOT __attribute__ (( hot ))
#else
	#define CZ_HOT
#endif

#if CZ_HAS_ATTRIBUTE(cold)
	#define CZ_COLD __attribute__ (( cold ))
#else
	#define CZ_COLD
#endif

#if CZ_HAS_ATTRIBUTE(const)
	#define CZ_CONST __attribute__ (( const ))
#elif defined(_MSC_VER)
	#define CZ_PURE __declspec(noalias)
#else
	#define CZ_CONST
#endif

#if CZ_HAS_ATTRIBUTE(pure)
	#define CZ_PURE __attribute__ (( pure ))
#elif defined(_MSC_VER)
	#define CZ_PURE __declspec(noalias)
#else
	#define CZ_PURE
#endif

#if CZ_HAS_ATTRIBUTE(reproducible)
	#define CZ_REPRODUCIBLE __attribute__ (( reproducible ))
#elif defined(_MSC_VER)
	#define CZ_PURE __declspec(noalias)
#else
	#define CZ_REPRODUCIBLE
#endif

#if CZ_HAS_ATTRIBUTE(unsequenced)
	#define CZ_UNSEQUENCED __attribute__ (( unsequenced ))
#elif defined(_MSC_VER)
	#define CZ_PURE __declspec(noalias)
#else
	#define CZ_UNSEQUENCED
#endif

#if CZ_HAS_ATTRIBUTE(malloc)
	#define CZ_MALLOC __attribute__ (( malloc ))
#elif defined(_MSC_VER)
	#define CZ_MALLOC __declspec(allocator)
#else
	#define CZ_MALLOC
#endif

#if CZ_HAS_ATTRIBUTE(malloc) && !defined(__clang__)
	#define CZ_FREE(func, arg) __attribute__ (( malloc(func, arg) ))
#else
	#define CZ_FREE(func, arg)
#endif

#if CZ_HAS_ATTRIBUTE(format)
	#ifdef __clang__
		#define CZ_PRINTF(fmt, args) __attribute__ (( format(printf, fmt, args) ))
		#define CZ_SCANF(fmt, args)  __attribute__ (( format(scanf, fmt, args) ))
	#else
		#define CZ_PRINTF(fmt, args) __attribute__ (( format(gnu_printf, fmt, args) ))
		#define CZ_SCANF(fmt, args)  __attribute__ (( format(gnu_scanf, fmt, args) ))
	#endif
#else
	#define CZ_PRINTF(fmt, args)
	#define CZ_SCANF(fmt, args)
#endif

#if CZ_HAS_ATTRIBUTE(nonnull)
	#define CZ_NONNULL_ARG(...) __attribute__ (( nonnull(__VA_ARGS__) ))
	#define CZ_NONNULL_ARGS     __attribute__ (( nonnull ))
#else
	#define CZ_NONNULL_ARG(...)
	#define CZ_NONNULL_ARGS
#endif

#if CZ_HAS_ATTRIBUTE(nonnull_if_nonzero)
	#define CZ_NONNULL_NONZERO_ARG(ptr, arg)         __attribute__ (( nonnull_if_nonzero(ptr, arg) ))
	#define CZ_NONNULL_NONZERO_ARGS(ptr, arg1, arg2) __attribute__ (( nonnull_if_nonzero(ptr, arg1, arg2) ))
#else
	#define CZ_NONNULL_NONZERO_ARG(ptr, arg)
	#define CZ_NONNULL_NONZERO_ARGS(ptr, arg1, arg2)
#endif

#if CZ_HAS_ATTRIBUTE(alloc_size)
	#define CZ_ALLOC_ARG(arg)         __attribute__ (( alloc_size(arg) ))
	#define CZ_ALLOC_ARGS(arg1, arg2) __attribute__ (( alloc_size(arg1, arg2) ))
#else
	#define CZ_ALLOC_ARG(arg)
	#define CZ_ALLOC_ARGS(arg1, arg2)
#endif

#if CZ_HAS_ATTRIBUTE(alloc_align)
	#define CZ_ALIGN_ARG(arg) __attribute__ (( alloc_align(arg) ))
#else
	#define CZ_ALIGN_ARG(arg)
#endif

#if CZ_HAS_ATTRIBUTE(null_terminated_string_arg)
	#define CZ_NULLTERM_ARG(arg) __attribute__ (( null_terminated_string_arg(arg) ))
#else
	#define CZ_NULLTERM_ARG(arg)
#endif

#if CZ_HAS_ATTRIBUTE(access)
	#define CZ_NO_ACCESS(arg) __attribute__ (( access(none, arg) ))
	#define CZ_RE_ACCESS(arg) __attribute__ (( access(read_only, arg) ))
	#define CZ_WR_ACCESS(arg) __attribute__ (( access(write_only, arg) ))
	#define CZ_RW_ACCESS(arg) __attribute__ (( access(read_write, arg) ))
#else
	#define CZ_NO_ACCESS(arg)
	#define CZ_RE_ACCESS(arg)
	#define CZ_WR_ACCESS(arg)
	#define CZ_RW_ACCESS(arg)
#endif

#if CZ_HAS_ATTRIBUTE(returns_nonnull)
	#define CZ_NONNULL_RET __attribute__ (( returns_nonnull ))
#else
	#define CZ_NONNULL_RET
#endif

#if CZ_HAS_ATTRIBUTE(warn_unused_result)
	#define CZ_USE_RET __attribute__ (( warn_unused_result ))
#else
	#define CZ_USE_RET
#endif

#if CZ_HAS_ATTRIBUTE(counted_by)
	#define CZ_COUNTBY(x) __attribute__ (( counted_by(x) ))
#else
	#define CZ_COUNTBY(x)
#endif

// Check support for nonstandard (in C17) extensions and features

#if CZ_HAS_INCLUDE(stdcountof.h)
	#include <stdcountof.h>
	#define CZ_COUNTOF(a) countof(a)
#else
	#define CZ_COUNTOF(a) ( sizeof(a) / sizeof(*(a)) )
#endif

// Check support for nonstandard predefined macros

#ifdef __FILE_NAME__
	#define CZ_FILENAME __FILE_NAME__
#else
	#define CZ_FILENAME __FILE__
#endif

// ANSI escape codes regarding Select Graphic Rendition (SGR)

#define CZ_SGR_RESET      "\033[m"
#define CZ_SGR_BOLD       "\033[1m"
#define CZ_SGR_FAINT      "\033[2m"
#define CZ_SGR_ITALIC     "\033[3m"
#define CZ_SGR_UNDERLINE  "\033[4m"
#define CZ_SGR_SLOW_BLINK "\033[5m"
#define CZ_SGR_FAST_BLINK "\033[6m"
#define CZ_SGR_INVERT     "\033[7m"
#define CZ_SGR_CONCEAL    "\033[8m"
#define CZ_SGR_STRIKE     "\033[9m"

#define CZ_SGR_FG_BLACK   "\033[30m"
#define CZ_SGR_FG_RED     "\033[31m"
#define CZ_SGR_FG_GREEN   "\033[32m"
#define CZ_SGR_FG_YELLOW  "\033[33m"
#define CZ_SGR_FG_BLUE    "\033[34m"
#define CZ_SGR_FG_MAGENTA "\033[35m"
#define CZ_SGR_FG_CYAN    "\033[36m"
#define CZ_SGR_FG_WHITE   "\033[37m"
#define CZ_SGR_FG_DEFAULT "\033[39m"

#define CZ_SGR_FG_8BIT(n)        "\033[38;5;" #n "m"
#define CZ_SGR_FG_24BIT(r, g, b) "\033[38;2;" #r ";" #g ";" #b "m"

#define CZ_SGR_BG_BLACK   "\033[40m"
#define CZ_SGR_BG_RED     "\033[41m"
#define CZ_SGR_BG_GREEN   "\033[42m"
#define CZ_SGR_BG_YELLOW  "\033[43m"
#define CZ_SGR_BG_BLUE    "\033[44m"
#define CZ_SGR_BG_MAGENTA "\033[45m"
#define CZ_SGR_BG_CYAN    "\033[46m"
#define CZ_SGR_BG_WHITE   "\033[47m"
#define CZ_SGR_BG_DEFAULT "\033[49m"

#define CZ_SGR_BG_8BIT(n)        "\033[48;5;" #n "m"
#define CZ_SGR_BG_24BIT(r, g, b) "\033[48;2;" #r ";" #g ";" #b "m"

// Useful values

#define CZ_KIB_SIZE ( UINT64_C(1) << 10 )
#define CZ_MIB_SIZE ( UINT64_C(1) << 20 )
#define CZ_GIB_SIZE ( UINT64_C(1) << 30 )
#define CZ_TIB_SIZE ( UINT64_C(1) << 40 )
#define CZ_PIB_SIZE ( UINT64_C(1) << 50 )
#define CZ_EIB_SIZE ( UINT64_C(1) << 60 )

#define CZ_MS_PER_CLOCK ( 1000.0 / CLOCKS_PER_SEC )

#define CZ_DEBUG_LOG_NAME      "debug.log"
#define CZ_PIPELINE_CACHE_NAME "pipeline_cache.bin"
#define CZ_PROGRESS_FILE_NAME  "position.txt"

// Helper macros

#define CZ_NEWLINE() putchar('\n')

#define CZ_UINT128_UPPER(x) ( (uint64_t) ((x) >> 64) )
#define CZ_UINT128_LOWER(x) ( (uint64_t) ((x) & ~UINT64_C(0)) )

#define CZ_UINT128(upper, lower) ( (unsigned __int128) (upper) << 64 | (unsigned __int128) (lower) )

#define CZ_PNEXT_ADD(p, s) \
	do {                   \
		*(p) = &(s);       \
		(p) = &(s).pNext;  \
	}                      \
	while (0)

// Enumerations

enum CzEndianness
{
	CZ_ENDIANNESS_BIG,
	CZ_ENDIANNESS_LITTLE,
};

enum CzOutputLevel
{
	CZ_OUTPUT_LEVEL_SILENT,
	CZ_OUTPUT_LEVEL_QUIET,
	CZ_OUTPUT_LEVEL_DEFAULT,
	CZ_OUTPUT_LEVEL_VERBOSE,
};

enum CzColourLevel
{
	CZ_COLOUR_LEVEL_NONE,
	CZ_COLOUR_LEVEL_TTY,
	CZ_COLOUR_LEVEL_ALL,
};

enum CzResult
{
	CZ_RESULT_SUCCESS,
	CZ_RESULT_INTERNAL_ERROR,
	CZ_RESULT_BAD_ACCESS,
	CZ_RESULT_BAD_ADDRESS,
	CZ_RESULT_BAD_ALIGNMENT,
	CZ_RESULT_BAD_FILE,
	CZ_RESULT_BAD_OFFSET,
	CZ_RESULT_BAD_PATH,
	CZ_RESULT_BAD_SIZE,
	CZ_RESULT_NO_FILE,
	CZ_RESULT_NO_MEMORY,
};
