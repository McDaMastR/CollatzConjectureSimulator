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
 * @note Due to differences in error reporting between various platforms and standards (e.g. MacOS, Windows, POSIX), the
 * documented return values for wrappers are @b not guarantees. That is, if an error occurs in a wrapper function, the
 * wrapper provides no guarantee the corresponding error value will be returned. And in particular, if the platform does
 * not provide any reliable method to identify the type of error that occurred, @c CZ_RESULT_INTERNAL_ERROR will
 * commonly be returned.
 */

#pragma once

#include "def.h"

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
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_realloc(void* restrict* res, void* ptr, size_t size);

/**
 * @def CZ_WRAP_REALLOCF
 * 
 * @brief Specifies whether @c reallocf is defined.
 */
#if !defined(CZ_WRAP_REALLOCF)
	#if CZ_APPLE
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
 * @def CZ_WRAP_RECALLOC
 * 
 * @brief Specifies whether @c _recalloc is defined.
 */
#if !defined(CZ_WRAP_RECALLOC)
	#if CZ_WINDOWS
		#define CZ_WRAP_RECALLOC 1
	#else
		#define CZ_WRAP_RECALLOC 0
	#endif
#endif

#if CZ_WRAP_RECALLOC
/**
 * @brief Wraps @c _recalloc.
 * 
 * Calls @c _recalloc with @p ptr, @p count, and @p size. On success, the returned @c void* is synchronously written to
 * @p res. On failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] ptr The first argument to pass to @c _recalloc.
 * @param[in] count The second argument to pass to @c _recalloc.
 * @param[in] size The third argument to pass to @c _recalloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_RECALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_recalloc(void* restrict* res, void* ptr, size_t count, size_t size);
#endif

/**
 * @def CZ_WRAP_POSIX_MEMALIGN
 * 
 * @brief Specifies whether @c posix_memalign is defined.
 */
#if !defined(CZ_WRAP_POSIX_MEMALIGN)
	#if CZ_APPLE || CZ_POSIX_ADVISORY_INFO >= 200112L
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
 * @def CZ_WRAP_ALIGNED_OFFSET_MALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_malloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_MALLOC)
	#if CZ_WINDOWS
		#define CZ_WRAP_ALIGNED_OFFSET_MALLOC 1
	#else
		#define CZ_WRAP_ALIGNED_OFFSET_MALLOC 0
	#endif
#endif

#if CZ_WRAP_ALIGNED_OFFSET_MALLOC
/**
 * @brief Wraps @c _aligned_offset_malloc.
 * 
 * Calls @c _aligned_offset_malloc with @p size, @p alignment, and @p offset. On success, the returned @c void* is
 * synchronously written to @p res. On failure, the contents of @p res are unchanged and the call is logged to
 * @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] size The first argument to pass to @c _aligned_offset_malloc.
 * @param[in] alignment The second argument to pass to @c _aligned_offset_malloc.
 * @param[in] offset The third argument to pass to @c _aligned_offset_malloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to @p size.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_MALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_malloc(void* restrict* res, size_t size, size_t alignment, size_t offset);
#endif

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_REALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_realloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_REALLOC)
	#if CZ_WINDOWS
		#define CZ_WRAP_ALIGNED_OFFSET_REALLOC 1
	#else
		#define CZ_WRAP_ALIGNED_OFFSET_REALLOC 0
	#endif
#endif

#if CZ_WRAP_ALIGNED_OFFSET_REALLOC
/**
 * @brief Wraps @c _aligned_offset_realloc.
 * 
 * Calls @c _aligned_offset_realloc with @p ptr, @p size, @p alignment, and @p offset. On success, the returned @c void*
 * is synchronously written to @p res. On failure, the contents of @p res are unchanged and the call is logged to
 * @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] ptr The first argument to pass to @c _aligned_offset_realloc.
 * @param[in] size The second argument to pass to @c _aligned_offset_realloc.
 * @param[in] alignment The third argument to pass to @c _aligned_offset_realloc.
 * @param[in] offset The fourth argument to pass to @c _aligned_offset_realloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to @p size.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_REALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_realloc(
	void* restrict* res, void* ptr, size_t size, size_t alignment, size_t offset);
#endif

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_RECALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_recalloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_RECALLOC)
	#if CZ_WINDOWS
		#define CZ_WRAP_ALIGNED_OFFSET_RECALLOC 1
	#else
		#define CZ_WRAP_ALIGNED_OFFSET_RECALLOC 0
	#endif
#endif

#if CZ_WRAP_ALIGNED_OFFSET_RECALLOC
/**
 * @brief Wraps @c _aligned_offset_recalloc.
 * 
 * Calls @c _aligned_offset_recalloc with @p ptr, @p count, @p size, @p alignment, and @p offset. On success, the
 * returned @c void* is synchronously written to @p res. On failure, the contents of @p res are unchanged and the call
 * is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] ptr The first argument to pass to @c _aligned_offset_recalloc.
 * @param[in] count The second argument to pass to @c _aligned_offset_recalloc.
 * @param[in] size The third argument to pass to @c _aligned_offset_recalloc.
 * @param[in] alignment The fourth argument to pass to @c _aligned_offset_recalloc.
 * @param[in] offset The fifth argument to pass to @c _aligned_offset_recalloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to (@p count * @p size).
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_RECALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_recalloc(
	void* restrict* res, void* ptr, size_t count, size_t size, size_t alignment, size_t offset);
#endif

/**
 * @def CZ_WRAP_MADVISE
 * 
 * @brief Specifies whether @c madvise is defined.
 */
#if !defined(CZ_WRAP_MADVISE)
	#if CZ_APPLE
		#define CZ_WRAP_MADVISE 1
	#else
		#define CZ_WRAP_MADVISE 0
	#endif
#endif

#if CZ_WRAP_MADVISE
/**
 * @brief Wraps @c madvise.
 * 
 * Calls @c madvise with @p addr, @p size, and @p advice. If @p res is nonnull, the returned @c int is synchronously
 * written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in,out] addr The first argument to pass to @c madvise.
 * @param[in] size The second argument to pass to @c madvise.
 * @param[in] advice The third argument to pass to @c madvise.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to enact @p advice on the address range was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p advice was invalid or the address range included unallocated memory.
 * @retval CZ_RESULT_NO_MEMORY The address range included invalid memory regions.
 * @retval CZ_RESULT_NO_SUPPORT @p advice was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MADVISE is defined as a nonzero value.
 */
CZ_WR_ACCESS(1)
enum CzResult czWrap_madvise(int* res, void* addr, size_t size, int advice);
#endif

/**
 * @def CZ_WRAP_POSIX_MADVISE
 * 
 * @brief Specifies whether @c posix_madvise is defined.
 */
#if !defined(CZ_WRAP_POSIX_MADVISE)
	#if CZ_APPLE || CZ_POSIX_ADVISORY_INFO >= 200112L
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
 * @retval CZ_RESULT_BAD_ADDRESS @p advice was invalid or the address range included unallocated memory.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_NO_MEMORY The address range included invalid memory regions.
 * @retval CZ_RESULT_NO_SUPPORT @p advice was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_POSIX_MADVISE is defined as a nonzero value.
 */
CZ_WR_ACCESS(1)
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
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_fopen(FILE* restrict* res, const char* path, const char* mode);

/**
 * @brief Wraps @c fclose.
 * 
 * Calls @c fclose with @p stream.
 * 
 * @param[in,out] stream The argument to pass to @c fclose.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 */
enum CzResult czWrap_fclose(FILE* stream);

/**
 * @brief Wraps @c fseek.
 * 
 * Calls @c fseek with @p stream, @p offset, and @p origin.
 * 
 * @param[in,out] stream The first argument to pass to @c fseek.
 * @param[in] offset The second argument to pass to @c fseek.
 * @param[in] origin The third argument to pass to @c fseek.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p origin was invalid or the resultant file offset would be invalid.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 */
enum CzResult czWrap_fseek(FILE* stream, long offset, int origin);

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
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_ftell(long* res, FILE* stream);

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
 * @param[in] stream The fourth argument to pass to @c fread.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET The file was already at @c EOF.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 */
CZ_WR_ACCESS(1)
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
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 */
CZ_WR_ACCESS(1)
enum CzResult czWrap_fwrite(size_t* res, const void* buffer, size_t size, size_t count, FILE* stream);

/**
 * @brief Wraps @c remove.
 * 
 * Calls @c remove with @p path.
 * 
 * @param[in] path The argument to pass to @c remove.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to remove the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 */
enum CzResult czWrap_remove(const char* path);

/**
 * @def CZ_WRAP_FILENO
 * 
 * @brief Specifies whether @c fileno is defined.
 */
#if !defined(CZ_WRAP_FILENO)
	#if CZ_WINDOWS || CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FILENO is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_fileno(int* res, FILE* stream);
#endif

/**
 * @def CZ_WRAP_ISATTY
 * 
 * @brief Specifies whether @c isatty is defined.
 */
#if !defined(CZ_WRAP_ISATTY)
	#if CZ_WINDOWS || CZ_POSIX_VERSION >= 200112L
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
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ISATTY is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_isatty(int* res, int fd);
#endif

/**
 * @def CZ_WRAP_STAT
 * 
 * @brief Specifies whether @c stat is defined.
 */
#if !defined(CZ_WRAP_STAT)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_STAT is defined as a nonzero value.
 */
enum CzResult czWrap_stat(const char* path, struct stat* st);
#endif

/**
 * @def CZ_WRAP_FSTAT
 * 
 * @brief Specifies whether @c fstat is defined.
 */
#if !defined(CZ_WRAP_FSTAT)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_BAD_ADDRESS @p st was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FSTAT is defined as a nonzero value.
 */
enum CzResult czWrap_fstat(int fd, struct stat* st);
#endif

/**
 * @def CZ_WRAP_TRUNCATE
 * 
 * @brief Specifies whether @c truncate is defined.
 */
#if !defined(CZ_WRAP_TRUNCATE)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was negative or too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * 
 * @note This function is only defined if @ref CZ_WRAP_TRUNCATE is defined as a nonzero value.
 */
enum CzResult czWrap_truncate(const char* path, off_t size);
#endif

/**
 * @def CZ_WRAP_FTRUNCATE
 * 
 * @brief Specifies whether @c ftruncate is defined.
 */
#if !defined(CZ_WRAP_FTRUNCATE)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @param[in,out] fd The first argument to pass to @c ftruncate.
 * @param[in] size The second argument to pass to @c ftruncate.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to truncate the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_SIZE @p size was negative or too large.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FTRUNCATE is defined as a nonzero value.
 */
enum CzResult czWrap_ftruncate(int fd, off_t size);
#endif

/**
 * @def CZ_WRAP_OPEN
 * 
 * @brief Specifies whether @c open is defined.
 */
#if !defined(CZ_WRAP_OPEN)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file with @p flags was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_OPEN is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_open(int* res, const char* path, int flags, mode_t mode);
#endif

/**
 * @def CZ_WRAP_CLOSE
 * 
 * @brief Specifies whether @c close is defined.
 */
#if !defined(CZ_WRAP_CLOSE)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @param[in,out] fd The argument to pass to @c close.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CLOSE is defined as a nonzero value.
 */
enum CzResult czWrap_close(int fd);
#endif

/**
 * @def CZ_WRAP_PREAD
 * 
 * @brief Specifies whether @c pread is defined.
 */
#if !defined(CZ_WRAP_PREAD)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_FILE The file was empty or deleted.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * @retval CZ_RESULT_TIMEOUT A system operation timed out.
 * 
 * @note This function is only defined if @ref CZ_WRAP_PREAD is defined as a nonzero value.
 */
CZ_WR_ACCESS(1)
enum CzResult czWrap_pread(ssize_t* res, int fd, void* buffer, size_t size, off_t offset);
#endif

/**
 * @def CZ_WRAP_WRITE
 * 
 * @brief Specifies whether @c write is defined.
 */
#if !defined(CZ_WRAP_WRITE)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @param[in,out] fd The first argument to pass to @c write.
 * @param[in] buffer The second argument to pass to @c write.
 * @param[in] size The third argument to pass to @c write.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_WRITE is defined as a nonzero value.
 */
CZ_WR_ACCESS(1)
enum CzResult czWrap_write(ssize_t* res, int fd, const void* buffer, size_t size);
#endif

/**
 * @def CZ_WRAP_PWRITE
 * 
 * @brief Specifies whether @c pwrite is defined.
 */
#if !defined(CZ_WRAP_PWRITE)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @param[in,out] fd The first argument to pass to @c pwrite.
 * @param[in] buffer The second argument to pass to @c pwrite.
 * @param[in] size The third argument to pass to @c pwrite.
 * @param[in] offset The fourth argument to pass to @c pwrite.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_BAD_SIZE @p size was too large.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_PWRITE is defined as a nonzero value.
 */
CZ_WR_ACCESS(1)
enum CzResult czWrap_pwrite(ssize_t* res, int fd, const void* buffer, size_t size, off_t offset);
#endif

/**
 * @def CZ_WRAP_MMAP
 * 
 * @brief Specifies whether @c mmap is defined.
 */
#if !defined(CZ_WRAP_MMAP)
	#if CZ_POSIX_MAPPED_FILES >= 200112L || CZ_POSIX_SHARED_MEMORY_OBJECTS >= 200112L
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
 * @retval CZ_RESULT_BAD_ADDRESS The address range was invalid for @p fd.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr or @p offset was not page-aligned.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET The address range exceeds the maximum offset for @p fd.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of mapped files was reached.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
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
	#if CZ_POSIX_MAPPED_FILES >= 200112L || CZ_POSIX_SHARED_MEMORY_OBJECTS >= 200112L
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
 * @param[in,out] addr The first argument to pass to @c munmap.
 * @param[in] size The second argument to pass to @c munmap.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ADDRESS @p addr was not page-aligned or the address range was invalid.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MUNMAP is defined as a nonzero value.
 */
enum CzResult czWrap_munmap(void* addr, size_t size);
#endif

/**
 * @def CZ_WRAP_MSYNC
 * 
 * @brief Specifies whether @c msync is defined.
 */
#if !defined(CZ_WRAP_MSYNC)
	#if CZ_POSIX_MAPPED_FILES >= 200112L
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
 * @retval CZ_RESULT_BAD_ADDRESS The address range was invalid or unmapped.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p addr was not page-aligned.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The address range was locked.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MSYNC is defined as a nonzero value.
 */
enum CzResult czWrap_msync(void* addr, size_t size, int flags);
#endif

/**
 * @def CZ_WRAP_GET_OSFHANDLE
 * 
 * @brief Specifies whether @c _get_osfhandle is defined.
 */
#if !defined(CZ_WRAP_GET_OSFHANDLE)
	#if CZ_WINDOWS
		#define CZ_WRAP_GET_OSFHANDLE 1
	#else
		#define CZ_WRAP_GET_OSFHANDLE 0
	#endif
#endif

#if CZ_WRAP_GET_OSFHANDLE
/**
 * @brief Wraps @c _get_osfhandle.
 * 
 * Calls @c _get_osfhandle with @p fd. On success, the returned @c intptr_t is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The argument to pass to @c _get_osfhandle.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_OSFHANDLE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
enum CzResult czWrap_get_osfhandle(intptr_t* res, int fd);
#endif

/**
 * @def CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR
 * 
 * @brief Specifies whether @c MultiByteToWideChar is defined.
 */
#if !defined(CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR)
	#if CZ_WINDOWS
		#define CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR 1
	#else
		#define CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR 0
	#endif
#endif

#if CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR
/**
 * @brief Wraps @c MultiByteToWideChar.
 * 
 * Calls @c MultiByteToWideChar with @p codePage, @p flags, @p mbStr, @p mbSize, @p wcStr, and @p wcSize. If @p res is
 * nonnull, the returned @c int is synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] codePage The first argument to pass to @c MultiByteToWideChar.
 * @param[in] flags The second argument to pass to @c MultiByteToWideChar.
 * @param[in] mbStr The third argument to pass to @c MultiByteToWideChar.
 * @param[in] mbSize The fourth argument to pass to @c MultiByteToWideChar.
 * @param[out] wcStr The fifth argument to pass to @c MultiByteToWideChar.
 * @param[in] wcSize The sixth argument to pass to @c MultiByteToWideChar.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ADDRESS @p mbStr or @p wcStr was an invalid pointer.
 * @retval CZ_RESULT_BAD_PATH @p mbStr was an invalid multi-byte string.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR is defined as a nonzero value.
 */
CZ_WR_ACCESS(1)
enum CzResult czWrap_MultiByteToWideChar(
	LPINT res, UINT codePage, DWORD flags, LPCCH mbStr, INT mbSize, LPWSTR wcStr, INT wcSize);
#endif

/**
 * @def CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W
 * 
 * @brief Specifies whether @c GetFileAttributesExW is defined.
 */
#if !defined(CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W)
	#if CZ_WINDOWS
		#define CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W 1
	#else
		#define CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W 0
	#endif
#endif

#if CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W
/**
 * @brief Wraps @c GetFileAttributesExW.
 * 
 * Calls @c GetFileAttributesExW with @p path, @p level, and @p info.
 * 
 * @param[in] path The first argument to pass to @c GetFileAttributesExW.
 * @param[in] level The second argument to pass to @c GetFileAttributesExW.
 * @param[out] info The third argument to pass to @c GetFileAttributesExW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p info was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W is defined as a nonzero value.
 */
enum CzResult czWrap_GetFileAttributesExW(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info);
#endif

/**
 * @def CZ_WRAP_GET_FILE_SIZE_EX
 * 
 * @brief Specifies whether @c GetFileSizeEx is defined.
 */
#if !defined(CZ_WRAP_GET_FILE_SIZE_EX)
	#if CZ_WINDOWS
		#define CZ_WRAP_GET_FILE_SIZE_EX 1
	#else
		#define CZ_WRAP_GET_FILE_SIZE_EX 0
	#endif
#endif

#if CZ_WRAP_GET_FILE_SIZE_EX
/**
 * @brief Wraps @c GetFileSizeEx.
 * 
 * Calls @c GetFileSizeEx with @p file and @p size.
 * 
 * @param[in] file The first argument to pass to @c GetFileSizeEx.
 * @param[out] size The second argument to pass to @c GetFileSizeEx.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p size was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_FILE_SIZE_EX is defined as a nonzero value.
 */
enum CzResult czWrap_GetFileSizeEx(HANDLE file, PLARGE_INTEGER size);
#endif

/**
 * @def CZ_WRAP_CREATE_FILE_W
 * 
 * @brief Specifies whether @c CreateFileW is defined.
 */
#if !defined(CZ_WRAP_CREATE_FILE_W)
	#if CZ_WINDOWS
		#define CZ_WRAP_CREATE_FILE_W 1
	#else
		#define CZ_WRAP_CREATE_FILE_W 0
	#endif
#endif

#if CZ_WRAP_CREATE_FILE_W
/**
 * @brief Wraps @c CreateFileW.
 * 
 * Calls @c CreateFileW with @p path, @p desiredAccess, @p shareMode, @p securityAttributes, @p creationDisposition,
 * @p flagsAndAttributes, and @p templateFile. On success, the returned @c HANDLE is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] path The first argument to pass to @c CreateFileW.
 * @param[in] desiredAccess The second argument to pass to @c CreateFileW.
 * @param[in] shareMode The third argument to pass to @c CreateFileW.
 * @param[in] securityAttributes The fourth argument to pass to @c CreateFileW.
 * @param[in] creationDisposition The fifth argument to pass to @c CreateFileW.
 * @param[in] flagsAndAttributes The sixth argument to pass to @c CreateFileW.
 * @param[in] templateFile The seventh argument to pass to @c CreateFileW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p securityAttributes was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CREATE_FILE_W is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_CreateFileW(
	LPHANDLE res,
	LPCWSTR path,
	DWORD desiredAccess,
	DWORD shareMode,
	LPSECURITY_ATTRIBUTES securityAttributes,
	DWORD creationDisposition,
	DWORD flagsAndAttributes,
	HANDLE templateFile);
#endif

/**
 * @def CZ_WRAP_CLOSE_HANDLE
 * 
 * @brief Specifies whether @c CloseHandle is defined.
 */
#if !defined(CZ_WRAP_CLOSE_HANDLE)
	#if CZ_WINDOWS
		#define CZ_WRAP_CLOSE_HANDLE 1
	#else
		#define CZ_WRAP_CLOSE_HANDLE 0
	#endif
#endif

#if CZ_WRAP_CLOSE_HANDLE
/**
 * @brief Wraps @c CloseHandle.
 * 
 * Calls @c CloseHandle with @p handle.
 * 
 * @param[in,out] handle The argument to pass to @c CloseHandle.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CLOSE_HANDLE is defined as a nonzero value.
 */
enum CzResult czWrap_CloseHandle(HANDLE handle);
#endif

/**
 * @def CZ_WRAP_SET_END_OF_FILE
 * 
 * @brief Specifies whether @c SetEndOfFile is defined.
 */
#if !defined(CZ_WRAP_SET_END_OF_FILE)
	#if CZ_WINDOWS
		#define CZ_WRAP_SET_END_OF_FILE 1
	#else
		#define CZ_WRAP_SET_END_OF_FILE 0
	#endif
#endif

#if CZ_WRAP_SET_END_OF_FILE
/**
 * @brief Wraps @c SetEndOfFile.
 * 
 * Calls @c SetEndOfFile with @p file.
 * 
 * @param[in,out] file The argument to pass to @c SetEndOfFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_SET_END_OF_FILE is defined as a nonzero value.
 */
enum CzResult czWrap_SetEndOfFile(HANDLE file);
#endif

/**
 * @def CZ_WRAP_READ_FILE
 * 
 * @brief Specifies whether @c ReadFile is defined.
 */
#if !defined(CZ_WRAP_READ_FILE)
	#if CZ_WINDOWS
		#define CZ_WRAP_READ_FILE 1
	#else
		#define CZ_WRAP_READ_FILE 0
	#endif
#endif

#if CZ_WRAP_READ_FILE
/**
 * @brief Wraps @c ReadFile.
 * 
 * Calls @c ReadFile with @p file, @p buffer, @p numberOfBytesToRead, @p numberOfBytesRead, and @p overlapped.
 * 
 * @param[in] file The first argument to pass to @c ReadFile.
 * @param[out] buffer The second argument to pass to @c ReadFile.
 * @param[in] numberOfBytesToRead The third argument to pass to @c ReadFile.
 * @param[out] numberOfBytesRead The fourth argument to pass to @c ReadFile.
 * @param[in] overlapped The fifth argument to pass to @c ReadFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer or @p numberOfBytesRead was an invalid pointer or @p buffer was too small.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_READ_FILE is defined as a nonzero value.
 */
enum CzResult czWrap_ReadFile(
	HANDLE file, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead, LPOVERLAPPED overlapped);
#endif

/**
 * @def CZ_WRAP_WRITE_FILE
 * 
 * @brief Specifies whether @c WriteFile is defined.
 */
#if !defined(CZ_WRAP_WRITE_FILE)
	#if CZ_WINDOWS
		#define CZ_WRAP_WRITE_FILE 1
	#else
		#define CZ_WRAP_WRITE_FILE 0
	#endif
#endif

#if CZ_WRAP_WRITE_FILE
/**
 * @brief Wraps @c WriteFile.
 * 
 * Calls @c WriteFile with @p file, @p buffer, @p numberOfBytesToRead, @p numberOfBytesRead, and @p overlapped.
 * 
 * @param[in,out] file The first argument to pass to @c WriteFile.
 * @param[in] buffer The second argument to pass to @c WriteFile.
 * @param[in] numberOfBytesToRead The third argument to pass to @c WriteFile.
 * @param[out] numberOfBytesRead The fourth argument to pass to @c WriteFile.
 * @param[in] overlapped The fifth argument to pass to @c WriteFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p buffer or @p numberOfBytesWritten was an invalid pointer or @p buffer was too small.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was an invalid file offset.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_WRITE_FILE is defined as a nonzero value.
 */
enum CzResult czWrap_WriteFile(
	HANDLE file, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LPOVERLAPPED overlapped);
#endif

/**
 * @def CZ_WRAP_DELETE_FILE_W
 * 
 * @brief Specifies whether @c DeleteFileW is defined.
 */
#if !defined(CZ_WRAP_DELETE_FILE_W)
	#if CZ_WINDOWS
		#define CZ_WRAP_DELETE_FILE_W 1
	#else
		#define CZ_WRAP_DELETE_FILE_W 0
	#endif
#endif

#if CZ_WRAP_DELETE_FILE_W
/**
 * @brief Wraps @c DeleteFileW.
 * 
 * Calls @c DeleteFileW with @p path.
 * 
 * @param[in] path The argument to pass to @c DeleteFileW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to delete the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @note This function is only defined if @ref CZ_WRAP_DELETE_FILE_W is defined as a nonzero value.
 */
enum CzResult czWrap_DeleteFileW(LPCWSTR path);
#endif

/**
 * @def CZ_WRAP_SYSCONF
 * 
 * @brief Specifies whether @c sysconf is defined.
 */
#if !defined(CZ_WRAP_SYSCONF)
	#if CZ_POSIX_VERSION >= 200112L
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
 * @retval CZ_RESULT_NO_SUPPORT @p name was unsupported by the platform.
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
CZ_WR_ACCESS(1)
enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength);
