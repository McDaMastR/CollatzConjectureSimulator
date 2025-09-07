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
#include "dyarray.h"
#include "dyqueue.h"
#include "dyrecord.h"
#include "dystring.h"


/**
 * @relates DyRecord_T
 * 
 * @brief @c FreeCallback compatible version of @c dyarray_destroy.
 * 
 * Calls @c dyarray_destroy with @p array. Can be used as the free callback when recording a dynamic array to a dynamic
 * record.
 * 
 * @param array The dynamic array.
 */
void dyarray_destroy_stub(void* array);

/**
 * @relates DyRecord_T
 * 
 * @brief @c FreeCallback compatible version of @c dyqueue_destroy.
 * 
 * Calls @c dyqueue_destroy with @p queue. Can be used as the free callback when recording a dynamic queue to a dynamic
 * record.
 * 
 * @param queue The dynamic queue.
 */
void dyqueue_destroy_stub(void* queue);

/**
 * @relates DyRecord_T
 * 
 * @brief @c FreeCallback compatible version of @c dyrecord_destroy.
 * 
 * Calls @c dyrecord_destroy with @p record. Can be used as the free callback when recording a dynamic record to another
 * dynamic record.
 * 
 * @param record The dynamic record.
 */
void dyrecord_destroy_stub(void* record);

/**
 * @relates DyRecord_T
 * 
 * @brief @c FreeCallback compatible version of @c dystring_destroy.
 * 
 * Calls @c dystring_destroy with @p string. Can be used as the free callback when recording a dynamic string to a
 * dynamic record.
 * 
 * @param string The dynamic string.
 */
void dystring_destroy_stub(void* string);
