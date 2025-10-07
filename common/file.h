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
 * @brief File and IO management.
 */

#pragma once

#include "def.h"

/**
 * @brief Specifies the behaviour of file and IO functions.
 */
struct CzFileFlags
{
	bool relativeToExe : 1;
};

/**
 * @brief Determines if an IO stream is a terminal.
 * 
 * Determines whether @p stream refers to a terminal or terminal emulator (abbreviated TTY) and synchronously writes the
 * result to @p istty. If @p stream does refer to a TTY, @p istty is set to true. Otherwise, @p istty is set to false.
 * If the platform does not provide this ability, @p stream is assumed to not refer to a TTY.
 * 
 * Thread-safety is guaranteed if the @p istty arguments of any concurrent invocations are all nonoverlapping with
 * respect to one another. If overlap does occur, the contents of the overlapping memory are undefined.
 * 
 * @param[in] stream The open IO stream.
 * @param[out] istty The memory to write the result to.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * 
 * @pre @p stream is nonnull.
 * @pre @p istty is nonnull.
 * @pre @p stream and @p istty do not overlap in memory.
 */
CZ_NONNULL_ARGS CZ_WR_ACCESS(2)
enum CzResult czStreamIsTerminal(FILE* stream, bool* istty);

/**
 * @brief Obtains the size of a file.
 * 
 * Determines the size of the file located at the filepath @p path and synchronously writes the file size to @p size.
 * The file size is measured in bytes. If the filepath style of @p path is not POSIX or Windows style, or is unsupported
 * by the platform, failure occurs. If @p path is an invalid filepath or locates a nonexistent or invalid resource,
 * failure occurs. If @p path locates a symbolic link, the link is followed. If @p path locates any other non-regular
 * file resource, such as a pipe or socket, the behaviour is platform dependent. On failure, the contents of @p size are
 * unchanged.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * 
 * Thread-safety is guaranteed if for any concurrent invocations, the @p path arguments all locate distinct system
 * resources and the @p size arguments are all nonoverlapping with respect to one another. If multiple @p path arguments
 * locate the same resource, such as the same file or directory, the behaviour is undefined. If multiple @p size
 * arguments overlap in memory, the contents of the overlapping memory are undefined.
 * 
 * @param[in] path The path to the file.
 * @param[out] size The memory to write the size to.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid filepath.
 * @retval CZ_RESULT_NO_FILE The file does not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p path is nonnull and null-terminated.
 * @pre @p size is nonnull.
 * @pre @p path and @p size do not overlap in memory.
 */
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1) CZ_RE_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czFileSize(const char* path, size_t* size, struct CzFileFlags flags);
