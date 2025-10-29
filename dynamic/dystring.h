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
 * @brief The types and functions for dynamically sized NUL-terminated byte strings.
 */

#pragma once

#include "common.h"

/**
 * @brief Handle for a dynamically sized NUL-terminated byte string.
 */
typedef struct DyString_* DyString;

/**
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
 * @brief Creates a new dynamic string.
 * 
 * Creates a dynamic string containing only the NUL terminator. Memory is preallocated for @p count characters,
 * including the NUL terminator. All preallocated memory is zero initialised. Failure can occur if sufficient memory is
 * unable to be allocated.
 * 
 * @param[in] count The number of characters to preallocate memory for.
 * 
 * @return The new dynamic string, or null on failure.
 * 
 * @pre @p count is nonzero.
 * 
 * @note Failing to destroy the returned dynamic string may result in a memory leak.
 */
CZ_FREE(dystring_destroy, 1) CZ_USE_RET
DyString dystring_create(size_t count);

/**
 * @brief Retrieves the length of a dynamic string.
 * 
 * Retrieves the number of characters in @p string, including the NUL terminator.
 * 
 * @param[in] string The dynamic string.
 * 
 * @return The number of characters in @p string.
 * 
 * @pre @p string is nonnull.
 * 
 * @invariant The length is nonzero.
 */
CZ_PURE CZ_NONNULL_ARGS() CZ_RD_ACCESS(1) CZ_USE_RET
size_t dystring_length(DyString string);

/**
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
CZ_PURE CZ_NONNULL_ARGS() CZ_RD_ACCESS(1) CZ_NONNULL_RET CZ_USE_RET
char* dystring_raw(DyString string);

/**
 * @brief Appends a string to a dynamic string.
 * 
 * Lengthens @p string and copies the string pointed to by @p substring into the lengthened end of @p string. Failure
 * can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] string The dynamic string.
 * @param[in] substring The string to copy from.
 * 
 * @return A pointer to the added substring in @p string, or null on failure.
 * 
 * @pre @p string is nonnull.
 * @pre @p substring is nonnull and NUL-terminated.
 * @pre @p string and @p substring do not overlap in memory.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_RW_ACCESS(1) CZ_RD_ACCESS(2)
char* dystring_append(DyString string, const char* substring);

/**
 * @brief Prepends a substring to a dynamic string.
 * 
 * Lengthens @p string and copies the string pointed to by @p substring into the lengthened start of @p string. Failure
 * can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] string The dynamic string.
 * @param[in] substring The string to copy from.
 * 
 * @return A pointer to the added substring in @p string, or null on failure.
 * 
 * @pre @p string is nonnull.
 * @pre @p substring is nonnull and NUL-terminated.
 * @pre @p string and @p substring do not overlap in memory.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_RW_ACCESS(1) CZ_RD_ACCESS(2)
char* dystring_prepend(DyString string, const char* substring);

/**
 * @brief Adds a substring to a dynamic string.
 * 
 * Lengthens @p string and copies the string pointed to by @p substring into the lengthened part of @p string at the
 * zero-based position @p index. Failure can occur if sufficient memory is unable to be allocated.
 * 
 * @param[in,out] string The dynamic string.
 * @param[in] substring The string to copy from.
 * @param[in] index The index to lengthen at.
 * 
 * @return A pointer to the added substring in @p string, or null on failure.
 * 
 * @pre @p string is nonnull.
 * @pre @p substring is nonnull and NUL-terminated.
 * @pre @p string and @p substring do not overlap in memory.
 * @pre @p index is less than the length of @p string.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_RW_ACCESS(1) CZ_RD_ACCESS(2)
char* dystring_add(DyString string, const char* substring, size_t index);
