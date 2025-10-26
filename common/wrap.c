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

#if CZ_DARWIN || CZ_GNU_LINUX
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	default:
		return CZ_RESULT_NO_MEMORY;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
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

#if CZ_DARWIN || CZ_GNU_LINUX
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	default:
		return CZ_RESULT_NO_MEMORY;
	}
#else
	if (!count)
		return CZ_RESULT_BAD_SIZE;
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
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

#if CZ_DARWIN || CZ_GNU_LINUX
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return size ? CZ_RESULT_INTERNAL_ERROR : CZ_RESULT_BAD_SIZE;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
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

#if CZ_DARWIN
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
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

	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
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
		if (!alignment)
			return CZ_RESULT_BAD_ALIGNMENT;
		if (alignment & (alignment - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_OFFSET;
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
		if (!alignment)
			return CZ_RESULT_BAD_ALIGNMENT;
		if (alignment & (alignment - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_OFFSET;
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
		if (!alignment)
			return CZ_RESULT_BAD_ALIGNMENT;
		if (alignment & (alignment - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_OFFSET;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_MADVISE
enum CzResult czWrap_madvise(void* addr, size_t size, int advice)
{
	int r = madvise(addr, size, advice);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
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
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize > 0 && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;

		switch (advice) {
#if CZ_LINUX_KSM
		case MADV_MERGEABLE:
		case MADV_UNMERGEABLE:
			return CZ_RESULT_NO_SUPPORT;
#endif
		default:
			return CZ_RESULT_BAD_ADDRESS;
		}
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EBUSY:
		switch (advice) {
#if CZ_LINUX_MEMORY_FAILURE
		case MADV_SOFT_OFFLINE:
			return CZ_RESULT_IN_USE;
#endif
#if CZ_LINUX_TRANSPARENT_HUGEPAGE
		case MADV_COLLAPSE:
			return CZ_RESULT_NO_OPEN;
#endif
		default:
			return CZ_RESULT_INTERNAL_ERROR;
		}
	case EBADF:
		return CZ_RESULT_NO_FILE;
	case EIO:
		return CZ_RESULT_NO_MEMORY;
	case ENOMEM:
		switch (advice) {
		case MADV_POPULATE_READ:
		case MADV_POPULATE_WRITE:
		case MADV_WILLNEED:
			return CZ_RESULT_NO_MEMORY;
#if CZ_LINUX_TRANSPARENT_HUGEPAGE
		case MADV_COLLAPSE:
			return CZ_RESULT_NO_MEMORY;
#endif
		default:
			return CZ_RESULT_BAD_ADDRESS;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_MADVISE
enum CzResult czWrap_posix_madvise(int* res, void* addr, size_t size, int advice)
{
	int r = posix_madvise(addr, size, advice);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (r) {
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
#elif CZ_GNU_LINUX
	switch (r) {
	case EINVAL:
		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_NO_SUPPORT;
		if (!((uintptr_t) addr & (uintptr_t) (pageSize - 1)))
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	switch (r) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_NO_SUPPORT;
		if (!((uintptr_t) addr & (uintptr_t) (pageSize - 1)))
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#endif
}
#endif

enum CzResult czWrap_fopen(FILE* restrict* res, const char* path, const char* mode)
{
	FILE* s = fopen(path, mode);
	if CZ_EXPECT (s) {
		*res = s;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EINVAL:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOENT:
		if (!mode)
			return CZ_RESULT_BAD_ADDRESS;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case EDEADLK:
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if (!mode)
			return CZ_RESULT_BAD_ADDRESS;
		if (mode[0] == 'r')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!mode)
			return CZ_RESULT_BAD_ADDRESS;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case EEXIST:
	case EFBIG:
	case EISDIR:
	case ENODEV:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EACCES:
	case EINVAL:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!mode)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EINVAL:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!mode)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!mode)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path)
		return CZ_RESULT_BAD_ADDRESS;
	if (!mode)
		return CZ_RESULT_BAD_ADDRESS;
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (mode[0] == 'r')
		return CZ_RESULT_NO_FILE;
	if (mode[0] == 'w')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'a')
		return CZ_RESULT_INTERNAL_ERROR;
	return CZ_RESULT_BAD_ACCESS;
#endif
}

enum CzResult czWrap_fclose(FILE* stream)
{
	int r = fclose(stream);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EDEADLK:
	case EFBIG:
	case ENXIO:
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EPERM:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (errno) {
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!stream)
		return CZ_RESULT_BAD_STREAM;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fseek(FILE* stream, long offset, int whence)
{
	int r = fseek(stream, offset, whence);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EDEADLK:
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!stream)
		CZ_RESULT_BAD_STREAM;
	return CZ_RESULT_BAD_OFFSET;
#endif
}

enum CzResult czWrap_ftell(long* res, FILE* stream)
{
	long r = ftell(stream);
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case EDEADLK:
	case EFBIG:
	case ENXIO:
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EINVAL:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_BAD_STREAM;
#endif
}

enum CzResult czWrap_fread(size_t* res, void* buffer, size_t size, size_t count, FILE* stream)
{
	size_t r = fread(buffer, size, count, stream);
	int err = ferror(stream);
	int eof = feof(stream);

	if (res)
		*res = r;
	if CZ_EXPECT (!err && r)
		return CZ_RESULT_SUCCESS;
	if (!err && !size)
		return CZ_RESULT_SUCCESS;
	if (!err && !count)
		return CZ_RESULT_SUCCESS;
	if (eof && !r) {
		long pos = ftell(stream);
		if (pos > 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_NO_FILE;
	}

#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fwrite(size_t* res, const void* buffer, size_t size, size_t count, FILE* stream)
{
	size_t r = fwrite(buffer, size, count, stream);
	int err = ferror(stream);
	if (res)
		*res = r;
	if CZ_EXPECT (!err && r == count)
		return CZ_RESULT_SUCCESS;
	if (!err && !size)
		return CZ_RESULT_SUCCESS;

#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_remove(const char* path)
{
	int r = remove(path);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOTEMPTY:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_XPG_1989
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path)
		return CZ_RESULT_BAD_ADDRESS;
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_NO_FILE;
#endif
}

#if CZ_WRAP_FILENO
enum CzResult czWrap_fileno(int* res, FILE* stream)
{
	int fd = fileno(stream);

#if CZ_DARWIN || CZ_WINDOWS
	*res = fd;
	return CZ_RESULT_SUCCESS;
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	if CZ_EXPECT (r != -1) {
		*res = fd;
		return CZ_RESULT_SUCCESS;
	}

	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if CZ_EXPECT (r != -1) {
		*res = fd;
		return CZ_RESULT_SUCCESS;
	}
	return CZ_RESULT_BAD_STREAM;
#endif
}
#endif

#if CZ_WRAP_ISATTY
enum CzResult czWrap_isatty(int* res, int fd)
{
	errno = 0;
	int r = isatty(fd);

#if CZ_WINDOWS
	if CZ_EXPECT (r || errno != EBADF) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}
	return CZ_RESULT_BAD_ACCESS;
#elif CZ_GNU_LINUX
	if CZ_EXPECT (r || errno == EINVAL || errno == ENOTTY) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	if CZ_EXPECT (r || !errno || errno == ENOTTY) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	*res = r;
	return CZ_RESULT_SUCCESS;
#endif
}
#endif

#if CZ_WRAP_STAT
enum CzResult czWrap_stat(const char* path, struct stat* st)
{
	int r = stat(path, st);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path)
		return CZ_RESULT_BAD_ADDRESS;
	if (!st)
		return CZ_RESULT_BAD_ADDRESS;
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_FSTAT
enum CzResult czWrap_fstat(int fd, struct stat* st)
{
	int r = fstat(fd, st);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (!st)
		return CZ_RESULT_BAD_ADDRESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_TRUNCATE
enum CzResult czWrap_truncate(const char* path, off_t size)
{
	int r = truncate(path, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_FILE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_XPG_1994
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path)
		return CZ_RESULT_BAD_ADDRESS;
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (size < 0)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_FTRUNCATE
enum CzResult czWrap_ftruncate(int fd, off_t size)
{
	int r = ftruncate(fd, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EDEADLK:
		return CZ_RESULT_BAD_FILE;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_FILE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_FILE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996
	switch (errno) {
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_XPG_1994
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (size < 0)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_OPEN
enum CzResult czWrap_open(int* res, const char* path, int flags, mode_t mode)
{
	int f = open(path, flags, mode);
	if CZ_EXPECT (f != -1) {
		*res = f;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EINVAL:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EDEADLK:
	case EEXIST:
	case EISDIR:
	case ENOTCAPABLE:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & (O_DIRECTORY | O_SEARCH))
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (flags & O_TMPFILE && !(flags & (O_WRONLY | O_RDWR)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & O_DIRECT)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_CREAT && flags & O_DIRECTORY)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_PATH;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		if (flags & O_TMPFILE && flags & (O_WRONLY | O_RDWR))
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case EFBIG:
	case ENODEV:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	case EISDIR:
		if (flags & O_TMPFILE && flags & (O_WRONLY | O_RDWR))
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & (O_DSYNC | O_RSYNC | O_SYNC))
			return CZ_RESULT_NO_SUPPORT
		return CZ_RESULT_BAD_FILE;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & (O_DSYNC | O_RSYNC | O_SYNC))
			return CZ_RESULT_NO_SUPPORT
		return CZ_RESULT_BAD_FILE;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & (O_DSYNC | O_RSYNC | O_SYNC))
			return CZ_RESULT_NO_SUPPORT
		return CZ_RESULT_BAD_FILE;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (!path)
			return CZ_RESULT_BAD_ADDRESS;
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EEXIST:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	case EINVAL:
		if (flags & (O_DSYNC | O_RSYNC | O_SYNC))
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path)
		return CZ_RESULT_BAD_ADDRESS;
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_CLOSE
enum CzResult czWrap_close(int fd)
{
	int r = close(fd);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_GNU_LINUX
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EINPROGRESS:
		return CZ_RESULT_SUCCESS;
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if POSIX_CLOSE_RESTART
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_PREAD
enum CzResult czWrap_pread(ssize_t* res, int fd, void* buffer, size_t size, off_t offset)
{
	ssize_t r = pread(fd, buffer, size, offset);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r) {
		if (!size)
			return CZ_RESULT_SUCCESS;
		if (offset)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_NO_FILE;
	}

#if CZ_DARWIN
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EISDIR:
	case ESPIPE:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_OFFSET;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ESTALE:
		return CZ_RESULT_NO_FILE;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ETIMEDOUT:
		return CZ_RESULT_TIMEOUT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EBADMSG:
	case EISDIR:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EBADMSG:
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (!buffer)
		return CZ_RESULT_BAD_ADDRESS;
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_WRITE
enum CzResult czWrap_write(ssize_t* res, int fd, const void* buffer, size_t size)
{
	ssize_t r = write(fd, buffer, size);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r) {
		if (!size)
			return CZ_RESULT_SUCCESS;
		return CZ_RESULT_INTERNAL_ERROR;
	}

#if CZ_DARWIN
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOBUFS:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINVAL:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOBUFS:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (!buffer)
		return CZ_RESULT_BAD_ADDRESS;
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_PWRITE
enum CzResult czWrap_pwrite(ssize_t* res, int fd, const void* buffer, size_t size, off_t offset)
{
	ssize_t r = pwrite(fd, buffer, size, offset);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r) {
		if (!size)
			return CZ_RESULT_SUCCESS;
		return CZ_RESULT_INTERNAL_ERROR;
	}

#if CZ_DARWIN
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ECONNRESET:
	case EDEADLK:
	case EFBIG:
	case ENETDOWN:
	case ENETUNREACH:
	case ENXIO:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDESTADDRREQ:
	case EFBIG:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOBUFS:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOBUFS:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (!buffer)
		return CZ_RESULT_BAD_ADDRESS;
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_MMAP
enum CzResult czWrap_mmap(void* restrict* res, void* addr, size_t size, int prot, int flags, int fd, off_t offset)
{
	void* p = mmap(addr, size, prot, flags, fd, offset);
	if CZ_EXPECT (p != MAP_FAILED) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
			return CZ_RESULT_BAD_ACCESS;
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_BAD_ADDRESS;
		if (offset & (off_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1) && flags & MAP_FIXED)
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ADDRESS;
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_ADDRESS;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case ENOMEM:
		if (flags & MAP_ANON)
			return CZ_RESULT_NO_MEMORY;
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case EACCES:
	case EBADF:
	case EPERM:
	case ETXTBSY:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (MAP_PRIVATE | MAP_SHARED | MAP_SHARED_VALIDATE)))
			return CZ_RESULT_BAD_ACCESS;
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize > 0 && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (pageSize > 0 && offset & (off_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (addr)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_OFFSET;
	case EEXIST:
	case EOVERFLOW:
		return CZ_RESULT_BAD_ADDRESS;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case ENODEV:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
			return CZ_RESULT_BAD_ACCESS;
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize > 0 && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (pageSize > 0 && offset & (off_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (addr)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_OFFSET;
	case EOVERFLOW:
		return CZ_RESULT_BAD_ADDRESS;
	case ENXIO:
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		if (!(flags & MAP_ANON))
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_FILE;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
			return CZ_RESULT_BAD_ACCESS;
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize > 0 && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (pageSize > 0 && offset & (off_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (addr)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_OFFSET;
	case EOVERFLOW:
		return CZ_RESULT_BAD_ADDRESS;
	case ENXIO:
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_FILE;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENXIO:
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_FILE;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_XPG_1994
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENXIO:
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_FILE;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
		return CZ_RESULT_BAD_ACCESS;
	if (fd < -1)
		return CZ_RESULT_BAD_ACCESS;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;

	long pageSize = sysconf(_SC_PAGESIZE);
	if (pageSize <= 0)
		return CZ_RESULT_INTERNAL_ERROR;
	if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	if (offset & (off_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_MUNMAP
enum CzResult czWrap_munmap(void* addr, size_t size)
{
	int r = munmap(addr, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_GNU_LINUX
	switch (errno) {
	case EINVAL:
		if (!addr)
			return CZ_RESULT_BAD_ADDRESS;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_BAD_ADDRESS;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ADDRESS;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EINVAL:
		if (!addr)
			return CZ_RESULT_BAD_ADDRESS;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_BAD_ADDRESS;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!addr)
		return CZ_RESULT_BAD_ADDRESS;
	if (!size)
		return CZ_RESULT_BAD_SIZE;

	long pageSize = sysconf(_SC_PAGESIZE);
	if (pageSize <= 0)
		return CZ_RESULT_INTERNAL_ERROR;
	if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_MSYNC
enum CzResult czWrap_msync(void* addr, size_t size, int flags)
{
	int r = msync(addr, size, flags);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if (!addr)
			return CZ_RESULT_BAD_ADDRESS;
		if (!size)
			return CZ_RESULT_BAD_SIZE;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_BAD_ACCESS;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if (!addr)
			return CZ_RESULT_BAD_ADDRESS;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_BAD_ACCESS;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_XPG_1994
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if (!addr)
			return CZ_RESULT_BAD_ADDRESS;

		long pageSize = sysconf(_SC_PAGESIZE);
		if (pageSize <= 0)
			return CZ_RESULT_BAD_ACCESS;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!addr)
		return CZ_RESULT_BAD_ADDRESS;
	if (!size)
		return CZ_RESULT_BAD_SIZE;

	long pageSize = sysconf(_SC_PAGESIZE);
	if (pageSize <= 0)
			return CZ_RESULT_INTERNAL_ERROR;
	if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_GET_OSFHANDLE
enum CzResult czWrap_get_osfhandle(intptr_t* res, int fd)
{
	intptr_t h = _get_osfhandle(fd);
	if CZ_EXPECT (h != INVALID_HANDLE_VALUE) {
		*res = h;
		return CZ_RESULT_SUCCESS;
	}
	return CZ_RESULT_INTERNAL_ERROR;
}
#endif

#if CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR
enum CzResult czWrap_MultiByteToWideChar(
	LPINT res, UINT codePage, DWORD flags, LPCCH mbStr, INT mbSize, LPWSTR wcStr, INT wcSize)
{
	INT s = MultiByteToWideChar(codePage, flags, mbStr, mbSize, wcStr, wcSize);
	if (res)
		*res = s;
	if CZ_EXPECT (s)
		return CZ_RESULT_SUCCESS;

	// General strategy for Windows API error handling: its documentation is shit, so just guess and check a bunch
	switch (GetLastError()) {
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_PARAMETER:
		return CZ_RESULT_BAD_PATH;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W
enum CzResult czWrap_GetFileAttributesExW(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info)
{
	BOOL r = GetFileAttributesExW(path, level, info);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_DYNLINK_FROM_INVALID_RING:
	case ERROR_FORMS_AUTH_REQUIRED:
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_EA_HANDLE:
	case ERROR_INVALID_EA_NAME:
	case ERROR_NO_MORE_ITEMS:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_BAD_DEVICE_PATH:
	case ERROR_BAD_NET_NAME:
	case ERROR_BAD_PATHNAME:
	case ERROR_BUFFER_OVERFLOW:
	case ERROR_DIR_NOT_ROOT:
	case ERROR_DIRECTORY:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_DRIVE:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_NAME:
	case ERROR_INVALID_PARAMETER:
	case ERROR_LABEL_TOO_LONG:
	case ERROR_META_EXPANSION_TOO_LONG:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_SHORT_NAMES_NOT_ENABLED_ON_VOLUME:
		return CZ_RESULT_BAD_PATH;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PATH_BUSY:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_UNIT:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_FILE_NOT_FOUND:
	case ERROR_MOD_NOT_FOUND:
	case ERROR_NETNAME_DELETED:
	case ERROR_PROC_NOT_FOUND:
		return CZ_RESULT_NO_FILE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_FILE_LEVEL_TRIM_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_GET_FILE_SIZE_EX
enum CzResult czWrap_GetFileSizeEx(HANDLE file, PLARGE_INTEGER size)
{
	BOOL r = GetFileSizeEx(file, size);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_DATA:
	case ERROR_NO_MORE_ITEMS:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_CREATE_FILE_W
enum CzResult czWrap_CreateFileW(
	LPHANDLE res,
	LPCWSTR path,
	DWORD desiredAccess,
	DWORD shareMode,
	LPSECURITY_ATTRIBUTES securityAttributes,
	DWORD creationDisposition,
	DWORD flagsAndAttributes,
	HANDLE templateFile)
{
	HANDLE h = CreateFileW(
		path, desiredAccess, shareMode, securityAttributes, creationDisposition, flagsAndAttributes, templateFile);

	if CZ_EXPECT (h != INVALID_HANDLE_VALUE) {
		*res = h;
		return CZ_RESULT_SUCCESS;
	}

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_DYNLINK_FROM_INVALID_RING:
	case ERROR_FORMS_AUTH_REQUIRED:
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_READ_FAULT:
	case ERROR_WRITE_FAULT:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_ALREADY_EXISTS:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_CANNOT_MAKE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_CURRENT_DIRECTORY:
	case ERROR_DIR_NOT_EMPTY:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_EXISTS:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_EA_HANDLE:
	case ERROR_INVALID_EA_NAME:
	case ERROR_NO_MORE_ITEMS:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_OPEN_FAILED:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_BAD_DEVICE_PATH:
	case ERROR_BAD_NET_NAME:
	case ERROR_BAD_PATHNAME:
	case ERROR_BUFFER_OVERFLOW:
	case ERROR_DIR_NOT_ROOT:
	case ERROR_DIRECTORY:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_DRIVE:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_NAME:
	case ERROR_INVALID_PARAMETER:
	case ERROR_LABEL_TOO_LONG:
	case ERROR_META_EXPANSION_TOO_LONG:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_SHORT_NAMES_NOT_ENABLED_ON_VOLUME:
		return CZ_RESULT_BAD_PATH;
	case ERROR_BUSY:
	case ERROR_DELETE_PENDING:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PATH_BUSY:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_BAD_NETPATH:
	case ERROR_BAD_UNIT:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_FILE_NOT_FOUND:
	case ERROR_HANDLE_EOF:
	case ERROR_MOD_NOT_FOUND:
	case ERROR_NETNAME_DELETED:
	case ERROR_PROC_NOT_FOUND:
		return CZ_RESULT_NO_FILE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_NO_MORE_SEARCH_HANDLES:
	case ERROR_SHARING_BUFFER_EXCEEDED:
	case ERROR_TOO_MANY_DESCRIPTORS:
	case ERROR_TOO_MANY_MODULES:
	case ERROR_TOO_MANY_OPEN_FILES:
		return CZ_RESULT_NO_OPEN;
	case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED:
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_FILE_LEVEL_TRIM_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_CLOSE_HANDLE
enum CzResult czWrap_CloseHandle(HANDLE handle)
{
	BOOL r = CloseHandle(handle);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCKED:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
		return CZ_RESULT_IN_USE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_SET_END_OF_FILE
enum CzResult czWrap_SetEndOfFile(HANDLE file)
{
	BOOL r = SetEndOfFile(file);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_DATA:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_SET_FILE_POINTER_EX
enum CzResult czWrap_SetFilePointerEx(
	HANDLE file, LARGE_INTEGER distanceToMove, PLARGE_INTEGER newFilePointer, DWORD moveMethod)
{
	BOOL r = SetFilePointerEx(file, distanceToMove, newFilePointer, moveMethod);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_HANDLE_EOF:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_PARAMETER:
	case ERROR_NEGATIVE_SEEK:
	case ERROR_OFFSET_ALIGNMENT_VIOLATION:
		return CZ_RESULT_BAD_OFFSET;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_READ_FILE
enum CzResult czWrap_ReadFile(
	HANDLE file, LPVOID buffer, DWORD numberOfBytesToRead, LPDWORD numberOfBytesRead, LPOVERLAPPED overlapped)
{
	SetLastError(ERROR_SUCCESS);
	BOOL r = ReadFile(file, buffer, numberOfBytesToRead, numberOfBytesRead, overlapped);
	DWORD err = GetLastError();
	if CZ_EXPECT (r && !err)
		return CZ_RESULT_SUCCESS;

	if (err == ERROR_HANDLE_EOF) {
		if (!overlapped)
			return CZ_RESULT_NO_FILE;

		ULARGE_INTEGER offset;
		offset.LowPart = overlapped->Offset;
		offset.HighPart = overlapped->OffsetHigh;
		return offset.QuadPart ? CZ_RESULT_BAD_OFFSET : CZ_RESULT_NO_FILE;
	}

	switch (err) {
	case ERROR_IO_PENDING:
	case ERROR_MORE_DATA:
		return CZ_RESULT_SUCCESS;
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_DATA:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_PARAMETER:
	case ERROR_NEGATIVE_SEEK:
	case ERROR_OFFSET_ALIGNMENT_VIOLATION:
		return CZ_RESULT_BAD_OFFSET;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_OPERATION_ABORTED:
		return CZ_RESULT_INTERRUPT;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED:
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_WRITE_FILE
enum CzResult czWrap_WriteFile(
	HANDLE file, LPCVOID buffer, DWORD numberOfBytesToWrite, LPDWORD numberOfBytesWritten, LPOVERLAPPED overlapped)
{
	SetLastError(ERROR_SUCCESS);
	BOOL r = WriteFile(file, buffer, numberOfBytesToWrite, numberOfBytesWritten, overlapped);
	DWORD err = GetLastError();
	if CZ_EXPECT (r && !err)
		return CZ_RESULT_SUCCESS;

	switch (err) {
	case ERROR_IO_PENDING:
		return CZ_RESULT_SUCCESS;
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_PARAMETER:
	case ERROR_NEGATIVE_SEEK:
	case ERROR_OFFSET_ALIGNMENT_VIOLATION:
		return CZ_RESULT_BAD_OFFSET;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_OPERATION_ABORTED:
		return CZ_RESULT_INTERRUPT;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED:
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_DELETE_FILE_W
enum CzResult czWrap_DeleteFileW(LPCWSTR path)
{
	BOOL r = DeleteFileW(path);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_EA_NAME:
	case ERROR_MORE_DATA:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_ARGUMENTS:
	case ERROR_BAD_DEVICE_PATH:
	case ERROR_BAD_NET_NAME:
	case ERROR_BAD_PATHNAME:
	case ERROR_BUFFER_OVERFLOW:
	case ERROR_DIR_NOT_ROOT:
	case ERROR_DIRECTORY:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_NAME:
	case ERROR_INVALID_PARAMETER:
	case ERROR_LABEL_TOO_LONG:
	case ERROR_META_EXPANSION_TOO_LONG:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_SHORT_NAMES_NOT_ENABLED_ON_VOLUME:
		return CZ_RESULT_BAD_PATH;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PATH_BUSY:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_BAD_NETPATH:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_FILE_NOT_FOUND:
	case ERROR_MOD_NOT_FOUND:
	case ERROR_NETNAME_DELETED:
	case ERROR_PROC_NOT_FOUND:
		return CZ_RESULT_NO_FILE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
	case ERROR_OUT_OF_STRUCTURES:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_CREATE_FILE_MAPPING_W
enum CzResult czWrap_CreateFileMappingW(
	LPHANDLE res,
	HANDLE file,
	LPSECURITY_ATTRIBUTES fileMappingAttributes,
	DWORD protect,
	DWORD maximumSizeHigh,
	DWORD maximumSizeLow,
	LPCWSTR name)
{
	HANDLE h = CreateFileMappingW(file, fileMappingAttributes, protect, maximumSizeHigh, maximumSizeLow, name);
	if CZ_EXPECT (h) {
		*res = h;
		return CZ_RESULT_SUCCESS;
	}

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BAD_UNIT:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED;
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT;
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_INVALID:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_HANDLE:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_MAP_VIEW_OF_FILE
enum CzResult czWrap_MapViewOfFile(
	LPVOID* res,
	HANDLE fileMappingObject,
	DWORD desiredAccess,
	DWORD fileOffsetHigh,
	DWORD fileOffsetLow,
	SIZE_T numberOfBytesToMap)
{
	LPVOID p = MapViewOfFile(fileMappingObject, desiredAccess, fileOffsetHigh, fileOffsetLow, numberOfBytesToMap);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_BAD_UNIT:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_OFFSET_ALIGNMENT_VIOLATION:
		return CZ_RESULT_BAD_OFFSET;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_UNMAP_VIEW_OF_FILE
enum CzResult czWrap_UnmapViewOfFile(LPCVOID baseAddress)
{
	BOOL r = UnmapViewOfFile(baseAddress);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_BAD_ARGUMENTS:
	case ERROR_HANDLE_EOF:
	case ERROR_INVALID_ADDRESS:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_PARAMETER:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_FLUSH_VIEW_OF_FILE
enum CzResult czWrap_FlushViewOfFile(LPCVOID baseAddress, SIZE_T numberOfBytesToFlush)
{
	BOOL r = FlushViewOfFile(baseAddress, numberOfBytesToFlush);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_BAD_ARGUMENTS:
	case ERROR_HANDLE_EOF:
	case ERROR_INVALID_ADDRESS:
	case ERROR_INVALID_FIELD_IN_PARAMETER_LIST:
	case ERROR_INVALID_PARAMETER:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_FLUSH_FILE_BUFFERS
enum CzResult czWrap_FlushFileBuffers(HANDLE file)
{
	BOOL r = FlushFileBuffers(file);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_DEVICE_IO_CONTROL
enum CzResult czWrap_DeviceIoControl(
	HANDLE device,
	DWORD ioControlCode,
	LPVOID inBuffer,
	DWORD inBufferSize,
	LPVOID outBuffer,
	DWORD outBufferSize,
	LPDWORD bytesReturned,
	LPOVERLAPPED overlapped)
{
	BOOL r = DeviceIoControl(
		device, ioControlCode, inBuffer, inBufferSize, outBuffer, outBufferSize, bytesReturned, overlapped);

	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_IO_PENDING:
		return CZ_RESULT_SUCCESS;
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_INVALID_ADDRESS:
		return CZ_RESULT_BAD_ADDRESS;
	case ERROR_ALREADY_ASSIGNED:
	case ERROR_BAD_DEV_TYPE:
	case ERROR_BAD_FILE_TYPE:
	case ERROR_BAD_PIPE:
	case ERROR_BROKEN_PIPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_CURRENT_DIRECTORY:
	case ERROR_DEV_NOT_EXIST:
	case ERROR_DEVICE_UNREACHABLE:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_EA_FILE_CORRUPT:
	case ERROR_EA_LIST_INCONSISTENT:
	case ERROR_EA_TABLE_FULL:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_INVALID_DATA:
	case ERROR_INVALID_EA_NAME:
	case ERROR_NO_MORE_ITEMS:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_PIPE_LOCAL:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
	case ERROR_SEEK_ON_DEVICE:
	case ERROR_VIRUS_DELETED:
	case ERROR_VIRUS_INFECTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_EAS_DIDNT_FIT:
	case ERROR_INSUFFICIENT_BUFFER:
	case ERROR_MORE_DATA:
		return CZ_RESULT_BAD_SIZE;
	case ERROR_BUSY:
	case ERROR_DRIVE_LOCKED:
	case ERROR_FILE_CHECKED_OUT:
	case ERROR_LOCK_VIOLATION:
	case ERROR_LOCKED:
	case ERROR_NETWORK_BUSY:
	case ERROR_NOT_READY:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_PIPE_BUSY:
	case ERROR_REDIR_PAUSED:
	case ERROR_SHARING_PAUSED:
	case ERROR_SHARING_VIOLATION:
		return CZ_RESULT_IN_USE;
	case ERROR_NO_DATA:
	case ERROR_PIPE_NOT_CONNECTED:
	case ERROR_REQ_NOT_ACCEP:
	case ERROR_VC_DISCONNECTED:
		return CZ_RESULT_NO_CONNECTION;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_DISK_TOO_FRAGMENTED:
	case ERROR_HANDLE_DISK_FULL:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	case ERROR_ATOMIC_LOCKS_NOT_SUPPORTED:
	case ERROR_BAD_COMMAND:
	case ERROR_BAD_DRIVER_LEVEL:
	case ERROR_BAD_NET_RESP:
	case ERROR_CALL_NOT_IMPLEMENTED:
	case ERROR_DEVICE_FEATURE_NOT_SUPPORTED:
	case ERROR_DEVICE_SUPPORT_IN_PROGRESS:
	case ERROR_EAS_NOT_SUPPORTED:
	case ERROR_FILE_LEVEL_TRIM_NOT_SUPPORTED:
	case ERROR_NOT_REDUNDANT_STORAGE:
	case ERROR_NOT_SUPPORTED:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_SYSCONF
enum CzResult czWrap_sysconf(long* res, int name)
{
	errno = 0;
	long r = sysconf(name);
	if CZ_EXPECT (!errno) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_XOPEN_VERSION >= CZ_XPG_1989
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_NO_SUPPORT;
#endif
}
#endif

enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength)
{
	int r = wai_getExecutablePath(out, capacity, dirnameLength);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1)
		return CZ_RESULT_SUCCESS;
	return CZ_RESULT_INTERNAL_ERROR;
}
