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

#include "common.h"


/**
 * @struct DyArray
 * 
 * @brief A dynamically sized array.
 */
typedef struct DyArray_* DyArray;


/**
 * @memberof DyArray
 * 
 * @brief Destroys a dynamic array.
 * 
 * Destroys @p array and frees all associated memory. If @p array is null, nothing happens.
 * 
 * @param[in,out] array The dynamic array.
 * 
 * @warning Once @p array has been destroyed, any further usage of @p array will result in undefined behaviour.
 */
void dyarray_destroy(DyArray array);

/**
 * @memberof DyArray
 * 
 * @brief Creates a new dynamic array.
 * 
 * Creates an empty dynamic array. If @p count is nonzero, memory is preallocated for @p count elements. Any
 * preallocated memory is not initialised. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in] size The number of bytes per element.
 * @param[in] count The number of elements to preallocate memory for.
 * 
 * @return The new dynamic array, or null on failure.
 * 
 * @pre @p size is nonzero.
 * 
 * @note Failing to destroy the returned dynamic array will result in a memory leak.
 */
DyArray dyarray_create(size_t size, size_t count) CZ_FREE(dyarray_destroy, 1) CZ_USE_RET;

/**
 * @memberof DyArray
 * 
 * @brief Retrieves the size of a dynamic array.
 * 
 * Retrieves the number of elements in @p array.
 * 
 * @param[in] array The dynamic array.
 * 
 * @return The number of elements in @p array.
 * 
 * @pre @p array is nonnull.
 */
size_t dyarray_size(DyArray array) CZ_PURE CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_USE_RET;

/**
 * @memberof DyArray
 * 
 * @brief Retrieves the raw array of a dynamic array.
 * 
 * Retrieves the underlying raw array of @p array.
 * 
 * @param[in] array The dynamic array.
 * 
 * @return The raw array of @p array.
 * 
 * @pre @p array is nonnull.
 * 
 * @note Adding an element to @p array may result in the raw array changing memory location.
 */
void* dyarray_raw(DyArray array) CZ_PURE CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_USE_RET;

/**
 * @memberof DyArray
 * 
 * @brief Retrieves an element in a dynamic array.
 * 
 * Copies the element in @p array at the zero-based position @p index into the memory pointed to by @p value.
 * 
 * @param[in] array The dynamic array.
 * @param[out] value The memory to write the element to.
 * @param[in] index The index of the element.
 * 
 * @pre @p array is nonnull and nonempty.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 * @pre @p index is less than the size of @p array.
 */
void dyarray_get(DyArray array, void* value, size_t index) CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_WR_ACCESS(2);

/**
 * @memberof DyArray
 * 
 * @brief Sets an element in a dynamic array.
 * 
 * Copies the value pointed to by @p value into the element in @p array at the zero-based position @p index.
 * 
 * @param[in,out] array The dynamic array.
 * @param[in] value The value to set the element to.
 * @param[in] index The index of the element.
 * 
 * @pre @p array is nonnull and nonempty.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 * @pre @p index is less than the size of @p array.
 */
void dyarray_set(DyArray array, const void* value, size_t index) CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_RE_ACCESS(2);

/**
 * @memberof DyArray
 * 
 * @brief Retrieves the last element in a dynamic array.
 * 
 * Copies the last element in @p array into the memory pointed to by @p value.
 * 
 * @param[in] array The dynamic array.
 * @param[out] value The memory to write the element to.
 * 
 * @pre @p array is nonnull and nonempty.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 */
void dyarray_last(DyArray array, void* value) CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_WR_ACCESS(2);

/**
 * @memberof DyArray
 * 
 * @brief Retrieves the first element in a dynamic array.
 * 
 * Copies the first element in @p array into the memory pointed to by @p value.
 * 
 * @param[in] array The dynamic array.
 * @param[out] value The memory to write the element to.
 * 
 * @pre @p array is nonnull and nonempty.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 */
void dyarray_first(DyArray array, void* value) CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_WR_ACCESS(2);

/**
 * @memberof DyArray
 * 
 * @brief Appends an element to a dynamic array.
 * 
 * Adds a new element to the end of @p array. The element is initialised as a copy of the value pointed to by @p value.
 * Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] array The dynamic array.
 * @param[in] value The initialising value.
 * 
 * @return A pointer to the new element in @p array, or null on failure.
 * 
 * @pre @p array is nonnull.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 */
void* dyarray_append(DyArray array, const void* value) CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_RE_ACCESS(2);

/**
 * @memberof DyArray
 * 
 * @brief Prepends an element to a dynamic array.
 * 
 * Adds a new element to the start of @p array. The element is initialised as a copy of the value pointed to by
 * @p value. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] array The dynamic array.
 * @param[in] value The initialising value.
 * 
 * @return A pointer to the new element in @p array, or null on failure.
 * 
 * @pre @p array is nonnull.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 */
void* dyarray_prepend(DyArray array, const void* value) CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_RE_ACCESS(2);

/**
 * @memberof DyArray
 * 
 * @brief Adds an element to a dynamic array.
 * 
 * Adds a new element to @p array at the zero-based position @p index. The element is initialised as a copy of the value
 * pointed to by @p value. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] array The dynamic array.
 * @param[in] value The initialising value.
 * @param[in] index The index of the element.
 * 
 * @return A pointer to the new element in @p array, or null on failure.
 * 
 * @pre @p array is nonnull.
 * @pre @p value is nonnull.
 * @pre @p array and @p value do not overlap in memory.
 * @pre @p index is less than or equal to the size of @p array.
 */
void* dyarray_add(DyArray array, const void* value, size_t index) CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_RE_ACCESS(2);
