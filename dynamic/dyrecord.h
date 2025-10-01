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
 * @brief The types and functions for dynamically sized records of dynamic memory allocations.
 */

#pragma once

#include "common.h"

/**
 * @brief Handle for a dynamically sized record of dynamic memory allocations.
 */
typedef struct DyRecord_* DyRecord;

/**
 * @brief A memory freeing callback.
 * 
 * A user-defined function which frees the allocated memory pointed to by @p memory.
 * 
 * @param[in,out] memory The dynamically allocated memory.
 */
typedef void (*FreeCallback)(void* memory);

/**
 * @brief Destroys a dynamic record.
 * 
 * Destroys @p record and frees all recorded allocations in a LIFO fashion. If @p record is null, nothing happens.
 * 
 * @param[in,out] record The dynamic record.
 * 
 * @warning Once @p record has been destroyed, any further usage of @p record will result in undefined behaviour.
 */
void dyrecord_destroy(DyRecord record);

/**
 * @brief Creates a new dynamic record.
 * 
 * Creates an empty dynamic record. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @return The new dynamic record, or null on failure.
 * 
 * @note Failing to destroy the returned dynamic record may result in a memory leak.
 */
CZ_FREE(dyrecord_destroy, 1) CZ_USE_RET
DyRecord dyrecord_create(void);

/**
 * @brief Retrieves the size of a dynamic record.
 * 
 * Retrieves the number of recorded allocations in @p record.
 * 
 * @param[in] record The dynamic record.
 *
 * @return The number of recorded allocations in @p record.
 * 
 * @pre @p record is nonnull.
 */
CZ_PURE CZ_NONNULL_ARGS CZ_RE_ACCESS(1) CZ_USE_RET
size_t dyrecord_size(DyRecord record);

/**
 * @brief Adds an allocation to a dynamic record.
 * 
 * Records the dynamic memory allocation pointed to by @p memory to @p record. Failure can occur if sufficient memory is
 * unable to be allocated.
 * 
 * @param[in,out] record The dynamic record.
 * @param[in] memory The allocated memory.
 * @param[in] callback The free function for the allocation.
 * 
 * @retval true on success.
 * @retval false on failure.
 * 
 * @pre @p record is nonnull.
 * @pre @p memory is nonnull.
 * @pre @p record and @p memory do not overlap in memory.
 * @pre @p callback is nonnull.
 */
CZ_NONNULL_ARGS CZ_RW_ACCESS(1) CZ_NO_ACCESS(2)
bool dyrecord_add(DyRecord record, void* memory, FreeCallback callback);

/**
 * @brief Allocates and adds memory to a dynamic record.
 * 
 * Dynamically allocates memory via @c malloc. The allocation is recorded to @p record. Failure can occur if sufficient
 * memory is unable to be allocated.
 * 
 * @param[in,out] record The dynamic record.
 * @param[in] size The size to pass to malloc.
 * 
 * @return The allocated memory, or null on failure.
 * 
 * @pre @p record is nonnull.
 */
CZ_MALLOC CZ_NONNULL_ARGS CZ_ALLOC_ARG(2) CZ_RW_ACCESS(1) CZ_USE_RET
void* dyrecord_malloc(DyRecord record, size_t size);

/**
 * @brief Allocates and adds zero initialised memory to a dynamic record.
 * 
 * Dynamically allocates memory via @c calloc. The allocation is recorded to @p record. Failure can occur if sufficient
 * memory is unable to be allocated.
 * 
 * @param[in,out] record The dynamic record.
 * @param[in] count The count to pass to calloc.
 * @param[in] size The size to pass to calloc.
 * 
 * @return The allocated memory, or null on failure.
 * 
 * @pre @p record is nonnull.
 */
CZ_MALLOC CZ_NONNULL_ARGS CZ_ALLOC_ARGS(2, 3) CZ_RW_ACCESS(1) CZ_USE_RET
void* dyrecord_calloc(DyRecord record, size_t count, size_t size);

/**
 * @brief Frees allocations recorded in a dynamic record.
 * 
 * Frees all recorded allocations in @p record in a LIFO fashion. The recorded allocations are removed from @p record.
 * If @p record is empty, nothing happens.
 * 
 * @param[in,out] record The dynamic record.
 * 
 * @pre @p record is nonnull.
 */
CZ_NONNULL_ARGS CZ_RW_ACCESS(1)
void dyrecord_free(DyRecord record);
