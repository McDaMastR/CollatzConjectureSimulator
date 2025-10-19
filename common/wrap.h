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
 * A non-comprehensive set of thin wrapper functions to provide consistent error management.
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
CZ_NONNULL_ARGS CZ_WR_ACCESS(1)
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
CZ_NONNULL_ARGS CZ_WR_ACCESS(1)
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
CZ_NONNULL_ARG(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_realloc(void* restrict* res, void* ptr, size_t size);

/**
 * @def CZ_WRAP_REALLOCF
 * 
 * @brief Specifies whether @c reallocf is defined.
 */
#if !defined(CZ_WRAP_REALLOCF)
	#if defined(__APPLE__)
		#define CZ_WRAP_REALLOCF 1
	#else
		#define CZ_WRAP_REALLOCF 0
	#endif
#endif

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
CZ_NONNULL_ARG(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_reallocf(void* restrict* res, void* ptr, size_t size);

/**
 * @def CZ_WRAP_RECALLOC
 * 
 * @brief Specifies whether @c _recalloc is defined.
 */
#if !defined(CZ_WRAP_RECALLOC)
	#if defined(_WIN32)
		#define CZ_WRAP_RECALLOC 1
	#else
		#define CZ_WRAP_RECALLOC 0
	#endif
#endif

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
CZ_NONNULL_ARG(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_recalloc(void* restrict* res, void* ptr, size_t count, size_t size);

/**
 * @def CZ_WRAP_POSIX_MEMALIGN
 * 
 * @brief Specifies whether @c posix_memalign is defined.
 */
#if !defined(CZ_WRAP_POSIX_MEMALIGN)
	#if defined(__APPLE__) || (defined(_POSIX_ADVISORY_INFO) && _POSIX_ADVISORY_INFO >= 200112L)
		#define CZ_WRAP_POSIX_MEMALIGN 1
	#else
		#define CZ_WRAP_POSIX_MEMALIGN 0
	#endif
#endif

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
CZ_NONNULL_ARG(2) CZ_WR_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czWrap_posix_memalign(int* res, void* restrict* ptr, size_t alignment, size_t size);

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_MALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_malloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_MALLOC)
	#if defined(_WIN32)
		#define CZ_WRAP_ALIGNED_OFFSET_MALLOC 1
	#else
		#define CZ_WRAP_ALIGNED_OFFSET_MALLOC 0
	#endif
#endif

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
CZ_NONNULL_ARGS CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_malloc(void* restrict* res, size_t size, size_t alignment, size_t offset);

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_REALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_realloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_REALLOC)
	#if defined(_WIN32)
		#define CZ_WRAP_ALIGNED_OFFSET_REALLOC 1
	#else
		#define CZ_WRAP_ALIGNED_OFFSET_REALLOC 0
	#endif
#endif

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
CZ_NONNULL_ARG(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_realloc(
	void* restrict* res, void* ptr, size_t size, size_t alignment, size_t offset);

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_RECALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_recalloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_RECALLOC)
	#if defined(_WIN32)
		#define CZ_WRAP_ALIGNED_OFFSET_RECALLOC 1
	#else
		#define CZ_WRAP_ALIGNED_OFFSET_RECALLOC 0
	#endif
#endif

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
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to ( @p count * @p size ).
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_RECALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARG(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_recalloc(
	void* restrict* res, void* ptr, size_t count, size_t size, size_t alignment, size_t offset);

/**
 * @def CZ_WRAP_MADVISE
 * 
 * @brief Specifies whether @c madvise is defined.
 */
#if !defined(CZ_WRAP_MADVISE)
	#if defined(__APPLE__)
		#define CZ_WRAP_MADVISE 1
	#else
		#define CZ_WRAP_MADVISE 0
	#endif
#endif

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
