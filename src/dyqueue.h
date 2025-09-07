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
 * @memberof DyQueue_T
 * 
 * @brief Destroys a DyQueue object.
 * 
 * Destroys @p queue and frees all associated memory. If @p queue is null, nothing happens.
 * 
 * @param[in,out] queue The dynamic queue.
 */
void dyqueue_destroy(DyQueue queue);

/**
 * @memberof DyQueue_T
 * 
 * @brief Creates a new DyQueue object.
 * 
 * Creates an empty dynamic queue.
 * 
 * @param[in] size The number of bytes per element.
 * 
 * @return The new dynamic queue, or null on failure.
 * 
 * @pre @p size is nonzero.
 */
DyQueue dyqueue_create(size_t size) FREE_FUNC(dyqueue_destroy, 1) USE_RET;


/**
 * @memberof DyQueue_T
 * 
 * @brief Retrieves the size of a DyQueue object.
 * 
 * Retrieves the number of elements in @p queue.
 * 
 * @param[in] queue The dynamic queue.
 * 
 * @return The number of elements in @p queue.
 * 
 * @pre @p queue is nonnull.
 */
size_t dyqueue_size(DyQueue queue) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;


/**
 * @memberof DyQueue_T
 * 
 * @brief Adds an element to a DyQueue object.
 * 
 * Enqueues an element to the back of @p queue. The element is initialised as a copy of the value pointed to by
 * @p value.
 * 
 * @param[in,out] queue The dynamic queue.
 * @param[in] value The initialising value.
 * 
 * @return true, or false on failure.
 * 
 * @pre @p queue is nonnull.
 * @pre @p value is nonnull.
 */
bool dyqueue_enqueue(DyQueue queue, const void* value) NONNULL_ARGS_ALL RW_ACCESS(1) RE_ACCESS(2);

/**
 * @memberof DyQueue_T
 * 
 * @brief Removes an element from a DyQueue object.
 * 
 * Dequeues an element from the front of @p queue. The element is copied into the memory pointed to by @p value.
 * 
 * @param[in,out] queue The dynamic queue.
 * @param[out] value The memory to write the element to.
 * 
 * @pre @p queue is nonnull and nonempty.
 * @pre @p value is nonnull and does not overlap in memory with @p queue.
 */
void dyqueue_dequeue(DyQueue queue, void* value) NONNULL_ARGS_ALL RW_ACCESS(1) WR_ACCESS(2);
