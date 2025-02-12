/* 
 * Copyright (C) 2024-2025 Seth McDonald
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


typedef struct DyArray_T
{
	size_t size;     // # bytes per element
	size_t count;    // # elements currently in array
	size_t capacity; // # elements that could fit in allocated memory
	void*  raw;      // Raw array
} DyArray_T;


static void* DyArray_stretch(restrict DyArray array)
{
	ASSUME(array->size != 0);

	size_t size = array->size;
	size_t cap  = array->capacity;
	void*  raw  = array->raw;

	cap += (cap + 1) / 2 + 1; // TODO check for overflow

	void* raw2 = realloc(raw, cap * size);

	if EXPECT_FALSE (!raw2) { REALLOC_FAILURE(raw2, raw, cap * size); return NULL; }

	array->capacity = cap;
	array->raw      = raw2;

	return raw2;
}


void DyArray_destroy(restrict DyArray array)
{
	free(array->raw);
	free(array);
}

DyArray DyArray_create(size_t size, size_t count)
{
	ASSUME(size != 0);

	DyArray array = (DyArray) malloc(sizeof(DyArray_T));

	if EXPECT_FALSE (!array) { MALLOC_FAILURE(array, sizeof(DyArray_T)); return NULL; }

	array->size     = size;
	array->count    = 0;
	array->capacity = count;
	array->raw      = NULL;

	if EXPECT_TRUE (count) {
		void* raw = malloc(count * size);

		if EXPECT_FALSE (!raw) { MALLOC_FAILURE(raw, count * size); free(array); return NULL; }

		array->raw = raw;
	}

	return array;
}

size_t DyArray_size(restrict DyArray array)
{
	return array->count;
}

void* DyArray_raw(restrict DyArray array)
{
	return array->raw;
}

void DyArray_get(restrict DyArray array, void* restrict value, size_t index)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t      size = array->size;
	const void* raw  = array->raw;

	const void* element = (const char*) raw + index * size;

	memcpy(value, element, size);
}

void DyArray_set(restrict DyArray array, const void* restrict value, size_t index)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t size = array->size;
	void*  raw  = array->raw;

	void* element = (char*) raw + index * size;

	memcpy(element, value, size);
}

void DyArray_last(restrict DyArray array, void* restrict value)
{
	ASSUME(array->size != 0);
	ASSUME(array->count != 0);
	ASSUME(array->raw != NULL);

	size_t      size  = array->size;
	size_t      count = array->count;
	const void* raw   = array->raw;

	const void* element = (const char*) raw + (count - 1) * size;

	memcpy(value, element, size);
}

void DyArray_first(restrict DyArray array, void* restrict value)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t      size = array->size;
	const void* raw  = array->raw;

	const void* element = raw;

	memcpy(value, element, size);
}

void* DyArray_append(restrict DyArray array, const void* restrict value)
{
	ASSUME(array->size != 0);

	size_t size  = array->size;
	size_t count = array->count;
	size_t cap   = array->capacity;
	void*  raw   = array->raw;

	if EXPECT_FALSE (count == cap) {
		void* raw2 = DyArray_stretch(array);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = (char*) raw + count * size;

	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}

void* DyArray_prepend(restrict DyArray array, const void* restrict value)
{
	ASSUME(array->size != 0);

	size_t size  = array->size;
	size_t count = array->count;
	size_t cap   = array->capacity;
	void*  raw   = array->raw;

	if EXPECT_FALSE (count == cap) {
		void* raw2 = DyArray_stretch(array);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = raw;

	memmove((char*) element + size, element, count * size);
	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}
