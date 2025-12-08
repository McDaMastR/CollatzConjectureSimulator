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

#include "wrap.h"

enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength)
{
	int r = wai_getExecutablePath(out, capacity, dirnameLength);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1)
		return CZ_RESULT_SUCCESS;
	return CZ_RESULT_INTERNAL_ERROR;
}
