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
 * @brief Easy and cross-platform dynamic memory management.
 * 
 * A set of configurable functions that can allocate, reallocate, deallocate, and otherwise manage dynamic memory
 * allocations.
 */

#pragma once

#include "def.h"

/**
 * @brief Specifies the behaviour of allocation functions.
 * 
 * A set of flags specifying the desired behaviour of @ref czAlloc, @ref czRealloc, @ref czAllocAlign, or
 * @ref czReallocAlign.
 */
struct CzAllocFlags
{
	/**
	 * @brief Whether to zero out any newly allocated memory.
	 */
	bool zeroInitialise : 1;

	/**
	 * @brief Whether to free the allocated memory if the function fails.
	 */
	bool freeOnFail : 1;
};

/**
 * @brief Dynamically allocates a block of memory.
 * 
 * Allocates @p size bytes of contiguous memory from the heap and synchronously writes the memory address of the first
 * byte of the allocation to @p memory. The allocation is aligned to the fundamental alignment of the implementation.
 * That is, the alignment of the @c max_align_t type (typically 8 or 16 bytes). If @p size is zero or greater than
 * @c PTRDIFF_MAX, failure occurs. On failure, the contents of @p memory are unchanged.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.zeroInitialise is set, the contents of the allocation are initialised to zero. Otherwise, the contents
 *   are initially undefined.
 * - @p flags.freeOnFail is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czAlloc if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czAlloc, @ref czRealloc, @ref czAllocAlign, or @ref czReallocAlign, the
 *   @p memory arguments of @b A and @b B are nonoverlapping in memory. If overlap does occur, the contents of the
 *   overlapping memory are undefined.
 * 
 * @param[out] memory The memory to write the address of the allocation to.
 * @param[in] size The size of the allocation.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero or greater than @c PTRDIFF_MAX.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p memory is nonnull.
 * 
 * @note On success, failing to free the allocation via @ref czFree will result in a memory leak.
 */
CZ_HOT CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czAlloc(void* restrict* memory, size_t size, struct CzAllocFlags flags);

/**
 * @brief Extends or trims a dynamically allocated block of memory.
 * 
 * Reallocates the contiguous dynamic memory allocation of size @p oldSize whose first byte is located at the memory
 * address pointed to by @p memory. The new allocation contains @p newSize bytes of contiguous memory from the heap and
 * is aligned to the fundamental alignment of the implementation. That is, the alignment of the @c max_align_t type
 * (typically 8 or 16 bytes). The memory address of the first byte of the new allocation is synchronously written to
 * @p memory.
 * 
 * Let @e minSize and @e difSize denote the minimum and positive difference, respectively, of @p oldSize and @p newSize.
 * The contents of the first @e minSize bytes of the original allocation are preserved in the first @e minSize bytes of
 * the new allocation. If @p newSize is zero, the original allocation is freed and @p memory is not written to;
 * @p oldSize is ignored. If @p oldSize is zero, or @p oldSize or @p newSize is greater than @c PTRDIFF_MAX, failure
 * occurs. On failure, the contents of @p memory are unchanged.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.zeroInitialise is set and @p oldSize is less than @p newSize, the contents of the last @e difSize bytes
 *   of the new allocation are initialised to zero. Otherwise, the contents are initially undefined.
 * - If @p flags.freeOnFail is set and failure occurs, the original allocation is freed. Otherwise if failure occurs,
 *   the original allocation is unchanged.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czRealloc if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czAlloc, @ref czRealloc, @ref czAllocAlign, or @ref czReallocAlign, the
 *   @p memory arguments of @b A and @b B are nonoverlapping in memory. If overlap does occur, the contents of the
 *   overlapping memory are undefined.
 * 
 * @param[in,out] memory The memory to read the current address of the allocation from, and to write the address of the
 *   new allocation to.
 * @param[in] oldSize The current size of the allocation.
 * @param[in] newSize The size of the new allocation.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_BAD_SIZE @p oldSize was zero, or @p oldSize or @p newSize was greater than @c PTRDIFF_MAX.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p memory and @p *memory are nonnull.
 * @pre @p *memory was allocated via @ref czAlloc or @ref czRealloc.
 * 
 * @note On success if @p newSize is nonzero, or on failure if @p flags.freeOnFail is not set, failing to free the
 * allocation via @ref czFree will result in a memory leak.
 * 
 * @warning On success if @p newSize is zero, or on failure if @p flags.freeOnFail is set, any further access of the
 * freed memory will result in undefined behaviour.
 */
CZ_HOT CZ_NONNULL_ARGS(1) CZ_RW_ACCESS(1)
enum CzResult czRealloc(void* restrict* memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags);

/**
 * @brief Frees a dynamically allocated block of memory.
 * 
 * Deallocates the contiguous dynamic memory allocation whose first byte is located at the memory address @p memory. If
 * @p memory is null, nothing happens.
 * 
 * Thread-safety is guaranteed for any set of concurrent invocations.
 * 
 * @param[in] memory The address of the allocation.
 * 
 * @pre @p memory was allocated via @ref czAlloc or @ref czRealloc.
 * 
 * @warning Any further access of the freed memory will result in undefined behaviour.
 */
CZ_HOT
void czFree(void* memory);

/**
 * @brief Dynamically allocates an aligned block of memory.
 * 
 * Allocates @p size bytes of contiguous memory from the heap and synchronously writes the memory address of the first
 * byte of the allocation to @p memory. The byte at the zero-based position @p offset in the allocation is aligned to
 * @p alignment bytes. If @p size is zero or greater than @c PTRDIFF_MAX, @p alignment is not a power of two, or
 * @p offset is not less than @p size, failure occurs. On failure, the contents of @p memory are unchanged.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.zeroInitialise is set, the contents of the allocation are initialised to zero. Otherwise, the contents
 *   are initially undefined.
 * - @p flags.freeOnFail is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czAllocAlign if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czAlloc, @ref czRealloc, @ref czAllocAlign, or @ref czReallocAlign, the
 *   @p memory arguments of @b A and @b B are nonoverlapping in memory. If overlap does occur, the contents of the
 *   overlapping memory are undefined.
 * 
 * @param[out] memory The memory to write the address of the allocation to.
 * @param[in] size The size of the allocation.
 * @param[in] alignment The alignment of the allocation.
 * @param[in] offset The index of the aligned byte.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was greater than or equal to @p size.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero or greater than @c PTRDIFF_MAX.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p memory is nonnull.
 * 
 * @note On success, failing to free the allocation via @ref czFreeAlign will result in a memory leak.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czAllocAlign(
	void* restrict* memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags);

/**
 * @brief Extends or trims a dynamically allocated and aligned block of memory.
 * 
 * Reallocates the contiguous dynamic memory allocation of size @p oldSize whose first byte is located at the memory
 * address pointed to by @p memory. The new allocation contains @p newSize bytes of contiguous memory from the heap. The
 * byte at the zero-based position @p offset in the new allocation is aligned to @p alignment bytes. The values of
 * @p alignment and @p offset need not be the same as the original allocation. The memory address of the first byte of
 * the new allocation is synchronously written to @p memory.
 * 
 * Let @e minSize and @e difSize denote the minimum and positive difference, respectively, of @p oldSize and @p newSize.
 * The contents of the first @e minSize bytes of the original allocation are preserved in the first @e minSize bytes of
 * the new allocation. If @p newSize is zero, the original allocation is freed and @p memory is not written to;
 * @p oldSize, @p alignment, and @p offset are ignored. If @p oldSize is zero, or @p oldSize or @p newSize is greater
 * than @c PTRDIFF_MAX, failure occurs. If @p alignment is not a power of two or @p offset is not less than @p newSize,
 * failure occurs if @p newSize is nonzero. On failure, the contents of @p memory are unchanged.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.zeroInitialise is set and @p oldSize is less than @p newSize, the contents of the last @e difSize bytes
 *   of the new allocation are initialised to zero. Otherwise, the contents are initially undefined.
 * - If @p flags.freeOnFail is set and failure occurs, the original allocation is freed. Otherwise if failure occurs,
 *   the original allocation is unchanged.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czReallocAlign if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czAlloc, @ref czRealloc, @ref czAllocAlign, or @ref czReallocAlign, the
 *   @p memory arguments of @b A and @b B are nonoverlapping in memory. If overlap does occur, the contents of the
 *   overlapping memory are undefined.
 * 
 * @param[in,out] memory The memory to read the current address of the allocation from, and to write the address of the
 *   new allocation to.
 * @param[in] oldSize The current size of the allocation.
 * @param[in] newSize The size of the new allocation.
 * @param[in] alignment The alignment of the new allocation.
 * @param[in] offset The index of the aligned byte.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was greater than or equal to @p newSize.
 * @retval CZ_RESULT_BAD_SIZE @p oldSize was zero, or @p oldSize or @p newSize was greater than @c PTRDIFF_MAX.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p memory and @p *memory are nonnull.
 * @pre @p *memory was allocated via @ref czAllocAlign or @ref czReallocAlign.
 * 
 * @note On success if @p newSize is nonzero, or on failure if @p flags.freeOnFail is not set, failing to free the
 * allocation via @ref czFreeAlign will result in a memory leak.
 * 
 * @warning On success if @p newSize is zero, or on failure if @p flags.freeOnFail is set, any further access of the
 * freed memory will result in undefined behaviour.
 */
CZ_NONNULL_ARGS(1) CZ_RW_ACCESS(1)
enum CzResult czReallocAlign(
	void* restrict* memory, size_t oldSize, size_t newSize, size_t alignment, size_t offset, struct CzAllocFlags flags);

/**
 * @brief Frees a dynamically allocated and aligned block of memory.
 * 
 * Deallocates the contiguous dynamic memory allocation whose first byte is located at the memory address @p memory. If
 * @p memory is null, nothing happens.
 * 
 * Thread-safety is guaranteed for any set of concurrent invocations.
 * 
 * @param[in] memory The address of the allocation.
 * 
 * @pre @p memory was allocated via @ref czAllocAlign or @ref czReallocAlign.
 * 
 * @warning Any further access of the freed memory will result in undefined behaviour.
 */
void czFreeAlign(void* memory);
