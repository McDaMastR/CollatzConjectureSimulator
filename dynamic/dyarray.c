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


struct DyArray_
{
	size_t size; // Number of bytes per element
	size_t count; // Number of elements currently in array
	size_t capacity; // Number of elements that could fit in allocated memory
	void* restrict raw; // Raw array
};


static void* dyarray_stretch(struct DyArray_* restrict array)
{
	ASSUME(array->size != 0);

	size_t size = array->size;
	size_t capacity = array->capacity;
	void* raw = array->raw;

	size_t newCapacity = capacity + capacity / 2 + 1;

	if (capacity > newCapacity) {
		newCapacity = capacity + (SIZE_MAX / size - capacity) / 2 + 1;
	}

	size_t allocSize = newCapacity * size;
	void* newRaw = realloc(raw, allocSize);
	if NOEXPECT (!newRaw) { REALLOC_FAILURE(newRaw, raw, allocSize); return NULL; }

	array->capacity = newCapacity;
	array->raw = newRaw;

	return newRaw;
}

void dyarray_destroy(struct DyArray_* restrict array)
{
	if NOEXPECT (!array) { return; }

	free(array->raw);
	free(array);
}

struct DyArray_* dyarray_create(size_t size, size_t count)
{
	ASSUME(size != 0);

	size_t allocSize = sizeof(struct DyArray_);
	struct DyArray_* array = malloc(allocSize);
	if NOEXPECT (!array) { MALLOC_FAILURE(array, allocSize); return NULL; }

	array->size = size;
	array->count = 0;
	array->capacity = count;
	array->raw = NULL;

	if (count) {
		allocSize = count * size;
		void* raw = malloc(allocSize);
		if NOEXPECT (!raw) { MALLOC_FAILURE(raw, allocSize); free(array); return NULL; }
		array->raw = raw;
	}

	return array;
}

size_t dyarray_size(struct DyArray_* restrict array)
{
	return array->count;
}

void* dyarray_raw(struct DyArray_* restrict array)
{
	return array->raw;
}

void dyarray_get(struct DyArray_* restrict array, void* restrict value, size_t index)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t size = array->size;
	void* raw = array->raw;

	void* element = (char*) raw + index * size;
	memcpy(value, element, size);
}

void dyarray_set(struct DyArray_* restrict array, const void* restrict value, size_t index)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t size = array->size;
	void* raw = array->raw;

	void* element = (char*) raw + index * size;
	memcpy(element, value, size);
}

void dyarray_last(struct DyArray_* restrict array, void* restrict value)
{
	ASSUME(array->size != 0);
	ASSUME(array->count != 0);
	ASSUME(array->raw != NULL);

	size_t size = array->size;
	size_t count = array->count;
	void* raw = array->raw;

	void* element = (char*) raw + (count - 1) * size;
	memcpy(value, element, size);
}

void dyarray_first(struct DyArray_* restrict array, void* restrict value)
{
	ASSUME(array->size != 0);
	ASSUME(array->raw != NULL);

	size_t size = array->size;
	void* raw = array->raw;

	void* element = raw;
	memcpy(value, element, size);
}

void* dyarray_append(struct DyArray_* restrict array, const void* restrict value)
{
	ASSUME(array->size != 0);

	size_t size = array->size;
	size_t count = array->count;
	size_t capacity = array->capacity;
	void* raw = array->raw;

	if (count == capacity) {
		void* newRaw = dyarray_stretch(array);
		if NOEXPECT (!newRaw) { return NULL; }
		raw = newRaw;
	}

	void* element = (char*) raw + count * size;
	memcpy(element, value, size);

	array->count = count + 1;
	return element;
}

void* dyarray_prepend(struct DyArray_* restrict array, const void* restrict value)
{
	ASSUME(array->size != 0);

	size_t size = array->size;
	size_t count = array->count;
	size_t capacity = array->capacity;
	void* raw = array->raw;

	if (count == capacity) {
		void* newRaw = dyarray_stretch(array);
		if NOEXPECT (!newRaw) { return NULL; }
		raw = newRaw;
	}

	void* element = raw;
	void* nextElement = (char*) element + size;
	size_t moveSize = count * size;

	memmove(nextElement, element, moveSize);
	memcpy(element, value, size);

	array->count = count + 1;
	return element;
}

void* dyarray_add(struct DyArray_* restrict array, const void* restrict value, size_t index)
{
	ASSUME(array->size != 0);

	size_t size = array->size;
	size_t count = array->count;
	size_t capacity = array->capacity;
	void* raw = array->raw;

	if (count == capacity) {
		void* newRaw = dyarray_stretch(array);
		if NOEXPECT (!newRaw) { return NULL; }
		raw = newRaw;
	}

	void* element = (char*) raw + index * size;
	void* nextElement = (char*) element + size;
	size_t moveSize = (count - index) * size;

	memmove(nextElement, element, moveSize);
	memcpy(element, value, size);

	array->count = count + 1;
	return element;
}
