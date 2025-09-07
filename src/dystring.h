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
 * @brief A dynamically sized null-terminated byte string.
 */
typedef struct DyString_T* DyString;


/**
 * @memberof DyString_T
 * 
 * @brief Destroys a DyString object.
 * 
 * Destroys @p string and frees all associated memory. If @p string is null, nothing happens.
 * 
 * @param[in,out] string The dynamic string.
 */
void dystring_destroy(DyString string);

/**
 * @memberof DyString_T
 * 
 * @brief Creates a new DyString object.
 * 
 * Creates a dynamic string containing only the null terminator. Memory is preallocated for @p count characters,
 * including the null terminator. All preallocated memory is zero initialised.
 * 
 * @param[in] count The number of characters to preallocate memory for.
 * 
 * @return The new dynamic string, or null on failure.
 * 
 * @pre @p count is nonzero.
 */
DyString dystring_create(size_t count) FREE_FUNC(dystring_destroy, 1) USE_RET;


/**
 * @memberof DyString_T
 * 
 * @brief Retrieves the length of a DyString object.
 * 
 * Retrieves the number of characters in @p string, including the null terminator.
 * 
 * @param[in] string The dynamic string.
 * 
 * @return The number of characters in @p string.
 * 
 * @pre @p string is nonnull.
 */
size_t dystring_length(DyString string) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;

/**
 * @memberof DyString_T
 * 
 * @brief Retrieves the raw string of a DyString object.
 * 
 * Retrieves the underlying raw string of @p string.
 * 
 * @param[in] string The dynamic string.
 * 
 * @return The raw string of @p string.
 * 
 * @pre @p string is nonnull.
 */
char* dystring_raw(DyString string) NONNULL_ARGS_ALL RE_ACCESS(1) NONNULL_RET USE_RET;


/**
 * @memberof DyString_T
 * 
 * @brief Appends a string to a DyString object.
 * 
 * Lengthens @p string and copies the string pointed to by @p substring into the lengthened end of @p string.
 * 
 * @param[in,out] string The dynamic string.
 * @param[in] substring The string to copy from.
 * 
 * @return A pointer to the added substring in @p string, or null on failure.
 * 
 * @pre @p string is nonnull.
 * @pre @p substring is nonnull, null-terminated, and does not overlap in memory with @p string.
 */
char* dystring_append(DyString string, const char* substring) NONNULL_ARGS_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);

/**
 * @memberof DyString_T
 * 
 * @brief Prepends a substring to a DyString object.
 * 
 * Lengthens @p string and copies the string pointed to by @p substring into the lengthened start of @p string.
 * 
 * @param[in,out] string The dynamic string.
 * @param[in] substring The string to copy from.
 * 
 * @return A pointer to the added substring in @p string, or null on failure.
 * 
 * @pre @p string is nonnull.
 * @pre @p substring is nonnull, null-terminated, and does not overlap in memory with @p string.
 */
char* dystring_prepend(DyString string, const char* substring)
	NONNULL_ARGS_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);

/**
 * @memberof DyString_T
 * 
 * @brief Adds a substring into a DyString object.
 * 
 * Lengthens @p string and copies the string pointed to by @p substring into the lengthened part of @p string at the
 * zero-based position @p index.
 * 
 * @param[in,out] string The dynamic string.
 * @param[in] substring The string to copy from.
 * @param[in] index The index to lengthen at.
 * 
 * @return A pointer to the added substring in @p string, or null on failure.
 * 
 * @pre @p string is nonnull.
 * @pre @p substring is nonnull, null-terminated, and does not overlap in memory with @p string.
 * @pre @p index is less than the length of @p string.
 */
char* dystring_add(DyString string, const char* substring, size_t index)
	NONNULL_ARGS_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);
