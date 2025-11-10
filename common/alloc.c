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
#include "wrap.h"

/**********************************************************************************************************************
 * Windows implementation                                                                                             *
 **********************************************************************************************************************/

#define HAVE_realloc_win32 ( 1 )

#if HAVE_realloc_win32
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_win32(void* restrict* memory, size_t newSize)
{
	return czWrap_realloc(memory, *memory, newSize);
}
#endif

#define HAVE_realloc_zero_win32 ( CZ_WRAP_RECALLOC )

#if HAVE_realloc_zero_win32
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_zero_win32(void* restrict* memory, size_t newSize)
{
	return czWrap_recalloc(memory, *memory, newSize, sizeof(char));
}
#endif

#define HAVE_alloc_align_win32 ( CZ_WRAP_ALIGNED_OFFSET_MALLOC )

#if HAVE_alloc_align_win32
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
static enum CzResult alloc_align_win32(void* restrict* memory, size_t size, size_t alignment, size_t offset)
{
	return czWrap_aligned_offset_malloc(memory, size, alignment, offset);
}
#endif

#define HAVE_alloc_align_zero_win32 ( CZ_WRAP_ALIGNED_OFFSET_RECALLOC )

#if HAVE_alloc_align_zero_win32
CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
static enum CzResult alloc_align_zero_win32(void* restrict* memory, size_t size, size_t alignment, size_t offset)
{
	return czWrap_aligned_offset_recalloc(memory, NULL, size, sizeof(char), alignment, offset);
}
#endif

#define HAVE_free_align_win32 ( CZ_WIN32 )

#if HAVE_free_align_win32
CZ_NONNULL_ARGS()
static enum CzResult free_align_win32(void* memory)
{
	_aligned_free(memory);
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_realloc_align_win32 ( CZ_WRAP_ALIGNED_OFFSET_REALLOC )

#if HAVE_realloc_align_win32
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_align_win32(void* restrict* memory, size_t newSize, size_t alignment, size_t offset)
{
	return czWrap_aligned_offset_realloc(memory, *memory, newSize, alignment, offset);
}
#endif

#define HAVE_realloc_align_zero_win32 ( CZ_WRAP_ALIGNED_OFFSET_RECALLOC )

#if HAVE_realloc_align_zero_win32
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_align_zero_win32(void* restrict* memory, size_t newSize, size_t alignment, size_t offset)
{
	return czWrap_aligned_offset_recalloc(memory, *memory, newSize, sizeof(char), alignment, offset);
}
#endif

/**********************************************************************************************************************
 * Standard C implementation                                                                                          *
 **********************************************************************************************************************/

#define ADDR_ALIGN_STDC(ptr) ( *((void**) ((uintptr_t) ptr & ~(sizeof(void*) - 1)) - 1) )

CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
static enum CzResult alloc_stdc(void* restrict* memory, size_t size)
{
	return czWrap_malloc(memory, size);
}

CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
static enum CzResult alloc_zero_stdc(void* restrict* memory, size_t size)
{
	return czWrap_calloc(memory, size, sizeof(char));
}

CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_stdc(void* restrict* memory, size_t newSize)
{
	return czWrap_realloc(memory, *memory, newSize);
}

CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_zero_stdc(void* restrict* memory, size_t oldSize, size_t newSize)
{
	if (oldSize >= newSize)
		return realloc_stdc(memory, newSize);

	if (newSize - oldSize > oldSize) {
		void* oldMemory = *memory;
		enum CzResult ret = alloc_zero_stdc(memory, newSize);
		if CZ_NOEXPECT (ret)
			return ret;

		memcpy(*memory, oldMemory, oldSize);
		free(oldMemory);
		return CZ_RESULT_SUCCESS;
	}

	enum CzResult ret = realloc_stdc(memory, newSize);
	if CZ_NOEXPECT (ret)
		return ret;

	void* addedMemory = (char*) *memory + oldSize;
	size_t addedSize = newSize - oldSize;
	memset(addedMemory, 0, addedSize);
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
static enum CzResult alloc_align_stdc(void* restrict* memory, size_t size, size_t alignment, size_t offset)
{
	if (alignment < sizeof(void*))
		alignment = sizeof(void*);

	void* addr;
	size_t allocSize = size + alignment * 2;
	size_t paddingSize = alignment * 2 - offset;

	enum CzResult ret = alloc_stdc(&addr, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) ((uintptr_t) addr & ~(alignment - 1)) + paddingSize;
	ADDR_ALIGN_STDC(*memory) = addr;
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS() CZ_WR_ACCESS(1)
static enum CzResult alloc_align_zero_stdc(void* restrict* memory, size_t size, size_t alignment, size_t offset)
{
	if (alignment < sizeof(void*))
		alignment = sizeof(void*);

	void* addr;
	size_t allocSize = size + alignment * 2;
	size_t paddingSize = alignment * 2 - offset;

	enum CzResult ret = alloc_zero_stdc(&addr, allocSize);
	if CZ_NOEXPECT (ret)
		return ret;

	*memory = (char*) ((uintptr_t) addr & ~(alignment - 1)) + paddingSize;
	ADDR_ALIGN_STDC(*memory) = addr;
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS()
static enum CzResult free_align_stdc(void* memory)
{
	void* addr = ADDR_ALIGN_STDC(memory);
	free(addr);
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_align_stdc(
	void* restrict* memory, size_t oldSize, size_t newSize, size_t alignment, size_t offset)
{
	void* oldMemory = *memory;
	enum CzResult ret = alloc_align_stdc(memory, newSize, alignment, offset);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t cpySize = (newSize > oldSize) ? oldSize : newSize;
	memcpy(*memory, oldMemory, cpySize);
	return free_align_stdc(oldMemory);
}

CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult realloc_align_zero_stdc(
	void* restrict* memory, size_t oldSize, size_t newSize, size_t alignment, size_t offset)
{
	if (oldSize >= newSize)
		return realloc_align_stdc(memory, oldSize, newSize, alignment, offset);

	void* oldMemory = *memory;
	enum CzResult ret = alloc_align_zero_stdc(memory, newSize, alignment, offset);
	if CZ_NOEXPECT (ret)
		return ret;

	memcpy(*memory, oldMemory, oldSize);
	return free_align_stdc(oldMemory);
}

/**********************************************************************************************************************
 * API function definitions                                                                                           *
 **********************************************************************************************************************/

enum CzResult czAlloc(void* restrict* memory, size_t size, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;

	if (flags.zeroInitialise)
		return alloc_zero_stdc(memory, size);
	return alloc_stdc(memory, size);
}

enum CzResult czFree(void* memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;

	free(memory);
	return CZ_RESULT_SUCCESS;
}

#define HAVE_czRealloc_win32 ( \
	HAVE_realloc_win32 &&      \
	HAVE_realloc_zero_win32 )

#if HAVE_czRealloc_win32
CZ_COPY_ATTR(czRealloc)
static enum CzResult czRealloc_win32(void* restrict* memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	(void) oldSize;

	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = realloc_zero_win32(memory, newSize);
	else
		ret = realloc_win32(memory, newSize);

	if CZ_NOEXPECT (ret && flags.freeOnFail)
		free(*memory);
	return ret;
}
#endif

CZ_COPY_ATTR(czRealloc)
static enum CzResult czRealloc_stdc(void* restrict* memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = realloc_zero_stdc(memory, oldSize, newSize);
	else
		ret = realloc_stdc(memory, newSize);

	if CZ_NOEXPECT (ret && flags.freeOnFail)
		free(*memory);
	return ret;
}

enum CzResult czRealloc(void* restrict* memory, size_t oldSize, size_t newSize, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!oldSize) {
		if (flags.freeOnFail)
			free(*memory);
		return CZ_RESULT_BAD_SIZE;
	}
	if CZ_NOEXPECT (!newSize) {
		free(*memory);
		return CZ_RESULT_SUCCESS;
	}

#if HAVE_czRealloc_win32
	return czRealloc_win32(memory, oldSize, newSize, flags);
#else
	return czRealloc_stdc(memory, oldSize, newSize, flags);
#endif
}

#define HAVE_czAllocAlign_win32 ( \
	HAVE_alloc_align_win32 &&     \
	HAVE_alloc_align_zero_win32 )

#if HAVE_czAllocAlign_win32
CZ_COPY_ATTR(czAllocAlign)
static enum CzResult czAllocAlign_win32(
	void* restrict* memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if (flags.zeroInitialise)
		return alloc_align_zero_win32(memory, size, alignment, offset);
	return alloc_align_win32(memory, size, alignment, offset);
}
#endif

CZ_COPY_ATTR(czAllocAlign)
static enum CzResult czAllocAlign_stdc(
	void* restrict* memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if (flags.zeroInitialise)
		return alloc_align_zero_stdc(memory, size, alignment, offset);
	return alloc_align_stdc(memory, size, alignment, offset);
}

enum CzResult czAllocAlign(
	void* restrict* memory, size_t size, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (!alignment || alignment & (alignment - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	if CZ_NOEXPECT (offset >= size)
		return CZ_RESULT_BAD_OFFSET;

	offset &= alignment - 1; // Ensure offset < alignment

#if HAVE_czAllocAlign_win32
	return czAllocAlign_win32(memory, size, alignment, offset, flags);
#else
	return czAllocAlign_stdc(memory, size, alignment, offset, flags);
#endif
}

#define HAVE_czFreeAlign_win32 ( HAVE_free_align_win32 )

#if HAVE_czFreeAlign_win32
CZ_COPY_ATTR(czFreeAlign)
static enum CzResult czFreeAlign_win32(void* memory)
{
	return free_align_win32(memory);
}
#endif

CZ_COPY_ATTR(czFreeAlign)
static enum CzResult czFreeAlign_stdc(void* memory)
{
	return free_align_stdc(memory);
}

enum CzResult czFreeAlign(void* memory)
{
	if CZ_NOEXPECT (!memory)
		return CZ_RESULT_BAD_ADDRESS;

#if HAVE_czFreeAlign_win32
	return czFreeAlign_win32(memory);
#else
	return czFreeAlign_stdc(memory);
#endif
}

#define HAVE_czReallocAlign_win32 ( \
	HAVE_free_align_win32 &&        \
	HAVE_realloc_align_win32 &&     \
	HAVE_realloc_align_zero_win32 )

#if HAVE_czReallocAlign_win32
CZ_COPY_ATTR(czReallocAlign)
static enum CzResult czReallocAlign_win32(
	void* restrict* memory, size_t oldSize, size_t newSize, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	(void) oldSize;

	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = realloc_align_zero_win32(memory, newSize, alignment, offset);
	else
		ret = realloc_align_win32(memory, newSize, alignment, offset);

	if CZ_NOEXPECT (ret && flags.freeOnFail)
		free_align_win32(*memory);
	return ret;
}
#endif

CZ_COPY_ATTR(czReallocAlign)
static enum CzResult czReallocAlign_stdc(
	void* restrict* memory, size_t oldSize, size_t newSize, size_t alignment, size_t offset, struct CzAllocFlags flags)
{
	enum CzResult ret;
	if (flags.zeroInitialise)
		ret = realloc_align_zero_stdc(memory, oldSize, newSize, alignment, offset);
	else
		ret = realloc_align_stdc(memory, oldSize, newSize, alignment, offset);

	if CZ_NOEXPECT (ret && flags.freeOnFail)
		free_align_stdc(*memory);
	return ret;
}

enum CzResult czReallocAlign(
	void* restrict* memory, size_t oldSize, size_t newSize, size_t alignment, size_t offset, struct CzAllocFlags flags)
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

#if HAVE_czReallocAlign_win32
	return czReallocAlign_win32(memory, oldSize, newSize, alignment, offset, flags);
#else
	return czReallocAlign_stdc(memory, oldSize, newSize, alignment, offset, flags);
#endif
}
