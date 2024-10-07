/* 
 * Copyright (C) 2024  Seth McDonald <seth.i.mcdonald@gmail.com>
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

#include "dyarray.h"
#include "debug.h"


struct DyArray_T
{
	size_t size;     // # bytes per element
	size_t count;    // # elements currently in array
	size_t capacity; // Maximum # elements that could fit in allocated memory
	void*  array;    // Raw array
};


void DyArray_destroy(restrict DyArray array)
{
	free(array->array);
	free(array);
}

DyArray DyArray_create(size_t size, size_t count)
{
	ASSUME(size > 0);

	DyArray array = (DyArray) malloc(sizeof(struct DyArray_T));
#ifndef NDEBUG
	if (!array) {
		MALLOC_FAILURE(array, sizeof(struct DyArray_T))
		return NULL;
	}
#endif

	array->size     = size;
	array->count    = 0;
	array->capacity = count;
	array->array    = NULL;

	if (EXPECT_TRUE(count)) {
		void* raw = malloc(count * size);
#ifndef NDEBUG
		if (!raw) {
			MALLOC_FAILURE(raw, count * size)
			free(array);
			return NULL;
		}
#endif

		array->array = raw;
	}

	return array;
}


size_t DyArray_size(restrict DyArray array)
{
	return array->count;
}

void* DyArray_raw(restrict DyArray array)
{
	return array->array;
}


void DyArray_get(restrict DyArray array, void* restrict value, size_t index)
{
	ASSUME(array->size > 0);

	size_t size = array->size;

	const void* element = (char*) array->array + index * size;

	memcpy(value, element, size);
}

void DyArray_set(restrict DyArray array, const void* restrict value, size_t index)
{
	ASSUME(array->size > 0);

	size_t size = array->size;

	void* element = (char*) array->array + index * size;

	memcpy(element, value, size);
}

void DyArray_last(restrict DyArray array, void* restrict value)
{
	ASSUME(array->size > 0);

	size_t size  = array->size;
	size_t count = array->count;

	const void* element = (char*) array->array + (count - 1) * size;

	memcpy(value, element, size);
}

void DyArray_first(restrict DyArray array, void* restrict value)
{
	ASSUME(array->size > 0);

	size_t size = array->size;

	const void* element = array->array;

	memcpy(value, element, size);
}


void* DyArray_append(restrict DyArray array, const void* restrict value)
{
	ASSUME(array->size > 0);

	size_t size     = array->size;
	size_t count    = array->count;
	size_t capacity = array->capacity;
	void*  raw      = array->array;

	if (EXPECT_FALSE(count == capacity)) {
		capacity += (capacity + 1) / 2 + 1;

		void* raw2 = realloc(raw, capacity * size);
#ifndef NDEBUG
		if (!raw2) {
			REALLOC_FAILURE(raw2, raw, capacity * size)
			return NULL;
		}
#endif

		raw = raw2;

		array->capacity = capacity;
		array->array    = raw;
	}

	void* element = (char*) raw + count * size;

	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}

void* DyArray_prepend(restrict DyArray array, const void* restrict value)
{
	ASSUME(array->size > 0);

	size_t size     = array->size;
	size_t count    = array->count;
	size_t capacity = array->capacity;
	void*  raw      = array->array;

	if (EXPECT_FALSE(count == capacity)) {
		capacity += (capacity + 1) / 2 + 1;

		void* raw2 = realloc(raw, capacity * size);
#ifndef NDEBUG
		if (!raw2) {
			REALLOC_FAILURE(raw2, raw, capacity * size)
			return NULL;
		}
#endif

		raw = raw2;

		array->capacity = capacity;
		array->array    = raw;
	}

	void* element = raw;

	memmove((char*) raw + size, raw, count * size);
	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}
