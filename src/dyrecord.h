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
 * @brief A dynamically sized record of dynamic memory allocations.
 */
typedef struct DyRecord_T* DyRecord;

/**
 * @brief A function pointer to a dynamic record free callback.
 */
typedef void (*FreeCallback)(void*);


/**
 * @memberof DyRecord_T
 * 
 * @brief Destroys a DyRecord object.
 * 
 * Destroys @p record and frees all recorded allocations in a LIFO fashion. If @p record is null, nothing happens.
 * 
 * @param[in,out] record The dynamic record.
 */
void dyrecord_destroy(DyRecord record);

/**
 * @memberof DyRecord_T
 * 
 * @brief Creates a new DyRecord object.
 * 
 * Creates an empty dynamic record.
 * 
 * @return The new dynamic record, or null on failure.
 */
DyRecord dyrecord_create(void) FREE_FUNC(dyrecord_destroy, 1) USE_RET;


/**
 * @memberof DyRecord_T
 * 
 * @brief Retrieves the size of a DyRecord object.
 * 
 * Retrieves the number of recorded allocations in @p record.
 * 
 * @param[in] record The dynamic record.
 *
 * @return The number of recorded allocations in @p record.
 * 
 * @pre @p record is nonnull.
 */
size_t dyrecord_size(DyRecord record) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;


/**
 * @memberof DyRecord_T
 * 
 * @brief Adds an allocation to a DyRecord object.
 * 
 * Records a new dynamic memory allocation to @p record.
 * 
 * @param[in,out] record The dynamic record.
 * @param[in] memory The allocated memory.
 * @param[in] callback The free function for the allocation.
 * 
 * @return true, or false on failure.
 * 
 * @pre @p record is nonnull.
 * @pre @p memory is nonnull and does not overlap in memory with @p record.
 * @pre @p callback is nonnull.
 */
bool dyrecord_add(DyRecord record, void* memory, FreeCallback callback) NONNULL_ARGS_ALL RW_ACCESS(1) NO_ACCESS(2);

/**
 * @memberof DyRecord_T
 * 
 * @brief Allocates and adds memory to a DyRecord object.
 * 
 * Dynamically allocates memory via @c malloc. The allocation is recorded to @p record.
 * 
 * @param[in,out] record The dynamic record.
 * @param[in] size The size to pass to malloc.
 * 
 * @return The allocated memory, or null on failure.
 * 
 * @pre @p record is nonnull.
 */
void* dyrecord_malloc(DyRecord record, size_t size) MALLOC_FUNC NONNULL_ARGS_ALL ALLOC_ARG(2) RW_ACCESS(1) USE_RET;

/**
 * @memberof DyRecord_T
 * 
 * @brief Allocates and adds zero initialised memory to a DyRecord object.
 * 
 * Dynamically allocates memory via @c calloc. The allocation is recorded to @p record.
 * 
 * @param[in,out] record The dynamic record.
 * @param[in] count The count to pass to calloc.
 * @param[in] size The size to pass to calloc.
 * 
 * @return The allocated memory, or null on failure.
 * 
 * @pre @p record is nonnull.
 */
void* dyrecord_calloc(DyRecord record, size_t count, size_t size)
	MALLOC_FUNC NONNULL_ARGS_ALL ALLOC_ARGS(2, 3) RW_ACCESS(1) USE_RET;

/**
 * @memberof DyRecord_T
 * 
 * @brief Frees allocations recorded in a DyRecord object.
 * 
 * Frees all recorded allocations in @p record in a LIFO fashion. The recorded allocations are removed from @p record.
 * If @p record is empty, nothing happens.
 * 
 * @param[in,out] record The dynamic record.
 * 
 * @pre @p record is nonnull.
 */
void dyrecord_free(DyRecord record) NONNULL_ARGS_ALL RW_ACCESS(1);
