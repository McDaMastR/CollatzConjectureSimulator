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


static void* DyArray_stretch(restrict DyArray arr)
{
	ASSUME(arr->size != 0);

	size_t size = arr->size;
	size_t cap  = arr->capacity;
	void*  raw  = arr->raw;

	size_t cap2 = cap + cap / 2 + 1;

	if EXPECT_FALSE (cap > cap2) { cap2 = cap + (SIZE_MAX / size - cap) / 2 + 1; }

	void* raw2 = realloc(raw, cap2 * size);

	if EXPECT_FALSE (!raw2) { REALLOC_FAILURE(raw2, raw, cap2 * size); return NULL; }

	arr->capacity = cap2;
	arr->raw      = raw2;

	return raw2;
}


void DyArray_destroy(restrict DyArray arr)
{
	if EXPECT_TRUE (arr) {
		free(arr->raw);
		free(arr);
	}
}

DyArray DyArray_create(size_t size, size_t count)
{
	ASSUME(size != 0);

	DyArray arr = (DyArray) malloc(sizeof(DyArray_T));

	if EXPECT_FALSE (!arr) { MALLOC_FAILURE(arr, sizeof(DyArray_T)); return NULL; }

	arr->size     = size;
	arr->count    = 0;
	arr->capacity = count;
	arr->raw      = NULL;

	if EXPECT_TRUE (count) {
		void* raw = malloc(count * size);

		if EXPECT_FALSE (!raw) { MALLOC_FAILURE(raw, count * size); free(arr); return NULL; }

		arr->raw = raw;
	}

	return arr;
}

size_t DyArray_size(restrict DyArray arr)
{
	return EXPECT_TRUE (arr) ? arr->count : 0;
}

void* DyArray_raw(restrict DyArray arr)
{
	return EXPECT_TRUE (arr) ? arr->raw : NULL;
}

void DyArray_get(restrict DyArray arr, void* restrict val, size_t idx)
{
	ASSUME(arr->size != 0);
	ASSUME(arr->raw != NULL);

	size_t      size = arr->size;
	const void* raw  = arr->raw;

	const void* element = (const char*) raw + idx * size;

	memcpy(val, element, size);
}

void DyArray_set(restrict DyArray arr, const void* restrict val, size_t idx)
{
	ASSUME(arr->size != 0);
	ASSUME(arr->raw != NULL);

	size_t size = arr->size;
	void*  raw  = arr->raw;

	void* element = (char*) raw + idx * size;

	memcpy(element, val, size);
}

void DyArray_last(restrict DyArray arr, void* restrict val)
{
	ASSUME(arr->size != 0);
	ASSUME(arr->count != 0);
	ASSUME(arr->raw != NULL);

	size_t      size  = arr->size;
	size_t      count = arr->count;
	const void* raw   = arr->raw;

	const void* element = (const char*) raw + (count - 1) * size;

	memcpy(val, element, size);
}

void DyArray_first(restrict DyArray arr, void* restrict val)
{
	ASSUME(arr->size != 0);
	ASSUME(arr->raw != NULL);

	size_t      size = arr->size;
	const void* raw  = arr->raw;

	const void* element = raw;

	memcpy(val, element, size);
}

void* DyArray_append(restrict DyArray arr, const void* restrict val)
{
	ASSUME(arr->size != 0);

	size_t size  = arr->size;
	size_t count = arr->count;
	size_t cap   = arr->capacity;
	void*  raw   = arr->raw;

	if EXPECT_FALSE (count == cap) {
		void* raw2 = DyArray_stretch(arr);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = (char*) raw + count * size;

	memcpy(element, val, size);

	arr->count = count + 1;

	return element;
}

void* DyArray_prepend(restrict DyArray arr, const void* restrict val)
{
	ASSUME(arr->size != 0);

	size_t size  = arr->size;
	size_t count = arr->count;
	size_t cap   = arr->capacity;
	void*  raw   = arr->raw;

	if EXPECT_FALSE (count == cap) {
		void* raw2 = DyArray_stretch(arr);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = raw;

	memmove((char*) element + size, element, count * size);
	memcpy(element, val, size);

	arr->count = count + 1;

	return element;
}

void* DyArray_insert(restrict DyArray arr, const void* restrict val, size_t idx)
{
	ASSUME(arr->size != 0);

	size_t size  = arr->size;
	size_t count = arr->count;
	size_t cap   = arr->capacity;
	void*  raw   = arr->raw;

	if EXPECT_FALSE (count == cap) {
		void* raw2 = DyArray_stretch(arr);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	void* element = (char*) raw + idx * size;

	memmove((char*) element + size, element, (count - idx) * size);
	memcpy(element, val, size);

	arr->count = count + 1;

	return element;
}
