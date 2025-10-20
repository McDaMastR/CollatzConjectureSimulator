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
#include "util.h"
#include "wrap.h"

enum CzResult czAlloc(void* restrict* restrict memory, size_t size, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if (flags.zeroInitialise)
		return czWrap_calloc(memory, size, sizeof(char));
	return czWrap_malloc(memory, size);
}

enum CzResult czFree(void* restrict memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;

	free(memory);
	return CZ_RESULT_SUCCESS;
}

#if defined(_WIN32)
static enum CzResult realloc_win32(
	void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	(void) oldSize;
	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = czWrap_recalloc(memory, *memory, newSize, sizeof(char));
	else
		ret = czWrap_realloc(memory, *memory, newSize);
	if CZ_NOEXPECT (ret && flags.freeOnFail)
		czFree(*memory);
	return ret;
}
#endif

#if defined(__APPLE__)
static enum CzResult realloc_apple(
	void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	enum CzResult ret;
	if (flags.freeOnFail)
		ret = czWrap_reallocf(memory, *memory, newSize);
	else
		ret = czWrap_realloc(memory, *memory, newSize);
	if CZ_NOEXPECT (ret)
		return ret;

	if (flags.zeroInitialise && oldSize < newSize) {
		void* addedMemory = (char*) *memory + oldSize;
		size_t addedSize = newSize - oldSize;
		ret = czWrap_madvise(NULL, addedMemory, addedSize, MADV_ZERO);
		if (ret)
			memset(addedMemory, 0, addedSize);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

static enum CzResult realloc_other(
	void* restrict* restrict memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	enum CzResult ret = czWrap_realloc(memory, *memory, newSize);
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
static enum CzResult alloc_align_win32(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if (flags.zeroInitialise)
		return czWrap_aligned_offset_recalloc(memory, NULL, size, sizeof(char), alignment, offset);
	return czWrap_aligned_offset_malloc(memory, size, alignment, offset);
}
#endif

#if defined(__APPLE__)
static enum CzResult alloc_align_apple(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t extraSize = (offset && alignment <= sizeof(void*)) ? sizeof(void*) : 0;
	size_t allocSize = size + allocAlignment + extraSize;
	size_t paddingSize = allocAlignment + extraSize - offset;

	enum CzResult ret = czWrap_posix_memalign(NULL, &p, allocAlignment, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) p + paddingSize;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;
	czWrap_madvise(NULL, p, paddingSize, MADV_DONTNEED);

	if (flags.zeroInitialise) {
		ret = czWrap_madvise(NULL, *memory, size, MADV_ZERO);
		if (ret)
			memset(*memory, 0, size);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_POSIX_MEMALIGN
static enum CzResult alloc_align_posix(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t extraSize = (offset && alignment <= sizeof(void*)) ? sizeof(void*) : 0;
	size_t allocSize = size + allocAlignment + extraSize;
	size_t paddingSize = allocAlignment + extraSize - offset;

	enum CzResult ret = czWrap_posix_memalign(NULL, &p, allocAlignment, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) p + paddingSize;
	*((void**) ((uintptr_t) *memory & ~(sizeof(void*) - 1)) - 1) = p;

	if (flags.zeroInitialise)
		memset(*memory, 0, size);
	return CZ_RESULT_SUCCESS;
}
#endif

static enum CzResult alloc_align_other(
	void* restrict* restrict memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	void* p;
	size_t allocAlignment = maxz(alignment, sizeof(void*));
	size_t allocSize = size + allocAlignment * 2;
	size_t paddingSize = allocAlignment * 2 - offset;

	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = czWrap_calloc(&p, allocSize, sizeof(char));
	else
		ret = czWrap_malloc(&p, allocSize);
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
#elif CZ_WRAP_POSIX_MEMALIGN
	return alloc_align_posix(memory, size, alignment, offset, flags);
#else
	return alloc_align_other(memory, size, alignment, offset, flags);
#endif
}

#if defined(_WIN32)
static enum CzResult free_align_win32(void* restrict memory)
{
	return _aligned_free(memory);
}
#endif

static enum CzResult free_align_other(void* restrict memory)
{
	free(*((void**) ((uintptr_t) memory & ~(sizeof(void*) - 1)) - 1));
	return CZ_RESULT_SUCCESS;
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
		ret = czWrap_aligned_offset_recalloc(memory, *memory, newSize, sizeof(char), alignment, offset);
	else
		ret = czWrap_aligned_offset_realloc(memory, *memory, newSize, alignment, offset);
	if CZ_NOEXPECT (ret && flags.freeOnFail)
		free_align_win32(*memory);
	return ret;
}
#endif

#if defined(__APPLE__)
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
		ret = czWrap_madvise(NULL, addedMemory, addedSize, MADV_ZERO);
		if (ret)
			memset(addedMemory, 0, addedSize);
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_POSIX_MEMALIGN
static enum CzResult realloc_align_posix(
	void* restrict* restrict memory,
	size_t oldSize,
	size_t newSize,
	size_t alignment,
	size_t offset,
	struct CzAllocFlags flags)
{
	void* oldMemory = *memory;
	struct CzAllocFlags allocFlags = {0};
	enum CzResult ret = alloc_align_posix(memory, newSize, alignment, offset, allocFlags);
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
#elif CZ_WRAP_POSIX_MEMALIGN
	return realloc_align_posix(memory, oldSize, newSize, alignment, offset, flags);
#else
	return realloc_align_other(memory, oldSize, newSize, alignment, offset, flags);
#endif
}
