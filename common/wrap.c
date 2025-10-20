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

#if CZ_WRAP_POSIX_MADVISE
enum CzResult czWrap_posix_madvise(int* res, void* addr, size_t size, int advice)
{
	int r = posix_madvise(addr, size, advice);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_APPLE
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
#else
	if (r == EINVAL)
		return size ? CZ_RESULT_BAD_ADDRESS : CZ_RESULT_BAD_SIZE;
	return (r == ENOMEM) ? CZ_RESULT_NO_MEMORY : CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

enum CzResult czWrap_fopen(FILE* restrict* res, const char* path, const char* mode)
{
	FILE* f = fopen(path, mode);
	if CZ_EXPECT (f) {
		*res = f;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_WINDOWS
	return (errno == EINVAL) ? CZ_RESULT_BAD_ADDRESS : CZ_RESULT_NO_FILE;
#elif CZ_APPLE
	switch (errno) {
	case EACCES:
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
	case ENOENT:
		return CZ_RESULT_NO_FILE;
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
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= 200112L
	if (errno == ENOENT)
		return (mode && mode[0] == 'r') ? CZ_RESULT_NO_FILE : CZ_RESULT_BAD_PATH;

	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
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
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_NO_FILE;
#endif
}

enum CzResult czWrap_fclose(FILE* stream)
{
	int r = fclose(stream);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_APPLE
	switch (errno) {
	case EDEADLK:
	case EFBIG:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= 200112L
	switch (errno) {
	case EFBIG:
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fseek(FILE* stream, long offset, int origin)
{
	int r = fseek(stream, offset, origin);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_APPLE
	switch (errno) {
	case EDEADLK:
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= 200112L
	switch (errno) {
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
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_ftell(long* res, FILE* stream)
{
	long r = ftell(stream);
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_WINDOWS
	switch (errno) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_APPLE
	switch (errno) {
	case EDEADLK:
	case EFBIG:
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= 200112L
	switch (errno) {
	case EOVERFLOW:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EBADF:
		return CZ_RESULT_BAD_STREAM;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}

enum CzResult czWrap_fread(size_t* res, void* buffer, size_t size, size_t count, FILE* stream)
{
	long pos = ftell(stream);
	if CZ_NOEXPECT (pos == -1)
		pos = 0;

	size_t r = fread(buffer, size, count, stream);
	if (res)
		*res = r;
	if CZ_EXPECT ((r || !size || !count) && !ferror(stream))
		return CZ_RESULT_SUCCESS;
	if (!r && feof(stream))
		return pos ? CZ_RESULT_BAD_OFFSET : CZ_RESULT_NO_FILE;

#if CZ_POSIX_VERSION >= 200112L
	switch (errno) {
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
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
	if (res)
		*res = r;
	if CZ_EXPECT ((r == count || !size) && !ferror(stream))
		return CZ_RESULT_SUCCESS;

#if CZ_POSIX_VERSION >= 200112L
	switch (errno) {
	case EFBIG:
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
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

#if CZ_WINDOWS
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_FILE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_APPLE
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
#elif CZ_POSIX_VERSION >= 200112L
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case EINVAL:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EBUSY:
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_NO_FILE;
#endif
}

#if CZ_WRAP_FILENO
enum CzResult czWrap_fileno(int* res, FILE* stream)
{
#if CZ_WINDOWS
	*res = _fileno(stream);
#elif CZ_APPLE
	*res = fileno(stream);
#else
	int f = fileno(stream);
	if CZ_NOEXPECT (f == -1)
		return CZ_RESULT_BAD_STREAM;
	*res = f;
#endif
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_ISATTY
enum CzResult czWrap_isatty(int* res, int fd)
{
#if CZ_WINDOWS
	int r = _isatty(fd);
	if CZ_NOEXPECT (!r && errno == EBADF)
		return CZ_RESULT_INTERNAL_ERROR;
	*res = r;
	return CZ_RESULT_SUCCESS;
#else
	int r = isatty(fd);
	if CZ_EXPECT (r || errno == ENOTTY) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}
	return (errno == EBADF) ? CZ_RESULT_NO_FILE : CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_STAT
enum CzResult czWrap_stat(const char* path, struct stat* st)
{
	int r = stat(path, st);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

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
#if !CZ_APPLE
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_FSTAT
enum CzResult czWrap_fstat(int fd, struct stat* st)
{
	int r = fstat(fd, st);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
#if CZ_APPLE
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
#endif
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_TRUNCATE
enum CzResult czWrap_truncate(const char* path, off_t size)
{
	int r = truncate(path, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_APPLE
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
#endif
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
#if CZ_APPLE
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if CZ_WRAP_FTRUNCATE
enum CzResult czWrap_ftruncate(int fd, off_t size)
{
	int r = ftruncate(fd, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
#if CZ_APPLE
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EDEADLK:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
#endif
	case EFBIG:
#if !CZ_APPLE
	case EINVAL:
#endif
		return CZ_RESULT_BAD_SIZE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
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

#if CZ_APPLE
	switch (errno) {
	case EACCES:
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
	case ENOENT:
		return CZ_RESULT_NO_FILE;
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
#else
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
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case EILSEQ:
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case EAGAIN:
	case EBUSY:
	case ETXTBSY:
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
#endif
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENODEV:
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	case ENOMEM:
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EINVAL:
	case EOPNOTSUPP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#endif
}
#endif

#if CZ_WRAP_CLOSE
enum CzResult czWrap_close(int fd)
{
	int r = close(fd);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#if !CZ_APPLE
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
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
	if (!r)
		return offset ? CZ_RESULT_BAD_OFFSET : CZ_RESULT_NO_FILE;

#if CZ_APPLE
	if (errno == EINVAL)
		return (size > INT_MAX) ? CZ_RESULT_BAD_SIZE : CZ_RESULT_BAD_OFFSET;

	switch (errno) {
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ESTALE:
		return CZ_RESULT_NO_FILE;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	case ETIMEDOUT:
		return CZ_RESULT_TIMEOUT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	switch (errno) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EBADMSG:
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
#endif
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#endif
}
#endif

#if CZ_WRAP_WRITE
enum CzResult czWrap_write(ssize_t* res, int fd, const void* buffer, size_t size)
{
	ssize_t r = write(fd, buffer, size);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1 && (size_t) r == size)
		return CZ_RESULT_SUCCESS;

#if CZ_APPLE
	if (errno == EINVAL)
		return (size > INT_MAX) ? CZ_RESULT_BAD_SIZE : CZ_RESULT_NO_FILE;

	switch (errno) {
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EFBIG:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDESTADDRREQ:
	case EFBIG:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
#if EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
#endif
		return CZ_RESULT_IN_USE;
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
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#endif
}
#endif

#if CZ_WRAP_PWRITE
enum CzResult czWrap_pwrite(ssize_t* res, int fd, const void* buffer, size_t size, off_t offset)
{
	ssize_t r = pwrite(fd, buffer, size, offset);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1 && (size_t) r == size)
		return CZ_RESULT_SUCCESS;

#if CZ_APPLE
	if (errno == EINVAL)
		return (size > INT_MAX) ? CZ_RESULT_BAD_SIZE : CZ_RESULT_BAD_OFFSET;

	switch (errno) {
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EFBIG:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	switch (errno) {
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDESTADDRREQ:
	case EFBIG:
	case ERANGE:
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
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
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
	if (errno == EINVAL)
		return size ? CZ_RESULT_BAD_ALIGNMENT : CZ_RESULT_BAD_SIZE;

#if CZ_APPLE
	switch (errno) {
	case ENXIO:
		return CZ_RESULT_BAD_ADDRESS;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	switch (errno) {
	case ENODEV:
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
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
#endif
}
#endif

#if CZ_WRAP_MUNMAP
enum CzResult czWrap_munmap(void* addr, size_t size)
{
	int r = munmap(addr, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;
	if (errno == EINVAL)
		return size ? CZ_RESULT_BAD_ADDRESS : CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_INTERNAL_ERROR;
}
#endif

#if CZ_WRAP_MSYNC
enum CzResult czWrap_msync(void* addr, size_t size, int flags)
{
	int r = msync(addr, size, flags);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
#if CZ_APPLE
		return size ? CZ_RESULT_BAD_ALIGNMENT : CZ_RESULT_BAD_SIZE;
#else
		return CZ_RESULT_BAD_ALIGNMENT;
#endif
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
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
	case ERROR_DELETE_PENDING:
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
	case ERROR_INSUFFICIENT_BUFFER:
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
	case ERROR_INSUFFICIENT_BUFFER:
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

#if CZ_WRAP_SYSCONF
enum CzResult czWrap_sysconf(long* res, int name)
{
	errno = 0;
	long r = sysconf(name);
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}
	return errno ? CZ_RESULT_INTERNAL_ERROR : CZ_RESULT_NO_SUPPORT;
}
#endif

enum CzResult czWrap_getExecutablePath(int* res, char* out, int capacity, int* dirnameLength)
{
	int r = wai_getExecutablePath(out, capacity, dirnameLength);
	if (res)
		*res = r;
	return (r == -1) ? CZ_RESULT_INTERNAL_ERROR : CZ_RESULT_SUCCESS;
}
