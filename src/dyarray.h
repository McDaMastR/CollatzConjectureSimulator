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

#include "defs.h"


/**
 * @brief A dynamically sized array.
 */
typedef struct DyArray_T* DyArray;


/**
 * @brief Destroys a DyArray object.
 * 
 * This function destroys a dynamic array and frees all memory associated with it.
 * 
 * If @p array is NULL, the function does nothing.
 * 
 * @param[in,out] array The dynamic array.
 */
void dyarray_destroy(DyArray array);

/**
 * @brief Creates a new DyArray object.
 * 
 * This function allocates memory and creates a new dynamic array. The array has no elements, and any reserved memory is
 * not initialised. On failure, returns NULL.
 * 
 * @param[in] size The number of bytes per element. Must be greater than 0.
 * @param[in] count The number of elements to reserve memory for.
 * 
 * @return The new dynamic array, or NULL.
 */
DyArray dyarray_create(size_t size, size_t count) FREE_FUNC(dyarray_destroy, 1) USE_RET;


/**
 * @brief Retrieves the size of a DyArray object.
 * 
 * This function retrieves the number of elements in a dynamic array.
 * 
 * @param[in] array The dynamic array. Must not be NULL.
 * 
 * @return The number of elements in the dynamic array.
 */
size_t dyarray_size(DyArray array) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;

/**
 * @brief Retrieves the raw array of a DyArray object.
 * 
 * This function retrieves the underlying raw array of a dynamic array.
 * 
 * @param[in] array The dynamic array. Must not be NULL.
 * 
 * @return The raw array.
 */
void* dyarray_raw(DyArray array) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;


/**
 * @brief Retrieves an element from a DyArray object.
 * 
 * This function copies the value of an element in a dynamic array into @p value.
 * 
 * @param[in] array The non-empty dynamic array. Must not be NULL.
 * @param[out] value A pointer to the memory to copy the retrieved value into. Must not be NULL.
 * @param[in] index The index of the element to retrieve. Must be less than the size of @p array.
 */
void dyarray_get(DyArray array, void* restrict value, size_t index) NONNULL_ARGS_ALL RE_ACCESS(1) WR_ACCESS(2);

/**
 * @brief Sets an element from a DyArray object.
 * 
 * This function copies the value of @p value into an element in a dynamic array.
 * 
 * @param[in,out] array The non-empty dynamic array. Must not be NULL.
 * @param[in] value A pointer to the value to set the element to. Must not be NULL.
 * @param[in] index The index of the element to set. Must be less than the size of @p array.
 */
void dyarray_set(DyArray array, const void* restrict value, size_t index) NONNULL_ARGS_ALL RW_ACCESS(1) RE_ACCESS(2);

/**
 * @brief Retrieves the last element from a DyArray object.
 * 
 * This function copies the value of the last element in a dynamic array into @p value.
 * 
 * @param[in] array The non-empty dynamic array. Must not be NULL.
 * @param[out] value A pointer to the memory to copy the retrieved value into. Must not be NULL.
 */
void dyarray_last(DyArray array, void* restrict value) NONNULL_ARGS_ALL RE_ACCESS(1) WR_ACCESS(2);

/**
 * @brief Retrieves the first element from a DyArray object.
 * 
 * This function copies the value of the first element in a dynamic array into @p value.
 * 
 * @param[in] array The non-empty dynamic array. Must not be NULL.
 * @param[out] value A pointer to the memory to copy the retrieved value into. Must not be NULL.
 */
void dyarray_first(DyArray array, void* restrict value) NONNULL_ARGS_ALL RE_ACCESS(1) WR_ACCESS(2);


/**
 * @brief Appends an element to a DyArray object.
 * 
 * This function adds a new element onto the end of a dynamic array. The new element is initialised as a copy of
 * @p value. On failure, returns NULL.
 * 
 * @param[in,out] array The dynamic array. Must not be NULL.
 * @param[in] value A pointer to the initialising value. Must not be NULL.
 * 
 * @return A pointer to the new element, or NULL.
 */
void* dyarray_append(DyArray array, const void* restrict value) NONNULL_ARGS_ALL RW_ACCESS(1) RE_ACCESS(2);

/**
 * @brief Prepends an element to a DyArray object.
 * 
 * This function adds a new element onto the start of a dynamic array. The new element is initialised as a copy of
 * @p value. On failure, returns NULL.
 * 
 * @param[in,out] array The dynamic array. Must not be NULL.
 * @param[in] value A pointer to the initialising value. Must not be NULL.
 * 
 * @return A pointer to the new element, or NULL.
 */
void* dyarray_prepend(DyArray array, const void* restrict value) NONNULL_ARGS_ALL RW_ACCESS(1) RE_ACCESS(2);

/**
 * @brief Inserts an element into a DyArray object.
 * 
 * This function adds a new element to a dynamic array at the specified index. The new element is initialised as a copy
 * of @p value. On failure, returns NULL.
 * 
 * @param[in,out] array The dynamic array. Must not be NULL.
 * @param[in] value A pointer to the initialising value. Must not be NULL.
 * @param[in] index The index of the new element. Must be less than or equal to the size of @p array.
 * 
 * @return A pointer to the new element, or NULL.
 */
void* dyarray_insert(DyArray array, const void* restrict value, size_t index)
	NONNULL_ARGS_ALL RW_ACCESS(1) RE_ACCESS(2);
