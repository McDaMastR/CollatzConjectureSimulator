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

CZ_COLD
static void malloc_failure(size_t size)
{
	double t = program_time();
	log_error(stderr, "malloc failed with size %zu (%.3fms)", size, t);
}

CZ_COLD
static void calloc_failure(size_t count, size_t size)
{
	double t = program_time();
	log_error(stderr, "calloc failed with count %zu, size %zu (%.3fms)", count, size, t);
}

CZ_COLD
static void realloc_failure(void* ptr, size_t size)
{
	double t = program_time();
	log_error(stderr, "realloc failed with ptr 0x%016" PRIxPTR ", size %zu (%.3fms)", (uintptr_t) ptr, size, t);
}

#if defined(_WIN32)
CZ_COLD
static void recalloc_failure(void* ptr, size_t count, size_t size)
{
	double t = program_time();
	log_error(
		stderr, "_recalloc failed with ptr 0x%016" PRIxPTR ", count %zu, size %zu (%.3fms)",
		(uintptr_t) ptr, count, size, t);
}

CZ_COLD
static void aligned_offset_malloc_failure(size_t size, size_t alignment, size_t offset)
{
	double t = program_time();
	log_error(
		stderr, "_aligned_offset_malloc failed with size %zu, alignment %zu, offset %zu (%.3fms)",
		size, alignment, offset, t);
}

CZ_COLD
static void aligned_offset_realloc_failure(void* ptr, size_t size, size_t alignment, size_t offset)
{
	double t = program_time();
	log_error(
		stderr,
		"_aligned_offset_realloc failed with ptr 0x%016" PRIxPTR ", size %zu, alignment %zu, offset %zu (%.3fms)",
		(uintptr_t) ptr, size, alignment, offset, t);
}

CZ_COLD
static void aligned_offset_recalloc_failure(void* ptr, size_t count, size_t size, size_t alignment, size_t offset)
{
	double t = program_time();
	log_error(
		stderr,
		"_aligned_offset_recalloc failed with "
		"ptr 0x%016" PRIxPTR ", count %zu, size %zu, alignment %zu, offset %zu (%.3fms)",
		(uintptr_t) ptr, count, size, alignment, offset, t);
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_COLD
static void posix_memalign_failure(void** ptr, size_t alignment, size_t size)
{
	double t = program_time();
	log_error(
		stderr, "posix_memalign failed with ptr 0x%016" PRIxPTR ", alignment %zu, size %zu (%.3fms)",
		(uintptr_t) ptr, alignment, size, t);
}
#endif

enum CzResult czAlloc(void* restrict* restrict memory, size_t size, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!size) { return CZ_RESULT_BAD_SIZE; }

	void* p;
	if (flags.zeroInitialise) {
		p = calloc(size, sizeof(char));
		if CZ_NOEXPECT (!p) {
			calloc_failure(size, sizeof(char));
			return CZ_RESULT_NO_MEMORY;
		}
	}
	else {
		p = malloc(size);
		if CZ_NOEXPECT (!p) {
			malloc_failure(size);
			return CZ_RESULT_NO_MEMORY;
		}
	}

	*memory = p;
	return CZ_RESULT_SUCCESS;
}

enum CzResult czRealloc(void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	CZ_ASSUME(*memory != NULL);
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;

	if CZ_NOEXPECT (!oldSize) {
		ret = CZ_RESULT_BAD_SIZE;
		goto err_free_memory;
	}
	if CZ_NOEXPECT (!newSize) {
		czFree(*memory);
		return CZ_RESULT_SUCCESS;
	}

	void* p;
#if defined(_WIN32)
	if (flags.zeroInitialise) {
		p = _recalloc(*memory, newSize, sizeof(char));
		if CZ_NOEXPECT (!p) {
			recalloc_failure(*memory, newSize, sizeof(char));
			ret = CZ_RESULT_NO_MEMORY;
			goto err_free_memory;
		}
	}
	else {
		p = realloc(*memory, newSize);
		if CZ_NOEXPECT (!p) {
			realloc_failure(*memory, newSize);
			ret = CZ_RESULT_NO_MEMORY;
			goto err_free_memory;
		}
	}
#else
	p = realloc(*memory, newSize);
	if CZ_NOEXPECT (!p) {
		realloc_failure(*memory, newSize);
		ret = CZ_RESULT_NO_MEMORY;
		goto err_free_memory;
	}

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedBytes = (char*) p + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedBytes, 0, addedSize);
	}
#endif

	*memory = p;
	return CZ_RESULT_SUCCESS;

err_free_memory:
	if (flags.freeOnFail) {
		czFree(*memory);
	}
	return ret;
}

enum CzResult czFree(void* restrict memory)
{
	if CZ_NOEXPECT (!memory) { return CZ_RESULT_BAD_POINTER; }
	free(memory);
	return CZ_RESULT_SUCCESS;
}

enum CzResult czAllocAlign(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!size)                                     { return CZ_RESULT_BAD_SIZE; }
	if CZ_NOEXPECT (!alignment || alignment & (alignment - 1)) { return CZ_RESULT_BAD_ALIGNMENT; }
	if CZ_NOEXPECT (offset >= size)                            { return CZ_RESULT_BAD_OFFSET; }

	void* p;
	offset &= alignment - 1; // Ensure offset < alignment

#if defined(_WIN32)
	if (flags.zeroInitialise) {
		// No _aligned_offset_calloc function ???
		p = _aligned_offset_recalloc(NULL, size, sizeof(char), alignment, offset);
		if CZ_NOEXPECT (!p) {
			aligned_offset_recalloc_failure(NULL, size, sizeof(char), alignment, offset);
			return CZ_RESULT_NO_MEMORY;
		}
	}
	else {
		p = _aligned_offset_malloc(size, alignment, offset);
		if CZ_NOEXPECT (!p) {
			aligned_offset_malloc_failure(size, alignment, offset);
			return CZ_RESULT_NO_MEMORY;
		}
	}
#elif defined(__APPLE__) || defined(__unix__)
	void* raw;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t extraSize = offset && alignment <= sizeof(void*) ? sizeof(void*) : 0;
	size_t allocSize = size + allocAlignment + extraSize;

	int res = posix_memalign(&raw, allocAlignment, allocSize);
	if CZ_NOEXPECT (res) {
		posix_memalign_failure(&raw, allocAlignment, allocSize);
		return CZ_RESULT_NO_MEMORY;
	}

	p = (char*) raw + allocAlignment - offset + extraSize;
	*((void**) ((uintptr_t) p & ~(sizeof(void*) - 1)) - 1) = raw;
	if (flags.zeroInitialise) {
		memset(p, 0, size);
	}
#else
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t allocSize = size + allocAlignment * 2;
	void* raw;

	if (flags.zeroInitialise) {
		raw = calloc(allocSize, sizeof(char));
		if CZ_NOEXPECT (!raw) {
			calloc_failure(allocSize, sizeof(char));
			return CZ_RESULT_NO_MEMORY;
		}
	}
	else {
		raw = malloc(allocSize);
		if CZ_NOEXPECT (!raw) {
			malloc_failure(allocSize);
			return CZ_RESULT_NO_MEMORY;
		}
	}

	p = (char*) ((uintptr_t) raw & ~(allocAlignment - 1)) + allocAlignment * 2 - offset;
	*((void**) ((uintptr_t) p & ~(sizeof(void*) - 1)) - 1) = raw;
#endif

	*memory = p;
	return CZ_RESULT_SUCCESS;
}

enum CzResult czReallocAlign(
	void* restrict* restrict memory,
	size_t oldSize,
	size_t newSize,
	size_t alignment,
	size_t offset,
	struct CzAllocFlags flags)
{
	CZ_ASSUME(*memory != NULL);
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;

	if CZ_NOEXPECT (!oldSize) {
		ret = CZ_RESULT_BAD_SIZE;
		goto err_free_memory;
	}
	if CZ_NOEXPECT (!newSize) {
		czFreeAlign(*memory);
		return CZ_RESULT_SUCCESS;
	}
	if CZ_NOEXPECT (!alignment || alignment & (alignment - 1)) {
		ret = CZ_RESULT_BAD_ALIGNMENT;
		goto err_free_memory;
	}
	if CZ_NOEXPECT (offset >= newSize) {
		ret = CZ_RESULT_BAD_OFFSET;
		goto err_free_memory;
	}

	void* p;
	offset &= alignment - 1; // Ensure offset < alignment

#if defined(_WIN32)
	if (flags.zeroInitialise) {
		p = _aligned_offset_recalloc(*memory, newSize, sizeof(char), alignment, offset);
		if CZ_NOEXPECT (!p) {
			aligned_offset_recalloc_failure(*memory, newSize, sizeof(char), alignment, offset);
			ret = CZ_RESULT_NO_MEMORY;
			goto err_free_memory;
		}
	}
	else {
		p = _aligned_offset_realloc(*memory, newSize, alignment, offset);
		if CZ_NOEXPECT (!p) {
			aligned_offset_realloc_failure(*memory, newSize, alignment, offset);
			ret = CZ_RESULT_NO_MEMORY;
			goto err_free_memory;
		}
	}
#else
	struct CzAllocFlags allocFlags = {0};
	enum CzResult res = czAllocAlign(&p, newSize, alignment, offset, allocFlags);
	if CZ_NOEXPECT (res) {
		ret = res;
		goto err_free_memory;
	}

	memcpy(p, *memory, minz(oldSize, newSize));
	czFreeAlign(*memory);

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedBytes = (char*) p + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedBytes, 0, addedSize);
	}
#endif

	*memory = p;
	return CZ_RESULT_SUCCESS;

err_free_memory:
	if (flags.freeOnFail) {
		czFreeAlign(*memory);
	}
	return ret;
}

enum CzResult czFreeAlign(void* restrict memory)
{
	if CZ_NOEXPECT (!memory) { return CZ_RESULT_BAD_POINTER; }
#if defined(_WIN32)
	_aligned_free(memory);
#else
	free(*((void**) ((uintptr_t) memory & ~(sizeof(void*) - 1)) - 1));
#endif
	return CZ_RESULT_SUCCESS;
}
