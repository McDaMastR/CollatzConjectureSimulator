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
 * @brief Thin wrappers for common standard C functions.
 * 
 * A non-comprehensive set of thin wrapper functions over the standard C library (stdc) to provide consistent error
 * management. This includes functions from C89 to C17. These wrappers are intended for use within cz* API
 * implementations rather than for general use.
 * 
 * Each wrapper function wraps exactly one stdc function, though may also call other stdc functions to aid in error
 * reporting. Each wrapper is prefixed with 'czWrap_' and suffixed with the name of the respective wrapped function. For
 * example, the function @c czWrap_fread wraps the @c fread stdc function.
 * 
 * Wrapper functions are accompanied by a preprocessor macro of the same name, but in screaming snake case. For example,
 * the wrapper @c czWrap_fread is accompanied by the macro @c CZ_WRAP_FREAD. For any wrapper function, the corresponding
 * macro is defined as a nonzero value if and only if the wrapped function is available on the target platform. And the
 * declaration and definition of the wrapper function are visible if and only if the macro is nonzero.
 * 
 * Each wrapper guarantees that on return, the calling thread's value of @c errno is the same as the value of @c errno
 * immediately following the wrapped function's return. So the wrapper's observable effect on the value of @c errno is
 * functionally identical to the wrapped function.
 */

#pragma once

#include "def.h"
#include "support.h"

/**
 * @def CZ_WRAP_MALLOC
 * 
 * @brief Specifies whether @c malloc is defined.
 */
#if !defined(CZ_WRAP_MALLOC)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_MALLOC 1
#else
#define CZ_WRAP_MALLOC 0
#endif
#endif

#if CZ_WRAP_MALLOC
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
 * @retval CZ_RESULT_BAD_SIZE @p size was zero, which was unsupported.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_malloc(void* restrict* res, size_t size);
#endif

/**
 * @def CZ_WRAP_CALLOC
 * 
 * @brief Specifies whether @c calloc is defined.
 */
#if !defined(CZ_WRAP_CALLOC)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_CALLOC 1
#else
#define CZ_WRAP_CALLOC 0
#endif
#endif

#if CZ_WRAP_CALLOC
/**
 * @brief Wraps @c calloc.
 * 
 * Calls @c calloc with @p nelem and @p elsize. On success, the returned @c void* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] nelem The first argument to pass to @c calloc.
 * @param[in] elsize The second argument to pass to @c calloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_BAD_SIZE @p nelem or @p elsize was zero, which was unsupported.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_calloc(void* restrict* res, size_t nelem, size_t elsize);
#endif

/**
 * @def CZ_WRAP_REALLOC
 * 
 * @brief Specifies whether @c realloc is defined.
 */
#if !defined(CZ_WRAP_REALLOC)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_REALLOC 1
#else
#define CZ_WRAP_REALLOC 0
#endif
#endif

#if CZ_WRAP_REALLOC
/**
 * @brief Wraps @c realloc.
 * 
 * Calls @c realloc with @p ptr and @p size. On success, the returned @c void* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] ptr The first argument to pass to @c realloc.
 * @param[in] size The second argument to pass to @c realloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero, which was unsupported.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_REALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_realloc(void* restrict* res, void* ptr, size_t size);
#endif

/**
 * @def CZ_WRAP_FREE
 * 
 * @brief Specifies whether @c free is defined.
 */
#if !defined(CZ_WRAP_FREE)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FREE 1
#else
#define CZ_WRAP_FREE 0
#endif
#endif

#if CZ_WRAP_FREE
/**
 * @brief Wraps @c free.
 * 
 * Calls @c free with @p ptr.
 * 
 * @param[in] ptr The argument to pass to @c free.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FREE is defined as a nonzero value.
 */
enum CzResult czWrap_free(void* ptr);
#endif

/**
 * @def CZ_WRAP_ALIGNED_ALLOC
 * 
 * @brief Specifies whether @c aligned_alloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_ALLOC)
#if (                                                        \
		CZ_DARWIN &&                                         \
		CZ_DARWIN_C_SOURCE &&                                \
		(                                                    \
			CZ_MACOS_VERSION >= CZ_MAKE_VERSION(10, 15) ||   \
			CZ_IOS_VERSION >= CZ_MAKE_VERSION(13, 0))) ||    \
	(                                                        \
		CZ_GNU_LINUX &&                                      \
		CZ_ISOC11_SOURCE &&                                  \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 16)) ||       \
	(                                                        \
		CZ_FREE_BSD &&                                       \
		CZ_FREE_BSD_USE_STDC_2011 &&                         \
		CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(10, 0, 5)) || \
	CZ_STDC_VERSION >= CZ_STDC_2011 ||                       \
	CZ_POSIX_VERSION >= CZ_POSIX_2024 ||                     \
	CZ_XOPEN_VERSION >= CZ_SUS_2024
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
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was invalid or unsupported.
 * @retval CZ_RESULT_BAD_SIZE @p size was not a nonzero multiple of @p alignment.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_ALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_alloc(void* restrict* res, size_t alignment, size_t size);
#endif

/**
 * @def CZ_WRAP_FOPEN
 * 
 * @brief Specifies whether @c fopen is defined.
 */
#if !defined(CZ_WRAP_FOPEN)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FOPEN 1
#else
#define CZ_WRAP_FOPEN 0
#endif
#endif

#if CZ_WRAP_FOPEN
/**
 * @brief Wraps @c fopen.
 * 
 * Calls @c fopen with @p pathname and @p mode. On success, the returned @c FILE* is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] pathname The first argument to pass to @c fopen.
 * @param[in] mode The second argument to pass to @c fopen.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p pathname or @p mode was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when creating the file.
 * @retval CZ_RESULT_BAD_PATH @p pathname was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files or streams was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p res is nonnull.
 * @pre @p pathname is nonnull and NUL-terminated.
 * @pre @p mode is nonnull and NUL-terminated.
 * @pre @p pathname and @p mode do not overlap in memory.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FOPEN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2, 3) CZ_NULTERM_ARG(2) CZ_NULTERM_ARG(3) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2) CZ_RD_ACCESS(3)
enum CzResult czWrap_fopen(FILE* restrict* res, const char* pathname, const char* mode);
#endif

/**
 * @def CZ_WRAP_FREOPEN
 * 
 * @brief Specifies whether @c freopen is defined.
 */
#if !defined(CZ_WRAP_FREOPEN)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FREOPEN 1
#else
#define CZ_WRAP_FREOPEN 0
#endif
#endif

#if CZ_WRAP_FREOPEN
/**
 * @brief Wraps @c freopen.
 * 
 * Calls @c freopen with @p pathname, @p mode, and @p stream.
 * 
 * @param[in] pathname The first argument to pass to @c freopen.
 * @param[in] mode The second argument to pass to @c freopen.
 * @param[in,out] stream The third argument to pass to @c freopen.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p pathname, @p mode, or @p stream was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing, closing, or creating the file.
 * @retval CZ_RESULT_BAD_PATH @p pathname was an invalid or unsupported filepath.
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
 * @pre @p pathname is NUL-terminated.
 * @pre @p mode is nonnull and NUL-terminated.
 * @pre @p stream is nonnull.
 * @pre @p pathname, @p mode, and @p stream do not overlap in memory.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FREOPEN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(2, 3) CZ_NULTERM_ARG(1) CZ_NULTERM_ARG(2) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2) CZ_RW_ACCESS(3)
enum CzResult czWrap_freopen(const char* pathname, const char* mode, FILE* stream);
#endif

/**
 * @def CZ_WRAP_FCLOSE
 * 
 * @brief Specifies whether @c fclose is defined.
 */
#if !defined(CZ_WRAP_FCLOSE)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FCLOSE 1
#else
#define CZ_WRAP_FCLOSE 0
#endif
#endif

#if CZ_WRAP_FCLOSE
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
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
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
 * 
 * @note This function is only defined if @ref CZ_WRAP_FCLOSE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1)
enum CzResult czWrap_fclose(FILE* stream);
#endif

/**
 * @def CZ_WRAP_FERROR
 * 
 * @brief Specifies whether @c ferror is defined.
 */
#if !defined(CZ_WRAP_FERROR)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FERROR 1
#else
#define CZ_WRAP_FERROR 0
#endif
#endif

#if CZ_WRAP_FERROR
/**
 * @brief Wraps @c ferror.
 * 
 * Calls @c ferror with @p stream. The returned @c int is synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] stream The argument to pass to @c ferror.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * 
 * @pre @p res is nonnull.
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FERROR is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2)
enum CzResult czWrap_ferror(int* res, FILE* stream);
#endif

/**
 * @def CZ_WRAP_FEOF
 * 
 * @brief Specifies whether @c feof is defined.
 */
#if !defined(CZ_WRAP_FEOF)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FEOF 1
#else
#define CZ_WRAP_FEOF 0
#endif
#endif

#if CZ_WRAP_FEOF
/**
 * @brief Wraps @c feof.
 * 
 * Calls @c feof with @p stream. The returned @c int is synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] stream The argument to pass to @c feof.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * 
 * @pre @p res is nonnull.
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FEOF is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2)
enum CzResult czWrap_feof(int* res, FILE* stream);
#endif

/**
 * @def CZ_WRAP_CLEARERR
 * 
 * @brief Specifies whether @c clearerr is defined.
 */
#if !defined(CZ_WRAP_CLEARERR)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_CLEARERR 1
#else
#define CZ_WRAP_CLEARERR 0
#endif
#endif

#if CZ_WRAP_CLEARERR
/**
 * @brief Wraps @c clearerr.
 * 
 * Calls @c clearerr with @p stream.
 * 
 * @param[in,out] stream The argument to pass to @c clearerr.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * 
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CLEARERR is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_RW_ACCESS(1)
enum CzResult czWrap_clearerr(FILE* stream);
#endif

/**
 * @def CZ_WRAP_FSEEK
 * 
 * @brief Specifies whether @c fseek is defined.
 */
#if !defined(CZ_WRAP_FSEEK)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FSEEK 1
#else
#define CZ_WRAP_FSEEK 0
#endif
#endif

#if CZ_WRAP_FSEEK
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
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
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
 * @note This function is only defined if @ref CZ_WRAP_FSEEK is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_RW_ACCESS(1)
enum CzResult czWrap_fseek(FILE* stream, long offset, int whence);
#endif

/**
 * @def CZ_WRAP_FTELL
 * 
 * @brief Specifies whether @c ftell is defined.
 */
#if !defined(CZ_WRAP_FTELL)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FTELL 1
#else
#define CZ_WRAP_FTELL 0
#endif
#endif

#if CZ_WRAP_FTELL
/**
 * @brief Wraps @c ftell.
 * 
 * Calls @c ftell with @p stream. On success, the returned @c long is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] stream The argument to pass to @c ftell.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
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
 * @note This function is only defined if @ref CZ_WRAP_FTELL is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(1) CZ_RW_ACCESS(2)
enum CzResult czWrap_ftell(long* res, FILE* stream);
#endif

/**
 * @def CZ_WRAP_FGETPOS
 * 
 * @brief Specifies whether @c fgetpos is defined.
 */
#if !defined(CZ_WRAP_FGETPOS)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1996 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1992
#define CZ_WRAP_FGETPOS 1
#else
#define CZ_WRAP_FGETPOS 0
#endif
#endif

#if CZ_WRAP_FGETPOS
/**
 * @brief Wraps @c fgetpos.
 * 
 * Calls @c fgetpos with @p stream and @p pos.
 * 
 * @param[in,out] stream The first argument to pass to @c fgetpos.
 * @param[out] pos The second argument to pass to @c fgetpos.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to flush the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p stream or @p pos was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
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
 * 
 * @note This function is only defined if @ref CZ_WRAP_FGETPOS is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_RW_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_fgetpos(FILE* stream, fpos_t* pos);
#endif

/**
 * @def CZ_WRAP_FSETPOS
 * 
 * @brief Specifies whether @c fsetpos is defined.
 */
#if !defined(CZ_WRAP_FSETPOS)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1996 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1992
#define CZ_WRAP_FSETPOS 1
#else
#define CZ_WRAP_FSETPOS 0
#endif
#endif

#if CZ_WRAP_FSETPOS
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
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
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
 * 
 * @note This function is only defined if @ref CZ_WRAP_FSETPOS is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_RW_ACCESS(1) CZ_RD_ACCESS(2)
enum CzResult czWrap_fsetpos(FILE* stream, const fpos_t* pos);
#endif

/**
 * @def CZ_WRAP_REWIND
 * 
 * @brief Specifies whether @c rewind is defined.
 */
#if !defined(CZ_WRAP_REWIND)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_REWIND 1
#else
#define CZ_WRAP_REWIND 0
#endif
#endif

#if CZ_WRAP_REWIND
/**
 * @brief Wraps @c rewind.
 * 
 * Calls @c rewind with @p stream.
 * 
 * @param[in,out] stream The argument to pass to @c rewind.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @pre @p stream is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_REWIND is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_RW_ACCESS(1)
enum CzResult czWrap_rewind(FILE* stream);
#endif

/**
 * @def CZ_WRAP_FREAD
 * 
 * @brief Specifies whether @c fread is defined.
 */
#if !defined(CZ_WRAP_FREAD)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FREAD 1
#else
#define CZ_WRAP_FREAD 0
#endif
#endif

#if CZ_WRAP_FREAD
/**
 * @brief Wraps @c fread.
 * 
 * Calls @c fread with @p ptr, @p size, @p nitems, and @p stream. If @p res is nonnull, the returned @c size_t is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[out] ptr The first argument to pass to @c fread.
 * @param[in] size The second argument to pass to @c fread.
 * @param[in] nitems The third argument to pass to @c fread.
 * @param[in,out] stream The fourth argument to pass to @c fread.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_OFFSET The file was already at @c EOF.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p ptr is nonnull.
 * @pre @p stream is nonnull.
 * @pre @p ptr and @p stream do not overlap in memory.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FREAD is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(2, 5) CZ_WR_ACCESS(1) CZ_WR_ACCESS(2) CZ_RW_ACCESS(5)
enum CzResult czWrap_fread(size_t* res, void* ptr, size_t size, size_t nitems, FILE* stream);
#endif

/**
 * @def CZ_WRAP_FWRITE
 * 
 * @brief Specifies whether @c fwrite is defined.
 */
#if !defined(CZ_WRAP_FWRITE)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FWRITE 1
#else
#define CZ_WRAP_FWRITE 0
#endif
#endif

#if CZ_WRAP_FWRITE
/**
 * @brief Wraps @c fwrite.
 * 
 * Calls @c fwrite with @p ptr, @p size, @p nitems, and @p stream. If @p res is nonnull, the returned @c size_t is
 * synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] ptr The first argument to pass to @c fwrite.
 * @param[in] size The second argument to pass to @c fwrite.
 * @param[in] nitems The third argument to pass to @c fwrite.
 * @param[in,out] stream The fourth argument to pass to @c fwrite.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when writing to the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p ptr is nonnull.
 * @pre @p stream is nonnull.
 * @pre @p ptr and @p stream do not overlap in memory.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FWRITE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(2, 5) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2) CZ_RW_ACCESS(5)
enum CzResult czWrap_fwrite(size_t* res, const void* ptr, size_t size, size_t nitems, FILE* stream);
#endif

/**
 * @def CZ_WRAP_FFLUSH
 * 
 * @brief Specifies whether @c fflush is defined.
 */
#if !defined(CZ_WRAP_FFLUSH)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1985
#define CZ_WRAP_FFLUSH 1
#else
#define CZ_WRAP_FFLUSH 0
#endif
#endif

#if CZ_WRAP_FFLUSH
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
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_BAD_STREAM @p stream was a nonnull invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FFLUSH is defined as a nonzero value.
 */
CZ_RW_ACCESS(1)
enum CzResult czWrap_fflush(FILE* stream);
#endif

/**
 * @def CZ_WRAP_REMOVE
 * 
 * @brief Specifies whether @c remove is defined.
 */
#if !defined(CZ_WRAP_REMOVE)
#if (                                    \
		CZ_FREE_BSD &&                   \
		CZ_FREE_BSD_USE_STDC_1989) ||    \
	CZ_STDC_VERSION >= CZ_STDC_1989 ||   \
	CZ_POSIX_VERSION >= CZ_POSIX_1988 || \
	CZ_XOPEN_VERSION >= CZ_XPG_1989
#define CZ_WRAP_REMOVE 1
#else
#define CZ_WRAP_REMOVE 0
#endif
#endif

#if CZ_WRAP_REMOVE
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
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when deleting the file.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_REMOVE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czWrap_remove(const char* path);
#endif
