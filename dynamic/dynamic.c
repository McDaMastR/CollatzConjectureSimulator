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

#include "dynamic.h"


void czFree_stub(void* memory)
{
	czFree(memory);
}

void dyarray_destroy_stub(void* array)
{
	dyarray_destroy(array);
}

void dyqueue_destroy_stub(void* queue)
{
	dyqueue_destroy(queue);
}

void dyrecord_destroy_stub(void* record)
{
	dyrecord_destroy(record);
}

void dystring_destroy_stub(void* string)
{
	dystring_destroy(string);
}
