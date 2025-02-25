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
	size_t length;   // # characters currently in string
	size_t capacity; // # characters that could fit in allocated memory
	char*  raw;      // Raw string
} DyString_T;


static char* dystring_stretch(restrict DyString str, size_t size)
{
	ASSUME(str->capacity != 0);
	ASSUME(str->raw != NULL);
	ASSUME(size != 0);

	size_t cap = str->capacity;
	char*  raw = str->raw;

	size_t cap2 = size + cap / 2;

	if EXPECT_FALSE (cap > cap2) { cap2 = size; }

	char* raw2 = (char*) realloc(raw, cap2);

	if EXPECT_FALSE (!raw2) { REALLOC_FAILURE(raw2, raw, cap2); return NULL; }

	memset(raw2 + cap, '\0', cap2 - cap);

	str->capacity = cap2;
	str->raw      = raw2;

	return raw2;
}


void dystring_destroy(restrict DyString str)
{
	if EXPECT_TRUE (str) {
		free(str->raw);
		free(str);
	}
}

DyString dystring_create(size_t count)
{
	DyString str = (DyString) malloc(sizeof(DyString_T));

	if EXPECT_FALSE (!str) { MALLOC_FAILURE(str, sizeof(DyString_T)); return NULL; }

	char* raw = (char*) calloc(count ?: 1, sizeof(char));

	if EXPECT_FALSE (!raw) { CALLOC_FAILURE(raw, count ?: 1, sizeof(char)); free(str); return NULL; }

	str->length   = 1;
	str->capacity = count ?: 1;
	str->raw      = raw;

	return str;
}

size_t dystring_length(restrict DyString str)
{
	return EXPECT_TRUE (str) ? str->length : 0;
}

char* dystring_raw(restrict DyString str)
{
	return EXPECT_TRUE (str) ? str->raw : NULL;
}

char* dystring_append(restrict DyString str, const char* restrict sub)
{
	ASSUME(str->length != 0);
	ASSUME(str->capacity != 0);
	ASSUME(str->raw != NULL);

	size_t len = str->length;
	size_t cap = str->capacity;
	char*  raw = str->raw;

	size_t sublen = strlen(sub);

	if EXPECT_FALSE (len + sublen > cap) {
		char* raw2 = dystring_stretch(str, len + sublen);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	char* subraw = raw + len - 1;

	memcpy(subraw, sub, sublen);

	str->length = len + sublen;

	return subraw;
}

char* dystring_prepend(restrict DyString str, const char* restrict sub)
{
	ASSUME(str->length != 0);
	ASSUME(str->capacity != 0);
	ASSUME(str->raw != NULL);

	size_t len = str->length;
	size_t cap = str->capacity;
	char*  raw = str->raw;

	size_t sublen = strlen(sub);

	if EXPECT_FALSE (len + sublen > cap) {
		char* raw2 = dystring_stretch(str, len + sublen);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	char* subraw = raw;

	memmove(subraw + sublen, subraw, len - 1);
	memcpy(subraw, sub, sublen);

	str->length = len + sublen;

	return subraw;
}

char* dystring_insert(restrict DyString str, const char* restrict sub, size_t idx)
{
	ASSUME(str->length != 0);
	ASSUME(str->capacity != 0);
	ASSUME(str->raw != NULL);

	size_t len = str->length;
	size_t cap = str->capacity;
	char*  raw = str->raw;

	size_t sublen = strlen(sub);

	if EXPECT_FALSE (len + sublen > cap) {
		char* raw2 = dystring_stretch(str, len + sublen);

		if EXPECT_FALSE (!raw2) return NULL;

		raw = raw2;
	}

	char* subraw = raw + idx;

	memmove(subraw + sublen, subraw, len - idx - 1);
	memcpy(subraw, sub, sublen);

	str->length = len + sublen;

	return subraw;
}
