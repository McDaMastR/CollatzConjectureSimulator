/* 
 * Copyright (C) 2025 Seth McDonald
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

/**
 * @file
 * 
 * @brief Thin wrappers for common functions.
 * 
 * A non-comprehensive set of thin wrapper functions to provide consistent error management. These wrappers are intended
 * for use within cz* API implementations rather than for general use.
 * 
 * @note Due to differences in error reporting between various platforms and standards (e.g. MacOS, Windows, POSIX), as
 * well as differences in error categorisation compared to this API (@c CzResult enumeration values), the documented
 * return values for wrappers are @b not guarantees. If failure occurs in a wrapper function, the wrapper provides no
 * guarantee the corresponding error value will be returned. And in particular, if the platform does not provide any
 * reliable method to identify the type of error that occurred, @c CZ_RESULT_INTERNAL_ERROR will commonly be returned.
 */

#pragma once

#include "def.h"
#include "wrap_win32.h"

/**
 * @brief Wraps @c malloc.
 * 
 * Calls @c malloc with @p size. On success, the returned @c void* is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] size The argument to pass to @c malloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_malloc(void* restrict* res, size_t size);

/**
 * @brief Wraps @c calloc.
 * 
 * Calls @c calloc with @p count and @p size. On success, the returned @c void* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] count The first argument to pass to @c calloc.
 * @param[in] size The second argument to pass to @c calloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_SIZE @p count or @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_calloc(void* restrict* res, size_t count, size_t size);

/**
 * @brief Wraps @c realloc.
 * 
 * Calls @c realloc with @p ptr and @p size. On success, the returned @c void* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] ptr The first argument to pass to @c realloc.
 * @param[in] size The second argument to pass to @c realloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_realloc(void* restrict* res, void* ptr, size_t size);

/**
 * @def CZ_WRAP_REALLOCARRAY
 * 
 * @brief Specifies whether @c reallocarray is defined.
 */
#if !defined(CZ_WRAP_REALLOCARRAY)
#if (                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_GNU_SOURCE &&                               \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 26) &&  \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 29)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_DEFAULT_SOURCE &&                           \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 29)) || \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(11, 0) ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_2024
#define CZ_WRAP_REALLOCARRAY 1
#else
#define CZ_WRAP_REALLOCARRAY 0
#endif
#endif

#if CZ_WRAP_REALLOCARRAY
/**
 * @brief Wraps @c reallocarray.
 * 
 * Calls @c reallocarray with @p ptr, @p count, and @p size. On success, the returned @c void* is synchronously written
 * to @p res. On failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] ptr The first argument to pass to @c reallocarray.
 * @param[in] count The second argument to pass to @c reallocarray.
 * @param[in] size The third argument to pass to @c reallocarray.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_SIZE @p count or @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_REALLOCARRAY is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_reallocarray(void* restrict* res, void* ptr, size_t count, size_t size);
#endif

/**
 * @def CZ_WRAP_REALLOCF
 * 
 * @brief Specifies whether @c reallocf is defined.
 */
#if !defined(CZ_WRAP_REALLOCF)
#if CZ_DARWIN || \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(3, 0)
#define CZ_WRAP_REALLOCF 1
#else
#define CZ_WRAP_REALLOCF 0
#endif
#endif

#if CZ_WRAP_REALLOCF
/**
 * @brief Wraps @c reallocf.
 * 
 * Calls @c reallocf with @p ptr and @p size. On success, the returned @c void* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] ptr The first argument to pass to @c reallocf.
 * @param[in] size The second argument to pass to @c reallocf.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_REALLOCF is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_reallocf(void* restrict* res, void* ptr, size_t size);
#endif

/**
 * @def CZ_WRAP_ALIGNED_ALLOC
 * 
 * @brief Specifies whether @c aligned_alloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_ALLOC)
#if (                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_ISOC11_SOURCE &&                            \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 16)) || \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(10, 0) ||   \
	CZ_STDC_VERSION >= CZ_STDC_2011
#define CZ_WRAP_ALIGNED_ALLOC 1
#else
#define CZ_WRAP_ALIGNED_ALLOC 0
#endif
#endif

#if CZ_WRAP_ALIGNED_ALLOC
/**
 * @brief Wraps @c aligned_alloc.
 * 
 * Calls @c aligned_alloc with @p alignment and @p size. On success, the returned @c void* is synchronously written to
 * @p res. On failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] alignment The first argument to pass to @c aligned_alloc.
 * @param[in] size The second argument to pass to @c aligned_alloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was invalid or unsupported.
 * @retval CZ_RESULT_BAD_SIZE @p size was not a nonzero multiple of @p alignment.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_ALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_alloc(void* restrict* res, size_t alignment, size_t size);
#endif

/**
 * @def CZ_WRAP_POSIX_MEMALIGN
 * 
 * @brief Specifies whether @c posix_memalign is defined.
 */
#if !defined(CZ_WRAP_POSIX_MEMALIGN)
#if CZ_DARWIN ||                                          \
	(                                                     \
		CZ_GNU_LINUX &&                                   \
		(                                                 \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||         \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&            \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 1, 91)) || \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(7, 0) ||       \
	CZ_POSIX_ADVISORY_INFO >= CZ_POSIX_2001
#define CZ_WRAP_POSIX_MEMALIGN 1
#else
#define CZ_WRAP_POSIX_MEMALIGN 0
#endif
#endif

#if CZ_WRAP_POSIX_MEMALIGN
/**
 * @brief Wraps @c posix_memalign.
 * 
 * Calls @c posix_memalign with @c &tmp, @p alignment, and @p size, where @c tmp is a temporary @c void*. If @p res is
 * nonnull, the returned @c int is synchronously written to @p res. On success, the value of @c tmp is synchronously
 * written to @p ptr. On failure, the contents of @p ptr are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[out] ptr The first argument to pass to @c posix_memalign.
 * @param[in] alignment The second argument to pass to @c posix_memalign.
 * @param[in] size The third argument to pass to @c posix_memalign.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two or not a multiple of @c sizeof(void*).
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p ptr is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_POSIX_MEMALIGN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(2) CZ_WR_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_posix_memalign(int* res, void* restrict* ptr, size_t alignment, size_t size);
#endif

/**
 * @def CZ_WRAP_MADVISE
 * 
 * @brief Specifies whether @c madvise is defined.
 */
#if !defined(CZ_WRAP_MADVISE)
#if (                                                  \
		CZ_DARWIN &&                                   \
		CZ_DARWIN_C_SOURCE) ||                         \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_DEFAULT_SOURCE &&                           \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 19)) || \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_BSD_SOURCE &&                               \
		CZ_GLIBC &&                                    \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 20)) ||  \
	CZ_FREE_BSD
#define CZ_WRAP_MADVISE 1
#else
#define CZ_WRAP_MADVISE 0
#endif
#endif

#if CZ_WRAP_MADVISE
/**
 * @brief Wraps @c madvise.
 * 
 * Calls @c madvise with @p addr, @p size, and @p advice.
 * 
 * @param[in,out] addr The first argument to pass to @c madvise.
 * @param[in] size The second argument to pass to @c madvise.
 * @param[in] advice The third argument to pass to @c madvise.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to enact @p advice on the address range was denied.
 * @retval CZ_RESULT_BAD_ADDRESS The address range included invalid or unallocated memory.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr was not page-aligned.
 * @retval CZ_RESULT_IN_USE The address range was already in use by the system.
 * @retval CZ_RESULT_NO_FILE The address range does not map a file.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of cgroups was reached.
 * @retval CZ_RESULT_NO_SUPPORT @p advice was invalid or unsupported by the platform.
 * 
 * @pre @p addr is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MADVISE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1, 2)
enum CzResult czWrap_madvise(void* addr, size_t size, int advice);
#endif

/**
 * @def CZ_WRAP_POSIX_MADVISE
 * 
 * @brief Specifies whether @c posix_madvise is defined.
 */
#if !defined(CZ_WRAP_POSIX_MADVISE)
#if (                                                  \
		CZ_DARWIN &&                                   \
		CZ_MACOS_VERSION >= CZ_MAKE_VERSION(10, 2)) || \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		(                                              \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||      \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&         \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 2)) ||  \
	CZ_FREE_BSD ||                                     \
	CZ_POSIX_ADVISORY_INFO >= CZ_POSIX_2001
#define CZ_WRAP_POSIX_MADVISE 1
#else
#define CZ_WRAP_POSIX_MADVISE 0
#endif
#endif

#if CZ_WRAP_POSIX_MADVISE
/**
 * @brief Wraps @c posix_madvise.
 * 
 * Calls @c posix_madvise with @p addr, @p size, and @p advice. If @p res is nonnull, the returned @c int is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] addr The first argument to pass to @c posix_madvise.
 * @param[in] size The second argument to pass to @c posix_madvise.
 * @param[in] advice The third argument to pass to @c posix_madvise.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to enact @p advice on the address range was denied.
 * @retval CZ_RESULT_BAD_ADDRESS The address range included invalid or unallocated memory.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr was not page-aligned.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_NO_SUPPORT @p advice was invalid or unsupported by the platform.
 * 
 * @pre @p addr is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_POSIX_MADVISE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(2) CZ_WR_ACCESS(1) CZ_NO_ACCESS(2, 3)
enum CzResult czWrap_posix_madvise(int* res, void* addr, size_t size, int advice);
#endif

/**
 * @brief Wraps @c fopen.
 * 
 * Calls @c fopen with @p path and @p mode. On success, the returned @c FILE* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] path The first argument to pass to @c fopen.
 * @param[in] mode The second argument to pass to @c fopen.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p mode was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when creating the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files or streams was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p res is nonnull.
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p mode is nonnull and NUL-terminated.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_NULTERM_ARG(3) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2) CZ_RD_ACCESS(3)
enum CzResult czWrap_fopen(FILE* restrict* res, const char* path, const char* mode);

/**
 * @def CZ_WRAP_FDOPEN
 * 
 * @brief Specifies whether @c fdopen is defined.
 */
#if !defined(CZ_WRAP_FDOPEN)
#if (                                               \
		CZ_DARWIN &&                                \
		(                                           \
			CZ_DARWIN_C_SOURCE ||                   \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988)) || \
	(                                               \
		CZ_GNU_LINUX &&                             \
		(                                           \
			CZ_POSIX_C_SOURCE ||                    \
			CZ_XOPEN_SOURCE) &&                     \
		CZ_GLIBC) ||                                \
	CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_FDOPEN 1
#else
#define CZ_WRAP_FDOPEN 0
#endif
#endif

#if CZ_WRAP_FDOPEN
/**
 * @brief Wraps @c fdopen.
 * 
 * Calls @c fdopen with @p fd and @p mode. On success, the returned @c FILE* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c fdopen.
 * @param[in] mode The second argument to pass to @c fdopen.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open streams was reached.
 * 
 * @pre @p res is nonnull.
 * @pre @p fd is an open file descriptor.
 * @pre @p mode is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FDOPEN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(3) CZ_FILDES(2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(3)
enum CzResult czWrap_fdopen(FILE* restrict* res, int fd, const char* mode);
#endif

/**
 * @brief Wraps @c freopen.
 * 
 * Calls @c freopen with @p path, @p mode, and @p stream.
 * 
 * @param[in] path The first argument to pass to @c freopen.
 * @param[in] mode The second argument to pass to @c freopen.
 * @param[in,out] stream The third argument to pass to @c freopen.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path, @p mode, or @p stream was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing, closing, or creating the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files or streams was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p path is NUL-terminated.
 * @pre @p mode is nonnull and NUL-terminated.
 */
CZ_NONNULL_ARGS(2, 3) CZ_NULTERM_ARG(1) CZ_NULTERM_ARG(2) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2) CZ_RW_ACCESS(3)
enum CzResult czWrap_freopen(const char* path, const char* mode, FILE* stream);

/**
 * @def CZ_WRAP_FMEMOPEN
 * 
 * @brief Specifies whether @c fmemopen is defined.
 */
#if !defined(CZ_WRAP_FMEMOPEN)
#if (                                                      \
		CZ_DARWIN &&                                       \
		(                                                  \
			CZ_DARWIN_C_SOURCE ||                          \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008) &&         \
		(                                                  \
			CZ_MACOS_VERSION >= CZ_MAKE_VERSION(10, 13) || \
			CZ_IOS_VERSION >= CZ_MAKE_VERSION(11, 0))) ||  \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		CZ_GNU_SOURCE &&                                   \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(1, 1) &&       \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 10)) ||      \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		(                                                  \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 ||          \
			CZ_XOPEN_SOURCE >= CZ_SUS_2008) &&             \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 10)) ||     \
	CZ_FREE_BSD ||                                         \
	CZ_POSIX_VERSION >= CZ_POSIX_2008
#define CZ_WRAP_FMEMOPEN 1
#else
#define CZ_WRAP_FMEMOPEN 0
#endif
#endif

#if CZ_WRAP_FMEMOPEN
/**
 * @brief Wraps @c fmemopen.
 * 
 * Calls @c fmemopen with @p buffer, @p size, and @p mode. On success, the returned @c FILE* is synchronously written to
 * @p res. On failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] buffer The first argument to pass to @c fmemopen.
 * @param[in] size The second argument to pass to @c fmemopen.
 * @param[in] mode The third argument to pass to @c fmemopen.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p mode was invalid.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open streams was reached.
 * 
 * @pre @p res is nonnull.
 * @pre @p mode is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FMEMOPEN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 4) CZ_NULTERM_ARG(4) CZ_WR_ACCESS(1) CZ_NO_ACCESS(2, 3) CZ_RD_ACCESS(4)
enum CzResult czWrap_fmemopen(FILE* restrict* res, void* buffer, size_t size, const char* mode);
#endif

/**
 * @brief Wraps @c fclose.
 * 
 * Calls @c fclose with @p stream.
 * 
 * @param[in] stream The argument to pass to @c fclose.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing or closing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p stream is nonnull.
 */
CZ_NONNULL_ARGS()
enum CzResult czWrap_fclose(FILE* stream);

/**
 * @brief Wraps @c fseek.
 * 
 * Calls @c fseek with @p stream, @p offset, and @p whence.
 * 
 * @param[in,out] stream The first argument to pass to @c fseek.
 * @param[in] offset The second argument to pass to @c fseek.
 * @param[in] whence The third argument to pass to @c fseek.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_OFFSET @p whence or the resultant file offset was invalid.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p stream is nonnull.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
enum CzResult czWrap_fseek(FILE* stream, long offset, int whence);

/**
 * @def CZ_WRAP_FSEEKO
 * 
 * @brief Specifies whether @c fseeko is defined.
 */
#if !defined(CZ_WRAP_FSEEKO)
#if (                                                 \
		CZ_DARWIN &&                                  \
		(                                             \
			CZ_DARWIN_C_SOURCE ||                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001)) ||   \
	(                                                 \
		CZ_GNU_LINUX &&                               \
		(                                             \
			CZ_FILE_OFFSET_BITS == 64 ||              \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||     \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&        \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 1)) || \
	CZ_FREE_BSD ||                                    \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_FSEEKO 1
#else
#define CZ_WRAP_FSEEKO 0
#endif
#endif

#if CZ_WRAP_FSEEKO
/**
 * @brief Wraps @c fseeko.
 * 
 * Calls @c fseeko with @p stream, @p offset, and @p whence.
 * 
 * @param[in,out] stream The first argument to pass to @c fseeko.
 * @param[in] offset The second argument to pass to @c fseeko.
 * @param[in] whence The third argument to pass to @c fseeko.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_OFFSET @p whence or the resultant file offset was invalid.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FSEEKO is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
enum CzResult czWrap_fseeko(FILE* stream, off_t offset, int whence);
#endif

/**
 * @brief Wraps @c ftell.
 * 
 * Calls @c ftell with @p stream. On success, the returned @c long is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] stream The argument to pass to @c ftell.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p res is nonnull.
 * @pre @p stream is nonnull.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1) CZ_RW_ACCESS(2)
enum CzResult czWrap_ftell(long* res, FILE* stream);

/**
 * @def CZ_WRAP_FTELLO
 * 
 * @brief Specifies whether @c ftello is defined.
 */
#if !defined(CZ_WRAP_FTELLO)
#if (                                                 \
		CZ_DARWIN &&                                  \
		(                                             \
			CZ_DARWIN_C_SOURCE ||                     \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001)) ||   \
	(                                                 \
		CZ_GNU_LINUX &&                               \
		(                                             \
			CZ_FILE_OFFSET_BITS == 64 ||              \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||     \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&        \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 1)) || \
	CZ_FREE_BSD ||                                    \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_FTELLO 1
#else
#define CZ_WRAP_FTELLO 0
#endif
#endif

#if CZ_WRAP_FTELLO
/**
 * @brief Wraps @c ftello.
 * 
 * Calls @c ftello with @p stream. On success, the returned @c off_t is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] stream The argument to pass to @c ftello.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p res is nonnull.
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FTELLO is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1) CZ_RW_ACCESS(2)
enum CzResult czWrap_ftello(off_t* res, FILE* stream);
#endif

/**
 * @brief Wraps @c fgetpos.
 * 
 * Calls @c fgetpos with @p stream and @p pos.
 * 
 * @param[in] stream The first argument to pass to @c fgetpos.
 * @param[out] pos The second argument to pass to @c fgetpos.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p stream or @p pos was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p stream is nonnull.
 * @pre @p pos is nonnull.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_fgetpos(FILE* stream, fpos_t* pos);

/**
 * @brief Wraps @c fsetpos.
 * 
 * Calls @c fsetpos with @p stream and @p pos.
 * 
 * @param[in,out] stream The first argument to pass to @c fsetpos.
 * @param[in] pos The second argument to pass to @c fsetpos.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p stream or @p pos was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p stream is nonnull.
 * @pre @p pos is nonnull.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2)
enum CzResult czWrap_fsetpos(FILE* stream, const fpos_t* pos);

/**
 * @brief Wraps @c rewind.
 * 
 * Calls @c rewind with @p stream.
 * 
 * @param[in,out] stream The argument to pass to @c rewind.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p stream is nonnull.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
enum CzResult czWrap_rewind(FILE* stream);

/**
 * @brief Wraps @c fread.
 * 
 * Calls @c fread with @p buffer, @p size, @p count, and @p stream. If @p res is nonnull, the returned @c size_t is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[out] buffer The first argument to pass to @c fread.
 * @param[in] size The second argument to pass to @c fread.
 * @param[in] count The third argument to pass to @c fread.
 * @param[in,out] stream The fourth argument to pass to @c fread.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_OFFSET The file was already at @c EOF.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p buffer is nonnull.
 * @pre @p stream is nonnull.
 */
CZ_NONNULL_ARGS(2, 5) CZ_WR_ACCESS(1) CZ_WR_ACCESS(2) CZ_RW_ACCESS(5)
enum CzResult czWrap_fread(size_t* res, void* buffer, size_t size, size_t count, FILE* stream);

/**
 * @brief Wraps @c fwrite.
 * 
 * Calls @c fwrite with @p buffer, @p size, @p count, and @p stream. If @p res is nonnull, the returned @c size_t is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] buffer The first argument to pass to @c fwrite.
 * @param[in] size The second argument to pass to @c fwrite.
 * @param[in] count The third argument to pass to @c fwrite.
 * @param[in,out] stream The fourth argument to pass to @c fwrite.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when writing to the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p buffer is nonnull.
 * @pre @p stream is nonnull.
 */
CZ_NONNULL_ARGS(2, 5) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2) CZ_RW_ACCESS(5)
enum CzResult czWrap_fwrite(size_t* res, const void* buffer, size_t size, size_t count, FILE* stream);

/**
 * @brief Wraps @c fflush.
 * 
 * Calls @c fflush with @p stream.
 * 
 * @param[in,out] stream The argument to pass to @c fflush.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 */
CZ_RW_ACCESS(1)
enum CzResult czWrap_fflush(FILE* stream);

/**
 * @brief Wraps @c remove.
 * 
 * Calls @c remove with @p path.
 * 
 * @param[in] path The argument to pass to @c remove.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to delete the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when deleting the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czWrap_remove(const char* path);

/**
 * @def CZ_WRAP_RMDIR
 * 
 * @brief Specifies whether @c rmdir is defined.
 */
#if !defined(CZ_WRAP_RMDIR)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_RMDIR 1
#else
#define CZ_WRAP_RMDIR 0
#endif
#endif

#if CZ_WRAP_RMDIR
/**
 * @brief Wraps @c rmdir.
 * 
 * Calls @c rmdir with @p path.
 * 
 * @param[in] path The argument to pass to @c rmdir.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to delete the directory was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was not an empty directory.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when deleting the directory.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The directory was already in use by the system.
 * @retval CZ_RESULT_NO_FILE The directory did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_RMDIR is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czWrap_rmdir(const char* path);
#endif

/**
 * @def CZ_WRAP_UNLINK
 * 
 * @brief Specifies whether @c unlink is defined.
 */
#if !defined(CZ_WRAP_UNLINK)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_UNLINK 1
#else
#define CZ_WRAP_UNLINK 0
#endif
#endif

#if CZ_WRAP_UNLINK
/**
 * @brief Wraps @c unlink.
 * 
 * Calls @c unlink with @p path.
 * 
 * @param[in] path The argument to pass to @c unlink.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to delete the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when deleting the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_UNLINK is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czWrap_unlink(const char* path);
#endif

/**
 * @def CZ_WRAP_UNLINKAT
 * 
 * @brief Specifies whether @c unlinkat is defined.
 */
#if !defined(CZ_WRAP_UNLINKAT)
#if (                                                      \
		CZ_DARWIN &&                                       \
		(                                                  \
			CZ_DARWIN_C_SOURCE ||                          \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008) &&         \
		(                                                  \
			CZ_MACOS_VERSION >= CZ_MAKE_VERSION(10, 10) || \
			CZ_IOS_VERSION >= CZ_MAKE_VERSION(8, 0))) ||   \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		CZ_ATFILE_SOURCE &&                                \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 4) &&       \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 10)) ||      \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		(                                                  \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 ||          \
			CZ_XOPEN_SOURCE >= CZ_SUS_2008) &&             \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 10)) ||     \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0) ||        \
	CZ_POSIX_VERSION >= CZ_POSIX_2008
#define CZ_WRAP_UNLINKAT 1
#else
#define CZ_WRAP_UNLINKAT 0
#endif
#endif

#if CZ_WRAP_UNLINKAT
/**
 * @brief Wraps @c unlinkat.
 * 
 * Calls @c unlinkat with @p fd, @p path, and @p flags.
 * 
 * @param[in] fd The first argument to pass to @c unlinkat.
 * @param[in] path The second argument to pass to @c unlinkat.
 * @param[in] flags The third argument to pass to @c unlinkat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to delete the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when deleting the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p fd is @c AT_FDCWD or an open file descriptor.
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_UNLINKAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_RD_ACCESS(2)
enum CzResult czWrap_unlinkat(int fd, const char* path, int flags);
#endif

/**
 * @def CZ_WRAP_FILENO
 * 
 * @brief Specifies whether @c fileno is defined.
 */
#if !defined(CZ_WRAP_FILENO)
#if (                                               \
		CZ_DARWIN &&                                \
		(                                           \
			CZ_DARWIN_C_SOURCE ||                   \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1988)) || \
	(                                               \
		CZ_GNU_LINUX &&                             \
		CZ_POSIX_C_SOURCE &&                        \
		CZ_GLIBC) ||                                \
	CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_FILENO 1
#else
#define CZ_WRAP_FILENO 0
#endif
#endif

#if CZ_WRAP_FILENO
/**
 * @brief Wraps @c fileno.
 * 
 * Calls @c fileno with @p stream. On success, the returned @c int is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] stream The argument to pass to @c fileno.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * 
 * @pre @p res is nonnull.
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FILENO is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1) CZ_RW_ACCESS(2)
enum CzResult czWrap_fileno(int* res, FILE* stream);
#endif

/**
 * @def CZ_WRAP_ISATTY
 * 
 * @brief Specifies whether @c isatty is defined.
 */
#if !defined(CZ_WRAP_ISATTY)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_ISATTY 1
#else
#define CZ_WRAP_ISATTY 0
#endif
#endif

#if CZ_WRAP_ISATTY
/**
 * @brief Wraps @c isatty.
 * 
 * Calls @c isatty with @p fd. On success, the returned @c int is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The argument to pass to @c isatty.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * 
 * @pre @p res is nonnull.
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ISATTY is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1) CZ_FILDES(2)
enum CzResult czWrap_isatty(int* res, int fd);
#endif

/**
 * @def CZ_WRAP_STAT
 * 
 * @brief Specifies whether @c stat is defined.
 */
#if !defined(CZ_WRAP_STAT)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_STAT 1
#else
#define CZ_WRAP_STAT 0
#endif
#endif

#if CZ_WRAP_STAT
/**
 * @brief Wraps @c stat.
 * 
 * Calls @c stat with @p path and @p st.
 * 
 * @param[in] path The first argument to pass to @c stat.
 * @param[out] st The second argument to pass to @c stat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p st was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p st is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_STAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_stat(const char* path, struct stat* st);
#endif

/**
 * @def CZ_WRAP_LSTAT
 * 
 * @brief Specifies whether @c lstat is defined.
 */
#if !defined(CZ_WRAP_LSTAT)
#if (                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&              \
		CZ_GLIBC) ||                                   \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 10)) || \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_BSD_SOURCE &&                               \
		CZ_GLIBC &&                                    \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 20)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_DEFAULT_SOURCE &&                           \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 20)) || \
	CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_LSTAT 1
#else
#define CZ_WRAP_LSTAT 0
#endif
#endif

#if CZ_WRAP_LSTAT
/**
 * @brief Wraps @c lstat.
 * 
 * Calls @c lstat with @p path and @p st.
 * 
 * @param[in] path The first argument to pass to @c lstat.
 * @param[out] st The second argument to pass to @c lstat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p st was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p st is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_LSTAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_lstat(const char* path, struct stat* st);
#endif

/**
 * @def CZ_WRAP_FSTAT
 * 
 * @brief Specifies whether @c fstat is defined.
 */
#if !defined(CZ_WRAP_FSTAT)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_FSTAT 1
#else
#define CZ_WRAP_FSTAT 0
#endif
#endif

#if CZ_WRAP_FSTAT
/**
 * @brief Wraps @c fstat.
 * 
 * Calls @c fstat with @p fd and @p st.
 * 
 * @param[in] fd The first argument to pass to @c fstat.
 * @param[out] st The second argument to pass to @c fstat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_ADDRESS @p st was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p fd is an open file descriptor.
 * @pre @p st is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FSTAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_FILDES(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_fstat(int fd, struct stat* st);
#endif

/**
 * @def CZ_WRAP_FSTATAT
 * 
 * @brief Specifies whether @c fstatat is defined.
 */
#if !defined(CZ_WRAP_FSTATAT)
#if (                                                      \
		CZ_DARWIN &&                                       \
		(                                                  \
			CZ_DARWIN_C_SOURCE ||                          \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008) &&         \
		(                                                  \
			CZ_MACOS_VERSION >= CZ_MAKE_VERSION(10, 10) || \
			CZ_IOS_VERSION >= CZ_MAKE_VERSION(8, 0))) ||   \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		CZ_ATFILE_SOURCE &&                                \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 4) &&       \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 10)) ||      \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		(                                                  \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 ||          \
			CZ_XOPEN_SOURCE >= CZ_SUS_2008) &&             \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 10)) ||     \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0) ||        \
	CZ_POSIX_VERSION >= CZ_POSIX_2008
#define CZ_WRAP_FSTATAT 1
#else
#define CZ_WRAP_FSTATAT 0
#endif
#endif

#if CZ_WRAP_FSTATAT
/**
 * @brief Wraps @c fstatat.
 * 
 * Calls @c fstatat with @p fd, @p path, @p st, and @p flag.
 * 
 * @param[in] fd The first argument to pass to @c fstatat.
 * @param[in] path The second argument to pass to @c fstatat.
 * @param[out] st The third argument to pass to @c fstatat.
 * @param[in] flag The fourth argument to pass to @c fstatat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p st was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p fd is @c AT_FDCWD or an open file descriptor.
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p st is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FSTATAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_RD_ACCESS(2) CZ_WR_ACCESS(3)
enum CzResult czWrap_fstatat(int fd, const char* path, struct stat* st, int flag);
#endif

/**
 * @def CZ_WRAP_FLOCK
 * 
 * @brief Specifies whether @c flock is defined.
 */
#if !defined(CZ_WRAP_FLOCK)
#if (                          \
		CZ_DARWIN &&           \
		CZ_DARWIN_C_SOURCE) || \
	(                          \
		CZ_GNU_LINUX &&        \
		CZ_GLIBC) ||           \
	CZ_FREE_BSD
#define CZ_WRAP_FLOCK 1
#else
#define CZ_WRAP_FLOCK 0
#endif
#endif

#if CZ_WRAP_FLOCK
/**
 * @brief Wraps @c flock.
 * 
 * Calls @c flock with @p fd and @p op.
 * 
 * @param[in] fd The first argument to pass to @c flock.
 * @param[in] op The second argument to pass to @c flock.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_IN_USE The file was already locked.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_LOCK No file locks were available.
 * @retval CZ_RESULT_NO_SUPPORT @p op was invalid or unsupported by the platform.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FLOCK is defined as a nonzero value.
 */
CZ_FILDES(1)
enum CzResult czWrap_flock(int fd, int op);
#endif

/**
 * @def CZ_WRAP_LOCKF
 * 
 * @brief Specifies whether @c lockf is defined.
 */
#if !defined(CZ_WRAP_LOCKF)
#if (                                                  \
		CZ_DARWIN &&                                   \
		(                                              \
			CZ_DARWIN_C_SOURCE ||                      \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996)) ||    \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&              \
		CZ_GLIBC) ||                                   \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		(                                              \
			CZ_BSD_SOURCE ||                           \
			CZ_SVID_SOURCE) &&                         \
		CZ_GLIBC &&                                    \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 20)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_DEFAULT_SOURCE &&                           \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 19)) || \
	CZ_FREE_BSD ||                                     \
	(                                                  \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 &&             \
		CZ_XOPEN_UNIX > 0) ||                          \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||                 \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_LOCKF 1
#else
#define CZ_WRAP_LOCKF 0
#endif
#endif

#if CZ_WRAP_LOCKF
/**
 * @brief Wraps @c lockf.
 * 
 * Calls @c lockf with @p fd and @p op.
 * 
 * @param[in] fd The first argument to pass to @c lockf.
 * @param[in] func The second argument to pass to @c lockf.
 * @param[in] size The third argument to pass to @c lockf.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_RANGE The file section extended past the maximum file size.
 * @retval CZ_RESULT_BAD_SIZE The sum of @p size and the current file offset was negative.
 * @retval CZ_RESULT_DEADLOCK The file lock could cause a deadlock.
 * @retval CZ_RESULT_IN_USE The file section was already locked.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_LOCK No file locks were available.
 * @retval CZ_RESULT_NO_SUPPORT @p func was invalid or unsupported by the platform.
 * 
 * @pre @p fd is an open file descriptor with write access.
 * 
 * @note This function is only defined if @ref CZ_WRAP_LOCKF is defined as a nonzero value.
 */
CZ_WR_FILDES(1)
enum CzResult czWrap_lockf(int fd, int func, off_t size);
#endif

/**
 * @def CZ_WRAP_FCNTL
 * 
 * @brief Specifies whether @c fcntl is defined.
 */
#if !defined(CZ_WRAP_FCNTL)
#if CZ_DARWIN ||        \
	(                   \
		CZ_GNU_LINUX && \
		CZ_GLIBC) ||    \
	CZ_FREE_BSD ||      \
	CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_FCNTL 1
#else
#define CZ_WRAP_FCNTL 0
#endif
#endif

#if CZ_WRAP_FCNTL
/**
 * @brief Wraps @c fcntl.
 * 
 * Calls @c fcntl with @p fd, @p cmd, and 0-1 variadic arguments. On success, the returned @c int is synchronously
 * written to @p res. On failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c fcntl.
 * @param[in] cmd The second argument to pass to @c fcntl.
 * @param[in] ... The variadic arguments to pass to @c fcntl.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to enact @p cmd on the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS A variadic argument was an invalid pointer.
 * @retval CZ_RESULT_BAD_ALIGNMENT A variadic argument had an invalid alignment.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET The file offset was invalid.
 * @retval CZ_RESULT_BAD_RANGE The file section extended past the maximum file size.
 * @retval CZ_RESULT_BAD_SIZE The requested size or capacity was invalid.
 * @retval CZ_RESULT_DEADLOCK The file lock could cause a deadlock.
 * @retval CZ_RESULT_IN_USE The file section was already locked.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_LOCK No file locks were available.
 * @retval CZ_RESULT_NO_OPEN The requested file descriptor values were unavailable.
 * @retval CZ_RESULT_NO_PROCESS The process or process group did not exist.
 * @retval CZ_RESULT_NO_SUPPORT @p cmd was invalid or unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p fd is an open file descriptor.
 * @pre Any variadic arguments of pointer type are nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FCNTL is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_FILDES(2)
enum CzResult czWrap_fcntl(int* res, int fd, int cmd, ...);
#endif

/**
 * @def CZ_WRAP_TRUNCATE
 * 
 * @brief Specifies whether @c truncate is defined.
 */
#if !defined(CZ_WRAP_TRUNCATE)
#if (                                                  \
		CZ_DARWIN &&                                   \
		(                                              \
			CZ_DARWIN_C_SOURCE ||                      \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996)) ||    \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&              \
		CZ_GLIBC) ||                                   \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		(                                              \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 ||      \
			CZ_XOPEN_SOURCE >= CZ_SUS_2008) &&         \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_BSD_SOURCE &&                               \
		CZ_GLIBC &&                                    \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 20)) ||  \
	CZ_FREE_BSD ||                                     \
	(                                                  \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 &&             \
		CZ_XOPEN_UNIX > 0) ||                          \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||                 \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_TRUNCATE 1
#else
#define CZ_WRAP_TRUNCATE 0
#endif
#endif

#if CZ_WRAP_TRUNCATE
/**
 * @brief Wraps @c truncate.
 * 
 * Calls @c truncate with @p path and @p size.
 * 
 * @param[in] path The first argument to pass to @c truncate.
 * @param[in] size The second argument to pass to @c truncate.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to truncate the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when truncating the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was negative or too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_TRUNCATE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czWrap_truncate(const char* path, off_t size);
#endif

/**
 * @def CZ_WRAP_FTRUNCATE
 * 
 * @brief Specifies whether @c ftruncate is defined.
 */
#if !defined(CZ_WRAP_FTRUNCATE)
#if (                                                    \
		CZ_DARWIN &&                                     \
		(                                                \
			CZ_DARWIN_C_SOURCE ||                        \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996)) ||      \
	(                                                    \
		CZ_GNU_LINUX &&                                  \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&                \
		CZ_GLIBC) ||                                     \
	(                                                    \
		CZ_GNU_LINUX &&                                  \
		(                                                \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||        \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&           \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 3, 5)) || \
	(                                                    \
		CZ_GNU_LINUX &&                                  \
		CZ_BSD_SOURCE &&                                 \
		CZ_GLIBC &&                                      \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 20)) ||    \
	CZ_FREE_BSD ||                                       \
	(                                                    \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 &&               \
		CZ_XOPEN_UNIX > 0) ||                            \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||                   \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_FTRUNCATE 1
#else
#define CZ_WRAP_FTRUNCATE 0
#endif
#endif

#if CZ_WRAP_FTRUNCATE
/**
 * @brief Wraps @c ftruncate.
 * 
 * Calls @c ftruncate with @p fd and @p size.
 * 
 * @param[in] fd The first argument to pass to @c ftruncate.
 * @param[in] size The second argument to pass to @c ftruncate.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to truncate the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when truncating the file.
 * @retval CZ_RESULT_BAD_SIZE @p size was negative or too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * 
 * @pre @p fd is an open file descriptor with write access.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FTRUNCATE is defined as a nonzero value.
 */
CZ_WR_FILDES(1)
enum CzResult czWrap_ftruncate(int fd, off_t size);
#endif

/**
 * @def CZ_WRAP_POSIX_FADVISE
 * 
 * @brief Specifies whether @c posix_fadvise is defined.
 */
#if !defined(CZ_WRAP_POSIX_FADVISE)
#if (                                                 \
		CZ_GNU_LINUX &&                               \
		(                                             \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||     \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&        \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 2)) || \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(9, 1) ||   \
	CZ_POSIX_ADVISORY_INFO >= CZ_POSIX_2001
#define CZ_WRAP_POSIX_FADVISE 1
#else
#define CZ_WRAP_POSIX_FADVISE 0
#endif
#endif

#if CZ_WRAP_POSIX_FADVISE
/**
 * @brief Wraps @c posix_fadvise.
 * 
 * Calls @c posix_fadvise with @p fd, @p offset, @p size, and @p advice. If @p res is nonnull, the returned @c int is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c posix_fadvise.
 * @param[in] offset The second argument to pass to @c posix_fadvise.
 * @param[in] size The third argument to pass to @c posix_fadvise.
 * @param[in] advice The fourth argument to pass to @c posix_fadvise.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when enacting @p advice.
 * @retval CZ_RESULT_BAD_SIZE @p size was negative.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_POSIX_FADVISE is defined as a nonzero value.
 */
CZ_WR_ACCESS(1) CZ_FILDES(2)
enum CzResult czWrap_posix_fadvise(int* res, int fd, off_t offset, off_t size, int advice);
#endif

/**
 * @def CZ_WRAP_FALLOCATE
 * 
 * @brief Specifies whether @c fallocate is defined.
 */
#if !defined(CZ_WRAP_FALLOCATE)
#if CZ_GNU_LINUX &&  \
	CZ_GNU_SOURCE && \
	CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 10)
#define CZ_WRAP_FALLOCATE 1
#else
#define CZ_WRAP_FALLOCATE 0
#endif
#endif

#if CZ_WRAP_FALLOCATE
/**
 * @brief Wraps @c fallocate.
 * 
 * Calls @c fallocate with @p fd, @p mode, @p offset, and @p size.
 * 
 * @param[in] fd The first argument to pass to @c fallocate.
 * @param[in] mode The second argument to pass to @c fallocate.
 * @param[in] offset The third argument to pass to @c fallocate.
 * @param[in] size The fourth argument to pass to @c fallocate.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p offset or @p size was not a multiple of the block size.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was negative.
 * @retval CZ_RESULT_BAD_RANGE (@p offset + @p size) was too large.
 * @retval CZ_RESULT_BAD_SIZE @p size was nonpositive.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the filesystem or platform.
 * 
 * @pre @p fd is an open file descriptor with write access.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FALLOCATE is defined as a nonzero value.
 */
CZ_WR_FILDES(1)
enum CzResult czWrap_fallocate(int fd, int mode, off_t offset, off_t size);
#endif

/**
 * @def CZ_WRAP_POSIX_FALLOCATE
 * 
 * @brief Specifies whether @c posix_fallocate is defined.
 */
#if !defined(CZ_WRAP_POSIX_FALLOCATE)
#if (                                                     \
		CZ_GNU_LINUX &&                                   \
		(                                                 \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 ||         \
			CZ_XOPEN_SOURCE >= CZ_SUS_2001) &&            \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 1, 94)) || \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(9, 0) ||       \
	CZ_POSIX_ADVISORY_INFO >= CZ_POSIX_2001
#define CZ_WRAP_POSIX_FALLOCATE 1
#else
#define CZ_WRAP_POSIX_FALLOCATE 0
#endif
#endif

#if CZ_WRAP_POSIX_FALLOCATE
/**
 * @brief Wraps @c posix_fallocate.
 * 
 * Calls @c posix_fallocate with @p fd, @p offset, and @p size. If @p res is nonnull, the returned @c int is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c posix_fallocate.
 * @param[in] offset The second argument to pass to @c posix_fallocate.
 * @param[in] size The third argument to pass to @c posix_fallocate.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was negative.
 * @retval CZ_RESULT_BAD_RANGE (@p offset + @p size) was too large.
 * @retval CZ_RESULT_BAD_SIZE @p size was nonpositive.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the filesystem or platform.
 * 
 * @pre @p fd is an open file descriptor with write access.
 * 
 * @note This function is only defined if @ref CZ_WRAP_POSIX_FALLOCATE is defined as a nonzero value.
 */
CZ_WR_ACCESS(1) CZ_WR_FILDES(2)
enum CzResult czWrap_posix_fallocate(int* res, int fd, off_t offset, off_t size);
#endif

/**
 * @def CZ_WRAP_FSYNC
 * 
 * @brief Specifies whether @c fsync is defined.
 */
#if !defined(CZ_WRAP_FSYNC)
#if (                                                  \
		CZ_DARWIN &&                                   \
		(                                              \
			CZ_DARWIN_C_SOURCE ||                      \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996)) ||    \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2001 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 8) &&   \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 16)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		(                                              \
			CZ_BSD_SOURCE ||                           \
			CZ_XOPEN_SOURCE >= CZ_XPG_1985) &&         \
		CZ_GLIBC &&                                    \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 16)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 16)) || \
	CZ_FREE_BSD ||                                     \
	CZ_XOPEN_VERSION >= CZ_XPG_1989 ||                 \
	CZ_POSIX_FSYNC >= CZ_POSIX_2001
#define CZ_WRAP_FSYNC 1
#else
#define CZ_WRAP_FSYNC 0
#endif
#endif

#if CZ_WRAP_FSYNC
/**
 * @brief Wraps @c fsync.
 * 
 * Calls @c fsync with @p fd.
 * 
 * @param[in] fd The argument to pass to @c fsync.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FSYNC is defined as a nonzero value.
 */
CZ_FILDES(1)
enum CzResult czWrap_fsync(int fd);
#endif

/**
 * @def CZ_WRAP_FDATASYNC
 * 
 * @brief Specifies whether @c fdatasync is defined.
 */
#if !defined(CZ_WRAP_FDATASYNC)
#if (                                                \
		CZ_GNU_LINUX &&                              \
		(                                            \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1993 ||    \
			CZ_XOPEN_SOURCE >= CZ_SUS_1997) &&       \
		CZ_GLIBC) ||                                 \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(11, 1) || \
	CZ_XOPEN_REALTIME > 0
#define CZ_WRAP_FDATASYNC 1
#else
#define CZ_WRAP_FDATASYNC 0
#endif
#endif

#if CZ_WRAP_FDATASYNC
/**
 * @brief Wraps @c fdatasync.
 * 
 * Calls @c fdatasync with @p fd.
 * 
 * @param[in] fd The argument to pass to @c fdatasync.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FDATASYNC is defined as a nonzero value.
 */
CZ_FILDES(1)
enum CzResult czWrap_fdatasync(int fd);
#endif

/**
 * @def CZ_WRAP_OPEN
 * 
 * @brief Specifies whether @c open is defined.
 */
#if !defined(CZ_WRAP_OPEN)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_OPEN 1
#else
#define CZ_WRAP_OPEN 0
#endif
#endif

#if CZ_WRAP_OPEN
/**
 * @brief Wraps @c open.
 * 
 * Calls @c open with @p path, @p flags, and @p mode. On success, the returned @c int is synchronously written to
 * @p res. On failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] path The first argument to pass to @c open.
 * @param[in] flags The second argument to pass to @c open.
 * @param[in] mode The third argument to pass to @c open.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when creating the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_OPEN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2)
enum CzResult czWrap_open(int* res, const char* path, int flags, mode_t mode);
#endif

/**
 * @def CZ_WRAP_OPENAT
 * 
 * @brief Specifies whether @c openat is defined.
 */
#if !defined(CZ_WRAP_OPENAT)
#if (                                                      \
		CZ_DARWIN &&                                       \
		(                                                  \
			CZ_DARWIN_C_SOURCE ||                          \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008) &&         \
		(                                                  \
			CZ_MACOS_VERSION >= CZ_MAKE_VERSION(10, 10) || \
			CZ_IOS_VERSION >= CZ_MAKE_VERSION(8, 0))) ||   \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		CZ_ATFILE_SOURCE &&                                \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 4) &&       \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 10)) ||      \
	(                                                      \
		CZ_GNU_LINUX &&                                    \
		(                                                  \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 ||          \
			CZ_XOPEN_SOURCE >= CZ_SUS_2008) &&             \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 10)) ||     \
	CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(8, 0) ||        \
	CZ_POSIX_VERSION >= CZ_POSIX_2008
#define CZ_WRAP_OPENAT 1
#else
#define CZ_WRAP_OPENAT 0
#endif
#endif

#if CZ_WRAP_OPENAT
/**
 * @brief Wraps @c openat.
 * 
 * Calls @c openat with @p fd, @p path, @p flags, and @p mode. On success, the returned @c int is synchronously written
 * to @p res. On failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c openat.
 * @param[in] path The second argument to pass to @c openat.
 * @param[in] flags The third argument to pass to @c openat.
 * @param[in] mode The fourth argument to pass to @c openat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when creating the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p fd is @c AT_FDCWD or an open file descriptor.
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_OPENAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(3) CZ_WR_ACCESS(1) CZ_RD_ACCESS(3)
enum CzResult czWrap_openat(int* res, int fd, const char* path, int flags, mode_t mode);
#endif

/**
 * @def CZ_WRAP_CREAT
 * 
 * @brief Specifies whether @c creat is defined.
 */
#if !defined(CZ_WRAP_CREAT)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_CREAT 1
#else
#define CZ_WRAP_CREAT 0
#endif
#endif

#if CZ_WRAP_CREAT
/**
 * @brief Wraps @c creat.
 * 
 * Calls @c creat with @p path and @p mode. On success, the returned @c int is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] path The first argument to pass to @c creat.
 * @param[in] mode The third argument to pass to @c creat.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to create the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when creating the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p res is nonnull.
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CREAT is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2)
enum CzResult czWrap_creat(int* res, const char* path, mode_t mode);
#endif

/**
 * @def CZ_WRAP_CLOSE
 * 
 * @brief Specifies whether @c close is defined.
 */
#if !defined(CZ_WRAP_CLOSE)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_CLOSE 1
#else
#define CZ_WRAP_CLOSE 0
#endif
#endif

#if CZ_WRAP_CLOSE
/**
 * @brief Wraps @c close.
 * 
 * Calls @c close with @p fd.
 * 
 * @param[in] fd The argument to pass to @c close.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CLOSE is defined as a nonzero value.
 */
CZ_FILDES(1)
enum CzResult czWrap_close(int fd);
#endif

/**
 * @def CZ_WRAP_POSIX_CLOSE
 * 
 * @brief Specifies whether @c posix_close is defined.
 */
#if !defined(CZ_WRAP_POSIX_CLOSE)
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
#define CZ_WRAP_POSIX_CLOSE 1
#else
#define CZ_WRAP_POSIX_CLOSE 0
#endif
#endif

#if CZ_WRAP_POSIX_CLOSE
/**
 * @brief Wraps @c posix_close.
 * 
 * Calls @c posix_close with @p fd and @p flag.
 * 
 * @param[in] fd The first argument to pass to @c posix_close.
 * @param[in] flag The second argument to pass to @c posix_close.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd or @p flag was invalid.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_POSIX_CLOSE is defined as a nonzero value.
 */
CZ_FILDES(1)
enum CzResult czWrap_posix_close(int fd, int flag);
#endif

/**
 * @def CZ_WRAP_LSEEK
 * 
 * @brief Specifies whether @c lseek is defined.
 */
#if !defined(CZ_WRAP_LSEEK)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_LSEEK 1
#else
#define CZ_WRAP_LSEEK 0
#endif
#endif

#if CZ_WRAP_LSEEK
/**
 * @brief Wraps @c lseek.
 * 
 * Calls @c lseek with @p fd, @p offset, and @p whence. If @p res is nonnull, the returned @c off_t is synchronously
 * written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c lseek.
 * @param[in] offset The second argument to pass to @c lseek.
 * @param[in] whence The third argument to pass to @c lseek.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid file descriptor.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p whence or the resultant file offset was invalid.
 * 
 * @pre @p fd is an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_LSEEK is defined as a nonzero value.
 */
CZ_WR_ACCESS(1) CZ_FILDES(2)
enum CzResult czWrap_lseek(off_t* res, int fd, off_t offset, int whence);
#endif

/**
 * @def CZ_WRAP_READ
 * 
 * @brief Specifies whether @c read is defined.
 */
#if !defined(CZ_WRAP_READ)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_READ 1
#else
#define CZ_WRAP_READ 0
#endif
#endif

#if CZ_WRAP_READ
/**
 * @brief Wraps @c read.
 * 
 * Calls @c read with @p fd, @p buffer, and @p size. If @p res is nonnull, the returned @c ssize_t is synchronously
 * written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c read.
 * @param[out] buffer The second argument to pass to @c read.
 * @param[in] size The third argument to pass to @c read.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_OFFSET The file was already at @c EOF.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file was empty or deleted.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_TIMEOUT A system operation timed out.
 * 
 * @pre @p fd is an open file descriptor with read access.
 * @pre @p buffer is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_READ is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(3) CZ_WR_ACCESS(1) CZ_RD_FILDES(2) CZ_WR_ACCESS(3, 4)
enum CzResult czWrap_read(ssize_t* res, int fd, void* buffer, size_t size);
#endif

/**
 * @def CZ_WRAP_PREAD
 * 
 * @brief Specifies whether @c pread is defined.
 */
#if !defined(CZ_WRAP_PREAD)
#if (                                                  \
		CZ_DARWIN &&                                   \
		(                                              \
			CZ_DARWIN_C_SOURCE ||                      \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996)) ||    \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&              \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 1)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||                 \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_PREAD 1
#else
#define CZ_WRAP_PREAD 0
#endif
#endif

#if CZ_WRAP_PREAD
/**
 * @brief Wraps @c pread.
 * 
 * Calls @c pread with @p fd, @p buffer, @p size, and @p offset. If @p res is nonnull, the returned @c ssize_t is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c pread.
 * @param[out] buffer The second argument to pass to @c pread.
 * @param[in] size The third argument to pass to @c pread.
 * @param[in] offset The fourth argument to pass to @c pread.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file was empty or deleted.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_TIMEOUT A system operation timed out.
 * 
 * @pre @p fd is an open file descriptor with read access.
 * @pre @p buffer is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_PREAD is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(3) CZ_WR_ACCESS(1) CZ_RD_FILDES(2) CZ_WR_ACCESS(3, 4)
enum CzResult czWrap_pread(ssize_t* res, int fd, void* buffer, size_t size, off_t offset);
#endif

/**
 * @def CZ_WRAP_WRITE
 * 
 * @brief Specifies whether @c write is defined.
 */
#if !defined(CZ_WRAP_WRITE)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_WRITE 1
#else
#define CZ_WRAP_WRITE 0
#endif
#endif

#if CZ_WRAP_WRITE
/**
 * @brief Wraps @c write.
 * 
 * Calls @c write with @p fd, @p buffer, and @p size. If @p res is nonnull, the returned @c ssize_t is synchronously
 * written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c write.
 * @param[in] buffer The second argument to pass to @c write.
 * @param[in] size The third argument to pass to @c write.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when writing to the file.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p fd is an open file descriptor with write access.
 * @pre @p buffer is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_WRITE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(3) CZ_WR_ACCESS(1) CZ_WR_FILDES(2) CZ_RD_ACCESS(3, 4)
enum CzResult czWrap_write(ssize_t* res, int fd, const void* buffer, size_t size);
#endif

/**
 * @def CZ_WRAP_PWRITE
 * 
 * @brief Specifies whether @c pwrite is defined.
 */
#if !defined(CZ_WRAP_PWRITE)
#if (                                                  \
		CZ_DARWIN &&                                   \
		(                                              \
			CZ_DARWIN_C_SOURCE ||                      \
			CZ_POSIX_C_SOURCE >= CZ_POSIX_1996)) ||    \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_XOPEN_SOURCE >= CZ_SUS_1997 &&              \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 1)) ||  \
	(                                                  \
		CZ_GNU_LINUX &&                                \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||                 \
	CZ_POSIX_VERSION >= CZ_POSIX_2001
#define CZ_WRAP_PWRITE 1
#else
#define CZ_WRAP_PWRITE 0
#endif
#endif

#if CZ_WRAP_PWRITE
/**
 * @brief Wraps @c pwrite.
 * 
 * Calls @c pwrite with @p fd, @p buffer, @p size, and @p offset. If @p res is nonnull, the returned @c ssize_t is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The first argument to pass to @c pwrite.
 * @param[in] buffer The second argument to pass to @c pwrite.
 * @param[in] size The third argument to pass to @c pwrite.
 * @param[in] offset The fourth argument to pass to @c pwrite.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when writing to the file.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p fd is an open file descriptor with write access.
 * @pre @p buffer is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_PWRITE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(3) CZ_WR_ACCESS(1) CZ_WR_FILDES(2) CZ_RD_ACCESS(3, 4)
enum CzResult czWrap_pwrite(ssize_t* res, int fd, const void* buffer, size_t size, off_t offset);
#endif

/**
 * @def CZ_WRAP_MMAP
 * 
 * @brief Specifies whether @c mmap is defined.
 */
#if !defined(CZ_WRAP_MMAP)
#if (                                         \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 &&    \
		CZ_XOPEN_UNIX > 0) ||                 \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||        \
	CZ_POSIX_MAPPED_FILES >= CZ_POSIX_2001 || \
	CZ_POSIX_SHARED_MEMORY_OBJECTS >= CZ_POSIX_2001
#define CZ_WRAP_MMAP 1
#else
#define CZ_WRAP_MMAP 0
#endif
#endif

#if CZ_WRAP_MMAP
/**
 * @brief Wraps @c mmap.
 * 
 * Calls @c mmap with @p addr, @p size, @p prot, @p flags, @p fd, and @p offset.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] addr The first argument to pass to @c mmap.
 * @param[in] size The second argument to pass to @c mmap.
 * @param[in] prot The third argument to pass to @c mmap.
 * @param[in] flags The fourth argument to pass to @c mmap.
 * @param[in] fd The fifth argument to pass to @c mmap.
 * @param[in] offset The sixth argument to pass to @c mmap.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to map the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS The address range was invalid for the file.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr or @p offset was not page-aligned.
 * @retval CZ_RESULT_BAD_FILE The file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_LOCK No memory locks were available.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of mapped files was reached.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the filesystem or platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p fd is -1 or an open file descriptor.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MMAP is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_mmap(void* restrict* res, void* addr, size_t size, int prot, int flags, int fd, off_t offset);
#endif

/**
 * @def CZ_WRAP_MUNMAP
 * 
 * @brief Specifies whether @c munmap is defined.
 */
#if !defined(CZ_WRAP_MUNMAP)
#if (                                         \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 &&    \
		CZ_XOPEN_UNIX > 0) ||                 \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||        \
	CZ_POSIX_MAPPED_FILES >= CZ_POSIX_2001 || \
	CZ_POSIX_SHARED_MEMORY_OBJECTS >= CZ_POSIX_2001
#define CZ_WRAP_MUNMAP 1
#else
#define CZ_WRAP_MUNMAP 0
#endif
#endif

#if CZ_WRAP_MUNMAP
/**
 * @brief Wraps @c munmap.
 * 
 * Calls @c munmap with @p addr and @p size.
 * 
 * @param[in] addr The first argument to pass to @c munmap.
 * @param[in] size The second argument to pass to @c munmap.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ADDRESS The address range was invalid or unmapped.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr was not page-aligned.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The mapped memory was already in use by the system.
 * 
 * @pre @p addr is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MUNMAP is defined as a nonzero value.
 */
CZ_NONNULL_ARGS()
enum CzResult czWrap_munmap(void* addr, size_t size);
#endif

/**
 * @def CZ_WRAP_MSYNC
 * 
 * @brief Specifies whether @c msync is defined.
 */
#if !defined(CZ_WRAP_MSYNC)
#if (                                         \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 &&    \
		CZ_XOPEN_UNIX > 0) ||                 \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||        \
	CZ_POSIX_MAPPED_FILES >= CZ_POSIX_2001 || \
	CZ_POSIX_SYNCHRONIZED_IO >= CZ_POSIX_2001
#define CZ_WRAP_MSYNC 1
#else
#define CZ_WRAP_MSYNC 0
#endif
#endif

#if CZ_WRAP_MSYNC
/**
 * @brief Wraps @c msync.
 * 
 * Calls @c msync with @p addr, @p size, and @p flags.
 * 
 * @param[in,out] addr The first argument to pass to @c msync.
 * @param[in] size The second argument to pass to @c msync.
 * @param[in] flags The third argument to pass to @c msync.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p flags was invalid.
 * @retval CZ_RESULT_BAD_ADDRESS The address range was invalid or unmapped.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr was not page-aligned.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when writing to the file.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The mapped memory was already in use by the system.
 * 
 * @pre @p addr is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MSYNC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1, 2)
enum CzResult czWrap_msync(void* addr, size_t size, int flags);
#endif

/**
 * @def CZ_WRAP_SYSCONF
 * 
 * @brief Specifies whether @c sysconf is defined.
 */
#if !defined(CZ_WRAP_SYSCONF)
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
#define CZ_WRAP_SYSCONF 1
#else
#define CZ_WRAP_SYSCONF 0
#endif
#endif

#if CZ_WRAP_SYSCONF
/**
 * @brief Wraps @c sysconf.
 * 
 * Calls @c sysconf with @p name. On success, the returned @c long is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] name The argument to pass to @c sysconf.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_NO_SUPPORT @p name was invalid or unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_SYSCONF is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_sysconf(long* res, int name);
#endif

/**
 * @brief Wraps @c wai_getExecutablePath.
 * 
 * Calls @c wai_getExecutablePath with @p out, @p capacity, and @p dirnameLength. If @p res is nonnull, the returned
 * @c int is synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[out] out The first argument to pass to @c wai_getExecutablePath.
 * @param[in] capacity The second argument to pass to @c wai_getExecutablePath.
 * @param[out] dirnameLength The third argument to pass to @c wai_getExecutablePath.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 */
CZ_WR_ACCESS(1) CZ_WR_ACCESS(2, 3) CZ_WR_ACCESS(4)
enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength);
