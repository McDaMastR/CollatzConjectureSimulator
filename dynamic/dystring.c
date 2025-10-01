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
	CZ_ASSUME(string->capacity != 0);
	CZ_ASSUME(string->raw != NULL);
	CZ_ASSUME(length != 0);

	size_t capacity = string->capacity;
	size_t newCapacity = length + capacity / 2;
	if (capacity > newCapacity) {
		newCapacity = length;
	}

	struct CzAllocFlags flags = {0};
	flags.zeroInitialise = true;

	enum CzResult czres = czRealloc(
		(void* restrict*) &string->raw, sizeof(char) * capacity, sizeof(char) * newCapacity, flags);

	if CZ_NOEXPECT (czres) { return NULL; }

	string->capacity = newCapacity;
	return string->raw;
}

void dystring_destroy(struct DyString_* restrict string)
{
	if CZ_NOEXPECT (!string) { return; }

	czFree(string->raw);
	czFree(string);
}

struct DyString_* dystring_create(size_t count)
{
	struct DyString_* restrict string;
	struct CzAllocFlags strFlags = {0};

	enum CzResult czres = czAlloc((void* restrict*) &string, sizeof(*string), strFlags);
	if CZ_NOEXPECT (czres) { return NULL; }

	struct CzAllocFlags rawFlags = {0};
	rawFlags.zeroInitialise = true;

	czres = czAlloc((void* restrict*) &string->raw, sizeof(char) * count, rawFlags);
	if CZ_NOEXPECT (czres) { goto err_free_string; }

	string->length = 1; // Start as a single null terminator
	string->capacity = count;
	return string;

err_free_string:
	czFree(string);
	return NULL;
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
	CZ_ASSUME(string->length != 0);
	CZ_ASSUME(string->capacity != 0);
	CZ_ASSUME(string->raw != NULL);

	size_t length = string->length;
	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t subLength = strlen(substring);
	size_t newLength = length + subLength;

	if (newLength > capacity) {
		raw = dystring_stretch(string, newLength);
		if CZ_NOEXPECT (!raw) { return NULL; }
	}

	char* subRaw = raw + length - 1;
	memcpy(subRaw, substring, subLength);

	string->length = newLength;
	return subRaw;
}

char* dystring_prepend(struct DyString_* restrict string, const char* restrict substring)
{
	CZ_ASSUME(string->length != 0);
	CZ_ASSUME(string->capacity != 0);
	CZ_ASSUME(string->raw != NULL);

	size_t length = string->length;
	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t subLength = strlen(substring);
	size_t newLength = length + subLength;

	if (newLength > capacity) {
		raw = dystring_stretch(string, newLength);
		if CZ_NOEXPECT (!raw) { return NULL; }
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
	CZ_ASSUME(string->length != 0);
	CZ_ASSUME(string->capacity != 0);
	CZ_ASSUME(string->raw != NULL);

	size_t length = string->length;
	size_t capacity = string->capacity;
	char* raw = string->raw;

	size_t subLength = strlen(substring);
	size_t newLength = length + subLength;

	if (newLength > capacity) {
		raw = dystring_stretch(string, newLength);
		if CZ_NOEXPECT (!raw) { return NULL; }
	}

	char* subRaw = raw + index;
	char* offsetRaw = subRaw + subLength;
	size_t moveSize = length - index - 1;

	memmove(offsetRaw, subRaw, moveSize);
	memcpy(subRaw, substring, subLength);

	string->length = newLength;
	return subRaw;
}
