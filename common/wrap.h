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
 * @brief Thin wrappers for common functions.
 * 
 * A set of thin wrapper functions to provide consistent error management. These wrappers are intended for use within
 * cz* API implementations rather than for general use. The majority of wrappers' declarations are split across headers
 * based on the API/library being wrapped, all of which are included in this header.
 */

#pragma once

#include "def.h"
#include "wrap_posix.h"
#include "wrap_stdc.h"
#include "wrap_win32.h"

/**
 * @brief Wraps @c wai_getExecutablePath.
 * 
 * Calls @c wai_getExecutablePath with @p out, @p capacity, and @p dirnameLength. If @p res is nonnull, the returned
 * @c int is synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[out] out The first argument to pass to @c wai_getExecutablePath.
 * @param[in] capacity The second argument to pass to @c wai_getExecutablePath.
 * @param[out] dirnameLength The third argument to pass to @c wai_getExecutablePath.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 */
CZ_WR_ACCESS(1) CZ_WR_ACCESS(2, 3) CZ_WR_ACCESS(4)
enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength);
