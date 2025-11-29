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

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
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

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
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

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
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
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
}

#if CZ_WRAP_REALLOCARRAY
enum CzResult czWrap_reallocarray(void* restrict* res, void* ptr, size_t count, size_t size)
{
	void* p = reallocarray(ptr, count, size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "reallocarray failed with ptr 0x%016" PRIxPTR ", count %zu, size %zu (%.3fms)",
		(uintptr_t) ptr, count, size, t);

#if CZ_GNU_LINUX || CZ_FREE_BSD
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
#else
	if (!count)
		return CZ_RESULT_BAD_SIZE;
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
}
#endif

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

#if CZ_DARWIN || CZ_FREE_BSD
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

#if CZ_WRAP_ALIGNED_ALLOC
enum CzResult czWrap_aligned_alloc(void* restrict* res, size_t alignment, size_t size)
{
	void* p = aligned_alloc(alignment, size);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "aligned_alloc failed with alignment %zu, size %zu (%.3fms)", alignment, size, t);

#if CZ_DARWIN
	switch (errno) {
	case EINVAL:
		if (alignment < sizeof(void*))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (alignment & (alignment - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_SIZE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX || CZ_FREE_BSD
	switch (errno) {
	case EINVAL:
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!alignment)
		return CZ_RESULT_BAD_ALIGNMENT;
	if (alignment & (alignment - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	if (size & (alignment - 1))
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
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

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD || CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (r) {
	case EINVAL:
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (alignment < sizeof(void*))
		return CZ_RESULT_BAD_ALIGNMENT;
	if (alignment & (alignment - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_NO_MEMORY;
#endif
}
#endif

#if CZ_WRAP_MADVISE
enum CzResult czWrap_madvise(void* addr, size_t size, int advice)
{
	int r = madvise(addr, size, advice);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if CZ_GNU_LINUX
	long pageSize = sysconf(_SC_PAGESIZE);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
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
	switch (err) {
	case EACCES:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;

		switch (advice) {
#if defined(MADV_MERGEABLE)
		case MADV_MERGEABLE:
			return CZ_RESULT_NO_SUPPORT;
#endif
#if defined(MADV_UNMERGEABLE)
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
#if defined(MADV_SOFT_OFFLINE)
		case MADV_SOFT_OFFLINE:
			return CZ_RESULT_IN_USE;
#endif
#if defined(MADV_COLLAPSE)
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
#if defined(MADV_COLLAPSE)
		case MADV_COLLAPSE:
			return CZ_RESULT_NO_MEMORY;
#endif
		default:
			return CZ_RESULT_BAD_ADDRESS;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		return CZ_RESULT_NO_SUPPORT;
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

#if !CZ_DARWIN && !CZ_FREE_BSD
	int err = errno;
	long pageSize = sysconf(_SC_PAGESIZE);
	errno = err;
#endif

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
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (r) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (r) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
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
	case EDEADLK:
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
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
		if (mode[0] == 'r')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case ECAPMODE:
	case ENOTCAPABLE:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EEXIST:
	case EINTEGRITY:
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EEXIST:
	case EISDIR:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EEXIST:
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
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

#if CZ_WRAP_FDOPEN
enum CzResult czWrap_fdopen(FILE* restrict* res, int fd, const char* mode)
{
	FILE* s = fdopen(fd, mode);
	if CZ_EXPECT (s) {
		*res = s;
		return CZ_RESULT_SUCCESS;
	}

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
	case EINVAL:
	case ENOTTY:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax != -1 && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	if (mode[0] == 'r')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'w')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'a')
		return CZ_RESULT_INTERNAL_ERROR;
	return CZ_RESULT_BAD_ACCESS;
#endif
}
#endif

enum CzResult czWrap_freopen(const char* path, const char* mode, FILE* stream)
{
	FILE* s = freopen(path, mode, stream);
	if CZ_EXPECT (s)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EINVAL:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EEXIST:
	case EFBIG:
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
	case ETXTBSY:
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
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
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
	case EEXIST:
	case EFBIG:
	case EISDIR:
	case ENODEV:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (mode[0] == 'r')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
	case EBUSY:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case ECAPMODE:
	case ENOTCAPABLE:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EEXIST:
	case EFBIG:
	case EINTEGRITY:
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
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
	case EBADF:
	case EINVAL:
	case EROFS:
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
	case ENOENT:
		if (path && !path[0])
			return CZ_RESULT_BAD_PATH;
		if (path && mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (path && mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_ACCESS;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EBADF:
	case EINVAL:
	case EROFS:
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
	case ENOENT:
		if (path && !path[0])
			return CZ_RESULT_BAD_PATH;
		if (path && mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (path && mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_ACCESS;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EEXIST:
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (path && !path[0])
			return CZ_RESULT_BAD_PATH;
		if (path && mode[0] == 'w')
			return CZ_RESULT_BAD_PATH;
		if (path && mode[0] == 'a')
			return CZ_RESULT_BAD_PATH;
		if (mode[0] == 'r')
			return CZ_RESULT_NO_FILE;
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (path && !path[0])
		return CZ_RESULT_BAD_PATH;
	if (path && mode[0] == 'r')
		return CZ_RESULT_NO_FILE;
	if (mode[0] == 'r')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'w')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'a')
		return CZ_RESULT_INTERNAL_ERROR;
	return CZ_RESULT_BAD_ACCESS;
#endif
}

#if CZ_WRAP_FMEMOPEN
enum CzResult czWrap_fmemopen(FILE* restrict* res, void* buffer, size_t size, const char* mode)
{
	FILE* s = fmemopen(buffer, size, mode);
	if CZ_EXPECT (s) {
		*res = s;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
	switch (errno) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (errno) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	if (mode[0] == 'r')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'w')
		return CZ_RESULT_INTERNAL_ERROR;
	if (mode[0] == 'a')
		return CZ_RESULT_INTERNAL_ERROR;
	return CZ_RESULT_BAD_ACCESS;
#endif
}
#endif

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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
		return CZ_RESULT_NO_DISK;
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
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
	case ECONNRESET:
	case EDEADLK:
	case EFBIG:
	case ENETDOWN:
	case ENETUNREACH:
	case ENXIO:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EDESTADDRREQ:
	case EFBIG:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_BAD_OFFSET;
#endif
}

#if CZ_WRAP_FSEEKO
enum CzResult czWrap_fseeko(FILE* stream, off_t offset, int whence)
{
	int r = fseeko(stream, offset, whence);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
	case ECONNRESET:
	case EDEADLK:
	case EFBIG:
	case ENETDOWN:
	case ENETUNREACH:
	case ENXIO:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EDESTADDRREQ:
	case EFBIG:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_BAD_OFFSET;
#endif
}
#endif

enum CzResult czWrap_ftell(long* res, FILE* stream)
{
	long r = ftell(stream);
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case ECONNRESET:
	case EDEADLK:
	case EFBIG:
	case ENETDOWN:
	case ENETUNREACH:
	case ENXIO:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EDESTADDRREQ:
	case EFBIG:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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

#if CZ_WRAP_FTELLO
enum CzResult czWrap_ftello(off_t* res, FILE* stream)
{
	off_t r = ftell(stream);
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case ECONNRESET:
	case EDEADLK:
	case EFBIG:
	case ENETDOWN:
	case ENETUNREACH:
	case ENXIO:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EDESTADDRREQ:
	case EFBIG:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
#endif

enum CzResult czWrap_fgetpos(FILE* stream, fpos_t* pos)
{
	int r = fgetpos(stream, pos);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDESTADDRREQ:
	case EFBIG:
	case EOVERFLOW:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
#elif CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fsetpos(FILE* stream, const fpos_t* pos)
{
	int r = fsetpos(stream, pos);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
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
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDESTADDRREQ:
	case EFBIG:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EFBIG:
	case ENXIO:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_rewind(FILE* stream)
{
#if CZ_POSIX_VERSION >= CZ_POSIX_1988
	errno = 0;
#endif
	rewind(stream);

#if CZ_DARWIN
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case ECONNRESET:
	case EDEADLK:
	case EFBIG:
	case ENETDOWN:
	case ENETUNREACH:
	case ENXIO:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case EFBIG:
	case ENXIO:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	long pos = ftell(stream);
	if CZ_EXPECT (!pos)
		return CZ_RESULT_SUCCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fread(size_t* res, void* buffer, size_t size, size_t count, FILE* stream)
{
	clearerr(stream);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	clearerr(stream);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fflush(FILE* stream)
{
	int r = fflush(stream);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
		return CZ_RESULT_NO_DISK;
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINTEGRITY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
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
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_NO_FILE;
#endif
}

#if CZ_WRAP_RMDIR
enum CzResult czWrap_rmdir(const char* path)
{
	int r = rmdir(path);
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
	case ENOTDIR:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
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
	case ENOTDIR:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case ENOTDIR:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
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
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_UNLINK
enum CzResult czWrap_unlink(const char* path)
{
	int r = unlink(path);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
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
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_UNLINKAT
enum CzResult czWrap_unlinkat(int fd, const char* path, int flags)
{
	int r = unlinkat(fd, path, flags);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2008
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOTCAPABLE:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & AT_REMOVEDIR)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EACCES:
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		size_t pathLen = strlen(path);
		if (!pathLen)
			return CZ_RESULT_BAD_PATH;
		if (path[pathLen - 1] != '.')
			return CZ_RESULT_BAD_ACCESS;
		if (pathLen == 1)
			return CZ_RESULT_BAD_PATH;
		if (path[pathLen - 2] != '/')
			return CZ_RESULT_BAD_ACCESS;
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EISDIR:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case ENOTCAPABLE:
		if (flags & AT_RESOLVE_BENEATH)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & AT_REMOVEDIR)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (fd == AT_FDCWD)
		return CZ_RESULT_NO_FILE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_NO_FILE;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_FILENO
enum CzResult czWrap_fileno(int* res, FILE* stream)
{
	int fd = fileno(stream);

#if CZ_DARWIN || CZ_WIN32
	*res = fd;
	return CZ_RESULT_SUCCESS;
#elif CZ_GNU_LINUX || CZ_FREE_BSD || CZ_POSIX_VERSION >= CZ_POSIX_1988
	if CZ_EXPECT (fd != -1) {
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
	if CZ_EXPECT (fd != -1) {
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

#if CZ_WIN32
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
#elif CZ_DARWIN || CZ_FREE_BSD || CZ_POSIX_VERSION >= CZ_POSIX_2001
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
	int err = errno;
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;

	if CZ_NOEXPECT (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if CZ_NOEXPECT (openMax != -1 && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;

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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_LSTAT
enum CzResult czWrap_lstat(const char* path, struct stat* st)
{
	int r = lstat(path, st);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
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

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_1988
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
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
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax != -1 && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FSTATAT
enum CzResult czWrap_fstatat(int fd, const char* path, struct stat* st, int flag)
{
	int r = fstatat(fd, path, st, flag);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2008
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOTCAPABLE:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
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
		if (path[0])
			return CZ_RESULT_NO_FILE;
#if CZ_GNU_SOURCE
		if (flag & AT_EMPTY_PATH)
			return CZ_RESULT_NO_FILE;
#endif
		return CZ_RESULT_BAD_PATH;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTCAPABLE:
		if (flags & AT_RESOLVE_BENEATH)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (fd == AT_FDCWD)
		return CZ_RESULT_NO_FILE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_NO_FILE;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_NO_FILE;
#endif
}
#endif

#if CZ_WRAP_FLOCK
enum CzResult czWrap_flock(int fd, int op)
{
	int r = flock(fd, op);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
	case ENOTSUP:
		return CZ_RESULT_BAD_FILE;
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EINVAL:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_LOCKF
enum CzResult czWrap_lockf(int fd, int func, off_t size)
{
	int r = lockf(fd, func, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD
	off_t pos = lseek(fd, 0, SEEK_CUR);
	int posErr = (pos == -1) ? errno : 0;
	errno = err;
#endif
#if !CZ_DARWIN &&                         \
	!CZ_GNU_LINUX &&                      \
	!CZ_FREE_BSD &&                       \
	CZ_POSIX_VERSION < CZ_POSIX_2001 &&   \
	CZ_XOPEN_VERSION < CZ_SUS_1997 &&     \
	(                                     \
		CZ_XOPEN_VERSION < CZ_SUS_1994 || \
		CZ_XOPEN_UNIX <= 0)

	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (func == F_LOCK)
			return CZ_RESULT_BAD_FILE;
		if (func == F_TEST)
			return CZ_RESULT_BAD_FILE;
		if (func == F_TLOCK)
			return CZ_RESULT_BAD_FILE;
		if (func == F_ULOCK)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_NO_SUPPORT;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EINVAL:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (func == F_LOCK)
			return CZ_RESULT_BAD_FILE;
		if (func == F_TEST)
			return CZ_RESULT_BAD_FILE;
		if (func == F_TLOCK)
			return CZ_RESULT_BAD_FILE;
		if (func == F_ULOCK)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_NO_SUPPORT;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif (                                    \
		CZ_XOPEN_VERSION >= CZ_SUS_1994 && \
		CZ_XOPEN_UNIX > 0) ||              \
	CZ_XOPEN_VERSION >= CZ_SUS_1997 ||     \
	CZ_POSIX_VERSION >= CZ_POSIX_2001

	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
	case EINVAL:
		if (!posErr && size < 0 && pos + size < 0)
			return CZ_RESULT_BAD_SIZE;
		if (func == F_LOCK)
			return CZ_RESULT_BAD_FILE;
		if (func == F_TEST)
			return CZ_RESULT_BAD_FILE;
		if (func == F_TLOCK)
			return CZ_RESULT_BAD_FILE;
		if (func == F_ULOCK)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_NO_SUPPORT;
	case EDEADLK:
		if (func == F_LOCK)
			return CZ_RESULT_DEADLOCK;
		return CZ_RESULT_NO_LOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax != -1 && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	if (posErr)
		return CZ_RESULT_BAD_FILE;
	if (size < 0 && pos + size < 0)
		return CZ_RESULT_BAD_SIZE;
	if (func == F_LOCK)
		return CZ_RESULT_INTERNAL_ERROR;
	if (func == F_TEST)
		return CZ_RESULT_INTERNAL_ERROR;
	if (func == F_TLOCK)
		return CZ_RESULT_INTERNAL_ERROR;
	if (func == F_ULOCK)
		return CZ_RESULT_INTERNAL_ERROR;
	return CZ_RESULT_NO_SUPPORT;
#endif
}
#endif

#if CZ_WRAP_FCNTL
enum CzResult czWrap_fcntl(int* res, int fd, int cmd, ...)
{
	int r = -1;
	int intArg = 0;
	struct flock* lockArg = NULL;
#if CZ_DARWIN && CZ_DARWIN_C_SOURCE
	char* pathArg = NULL;
#endif
#if CZ_DARWIN && CZ_DARWIN_C_SOURCE
	off_t* offArg = NULL;
#endif
#if CZ_DARWIN && CZ_DARWIN_C_SOURCE
	struct fstore* storeArg = NULL;
#endif
#if CZ_DARWIN && CZ_DARWIN_C_SOURCE
	struct fpunchhole* holeArg = NULL;
#endif
#if CZ_DARWIN && CZ_DARWIN_C_SOURCE
	struct radvisory* readAdviceArg = NULL;
#endif
#if CZ_DARWIN && CZ_DARWIN_C_SOURCE
	struct log2phys* logPhysArg = NULL;
#endif
#if CZ_GNU_LINUX && CZ_GNU_SOURCE
	CzU64* hintArg = NULL;
#endif
#if CZ_FREE_BSD
	struct kinfo_file* kinfoArg = NULL;
#endif
#if (CZ_GNU_LINUX && CZ_GNU_SOURCE) || CZ_POSIX_VERSION >= CZ_POSIX_2024
	struct f_owner_ex* ownerArg = NULL;
#endif

	va_list vargs;
	va_start(vargs, cmd);

#if CZ_DARWIN
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
		r = fcntl(fd, cmd);
		break;
#if CZ_DARWIN_C_SOURCE
	case F_BARRIERFSYNC:
	case F_FULLFSYNC:
	case F_GETNOSIGPIPE:
		r = fcntl(fd, cmd);
		break;
#endif
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#if CZ_DARWIN_C_SOURCE || CZ_POSIX_C_SOURCE >= CZ_POSIX_2008
	case F_DUPFD_CLOEXEC:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_NOCACHE:
	case F_RDAHEAD:
	case F_SETNOSIGPIPE:
	case F_TRANSFEREXTENTS:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#endif
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
#if CZ_DARWIN_C_SOURCE
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_GETPATH:
	case F_GETPATH_NOFIRMLINK:
		pathArg = va_arg(vargs, char*);
		r = fcntl(fd, cmd, pathArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_SETSIZE:
		offArg = va_arg(vargs, off_t*);
		r = fcntl(fd, cmd, offArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_PREALLOCATE:
		storeArg = va_arg(vargs, struct fstore*);
		r = fcntl(fd, cmd, storeArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_PUNCHHOLE:
		holeArg = va_arg(vargs, struct fpunchhole*);
		r = fcntl(fd, cmd, holeArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_RDADVISE:
		readAdviceArg = va_arg(vargs, struct radvisory*);
		r = fcntl(fd, cmd, readAdviceArg);
		break;
#endif
#if CZ_DARWIN_C_SOURCE
	case F_LOG2PHYS:
	case F_LOG2PHYS_EXT:
		logPhysArg = va_arg(vargs, struct log2phys*);
		r = fcntl(fd, cmd, logPhysArg);
		break;
#endif
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
		r = fcntl(fd, cmd);
		break;
#if CZ_GNU_SOURCE
	case F_GET_SEALS:
	case F_GETLEASE:
	case F_GETPIPE_SZ:
	case F_GETSIG:
		r = fcntl(fd, cmd);
		break;
#endif
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_1997
	case F_GETOWN:
		r = fcntl(fd, cmd);
		break;
#endif
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#if CZ_GNU_SOURCE
	case F_ADD_SEALS:
	case F_NOTIFY:
	case F_SETLEASE:
	case F_SETPIPE_SZ:
	case F_SETSIG:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#endif
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_2008
	case F_DUPFD_CLOEXEC:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#endif
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_1997
	case F_SETOWN:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#endif
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
#if CZ_GNU_SOURCE
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
#endif
#if CZ_GNU_SOURCE
	case F_GET_RW_HINT:
	case F_SET_RW_HINT:
	case F_GET_FILE_RW_HINT:
	case F_SET_FILE_RW_HINT:
		hintArg = va_arg(vargs, CzU64*);
		r = fcntl(fd, cmd, hintArg);
		break;
#endif
#if CZ_GNU_SOURCE
	case F_GETOWN_EX:
	case F_SETOWN_EX:
		ownerArg = va_arg(vargs, struct f_owner_ex*);
		r = fcntl(fd, cmd, ownerArg);
		break;
#endif
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (cmd) {
	case F_GET_SEALS:
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
	case F_ISUNIONSTACK:
		r = fcntl(fd, cmd);
		break;
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
	case F_READAHEAD:
	case F_RDAHEAD:
	case F_SET_SEALS:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#if CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(7, 1)
	case F_DUP2FD:
	case F_DUP2FD_CLOEXEC:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
#endif
	case F_KINFO:
		kinfoArg = va_arg(vargs, struct kinfo_file*);
		r = fcntl(fd, cmd, kinfoArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
		r = fcntl(fd, cmd);
		break;
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
	case F_DUPFD_CLOFORK:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
	case F_GETOWN_EX:
	case F_SETOWN_EX:
		ownerArg = va_arg(vargs, struct f_owner_ex*);
		r = fcntl(fd, cmd, ownerArg);
		break;
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
		r = fcntl(fd, cmd);
		break;
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
	case F_GETOWN:
		r = fcntl(fd, cmd);
		break;
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
		r = fcntl(fd, cmd);
		break;
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
		intArg = va_arg(vargs, int);
		r = fcntl(fd, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fd, cmd, lockArg);
		break;
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#endif

	va_end(vargs);
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

	int err = errno;
#if !CZ_GNU_LINUX && !CZ_FREE_BSD
	off_t pos = lseek(fd, 0, SEEK_CUR);
	int posErr = (pos == -1) ? errno : 0;
	errno = err;
#endif
#if !CZ_GNU_LINUX && !CZ_FREE_BSD
	struct stat st;
	int stRes = fstat(fd, &st);
	int stErr = (stRes == -1) ? errno : 0;
	errno = err;
#endif
#if CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(7, 1)
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EACCES:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EXDEV:
		return CZ_RESULT_BAD_FILE;
	case EFBIG:
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	case EBADF:
		switch (cmd) {
#if CZ_DARWIN_C_SOURCE
		case F_TRANSFEREXTENTS:
			if (stErr)
				return CZ_RESULT_BAD_ACCESS;
			if (!S_ISREG(st.st_mode))
				return CZ_RESULT_BAD_FILE;
			return CZ_RESULT_BAD_ACCESS;
#endif
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
	case EINVAL:
		switch (cmd) {
#if CZ_DARWIN_C_SOURCE
		case F_PUNCHHOLE:
			if (holeArg->fp_flags)
				return CZ_RESULT_BAD_ACCESS;
			if (holeArg->reserved)
				return CZ_RESULT_BAD_ACCESS;
			if (holeArg->fp_offset < 0)
				return CZ_RESULT_BAD_OFFSET;
			if (holeArg->fp_length < 0)
				return CZ_RESULT_BAD_SIZE;
			return CZ_RESULT_BAD_ALIGNMENT;
#endif
#if CZ_DARWIN_C_SOURCE
		case F_TRANSFEREXTENTS:
			if (intArg < 0)
				return CZ_RESULT_BAD_ACCESS;
			return CZ_RESULT_BAD_FILE;
#endif
#if CZ_DARWIN_C_SOURCE
		case F_PREALLOCATE:
			return CZ_RESULT_BAD_OFFSET;
#endif
		case F_DUPFD:
			return CZ_RESULT_NO_OPEN;
#if CZ_DARWIN_C_SOURCE || CZ_POSIX_C_SOURCE >= CZ_POSIX_2008
		case F_DUPFD_CLOEXEC:
			return CZ_RESULT_NO_OPEN;
#endif
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
#if CZ_DARWIN_C_SOURCE
		case F_OFD_GETLK:
		case F_OFD_SETLK:
		case F_OFD_SETLKW:
#endif
			switch (lockArg->l_whence) {
			case SEEK_SET:
				if (lockArg->l_start < 0)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_CUR:
				if (posErr)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END:
				if (stErr)
					break;
				if (lockArg->l_start < -st.st_size)
					return CZ_RESULT_BAD_OFFSET;
				break;
			default:
				return CZ_RESULT_BAD_OFFSET;
			}
			switch (lockArg->l_type) {
			case F_RDLCK:
			case F_WRLCK:
			case F_UNLCK:
				if (lockArg->l_len < 0)
					return CZ_RESULT_BAD_SIZE;
				return CZ_RESULT_BAD_FILE;
			default:
				return CZ_RESULT_BAD_ACCESS;
			}
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ENOTDIR:
		return CZ_RESULT_BAD_FILE;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case EBUSY:
		switch (cmd) {
#if CZ_GNU_SOURCE
		case F_SETPIPE_SZ:
			return CZ_RESULT_BAD_SIZE;
#endif
#if CZ_GNU_SOURCE
		case F_ADD_SEALS:
			return CZ_RESULT_IN_USE;
#endif
		default:
			return CZ_RESULT_INTERNAL_ERROR;
		}
	case EINVAL:
		switch (cmd) {
#if CZ_GNU_SOURCE
		case F_OFD_GETLK:
		case F_OFD_SETLK:
		case F_OFD_SETLKW:
		case F_SETSIG:
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		case F_ADD_SEALS:
			if (intArg & ~(F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE | F_SEAL_FUTURE_WRITE))
				return CZ_RESULT_BAD_ACCESS;
			return CZ_RESULT_NO_SUPPORT;
#endif
		case F_DUPFD:
			return CZ_RESULT_NO_OPEN;
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_2008
		case F_DUPFD_CLOEXEC:
			return CZ_RESULT_NO_OPEN;
#endif
#if CZ_GNU_SOURCE
		case F_GET_SEALS:
			return CZ_RESULT_NO_SUPPORT;
#endif
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case ENOTTY:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EAGAIN:
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
	case EBADF:
		switch (cmd) {
#if CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(7, 1)
		case F_DUP2FD:
		case F_DUP2FD_CLOEXEC:
			if (intArg < 0)
				return CZ_RESULT_NO_OPEN;
			if (openMax == -1)
				return CZ_RESULT_BAD_ACCESS;
			if (intArg >= openMax)
				return CZ_RESULT_NO_OPEN;
			return CZ_RESULT_BAD_ACCESS;
#endif
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
	case EINVAL:
		switch (cmd) {
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
			return CZ_RESULT_NO_OPEN;
		case F_ADD_SEALS:
		case F_GET_SEALS:
			return CZ_RESULT_NO_SUPPORT;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			switch (lockArg->l_type) {
			case F_RDLCK:
			case F_WRLCK:
			case F_UNLCK:
				return CZ_RESULT_BAD_OFFSET;
			default:
				return CZ_RESULT_BAD_ACCESS;
			}
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
	case EINVAL:
		switch (cmd) {
		case F_SETOWN:
		case F_SETOWN_EX:
			return CZ_RESULT_BAD_ACCESS;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
		case F_DUPFD_CLOFORK:
			return CZ_RESULT_NO_OPEN;
		case F_OFD_GETLK:
		case F_OFD_SETLK:
		case F_OFD_SETLKW:
			if (lockArg->l_pid)
				return CZ_RESULT_BAD_ACCESS;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			switch (lockArg->l_whence) {
			case SEEK_SET:
				if (lockArg->l_start < 0)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_CUR:
				if (posErr)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + pos < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END;
				if (stErr)
					break;
				if (lockArg->l_start < -st.st_size)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + st.st_size < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			default:
				return CZ_RESULT_BAD_OFFSET;
			}
			switch (lockArg->l_type) {
			case F_RDLCK:
			case F_WRLCK:
			case F_UNLCK:
				return CZ_RESULT_BAD_FILE;
			default:
				return CZ_RESULT_BAD_ACCESS;
			}
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
	case EINVAL:
		switch (cmd) {
		case F_SETOWN:
			return CZ_RESULT_BAD_ACCESS;
		case F_DUPFD:
		case F_DUPFD_CLOEXEC:
			return CZ_RESULT_NO_OPEN;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			switch (lockArg->l_whence) {
			case SEEK_SET:
				if (lockArg->l_start < 0)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_CUR:
				if (posErr)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + pos < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END;
				if (stErr)
					break;
				if (lockArg->l_start < -st.st_size)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + st.st_size < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			default:
				return CZ_RESULT_BAD_OFFSET;
			}
			switch (lockArg->l_type) {
			case F_RDLCK:
			case F_WRLCK:
			case F_UNLCK:
				return CZ_RESULT_BAD_FILE;
			default:
				return CZ_RESULT_BAD_ACCESS;
			}
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
	case EINVAL:
		switch (cmd) {
		case F_SETOWN:
			return CZ_RESULT_BAD_ACCESS;
		case F_DUPFD:
			return CZ_RESULT_NO_OPEN;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			switch (lockArg->l_whence) {
			case SEEK_SET:
				if (lockArg->l_start < 0)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_CUR:
				if (posErr)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + pos < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END;
				if (stErr)
					break;
				if (lockArg->l_start < -st.st_size)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + st.st_size < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			default:
				return CZ_RESULT_BAD_OFFSET;
			}
			switch (lockArg->l_type) {
			case F_RDLCK:
			case F_WRLCK:
			case F_UNLCK:
				return CZ_RESULT_BAD_FILE;
			default:
				return CZ_RESULT_BAD_ACCESS;
			}
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EDEADLK:
		return CZ_RESULT_DEADLOCK;
	case EACCES:
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOLCK:
		return CZ_RESULT_NO_LOCK;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
	case EINVAL:
		switch (cmd) {
		case F_DUPFD:
			return CZ_RESULT_NO_OPEN;
		case F_GETLK:
		case F_SETLK:
		case F_SETLKW:
			switch (lockArg->l_whence) {
			case SEEK_SET:
				if (lockArg->l_start < 0)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_CUR:
				if (posErr)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + pos < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END;
				if (stErr)
					break;
				if (lockArg->l_start < -st.st_size)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + st.st_size < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			default:
				return CZ_RESULT_BAD_OFFSET;
			}
			switch (lockArg->l_type) {
			case F_RDLCK:
			case F_WRLCK:
			case F_UNLCK:
				return CZ_RESULT_BAD_FILE;
			default:
				return CZ_RESULT_BAD_ACCESS;
			}
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
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
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
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
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997 || (CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_UNIX > 0)
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
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

	int err = errno;
#if !CZ_DARWIN &&                         \
	!CZ_GNU_LINUX &&                      \
	!CZ_FREE_BSD &&                       \
	CZ_POSIX_VERSION < CZ_POSIX_2001 &&   \
	CZ_XOPEN_VERSION < CZ_SUS_1997 &&     \
	(                                     \
		CZ_XOPEN_VERSION < CZ_SUS_1994 || \
		CZ_XOPEN_UNIX <= 0)

	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EDEADLK:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	switch (err) {
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
#elif CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_UNIX > 0
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	if (size < 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_FADVISE
enum CzResult czWrap_posix_fadvise(int* res, int fd, off_t offset, off_t size, int advice)
{
	int r = posix_fadvise(fd, offset, size, advice);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001
	int err = errno;
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_GNU_LINUX
	switch (r) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (r) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EINTEGRITY:
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (r) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (size < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (r) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (size < 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FALLOCATE
enum CzResult czWrap_fallocate(int fd, int mode, off_t offset, off_t size)
{
	int r = fallocate(fd, mode, offset, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if CZ_GNU_LINUX
	struct stat st;
	int stRes = fstat(fd, &st);
	int stErr = (stRes == -1) ? errno : 0;
	errno = err;
#endif
#if !CZ_GNU_LINUX
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_GNU_LINUX
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size <= 0)
			return CZ_RESULT_BAD_SIZE;
		if (mode & FALLOC_FL_COLLAPSE_RANGE && mode != FALLOC_FL_COLLAPSE_RANGE)
			return CZ_RESULT_BAD_ACCESS;
		if (mode & FALLOC_FL_INSERT_RANGE && mode != FALLOC_FL_INSERT_RANGE)
			return CZ_RESULT_BAD_ACCESS;
		if (stErr)
			return CZ_RESULT_INTERNAL_ERROR;
		if (!S_ISREG(st.st_mode) && mode == FALLOC_FL_COLLAPSE_RANGE)
			return CZ_RESULT_BAD_FILE;
		if (!S_ISREG(st.st_mode) && mode == FALLOC_FL_INSERT_RANGE)
			return CZ_RESULT_BAD_FILE;
		if (!S_ISREG(st.st_mode) && mode == FALLOC_FL_ZERO_RANGE)
			return CZ_RESULT_BAD_FILE;
		if (size >= st.st_size - offset && mode == FALLOC_FL_COLLAPSE_RANGE)
			return CZ_RESULT_BAD_RANGE;
		if (offset >= st.st_size && mode == FALLOC_FL_INSERT_RANGE)
			return CZ_RESLT_BAD_RANGE;
		if (offset & (st.st_blksize - 1) && mode == FALLOC_FL_COLLAPSE_RANGE)
			return CZ_RESULT_BAD_ALIGNMENT;
		if (offset & (st.st_blksize - 1) && mode == FALLOC_FL_INSERT_RANGE)
			return CZ_RESULT_BAD_ALIGNMENT;
		if (size & (st.st_blksize - 1) && mode == FALLOC_FL_COLLAPSE_RANGE)
			return CZ_RESULT_BAD_ALIGNMENT;
		if (size & (st.st_blksize - 1) && mode == FALLOC_FL_INSERT_RANGE)
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_INTERNAL_ERROR;
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOSYS:
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (size <= 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_FALLOCATE
enum CzResult czWrap_posix_fallocate(int* res, int fd, off_t offset, off_t size)
{
	int r = posix_fallocate(fd, offset, size);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001
	int err = errno;
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_GNU_LINUX
	switch (r) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size <= 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_NO_SUPPORT;
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (r) {
	case EBADF:
	case ENOTCAPABLE:
		return CZ_RESULT_BAD_ACCESS;
	case EINTEGRITY:
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size <= 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_NO_SUPPORT;
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (r) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size <= 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_INTERNAL_ERROR;
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (r) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size <= 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_NO_SUPPORT;
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (r) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENODEV:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_INTERNAL_ERROR;
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (size <= 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FSYNC
enum CzResult czWrap_fsync(int fd)
{
	int r = fsync(fd);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001 && CZ_XOPEN_VERSION < CZ_XPG_1989
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EROFS:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINTEGRITY:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_XPG_1989
	switch (err) {
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
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FDATASYNC
enum CzResult czWrap_fdatasync(int fd)
{
	int r = fdatasync(fd);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001 && CZ_XOPEN_VERSION < CZ_SUS_1997
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_GNU_LINUX
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EROFS:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINTEGRITY:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
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
	case EDEADLK:
	case EEXIST:
	case EISDIR:
	case ENOTCAPABLE:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		if (flags & O_SEARCH)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
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
		return CZ_RESULT_NO_DISK;
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
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && !(flags & O_WRONLY))
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && !(flags & O_RDWR))
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_DIRECT)
			return CZ_RESULT_NO_SUPPORT;
#endif
		if (flags & O_CREAT && flags & O_DIRECTORY)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_PATH;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_WRONLY)
			return CZ_RESULT_NO_SUPPORT;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_RDWR)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_NO_FILE;
	case EBUSY:
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	case EISDIR:
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_WRONLY)
			return CZ_RESULT_NO_SUPPORT;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_RDWR)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case ECAPMODE:
	case ENOTCAPABLE:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EEXIST:
	case EINTEGRITY:
	case EISDIR:
	case EMLINK:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & O_DSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_RSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
		if (flags & O_DSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_RSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
		if (flags & O_DSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_RSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	case EINVAL:
		if (flags & O_DSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_RSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_OPENAT
enum CzResult czWrap_openat(int* res, int fd, const char* path, int flags, mode_t mode)
{
	int f = openat(fd, path, flags, mode);
	if CZ_EXPECT (f != -1) {
		*res = f;
		return CZ_RESULT_SUCCESS;
	}

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2008
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EACCES:
	case EBADF:
	case EINVAL:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EEXIST:
	case EISDIR:
	case ENOTCAPABLE:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		if (flags & O_SEARCH)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
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
		return CZ_RESULT_NO_DISK;
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
	switch (err) {
	case EACCES:
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && !(flags & O_WRONLY))
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && !(flags & O_RDWR))
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_DIRECT)
			return CZ_RESULT_NO_SUPPORT;
#endif
		if (flags & O_CREAT && flags & O_DIRECTORY)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_PATH;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_WRONLY)
			return CZ_RESULT_NO_SUPPORT;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_RDWR)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_NO_FILE;
	case EBUSY:
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	case EISDIR:
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_WRONLY)
			return CZ_RESULT_NO_SUPPORT;
#endif
#if CZ_GNU_SOURCE
		if (flags & O_TMPFILE && flags & O_RDWR)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EACCES:
	case EBADF:
	case ECAPMODE:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOTCAPABLE:
		if (!(flags & O_RESOLVE_BENEATH))
			return CZ_RESULT_BAD_ACCESS;
		if (path[0] == '/')
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_FILE;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EEXIST:
	case EINTEGRITY:
	case EISDIR:
	case EMLINK:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (flags & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EACCES:
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & O_DSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_RSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (err) {
	case EACCES:
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & O_DSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_RSYNC)
			return CZ_RESULT_NO_SUPPORT;
		if (flags & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
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
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (flags & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (!(flags & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
		return CZ_RESULT_BAD_ACCESS;
	if (fd == AT_FDCWD)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_CREAT
enum CzResult czWrap_creat(int* res, const char* path, mode_t mode)
{
	int f = creat(path, mode);
	if CZ_EXPECT (f != -1) {
		*res = f;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EISDIR:
	case ENODEV:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case ECAPMODE:
	case ENOTCAPABLE:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
	case ENXIO:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
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
	case EISDIR:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
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

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_1988
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EINPROGRESS:
		return CZ_RESULT_SUCCESS;
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
#if POSIX_CLOSE_RESTART
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_CLOSE
enum CzResult czWrap_posix_close(int fd, int flag)
{
	int r = posix_close(fd, flag);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if CZ_POSIX_VERSION < CZ_POSIX_2024
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EINPROGRESS:
		return CZ_RESULT_SUCCESS;
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_LSEEK
enum CzResult czWrap_lseek(off_t* res, int fd, off_t offset, int whence)
{
#if CZ_POSIX_VERSION < CZ_POSIX_1996
	errno = 0;
#endif
	off_t r = lseek(fd, offset, whence);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1)
		return CZ_RESULT_SUCCESS;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_1988
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD || CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_READ
enum CzResult czWrap_read(ssize_t* res, int fd, void* buffer, size_t size)
{
	ssize_t r = read(fd, buffer, size);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r && !size)
		return CZ_RESULT_SUCCESS;

	int err = errno;
	off_t pos = lseek(fd, 0, SEEK_CUR);
	int posErr = (pos == -1) ? errno : 0;
	errno = err;

	if (!r && posErr)
		return CZ_RESULT_NO_CONNECTION;
	if (!r && pos)
		return CZ_RESULT_BAD_OFFSET;
	if (!r)
		return CZ_RESULT_NO_FILE;

#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_1988
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENOTCONN:
		return CZ_RESULT_NO_CONNECTION;
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
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EINTEGRITY:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EAGAIN:
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
		return CZ_RESULT_NO_CONNECTION;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENOTCONN:
		return CZ_RESULT_NO_CONNECTION;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ETIMEDOUT:
		return CZ_RESULT_TIMEOUT;
	default:
		return CZ_RESULT_SUCCESS;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EBADMSG:
	case EINVAL:
	case EISDIR:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ECONNRESET:
	case ENOTCONN:
		return CZ_RESULT_NO_CONNECTION;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ETIMEDOUT:
		return CZ_RESULT_TIMEOUT;
	default:
		return CZ_RESULT_SUCCESS;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EBADMSG:
	case EINVAL:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_SUCCESS;
	}
#else
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
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
	if (!r && !size)
		return CZ_RESULT_SUCCESS;
	if (!r && offset)
		return CZ_RESULT_BAD_OFFSET;
	if (!r)
		return CZ_RESULT_NO_FILE;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001 && CZ_XOPEN_VERSION < CZ_SUS_1997
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
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
		return CZ_RESULT_BAD_IO;
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
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case ECONNRESET:
	case EINTEGRITY:
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EAGAIN:
	case EBUSY:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EBADMSG:
	case EISDIR:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EBADMSG:
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
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
	if (!r && !size)
		return CZ_RESULT_SUCCESS;
	if (!r)
		return CZ_RESULT_INTERNAL_ERROR;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_1988
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
	case EBADF:
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EDESTADDRREQ:
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EFBIG:
	case EINTEGRITY:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINVAL:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
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
	if (!r && !size)
		return CZ_RESULT_SUCCESS;
	if (!r)
		return CZ_RESULT_INTERNAL_ERROR;

	int err = errno;
#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD && CZ_POSIX_VERSION < CZ_POSIX_2001 && CZ_XOPEN_VERSION < CZ_SUS_1997
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (err) {
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EFBIG:
	case EINTEGRITY:
	case EPIPE:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (size > INT_MAX)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ACCESS;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
		return CZ_RESULT_NO_DISK;
	case ENOBUFS:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
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
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (size > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
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

	int err = errno;
#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD || CZ_POSIX_VERSION >= CZ_POSIX_2001
	long pageSize = sysconf(_SC_PAGESIZE);
	errno = err;
#endif
#if !CZ_DARWIN &&                         \
	!CZ_GNU_LINUX &&                      \
	!CZ_FREE_BSD &&                       \
	CZ_POSIX_VERSION < CZ_POSIX_2001 &&   \
	CZ_XOPEN_VERSION < CZ_SUS_1997 &&     \
	(                                     \
		CZ_XOPEN_VERSION < CZ_SUS_1994 || \
		CZ_XOPEN_UNIX <= 0)

	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
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
	switch (err) {
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
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (offset & (off_t) (pageSize - 1))
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
#elif CZ_FREE_BSD
	switch (err) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_NO_MEMORY;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (flags & MAP_ANON && offset)
			return CZ_RESULT_BAD_OFFSET;
		if (flags & MAP_GUARD && offset)
			return CZ_RESULT_BAD_OFFSET;
		if (flags & MAP_ANON && fd != -1)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && fd != -1)
			return CZ_RESULT_BAD_ACCESS;
		if (!(flags & (MAP_ANON | MAP_GUARD | MAP_PRIVATE | MAP_SHARED | MAP_STACK)))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_PRIVATE && flags & MAP_SHARED)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_EXCL && !(flags & MAP_FIXED))
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && prot != PROT_NONE)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && flags & MAP_ANON)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && flags & MAP_PREFAULT)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && flags & MAP_PREFAULT_READ)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && flags & MAP_PRIVATE)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && flags & MAP_SHARED)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && flags & MAP_STACK)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_32BIT && flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		if (flags & MAP_EXCL && flags & MAP_FIXED)
			return CZ_RESULT_IN_USE;
		if (flags & MAP_FIXED && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_INTERNAL_ERROR;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (err) {
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
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (offset & (off_t) (pageSize - 1))
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
		return CZ_RESULT_NO_LOCK;
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
	switch (err) {
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
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (offset & (off_t) (pageSize - 1))
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
		return CZ_RESULT_NO_LOCK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
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
		return CZ_RESULT_NO_LOCK;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_UNIX > 0
	switch (err) {
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
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (fd < -1)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax == -1)
		return CZ_RESULT_INTERNAL_ERROR;
	if (fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
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

	int err = errno;
#if !CZ_FREE_BSD
	long pageSize = sysconf(_SC_PAGESIZE);
	errno = err;
#endif

#if CZ_GNU_LINUX
	switch (err) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
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
#elif CZ_FREE_BSD
	switch (err) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_DARWIN || CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (err) {
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
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

	int err = errno;
#if !CZ_FREE_BSD
	long pageSize = sysconf(_SC_PAGESIZE);
	errno = err;
#endif

#if CZ_DARWIN
	switch (err) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (!size)
			return CZ_RESULT_BAD_SIZE;
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (err) {
	case EINVAL:
		if (flags & MS_ASYNC && flags & MS_INVALIDATE)
			return CZ_RESULT_BAD_ACCESS;
		return CZ_RESULT_BAD_ALIGNMENT;
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX || CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (err) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_UNIX > 0
	switch (err) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	if ((uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_SYSCONF
enum CzResult czWrap_sysconf(long* res, int name)
{
	errno = 0;
	long r = sysconf(name);
	if CZ_EXPECT (r != -1 || !errno) {
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
