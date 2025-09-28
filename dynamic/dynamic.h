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

#include "common.h"
#include "dyarray.h"
#include "dyqueue.h"
#include "dyrecord.h"
#include "dystring.h"


/**
 * @relates DyRecord
 * 
 * @brief FreeCallback compatible version of @ref DyArray::dyarray_destroy "dyarray_destroy".
 * 
 * Calls DyArray::dyarray_destroy with @p array. Can be used as the free callback when recording a dynamic array to a
 * dynamic record.
 * 
 * @param[in,out] array The dynamic array.
 */
void dyarray_destroy_stub(void* array);

/**
 * @relates DyRecord
 * 
 * @brief FreeCallback compatible version of @ref DyQueue::dyqueue_destroy "dyqueue_destroy".
 * 
 * Calls DyQueue::dyqueue_destroy with @p queue. Can be used as the free callback when recording a dynamic queue to a
 * dynamic record.
 * 
 * @param[in,out] queue The dynamic queue.
 */
void dyqueue_destroy_stub(void* queue);

/**
 * @relates DyRecord
 * 
 * @brief FreeCallback compatible version of @ref DyRecord::dyrecord_destroy "dyrecord_destroy".
 * 
 * Calls DyRecord::dyrecord_destroy with @p record. Can be used as the free callback when recording a dynamic record to
 * another dynamic record.
 * 
 * @param[in,out] record The dynamic record.
 */
void dyrecord_destroy_stub(void* record);

/**
 * @relates DyRecord
 * 
 * @brief FreeCallback compatible version of @ref DyString::dystring_destroy "dystring_destroy".
 * 
 * Calls DyString::dystring_destroy with @p string. Can be used as the free callback when recording a dynamic string to
 * a dynamic record.
 * 
 * @param[in,out] string The dynamic string.
 */
void dystring_destroy_stub(void* string);
