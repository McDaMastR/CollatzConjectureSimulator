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
 * @struct DyString
 * 
 * @brief A dynamically sized null-terminated byte string.
 */
typedef struct DyString_T* DyString;


/**
 * @memberof DyString
 * 
 * @brief Destroys a dynamic string.
 * 
 * Destroys @p string and frees all associated memory. If @p string is null, nothing happens.
 * 
 * @param[in,out] string The dynamic string.
 * 
 * @warning Once @p string has been destroyed, any further usage of @p string will result in undefined behaviour.
 */
void dystring_destroy(DyString string);

/**
 * @memberof DyString
 * 
 * @brief Creates a new dynamic string.
 * 
 * Creates a dynamic string containing only the null terminator. Memory is preallocated for @p count characters,
 * including the null terminator. All preallocated memory is zero initialised.
 * 
 * @param[in] count The number of characters to preallocate memory for.
 * 
 * @return The new dynamic string, or null on failure.
 * 
 * @pre @p count is nonzero.
 * 
 * @note Failing to destroy the returned dynamic string will result in a memory leak.
 */
DyString dystring_create(size_t count) FREE_FUNC(dystring_destroy, 1) USE_RET;


/**
 * @memberof DyString
 * 
 * @brief Retrieves the length of a dynamic string.
 * 
 * Retrieves the number of characters in @p string, including the null terminator.
 * 
 * @param[in] string The dynamic string.
 * 
 * @return The number of characters in @p string.
 * 
 * @pre @p string is nonnull.
 * 
 * @invariant The length is nonzero.
 */
size_t dystring_length(DyString string) NONNULL_ALL RE_ACCESS(1) USE_RET;

/**
 * @memberof DyString
 * 
 * @brief Retrieves the raw string of a dynamic string.
 * 
 * Retrieves the underlying raw string of @p string.
 * 
 * @param[in] string The dynamic string.
 * 
 * @return The raw string of @p string.
 * 
 * @pre @p string is nonnull.
 * 
 * @invariant The raw string is nonnull.
 * 
 * @note Adding a substring to @p string may result in the raw string changing memory location.
 */
char* dystring_raw(DyString string) NONNULL_ALL RE_ACCESS(1) NONNULL_RET USE_RET;


/**
 * @memberof DyString
 * 
 * @brief Appends a string to a dynamic string.
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
char* dystring_append(DyString string, const char* substring) NONNULL_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);

/**
 * @memberof DyString
 * 
 * @brief Prepends a substring to a dynamic string.
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
char* dystring_prepend(DyString string, const char* substring) NONNULL_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);

/**
 * @memberof DyString
 * 
 * @brief Adds a substring to a dynamic string.
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
	NONNULL_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);
