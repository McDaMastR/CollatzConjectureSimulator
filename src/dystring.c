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
#include "debug.h"


typedef struct DyString_T
{
	size_t length;   // No. characters currently in string
	size_t capacity; // No. characters that could fit in allocated memory
	char*  raw;      // Raw string
} DyString_T;


static char* dystring_stretch(restrict DyString string, size_t size)
{
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);
	ASSUME(size != 0);

	size_t cap = string->capacity;
	char*  raw = string->raw;

	size_t cap2 = size + cap / 2;

	if EXPECT_FALSE (cap > cap2) { cap2 = size; }

	char* raw2 = (char*) realloc(raw, cap2);

	if EXPECT_FALSE (!raw2) { REALLOC_FAILURE(raw2, raw, cap2); return NULL; }

	memset(raw2 + cap, '\0', cap2 - cap);

	string->capacity = cap2;
	string->raw      = raw2;

	return raw2;
}


void dystring_destroy(restrict DyString string)
{
	if EXPECT_TRUE (string) {
		free(string->raw);
		free(string);
	}
}

DyString dystring_create(size_t count)
{
	DyString string = (DyString) malloc(sizeof(DyString_T));

	if EXPECT_FALSE (!string) { MALLOC_FAILURE(string, sizeof(DyString_T)); return NULL; }

	char* raw = (char*) calloc(count ?: 1, sizeof(char));

	if EXPECT_FALSE (!raw) { CALLOC_FAILURE(raw, count ?: 1, sizeof(char)); free(string); return NULL; }

	string->length   = 1;
	string->capacity = count ?: 1;
	string->raw      = raw;

	return string;
}

size_t dystring_length(restrict DyString string)
{
	return EXPECT_TRUE (string) ? string->length : 0;
}

char* dystring_raw(restrict DyString string)
{
	return EXPECT_TRUE (string) ? string->raw : NULL;
}

char* dystring_append(restrict DyString string, const char* restrict substring)
{
	ASSUME(string->length != 0);
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);

	size_t len = string->length;
	size_t cap = string->capacity;
	char*  raw = string->raw;

	size_t sublen = strlen(substring);

	if EXPECT_FALSE (len + sublen > cap) {
		char* raw2 = dystring_stretch(string, len + sublen);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	char* subraw = raw + len - 1;

	memcpy(subraw, substring, sublen);

	string->length = len + sublen;

	return subraw;
}

char* dystring_prepend(restrict DyString string, const char* restrict substring)
{
	ASSUME(string->length != 0);
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);

	size_t len = string->length;
	size_t cap = string->capacity;
	char*  raw = string->raw;

	size_t sublen = strlen(substring);

	if EXPECT_FALSE (len + sublen > cap) {
		char* raw2 = dystring_stretch(string, len + sublen);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	char* subraw = raw;

	memmove(subraw + sublen, subraw, len - 1);
	memcpy(subraw, substring, sublen);

	string->length = len + sublen;

	return subraw;
}

char* dystring_insert(restrict DyString string, const char* restrict substring, size_t index)
{
	ASSUME(string->length != 0);
	ASSUME(string->capacity != 0);
	ASSUME(string->raw != NULL);

	size_t len = string->length;
	size_t cap = string->capacity;
	char*  raw = string->raw;

	size_t sublen = strlen(substring);

	if EXPECT_FALSE (len + sublen > cap) {
		char* raw2 = dystring_stretch(string, len + sublen);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	char* subraw = raw + index;

	memmove(subraw + sublen, subraw, len - index - 1);
	memcpy(subraw, substring, sublen);

	string->length = len + sublen;

	return subraw;
}
