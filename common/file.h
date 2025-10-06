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
	bool relativeToExecutable : 1;
	bool followSymbolicLinks : 1;
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
 */
CZ_NONNULL_ARGS CZ_WR_ACCESS(2)
enum CzResult czStreamIsTerminal(FILE* stream, bool* istty);
