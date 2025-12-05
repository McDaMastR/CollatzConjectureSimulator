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

// Helper macros for versioning

#define CZ_MAKE_VERSION(major, minor, .../* patch */) ( ((major) << 48) | ((minor) << 32) __VA_OPT__(| (__VA_ARGS__)) )

// Standard C versions

#define CZ_STDC_1989 1       // C89 - ISO/IEC 9899:1990
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

// POSIX versions

#define CZ_POSIX_1988 198808L // POSIX.1-1988 - IEEE Std 1003.1-1988
#define CZ_POSIX_1990 199009L // POSIX.1-1990 - IEEE Std 1003.1-1990  - ISO/IEC 9945-1:1990
#define CZ_POSIX_1992 199209L // POSIX.2-1992 - IEEE Std 1003.2-1992  - ISO/IEC 9945-2:1993
#define CZ_POSIX_1993 199309L // POSIX.1b     - IEEE Std 1003.1b-1993
#define CZ_POSIX_1995 199506L // POSIX.1c     - IEEE Std 1003.1c-1995
#define CZ_POSIX_1996 199506L // POSIX.1-1996 - IEEE Std 1003.1-1996  - ISO/IEC 9945-1:1996
#define CZ_POSIX_2001 200112L // POSIX.1-2001 - IEEE Std 1003.1-2001  - ISO/IEC 9945:2002
#define CZ_POSIX_2008 200809L // POSIX.1-2008 - IEEE Std 1003.1-2008  - ISO/IEC/IEEE 9945:2009
#define CZ_POSIX_2017 200809L // POSIX.1-2017 - IEEE Std 1003.1-2017
#define CZ_POSIX_2024 202405L // POSIX.1-2024 - IEEE Std 1003.1-2024

// X/Open Portability Guide (XPG) and Single UNIX Specification (SUS...) versions

#define CZ_XPG_1985 1 // XPG
#define CZ_XPG_1987 2 // XPG2
#define CZ_XPG_1989 3 // XPG3
#define CZ_XPG_1992 4 // XPG4

#define CZ_SUS_1994 400 // SUS   - XPG4v2
#define CZ_SUS_1997 500 // SUSv2
#define CZ_SUS_2001 600 // SUSv3
#define CZ_SUS_2008 700 // SUSv4
#define CZ_SUS_2024 800 // SUSv5

// Check for predefined OS macros

#if !defined(CZ_AIX)
#if defined(_AIX) || defined(__TOS_AIX__)
#define CZ_AIX 1
#else
#define CZ_AIX 0
#endif
#endif

#if !defined(CZ_ANDROID)
#if defined(__ANDROID__)
#define CZ_ANDROID 1
#else
#define CZ_ANDROID 0
#endif
#endif

#if !defined(CZ_CYGWIN)
#if defined(__CYGWIN__)
#define CZ_CYGWIN 1
#else
#define CZ_CYGWIN 0
#endif
#endif

#if !defined(CZ_DARWIN)
#if defined(__APPLE__) && defined(__MACH__)
#define CZ_DARWIN 1
#else
#define CZ_DARWIN 0
#endif
#endif

#if !defined(CZ_DRAGONFLY_BSD)
#if defined(__DragonFly__)
#define CZ_DRAGONFLY_BSD 1
#else
#define CZ_DRAGONFLY_BSD 0
#endif
#endif

#if !defined(CZ_FREE_BSD)
#if defined(__FreeBSD__)
#define CZ_FREE_BSD 1
#else
#define CZ_FREE_BSD 0
#endif
#endif

#if !defined(CZ_FREE_BSD_MAJOR)
#if defined(__FreeBSD__)
#define CZ_FREE_BSD_MAJOR __FreeBSD__
#elif CZ_FREE_BSD
#define CZ_FREE_BSD_MAJOR 0
#else
#define CZ_FREE_BSD_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_FREE_BSD_MINOR)
#if defined(__FreeBSD_version)
#define CZ_FREE_BSD_MINOR ( (__FreeBSD_version / 1000) % 100 )
#elif CZ_FREE_BSD
#define CZ_FREE_BSD_MINOR 0
#else
#define CZ_FREE_BSD_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_FREE_BSD_PATCH)
#if defined(__FreeBSD_version)
#define CZ_FREE_BSD_PATCH ( __FreeBSD_version % 1000 )
#elif CZ_FREE_BSD
#define CZ_FREE_BSD_PATCH 0
#else
#define CZ_FREE_BSD_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_FREE_BSD_VERSION)
#define CZ_FREE_BSD_VERSION CZ_MAKE_VERSION(CZ_FREE_BSD_MAJOR, CZ_FREE_BSD_MINOR, CZ_FREE_BSD_PATCH)
#endif

#if !defined(CZ_GNU_LINUX)
#if defined(__gnu_linux__)
#define CZ_GNU_LINUX 1
#else
#define CZ_GNU_LINUX 0
#endif
#endif

#if !defined(CZ_HAIKU)
#if defined(__HAIKU__)
#define CZ_HAIKU 1
#else
#define CZ_HAIKU 0
#endif
#endif

#if !defined(CZ_IOS)
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define CZ_IOS 1
#else
#define CZ_IOS 0
#endif
#endif

#if !defined(CZ_IOS_MAJOR)
#if defined(__IPHONE_OS_VERSION_MAX_ALLOWED)
#define CZ_IOS_MAJOR ( __IPHONE_OS_VERSION_MAX_ALLOWED / 10000 )
#elif CZ_IOS
#define CZ_IOS_MAJOR 0
#else
#define CZ_IOS_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_IOS_MINOR)
#if defined(__IPHONE_OS_VERSION_MAX_ALLOWED)
#define CZ_IOS_MINOR ( (__IPHONE_OS_VERSION_MAX_ALLOWED / 100) % 100 )
#elif CZ_IOS
#define CZ_IOS_MINOR 0
#else
#define CZ_IOS_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_IOS_PATCH)
#if defined(__IPHONE_OS_VERSION_MAX_ALLOWED)
#define CZ_IOS_PATCH ( __IPHONE_OS_VERSION_MAX_ALLOWED % 100 )
#elif CZ_IOS
#define CZ_IOS_PATCH 0
#else
#define CZ_IOS_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_IOS_VERSION)
#define CZ_IOS_VERSION CZ_MAKE_VERSION(CZ_IOS_MAJOR, CZ_IOS_MINOR, CZ_IOS_PATCH)
#endif

#if !defined(CZ_LINUX)
#if defined(__linux__)
#define CZ_LINUX 1
#else
#define CZ_LINUX 0
#endif
#endif

#if !defined(CZ_MACOS)
#if defined(TARGET_OS_OSX) && TARGET_OS_OSX
#define CZ_MACOS 1
#else
#define CZ_MACOS 0
#endif
#endif

#if !defined(CZ_MACOS_MAJOR)
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 10000
#define CZ_MACOS_MAJOR ( __MAC_OS_X_VERSION_MAX_ALLOWED / 10000 )
#elif defined(__MAC_OS_X_VERSION_MAX_ALLOWED)
#define CZ_MACOS_MAJOR ( __MAC_OS_X_VERSION_MAX_ALLOWED / 100 )
#elif CZ_MACOS
#define CZ_MACOS_MAJOR 0
#else
#define CZ_MACOS_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_MACOS_MINOR)
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 10000
#define CZ_MACOS_MINOR ( (__MAC_OS_X_VERSION_MAX_ALLOWED / 100) % 100 )
#elif defined(__MAC_OS_X_VERSION_MAX_ALLOWED)
#define CZ_MACOS_MINOR ( (__MAC_OS_X_VERSION_MAX_ALLOWED / 10) % 10 )
#elif CZ_MACOS
#define CZ_MACOS_MINOR 0
#else
#define CZ_MACOS_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_MACOS_PATCH)
#if defined(__MAC_OS_X_VERSION_MAX_ALLOWED) && __MAC_OS_X_VERSION_MAX_ALLOWED >= 10000
#define CZ_MACOS_PATCH ( __MAC_OS_X_VERSION_MAX_ALLOWED % 100 )
#elif defined(__MAC_OS_X_VERSION_MAX_ALLOWED)
#define CZ_MACOS_PATCH ( __MAC_OS_X_VERSION_MAX_ALLOWED % 10 )
#elif CZ_MACOS
#define CZ_MACOS_PATCH 0
#else
#define CZ_MACOS_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_MACOS_VERSION)
#define CZ_MACOS_VERSION CZ_MAKE_VERSION(CZ_MACOS_MAJOR, CZ_MACOS_MINOR, CZ_MACOS_PATCH)
#endif

#if !defined(CZ_MIDNIGHT_BSD)
#if defined(__MidnightBSD__)
#define CZ_MIDNIGHT_BSD 1
#else
#define CZ_MIDNIGHT_BSD 0
#endif
#endif

#if !defined(CZ_NET_BSD)
#if defined(__NetBSD__)
#define CZ_NET_BSD 1
#else
#define CZ_NET_BSD 0
#endif
#endif

#if !defined(CZ_NET_BSD_MAJOR)
#if defined(__NetBSD_Version__)
#define CZ_NET_BSD_MAJOR ( __NetBSD_Version__ / 100000000 )
#elif CZ_NET_BSD
#define CZ_NET_BSD_MAJOR 0
#else
#define CZ_NET_BSD_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_NET_BSD_MINOR)
#if defined(__NetBSD_Version__)
#define CZ_NET_BSD_MINOR ( (__NetBSD_Version__ / 1000000) % 100 )
#elif CZ_NET_BSD
#define CZ_NET_BSD_MINOR 0
#else
#define CZ_NET_BSD_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_NET_BSD_PATCH)
#if defined(__NetBSD_Version__)
#define CZ_NET_BSD_PATCH ( (__NetBSD_Version__ / 100) % 100 )
#elif CZ_NET_BSD
#define CZ_NET_BSD_PATCH 0
#else
#define CZ_NET_BSD_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_NET_BSD_VERSION)
#define CZ_NET_BSD_VERSION CZ_MAKE_VERSION(CZ_NET_BSD_MAJOR, CZ_NET_BSD_MINOR, CZ_NET_BSD_PATCH)
#endif

#if !defined(CZ_OPEN_BSD)
#if defined(__OpenBSD__)
#define CZ_OPEN_BSD 1
#else
#define CZ_OPEN_BSD 0
#endif
#endif

#if !defined(CZ_QNX)
#if defined(__QNX__)
#define CZ_QNX 1
#else
#define CZ_QNX 0
#endif
#endif

#if !defined(CZ_QNX_MAJOR)
#if defined(__QNX__)
#define CZ_QNX_MAJOR ( __QNX__ / 100 )
#elif CZ_QNX
#define CZ_QNX_MAJOR 0
#else
#define CZ_QNX_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_QNX_MINOR)
#if defined(__QNX__)
#define CZ_QNX_MINOR ( (__QNX__ / 10) % 10 )
#elif CZ_QNX
#define CZ_QNX_MINOR 0
#else
#define CZ_QNX_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_QNX_PATCH)
#if defined(__QNX__)
#define CZ_QNX_PATCH ( __QNX__ % 10 )
#elif CZ_QNX
#define CZ_QNX_PATCH 0
#else
#define CZ_QNX_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_QNX_VERSION)
#define CZ_QNX_VERSION CZ_MAKE_VERSION(CZ_QNX_MAJOR, CZ_QNX_MINOR, CZ_QNX_PATCH)
#endif

#if !defined(CZ_UNIX)
#if defined(__unix__) || defined(__unix) || defined(unix)
#define CZ_UNIX 1
#else
#define CZ_UNIX 0
#endif
#endif

#if !defined(CZ_WIN32)
#if defined(_WIN32)
#define CZ_WIN32 1
#else
#define CZ_WIN32 0
#endif
#endif

#if !defined(CZ_WIN64)
#if defined(_WIN64)
#define CZ_WIN64 1
#else
#define CZ_WIN64 0
#endif
#endif

// Check for predefined compiler macros

#if !defined(CZ_APPLE_CLANG)
#if defined(__apple_build_version__) && defined(__clang__)
#define CZ_APPLE_CLANG 1
#else
#define CZ_APPLE_CLANG 0
#endif
#endif

#if !defined(CZ_CLANG)
#if defined(__clang__)
#define CZ_CLANG 1
#else
#define CZ_CLANG 0
#endif
#endif

#if !defined(CZ_CLANG_MAJOR)
#if defined(__clang_major__)
#define CZ_CLANG_MAJOR __clang_major__
#elif CZ_CLANG
#define CZ_CLANG_MAJOR 0
#else
#define CZ_CLANG_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_CLANG_MINOR)
#if defined(__clang_minor__)
#define CZ_CLANG_MINOR __clang_minor__
#elif CZ_CLANG
#define CZ_CLANG_MINOR 0
#else
#define CZ_CLANG_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_CLANG_PATCH)
#if defined(__clang_patchlevel__)
#define CZ_CLANG_PATCH __clang_patchlevel__
#elif CZ_CLANG
#define CZ_CLANG_PATCH 0
#else
#define CZ_CLANG_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_CLANG_VERSION)
#define CZ_CLANG_VERSION CZ_MAKE_VERSION(CZ_CLANG_MAJOR, CZ_CLANG_MINOR, CZ_CLANG_PATCH)
#endif

#if !defined(CZ_GNUC)
#if defined(__GNUC__)
#define CZ_GNUC 1
#else
#define CZ_GNUC 0
#endif
#endif

#if !defined(CZ_GNUC_MAJOR)
#if defined(__GNUC__)
#define CZ_GNUC_MAJOR __GNUC__
#elif CZ_GNUC
#define CZ_GNUC_MAJOR 0
#else
#define CZ_GNUC_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_GNUC_MINOR)
#if defined(__GNUC_MINOR__)
#define CZ_GNUC_MINOR __GNUC_MINOR__
#elif CZ_GNUC
#define CZ_GNUC_MINOR 0
#else
#define CZ_GNUC_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_GNUC_PATCH)
#if defined(__GNUC_PATCHLEVEL__)
#define CZ_GNUC_PATCH __GNUC_PATCHLEVEL__
#elif CZ_GNUC
#define CZ_GNUC_PATCH 0
#else
#define CZ_GNUC_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_GNUC_VERSION)
#define CZ_GNUC_VERSION CZ_MAKE_VERSION(CZ_GNUC_MAJOR, CZ_GNUC_MINOR, CZ_GNUC_PATCH)
#endif

#if !defined(CZ_MINGW32)
#if defined(__MINGW32__)
#define CZ_MINGW32 1
#else
#define CZ_MINGW32 0
#endif
#endif

#if !defined(CZ_MINGW32_MAJOR)
#if defined(__MINGW32_MAJOR_VERSION)
#define CZ_MINGW32_MAJOR __MINGW32_MAJOR_VERSION
#elif CZ_MINGW32
#define CZ_MINGW32_MAJOR 0
#else
#define CZ_MINGW32_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_MINGW32_MINOR)
#if defined(__MINGW32_MINOR_VERSION)
#define CZ_MINGW32_MINOR __MINGW32_MINOR_VERSION
#elif CZ_MINGW32
#define CZ_MINGW32_MINOR 0
#else
#define CZ_MINGW32_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_MINGW32_VERSION)
#define CZ_MINGW32_VERSION CZ_MAKE_VERSION(CZ_MINGW32_MAJOR, CZ_MINGW32_MINOR)
#endif

#if !defined(CZ_MINGW64)
#if defined(__MINGW64__)
#define CZ_MINGW64 1
#else
#define CZ_MINGW64 0
#endif
#endif

#if !defined(CZ_MINGW64_MAJOR)
#if defined(__MINGW64_MAJOR_VERSION)
#define CZ_MINGW64_MAJOR __MINGW64_MAJOR_VERSION
#elif CZ_MINGW64
#define CZ_MINGW64_MAJOR 0
#else
#define CZ_MINGW64_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_MINGW64_MINOR)
#if defined(__MINGW64_MINOR_VERSION)
#define CZ_MINGW64_MINOR __MINGW64_MINOR_VERSION
#elif CZ_MINGW64
#define CZ_MINGW64_MINOR 0
#else
#define CZ_MINGW64_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_MINGW64_VERSION)
#define CZ_MINGW64_VERSION CZ_MAKE_VERSION(CZ_MINGW64_MAJOR, CZ_MINGW64_MINOR)
#endif

#if !defined(CZ_MSVC)
#if defined(_MSC_VER)
#define CZ_MSVC 1
#else
#define CZ_MSVC 0
#endif
#endif

#if !defined(CZ_MSVC_MAJOR)
#if defined(_MSC_VER)
#define CZ_MSVC_MAJOR ( _MSC_VER / 100 )
#elif CZ_MSVC
#define CZ_MSVC_MAJOR 0
#else
#define CZ_MSVC_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_MSVC_MINOR)
#if defined(_MSC_VER)
#define CZ_MSVC_MINOR ( _MSC_VER % 100 )
#elif CZ_MSVC
#define CZ_MSVC_MINOR 0
#else
#define CZ_MSVC_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_MSVC_PATCH)
#if defined(_MSC_FULL_VER)
#define CZ_MSVC_PATCH ( _MSC_FULL_VER % 100000 )
#elif CZ_MSVC
#define CZ_MSVC_PATCH 0
#else
#define CZ_MSVC_PATCH ( -1 )
#endif
#endif

#if !defined(CZ_MSVC_VERSION)
#define CZ_MSVC_VERSION CZ_MAKE_VERSION(CZ_MSVC_MAJOR, CZ_MSVC_MINOR, CZ_MSVC_PATCH)
#endif

// Check for predefined libc macros

#if !defined(CZ_GLIBC)
#if defined(__GLIBC__) || defined(__GNU_LIBRARY__)
#define CZ_GLIBC 1
#else
#define CZ_GLIBC 0
#endif
#endif

#if !defined(CZ_GLIBC_MAJOR)
#if defined(__GLIBC__)
#define CZ_GLIBC_MAJOR __GLIBC__
#elif defined(__GNU_LIBRARY__)
#define CZ_GLIBC_MAJOR __GNU_LIBRARY__
#elif CZ_GLIBC
#define CZ_GLIBC_MAJOR 0
#else
#define CZ_GLIBC_MAJOR ( -1 )
#endif
#endif

#if !defined(CZ_GLIBC_MINOR)
#if defined(__GLIBC_MINOR__)
#define CZ_GLIBC_MINOR __GLIBC_MINOR__
#elif defined(__GNU_LIBRARY_MINOR__)
#define CZ_GLIBC_MINOR __GNU_LIBRARY_MINOR__
#elif CZ_GLIBC
#define CZ_GLIBC_MINOR 0
#else
#define CZ_GLIBC_MINOR ( -1 )
#endif
#endif

#if !defined(CZ_GLIBC_VERSION)
#define CZ_GLIBC_VERSION CZ_MAKE_VERSION(CZ_GLIBC_MAJOR, CZ_GLIBC_MINOR)
#endif

// Check for predefined stdc macros

#if !defined(CZ_STDC)
#if defined(__STDC__)
#define CZ_STDC 1
#else
#define CZ_STDC 0
#endif
#endif

#if !defined(CZ_STDC_VERSION)
#if defined(__STDC_VERSION__)
#define CZ_STDC_VERSION __STDC_VERSION__
#elif defined(__STDC__)
#define CZ_STDC_VERSION CZ_STDC_1989
#elif CZ_STDC
#define CZ_STDC_VERSION 0
#else
#define CZ_STDC_VERSION ( -1 )
#endif
#endif

// Check for predefined POSIX.1 macros

#if !defined(CZ_POSIX_VERSION)
#if defined(_POSIX_VERSION)
#define CZ_POSIX_VERSION _POSIX_VERSION
#else
#define CZ_POSIX_VERSION ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_ADVISORY_INFO)
#if defined(_POSIX_ADVISORY_INFO)
#define CZ_POSIX_ADVISORY_INFO _POSIX_ADVISORY_INFO
#else
#define CZ_POSIX_ADVISORY_INFO ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_ASYNCHRONOUS_IO)
#if defined(_POSIX_ASYNCHRONOUS_IO)
#define CZ_POSIX_ASYNCHRONOUS_IO _POSIX_ASYNCHRONOUS_IO
#else
#define CZ_POSIX_ASYNCHRONOUS_IO ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_BARRIERS)
#if defined(_POSIX_BARRIERS)
#define CZ_POSIX_BARRIERS _POSIX_BARRIERS
#else
#define CZ_POSIX_BARRIERS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_CLOCK_SELECTION)
#if defined(_POSIX_CLOCK_SELECTION)
#define CZ_POSIX_CLOCK_SELECTION _POSIX_CLOCK_SELECTION
#else
#define CZ_POSIX_CLOCK_SELECTION ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_CPUTIME)
#if defined(_POSIX_CPUTIME)
#define CZ_POSIX_CPUTIME _POSIX_CPUTIME
#else
#define CZ_POSIX_CPUTIME ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_FSYNC)
#if defined(_POSIX_FSYNC)
#define CZ_POSIX_FSYNC _POSIX_FSYNC
#else
#define CZ_POSIX_FSYNC ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_IPV6)
#if defined(_POSIX_IPV6)
#define CZ_POSIX_IPV6 _POSIX_IPV6
#else
#define CZ_POSIX_IPV6 ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_JOB_CONTROL)
#if defined(_POSIX_JOB_CONTROL)
#define CZ_POSIX_JOB_CONTROL _POSIX_JOB_CONTROL
#else
#define CZ_POSIX_JOB_CONTROL ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_MAPPED_FILES)
#if defined(_POSIX_MAPPED_FILES)
#define CZ_POSIX_MAPPED_FILES _POSIX_MAPPED_FILES
#else
#define CZ_POSIX_MAPPED_FILES ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_MEMLOCK)
#if defined(_POSIX_MEMLOCK)
#define CZ_POSIX_MEMLOCK _POSIX_MEMLOCK
#else
#define CZ_POSIX_MEMLOCK ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_MEMLOCK_RANGE)
#if defined(_POSIX_MEMLOCK_RANGE)
#define CZ_POSIX_MEMLOCK_RANGE _POSIX_MEMLOCK_RANGE
#else
#define CZ_POSIX_MEMLOCK_RANGE ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_MEMORY_PROTECTION)
#if defined(_POSIX_MEMORY_PROTECTION)
#define CZ_POSIX_MEMORY_PROTECTION _POSIX_MEMORY_PROTECTION
#else
#define CZ_POSIX_MEMORY_PROTECTION ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_MESSAGE_PASSING)
#if defined(_POSIX_MESSAGE_PASSING)
#define CZ_POSIX_MESSAGE_PASSING _POSIX_MESSAGE_PASSING
#else
#define CZ_POSIX_MESSAGE_PASSING ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_MONOTONIC_CLOCK)
#if defined(_POSIX_MONOTONIC_CLOCK)
#define CZ_POSIX_MONOTONIC_CLOCK _POSIX_MONOTONIC_CLOCK
#else
#define CZ_POSIX_MONOTONIC_CLOCK ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_NO_TRUNC)
#if defined(_POSIX_NO_TRUNC)
#define CZ_POSIX_NO_TRUNC _POSIX_NO_TRUNC
#else
#define CZ_POSIX_NO_TRUNC ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_PRIORITIZED_IO)
#if defined(_POSIX_PRIORITIZED_IO)
#define CZ_POSIX_PRIORITIZED_IO _POSIX_PRIORITIZED_IO
#else
#define CZ_POSIX_PRIORITIZED_IO ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_PRIORITY_SCHEDULING)
#if defined(_POSIX_PRIORITY_SCHEDULING)
#define CZ_POSIX_PRIORITY_SCHEDULING _POSIX_PRIORITY_SCHEDULING
#else
#define CZ_POSIX_PRIORITY_SCHEDULING ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_RAW_SOCKETS)
#if defined(_POSIX_RAW_SOCKETS)
#define CZ_POSIX_RAW_SOCKETS _POSIX_RAW_SOCKETS
#else
#define CZ_POSIX_RAW_SOCKETS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_READER_WRITER_LOCKS)
#if defined(_POSIX_READER_WRITER_LOCKS)
#define CZ_POSIX_READER_WRITER_LOCKS _POSIX_READER_WRITER_LOCKS
#else
#define CZ_POSIX_READER_WRITER_LOCKS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_REALTIME_SIGNALS)
#if defined(_POSIX_REALTIME_SIGNALS)
#define CZ_POSIX_REALTIME_SIGNALS _POSIX_REALTIME_SIGNALS
#else
#define CZ_POSIX_REALTIME_SIGNALS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_REGEXP)
#if defined(_POSIX_REGEXP)
#define CZ_POSIX_REGEXP _POSIX_REGEXP
#else
#define CZ_POSIX_REGEXP ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SAVED_IDS)
#if defined(_POSIX_SAVED_IDS)
#define CZ_POSIX_SAVED_IDS _POSIX_SAVED_IDS
#else
#define CZ_POSIX_SAVED_IDS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SEMAPHORES)
#if defined(_POSIX_SEMAPHORES)
#define CZ_POSIX_SEMAPHORES _POSIX_SEMAPHORES
#else
#define CZ_POSIX_SEMAPHORES ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SHARED_MEMORY_OBJECTS)
#if defined(_POSIX_SHARED_MEMORY_OBJECTS)
#define CZ_POSIX_SHARED_MEMORY_OBJECTS _POSIX_SHARED_MEMORY_OBJECTS
#else
#define CZ_POSIX_SHARED_MEMORY_OBJECTS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SHELL)
#if defined(_POSIX_SHELL)
#define CZ_POSIX_SHELL _POSIX_SHELL
#else
#define CZ_POSIX_SHELL ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SPAWN)
#if defined(_POSIX_SPAWN)
#define CZ_POSIX_SPAWN _POSIX_SPAWN
#else
#define CZ_POSIX_SPAWN ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SPIN_LOCKS)
#if defined(_POSIX_SPIN_LOCKS)
#define CZ_POSIX_SPIN_LOCKS _POSIX_SPIN_LOCKS
#else
#define CZ_POSIX_SPIN_LOCKS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SPORADIC_SERVER)
#if defined(_POSIX_SPORADIC_SERVER)
#define CZ_POSIX_SPORADIC_SERVER _POSIX_SPORADIC_SERVER
#else
#define CZ_POSIX_SPORADIC_SERVER ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_SYNCHRONIZED_IO)
#if defined(_POSIX_SYNCHRONIZED_IO)
#define CZ_POSIX_SYNCHRONIZED_IO _POSIX_SYNCHRONIZED_IO
#else
#define CZ_POSIX_SYNCHRONIZED_IO ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_ATTR_STACKADDR)
#if defined(_POSIX_THREAD_ATTR_STACKADDR)
#define CZ_POSIX_THREAD_ATTR_STACKADDR _POSIX_THREAD_ATTR_STACKADDR
#else
#define CZ_POSIX_THREAD_ATTR_STACKADDR ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_ATTR_STACKSIZE)
#if defined(_POSIX_THREAD_ATTR_STACKSIZE)
#define CZ_POSIX_THREAD_ATTR_STACKSIZE _POSIX_THREAD_ATTR_STACKSIZE
#else
#define CZ_POSIX_THREAD_ATTR_STACKSIZE ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_CPUTIME)
#if defined(_POSIX_THREAD_CPUTIME)
#define CZ_POSIX_THREAD_CPUTIME _POSIX_THREAD_CPUTIME
#else
#define CZ_POSIX_THREAD_CPUTIME ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_PRIO_INHERIT)
#if defined(_POSIX_THREAD_PRIO_INHERIT)
#define CZ_POSIX_THREAD_PRIO_INHERIT _POSIX_THREAD_PRIO_INHERIT
#else
#define CZ_POSIX_THREAD_PRIO_INHERIT ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_PRIO_PROTECT)
#if defined(_POSIX_THREAD_PRIO_PROTECT)
#define CZ_POSIX_THREAD_PRIO_PROTECT _POSIX_THREAD_PRIO_PROTECT
#else
#define CZ_POSIX_THREAD_PRIO_PROTECT ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_PRIORITY_SCHEDULING)
#if defined(_POSIX_THREAD_PRIORITY_SCHEDULING)
#define CZ_POSIX_THREAD_PRIORITY_SCHEDULING _POSIX_THREAD_PRIORITY_SCHEDULING
#else
#define CZ_POSIX_THREAD_PRIORITY_SCHEDULING ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_PROCESS_SHARED)
#if defined(_POSIX_THREAD_PROCESS_SHARED)
#define CZ_POSIX_THREAD_PROCESS_SHARED _POSIX_THREAD_PROCESS_SHARED
#else
#define CZ_POSIX_THREAD_PROCESS_SHARED ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_SAFE_FUNCTIONS)
#if defined(_POSIX_THREAD_SAFE_FUNCTIONS)
#define CZ_POSIX_THREAD_SAFE_FUNCTIONS _POSIX_THREAD_SAFE_FUNCTIONS
#else
#define CZ_POSIX_THREAD_SAFE_FUNCTIONS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREAD_SPORADIC_SERVER)
#if defined(_POSIX_THREAD_SPORADIC_SERVER)
#define CZ_POSIX_THREAD_SPORADIC_SERVER _POSIX_THREAD_SPORADIC_SERVER
#else
#define CZ_POSIX_THREAD_SPORADIC_SERVER ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_THREADS)
#if defined(_POSIX_THREADS)
#define CZ_POSIX_THREADS _POSIX_THREADS
#else
#define CZ_POSIX_THREADS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TIMEOUTS)
#if defined(_POSIX_TIMEOUTS)
#define CZ_POSIX_TIMEOUTS _POSIX_TIMEOUTS
#else
#define CZ_POSIX_TIMEOUTS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TIMERS)
#if defined(_POSIX_TIMERS)
#define CZ_POSIX_TIMERS _POSIX_TIMERS
#else
#define CZ_POSIX_TIMERS ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TRACE)
#if defined(_POSIX_TRACE)
#define CZ_POSIX_TRACE _POSIX_TRACE
#else
#define CZ_POSIX_TRACE ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TRACE_EVENT_FILTER)
#if defined(_POSIX_TRACE_EVENT_FILTER)
#define CZ_POSIX_TRACE_EVENT_FILTER _POSIX_TRACE_EVENT_FILTER
#else
#define CZ_POSIX_TRACE_EVENT_FILTER ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TRACE_INHERIT)
#if defined(_POSIX_TRACE_INHERIT)
#define CZ_POSIX_TRACE_INHERIT _POSIX_TRACE_INHERIT
#else
#define CZ_POSIX_TRACE_INHERIT ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TRACE_LOG)
#if defined(_POSIX_TRACE_LOG)
#define CZ_POSIX_TRACE_LOG _POSIX_TRACE_LOG
#else
#define CZ_POSIX_TRACE_LOG ( -1 )
#endif
#endif

#if !defined(CZ_POSIX_TYPED_MEMORY_OBJECTS)
#if defined(_POSIX_TYPED_MEMORY_OBJECTS)
#define CZ_POSIX_TYPED_MEMORY_OBJECTS _POSIX_TYPED_MEMORY_OBJECTS
#else
#define CZ_POSIX_TYPED_MEMORY_OBJECTS ( -1 )
#endif
#endif

// Check for predefined X/OPEN macros

#if !defined(CZ_XOPEN_VERSION)
#if defined(_XOPEN_VERSION) && _XOPEN_VERSION >= CZ_SUS_1997
#define CZ_XOPEN_VERSION _XOPEN_VERSION
#elif defined(_XOPEN_UNIX)
#define CZ_XOPEN_VERSION CZ_SUS_1994
#elif defined(_XOPEN_XPG4)
#define CZ_XOPEN_VERSION CZ_XPG_1992
#elif defined(_XOPEN_XPG3)
#define CZ_XOPEN_VERSION CZ_XPG_1989
#elif defined(_XOPEN_XPG2)
#define CZ_XOPEN_VERSION CZ_XPG_1987
#elif defined(_XOPEN_VERSION)
#define CZ_XOPEN_VERSION CZ_XPG_1985
#else
#define CZ_XOPEN_VERSION ( -1 )
#endif
#endif

#if !defined(CZ_XOPEN_CRYPT)
#if defined(_XOPEN_CRYPT)
#define CZ_XOPEN_CRYPT _XOPEN_CRYPT
#else
#define CZ_XOPEN_CRYPT ( -1 )
#endif
#endif

#if !defined (CZ_XOPEN_LEGACY)
#if defined(_XOPEN_LEGACY)
#define CZ_XOPEN_LEGACY _XOPEN_LEGACY
#else
#define CZ_XOPEN_LEGACY ( -1 )
#endif
#endif

#if !defined(CZ_XOPEN_REALTIME)
#if defined(_XOPEN_REALTIME)
#define CZ_XOPEN_REALTIME _XOPEN_REALTIME
#else
#define CZ_XOPEN_REALTIME ( -1 )
#endif
#endif

#if !defined(CZ_XOPEN_REALTIME_THREADS)
#if defined(_XOPEN_REALTIME_THREADS)
#define CZ_XOPEN_REALTIME_THREADS _XOPEN_REALTIME_THREADS
#else
#define CZ_XOPEN_REALTIME_THREADS ( -1 )
#endif
#endif

#if !defined(CZ_XOPEN_STREAMS)
#if defined(_XOPEN_STREAMS)
#define CZ_XOPEN_STREAMS _XOPEN_STREAMS
#else
#define CZ_XOPEN_STREAMS ( -1 )
#endif
#endif

// Check for user-defined feature macros

#if !defined(CZ_ANSI_SOURCE)
#if defined(_ANSI_SOURCE)
#define CZ_ANSI_SOURCE 1
#else
#define CZ_ANSI_SOURCE 0
#endif
#endif

#if !defined(CZ_ATFILE_SOURCE)
#if defined(_ATFILE_SOURCE)
#define CZ_ATFILE_SOURCE 1
#else
#define CZ_ATFILE_SOURCE 0
#endif
#endif

#if !defined(CZ_BSD_SOURCE)
#if defined(_BSD_SOURCE)
#define CZ_BSD_SOURCE 1
#else
#define CZ_BSD_SOURCE 0
#endif
#endif

#if !defined(CZ_C99_SOURCE)
#if defined(_C99_SOURCE)
#define CZ_C99_SOURCE 1
#else
#define CZ_C99_SOURCE 0
#endif
#endif

#if !defined(CZ_C11_SOURCE)
#if defined(_C11_SOURCE)
#define CZ_C11_SOURCE 1
#else
#define CZ_C11_SOURCE 0
#endif
#endif

#if !defined(CZ_DARWIN_C_SOURCE)
#if defined(_DARWIN_C_SOURCE)
#define CZ_DARWIN_C_SOURCE 1
#else
#define CZ_DARWIN_C_SOURCE 0
#endif
#endif

#if !defined(CZ_DEFAULT_SOURCE)
#if defined(_DEFAULT_SOURCE)
#define CZ_DEFAULT_SOURCE 1
#else
#define CZ_DEFAULT_SOURCE 0
#endif
#endif

#if !defined(CZ_FILE_OFFSET_BITS)
#if defined(_FILE_OFFSET_BITS) && _FILE_OFFSET_BITS > 0
#define CZ_FILE_OFFSET_BITS _FILE_OFFSET_BITS
#elif defined(_LARGEFILE_SOURCE)
#define CZ_FILE_OFFSET_BITS 64
#else
#define CZ_FILE_OFFSET_BITS 0
#endif
#endif

#if !defined(CZ_GNU_SOURCE)
#if defined(_GNU_SOURCE)
#define CZ_GNU_SOURCE 1
#else
#define CZ_GNU_SOURCE 0
#endif
#endif

#if !defined(CZ_ISOC99_SOURCE)
#if defined(_ISOC99_SOURCE) || defined(_ISOC9X_SOURCE)
#define CZ_ISOC99_SOURCE 1
#else
#define CZ_ISOC99_SOURCE 0
#endif
#endif

#if !defined(CZ_ISOC11_SOURCE)
#if defined(_ISOC11_SOURCE)
#define CZ_ISOC11_SOURCE 1
#else
#define CZ_ISOC11_SOURCE 0
#endif
#endif

#if !defined(CZ_NETBSD_SOURCE)
#if defined(_NETBSD_SOURCE)
#define CZ_NETBSD_SOURCE 1
#else
#define CZ_NETBSD_SOURCE 0
#endif
#endif

#if !defined(CZ_OPENBSD_SOURCE)
#if defined(_OPENBSD_SOURCE)
#define CZ_OPENBSD_SOURCE 1
#else
#define CZ_OPENBSD_SOURCE 0
#endif
#endif

#if !defined(CZ_POSIX_C_SOURCE)
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= CZ_POSIX_1993
#define CZ_POSIX_C_SOURCE _POSIX_C_SOURCE
#elif defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 2
#define CZ_POSIX_C_SOURCE CZ_POSIX_1992
#elif defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE == 1
#define CZ_POSIX_C_SOURCE CZ_POSIX_1990
#elif defined(_POSIX_C_SOURCE)
#define CZ_POSIX_C_SOURCE CZ_POSIX_1988
#else
#define CZ_POSIX_C_SOURCE 0
#endif
#endif

#if !defined(CZ_SVID_SOURCE)
#if defined(_SVID_SOURCE)
#define CZ_SVID_SOURCE 1
#else
#define CZ_SVID_SOURCE 0
#endif
#endif

#if !defined(CZ_TIME_BITS)
#if defined(_TIME_BITS) && _TIME_BITS > 0
#define CZ_TIME_BITS _TIME_BITS
#else
#define CZ_TIME_BITS 0
#endif
#endif

#if !defined(CZ_XOPEN_SOURCE)
#if defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= CZ_SUS_1997
#define CZ_XOPEN_SOURCE _XOPEN_SOURCE
#elif defined(_XOPEN_SOURCE)
#define CZ_XOPEN_SOURCE CZ_SUS_1994
#else
#define CZ_XOPEN_SOURCE 0
#endif
#endif

// Check support for attributes, builtins, features, etc.

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

#if !defined(CZ_HAS_EXTENSION)
#if defined( __has_extension)
#define CZ_HAS_EXTENSION(x)  __has_extension(x)
#else
#define CZ_HAS_EXTENSION(x) 0
#endif
#endif

#if !defined(CZ_HAS_FEATURE)
#if defined(__has_feature)
#define CZ_HAS_FEATURE(x) __has_feature(x)
#else
#define CZ_HAS_FEATURE(x) 0
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
#elif CZ_MSVC
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

#if !defined(CZ_COPY_ATTR)
#if CZ_HAS_ATTRIBUTE(copy)
#define CZ_COPY_ATTR(func) __attribute__ (( copy (func) ))
#else
#define CZ_COPY_ATTR(func)
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
#elif CZ_MSVC
#define CZ_CONST __declspec(noalias)
#else
#define CZ_CONST
#endif
#endif

#if !defined(CZ_PURE)
#if CZ_HAS_ATTRIBUTE(pure)
#define CZ_PURE __attribute__ (( pure ))
#elif CZ_MSVC
#define CZ_PURE __declspec(noalias)
#else
#define CZ_PURE
#endif
#endif

#if !defined(CZ_REPRODUCIBLE)
#if CZ_HAS_ATTRIBUTE(reproducible)
#define CZ_REPRODUCIBLE __attribute__ (( reproducible ))
#elif CZ_MSVC
#define CZ_REPRODUCIBLE __declspec(noalias)
#else
#define CZ_REPRODUCIBLE
#endif
#endif

#if !defined(CZ_UNSEQUENCED)
#if CZ_HAS_ATTRIBUTE(unsequenced)
#define CZ_UNSEQUENCED __attribute__ (( unsequenced ))
#elif CZ_MSVC
#define CZ_UNSEQUENCED __declspec(noalias)
#else
#define CZ_UNSEQUENCED
#endif
#endif

#if !defined(CZ_MALLOC)
#if CZ_HAS_ATTRIBUTE(malloc)
#define CZ_MALLOC __attribute__ (( malloc ))
#elif CZ_MSVC
#define CZ_MALLOC __declspec(allocator)
#else
#define CZ_MALLOC
#endif
#endif

#if !defined(CZ_FREE)
#if CZ_HAS_ATTRIBUTE(malloc) && CZ_GNUC_VERSION >= CZ_MAKE_VERSION(11, 1)
#define CZ_FREE(func, arg) __attribute__ (( malloc(func, arg) ))
#else
#define CZ_FREE(func, arg)
#endif
#endif

#if !defined(CZ_PRINTF)
#if CZ_HAS_ATTRIBUTE(format) && CZ_CLANG
#define CZ_PRINTF(fmt, args) __attribute__ (( format(printf, fmt, args) ))
#elif CZ_HAS_ATTRIBUTE(format)
#define CZ_PRINTF(fmt, args) __attribute__ (( format(gnu_printf, fmt, args) ))
#else
#define CZ_PRINTF(fmt, args)
#endif
#endif

#if !defined(CZ_SCANF)
#if CZ_HAS_ATTRIBUTE(format) && CZ_CLANG
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

#if !defined(CZ_NULTERM_ARG)
#if CZ_HAS_ATTRIBUTE(null_terminated_string_arg)
#define CZ_NULTERM_ARG(arg) __attribute__ (( null_terminated_string_arg(arg) ))
#else
#define CZ_NULTERM_ARG(arg)
#endif
#endif

#if !defined(CZ_FILDES)
#if CZ_HAS_ATTRIBUTE(fd_arg)
#define CZ_FILDES(arg) __attribute__ (( fd_arg(arg) ))
#else
#define CZ_FILDES(arg)
#endif
#endif

#if !defined(CZ_RD_FILDES)
#if CZ_HAS_ATTRIBUTE(fd_arg_read)
#define CZ_RD_FILDES(arg) __attribute__ (( fd_arg_read(arg) ))
#else
#define CZ_RD_FILDES(arg)
#endif
#endif

#if !defined(CZ_WR_FILDES)
#if CZ_HAS_ATTRIBUTE(fd_arg_write)
#define CZ_WR_FILDES(arg) __attribute__ (( fd_arg_write(arg) ))
#else
#define CZ_WR_FILDES(arg)
#endif
#endif

#if !defined(CZ_RW_FILDES)
#if CZ_HAS_ATTRIBUTE(fd_arg_read) && CZ_HAS_ATTRIBUTE(fd_arg_write)
#define CZ_RW_FILDES(arg) __attribute__ (( fd_arg_read(arg), fd_arg_write(arg) ))
#elif CZ_HAS_ATTRIBUTE(fd_arg_read)
#define CZ_RW_FILDES(arg) __attribute__ (( fd_arg_read(arg) ))
#elif CZ_HAS_ATTRIBUTE(fd_arg_write)
#define CZ_RW_FILDES(arg) __attribute__ (( fd_arg_write(arg) ))
#else
#define CZ_RW_FILDES(arg)
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

#if !defined(CZ_FILENAME)
#if defined(__FILE_NAME__)
#define CZ_FILENAME __FILE_NAME__
#else
#define CZ_FILENAME __FILE__
#endif
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

#define CZ_U128_UPPER(x) ( (CzU64) ((x) >> 64) )
#define CZ_U128_LOWER(x) ( (CzU64) ((x) & ~UINT64_C(0)) )

#define CZ_U128(upper, lower) ( ((CzU128) (upper) << 64) | ((CzU128) (lower)) )

#define CZ_PNEXT_ADD(p, s) \
	do {                   \
		*(p) = &(s);       \
		(p) = &(s).pNext;  \
	}                      \
	while (0)

// Typedefs

typedef int8_t Cz8;
typedef int16_t Cz16;
typedef int32_t Cz32;
typedef int64_t Cz64;
typedef __int128 Cz128;

typedef uint8_t CzU8;
typedef uint16_t CzU16;
typedef uint32_t CzU32;
typedef uint64_t CzU64;
typedef unsigned __int128 CzU128;

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
	CZ_RESULT_BAD_IO,
	CZ_RESULT_BAD_OFFSET,
	CZ_RESULT_BAD_PATH,
	CZ_RESULT_BAD_RANGE,
	CZ_RESULT_BAD_SIZE,
	CZ_RESULT_BAD_STREAM,
	CZ_RESULT_DEADLOCK,
	CZ_RESULT_IN_USE,
	CZ_RESULT_INTERRUPT,
	CZ_RESULT_NO_CONNECTION,
	CZ_RESULT_NO_DISK,
	CZ_RESULT_NO_FILE,
	CZ_RESULT_NO_LOCK,
	CZ_RESULT_NO_MEMORY,
	CZ_RESULT_NO_OPEN,
	CZ_RESULT_NO_PROCESS,
	CZ_RESULT_NO_QUOTA,
	CZ_RESULT_NO_SUPPORT,
	CZ_RESULT_TIMEOUT,
};
