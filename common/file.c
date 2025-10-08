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
#include "debug.h"
#include "util.h"

CZ_NULLTERM_ARG(2)
static enum CzResult getExecutablePath_wrap(int* length, char* out, int capacity, int* dirnameLength)
{
	int l = wai_getExecutablePath(out, capacity, dirnameLength);
	if CZ_EXPECT (l != -1) {
		if (length)
			*length = l;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(
		stderr,
		"wai_getExecutablePath failed with "
		"out 0x%016" PRIxPTR ", capacity %d, dirnameLength 0x%016" PRIxPTR " (%.3fms)",
		(uintptr_t) out, capacity, (uintptr_t) dirnameLength, t);

	return CZ_RESULT_INTERNAL_ERROR;
}

#if !defined(_WIN32) && !defined(__APPLE__) && !defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(2) CZ_NULLTERM_ARG(3)
static enum CzResult fopen_wrap(FILE** restrict stream, const char* restrict path, const char* restrict mode)
{
	FILE* f = fopen(path, mode);
	if CZ_EXPECT (f) {
		*stream = f;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "fopen failed with path '%s', mode '%s' (%.3fms)", path, mode, t);
	return CZ_RESULT_NO_FILE;
}

CZ_NONNULL_ARGS
static enum CzResult fclose_wrap(FILE* restrict stream)
{
	int r = fclose(stream);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	double t = program_time();
	log_error(stderr, "fclose failed with stream 0x%016" PRIxPTR " (%.3fms)", (uintptr_t) stream, t);
	return CZ_RESULT_INTERNAL_ERROR;
}

CZ_NONNULL_ARGS
static enum CzResult fseek_wrap(FILE* restrict stream, long offset, int origin)
{
	int r = fseek(stream, offset, origin);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	double t = program_time();
	log_error(
		stderr, "fseek failed with stream 0x%016" PRIxPTR ", offset %ld, origin %d (%.3fms)",
		(uintptr_t) stream, offset, origin, t);

	return CZ_RESULT_INTERNAL_ERROR;
}

CZ_NONNULL_ARGS
static enum CzResult ftell_wrap(long* restrict pos, FILE* restrict stream)
{
	long r = ftell(stream);
	if CZ_EXPECT (r != -1) {
		*pos = r;
		return CZ_RESULT_SUCCESS;
	}

	double t = program_time();
	log_error(stderr, "ftell failed with stream 0x%016" PRIxPTR " (%.3fms)", (uintptr_t) stream, t);
	return CZ_RESULT_INTERNAL_ERROR;
}
#endif

#if defined(_WIN32)
CZ_NONNULL_ARG(1, 4) CZ_NULLTERM_ARG(4)
static enum CzResult MultiByteToWideChar_wrap(
	int* restrict size, UINT codePage, DWORD flags, LPCCH mbString, int mbSize, LPWSTR wcString, int wcSize)
{
	int s = MultiByteToWideChar(codePage, flags, mbString, mbSize, wcString, wcSize);
	if CZ_EXPECT (s) {
		*size = s;
		return CZ_RESULT_SUCCESS;
	}

	switch (GetLastError()) {
	case ERROR_NO_UNICODE_TRANSLATION:
		return CZ_RESULT_BAD_PATH;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}

CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult GetFileAttributesExW_wrap(LPCWSTR path, GET_FILEEX_INFO_LEVELS level, LPVOID info)
{
	BOOL r = GetFileAttributesExW(path, level, info);
	if CZ_EXPECT (r)
		return CZ_RESULT_SUCCESS;

	switch (GetLastError()) {
	case ERROR_ACCESS_DENIED:
	case ERROR_DELETE_PENDING:
	case ERROR_OPERATION_IN_PROGRESS:
	case ERROR_READ_FAULT:
	case ERROR_WRITE_FAULT:
	case ERROR_WRITE_PROTECT:
		return CZ_RESULT_BAD_ACCESS;
	case ERROR_BAD_FILE_TYPE:
	case ERROR_COMPRESSED_FILE_NOT_SUPPORTED:
	case ERROR_DIRECTORY_NOT_SUPPORTED:
	case ERROR_FILE_TOO_LARGE:
	case ERROR_NOT_ALLOWED_ON_SYSTEM_FILE:
	case ERROR_RESIDENT_FILE_NOT_SUPPORTED:
		return CZ_RESULT_BAD_FILE;
	case ERROR_BAD_DEVICE_PATH:
	case ERROR_BAD_PATHNAME:
	case ERROR_BUFFER_OVERFLOW:
	case ERROR_DIR_NOT_ROOT:
	case ERROR_DIRECTORY:
	case ERROR_FILENAME_EXCED_RANGE:
	case ERROR_INVALID_NAME:
	case ERROR_LABEL_TOO_LONG:
	case ERROR_META_EXPANSION_TOO_LONG:
	case ERROR_PATH_NOT_FOUND:
	case ERROR_SHORT_NAMES_NOT_ENABLED_ON_VOLUME:
		return CZ_RESULT_BAD_PATH;
	case ERROR_DEV_NOT_EXIST:
	case ERROR_FILE_NOT_FOUND:
		return CZ_RESULT_NO_FILE;
	case ERROR_DEVICE_NO_RESOURCES:
	case ERROR_DISK_FULL:
	case ERROR_DISK_RESOURCES_EXHAUSTED:
	case ERROR_IS_JOIN_PATH:
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUT_OF_STRUCTURES:
	case ERROR_OUTOFMEMORY:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

#if defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult stat_wrap(const char* restrict path, struct stat* restrict st)
{
	int r = stat(path, st);
	if CZ_EXPECT (!r)
		return CZ_RESULT_SUCCESS;

	switch (errno) {
	case EACCES:
		return CZ_RESULT_BAD_ACCESS;
	case EOVERFLOW:
		return CZ_RESULT_BAD_FILE;
	case ELOOP:
	case ENAMETOOLONG:
	case ENOTDIR:
		return CZ_RESULT_BAD_PATH;
	case ENOENT:
		return CZ_RESULT_NO_FILE;
	case ENOMEM:
		return CZ_RESULT_NO_MEMORY;
	default:
		return CZ_RESULT_INTERNAL_ERROR;
	}
}
#endif

enum CzResult czStreamIsTerminal(FILE* restrict stream, bool* restrict istty)
{
#if defined(_WIN32)
	int fd = _fileno(stream);
	if (fd == -2) {
		*istty = false;
		return CZ_RESULT_SUCCESS;
	}

	intptr_t handle = _get_osfhandle(fd);
	if (handle == -2) {
		*istty = false;
		return CZ_RESULT_SUCCESS;
	}

	DWORD mode;
	*istty = (bool) GetConsoleMode((HANDLE) handle, &mode);
#elif defined(__APPLE__) || defined(__unix__)
	int fd = fileno(stream);
	*istty = (bool) isatty(fd);
#else
	*istty = false; // Just default to false if can't tell
#endif
	return CZ_RESULT_SUCCESS;
}

#if defined(_WIN32)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult file_size_win32(const char* restrict path, size_t* restrict size)
{
	UINT codePage = CP_UTF8;
	DWORD flags = MB_ERR_INVALID_CHARS;
	LPCCH mbStr = path;
	int mbStrLen = -1; // mbStr is null-terminated
	LPWSTR wcStr = NULL;
	int wcStrLen = 0;

	enum CzResult ret = MultiByteToWideChar_wrap(&wcStrLen, codePage, flags, mbStr, mbStrLen, wcStr, wcStrLen);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t allocSize = (size_t) wcStrLen * sizeof(WCHAR);
	struct CzAllocFlags allocFlags = {0};
	ret = czAlloc((void**) &wcStr, allocSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = MultiByteToWideChar_wrap(&wcStrLen, codePage, flags, mbStr, mbStrLen, wcStr, wcStrLen);
	if CZ_NOEXPECT (ret)
		goto out_free_wcstr;

	LPCWSTR wcPath = wcStr;
	GET_FILEEX_INFO_LEVELS infoLevel = GetFileExInfoStandard;
	WIN32_FILE_ATTRIBUTE_DATA attr;

	ret = GetFileAttributesExW_wrap(wcPath, infoLevel, &attr);
	if CZ_NOEXPECT (ret)
		goto out_free_wcstr;

	LARGE_INTEGER fileSize;
	fileSize.HighPart = attr.nFileSizeHigh;
	fileSize.LowPart = attr.nFileSizeLow;

	*size = (size_t) fileSize.QuadPart;
out_free_wcstr:
	czFree(wcStr);
	return ret;
}
#elif defined(__APPLE__) || defined(__unix__)
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult file_size_posix(const char* restrict path, size_t* restrict size)
{
	struct stat st;
	enum CzResult ret = stat_wrap(path, &st);
	if CZ_NOEXPECT (ret)
		return ret;

	*size = (size_t) st.st_size;
	return ret;
}
#else
CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1)
static enum CzResult file_size_other(const char* restrict path, size_t* restrict size)
{
	FILE* file;
	enum CzResult ret = fopen_wrap(&file, path, "rb");
	if CZ_NOEXPECT (ret)
		return ret;

	long offset = 0;
	ret = fseek_wrap(file, offset, SEEK_END); // Binary streams not guaranteed to support SEEK_END
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	long pos;
	ret = ftell_wrap(&pos, file);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	*size = (size_t) pos;
	return fclose_wrap(file);

err_close_file:
	fclose_wrap(file);
	return ret;
}
#endif

enum CzResult czFileSize(const char* restrict path, size_t* restrict size, struct CzFileFlags flags)
{
	enum CzResult ret = CZ_RESULT_SUCCESS;
	const char* realPath = path;
	char* fullPath = NULL;

	if (flags.relativeToExe && cwk_path_is_relative(path)) {
		int len;
		ret = getExecutablePath_wrap(&len, fullPath, 0, NULL);
		if CZ_NOEXPECT (ret)
			return ret;

		size_t allocSize = (size_t) len + strlen(path);
		struct CzAllocFlags allocFlags = {0};
		ret = czAlloc((void**) &fullPath, allocSize, allocFlags);
		if CZ_NOEXPECT (ret)
			return ret;

		int dirLen;
		ret = getExecutablePath_wrap(&len, fullPath, len, &dirLen);
		if CZ_NOEXPECT (ret)
			goto out_free_fullpath;

		fullPath[dirLen] = 0;
		cwk_path_get_absolute(fullPath, path, fullPath, allocSize);
		realPath = fullPath;
	}

#if defined(_WIN32)
	ret = file_size_win32(realPath, size);
#elif defined(__APPLE__) || defined(__unix__)
	ret = file_size_posix(realPath, size);
#else
	ret = file_size_other(realPath, size);
#endif

out_free_fullpath:
	czFree(fullPath);
	return ret;
}
