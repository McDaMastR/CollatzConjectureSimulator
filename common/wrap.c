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

enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength)
{
	int r = wai_getExecutablePath(out, capacity, dirnameLength);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1)
		return CZ_RESULT_SUCCESS;
	return CZ_RESULT_INTERNAL_ERROR;
}
