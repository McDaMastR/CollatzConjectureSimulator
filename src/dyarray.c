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
	size_t size;     // No. bytes per element
	size_t count;    // No. elements currently in array
	size_t capacity; // No. elements that could fit in allocated memory
	void*  raw;      // Raw array
} DyArray_T;


static void* dyarray_stretch(DyArray array)
{
	ASSUME(array->size != 0);

	size_t size = array->size;
	size_t cap  = array->capacity;
	void*  raw  = array->raw;

	size_t cap2 = cap + cap / 2 + 1;
	if (cap > cap2) { cap2 = cap + (SIZE_MAX / size - cap) / 2 + 1; }

	void* raw2 = realloc(raw, cap2 * size);
	if EXPECT_FALSE (!raw2) { REALLOC_FAILURE(raw2, raw, cap2 * size); return NULL; }

	array->capacity = cap2;
	array->raw      = raw2;

	return raw2;
}


void dyarray_destroy(DyArray array)
{
	if EXPECT_FALSE (!array) return;

	free(array->raw);
	free(array);
}

DyArray dyarray_create(size_t size, size_t count)
{
	ASSUME(size != 0);

	DyArray array = (DyArray) malloc(sizeof(DyArray_T));
	if EXPECT_FALSE (!array) { MALLOC_FAILURE(array, sizeof(DyArray_T)); return NULL; }

	array->size     = size;
	array->count    = 0;
	array->capacity = count;
	array->raw      = NULL;

	if (count) {
		void* raw = malloc(count * size);
		if EXPECT_FALSE (!raw) { MALLOC_FAILURE(raw, count * size); free(array); return NULL; }

		array->raw = raw;
	}

	return array;
}

size_t dyarray_size(DyArray array)
{
	return array->count;
}

void* dyarray_raw(DyArray array)
{
	return array->raw;
}

void dyarray_get(DyArray array, void* restrict value, size_t index)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t      size = array->size;
	const void* raw  = array->raw;

	const void* element = (const char*) raw + index * size;

	memcpy(value, element, size);
}

void dyarray_set(DyArray array, const void* restrict value, size_t index)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t size = array->size;
	void*  raw  = array->raw;

	void* element = (char*) raw + index * size;

	memcpy(element, value, size);
}

void dyarray_last(DyArray array, void* restrict value)
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

void dyarray_first(DyArray array, void* restrict value)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t      size = array->size;
	const void* raw  = array->raw;

	const void* element = raw;

	memcpy(value, element, size);
}

void* dyarray_append(DyArray array, const void* restrict value)
{
	ASSUME(array->size != 0);

	size_t size  = array->size;
	size_t count = array->count;
	size_t cap   = array->capacity;
	void*  raw   = array->raw;

	if (count == cap) {
		void* raw2 = dyarray_stretch(array);
		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = (char*) raw + count * size;

	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}

void* dyarray_prepend(DyArray array, const void* restrict value)
{
	ASSUME(array->size != 0);

	size_t size  = array->size;
	size_t count = array->count;
	size_t cap   = array->capacity;
	void*  raw   = array->raw;

	if (count == cap) {
		void* raw2 = dyarray_stretch(array);
		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = raw;

	memmove((char*) element + size, element, count * size);
	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}

void* dyarray_insert(DyArray array, const void* restrict value, size_t index)
{
	ASSUME(array->size != 0);

	size_t size  = array->size;
	size_t count = array->count;
	size_t cap   = array->capacity;
	void*  raw   = array->raw;

	if (count == cap) {
		void* raw2 = dyarray_stretch(array);
		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = (char*) raw + index * size;

	memmove((char*) element + size, element, (count - index) * size);
	memcpy(element, value, size);

	array->count = count + 1;

	return element;
}
