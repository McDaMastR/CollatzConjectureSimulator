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

#pragma once

#include "defs.h"


/**
 * @brief A dynamically sized FIFO queue.
 */
typedef struct DyQueue_T* DyQueue;


/**
 * @brief Destroys a DyQueue object.
 * 
 * This function destroys a dynamic queue and frees all memory associated with it.
 * 
 * If @p queue is NULL, the function does nothing.
 * 
 * @param[inout] queue The dynamic queue to destroy.
 */
void dyqueue_destroy(DyQueue queue);

/**
 * @brief Creates a new DyQueue object.
 * 
 * This function allocates memory and creates a new dynamic queue. The queue has no elements. On failure, returns NULL.
 * 
 * @param[in] size The number of bytes per element. Must be greater than 0.
 * 
 * @return The new dynamic queue, or NULL.
 */
DyQueue dyqueue_create(size_t size) FREE_FUNC(dyqueue_destroy, 1) USE_RET;


/**
 * @brief Retrieves the size of a DyQueue object.
 * 
 * This function retrieves the number of elements in a dynamic queue.
 * 
 * If @p queue is NULL, the function returns 0.
 * 
 * @param[in] queue The dynamic queue.
 * 
 * @return The number of elements in the dynamic queue.
 */
size_t dyqueue_size(DyQueue queue) RE_ACCESS(1) USE_RET;


/**
 * @brief Adds an element to a DyQueue object.
 * 
 * This function enqueues an element onto the end of a dynamic queue. The element is initialised as a copy of @p value.
 * On failure, returns NULL.
 * 
 * @param[inout] queue The dynamic queue. Must not be NULL.
 * @param[in] value A pointer to the initialising value. Must not be NULL.
 * 
 * @return A pointer to the new element, or NULL.
 */
void* dyqueue_add(DyQueue queue, const void* restrict value) NONNULL_ARGS_ALL RW_ACCESS(1) RE_ACCESS(2);

/**
 * @brief Removes an element from a DyQueue object.
 * 
 * This function dequeues an element from the front of a dynamic queue. The value of the element is copied into
 * @p value.
 * 
 * @param[inout] queue The dynamic queue. Must not be NULL. Must not be empty.
 * @param[out] value A pointer to the memory to copy the element's value into. Must not be NULL.
 */
void dyqueue_pop(DyQueue queue, void* restrict value) NONNULL_ARGS_ALL RW_ACCESS(1) WR_ACCESS(2);
