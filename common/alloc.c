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

#include "alloc.h"
#include "debug.h"
#include "util.h"


static void malloc_failure(size_t size)
{
	double t = program_time();
	log_error(stderr, "malloc failed with size %zu (%.3fms)", size, t);
}

static void calloc_failure(size_t count, size_t size)
{
	double t = program_time();
	log_error(stderr, "calloc failed with count %zu, size %zu (%.3fms)", count, size, t);
}

static void realloc_failure(const void* ptr, size_t size)
{
	double t = program_time();
	log_error(stderr, "calloc failed with ptr 0x%016" PRIxPTR ", size %zu (%.3fms)", (uintptr_t) ptr, size, t);
}

enum CzResult czAlloc(void* restrict* restrict memory, size_t size, struct CzAllocFlags flags)
{
	if NOEXPECT (!size) { return CZ_RESULT_BAD_SIZE; }

	void* p;
	if (flags.zeroInitialise) {
		p = malloc(size);
		if NOEXPECT (!p) {
			malloc_failure(size);
			return CZ_RESULT_NO_MEMORY;
		}
	}
	else {
		p = calloc(size, sizeof(char));
		if NOEXPECT (!p) {
			calloc_failure(size, sizeof(char));
			return CZ_RESULT_NO_MEMORY;
		}
	}

	*memory = p;
	return CZ_RESULT_SUCCESS;
}

enum CzResult czRealloc(void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	ASSUME(*memory != NULL);

	if NOEXPECT (!oldSize || !newSize) {
		if (flags.freeOnFail) {
			free(*memory);
		}
		return CZ_RESULT_BAD_SIZE;
	}

	void* p = realloc(*memory, newSize);
	if NOEXPECT (!p) {
		realloc_failure(*memory, newSize);
		if (flags.freeOnFail) {
			free(*memory);
		}
		return CZ_RESULT_NO_MEMORY;
	}

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedBytes = (char*) p + oldSize;
		size_t addedSize = newSize - oldSize;

		memset(addedBytes, 0, addedSize);
	}

	*memory = p;
	return CZ_RESULT_SUCCESS;
}

enum CzResult czFree(void* restrict memory)
{
	if NOEXPECT (!memory) { return CZ_RESULT_BAD_POINTER; }

	free(memory);
	return CZ_RESULT_SUCCESS;
}
