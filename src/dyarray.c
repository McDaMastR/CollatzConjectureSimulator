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

#include "defs.h"
#include "dyarray.h"


static const float ALLOC_RATE = 1.5f;

struct DyArray
{
	size_t size; // # bytes per element
	size_t count; // # elements currently in array
	size_t capacity; // Maximum # elements that could fit in allocated memory
	void*  array; // Raw array
};


DyArray* DyArray_create(size_t size, size_t count)
{
	DyArray* array = (DyArray*) malloc(sizeof(DyArray));
#ifndef NDEBUG
	if (!array) {
		MALLOC_FAILURE(array, sizeof(DyArray))
		return NULL;
	}
#endif

	array->size     = size;
	array->count    = 0;
	array->capacity = count;
	array->array    = NULL;

	if (count) {
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

void DyArray_destroy(DyArray* restrict array)
{
	free(array->array);
	free(array);
}


size_t DyArray_size(const DyArray* restrict array)
{
	return array->count;
}

void* DyArray_raw(const DyArray* restrict array)
{
	return array->array;
}


void DyArray_get(const DyArray* restrict array, void* restrict value, size_t index)
{
	size_t size = array->size;

	const void* element = (char*) array->array + index * size;

	memcpy(value, element, size);
}

void DyArray_set(DyArray* restrict array, const void* restrict value, size_t index)
{
	size_t size = array->size;

	void* element = (char*) array->array + index * size;

	memcpy(element, value, size);
}


void* DyArray_append(DyArray* restrict array, const void* restrict value)
{
	size_t size     = array->size;
	size_t count    = array->count;
	size_t capacity = array->capacity;
	void*  raw      = array->array;

	if (count == capacity) {
		capacity = (size_t) ((float) (capacity + 1) * ALLOC_RATE);

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
	count++;

	memcpy(element, value, size);

	array->count = count;

	return element;
}

void* DyArray_prepend(DyArray* restrict array, const void* restrict value)
{
	size_t size     = array->size;
	size_t count    = array->count;
	size_t capacity = array->capacity;
	void*  raw      = array->array;

	if (count == capacity) {
		capacity = (size_t) ((float) (capacity + 1) * ALLOC_RATE);

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

	memmove((char*) raw + size, raw, count * size);
	count++;

	void* element = raw;

	memcpy(element, value, size);

	array->count = count;

	return element;
}
