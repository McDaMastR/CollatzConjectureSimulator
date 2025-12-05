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
#include "wrap_posix.h"
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
