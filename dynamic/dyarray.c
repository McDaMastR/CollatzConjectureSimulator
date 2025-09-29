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

	// TODO Deal with overflow *properly*
	size_t newCapacity = capacity + capacity / 2 + 1;
	if (capacity > newCapacity) {
		newCapacity = capacity + (SIZE_MAX / size - capacity) / 2 + 1;
	}

	struct CzAllocFlags flags = {0};
	enum CzResult czres = czRealloc(&array->raw, capacity * size, newCapacity * size, flags);
	if NOEXPECT (czres) { return NULL; }

	array->capacity = newCapacity;
	return array->raw;
}

void dyarray_destroy(struct DyArray_* restrict array)
{
	if NOEXPECT (!array) { return; }

	czFree(array->raw);
	czFree(array);
}

struct DyArray_* dyarray_create(size_t size, size_t count)
{
	ASSUME(size != 0);

	struct DyArray_* restrict array;
	struct CzAllocFlags flags = {0};

	enum CzResult czres = czAlloc((void* restrict*) &array, sizeof(*array), flags);
	if NOEXPECT (czres) { return NULL; }

	array->size = size;
	array->count = 0;
	array->capacity = count;
	array->raw = NULL;

	if (count) {
		czres = czAlloc(&array->raw, count * size, flags);
		if NOEXPECT (czres) { goto err_free_array; }
	}

	return array;

err_free_array:
	czFree(array);
	return NULL;
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
		raw = dyarray_stretch(array);
		if NOEXPECT (!raw) { return NULL; }
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
		raw = dyarray_stretch(array);
		if NOEXPECT (!raw) { return NULL; }
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
		raw = dyarray_stretch(array);
		if NOEXPECT (!raw) { return NULL; }
	}

	void* element = (char*) raw + index * size;
	void* nextElement = (char*) element + size;
	size_t moveSize = (count - index) * size;

	memmove(nextElement, element, moveSize);
	memcpy(element, value, size);

	array->count = count + 1;
	return element;
}
