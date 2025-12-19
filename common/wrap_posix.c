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

#include "wrap_posix.h"
#include "debug.h"
#include "util.h"

/* 
 * Finds the file mode, file size, and block size of the file associated with the file descriptor. Does not modify
 * errno. If the file information cannot be found, sets fileMode to zero, fileSize to negative one, and blockSize to
 * zero.
 */
CZ_NONNULL_ARGS()
static void file_info(int fildes, mode_t* fileMode, off_t* fileSize, blksize_t* blockSize)
{
#if CZ_WRAP_FSTAT
	struct stat st = {0};
	int err = errno;
	int res = fstat(fildes, &st);
	errno = err;

	if (!res) {
		*fileMode = st.st_mode;
		*fileSize = st.st_size;
		*blockSize = st.st_blksize;
		return;
	}
#endif
	*fileMode = 0;
	*fileSize = -1;
	*blockSize = 0;
}

/* 
 * Returns the current position of the file descriptor. Does not modify errno. If the file is not seekable or the file
 * position is unknown, returns negative one.
 */
static off_t file_pos(int fildes)
{
#if CZ_WRAP_LSEEK
	int err = errno;
	off_t pos = lseek(fildes, 0, SEEK_CUR);
	errno = err;
	return pos;
#else
	return -1;
#endif
}

/* 
 * Returns the maximum number of open file descriptors. Does not modify errno. If there is no known maximum, returns
 * zero.
 */
static unsigned long system_open_max(void)
{
#if CZ_WRAP_SYSCONF && defined(_SC_OPEN_MAX)
	int err = errno;
	long openMax = sysconf(_SC_OPEN_MAX);
	errno = err;
	return (openMax > 0) ? (unsigned long) openMax : 0;
#elif defined(OPEN_MAX) && OPEN_MAX > 0
	return OPEN_MAX;
#else
	return 0;
#endif
}

/* 
 * Returns the page size of the system. Does not modify errno. If the page size cannot be found, returns zero.
 */
static unsigned long system_page_size(void)
{
#if CZ_WRAP_SYSCONF && defined(_SC_PAGESIZE)
	int err = errno;
	long pageSize = sysconf(_SC_PAGESIZE);
	errno = err;

	CZ_ASSERT(pageSize > 0);
	return (unsigned long) pageSize;
#elif defined(PAGESIZE) && PAGESIZE > 0
	return PAGESIZE;
#elif defined(PAGE_SIZE) && PAGE_SIZE > 0
	return PAGE_SIZE;
#else
	return 0;
#endif
}

#if CZ_WRAP_REALLOCARRAY
enum CzResult czWrap_reallocarray(void* restrict* res, void* ptr, size_t nelem, size_t elsize)
{
	void* p = reallocarray(ptr, nelem, elsize);
	if CZ_EXPECT (p) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "reallocarray failed with ptr 0x%016" PRIxPTR ", nelem %zu, elsize %zu (%.3fms)",
		(uintptr_t) ptr, nelem, elsize, t);

#if CZ_GNU_LINUX || CZ_FREE_BSD
	return CZ_RESULT_NO_MEMORY;
#else
	if (!nelem)
		return CZ_RESULT_BAD_SIZE;
	if (!elsize)
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
	return CZ_RESULT_NO_MEMORY;
#else
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
}
#endif

#if CZ_WRAP_POSIX_MEMALIGN
enum CzResult czWrap_posix_memalign(int* res, void* restrict* memptr, size_t alignment, size_t size)
{
	void* p;
	int r = posix_memalign(&p, alignment, size);
	if (res)
		*res = r;
	if CZ_EXPECT (!r) {
		*memptr = p;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr, "posix_memalign failed with memptr 0x%016" PRIxPTR ", alignment %zu, size %zu (%.3fms)",
		(uintptr_t) memptr, alignment, size, t);

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
	switch (r) {
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_BAD_ALIGNMENT;
	}
#else
	if (alignment < sizeof(void*))
		return CZ_RESULT_BAD_ALIGNMENT;
	if (alignment & (alignment - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	if (!size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_NO_MEMORY;
#endif
}
#endif

#if CZ_WRAP_MADVISE
enum CzResult czWrap_madvise(void* addr, size_t len, int advice)
{
	int r = madvise(addr, len, advice);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_GNU_LINUX
	unsigned long pageSize = system_page_size();
#endif

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
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EBADF:
		return CZ_RESULT_NO_FILE;
	case EIO:
		return CZ_RESULT_NO_MEMORY;
	case EINVAL:
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
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
	switch (errno) {
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
enum CzResult czWrap_posix_madvise(int* res, void* addr, size_t len, int advice)
{
	int r = posix_madvise(addr, len, advice);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if !CZ_DARWIN && !CZ_FREE_BSD
	unsigned long pageSize = system_page_size();
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
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
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
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!len)
		return CZ_RESULT_BAD_SIZE;
	if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FDOPEN
enum CzResult czWrap_fdopen(FILE* restrict* res, int fildes, const char* mode)
{
	FILE* s = fdopen(fildes, mode);
	if CZ_EXPECT (s) {
		*res = s;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
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
	switch (errno) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EBADF:
	case EINVAL:
	case ENOTTY:
		return CZ_RESULT_BAD_ACCESS;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
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
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a')
		return CZ_RESULT_BAD_ACCESS;
	if (mode[1] && mode[1] != 'b' && mode[1] != 'e' && mode[1] != 'x' && mode[1] != '+')
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FMEMOPEN
enum CzResult czWrap_fmemopen(FILE* restrict* res, void* buf, size_t max_size, const char* mode)
{
	FILE* s = fmemopen(buf, max_size, mode);
	if CZ_EXPECT (s) {
		*res = s;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
	switch (errno) {
	case EINVAL:
		if (!max_size)
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
		if (!max_size)
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
	if (mode[0] != 'r' && mode[0] != 'w' && mode[0] != 'a')
		return CZ_RESULT_BAD_ACCESS;
	if (mode[1] && mode[1] != 'b' && mode[1] != '+')
		return CZ_RESULT_BAD_ACCESS;
	if (mode[1] && mode[2] && mode[2] != 'b' && mode[2] != '+')
		return CZ_RESULT_BAD_ACCESS;
	if (mode[1] == 'b' && mode[2] == 'b')
		return CZ_RESULT_BAD_ACCESS;
	if (mode[1] == '+' && mode[2] == '+')
		return CZ_RESULT_BAD_ACCESS;
	if (mode[1] && mode[2] && mode[3])
		return CZ_RESULT_BAD_ACCESS;
	if (!buf && !mode[1])
		return CZ_RESULT_BAD_ACCESS;
	if (!buf && mode[1] == 'b' && !mode[2])
		return CZ_RESULT_BAD_ACCESS;
	if (!max_size)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (errno) {
	case EFBIG:
	case ENXIO:
	case EPIPE:
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
#endif
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
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
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
#endif

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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
#endif
	case EEXIST:
	case ENOTEMPTY:
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case ELOOP:
		return CZ_RESULT_BAD_PATH;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case EINVAL:
		return CZ_RESULT_BAD_PATH;
#endif
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
	return CZ_RESULT_INTERNAL_ERROR;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EACCES:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case ELOOP:
		return CZ_RESULT_BAD_PATH;
#endif
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EBUSY:
		return CZ_RESULT_IN_USE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
#endif
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

#if CZ_WRAP_UNLINKAT
enum CzResult czWrap_unlinkat(int fd, const char* path, int flag)
{
	int r = unlinkat(fd, path, flag);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
		if (flag & AT_REMOVEDIR)
			return CZ_RESULT_BAD_FILE;
#endif
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
	switch (errno) {
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
	switch (errno) {
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
		if (flag & AT_RESOLVE_BENEATH)
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
	switch (errno) {
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
		if (flag & AT_REMOVEDIR)
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
	unsigned long openMax = system_open_max();
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (fd != AT_FDCWD && fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (fd != AT_FDCWD && openMax && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FILENO
enum CzResult czWrap_fileno(int* res, FILE* stream)
{
	errno = 0;
	int fd = fileno(stream);
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
}
#endif

#if CZ_WRAP_ISATTY
enum CzResult czWrap_isatty(int* res, int fildes)
{
#if !CZ_GNU_LINUX
	errno = 0;
#endif
	int r = isatty(fildes);
	if (r) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_GNU_LINUX
	switch (errno) {
	case EINVAL:
	case ENOTTY:
		*res = r;
		return CZ_RESULT_SUCCESS;
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_DARWIN || CZ_FREE_BSD || CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case 0:
	case ENOTTY:
		*res = r;
		return CZ_RESULT_SUCCESS;
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if CZ_NOEXPECT (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if CZ_NOEXPECT (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;

	*res = r;
	return CZ_RESULT_SUCCESS;
#endif
}
#endif

#if CZ_WRAP_STAT
enum CzResult czWrap_stat(const char* path, struct stat* buf)
{
	int r = stat(path, buf);
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case ELOOP:
		return CZ_RESULT_BAD_PATH;
#endif
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
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_LSTAT
enum CzResult czWrap_lstat(const char* path, struct stat* buf)
{
	int r = lstat(path, buf);
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
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
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FSTAT
enum CzResult czWrap_fstat(int fildes, struct stat* buf)
{
	int r = fstat(fildes, buf);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
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
#elif CZ_FREE_BSD
	switch (errno) {
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988 || CZ_XOPEN_VERSION >= CZ_XPG_1985
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FSTATAT
enum CzResult czWrap_fstatat(int fd, const char* path, struct stat* buf, int flag)
{
	int r = fstatat(fd, path, buf, flag);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
	switch (errno) {
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
	switch (errno) {
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
	switch (errno) {
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
	unsigned long openMax = system_open_max();
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (fd != AT_FDCWD && fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (fd != AT_FDCWD && openMax && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FLOCK
enum CzResult czWrap_flock(int fd, int op)
{
	int r = flock(fd, op);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
	switch (errno) {
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
	switch (errno) {
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
	unsigned long openMax = system_open_max();
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_LOCKF
enum CzResult czWrap_lockf(int fildes, int function, off_t size)
{
	int r = lockf(fildes, function, size);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if !CZ_DARWIN && !CZ_GNU_LINUX && !CZ_FREE_BSD
	off_t pos = file_pos(fildes);
#endif

#if CZ_DARWIN
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		switch (function) {
		case F_LOCK:
		case F_TEST:
		case F_TLOCK:
		case F_ULOCK:
			return CZ_RESULT_BAD_FILE;
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
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
	switch (errno) {
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
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		switch (function) {
		case F_LOCK:
		case F_TEST:
		case F_TLOCK:
		case F_ULOCK:
			return CZ_RESULT_BAD_FILE;
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
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
#elif CZ_XOPEN_VERSION >= CZ_SUS_1994
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
#if CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
#endif
	case EINVAL:
		switch (function) {
		case F_LOCK:
		case F_TEST:
		case F_TLOCK:
		case F_ULOCK:
			if (pos >= 0 && size < -pos)
				return CZ_RESULT_BAD_SIZE;
			return CZ_RESULT_BAD_FILE;
		default:
			return CZ_RESULT_NO_SUPPORT;
		}
	case EDEADLK:
		if (function == F_LOCK)
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
	switch (function) {
	case F_LOCK:
	case F_TEST:
	case F_TLOCK:
	case F_ULOCK:
		break;
	default:
		return CZ_RESULT_NO_SUPPORT;
	}

	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	if (pos < 0)
		return CZ_RESULT_BAD_FILE;
	if (size < -pos)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FCNTL
enum CzResult czWrap_fcntl(int* res, int fildes, int cmd, ...)
{
	int r = -1;
	int intArg = 0;
	struct flock* lockArg = NULL;
#if CZ_DARWIN && (!CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE)
	char* pathArg = NULL;
#endif
#if CZ_DARWIN && (!CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE)
	off_t* offArg = NULL;
#endif
#if CZ_DARWIN && (!CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE)
	struct fstore* storeArg = NULL;
#endif
#if CZ_DARWIN && (!CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE)
	struct fpunchhole* holeArg = NULL;
#endif
#if CZ_DARWIN && (!CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE)
	struct radvisory* readAdviceArg = NULL;
#endif
#if CZ_DARWIN && (!CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE)
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
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_BARRIERFSYNC:
	case F_FULLFSYNC:
	case F_GETNOSIGPIPE:
#endif
		r = fcntl(fildes, cmd);
		break;
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
	case F_DUPFD_CLOEXEC:
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_NOCACHE:
	case F_RDAHEAD:
	case F_SETNOSIGPIPE:
	case F_TRANSFEREXTENTS:
#endif
		intArg = va_arg(vargs, int);
		r = fcntl(fildes, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
#if CZ_DARWIN_C_SOURCE
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
#endif
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fildes, cmd, lockArg);
		break;
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_GETPATH:
	case F_GETPATH_NOFIRMLINK:
		pathArg = va_arg(vargs, char*);
		r = fcntl(fildes, cmd, pathArg);
		break;
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_SETSIZE:
		offArg = va_arg(vargs, off_t*);
		r = fcntl(fildes, cmd, offArg);
		break;
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_PREALLOCATE:
		storeArg = va_arg(vargs, struct fstore*);
		r = fcntl(fildes, cmd, storeArg);
		break;
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_PUNCHHOLE:
		holeArg = va_arg(vargs, struct fpunchhole*);
		r = fcntl(fildes, cmd, holeArg);
		break;
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_RDADVISE:
		readAdviceArg = va_arg(vargs, struct radvisory*);
		r = fcntl(fildes, cmd, readAdviceArg);
		break;
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
	case F_LOG2PHYS:
	case F_LOG2PHYS_EXT:
		logPhysArg = va_arg(vargs, struct log2phys*);
		r = fcntl(fildes, cmd, logPhysArg);
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
#if CZ_GNU_SOURCE
	case F_GET_SEALS:
	case F_GETLEASE:
	case F_GETPIPE_SZ:
	case F_GETSIG:
#endif
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_1997
	case F_GETOWN:
#endif
		r = fcntl(fildes, cmd);
		break;
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
#if CZ_GNU_SOURCE
	case F_ADD_SEALS:
	case F_NOTIFY:
	case F_SETLEASE:
	case F_SETPIPE_SZ:
	case F_SETSIG:
#endif
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_2008
	case F_DUPFD_CLOEXEC:
#endif
#if CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_XOPEN_SOURCE >= CZ_SUS_1997
	case F_SETOWN:
#endif
		intArg = va_arg(vargs, int);
		r = fcntl(fildes, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
#if CZ_GNU_SOURCE
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
#endif
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fildes, cmd, lockArg);
		break;
#if CZ_GNU_SOURCE
	case F_GET_RW_HINT:
	case F_SET_RW_HINT:
	case F_GET_FILE_RW_HINT:
	case F_SET_FILE_RW_HINT:
		hintArg = va_arg(vargs, CzU64*);
		r = fcntl(fildes, cmd, hintArg);
		break;
#endif
#if CZ_GNU_SOURCE
	case F_GETOWN_EX:
	case F_SETOWN_EX:
		ownerArg = va_arg(vargs, struct f_owner_ex*);
		r = fcntl(fildes, cmd, ownerArg);
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
		r = fcntl(fildes, cmd);
		break;
	case F_DUPFD:
	case F_DUPFD_CLOEXEC:
	case F_READAHEAD:
	case F_RDAHEAD:
	case F_SET_SEALS:
	case F_SETFD:
	case F_SETFL:
	case F_SETOWN:
#if CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(7, 1)
	case F_DUP2FD:
	case F_DUP2FD_CLOEXEC:
#endif
		intArg = va_arg(vargs, int);
		r = fcntl(fildes, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fildes, cmd, lockArg);
		break;
	case F_KINFO:
		kinfoArg = va_arg(vargs, struct kinfo_file*);
		r = fcntl(fildes, cmd, kinfoArg);
		break;
	default:
		va_end(vargs);
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988 || CZ_XOPEN_VERSION >= CZ_XPG_1985
	switch (cmd) {
	case F_GETFD:
	case F_GETFL:
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case F_GETOWN:
#endif
		r = fcntl(fildes, cmd);
		break;
	case F_DUPFD:
	case F_SETFD:
	case F_SETFL:
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case F_SETOWN:
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2008
	case F_DUPFD_CLOEXEC:
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case F_DUPFD_CLOFORK:
#endif
		intArg = va_arg(vargs, int);
		r = fcntl(fildes, cmd, intArg);
		break;
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case F_OFD_GETLK:
	case F_OFD_SETLK:
	case F_OFD_SETLKW:
#endif
		lockArg = va_arg(vargs, struct flock*);
		r = fcntl(fildes, cmd, lockArg);
		break;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case F_GETOWN_EX:
	case F_SETOWN_EX:
		ownerArg = va_arg(vargs, struct f_owner_ex*);
		r = fcntl(fildes, cmd, ownerArg);
		break;
#endif
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

#if !CZ_GNU_LINUX && !CZ_FREE_BSD
	mode_t fileMode;
	off_t fileSize;
	blksize_t blockSize;
	file_info(fildes, &fileMode, &fileSize, &blockSize);
#endif
#if !CZ_GNU_LINUX && !CZ_FREE_BSD
	off_t pos = file_pos(fildes);
#endif
#if CZ_FREE_BSD_VERSION >= CZ_MAKE_VERSION(7, 1)
	unsigned long openMax = system_open_max();
#endif

#if CZ_DARWIN
	switch (errno) {
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
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
		case F_TRANSFEREXTENTS:
			if (fileMode && !S_ISREG(fileMode))
				return CZ_RESULT_BAD_FILE;
			return CZ_RESULT_BAD_ACCESS;
#endif
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
	case EINVAL:
		switch (cmd) {
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
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
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
		case F_TRANSFEREXTENTS:
			if (intArg < 0)
				return CZ_RESULT_BAD_ACCESS;
			return CZ_RESULT_BAD_FILE;
#endif
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
		case F_PREALLOCATE:
			return CZ_RESULT_BAD_OFFSET;
#endif
		case F_DUPFD:
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
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
				if (pos < 0)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END:
				if (fileSize < 0)
					break;
				if (lockArg->l_start < -fileSize)
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
	switch (errno) {
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
	switch (errno) {
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
			if (openMax && intArg >= openMax)
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988 || CZ_XOPEN_VERSION >= CZ_XPG_1985
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EOVERFLOW:
		return CZ_RESULT_BAD_RANGE;
#endif
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
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case ESRCH:
		return CZ_RESULT_NO_PROCESS;
#endif
	case EINVAL:
		switch (cmd) {
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
		case F_SETOWN:
		case F_SETOWN_EX:
			return CZ_RESULT_BAD_ACCESS;
#endif
		case F_DUPFD:
			return CZ_RESULT_NO_OPEN;
#if CZ_POSIX_VERSION >= CZ_POSIX_2008
		case F_DUPFD_CLOEXEC:
			return CZ_RESULT_NO_OPEN;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
		case F_DUPFD_CLOFORK:
			return CZ_RESULT_NO_OPEN;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
		case F_OFD_GETLK:
		case F_OFD_SETLK:
		case F_OFD_SETLKW:
			if (lockArg->l_pid)
				return CZ_RESULT_BAD_ACCESS;
#endif
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
				if (pos < 0)
					break;
				if (lockArg->l_start < -pos)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + pos < -lockArg->l_start)
					return CZ_RESULT_BAD_OFFSET;
				break;
			case SEEK_END;
				if (fileSize < 0)
					break;
				if (lockArg->l_start < -fileSize)
					return CZ_RESULT_BAD_OFFSET;
				if (lockArg->l_len < 0 && lockArg->l_len + fileSize < -lockArg->l_start)
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
enum CzResult czWrap_truncate(const char* path, off_t length)
{
	int r = truncate(path, length);
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008 || CZ_XOPEN_VERSION >= CZ_SUS_1994
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
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EINVAL:
		if (length < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_FILE;
#else
	case EINVAL:
		return CZ_RESULT_BAD_SIZE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (length < 0)
		return CZ_RESULT_BAD_SIZE;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FTRUNCATE
enum CzResult czWrap_ftruncate(int fildes, off_t length)
{
	int r = ftruncate(fildes, length);
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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
	case EINVAL:
		if (length < 0)
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
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if (CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1997) && CZ_POSIX_VERSION < CZ_POSIX_2008
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EFBIG:
		return CZ_RESULT_BAD_SIZE;
#endif
	case EINVAL:
		if (length < 0)
			return CZ_RESULT_BAD_SIZE;
#if CZ_POSIX_VERSION < CZ_POSIX_2001 || CZ_POSIX_VERSION >= CZ_POSIX_2024
		return CZ_RESULT_BAD_FILE;
#else
		return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (length < 0)
		return CZ_RESULT_BAD_SIZE;
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_FADVISE
enum CzResult czWrap_posix_fadvise(int* res, int fd, off_t offset, off_t len, int advice)
{
	int r = posix_fadvise(fd, offset, len, advice);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2001
	switch (r) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (len < 0)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (len < 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FALLOCATE
enum CzResult czWrap_fallocate(int fd, int mode, off_t offset, off_t len)
{
	int r = fallocate(fd, mode, offset, len);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_GNU_LINUX
	mode_t fileMode;
	off_t fileSize;
	blksize_t blockSize;
	file_info(fd, &fileMode, &fileSize, &blockSize);
#endif

#if CZ_GNU_LINUX
	switch (errno) {
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
		if (len <= 0)
			return CZ_RESULT_BAD_SIZE;
		switch (mode) {
		case FALLOC_FL_COLLAPSE_RANGE:
			if (fileMode && !S_ISREG(fileMode))
				return CZ_RESULT_BAD_FILE;
			if (fileSize >= 0 && len >= fileSize - offset)
				return CZ_RESULT_BAD_RANGE;
			if (blockSize > 0 && offset & (blockSize - 1))
				return CZ_RESULT_BAD_ALIGNMENT;
			if (blockSize > 0 && len & (blockSize - 1))
				return CZ_RESULT_BAD_ALIGNMENT;
			break;
		case FALLOC_FL_INSERT_RANGE:
			if (fileMode && !S_ISREG(fileMode))
				return CZ_RESULT_BAD_FILE;
			if (fileSize >= 0 && offset >= fileSize)
				return CZ_RESULT_BAD_RANGE;
			if (blockSize > 0 && offset & (blockSize - 1))
				return CZ_RESULT_BAD_ALIGNMENT;
			if (blockSize > 0 && len & (blockSize - 1))
				return CZ_RESULT_BAD_ALIGNMENT;
			break;
		case FALLOC_FL_ZERO_RANGE:
			if (fileMode && !S_ISREG(fileMode))
				return CZ_RESULT_BAD_FILE;
			break;
		default:
			if (mode & FALLOC_FL_COLLAPSE_RANGE)
				return CZ_RESULT_BAD_ACCESS;
			if (mode & FALLOC_FL_INSERT_RANGE)
				return CZ_RESULT_BAD_ACCESS;
			break;
		}
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
	unsigned long openMax = system_open_max();
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (len <= 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_FALLOCATE
enum CzResult czWrap_posix_fallocate(int* res, int fd, off_t offset, off_t len)
{
	int r = posix_fallocate(fd, offset, len);
	if (res)
		*res = r;
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

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
		if (len <= 0)
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
		if (len <= 0)
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
		if (len <= 0)
			return CZ_RESULT_BAD_SIZE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
		return CZ_RESULT_INTERNAL_ERROR;
#else
		return CZ_RESULT_NO_SUPPORT;
#endif
	case EFBIG:
		return CZ_RESULT_BAD_RANGE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (len <= 0)
		return CZ_RESULT_BAD_SIZE;
	if (fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fd >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FSYNC
enum CzResult czWrap_fsync(int fildes)
{
	int r = fsync(fildes);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
	switch (errno) {
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
	switch (errno) {
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_XPG_1989
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#if CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_FDATASYNC
enum CzResult czWrap_fdatasync(int fildes)
{
	int r = fdatasync(fildes);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_GNU_LINUX
	switch (errno) {
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
	switch (errno) {
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (errno) {
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
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_OPEN
enum CzResult czWrap_open(int* res, const char* path, int oflag, mode_t mode)
{
	int f = open(path, oflag, mode);
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
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
#endif
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
		if (oflag & O_SEARCH)
			return CZ_RESULT_BAD_FILE;
#endif
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
		if (oflag & O_CREAT)
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
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EOPNOTSUPP:
		if (oflag & O_SHLOCK)
			return CZ_RESULT_NO_SUPPORT;
		if (oflag & O_EXLOCK)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
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
		if (oflag & O_TMPFILE && !(oflag & (O_WRONLY | O_RDWR)))
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		if (oflag & O_DIRECT)
			return CZ_RESULT_NO_SUPPORT;
#endif
#if (                                                  \
		CZ_GNU_SOURCE &&                               \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 12)) ||  \
	(                                                  \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	(                                                  \
		CZ_XOPEN_SOURCE >= CZ_SUS_2008 &&              \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12))

		if (oflag & O_CREAT && oflag & O_DIRECTORY)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_PATH;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EEXIST:
	case EFBIG:
	case ENODEV:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
#if (                                                  \
		CZ_GNU_SOURCE &&                               \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 12)) ||  \
	(                                                  \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	(                                                  \
		CZ_XOPEN_SOURCE >= CZ_SUS_2008 &&              \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12))

		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
#endif
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
#if CZ_GNU_SOURCE
		if (oflag & O_TMPFILE)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_NO_FILE;
	case EBUSY:
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		if (oflag & O_NONBLOCK && oflag & O_WRONLY)
			return CZ_RESULT_NO_CONNECTION;
		return CZ_RESULT_BAD_FILE;
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
		if (oflag & O_TMPFILE)
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
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
	case EOPNOTSUPP:
		if (oflag & O_SHLOCK)
			return CZ_RESULT_NO_SUPPORT;
		if (oflag & O_EXLOCK)
			return CZ_RESULT_NO_SUPPORT;
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case ELOOP:
		return CZ_RESULT_BAD_PATH;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EILSEQ:
		return CZ_RESULT_BAD_PATH;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case ENOMEM:
	case ENOSR:
		return CZ_RESULT_NO_MEMORY;
#endif
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case ENOTDIR:
#if CZ_POSIX_VERSION >= CZ_POSIX_2008
		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024 && O_EXEC != O_SEARCH
		if (oflag & O_SEARCH)
			return CZ_RESULT_BAD_FILE;
#endif
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EINVAL:
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
		int accessFlag = oflag & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH);
		switch (accessFlag) {
		case O_RDONLY:
		case O_WRONLY:
		case O_RDWR:
		case O_EXEC:
			break;
#if O_EXEC != O_SEARCH
		case O_SEARCH:
			break;
#endif
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
#else
		int accessFlag = oflag & (O_RDONLY | O_WRONLY | O_RDWR);
		switch (accessFlag) {
		case O_RDONLY:
		case O_WRONLY:
		case O_RDWR:
			break;
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
#endif
#if CZ_POSIX_SYNCHRONIZED_IO >= 0
		if (oflag & (O_DSYNC | O_RSYNC))
			return CZ_RESULT_NO_SUPPORT;
#endif
#if CZ_POSIX_SYNCHRONIZED_IO >= 0 || CZ_XOPEN_VERSION >= CZ_SUS_2008
		if (oflag & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (!(oflag & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_OPENAT
enum CzResult czWrap_openat(int* res, int fd, const char* path, int oflag, mode_t mode)
{
	int f = openat(fd, path, oflag, mode);
	if CZ_EXPECT (f != -1) {
		*res = f;
		return CZ_RESULT_SUCCESS;
	}

#if CZ_DARWIN
	switch (errno) {
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
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
#endif
#if !CZ_ANSI_SOURCE && (CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 || CZ_DARWIN_C_SOURCE)
		if (oflag & O_SEARCH)
			return CZ_RESULT_BAD_FILE;
#endif
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
		if (oflag & O_CREAT)
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
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
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
	case EBADF:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
#if CZ_GNU_SOURCE
		if (oflag & O_TMPFILE && !(oflag & (O_WRONLY | O_RDWR)))
			return CZ_RESULT_BAD_ACCESS;
#endif
#if CZ_GNU_SOURCE
		if (oflag & O_DIRECT)
			return CZ_RESULT_NO_SUPPORT;
#endif
#if (                                                  \
		CZ_GNU_SOURCE &&                               \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 12)) ||  \
	(                                                  \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	(                                                  \
		CZ_XOPEN_SOURCE >= CZ_SUS_2008 &&              \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12))

		if (oflag & O_CREAT && oflag & O_DIRECTORY)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_PATH;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EEXIST:
	case EFBIG:
	case ENODEV:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
#if (                                                  \
		CZ_GNU_SOURCE &&                               \
		CZ_GLIBC_VERSION < CZ_MAKE_VERSION(2, 12)) ||  \
	(                                                  \
		CZ_POSIX_C_SOURCE >= CZ_POSIX_2008 &&          \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12)) || \
	(                                                  \
		CZ_XOPEN_SOURCE >= CZ_SUS_2008 &&              \
		CZ_GLIBC_VERSION >= CZ_MAKE_VERSION(2, 12))

		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
#endif
		return CZ_RESULT_BAD_PATH;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
#if CZ_GNU_SOURCE
		if (oflag & O_TMPFILE)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_NO_FILE;
	case EBUSY:
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
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
		if (oflag & O_TMPFILE)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
	case EACCES:
	case EBADF:
	case ECAPMODE:
	case EPERM:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case ENOTCAPABLE:
		if (!(oflag & O_RESOLVE_BENEATH))
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
		return CZ_RESULT_BAD_FILE;
	case ENOTDIR:
		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_PATH;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
	case EINVAL:
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_BAD_ACCESS;
	case ENOENT:
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case ETXTBSY:
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008
	switch (errno) {
	case EACCES:
	case EBADF:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EEXIST:
	case EISDIR:
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_2008 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
	case ELOOP:
	case ENAMETOOLONG:
		return CZ_RESULT_BAD_PATH;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EILSEQ:
		return CZ_RESULT_BAD_PATH;
#endif
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
#if CZ_XOPEN_VERSION >= CZ_SUS_2008
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#if CZ_XOPEN_VERSION >= CZ_SUS_2008 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case ENOMEM:
	case ENOSR:
		return CZ_RESULT_NO_MEMORY;
#endif
	case EMFILE:
	case ENFILE:
		return CZ_RESULT_NO_OPEN;
	case ENOTDIR:
		if (oflag & O_DIRECTORY)
			return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024 && O_EXEC != O_SEARCH
		if (oflag & O_SEARCH)
			return CZ_RESULT_BAD_FILE;
#endif
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		if (!path[0])
			return CZ_RESULT_BAD_PATH;
		if (oflag & O_CREAT)
			return CZ_RESULT_BAD_PATH;
		return CZ_RESULT_NO_FILE;
	case EINVAL:
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
		int accessFlag = oflag & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH);
		switch (accessFlag) {
		case O_RDONLY:
		case O_WRONLY:
		case O_RDWR:
		case O_EXEC:
			break;
#if O_EXEC != O_SEARCH
		case O_SEARCH:
			break;
#endif
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
#else
		int accessFlag = oflag & (O_RDONLY | O_WRONLY | O_RDWR);
		switch (accessFlag) {
		case O_RDONLY:
		case O_WRONLY:
		case O_RDWR:
			break;
		default:
			return CZ_RESULT_BAD_ACCESS;
		}
#endif
#if CZ_POSIX_SYNCHRONIZED_IO >= 0
		if (oflag & (O_DSYNC | O_RSYNC))
			return CZ_RESULT_NO_SUPPORT;
#endif
#if CZ_POSIX_SYNCHRONIZED_IO >= 0 || CZ_XOPEN_VERSION >= CZ_SUS_2008
		if (oflag & O_SYNC)
			return CZ_RESULT_NO_SUPPORT;
#endif
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (!path[0])
		return CZ_RESULT_BAD_PATH;
	if (!(oflag & (O_RDONLY | O_WRONLY | O_RDWR | O_EXEC | O_SEARCH)))
		return CZ_RESULT_BAD_ACCESS;
	if (fd != AT_FDCWD && fd < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (fd != AT_FDCWD && openMax && fd >= openMax)
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
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
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
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
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
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EACCES:
	case EROFS:
		return CZ_RESULT_BAD_ACCESS;
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EOPNOTSUPP:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
	case ENAMETOOLONG:
	case ENOENT:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case ELOOP:
		return CZ_RESULT_BAD_PATH;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EILSEQ:
		return CZ_RESULT_BAD_PATH;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	case ETXTBSY:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENXIO:
		return CZ_RESULT_NO_CONNECTION;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case ENOMEM:
	case ENOSR:
		return CZ_RESULT_NO_MEMORY;
#endif
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
enum CzResult czWrap_close(int fildes)
{
	int r = close(fildes);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN
	switch (errno) {
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
	switch (errno) {
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
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988 || CZ_XOPEN_VERSION >= CZ_XPG_1985
	switch (errno) {
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case EINPROGRESS:
		return CZ_RESULT_SUCCESS;
#endif
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
#if CZ_POSIX_VERSION < CZ_POSIX_2024 || POSIX_CLOSE_RESTART
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_POSIX_CLOSE
enum CzResult czWrap_posix_close(int fildes, int flag)
{
	int r = posix_close(fildes, flag);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	switch (errno) {
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
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_LSEEK
enum CzResult czWrap_lseek(off_t* res, int fildes, off_t offset, int whence)
{
	errno = 0;
	off_t r = lseek(fildes, offset, whence);
	if (res)
		*res = r;
	if CZ_EXPECT (r != -1)
		return CZ_RESULT_SUCCESS;

#if CZ_DARWIN || CZ_GNU_LINUX || CZ_FREE_BSD
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988 || CZ_XOPEN_VERSION >= CZ_XPG_1985
	switch (errno) {
	case 0:
		return CZ_RESULT_SUCCESS;
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2024
	case ENXIO:
		return CZ_RESULT_BAD_OFFSET;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_READ
enum CzResult czWrap_read(ssize_t* res, int fildes, void* buf, size_t nbyte)
{
	ssize_t r = read(fildes, buf, nbyte);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r && !nbyte)
		return CZ_RESULT_SUCCESS;

	off_t pos = file_pos(fildes);
	if (!r && pos < 0)
		return CZ_RESULT_NO_CONNECTION;
	if (!r && pos)
		return CZ_RESULT_BAD_OFFSET;
	if (!r)
		return CZ_RESULT_NO_FILE;

#if CZ_DARWIN
	switch (errno) {
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
		if (nbyte > INT_MAX)
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
	switch (errno) {
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
	switch (errno) {
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
		if (nbyte > INT_MAX)
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EBADMSG:
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
#endif
	case EIO:
		return CZ_RESULT_BAD_IO;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
#endif
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 && EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ECONNRESET:
	case ENOTCONN:
		return CZ_RESULT_NO_CONNECTION;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ETIMEDOUT:
		return CZ_RESULT_TIMEOUT;
#endif
	default:
		return CZ_RESULT_SUCCESS;
	}
#else
	unsigned long openMax = system_open_max();
	if (nbyte > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_PREAD
enum CzResult czWrap_pread(ssize_t* res, int fildes, void* buf, size_t nbyte, off_t offset)
{
	ssize_t r = pread(fildes, buf, nbyte, offset);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r && !nbyte)
		return CZ_RESULT_SUCCESS;
	if (!r && offset)
		return CZ_RESULT_BAD_OFFSET;
	if (!r)
		return CZ_RESULT_NO_FILE;

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
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (nbyte > INT_MAX)
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
	switch (errno) {
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
		if (nbyte > INT_MAX)
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
#if CZ_XOPEN_VERSION >= CZ_SUS_1997 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EBADMSG:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EISDIR:
		return CZ_RESULT_BAD_FILE;
#endif
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EOVERFLOW:
		return CZ_RESULT_BAD_OFFSET;
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
#if CZ_POSIX_VERSION >= CZ_POSIX_2008 || CZ_XOPEN_VERSION >= CZ_SUS_2001
	case ENOBUFS:
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1997 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
#else
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
#endif
	default:
		return CZ_RESULT_SUCCESS;
	}
#else
	unsigned long openMax = system_open_max();
	if (nbyte > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_WRITE
enum CzResult czWrap_write(ssize_t* res, int fildes, const void* buf, size_t nbyte)
{
	ssize_t r = write(fildes, buf, nbyte);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r && !nbyte)
		return CZ_RESULT_SUCCESS;
	/* 
	 * Should never happen on POSIX-conformant implementations, but might indicate something platform-specific on some
	 * old or shit implementation. Just return an error to avoid any potential for infinite retry loops.
	 */
	if (!r)
		return CZ_RESULT_INTERNAL_ERROR;

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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (nbyte > INT_MAX)
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
	switch (errno) {
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
	switch (errno) {
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
		if (nbyte > INT_MAX)
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
#endif
	case EFBIG:
		return CZ_RESULT_BAD_FILE;
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EINVAL:
		return CZ_RESULT_BAD_FILE;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_XPG_1992
	case ENXIO:
		return CZ_RESULT_BAD_FILE;
#endif
	case EIO:
		return CZ_RESULT_BAD_IO;
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
#endif
	case EAGAIN:
		return CZ_RESULT_IN_USE;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 && EAGAIN != EWOULDBLOCK
	case EWOULDBLOCK:
		return CZ_RESULT_IN_USE;
#endif
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case EPIPE:
		return CZ_RESULT_NO_CONNECTION;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ECONNRESET:
	case ENETDOWN:
	case ENETUNREACH:
		return CZ_RESULT_NO_CONNECTION;
#endif
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001
	case ENOBUFS:
		return CZ_RESULT_NO_MEMORY;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (nbyte > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_PWRITE
enum CzResult czWrap_pwrite(ssize_t* res, int fildes, const void* buf, size_t nbyte, off_t offset)
{
	ssize_t r = pwrite(fildes, buf, nbyte, offset);
	if (res)
		*res = r;
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r && !nbyte)
		return CZ_RESULT_SUCCESS;
	/* 
	 * Should never happen on POSIX-conformant implementations, but might indicate something platform-specific on some
	 * old or shit implementation. Just return an error to avoid any potential for infinite retry loops.
	 */
	if (!r)
		return CZ_RESULT_INTERNAL_ERROR;

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
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (nbyte > INT_MAX)
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
	switch (errno) {
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
		if (nbyte > INT_MAX)
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_2008 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	switch (errno) {
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EFBIG:
	case ENXIO:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_VERSION < CZ_POSIX_2024
	case EPIPE:
		return CZ_RESULT_BAD_FILE;
#endif
	case EIO:
		return CZ_RESULT_BAD_IO;
#if CZ_XOPEN_VERSION >= CZ_SUS_1997 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case ERANGE:
		return CZ_RESULT_BAD_SIZE;
#endif
	case EAGAIN:
		return CZ_RESULT_IN_USE;
	case EINTR:
		return CZ_RESULT_INTERRUPT;
	case ENOSPC:
		return CZ_RESULT_NO_DISK;
#if CZ_POSIX_VERSION >= CZ_POSIX_2008 || CZ_XOPEN_VERSION >= CZ_SUS_2001
	case ENOBUFS:
		return CZ_RESULT_NO_MEMORY;
#endif
#if CZ_XOPEN_VERSION >= CZ_SUS_1997 && CZ_XOPEN_VERSION < CZ_SUS_2024
	case EINVAL:
		if (offset < 0)
			return CZ_RESULT_BAD_OFFSET;
		return CZ_RESULT_BAD_FILE;
#else
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (nbyte > SSIZE_MAX)
		return CZ_RESULT_BAD_SIZE;
	if (offset < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (fildes < 0)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_MMAP
enum CzResult czWrap_mmap(void* restrict* res, void* addr, size_t len, int prot, int flags, int fildes, off_t off)
{
	void* p = mmap(addr, len, prot, flags, fildes, off);
	if CZ_EXPECT (p != MAP_FAILED) {
		*res = p;
		return CZ_RESULT_SUCCESS;
	}

	unsigned long pageSize = system_page_size();
#if CZ_DARWIN
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
	case EINVAL:
		if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
			return CZ_RESULT_BAD_ACCESS;
		if (off < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (pageSize && off & (off_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (flags & MAP_FIXED && pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ADDRESS;
	case ENXIO:
	case EOVERFLOW:
		return CZ_RESULT_BAD_ADDRESS;
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
	case ENOMEM:
#if !CZ_POSIX_C_SOURCE || CZ_DARWIN_C_SOURCE
		if (flags & MAP_ANON)
			return CZ_RESULT_NO_MEMORY;
#endif
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
		if (off < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (pageSize && off & (off_t) (pageSize - 1))
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
	switch (errno) {
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
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (off < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (flags & MAP_ANON && off)
			return CZ_RESULT_BAD_OFFSET;
		if (flags & MAP_GUARD && off)
			return CZ_RESULT_BAD_OFFSET;
		if (flags & MAP_ANON && fildes != -1)
			return CZ_RESULT_BAD_ACCESS;
		if (flags & MAP_GUARD && fildes != -1)
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
		if (flags & MAP_FIXED && pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_INTERNAL_ERROR;
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	switch (errno) {
	case EACCES:
	case EBADF:
		return CZ_RESULT_BAD_ACCESS;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EOVERFLOW:
		return CZ_RESULT_BAD_ADDRESS;
#endif
	case ENODEV:
		return CZ_RESULT_BAD_FILE;
#if CZ_POSIX_MEMLOCK >= 0
	case EAGAIN:
		return CZ_RESULT_NO_LOCK;
#endif
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#if CZ_POSIX_VERSION >= CZ_POSIX_2001 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	case EMFILE:
		return CZ_RESULT_NO_OPEN;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case ENOTSUP:
		return CZ_RESULT_NO_SUPPORT;
#endif
	case ENXIO:
		if (flags & MAP_FIXED)
			return CZ_RESULT_BAD_ADDRESS;
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (off < 0)
			return CZ_RESULT_BAD_OFFSET;
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		if (pageSize && off & (off_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	unsigned long openMax = system_open_max();
	if (!(flags & (MAP_PRIVATE | MAP_SHARED)))
		return CZ_RESULT_BAD_ACCESS;
	if (!len)
		return CZ_RESULT_BAD_SIZE;
	if (off < 0)
		return CZ_RESULT_BAD_OFFSET;
	if (fildes < -1)
		return CZ_RESULT_BAD_ACCESS;
	if (openMax && fildes >= openMax)
		return CZ_RESULT_BAD_ACCESS;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_MUNMAP
enum CzResult czWrap_munmap(void* addr, size_t len)
{
	int r = munmap(addr, len);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if !CZ_FREE_BSD
	unsigned long pageSize = system_page_size();
#endif

#if CZ_GNU_LINUX
	switch (errno) {
	case EINVAL:
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
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
	switch (errno) {
	case EINVAL:
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_DARWIN || CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	switch (errno) {
	case EINVAL:
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ADDRESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!len)
		return CZ_RESULT_BAD_SIZE;
	if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
		return CZ_RESULT_BAD_ALIGNMENT;
	return CZ_RESULT_INTERNAL_ERROR;
#endif
}
#endif

#if CZ_WRAP_MSYNC
enum CzResult czWrap_msync(void* addr, size_t len, int flags)
{
	int r = msync(addr, len, flags);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if !CZ_FREE_BSD
	unsigned long pageSize = system_page_size();
#endif

#if CZ_DARWIN
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EIO:
		return CZ_RESULT_BAD_IO;
	case EINVAL:
		if (!len)
			return CZ_RESULT_BAD_SIZE;
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_GNU_LINUX
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
	case EINVAL:
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	case EBUSY:
		return CZ_RESULT_IN_USE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif CZ_FREE_BSD
	switch (errno) {
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
#elif CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1994
	switch (errno) {
	case ENOMEM:
		return CZ_RESULT_BAD_ADDRESS;
#if CZ_XOPEN_VERSION >= CZ_SUS_1994 && CZ_XOPEN_VERSION < CZ_SUS_1997
	case EIO:
		return CZ_RESULT_BAD_IO;
#endif
#if CZ_POSIX_VERSION >= CZ_POSIX_1996 || CZ_XOPEN_VERSION >= CZ_SUS_1997
	case EBUSY:
		return CZ_RESULT_IN_USE;
#endif
	case EINVAL:
		if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
			return CZ_RESULT_BAD_ALIGNMENT;
		return CZ_RESULT_BAD_ACCESS;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#else
	if (!len)
		return CZ_RESULT_BAD_SIZE;
	if (pageSize && (uintptr_t) addr & (uintptr_t) (pageSize - 1))
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
	if CZ_EXPECT (r != -1) {
		*res = r;
		return CZ_RESULT_SUCCESS;
	}

	switch (errno) {
	case 0:
		*res = r;
		return CZ_RESULT_SUCCESS;
	case EINVAL:
		return CZ_RESULT_NO_SUPPORT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif
