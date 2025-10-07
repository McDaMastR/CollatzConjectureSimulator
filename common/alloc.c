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

CZ_NONNULL_ARGS
static enum CzResult malloc_wrap(void* restrict* memory, size_t size)
{
	CZ_ASSUME(size != 0);

	void* p = malloc(size);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "malloc failed with size %zu (%.3fms)", size, t);
	return CZ_RESULT_NO_MEMORY;
}

CZ_NONNULL_ARGS
static enum CzResult calloc_wrap(void* restrict* memory, size_t count, size_t size)
{
	CZ_ASSUME(count != 0);
	CZ_ASSUME(size != 0);

	void* p = calloc(count, size);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "calloc failed with count %zu, size %zu (%.3fms)", count, size, t);
	return CZ_RESULT_NO_MEMORY;
}

CZ_NONNULL_ARGS
static enum CzResult realloc_wrap(void* restrict* memory, void* ptr, size_t size)
{
	CZ_ASSUME(size != 0);

	void* p = realloc(ptr, size);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "realloc failed with ptr 0x%016" PRIxPTR ", size %zu (%.3fms)", (uintptr_t) ptr, size, t);
	return CZ_RESULT_NO_MEMORY;
}

#if defined(_WIN32)
CZ_NONNULL_ARGS
static enum CzResult recalloc_wrap(void* restrict* memory, void* ptr, size_t count, size_t size)
{
	CZ_ASSUME(count != 0);
	CZ_ASSUME(size != 0);

	void* p = _recalloc(ptr, count, size);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "_recalloc failed with ptr 0x%016" PRIxPTR ", count %zu, size %zu (%.3fms)",
		(uintptr_t) ptr, count, size, t);

	return CZ_RESULT_NO_MEMORY;
}

CZ_NONNULL_ARGS
static enum CzResult aligned_offset_malloc_wrap(void* restrict* memory, size_t size, size_t alignment, size_t offset)
{
	CZ_ASSUME(size != 0);
	CZ_ASSUME(alignment != 0);
	CZ_ASSUME(alignment & (alignment - 1) == 0);
	CZ_ASSUME(offset < size);

	void* p = _aligned_offset_malloc(size, alignment, offset);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "_aligned_offset_malloc failed with size %zu, alignment %zu, offset %zu (%.3fms)",
		size, alignment, offset, t);

	return CZ_RESULT_NO_MEMORY;
}

CZ_NONNULL_ARGS
static enum CzResult aligned_offset_realloc_wrap(
	void* restrict* memory, void* ptr, size_t size, size_t alignment, size_t offset)
{
	CZ_ASSUME(size != 0);
	CZ_ASSUME(alignment != 0);
	CZ_ASSUME(alignment & (alignment - 1) == 0);
	CZ_ASSUME(offset < size);

	void* p = _aligned_offset_realloc(ptr, size, alignment, offset);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr,
		"_aligned_offset_realloc failed with ptr 0x%016" PRIxPTR ", size %zu, alignment %zu, offset %zu (%.3fms)",
		(uintptr_t) ptr, size, alignment, offset, t);

	return CZ_RESULT_NO_MEMORY;
}

CZ_NONNULL_ARGS
static enum CzResult aligned_offset_recalloc_wrap(
	void* restrict* memory, void* ptr, size_t count, size_t size, size_t alignment, size_t offset)
{
	CZ_ASSUME(count != 0);
	CZ_ASSUME(size != 0);
	CZ_ASSUME(alignment != 0);
	CZ_ASSUME(alignment & (alignment - 1) == 0);
	CZ_ASSUME(offset < count * size);

	void* p = _aligned_offset_recalloc(ptr, count, size, alignment, offset);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr,
		"_aligned_offset_recalloc failed with "
		"ptr 0x%016" PRIxPTR ", count %zu, size %zu, alignment %zu, offset %zu (%.3fms)",
		(uintptr_t) ptr, count, size, alignment, offset, t);

	return CZ_RESULT_NO_MEMORY;
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS
static enum CzResult posix_memalign_wrap(void* restrict* memory, size_t alignment, size_t size)
{
	CZ_ASSUME(alignment >= sizeof(void*));
	CZ_ASSUME((alignment & (alignment - 1)) == 0);
	CZ_ASSUME(size != 0);

	void* p;
	int r = posix_memalign(&p, alignment, size);
	if CZ_EXPECT (!r) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "posix_memalign failed with ptr 0x%016" PRIxPTR ", alignment %zu, size %zu (%.3fms)",
		(uintptr_t) memory, alignment, size, t);

	return CZ_RESULT_NO_MEMORY;
}
#endif

enum CzResult czAlloc(void* restrict* restrict memory, size_t size, struct CzAllocFlags flags)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;

	if (flags.zeroInitialise)
		ret = calloc_wrap(memory, size, sizeof(char));
	else
		ret = malloc_wrap(memory, size);

	return ret;
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

#if defined(_WIN32)
	if (flags.zeroInitialise)
		ret = recalloc_wrap(memory, *memory, newSize, sizeof(char));
	else
		ret = realloc_wrap(memory, *memory, newSize);

	if CZ_NOEXPECT (ret)
		goto err_free_memory;
#else
	ret = realloc_wrap(memory, *memory, newSize);
	if CZ_NOEXPECT (ret)
		goto err_free_memory;

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedBytes = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedBytes, 0, addedSize);
	}
#endif
	return CZ_RESULT_SUCCESS;

err_free_memory:
	if (flags.freeOnFail)
		czFree(*memory);

	return ret;
}

enum CzResult czFree(void* restrict memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;

	free(memory);
	return CZ_RESULT_SUCCESS;
}

enum CzResult czAllocAlign(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (!alignment || alignment & (alignment - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	if CZ_NOEXPECT (offset >= size)
		return CZ_RESULT_BAD_OFFSET;

	offset &= alignment - 1; // Ensure offset < alignment

#if defined(_WIN32)
	if (flags.zeroInitialise)
		ret = aligned_offset_recalloc_wrap(memory, NULL, size, sizeof(char), alignment, offset);
	else
		ret = aligned_offset_malloc_wrap(memory, size, alignment, offset);

	if CZ_NOEXPECT (ret)
		return ret;
#elif defined(__APPLE__) || defined(__unix__)
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t extraSize = offset && alignment <= sizeof(void*) ? sizeof(void*) : 0;
	size_t allocSize = size + allocAlignment + extraSize;

	ret = posix_memalign_wrap(&p, allocAlignment, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) p + allocAlignment + extraSize - offset;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;
	if (flags.zeroInitialise)
		memset(*memory, 0, size);
#else
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t allocSize = size + allocAlignment * 2;

	if (flags.zeroInitialise)
		ret = calloc_wrap(&p, allocSize, sizeof(char));
	else
		ret = malloc_wrap(&p, allocSize);

	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) ((uintptr_t) p & ~(allocAlignment - 1)) + allocAlignment * 2 - offset;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;
#endif
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

	offset &= alignment - 1; // Ensure offset < alignment

#if defined(_WIN32)
	if (flags.zeroInitialise)
		ret = aligned_offset_recalloc_wrap(memory, *memory, newSize, sizeof(char), alignment, offset);
	else
		ret = aligned_offset_realloc_wrap(memory, *memory, newSize, alignment, offset);

	if CZ_NOEXPECT (ret)
		goto err_free_memory;
#else
	void* p;
	struct CzAllocFlags allocFlags = {0};

	ret = czAllocAlign(&p, newSize, alignment, offset, allocFlags);
	if CZ_NOEXPECT (ret)
		goto err_free_memory;

	memcpy(p, *memory, minz(oldSize, newSize));
	czFreeAlign(*memory);
	*memory = p;

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedBytes = (char*) p + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedBytes, 0, addedSize);
	}
#endif
	return CZ_RESULT_SUCCESS;

err_free_memory:
	if (flags.freeOnFail)
		czFreeAlign(*memory);

	return ret;
}

enum CzResult czFreeAlign(void* restrict memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;

#if defined(_WIN32)
	_aligned_free(memory);
#else
	free(*((void**) ((uintptr_t) memory & ~(sizeof(void*) - 1)) - 1));
#endif
	return CZ_RESULT_SUCCESS;
}
