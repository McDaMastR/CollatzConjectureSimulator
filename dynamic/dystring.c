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

#include "dystring.h"


struct DyString_
{
	size_t length; // Number of characters currently in string, including null terminator
	size_t capacity; // Number of characters that could fit in allocated memory
	char* restrict raw; // Raw string
};


static char* dystring_stretch(struct DyString_* restrict string, size_t length)
{
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);
	ASSUME(length != 0);

	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t newCapacity = length + capacity / 2;

	if (capacity > newCapacity) {
		newCapacity = length;
	}

	char* newRaw = realloc(raw, newCapacity);
	if NOEXPECT (!newRaw) { REALLOC_FAILURE(newRaw, raw, newCapacity); return NULL; }

	void* addedMemory = newRaw + capacity;
	size_t addedMemorySize = newCapacity - capacity;

	// realloc doesn't initialise added memory, so we gotta do it ourselves
	memset(addedMemory, 0, addedMemorySize);

	string->capacity = newCapacity;
	string->raw = newRaw;

	return newRaw;
}

void dystring_destroy(struct DyString_* restrict string)
{
	if NOEXPECT (!string) { return; }

	free(string->raw);
	free(string);
}

struct DyString_* dystring_create(size_t count)
{
	size_t allocSize = sizeof(struct DyString_);
	struct DyString_* string = malloc(allocSize);
	if NOEXPECT (!string) { MALLOC_FAILURE(string, allocSize); return NULL; }

	allocSize = sizeof(char);
	char* raw = calloc(count, allocSize);
	if NOEXPECT (!raw) { CALLOC_FAILURE(raw, count, allocSize); free(string); return NULL; }

	string->length = 1;
	string->capacity = count;
	string->raw = raw;

	return string;
}

size_t dystring_length(struct DyString_* restrict string)
{
	return string->length;
}

char* dystring_raw(struct DyString_* restrict string)
{
	return string->raw;
}

char* dystring_append(struct DyString_* restrict string, const char* restrict substring)
{
	ASSUME(string->length != 0);
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);

	size_t length = string->length;
	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t subLength = strlen(substring);
	size_t newLength = length + subLength;

	if (newLength > capacity) {
		char* newRaw = dystring_stretch(string, newLength);
		if NOEXPECT (!newRaw) { return NULL; }
		raw = newRaw;
	}

	char* subRaw = raw + length - 1;
	memcpy(subRaw, substring, subLength);

	string->length = newLength;
	return subRaw;
}

char* dystring_prepend(struct DyString_* restrict string, const char* restrict substring)
{
	ASSUME(string->length != 0);
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);

	size_t length = string->length;
	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t subLength = strlen(substring);
	size_t newLength = length + subLength;

	if (newLength > capacity) {
		char* newRaw = dystring_stretch(string, newLength);
		if NOEXPECT (!newRaw) { return NULL; }
		raw = newRaw;
	}

	char* subRaw = raw;
	char* offsetRaw = subRaw + subLength;
	size_t moveSize = length - 1;

	memmove(offsetRaw, subRaw, moveSize);
	memcpy(subRaw, substring, subLength);

	string->length = newLength;
	return subRaw;
}

char* dystring_add(struct DyString_* restrict string, const char* restrict substring, size_t index)
{
	ASSUME(string->length != 0);
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);

	size_t length = string->length;
	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t subLength = strlen(substring);
	size_t newLength = length + subLength;

	if (newLength > capacity) {
		char* newRaw = dystring_stretch(string, newLength);
		if NOEXPECT (!newRaw) { return NULL; }
		raw = newRaw;
	}

	char* subRaw = raw + index;
	char* offsetRaw = subRaw + subLength;
	size_t moveSize = length - index - 1;

	memmove(offsetRaw, subRaw, moveSize);
	memcpy(subRaw, substring, subLength);

	string->length = newLength;
	return subRaw;
}
