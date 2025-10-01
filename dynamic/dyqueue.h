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
 * @brief The types and functions for dynamically sized FIFO queues.
 */

#pragma once

#include "common.h"

/**
 * @brief Handle for a dynamically sized FIFO queue.
 */
typedef struct DyQueue_* DyQueue;

/**
 * @brief Destroys a dynamic queue.
 * 
 * Destroys @p queue and frees all associated memory. If @p queue is null, nothing happens.
 * 
 * @param[in,out] queue The dynamic queue.
 * 
 * @warning Once @p queue has been destroyed, any further usage of @p queue will result in undefined behaviour.
 */
void dyqueue_destroy(DyQueue queue);

/**
 * @brief Creates a new dynamic queue.
 * 
 * Creates an empty dynamic queue. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in] size The number of bytes per element.
 * 
 * @return The new dynamic queue, or null on failure.
 * 
 * @pre @p size is nonzero.
 * 
 * @note Failing to destroy the returned dynamic queue may result in a memory leak.
 */
CZ_FREE(dyqueue_destroy, 1) CZ_USE_RET
DyQueue dyqueue_create(size_t size);

/**
 * @brief Retrieves the size of a dynamic queue.
 * 
 * Retrieves the number of elements in @p queue.
 * 
 * @param[in] queue The dynamic queue.
 * 
 * @return The number of elements in @p queue.
 * 
 * @pre @p queue is nonnull.
 */
CZ_PURE CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_USE_RET
size_t dyqueue_size(DyQueue queue);

/**
 * @brief Adds an element to a dynamic queue.
 * 
 * Enqueues an element to the back of @p queue. The element is initialised as a copy of the value pointed to by
 * @p value. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] queue The dynamic queue.
 * @param[in] value The initialising value.
 * 
 * @retval true on success.
 * @retval false on failure.
 * 
 * @pre @p queue is nonnull.
 * @pre @p value is nonnull.
 */
CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_RE_ACCESS(2)
bool dyqueue_enqueue(DyQueue queue, const void* value);

/**
 * @brief Removes an element from a dynamic queue.
 * 
 * Dequeues an element from the front of @p queue. The element is copied into the memory pointed to by @p value.
 * 
 * @param[in,out] queue The dynamic queue.
 * @param[out] value The memory to write the element to.
 * 
 * @pre @p queue is nonnull and nonempty.
 * @pre @p value is nonnull.
 */
CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_WR_ACCESS(2)
void dyqueue_dequeue(DyQueue queue, void* value);
