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
 * @brief Destroys a DyString object.
 * 
 * This function destroys a dynamic string and frees all memory associated with it.
 * 
 * If @p string is NULL, the function does nothing.
 * 
 * @param[in,out] string The dynamic string.
 */
void dystring_destroy(DyString string);

/**
 * @brief Creates a new DyString object.
 * 
 * This function allocates memory and creates a new dynamic string. The string is empty, and all reserved memory is
 * initialised with the null character. On failure, returns NULL.
 * 
 * @param[in] count The number of characters to reserve memory for.
 * 
 * @return The new dynamic string, or NULL.
 */
DyString dystring_create(size_t count) FREE_FUNC(dystring_destroy, 1) USE_RET;


/**
 * @brief Retrieves the length of a DyString object.
 * 
 * This function retrieves the number of characters in a dynamic string, including the null terminator.
 * 
 * @param[in] string The dynamic string. Must not be NULL.
 * 
 * @return The number of characters in the dynamic string.
 */
size_t dystring_length(DyString string) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;

/**
 * @brief Retrieves the raw string of a DyString object.
 * 
 * This function retrieves the underlying raw string of a dynamic string.
 * 
 * @param[in] string The dynamic string. Must not be NULL.
 * 
 * @return The raw string.
 */
char* dystring_raw(DyString string) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;


/**
 * @brief Appends a substring to a DyString object.
 * 
 * This function adds a substring onto the end of a dynamic string. The substring is copied from @p substring. On
 * failure, returns NULL.
 * 
 * @param[in,out] string The dynamic string. Must not be NULL.
 * @param[in] substring The null-terminated substring to append. Must not be NULL.
 * 
 * @return A pointer to the added substring, or NULL.
 */
char* dystring_append(DyString string, const char* restrict substring)
	NONNULL_ARGS_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);

/**
 * @brief Prepends a substring to a DyString object.
 * 
 * This function adds a substring onto the start of a dynamic string. The substring is copied from @p substring. On
 * failure, returns NULL.
 * 
 * @param[in,out] string The dynamic string. Must not be NULL.
 * @param[in] substring The null-terminated substring to prepend. Must not be NULL.
 * 
 * @return A pointer to the added substring, or NULL.
 */
char* dystring_prepend(DyString string, const char* restrict substring)
	NONNULL_ARGS_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);

/**
 * @brief Inserts a substring into a DyString object.
 * 
 * This function adds a substring to a dynamic string at the specified index. The substring is copied from @p substring.
 * On failure, returns NULL.
 * 
 * @param[in,out] string The dynamic string. Must not be NULL.
 * @param[in] substring The null-terminated substring to insert. Must not be NULL.
 * @param[in] index The index at which to insert the substring. Must be less than the length of @p string.
 * 
 * @return A pointer to the added substring, or NULL.
 */
char* dystring_insert(DyString string, const char* restrict substring, size_t index)
	NONNULL_ARGS_ALL NULTSTR_ARG(2) RW_ACCESS(1) RE_ACCESS(2);
