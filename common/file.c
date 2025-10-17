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

#include "file.h"
#include "alloc.h"
#include "util.h"

#if defined(_WIN32)
	#define MAX_ACCESS_SIZE UINT32_MAX
#elif defined(__APPLE__)
	#define MAX_ACCESS_SIZE INT_MAX
#elif defined(__unix__)
	#define MAX_ACCESS_SIZE SSIZE_MAX
#else
	#define MAX_ACCESS_SIZE SIZE_MAX
#endif

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(2) CZ_NULLTERM_ARG(3)
static enum CzResult fopen_wrap(FILE* restrict* stream, const char* path, const char* mode)
{
	FILE* f = fopen(path, mode);
	if CZ_EXPECT (f) {
		*stream = f;
		return CZ_RESULT_SUCCESS;
	}

#if defined(_WIN32)
	return (errno == EINVAL) ? CZ_RESULT_BAD_PATH : CZ_RESULT_NO_FILE;
#elif defined(__APPLE__)
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
#elif defined(__unix__)
	if (errno == ENOENT)
		return (mode[0] == 'r') ? CZ_RESULT_NO_FILE : CZ_RESULT_BAD_PATH;

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

CZ_NONNULL_ARGS
static enum CzResult fclose_wrap(FILE* stream)
{
	int r = fclose(stream);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if defined(__APPLE__)
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
#elif defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult fseek_wrap(FILE* stream, long offset, int origin)
{
	int r = fseek(stream, offset, origin);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if defined(__APPLE__)
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
#elif defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult ftell_wrap(long* pos, FILE* stream)
{
	long r = ftell(stream);
	if CZ_EXPECT (r != -1) {
		*pos = r;
		return CZ_RESULT_SUCCESS;
	}

#if defined(_WIN32)
	switch (errno) {
	case EBADF:
	case EINVAL:
		return CZ_RESULT_BAD_STREAM;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif defined(__APPLE__)
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
#elif defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult fread_wrap(void* buffer, size_t size, size_t count, FILE* stream)
{
	long pos;
	enum CzResult ret = ftell_wrap(&pos, stream);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t r = fread(buffer, size, count, stream);
	int eof = feof(stream);
	int err = ferror(stream);

	if CZ_EXPECT ((r || !size || !count) && !err)
		return CZ_RESULT_SUCCESS;
	if (!r && !pos && eof) // File was empty
		return CZ_RESULT_NO_FILE;
	if (!r && pos && eof) // File was at EOF before reading
		return CZ_RESULT_BAD_OFFSET;

#if defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult fwrite_wrap(const void* buffer, size_t size, size_t count, FILE* stream)
{
	size_t r = fwrite(buffer, size, count, stream);
	int err = ferror(stream);
	if CZ_EXPECT ((r == count || !size) && !err)
		return CZ_RESULT_SUCCESS;

#if defined(__unix__)
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

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult remove_wrap(const char* path)
{
	int r = remove(path);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

#if defined(_WIN32)
	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_FILE;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif defined(__APPLE__)
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
#elif defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult fileno_wrap(int* fd, FILE* stream)
{
#if defined(_WIN32)
	*fd = _fileno(stream);
	return CZ_RESULT_SUCCESS;
#elif defined(__APPLE__)
	*fd = fileno(stream);
	return CZ_RESULT_SUCCESS;
#elif defined(__unix__)
	int f = fileno(stream);
	if CZ_NOEXPECT (f == -1)
		return CZ_RESULT_BAD_STREAM;

	*fd = f;
	return CZ_RESULT_SUCCESS;
#else
	return CZ_RESULT_NO_SUPPORT;
#endif
}

CZ_NONNULL_ARGS
static enum CzResult isatty_wrap(int* res, int fd)
{
#if defined(_WIN32)
	int r = _isatty(fd);
	if CZ_NOEXPECT (!r && errno == EBADF)
		return CZ_RESULT_INTERNAL_ERROR;

	*res = r;
	return CZ_RESULT_SUCCESS;
#elif defined(__APPLE__) || defined(__unix__)
	int r = isatty(fd);
	if CZ_NOEXPECT (!r && errno != ENOTTY)
		return CZ_RESULT_INTERNAL_ERROR;

	*res = r;
	return CZ_RESULT_SUCCESS;
#else
	return CZ_RESULT_NO_SUPPORT;
#endif
}

CZ_NULLTERM_ARG(2)
static enum CzResult getExecutablePath_wrap(int* length, char* out, int capacity, int* dirnameLength)
{
	int l = wai_getExecutablePath(out, capacity, dirnameLength);
	if CZ_EXPECT (l != -1) {
		if (length)
			*length = l;
		return CZ_RESULT_SUCCESS;
	}
	return CZ_RESULT_INTERNAL_ERROR;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(2)
static enum CzResult alloc_abspath_from_relpath_to_exe(char* restrict* restrict absPath, const char* restrict relPath)
{
	int pathLen;
	enum CzResult ret = getExecutablePath_wrap(&pathLen, NULL, 0, NULL);
	if CZ_NOEXPECT (ret)
		return ret;

	char* path;
	size_t pathSize = (size_t) pathLen + strlen(relPath);
	struct CzAllocFlags flags = {0};

	ret = czAlloc((void**) &path, pathSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	int dirLen;
	ret = getExecutablePath_wrap(&pathLen, path, pathLen, &dirLen);
	if CZ_NOEXPECT (ret) {
		czFree(path);
		return ret;
	}

	path[dirLen] = 0;
	cwk_path_get_absolute(path, relPath, path, pathSize);
	*absPath = path;
	return CZ_RESULT_SUCCESS;
}

#if defined(_WIN32)
CZ_NONNULL_ARG(1, 4) CZ_NULLTERM_ARG(4)
static enum CzResult MultiByteToWideChar_wrap(
	int* size, UINT codePage, DWORD flags, LPCCH mbStr, int mbSize, LPWSTR wcStr, int wcSize)
{
	int s = MultiByteToWideChar(codePage, flags, mbStr, mbSize, wcStr, wcSize);
	if CZ_EXPECT (s) {
		*size = s;
		return CZ_RESULT_SUCCESS;
	}

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

CZ_NONNULL_ARGS
static enum CzResult get_osfhandle_wrap(intptr_t* handle, int fd)
{
	intptr_t h = _get_osfhandle(fd);
	if CZ_NOEXPECT (h == INVALID_HANDLE_VALUE)
		return CZ_RESULT_INTERNAL_ERROR;

	*handle = h;
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult GetFileAttributesExW_wrap(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info)
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
	case ERROR_NO_MORE_SEARCH_HANDLES:
	case ERROR_SHARING_BUFFER_EXCEEDED:
	case ERROR_TOO_MANY_DESCRIPTORS:
	case ERROR_TOO_MANY_MODULES:
	case ERROR_TOO_MANY_OPEN_FILES:
		return CZ_RESULT_NO_OPEN;
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

CZ_NONNULL_ARGS
static enum CzResult GetFileSizeEx_wrap(HANDLE file, PLARGE_INTEGER size)
{
	BOOL r = GetFileSizeEx(file, size);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_NETWORK_ACCESS_DENIED:
		return CZ_RESULT_BAD_ACCESS;
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

CZ_NONNULL_ARG(1, 2) CZ_NULLTERM_ARG(2)
static enum CzResult CreateFileW_wrap(
	HANDLE* file,
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
		*file = h;
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

CZ_NONNULL_ARGS
static enum CzResult CloseHandle_wrap(HANDLE handle)
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

CZ_NONNULL_ARG(1, 2)
static enum CzResult ReadFile_wrap(
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

CZ_NONNULL_ARG(1, 2)
static enum CzResult WriteFile_wrap(
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

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(2)
static enum CzResult alloc_utf16_from_utf8_win32(wchar_t* restrict* restrict utf16, const char* restrict utf8)
{
	unsigned int codePage = CP_UTF8;
	unsigned long flags = MB_ERR_INVALID_CHARS;
	const char* mbStr = utf8;
	int mbSize = -1; // utf8 is null-terminated
	wchar_t* wcStr = NULL;
	int wcSize = 0; // first get required length of utf16

	enum CzResult ret = MultiByteToWideChar_wrap(&wcSize, codePage, flags, mbStr, mbSize, wcStr, wcSize);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t allocSize = (size_t) wcSize * sizeof(wchar_t);
	struct CzAllocFlags allocFlags = {0};
	ret = czAlloc((void**) &wcStr, allocSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = MultiByteToWideChar_wrap(&wcSize, codePage, flags, mbStr, mbSize, wcStr, wcSize);
	if CZ_EXPECT (!ret)
		*utf16 = wcStr;
	else
		czFree(wcStr);
	return ret;
}

CZ_NONNULL_ARGS
static enum CzResult read_section_win32(HANDLE file, void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		LPVOID readBuffer = (PCHAR) buffer + i;
		DWORD readSize = (DWORD) ((size - i) & MAX_ACCESS_SIZE);
		ULARGE_INTEGER readOffset = {.QuadPart = offset + i};

		OVERLAPPED overlapped = {0};
		overlapped.Offset = readOffset.LowPart;
		overlapped.OffsetHigh = readOffset.HighPart;

		ret = ReadFile_wrap(file, readBuffer, readSize, NULL, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS
static enum CzResult write_section_win32(HANDLE file, const void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		LPCVOID writeBuffer = (LPCSTR) buffer + i;
		DWORD writeSize = (DWORD) ((size - i) & MAX_ACCESS_SIZE);
		ULARGE_INTEGER writeOffset = {.QuadPart = offset + i};

		OVERLAPPED overlapped = {0};
		if (offset == CZ_EOF) {
			overlapped.Offset = UINT32_MAX;
			overlapped.OffsetHigh = UINT32_MAX;
		}
		else {
			overlapped.Offset = writeOffset.LowPart;
			overlapped.OffsetHigh = writeOffset.HighPart;
		}

		ret = WriteFile_wrap(file, writeBuffer, writeSize, NULL, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult stat_wrap(const char* path, struct stat* st)
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
#if defined(__unix__)
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}

CZ_NONNULL_ARGS
static enum CzResult fstat_wrap(int fd, struct stat* st)
{
	int r = fstat(fd, st);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
#if defined(__APPLE__)
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
#endif
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(2)
static enum CzResult open_wrap(int* fd, const char* path, int flags, mode_t mode)
{
	int f = open(path, flags, mode);
	if CZ_EXPECT (f != -1) {
		*fd = f;
		return CZ_RESULT_SUCCESS;
	}

#if defined(__APPLE__)
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
#elif defined(__unix__)
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

static enum CzResult close_wrap(int fd)
{
	int r = close(fd);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case EINTR:
#if defined(__unix__)
	case EINPROGRESS:
#endif
		return CZ_RESULT_INTERRUPT;
#if defined(__unix__)
	case ENOSPC:
		return CZ_RESULT_NO_MEMORY;
	case EDQUOT:
		return CZ_RESULT_NO_QUOTA;
#endif
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}

CZ_NONNULL_ARGS
static enum CzResult pread_wrap(int fd, void* buffer, size_t size, off_t offset)
{
	ssize_t r = pread(fd, buffer, size, offset);
	if CZ_EXPECT (r > 0)
		return CZ_RESULT_SUCCESS;
	if (!r)
		return offset ? CZ_RESULT_BAD_OFFSET : CZ_RESULT_NO_FILE;

#if defined(__APPLE__)
	switch (errno) {
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EISDIR:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
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
	case ENXIO:
		return CZ_RESULT_NO_SUPPORT;
	case ETIMEDOUT:
		return CZ_RESULT_TIMEOUT;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
#elif defined(__unix__)
	switch (errno) {
	case EPERM:
		return CZ_RESULT_BAD_ACCESS;
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
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

CZ_NONNULL_ARGS
static enum CzResult write_wrap(int fd, const void* buffer, size_t size)
{
	ssize_t r = write(fd, buffer, size);
	if CZ_EXPECT (r != -1 && (size_t) r == size)
		return CZ_RESULT_SUCCESS;

#if defined(__APPLE__)
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
#elif defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult pwrite_wrap(int fd, const void* buffer, size_t size, off_t offset)
{
	ssize_t r = pwrite(fd, buffer, size, offset);
	if CZ_EXPECT (r != -1 && (size_t) r == size)
		return CZ_RESULT_SUCCESS;

#if defined(__APPLE__)
	switch (errno) {
	case EFAULT:
		return CZ_RESULT_BAD_ADDRESS;
	case EDEADLK:
	case EFBIG:
	case ESPIPE:
		return CZ_RESULT_BAD_FILE;
	case EINVAL:
		return CZ_RESULT_BAD_OFFSET;
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
#elif defined(__unix__)
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

CZ_NONNULL_ARGS
static enum CzResult read_section_posix(int fd, void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		void* readBuffer = (char*) buffer + i;
		size_t readSize = (size - i) & MAX_ACCESS_SIZE;
		off_t readOffset = (off_t) (offset + i);

		enum CzResult ret = pread_wrap(fd, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS
static enum CzResult write_next_posix(int fd, const void* restrict buffer, size_t size)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		const void* writeBuffer = (const char*) buffer + i;
		size_t writeSize = (size - i) & MAX_ACCESS_SIZE;

		enum CzResult ret = write_wrap(fd, writeBuffer, writeSize);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}

CZ_NONNULL_ARGS
static enum CzResult write_section_posix(int fd, const void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		const void* writeBuffer = (const char*) buffer + i;
		size_t writeSize = (size - i) & MAX_ACCESS_SIZE;
		off_t writeOffset = (off_t) (offset + i);

		enum CzResult ret = pwrite_wrap(fd, writeBuffer, writeSize, writeOffset);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(_WIN32)
CZ_NONNULL_ARGS
static enum CzResult stream_is_tty_win32(FILE* restrict stream, bool* restrict istty)
{
	int fd;
	enum CzResult ret = fileno_wrap(&fd, stream);
	if CZ_NOEXPECT (ret)
		return ret;
	if (fd == -2)
		goto out_not_tty;

	intptr_t handle;
	ret = get_osfhandle_wrap(&handle, fd);
	if CZ_NOEXPECT (ret)
		return ret;
	if (handle == -2)
		goto out_not_tty;

	DWORD mode;
	*istty = (bool) GetConsoleMode((HANDLE) handle, &mode); // Hope failure = not TTY
	return CZ_RESULT_SUCCESS;

out_not_tty:
	*istty = false;
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS
static enum CzResult stream_is_tty_posix(FILE* restrict stream, bool* restrict istty)
{
	int fd;
	enum CzResult ret = fileno_wrap(&fd, stream);
	if CZ_NOEXPECT (ret)
		return ret;

	int tty;
	ret = isatty_wrap(&tty, fd);
	if CZ_EXPECT (!ret)
		*istty = (bool) tty;
	return ret;
}
#endif

CZ_NONNULL_ARGS
static enum CzResult stream_is_tty_other(FILE* restrict stream, bool* restrict istty)
{
	(void) stream;
	(void) istty;
	return CZ_RESULT_NO_SUPPORT;
}

enum CzResult czStreamIsTerminal(FILE* restrict stream, bool* restrict istty)
{
#if defined(_WIN32)
	return stream_is_tty_win32(stream, istty);
#elif defined(__APPLE__) || defined(__unix__)
	return stream_is_tty_posix(stream, istty);
#else
	return stream_is_tty_other(stream, istty);
#endif
}

#if defined(_WIN32)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult file_size_win32(const char* restrict path, size_t* restrict size)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	WIN32_FILE_ATTRIBUTE_DATA attr;
	ret = GetFileAttributesExW_wrap(wcPath, GetFileExInfoStandard, &attr);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	ULARGE_INTEGER fileSize;
	fileSize.LowPart = attr.nFileSizeLow;
	fileSize.HighPart = attr.nFileSizeHigh;

	*size = (size_t) fileSize.QuadPart;
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult file_size_posix(const char* restrict path, size_t* restrict size)
{
	struct stat st;
	enum CzResult ret = stat_wrap(path, &st);
	if CZ_EXPECT (!ret)
		*size = (size_t) st.st_size;
	return ret;
}
#endif

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult file_size_other(const char* restrict path, size_t* restrict size)
{
	FILE* restrict file;
	const char* mode = "rb";
	enum CzResult ret = fopen_wrap(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	long offset = 0;
	int origin = SEEK_END; // Binary streams not guaranteed to support SEEK_END
	ret = fseek_wrap(file, offset, origin);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	long pos;
	ret = ftell_wrap(&pos, file);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	ret = fclose_wrap(file);
	if CZ_EXPECT (!ret)
		*size = (size_t) pos;
	return ret;

err_close_file:
	fclose_wrap(file);
	return ret;
}

enum CzResult czFileSize(const char* restrict path, size_t* restrict size, struct CzFileFlags flags)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	const char* realPath = path;
	char* fullPath = NULL;

	if (flags.relativeToExe && cwk_path_is_relative(path)) {
		ret = alloc_abspath_from_relpath_to_exe(&fullPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		realPath = fullPath;
	}

#if defined(_WIN32)
	ret = file_size_win32(realPath, size);
#elif defined(__APPLE__) || defined(__unix__)
	ret = file_size_posix(realPath, size);
#else
	ret = file_size_other(realPath, size);
#endif

	czFree(fullPath);
	return ret;
}

#if defined(_WIN32)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult read_file_win32(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	HANDLE file;
	DWORD access = GENERIC_READ;
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	LPSECURITY_ATTRIBUTES security = NULL;
	DWORD disposition = OPEN_EXISTING;
	DWORD flags = FILE_FLAG_SEQUENTIAL_SCAN;
	HANDLE template = NULL;

	ret = CreateFileW_wrap(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	ret = read_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return CloseHandle_wrap(file);

err_close_file:
	CloseHandle_wrap(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult read_file_posix(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	int fd;
	int flags = O_RDONLY | O_NOCTTY;
	mode_t mode = 0;

	enum CzResult ret = open_wrap(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = read_section_posix(fd, buffer, size, offset);
	if CZ_EXPECT (!ret)
		return close_wrap(fd);

	close_wrap(fd);
	return ret;
}
#endif

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult read_file_other(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	FILE* restrict file;
	const char* mode = "rb";
	enum CzResult ret = fopen_wrap(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	int origin = SEEK_SET;
	ret = fseek_wrap(file, (long) offset, origin);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	ret = fread_wrap(buffer, sizeof(char), size, file);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	return fclose_wrap(file);

err_close_file:
	fclose_wrap(file);
	return ret;
}

enum CzResult czReadFile(
	const char* restrict path, void* restrict buffer, size_t size, size_t offset, struct CzFileFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;

	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	const char* realPath = path;
	char* fullPath = NULL;

	if (flags.relativeToExe && cwk_path_is_relative(path)) {
		ret = alloc_abspath_from_relpath_to_exe(&fullPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		realPath = fullPath;
	}

#if defined(_WIN32)
	ret = read_file_win32(realPath, buffer, size, offset);
#elif defined(__APPLE__) || defined(__unix__)
	ret = read_file_posix(realPath, buffer, size, offset);
#else
	ret = read_file_other(realPath, buffer, size, offset);
#endif

	czFree(fullPath);
	return ret;
}

#if defined(_WIN32)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult truncate_write_file_win32(const char* restrict path, const void* restrict buffer, size_t size)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	HANDLE file;
	DWORD access = GENERIC_WRITE;
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	LPSECURITY_ATTRIBUTES security = NULL;
	DWORD disposition = OPEN_EXISTING;
	DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	HANDLE template = NULL;

	ret = CreateFileW_wrap(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	size_t offset = 0;
	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return CloseHandle_wrap(file);

err_close_file:
	CloseHandle_wrap(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult append_file_win32(const char* restrict path, const void* restrict buffer, size_t size)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	HANDLE file;
	DWORD access = GENERIC_WRITE;
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	LPSECURITY_ATTRIBUTES security = NULL;
	DWORD disposition = OPEN_ALWAYS;
	DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	HANDLE template = NULL;

	ret = CreateFileW_wrap(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	size_t offset = CZ_EOF;
	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return CloseHandle_wrap(file);

err_close_file:
	CloseHandle_wrap(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult overwrite_file_win32(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	HANDLE file;
	DWORD access = GENERIC_WRITE;
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	LPSECURITY_ATTRIBUTES security = NULL;
	DWORD disposition = offset ? OPEN_EXISTING : OPEN_ALWAYS;
	DWORD flags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	HANDLE template = NULL;

	ret = CreateFileW_wrap(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return CloseHandle_wrap(file);

err_close_file:
	CloseHandle_wrap(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult insert_file_win32(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	HANDLE file;
	DWORD access = GENERIC_READ | GENERIC_WRITE;
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	LPSECURITY_ATTRIBUTES security = NULL;
	DWORD disposition = offset ? OPEN_EXISTING : OPEN_ALWAYS;
	DWORD flags = FILE_ATTRIBUTE_NORMAL;
	HANDLE template = NULL;

	ret = CreateFileW_wrap(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	LARGE_INTEGER fileSizeLarge;
	ret = GetFileSizeEx_wrap(file, &fileSizeLarge);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	size_t fileSize = (size_t) fileSizeLarge.QuadPart;
	if CZ_NOEXPECT (offset > fileSize) {
		ret = CZ_RESULT_BAD_OFFSET;
		goto err_close_file;
	}

	void* restrict content = NULL;
	size_t contentSize = fileSize - offset;
	struct CzAllocFlags allocFlags = {0};

	ret = czAlloc(&content, contentSize, allocFlags);
	if CZ_NOEXPECT (ret && ret != CZ_RESULT_BAD_SIZE)
		goto err_close_file;

	ret = read_section_win32(file, content, contentSize, offset);
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	ret = write_section_win32(file, content, contentSize, offset + size);
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	czFree(content);
	czFree(wcPath);
	return CloseHandle_wrap(file);

err_free_content:
	czFree(content);
err_close_file:
	CloseHandle_wrap(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult truncate_write_file_posix(const char* restrict path, const void* restrict buffer, size_t size)
{
	int fd;
	int flags = O_WRONLY | O_CREAT | O_TRUNC | O_NOCTTY;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = open_wrap(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_next_posix(fd, buffer, size);
	if CZ_EXPECT (!ret)
		return close_wrap(fd);

	close_wrap(fd);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult append_file_posix(const char* restrict path, const void* restrict buffer, size_t size)
{
	int fd;
	int flags = O_WRONLY | O_APPEND | O_CREAT | O_NOCTTY;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = open_wrap(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_next_posix(fd, buffer, size);
	if CZ_EXPECT (!ret)
		return close_wrap(fd);

	close_wrap(fd);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult overwrite_file_posix(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	int fd;
	int flags = O_WRONLY | O_NOCTTY | (offset ? 0 : O_CREAT);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = open_wrap(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_section_posix(fd, buffer, size, offset);
	if CZ_EXPECT (!ret)
		return close_wrap(fd);

	close_wrap(fd);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult insert_file_posix(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	int fd;
	int flags = O_RDWR | O_NOCTTY | (offset ? 0 : O_CREAT);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = open_wrap(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	struct stat st;
	ret = fstat_wrap(fd, &st);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	size_t fileSize = (size_t) st.st_size;
	if CZ_NOEXPECT (offset > fileSize) {
		ret = CZ_RESULT_BAD_OFFSET;
		goto err_close_file;
	}

	void* restrict content = NULL;
	size_t contentSize = fileSize - offset;
	struct CzAllocFlags allocFlags = {0};

	ret = czAlloc(&content, contentSize, allocFlags);
	if CZ_NOEXPECT (ret && ret != CZ_RESULT_BAD_SIZE)
		goto err_close_file;

	ret = read_section_posix(fd, content, contentSize, offset);
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	ret = write_section_posix(fd, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	ret = write_section_posix(fd, content, contentSize, offset + size);
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	czFree(content);
	return close_wrap(fd);

err_free_content:
	czFree(content);
err_close_file:
	close_wrap(fd);
	return ret;
}
#endif

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult truncate_write_file_other(const char* restrict path, const void* restrict buffer, size_t size)
{
	FILE* restrict file;
	const char* mode = "wb";
	enum CzResult ret = fopen_wrap(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = fwrite_wrap(buffer, sizeof(char), size, file);
	if CZ_EXPECT (!ret)
		return fclose_wrap(file);

	fclose_wrap(file);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult append_file_other(const char* restrict path, const void* restrict buffer, size_t size)
{
	FILE* restrict file;
	const char* mode = "ab";
	enum CzResult ret = fopen_wrap(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = fwrite_wrap(buffer, sizeof(char), size, file);
	if CZ_NOEXPECT (ret) {
		fclose_wrap(file);
		return ret;
	}
	return fclose_wrap(file);
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult overwrite_file_other(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	size_t fileSize;
	enum CzResult ret = file_size_other(path, &fileSize);
	if (ret == CZ_RESULT_NO_FILE)
		return offset ? CZ_RESULT_BAD_OFFSET : truncate_write_file_other(path, buffer, size);
	if CZ_NOEXPECT (ret)
		return ret;
	if CZ_NOEXPECT (offset > fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (offset == fileSize)
		return append_file_other(path, buffer, size);

	void* restrict contents;
	size_t contentsSize = maxz(fileSize, size + offset);
	struct CzAllocFlags flags = {0};

	ret = czAlloc(&contents, contentsSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset) {
		void* readBuffer = contents;
		size_t readSize = offset;
		size_t readOffset = 0;

		ret = read_file_other(path, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto err_free_contents;
	}

	void* copyDest = (char*) contents + offset;
	memcpy(copyDest, buffer, size);

	if (size + offset < fileSize) {
		void* readBuffer = (char*) contents + offset + size;
		size_t readSize = fileSize - size - offset;
		size_t readOffset = size + offset;

		ret = read_file_other(path, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto err_free_contents;
	}

	ret = truncate_write_file_other(path, contents, contentsSize);
err_free_contents:
	czFree(contents);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult insert_file_other(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	size_t fileSize;
	enum CzResult ret = file_size_other(path, &fileSize);
	if (ret == CZ_RESULT_NO_FILE)
		return offset ? CZ_RESULT_BAD_OFFSET : truncate_write_file_other(path, buffer, size);
	if CZ_NOEXPECT (ret)
		return ret;
	if CZ_NOEXPECT (offset > fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (offset == fileSize)
		return append_file_other(path, buffer, size);

	void* restrict contents;
	size_t contentsSize = fileSize + size;
	struct CzAllocFlags flags = {0};

	ret = czAlloc(&contents, contentsSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset) {
		void* readBuffer = contents;
		size_t readSize = offset;
		size_t readOffset = 0;

		ret = read_file_other(path, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto err_free_contents;
	}

	void* copyDest = (char*) contents + offset;
	memcpy(copyDest, buffer, size);

	void* readBuffer = (char*) contents + offset + size;
	size_t readSize = fileSize - offset;
	size_t readOffset = offset;

	ret = read_file_other(path, readBuffer, readSize, readOffset);
	if CZ_EXPECT (!ret)
		ret = truncate_write_file_other(path, contents, contentsSize);

err_free_contents:
	czFree(contents);
	return ret;
}

enum CzResult czWriteFile(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset, struct CzFileFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;

	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	const char* realPath = path;
	char* fullPath = NULL;

	if (flags.relativeToExe && cwk_path_is_relative(path)) {
		ret = alloc_abspath_from_relpath_to_exe(&fullPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		realPath = fullPath;
	}

#if defined(_WIN32)
	if (flags.truncateFile)
		ret = truncate_write_file_win32(realPath, buffer, size);
	else if (offset == CZ_EOF)
		ret = append_file_win32(realPath, buffer, size);
	else if (flags.overwriteFile)
		ret = overwrite_file_win32(realPath, buffer, size, offset);
	else
		ret = insert_file_win32(realPath, buffer, size, offset);
#elif defined(__APPLE__) || defined(__unix__)
	if (flags.truncateFile)
		ret = truncate_write_file_posix(realPath, buffer, size);
	else if (offset == CZ_EOF)
		ret = append_file_posix(realPath, buffer, size);
	else if (flags.overwriteFile)
		ret = overwrite_file_posix(realPath, buffer, size, offset);
	else
		ret = insert_file_posix(realPath, buffer, size, offset);
#else
	if (flags.truncateFile)
		ret = truncate_write_file_other(realPath, buffer, size);
	else if (offset == CZ_EOF)
		ret = append_file_other(realPath, buffer, size);
	else if (flags.overwriteFile)
		ret = overwrite_file_other(realPath, buffer, size, offset);
	else
		ret = insert_file_other(realPath, buffer, size, offset);
#endif

	czFree(fullPath);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult truncate_file_other(const char* restrict path)
{
	FILE* restrict file;
	const char* mode = "wb";
	enum CzResult ret = fopen_wrap(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;
	return fclose_wrap(file);
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult zero_file_end_other(const char* restrict path, size_t size)
{
	size_t fileSize;
	enum CzResult ret = file_size_other(path, &fileSize);
	if CZ_NOEXPECT (ret)
		return ret;
	if CZ_NOEXPECT (!fileSize)
		return CZ_RESULT_NO_FILE;

	void* restrict zeroed;
	size_t zeroedSize = minz(size, fileSize);
	struct CzAllocFlags allocFlags = {0};
	allocFlags.zeroInitialise = true;

	ret = czAlloc(&zeroed, zeroedSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t offset = fileSize - zeroedSize;
	ret = overwrite_file_other(path, zeroed, zeroedSize, offset);
	czFree(zeroed);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult cut_file_end_other(const char* restrict path, size_t size)
{
	size_t fileSize;
	enum CzResult ret = file_size_other(path, &fileSize);
	if CZ_NOEXPECT (ret)
		return ret;
	if CZ_NOEXPECT (!fileSize)
		return CZ_RESULT_NO_FILE;
	if (size >= fileSize)
		return truncate_file_other(path);

	void* restrict content;
	size_t contentSize = fileSize - size;
	struct CzAllocFlags allocFlags = {0};

	ret = czAlloc(&content, contentSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t offset = 0;
	ret = read_file_other(path, content, contentSize, offset);
	if CZ_EXPECT (!ret)
		ret = truncate_write_file_other(path, content, contentSize);

	czFree(content);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult zero_file_other(const char* restrict path, size_t size, size_t offset)
{
	size_t fileSize;
	enum CzResult ret = file_size_other(path, &fileSize);
	if CZ_NOEXPECT (ret)
		return ret;
	if CZ_NOEXPECT (!fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (offset >= fileSize)
		return CZ_RESULT_BAD_OFFSET;

	void* restrict zeroed;
	size_t zeroedSize = minz(size, fileSize - offset);
	struct CzAllocFlags allocFlags = {0};
	allocFlags.zeroInitialise = true;

	ret = czAlloc(&zeroed, zeroedSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = overwrite_file_other(path, zeroed, zeroedSize, offset);
	czFree(zeroed);
	return ret;
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult cut_file_other(const char* restrict path, size_t size, size_t offset)
{
	size_t fileSize;
	enum CzResult ret = file_size_other(path, &fileSize);
	if CZ_NOEXPECT (ret)
		return ret;
	if CZ_NOEXPECT (!fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (offset >= fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size >= fileSize && !offset)
		return truncate_file_other(path);

	void* restrict content;
	size_t contentSize = maxz(size + offset, fileSize) - size;
	struct CzAllocFlags allocFlags = {0};

	ret = czAlloc(&content, contentSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset) {
		void* readBuffer = content;
		size_t readSize = offset;
		size_t readOffset = 0;

		ret = read_file_other(path, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto err_free_content;
	}
	if (fileSize > size + offset) {
		void* readBuffer = (char*) content + offset;
		size_t readSize = fileSize - size - offset;
		size_t readOffset = size + offset;

		ret = read_file_other(path, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto err_free_content;
	}

	ret = truncate_write_file_other(path, content, contentSize);
err_free_content:
	czFree(content);
	return ret;
}

enum CzResult czTrimFile(const char* restrict path, size_t size, size_t offset, struct CzFileFlags flags)
{
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;

	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	const char* realPath = path;
	char* fullPath = NULL;

	if (flags.relativeToExe && cwk_path_is_relative(path)) {
		ret = alloc_abspath_from_relpath_to_exe(&fullPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		realPath = fullPath;
	}

	if (offset == CZ_EOF && flags.overwriteFile)
		ret = zero_file_end_other(realPath, size);
	else if (offset == CZ_EOF)
		ret = cut_file_end_other(realPath, size);
	else if (flags.overwriteFile)
		ret = zero_file_other(realPath, size, offset);
	else
		ret = cut_file_other(realPath, size, offset);

	czFree(fullPath);
	return ret;
}
