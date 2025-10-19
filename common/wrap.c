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

#include "wrap.h"
#include "debug.h"
#include "util.h"

enum CzResult czWrap_malloc(void* restrict* res, size_t size)
{
	void* p = malloc(size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "malloc failed with size %zu (%.3fms)", size, t);
	return CZ_RESULT_NO_MEMORY;
}

enum CzResult czWrap_calloc(void* restrict* res, size_t count, size_t size)
{
	void* p = calloc(count, size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "calloc failed with count %zu, size %zu (%.3fms)", count, size, t);
	return CZ_RESULT_NO_MEMORY;
}

enum CzResult czWrap_realloc(void* restrict* res, void* ptr, size_t size)
{
	void* p = realloc(ptr, size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "realloc failed with ptr 0x%016" PRIxPTR ", size %zu (%.3fms)", (uintptr_t) ptr, size, t);
	return CZ_RESULT_NO_MEMORY;
}

#if CZ_WRAP_REALLOCF
enum CzResult czWrap_reallocf(void* restrict* res, void* ptr, size_t size)
{
	void* p = reallocf(ptr, size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "reallocf failed with ptr 0x%016" PRIxPTR ", size %zu (%.3fms)", (uintptr_t) ptr, size, t);
	return CZ_RESULT_NO_MEMORY;
}
#endif

#if CZ_WRAP_RECALLOC
enum CzResult czWrap_recalloc(void* restrict* res, void* ptr, size_t count, size_t size)
{
	void* p = _recalloc(ptr, count, size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "_recalloc failed with ptr 0x%016" PRIxPTR ", count %zu, size %zu (%.3fms)",
		(uintptr_t) ptr, count, size, t);

	return CZ_RESULT_NO_MEMORY;
}
#endif

#if CZ_WRAP_POSIX_MEMALIGN
enum CzResult czWrap_posix_memalign(int* res, void* restrict* ptr, size_t alignment, size_t size)
{
	void* p;
	int r = posix_memalign(&p, alignment, size);
	if (res)
		*res = r;
	if CZ_EXPECT (!r) {
		*ptr = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "posix_memalign failed with ptr 0x%016" PRIxPTR ", alignment %zu, size %zu (%.3fms)",
		(uintptr_t) ptr, alignment, size, t);

	switch (r) {
	case EINVAL:
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_ALIGNED_OFFSET_MALLOC
enum CzResult czWrap_aligned_offset_malloc(void* restrict* res, size_t size, size_t alignment, size_t offset)
{
	void* p = _aligned_offset_malloc(size, alignment, offset);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "_aligned_offset_malloc failed with size %zu, alignment %zu, offset %zu (%.3fms)",
		size, alignment, offset, t);

	switch (errno) {
	case EINVAL:
		return (!offset || offset < size) ? CZ_RESULT_BAD_ALIGNMENT : CZ_RESULT_BAD_OFFSET;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_ALIGNED_OFFSET_REALLOC
enum CzResult czWrap_aligned_offset_realloc(
	void* restrict* res, void* ptr, size_t size, size_t alignment, size_t offset)
{
	void* p = _aligned_offset_realloc(ptr, size, alignment, offset);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr,
		"_aligned_offset_realloc failed with ptr 0x%016" PRIxPTR ", size %zu, alignment %zu, offset %zu (%.3fms)",
		(uintptr_t) ptr, size, alignment, offset, t);

	switch (errno) {
	case EINVAL:
		return (!offset || offset < size) ? CZ_RESULT_BAD_ALIGNMENT : CZ_RESULT_BAD_OFFSET;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_ALIGNED_OFFSET_RECALLOC
enum CzResult czWrap_aligned_offset_recalloc(
	void* restrict* res, void* ptr, size_t count, size_t size, size_t alignment, size_t offset)
{
	void* p = _aligned_offset_recalloc(ptr, count, size, alignment, offset);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr,
		"_aligned_offset_recalloc failed with "
		"ptr 0x%016" PRIxPTR ", count %zu, size %zu, alignment %zu, offset %zu (%.3fms)",
		(uintptr_t) ptr, count, size, alignment, offset, t);

	switch (errno) {
	case EINVAL:
		return (!offset || offset < count * size) ? CZ_RESULT_BAD_ALIGNMENT : CZ_RESULT_BAD_OFFSET;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_MADVISE
enum CzResult czWrap_madvise(int* res, void* addr, size_t size, int advice)
{
	int r = madvise(addr, size, advice);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif
