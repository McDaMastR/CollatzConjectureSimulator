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

CZ_NONNULL_ARG(1)
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

static enum CzResult free_wrap(void* memory)
{
	free(memory);
	return CZ_RESULT_SUCCESS;
}

#if defined(_WIN32)
CZ_NONNULL_ARG(1)
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

CZ_NONNULL_ARG(1)
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

CZ_NONNULL_ARG(1)
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

static enum CzResult aligned_free_wrap(void* memory)
{
	_aligned_free(memory);
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(__APPLE__)
CZ_NONNULL_ARG(1)
static enum CzResult reallocf_wrap(void* restrict* memory, void* ptr, size_t size)
{
	CZ_ASSUME(size != 0);

	void* p = reallocf(ptr, size);
	if CZ_EXPECT (p) {
		*memory = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "reallocf failed with ptr 0x%016" PRIxPTR ", size %zu (%.3fms)", (uintptr_t) ptr, size, t);
	return CZ_RESULT_NO_MEMORY;
}

CZ_NONNULL_ARGS
static enum CzResult madvise_wrap(void* memory, size_t size, int advice)
{
	CZ_ASSUME(size != 0);

	int r = madvise(memory, size, advice);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
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
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if (flags.zeroInitialise)
		return calloc_wrap(memory, size, sizeof(char));
	return malloc_wrap(memory, size);
}

enum CzResult czFree(void* restrict memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;
	return free_wrap(memory);
}

#if defined(_WIN32)
CZ_NONNULL_ARGS
static enum CzResult realloc_win32(
	void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	(void) oldSize;
	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = recalloc_wrap(memory, *memory, newSize, sizeof(char));
	else
		ret = realloc_wrap(memory, *memory, newSize);
	if CZ_NOEXPECT (ret && flags.freeOnFail)
		czFree(*memory);
	return ret;
}
#endif

#if defined(__APPLE__)
CZ_NONNULL_ARGS
static enum CzResult realloc_apple(
	void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	enum CzResult ret;
	if (flags.freeOnFail)
		ret = reallocf_wrap(memory, *memory, newSize);
	else
		ret = realloc_wrap(memory, *memory, newSize);
	if CZ_NOEXPECT (ret)
		return ret;

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedMemory = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		ret = madvise_wrap(addedMemory, addedSize, MADV_ZERO);
		if (ret)
			memset(addedMemory, 0, addedSize);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

CZ_NONNULL_ARGS
static enum CzResult realloc_other(
	void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	enum CzResult ret = realloc_wrap(memory, *memory, newSize);
	if CZ_NOEXPECT (ret) {
		if (flags.freeOnFail)
			czFree(*memory);
		return ret;
	}

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedMemory = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedMemory, 0, addedSize);
	}
	return CZ_RESULT_SUCCESS;
}

enum CzResult czRealloc(void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!oldSize) {
		if (flags.freeOnFail)
			czFree(*memory);
		return CZ_RESULT_BAD_SIZE;
	}
	if CZ_NOEXPECT (!newSize) {
		czFree(*memory);
		return CZ_RESULT_SUCCESS;
	}

#if defined(_WIN32)
	return realloc_win32(memory, oldSize, newSize, flags);
#elif defined(__APPLE__)
	return realloc_apple(memory, oldSize, newSize, flags);
#else
	return realloc_other(memory, oldSize, newSize, flags);
#endif
}

#if defined(_WIN32)
CZ_NONNULL_ARGS
static enum CzResult alloc_align_win32(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if (flags.zeroInitialise)
		return aligned_offset_recalloc_wrap(memory, NULL, size, sizeof(char), alignment, offset);
	return aligned_offset_malloc_wrap(memory, size, alignment, offset);
}
#endif

#if defined(__APPLE__)
CZ_NONNULL_ARGS
static enum CzResult alloc_align_apple(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t extraSize = (offset && alignment <= sizeof(void*)) ? sizeof(void*) : 0;
	size_t allocSize = size + allocAlignment + extraSize;
	size_t paddingSize = allocAlignment + extraSize - offset;

	enum CzResult ret = posix_memalign_wrap(&p, allocAlignment, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) p + paddingSize;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;
	madvise_wrap(p, paddingSize, MADV_DONTNEED);

	if (flags.zeroInitialise) {
		ret = madvise_wrap(*memory, size, MADV_ZERO);
		if (ret)
			memset(*memory, 0, size);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(__unix__)
CZ_NONNULL_ARGS
static enum CzResult alloc_align_unix(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t extraSize = (offset && alignment <= sizeof(void*)) ? sizeof(void*) : 0;
	size_t allocSize = size + allocAlignment + extraSize;
	size_t paddingSize = allocAlignment + extraSize - offset;

	enum CzResult ret = posix_memalign_wrap(&p, allocAlignment, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) p + paddingSize;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;

	if (flags.zeroInitialise)
		memset(*memory, 0, size);
	return CZ_RESULT_SUCCESS;
}
#endif

CZ_NONNULL_ARGS
static enum CzResult alloc_align_other(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t allocSize = size + allocAlignment * 2;
	size_t paddingSize = allocAlignment * 2 - offset;

	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = calloc_wrap(&p, allocSize, sizeof(char));
	else
		ret = malloc_wrap(&p, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) ((uintptr_t) p & ~(allocAlignment - 1)) + paddingSize;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;
	return CZ_RESULT_SUCCESS;
}

enum CzResult czAllocAlign(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (!alignment || alignment & (alignment - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	if CZ_NOEXPECT (offset >= size)
		return CZ_RESULT_BAD_OFFSET;

	offset &= alignment - 1; // Ensure offset < alignment

#if defined(_WIN32)
	return alloc_align_win32(memory, size, alignment, offset, flags);
#elif defined(__APPLE__)
	return alloc_align_apple(memory, size, alignment, offset, flags);
#elif defined(__unix__)
	return alloc_align_unix(memory, size, alignment, offset, flags);
#else
	return alloc_align_other(memory, size, alignment, offset, flags);
#endif
}

#if defined(_WIN32)
CZ_NONNULL_ARGS
static enum CzResult free_align_win32(void* restrict memory)
{
	return aligned_free_wrap(memory);
}
#endif

CZ_NONNULL_ARGS
static enum CzResult free_align_other(void* restrict memory)
{
	return free_wrap(*((void**) ((uintptr_t) memory & ~(sizeof(void*) - 1)) - 1));
}

enum CzResult czFreeAlign(void* restrict memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;
#if defined(_WIN32)
	return free_align_win32(memory);
#else
	return free_align_other(memory);
#endif
}

#if defined(_WIN32)
CZ_NONNULL_ARGS
static enum CzResult realloc_align_win32(
	void* restrict* restrict memory,
	size_t oldSize,
	size_t newSize,
	size_t alignment,
	size_t offset,
	struct CzAllocFlags flags)
{
	(void) oldSize;
	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = aligned_offset_recalloc_wrap(memory, *memory, newSize, sizeof(char), alignment, offset);
	else
		ret = aligned_offset_realloc_wrap(memory, *memory, newSize, alignment, offset);
	if CZ_NOEXPECT (ret && flags.freeOnFail)
		free_align_win32(*memory);
	return ret;
}
#endif

#if defined(__APPLE__)
CZ_NONNULL_ARGS
static enum CzResult realloc_align_apple(
	void* restrict* restrict memory,
	size_t oldSize,
	size_t newSize,
	size_t alignment,
	size_t offset,
	struct CzAllocFlags flags)
{
	void* oldMemory = *memory;
	struct CzAllocFlags allocFlags = {0};
	enum CzResult ret = alloc_align_apple(memory, newSize, alignment, offset, allocFlags);
	if CZ_NOEXPECT (ret) {
		if (flags.freeOnFail)
			free_align_other(oldMemory);
		return ret;
	}

	memcpy(*memory, oldMemory, minz(oldSize, newSize));
	free_align_other(oldMemory);

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedMemory = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		ret = madvise_wrap(addedMemory, addedSize, MADV_ZERO);
		if (ret)
			memset(addedMemory, 0, addedSize);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(__unix__)
CZ_NONNULL_ARGS
static enum CzResult realloc_align_unix(
	void* restrict* restrict memory,
	size_t oldSize,
	size_t newSize,
	size_t alignment,
	size_t offset,
	struct CzAllocFlags flags)
{
	void* oldMemory = *memory;
	struct CzAllocFlags allocFlags = {0};
	enum CzResult ret = alloc_align_unix(memory, newSize, alignment, offset, allocFlags);
	if CZ_NOEXPECT (ret) {
		if (flags.freeOnFail)
			free_align_other(oldMemory);
		return ret;
	}

	memcpy(*memory, oldMemory, minz(oldSize, newSize));
	free_align_other(oldMemory);

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedMemory = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedMemory, 0, addedSize);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

CZ_NONNULL_ARGS
static enum CzResult realloc_align_other(
	void* restrict* restrict memory,
	size_t oldSize,
	size_t newSize,
	size_t alignment,
	size_t offset,
	struct CzAllocFlags flags)
{
	void* oldMemory = *memory;
	struct CzAllocFlags allocFlags = {0};
	enum CzResult ret = alloc_align_other(memory, newSize, alignment, offset, allocFlags);
	if CZ_NOEXPECT (ret) {
		if (flags.freeOnFail)
			free_align_other(oldMemory);
		return ret;
	}

	memcpy(*memory, oldMemory, minz(oldSize, newSize));
	free_align_other(oldMemory);

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedMemory = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		memset(addedMemory, 0, addedSize);
	}
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
	if CZ_NOEXPECT (!oldSize) {
		if (flags.freeOnFail)
			czFreeAlign(*memory);
		return CZ_RESULT_BAD_SIZE;
	}
	if CZ_NOEXPECT (!newSize) {
		czFreeAlign(*memory);
		return CZ_RESULT_SUCCESS;
	}
	if CZ_NOEXPECT (!alignment || alignment & (alignment - 1)) {
		if (flags.freeOnFail)
			czFreeAlign(*memory);
		return CZ_RESULT_BAD_ALIGNMENT;
	}
	if CZ_NOEXPECT (offset >= newSize) {
		if (flags.freeOnFail)
			czFreeAlign(*memory);
		return CZ_RESULT_BAD_OFFSET;
	}

	offset &= alignment - 1; // Ensure offset < alignment

#if defined(_WIN32)
	return realloc_align_win32(memory, oldSize, newSize, alignment, offset, flags);
#elif defined(__APPLE__)
	return realloc_align_apple(memory, oldSize, newSize, alignment, offset, flags);
#elif defined(__unix__)
	return realloc_align_unix(memory, oldSize, newSize, alignment, offset, flags);
#else
	return realloc_align_other(memory, oldSize, newSize, alignment, offset, flags);
#endif
}
