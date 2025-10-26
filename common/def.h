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

#if defined(__STDC_NO_ATOMICS__)
	#error "Compiler must support C11 atomic types and operations"
#endif

#if !defined(__SIZEOF_INT128__)
	#error "Compiler must support GNU C 128-bit integer extension"
#elif __SIZEOF_INT128__ != 16
	#error "Target must support 128-bit integers"
#endif

// Check support for attributes and builtins

#if !defined(CZ_HAS_ATTRIBUTE)
	#if defined(__has_attribute)
		#define CZ_HAS_ATTRIBUTE(x) __has_attribute(x)
	#else
		#define CZ_HAS_ATTRIBUTE(x) 0
	#endif
#endif

#if !defined(CZ_HAS_BUILTIN)
	#if defined(__has_builtin)
		#define CZ_HAS_BUILTIN(x) __has_builtin(__builtin_##x)
	#else
		#define CZ_HAS_BUILTIN(x) 0
	#endif
#endif

#if !defined(CZ_HAS_INCLUDE)
	#if defined(__has_include)
		#define CZ_HAS_INCLUDE(x) __has_include(<x>)
	#else
		#define CZ_HAS_INCLUDE(x) 0
	#endif
#endif

#if !defined(CZ_ASSUME)
	#if CZ_HAS_BUILTIN(assume)
		#define CZ_ASSUME(x) __builtin_assume(x)
	#elif CZ_HAS_ATTRIBUTE(assume)
		#define CZ_ASSUME(x) __attribute__ (( assume(x) ))
	#elif defined(_MSC_VER)
		#define CZ_ASSUME(x) __assume(x)
	#else
		#define CZ_ASSUME(x)
	#endif
#endif

#if !defined(CZ_EXPECT)
	#if CZ_HAS_BUILTIN(expect)
		#define CZ_EXPECT(x) ( __builtin_expect((x) ? 1 : 0, 1) )
	#else
		#define CZ_EXPECT(x) (x)
	#endif
#endif

#if !defined(CZ_NOEXPECT)
	#if CZ_HAS_BUILTIN(expect)
		#define CZ_NOEXPECT(x) ( __builtin_expect((x) ? 1 : 0, 0) )
	#else
		#define CZ_NOEXPECT(x) (x)
	#endif
#endif

#if !defined(CZ_HOT)
	#if CZ_HAS_ATTRIBUTE(hot)
		#define CZ_HOT __attribute__ (( hot ))
	#else
		#define CZ_HOT
	#endif
#endif

#if !defined(CZ_COLD)
	#if CZ_HAS_ATTRIBUTE(cold)
		#define CZ_COLD __attribute__ (( cold ))
	#else
		#define CZ_COLD
	#endif
#endif

#if !defined(CZ_CONST)
	#if CZ_HAS_ATTRIBUTE(const)
		#define CZ_CONST __attribute__ (( const ))
	#elif defined(_MSC_VER)
		#define CZ_CONST __declspec(noalias)
	#else
		#define CZ_CONST
	#endif
#endif

#if !defined(CZ_PURE)
	#if CZ_HAS_ATTRIBUTE(pure)
		#define CZ_PURE __attribute__ (( pure ))
	#elif defined(_MSC_VER)
		#define CZ_PURE __declspec(noalias)
	#else
		#define CZ_PURE
	#endif
#endif

#if !defined(CZ_REPRODUCIBLE)
	#if CZ_HAS_ATTRIBUTE(reproducible)
		#define CZ_REPRODUCIBLE __attribute__ (( reproducible ))
	#elif defined(_MSC_VER)
		#define CZ_REPRODUCIBLE __declspec(noalias)
	#else
		#define CZ_REPRODUCIBLE
	#endif
#endif

#if !defined(CZ_UNSEQUENCED)
	#if CZ_HAS_ATTRIBUTE(unsequenced)
		#define CZ_UNSEQUENCED __attribute__ (( unsequenced ))
	#elif defined(_MSC_VER)
		#define CZ_UNSEQUENCED __declspec(noalias)
	#else
		#define CZ_UNSEQUENCED
	#endif
#endif

#if !defined(CZ_MALLOC)
	#if CZ_HAS_ATTRIBUTE(malloc)
		#define CZ_MALLOC __attribute__ (( malloc ))
	#elif defined(_MSC_VER)
		#define CZ_MALLOC __declspec(allocator)
	#else
		#define CZ_MALLOC
	#endif
#endif

#if !defined(CZ_FREE)
	#if CZ_HAS_ATTRIBUTE(malloc) && !defined(__clang__)
		#define CZ_FREE(func, arg) __attribute__ (( malloc(func, arg) ))
	#else
		#define CZ_FREE(func, arg)
	#endif
#endif

#if !defined(CZ_PRINTF)
	#if CZ_HAS_ATTRIBUTE(format) && defined(__clang__)
		#define CZ_PRINTF(fmt, args) __attribute__ (( format(printf, fmt, args) ))
	#elif CZ_HAS_ATTRIBUTE(format)
		#define CZ_PRINTF(fmt, args) __attribute__ (( format(gnu_printf, fmt, args) ))
	#else
		#define CZ_PRINTF(fmt, args)
	#endif
#endif

#if !defined(CZ_SCANF)
	#if CZ_HAS_ATTRIBUTE(format) && defined(__clang__)
		#define CZ_SCANF(fmt, args) __attribute__ (( format(scanf, fmt, args) ))
	#elif CZ_HAS_ATTRIBUTE(format)
		#define CZ_SCANF(fmt, args) __attribute__ (( format(gnu_scanf, fmt, args) ))
	#else
		#define CZ_SCANF(fmt, args)
	#endif
#endif

#if !defined(CZ_NONNULL_ARGS)
	#if CZ_HAS_ATTRIBUTE(nonnull)
		#define CZ_NONNULL_ARGS(...) __attribute__ (( nonnull __VA_OPT__((__VA_ARGS__)) ))
	#else
		#define CZ_NONNULL_ARGS(...)
	#endif
#endif

#if !defined(CZ_NONNULL_NONZERO_ARGS)
	#if CZ_HAS_ATTRIBUTE(nonnull_if_nonzero)
		#define CZ_NONNULL_NONZERO_ARGS(ptr, ...) __attribute__ (( nonnull_if_nonzero(ptr, __VA_ARGS__) ))
	#else
		#define CZ_NONNULL_NONZERO_ARGS(ptr, ...)
	#endif
#endif

#if !defined(CZ_ALLOC_ARGS)
	#if CZ_HAS_ATTRIBUTE(alloc_size)
		#define CZ_ALLOC_ARGS(...) __attribute__ (( alloc_size(__VA_ARGS__) ))
	#else
		#define CZ_ALLOC_ARGS(...)
	#endif
#endif

#if !defined(CZ_ALIGN_ARG)
	#if CZ_HAS_ATTRIBUTE(alloc_align)
		#define CZ_ALIGN_ARG(arg) __attribute__ (( alloc_align(arg) ))
	#else
		#define CZ_ALIGN_ARG(arg)
	#endif
#endif

#if !defined(CZ_NULLTERM_ARG)
	#if CZ_HAS_ATTRIBUTE(null_terminated_string_arg)
		#define CZ_NULLTERM_ARG(arg) __attribute__ (( null_terminated_string_arg(arg) ))
	#else
		#define CZ_NULLTERM_ARG(arg)
	#endif
#endif

#if !defined(CZ_NO_ACCESS)
	#if CZ_HAS_ATTRIBUTE(access)
		#define CZ_NO_ACCESS(...) __attribute__ (( access(none, __VA_ARGS__) ))
	#else
		#define CZ_NO_ACCESS(...)
	#endif
#endif

#if !defined(CZ_RD_ACCESS)
	#if CZ_HAS_ATTRIBUTE(access)
		#define CZ_RD_ACCESS(...) __attribute__ (( access(read_only, __VA_ARGS__) ))
	#else
		#define CZ_RD_ACCESS(...)
	#endif
#endif

#if !defined(CZ_WR_ACCESS)
	#if CZ_HAS_ATTRIBUTE(access)
		#define CZ_WR_ACCESS(...) __attribute__ (( access(write_only, __VA_ARGS__) ))
	#else
		#define CZ_WR_ACCESS(...)
	#endif
#endif

#if !defined(CZ_RW_ACCESS)
	#if CZ_HAS_ATTRIBUTE(access)
		#define CZ_RW_ACCESS(...) __attribute__ (( access(read_write, __VA_ARGS__) ))
	#else
		#define CZ_RW_ACCESS(...)
	#endif
#endif

#if !defined(CZ_NONNULL_RET)
	#if CZ_HAS_ATTRIBUTE(returns_nonnull)
		#define CZ_NONNULL_RET __attribute__ (( returns_nonnull ))
	#else
		#define CZ_NONNULL_RET
	#endif
#endif

#if !defined(CZ_USE_RET)
	#if CZ_HAS_ATTRIBUTE(warn_unused_result)
		#define CZ_USE_RET __attribute__ (( warn_unused_result ))
	#else
		#define CZ_USE_RET
	#endif
#endif

#if !defined(CZ_COUNTBY)
	#if CZ_HAS_ATTRIBUTE(counted_by)
		#define CZ_COUNTBY(x) __attribute__ (( counted_by(x) ))
	#else
		#define CZ_COUNTBY(x)
	#endif
#endif

// Check support for extensions and features

#if !defined(CZ_COUNTOF)
	#if CZ_HAS_INCLUDE(stdcountof.h)
		#define CZ_COUNTOF(a) countof(a)
	#else
		#define CZ_COUNTOF(a) ( sizeof(a) / sizeof(*(a)) )
	#endif
#endif

// Check support for predefined macros

#if !defined(CZ_FILENAME)
	#if defined(__FILE_NAME__)
		#define CZ_FILENAME __FILE_NAME__
	#else
		#define CZ_FILENAME __FILE__
	#endif
#endif

#if !defined(CZ_DARWIN)
	#if defined(__APPLE__) && defined(__MACH__)
		#define CZ_DARWIN 1
	#else
		#define CZ_DARWIN 0
	#endif
#endif

#if !defined(CZ_IOS)
	#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
		#define CZ_IOS 1
	#else
		#define CZ_IOS 0
	#endif
#endif

#if !defined(CZ_MACOS)
	#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
		#define CZ_MACOS 1
	#else
		#define CZ_MACOS 0
	#endif
#endif

#if !defined(CZ_UNIX)
	#if defined(__unix__) || defined(__unix) || defined(unix)
		#define CZ_UNIX 1
	#else
		#define CZ_UNIX 0
	#endif
#endif

#if !defined(CZ_LINUX)
	#if defined(__linux__)
		#define CZ_LINUX 1
	#else
		#define CZ_LINUX 0
	#endif
#endif

#if !defined(CZ_GNU_LINUX)
	#if defined(__gnu_linux__)
		#define CZ_GNU_LINUX 1
	#else
		#define CZ_GNU_LINUX 0
	#endif
#endif

#if !defined(CZ_CYGWIN)
	#if defined(__CYGWIN__)
		#define CZ_CYGWIN 1
	#else
		#define CZ_CYGWIN 0
	#endif
#endif

#if !defined(CZ_MINGW)
	#if defined(__MINGW32__)
		#define CZ_MINGW 1
	#else
		#define CZ_MINGW 0
	#endif
#endif

#if !defined(CZ_WINDOWS)
	#if defined(_WIN32)
		#define CZ_WINDOWS 1
	#else
		#define CZ_WINDOWS 0
	#endif
#endif

#if !defined(CZ_POSIX_VERSION)
	#if defined(_POSIX_VERSION)
		#define CZ_POSIX_VERSION _POSIX_VERSION
	#else
		#define CZ_POSIX_VERSION (-1)
	#endif
#endif

#if !defined(CZ_POSIX_ADVISORY_INFO)
	#if defined(_POSIX_ADVISORY_INFO)
		#define CZ_POSIX_ADVISORY_INFO _POSIX_ADVISORY_INFO
	#else
		#define CZ_POSIX_ADVISORY_INFO (-1)
	#endif
#endif

#if !defined(CZ_POSIX_ASYNCHRONOUS_IO)
	#if defined(_POSIX_ASYNCHRONOUS_IO)
		#define CZ_POSIX_ASYNCHRONOUS_IO _POSIX_ASYNCHRONOUS_IO
	#else
		#define CZ_POSIX_ASYNCHRONOUS_IO (-1)
	#endif
#endif

#if !defined(CZ_POSIX_BARRIERS)
	#if defined(_POSIX_BARRIERS)
		#define CZ_POSIX_BARRIERS _POSIX_BARRIERS
	#else
		#define CZ_POSIX_BARRIERS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_CLOCK_SELECTION)
	#if defined(_POSIX_CLOCK_SELECTION)
		#define CZ_POSIX_CLOCK_SELECTION _POSIX_CLOCK_SELECTION
	#else
		#define CZ_POSIX_CLOCK_SELECTION (-1)
	#endif
#endif

#if !defined(CZ_POSIX_CPUTIME)
	#if defined(_POSIX_CPUTIME)
		#define CZ_POSIX_CPUTIME _POSIX_CPUTIME
	#else
		#define CZ_POSIX_CPUTIME (-1)
	#endif
#endif

#if !defined(CZ_POSIX_FSYNC)
	#if defined(_POSIX_FSYNC)
		#define CZ_POSIX_FSYNC _POSIX_FSYNC
	#else
		#define CZ_POSIX_FSYNC (-1)
	#endif
#endif

#if !defined(CZ_POSIX_IPV6)
	#if defined(_POSIX_IPV6)
		#define CZ_POSIX_IPV6 _POSIX_IPV6
	#else
		#define CZ_POSIX_IPV6 (-1)
	#endif
#endif

#if !defined(CZ_POSIX_JOB_CONTROL)
	#if defined(_POSIX_JOB_CONTROL)
		#define CZ_POSIX_JOB_CONTROL _POSIX_JOB_CONTROL
	#else
		#define CZ_POSIX_JOB_CONTROL (-1)
	#endif
#endif

#if !defined(CZ_POSIX_MAPPED_FILES)
	#if defined(_POSIX_MAPPED_FILES)
		#define CZ_POSIX_MAPPED_FILES _POSIX_MAPPED_FILES
	#else
		#define CZ_POSIX_MAPPED_FILES (-1)
	#endif
#endif

#if !defined(CZ_POSIX_MEMLOCK)
	#if defined(_POSIX_MEMLOCK)
		#define CZ_POSIX_MEMLOCK _POSIX_MEMLOCK
	#else
		#define CZ_POSIX_MEMLOCK (-1)
	#endif
#endif

#if !defined(CZ_POSIX_MEMLOCK_RANGE)
	#if defined(_POSIX_MEMLOCK_RANGE)
		#define CZ_POSIX_MEMLOCK_RANGE _POSIX_MEMLOCK_RANGE
	#else
		#define CZ_POSIX_MEMLOCK_RANGE (-1)
	#endif
#endif

#if !defined(CZ_POSIX_MEMORY_PROTECTION)
	#if defined(_POSIX_MEMORY_PROTECTION)
		#define CZ_POSIX_MEMORY_PROTECTION _POSIX_MEMORY_PROTECTION
	#else
		#define CZ_POSIX_MEMORY_PROTECTION (-1)
	#endif
#endif

#if !defined(CZ_POSIX_MESSAGE_PASSING)
	#if defined(_POSIX_MESSAGE_PASSING)
		#define CZ_POSIX_MESSAGE_PASSING _POSIX_MESSAGE_PASSING
	#else
		#define CZ_POSIX_MESSAGE_PASSING (-1)
	#endif
#endif

#if !defined(CZ_POSIX_MONOTONIC_CLOCK)
	#if defined(_POSIX_MONOTONIC_CLOCK)
		#define CZ_POSIX_MONOTONIC_CLOCK _POSIX_MONOTONIC_CLOCK
	#else
		#define CZ_POSIX_MONOTONIC_CLOCK (-1)
	#endif
#endif

#if !defined(CZ_POSIX_NO_TRUNC)
	#if defined(_POSIX_NO_TRUNC)
		#define CZ_POSIX_NO_TRUNC _POSIX_NO_TRUNC
	#else
		#define CZ_POSIX_NO_TRUNC (-1)
	#endif
#endif

#if !defined(CZ_POSIX_PRIORITIZED_IO)
	#if defined(_POSIX_PRIORITIZED_IO)
		#define CZ_POSIX_PRIORITIZED_IO _POSIX_PRIORITIZED_IO
	#else
		#define CZ_POSIX_PRIORITIZED_IO (-1)
	#endif
#endif

#if !defined(CZ_POSIX_PRIORITY_SCHEDULING)
	#if defined(_POSIX_PRIORITY_SCHEDULING)
		#define CZ_POSIX_PRIORITY_SCHEDULING _POSIX_PRIORITY_SCHEDULING
	#else
		#define CZ_POSIX_PRIORITY_SCHEDULING (-1)
	#endif
#endif

#if !defined(CZ_POSIX_RAW_SOCKETS)
	#if defined(_POSIX_RAW_SOCKETS)
		#define CZ_POSIX_RAW_SOCKETS _POSIX_RAW_SOCKETS
	#else
		#define CZ_POSIX_RAW_SOCKETS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_READER_WRITER_LOCKS)
	#if defined(_POSIX_READER_WRITER_LOCKS)
		#define CZ_POSIX_READER_WRITER_LOCKS _POSIX_READER_WRITER_LOCKS
	#else
		#define CZ_POSIX_READER_WRITER_LOCKS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_REALTIME_SIGNALS)
	#if defined(_POSIX_REALTIME_SIGNALS)
		#define CZ_POSIX_REALTIME_SIGNALS _POSIX_REALTIME_SIGNALS
	#else
		#define CZ_POSIX_REALTIME_SIGNALS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_REGEXP)
	#if defined(_POSIX_REGEXP)
		#define CZ_POSIX_REGEXP _POSIX_REGEXP
	#else
		#define CZ_POSIX_REGEXP (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SAVED_IDS)
	#if defined(_POSIX_SAVED_IDS)
		#define CZ_POSIX_SAVED_IDS _POSIX_SAVED_IDS
	#else
		#define CZ_POSIX_SAVED_IDS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SEMAPHORES)
	#if defined(_POSIX_SEMAPHORES)
		#define CZ_POSIX_SEMAPHORES _POSIX_SEMAPHORES
	#else
		#define CZ_POSIX_SEMAPHORES (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SHARED_MEMORY_OBJECTS)
	#if defined(_POSIX_SHARED_MEMORY_OBJECTS)
		#define CZ_POSIX_SHARED_MEMORY_OBJECTS _POSIX_SHARED_MEMORY_OBJECTS
	#else
		#define CZ_POSIX_SHARED_MEMORY_OBJECTS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SHELL)
	#if defined(_POSIX_SHELL)
		#define CZ_POSIX_SHELL _POSIX_SHELL
	#else
		#define CZ_POSIX_SHELL (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SPAWN)
	#if defined(_POSIX_SPAWN)
		#define CZ_POSIX_SPAWN _POSIX_SPAWN
	#else
		#define CZ_POSIX_SPAWN (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SPIN_LOCKS)
	#if defined(_POSIX_SPIN_LOCKS)
		#define CZ_POSIX_SPIN_LOCKS _POSIX_SPIN_LOCKS
	#else
		#define CZ_POSIX_SPIN_LOCKS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SPORADIC_SERVER)
	#if defined(_POSIX_SPORADIC_SERVER)
		#define CZ_POSIX_SPORADIC_SERVER _POSIX_SPORADIC_SERVER
	#else
		#define CZ_POSIX_SPORADIC_SERVER (-1)
	#endif
#endif

#if !defined(CZ_POSIX_SYNCHRONIZED_IO)
	#if defined(_POSIX_SYNCHRONIZED_IO)
		#define CZ_POSIX_SYNCHRONIZED_IO _POSIX_SYNCHRONIZED_IO
	#else
		#define CZ_POSIX_SYNCHRONIZED_IO (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_ATTR_STACKADDR)
	#if defined(_POSIX_THREAD_ATTR_STACKADDR)
		#define CZ_POSIX_THREAD_ATTR_STACKADDR _POSIX_THREAD_ATTR_STACKADDR
	#else
		#define CZ_POSIX_THREAD_ATTR_STACKADDR (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_ATTR_STACKSIZE)
	#if defined(_POSIX_THREAD_ATTR_STACKSIZE)
		#define CZ_POSIX_THREAD_ATTR_STACKSIZE _POSIX_THREAD_ATTR_STACKSIZE
	#else
		#define CZ_POSIX_THREAD_ATTR_STACKSIZE (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_CPUTIME)
	#if defined(_POSIX_THREAD_CPUTIME)
		#define CZ_POSIX_THREAD_CPUTIME _POSIX_THREAD_CPUTIME
	#else
		#define CZ_POSIX_THREAD_CPUTIME (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_PRIO_INHERIT)
	#if defined(_POSIX_THREAD_PRIO_INHERIT)
		#define CZ_POSIX_THREAD_PRIO_INHERIT _POSIX_THREAD_PRIO_INHERIT
	#else
		#define CZ_POSIX_THREAD_PRIO_INHERIT (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_PRIO_PROTECT)
	#if defined(_POSIX_THREAD_PRIO_PROTECT)
		#define CZ_POSIX_THREAD_PRIO_PROTECT _POSIX_THREAD_PRIO_PROTECT
	#else
		#define CZ_POSIX_THREAD_PRIO_PROTECT (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_PRIORITY_SCHEDULING)
	#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
		#define CZ_POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_THREAD_PRIORITY_SCHEDULING
	#else
		#define CZ_POSIX_THREAD_PRIORITY_SCHEDULING (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_PROCESS_SHARED)
	#if defined(_POSIX_THREAD_PROCESS_SHARED)
		#define CZ_POSIX_THREAD_PROCESS_SHARED _POSIX_THREAD_PROCESS_SHARED
	#else
		#define CZ_POSIX_THREAD_PROCESS_SHARED (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_SAFE_FUNCTIONS)
	#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
		#define CZ_POSIX_THREAD_SAFE_FUNCTIONS _POSIX_THREAD_SAFE_FUNCTIONS
	#else
		#define CZ_POSIX_THREAD_SAFE_FUNCTIONS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREAD_SPORADIC_SERVER)
	#if defined(_POSIX_THREAD_SPORADIC_SERVER)
		#define CZ_POSIX_THREAD_SPORADIC_SERVER _POSIX_THREAD_SPORADIC_SERVER
	#else
		#define CZ_POSIX_THREAD_SPORADIC_SERVER (-1)
	#endif
#endif

#if !defined(CZ_POSIX_THREADS)
	#if defined(_POSIX_THREADS)
		#define CZ_POSIX_THREADS _POSIX_THREADS
	#else
		#define CZ_POSIX_THREADS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TIMEOUTS)
	#if defined(_POSIX_TIMEOUTS)
		#define CZ_POSIX_TIMEOUTS _POSIX_TIMEOUTS
	#else
		#define CZ_POSIX_TIMEOUTS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TIMERS)
	#if defined(_POSIX_TIMERS)
		#define CZ_POSIX_TIMERS _POSIX_TIMERS
	#else
		#define CZ_POSIX_TIMERS (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TRACE)
	#if defined(_POSIX_TRACE)
		#define CZ_POSIX_TRACE _POSIX_TRACE
	#else
		#define CZ_POSIX_TRACE (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TRACE_EVENT_FILTER)
	#if defined(_POSIX_TRACE_EVENT_FILTER)
		#define CZ_POSIX_TRACE_EVENT_FILTER _POSIX_TRACE_EVENT_FILTER
	#else
		#define CZ_POSIX_TRACE_EVENT_FILTER (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TRACE_INHERIT)
	#if defined(_POSIX_TRACE_INHERIT)
		#define CZ_POSIX_TRACE_INHERIT _POSIX_TRACE_INHERIT
	#else
		#define CZ_POSIX_TRACE_INHERIT (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TRACE_LOG)
	#if defined(_POSIX_TRACE_LOG)
		#define CZ_POSIX_TRACE_LOG _POSIX_TRACE_LOG
	#else
		#define CZ_POSIX_TRACE_LOG (-1)
	#endif
#endif

#if !defined(CZ_POSIX_TYPED_MEMORY_OBJECTS)
	#if defined(_POSIX_TYPED_MEMORY_OBJECTS)
		#define CZ_POSIX_TYPED_MEMORY_OBJECTS _POSIX_TYPED_MEMORY_OBJECTS
	#else
		#define CZ_POSIX_TYPED_MEMORY_OBJECTS (-1)
	#endif
#endif

#if !defined(CZ_XOPEN_VERSION)
	#if defined(_XOPEN_VERSION)
		#define CZ_XOPEN_VERSION _XOPEN_VERSION
	#else
		#define CZ_XOPEN_VERSION (-1)
	#endif
#endif

#if !defined(CZ_XOPEN_CRYPT)
	#if defined(_XOPEN_CRYPT)
		#define CZ_XOPEN_CRYPT _XOPEN_CRYPT
	#else
		#define CZ_XOPEN_CRYPT (-1)
	#endif
#endif

#if !defined (CZ_XOPEN_LEGACY)
	#if defined(_XOPEN_LEGACY)
		#define CZ_XOPEN_LEGACY _XOPEN_LEGACY
	#else
		#define CZ_XOPEN_LEGACY (-1)
	#endif
#endif

#if !defined(CZ_XOPEN_REALTIME)
	#if defined(_XOPEN_REALTIME)
		#define CZ_XOPEN_REALTIME _XOPEN_REALTIME
	#else
		#define CZ_XOPEN_REALTIME (-1)
	#endif
#endif

#if !defined(CZ_XOPEN_REALTIME_THREADS)
	#if defined(_XOPEN_REALTIME_THREADS)
		#define CZ_XOPEN_REALTIME_THREADS _XOPEN_REALTIME_THREADS
	#else
		#define CZ_XOPEN_REALTIME_THREADS (-1)
	#endif
#endif

#if !defined(CZ_XOPEN_STREAMS)
	#if defined(_XOPEN_STREAMS)
		#define CZ_XOPEN_STREAMS _XOPEN_STREAMS
	#else
		#define CZ_XOPEN_STREAMS (-1)
	#endif
#endif

#if !defined(CZ_XOPEN_UNIX)
	#if defined(_XOPEN_UNIX)
		#define CZ_XOPEN_UNIX _XOPEN_UNIX
	#else
		#define CZ_XOPEN_UNIX (-1)
	#endif
#endif

#if !defined(CZ_LINUX_ADVISE_SYSCALLS)
	#if defined(CONFIG_ADVISE_SYSCALLS)
		#define CZ_LINUX_ADVISE_SYSCALLS 1
	#else
		#define CZ_LINUX_ADVISE_SYSCALLS 0
	#endif
#endif

#if !defined(CZ_LINUX_KSM)
	#if defined(CONFIG_KSM)
		#define CZ_LINUX_KSM 1
	#else
		#define CZ_LINUX_KSM 0
	#endif
#endif

#if !defined(CZ_LINUX_MEMORY_FAILURE)
	#if defined(CONFIG_MEMORY_FAILURE)
		#define CZ_LINUX_MEMORY_FAILURE 1
	#else
		#define CZ_LINUX_MEMORY_FAILURE 0
	#endif
#endif

#if !defined(CZ_LINUX_TRANSPARENT_HUGEPAGE)
	#if defined(CONFIG_TRANSPARENT_HUGEPAGE)
		#define CZ_LINUX_TRANSPARENT_HUGEPAGE 1
	#else
		#define CZ_LINUX_TRANSPARENT_HUGEPAGE 0
	#endif
#endif

// Standard C versions

#define CZ_STDC_1995 199409L // C95 - ISO/IEC 9899:1990/AMD1:1995
#define CZ_STDC_1999 199901L // C99 - ISO/IEC 9899:1999
#define CZ_STDC_2011 201112L // C11 - ISO/IEC 9899:2011
#define CZ_STDC_2017 201710L // C17 - ISO/IEC 9899:2018
#define CZ_STDC_2023 202311L // C23 - ISO/IEC 9899:2024

// Standard C++ versions

#define CZ_STDCXX_1998 199711L // C++98 - ISO/IEC 14882:1998
#define CZ_STDCXX_2003 199711L // C++03 - ISO/IEC 14882:2003
#define CZ_STDCXX_2011 201103L // C++11 - ISO/IEC 14882:2011
#define CZ_STDCXX_2014 201402L // C++14 - ISO/IEC 14882:2014
#define CZ_STDCXX_2017 201703L // C++17 - ISO/IEC 14882:2017
#define CZ_STDCXX_2020 202002L // C++20 - ISO/IEC 14882:2020
#define CZ_STDCXX_2023 202302L // C++23 - ISO/IEC 14882:2024

// POSIX.1 versions

#define CZ_POSIX_1988 198808L // POSIX.1-1988 - IEEE 1003.1-1988
#define CZ_POSIX_1990 199009L // POSIX.1-1990 - IEEE 1003.1-1990 - ISO/IEC 9945:1990
#define CZ_POSIX_1996 199506L // POSIX.1-1996 - IEEE 1003.1-1996 - ISO/IEC 9945:1996
#define CZ_POSIX_2001 200112L // POSIX.1-2001 - IEEE 1003.1-2001 - ISO/IEC 9945:2002
#define CZ_POSIX_2008 200809L // POSIX.1-2008 - IEEE 1003.1-2008 - ISO/IEC/IEEE 9945:2009
#define CZ_POSIX_2017 200809L // POSIX.1-2017 - IEEE 1003.1-2017
#define CZ_POSIX_2024 202405L // POSIX.1-2024 - IEEE 1003.1-2024

// X/Open Portability Guide (XPG) versions

#define CZ_XPG_1985 1 // XPG
#define CZ_XPG_1987 2 // XPG2
#define CZ_XPG_1989 3 // XPG3
#define CZ_XPG_1992 4 // XPG4
#define CZ_XPG_1994 4 // XPG4v2

// Single UNIX Specification (SUS...) versions

#define CZ_SUS_1994 4   // SUS
#define CZ_SUS_1997 500 // SUSv2
#define CZ_SUS_2001 600 // SUSv3
#define CZ_SUS_2008 700 // SUSv4
#define CZ_SUS_2024 800 // SUSv5

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
	CZ_RESULT_BAD_STREAM,
	CZ_RESULT_IN_USE,
	CZ_RESULT_INTERRUPT,
	CZ_RESULT_NO_CONNECTION,
	CZ_RESULT_NO_FILE,
	CZ_RESULT_NO_MEMORY,
	CZ_RESULT_NO_OPEN,
	CZ_RESULT_NO_QUOTA,
	CZ_RESULT_NO_SUPPORT,
	CZ_RESULT_TIMEOUT,
};
