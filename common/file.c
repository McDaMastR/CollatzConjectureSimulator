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
#include "wrap.h"

#if defined(_WIN32)
	#define MAX_ACCESS_SIZE UINT32_MAX
#elif defined(__APPLE__)
	#define MAX_ACCESS_SIZE INT_MAX
#elif CZ_POSIX_VERSION >= 200112L
	#define MAX_ACCESS_SIZE SSIZE_MAX
#else
	#define MAX_ACCESS_SIZE SIZE_MAX
#endif

static enum CzResult alloc_abspath_from_relpath_to_exe(char* restrict* restrict absPath, const char* restrict relPath)
{
	int pathLen;
	enum CzResult ret = czWrap_getExecutablePath(&pathLen, NULL, 0, NULL);
	if CZ_NOEXPECT (ret)
		return ret;

	char* path;
	size_t pathSize = (size_t) pathLen + strlen(relPath);
	struct CzAllocFlags flags = {0};

	ret = czAlloc((void**) &path, pathSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	int dirLen;
	ret = czWrap_getExecutablePath(NULL, path, pathLen, &dirLen);
	if CZ_NOEXPECT (ret) {
		czFree(path);
		return ret;
	}

	path[dirLen] = 0;
	cwk_path_get_absolute(path, relPath, path, pathSize);
	*absPath = path;
	return CZ_RESULT_SUCCESS;
}

#if CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR
static enum CzResult alloc_utf16_from_utf8_win32(wchar_t* restrict* restrict utf16, const char* restrict utf8)
{
	unsigned int codePage = CP_UTF8;
	unsigned long flags = MB_ERR_INVALID_CHARS;
	const char* mbStr = utf8;
	int mbSize = -1; // utf8 is null-terminated
	wchar_t* wcStr = NULL;
	int wcSize = 0; // first get required length of utf16

	enum CzResult ret = czWrap_MultiByteToWideChar(&wcSize, codePage, flags, mbStr, mbSize, wcStr, wcSize);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t allocSize = (size_t) wcSize * sizeof(wchar_t);
	struct CzAllocFlags allocFlags = {0};
	ret = czAlloc((void**) &wcStr, allocSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = czWrap_MultiByteToWideChar(&wcSize, codePage, flags, mbStr, mbSize, wcStr, wcSize);
	if CZ_EXPECT (!ret)
		*utf16 = wcStr;
	else
		czFree(wcStr);
	return ret;
}
#endif

#if CZ_WRAP_READ_FILE
static enum CzResult read_section_win32(HANDLE file, void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		LPVOID readBuffer = (PCHAR) buffer + i;
		DWORD readSize = (DWORD) ((size - i) & MAX_ACCESS_SIZE);
		ULARGE_INTEGER readOffset = {.QuadPart = offset + i};

		OVERLAPPED overlapped = {0};
		overlapped.Offset = readOffset.LowPart;
		overlapped.OffsetHigh = readOffset.HighPart;

		ret = czWrap_ReadFile(file, readBuffer, readSize, NULL, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_WRITE_FILE
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

		ret = czWrap_WriteFile(file, writeBuffer, writeSize, NULL, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_PREAD
static enum CzResult read_section_posix(int fd, void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		void* readBuffer = (char*) buffer + i;
		size_t readSize = (size - i) & MAX_ACCESS_SIZE;
		off_t readOffset = (off_t) (offset + i);

		enum CzResult ret = czWrap_pread(NULL, fd, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_WRITE
static enum CzResult write_next_posix(int fd, const void* restrict buffer, size_t size)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		const void* writeBuffer = (const char*) buffer + i;
		size_t writeSize = (size - i) & MAX_ACCESS_SIZE;

		enum CzResult ret = czWrap_write(NULL, fd, writeBuffer, writeSize);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if CZ_WRAP_PWRITE
static enum CzResult write_section_posix(int fd, const void* restrict buffer, size_t size, size_t offset)
{
	for (size_t i = 0; i < size; i += MAX_ACCESS_SIZE) {
		const void* writeBuffer = (const char*) buffer + i;
		size_t writeSize = (size - i) & MAX_ACCESS_SIZE;
		off_t writeOffset = (off_t) (offset + i);

		enum CzResult ret = czWrap_pwrite(NULL, fd, writeBuffer, writeSize, writeOffset);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#if defined(_WIN32)
static enum CzResult stream_is_tty_win32(FILE* restrict stream, bool* restrict istty)
{
	int fd;
	enum CzResult ret = czWrap_fileno(&fd, stream);
	if CZ_NOEXPECT (ret)
		return ret;
	if (fd == -2)
		goto out_not_tty;

	intptr_t handle;
	ret = czWrap_get_osfhandle(&handle, fd);
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

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult stream_is_tty_posix(FILE* restrict stream, bool* restrict istty)
{
	int fd;
	enum CzResult ret = czWrap_fileno(&fd, stream);
	if CZ_NOEXPECT (ret)
		return ret;

	int tty;
	ret = czWrap_isatty(&tty, fd);
	if CZ_EXPECT (!ret)
		*istty = (bool) tty;
	return ret;
}
#endif

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
#elif CZ_POSIX_VERSION >= 200112L
	return stream_is_tty_posix(stream, istty);
#else
	return stream_is_tty_other(stream, istty);
#endif
}

#if defined(_WIN32)
static enum CzResult file_size_win32(const char* restrict path, size_t* restrict size)
{
	wchar_t* restrict wcPath;
	enum CzResult ret = alloc_utf16_from_utf8_win32(&wcPath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	WIN32_FILE_ATTRIBUTE_DATA attr;
	ret = czWrap_GetFileAttributesExW(wcPath, GetFileExInfoStandard, &attr);
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

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult file_size_posix(const char* restrict path, size_t* restrict size)
{
	struct stat st;
	enum CzResult ret = czWrap_stat(path, &st);
	if CZ_EXPECT (!ret)
		*size = (size_t) st.st_size;
	return ret;
}
#endif

static enum CzResult file_size_other(const char* restrict path, size_t* restrict size)
{
	FILE* restrict file;
	const char* mode = "rb";
	enum CzResult ret = czWrap_fopen(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	long offset = 0;
	int origin = SEEK_END; // Binary streams not guaranteed to support SEEK_END
	ret = czWrap_fseek(file, offset, origin);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	long pos;
	ret = czWrap_ftell(&pos, file);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	ret = czWrap_fclose(file);
	if CZ_NOEXPECT (ret)
		return ret;

	*size = (size_t) pos;
	return CZ_RESULT_SUCCESS;

err_close_file:
	czWrap_fclose(file);
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
#elif CZ_POSIX_VERSION >= 200112L
	ret = file_size_posix(realPath, size);
#else
	ret = file_size_other(realPath, size);
#endif

	czFree(fullPath);
	return ret;
}

#if defined(_WIN32)
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

	ret = czWrap_CreateFileW(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	ret = read_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return czWrap_CloseHandle(file);

err_close_file:
	czWrap_CloseHandle(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult read_file_posix(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	int fd;
	int flags = O_RDONLY | O_NOCTTY;
	mode_t mode = 0;

	enum CzResult ret = czWrap_open(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = read_section_posix(fd, buffer, size, offset);
	if CZ_NOEXPECT (ret) {
		czWrap_close(fd);
		return ret;
	}
	return czWrap_close(fd);
}
#endif

static enum CzResult read_file_other(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	FILE* restrict file;
	const char* mode = "rb";
	enum CzResult ret = czWrap_fopen(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	int origin = SEEK_SET;
	ret = czWrap_fseek(file, (long) offset, origin);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	ret = czWrap_fread(NULL, buffer, sizeof(char), size, file);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	return czWrap_fclose(file);

err_close_file:
	czWrap_fclose(file);
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
#elif CZ_POSIX_VERSION >= 200112L
	ret = read_file_posix(realPath, buffer, size, offset);
#else
	ret = read_file_other(realPath, buffer, size, offset);
#endif

	czFree(fullPath);
	return ret;
}

#if defined(_WIN32)
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

	ret = czWrap_CreateFileW(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	size_t offset = 0;
	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return czWrap_CloseHandle(file);

err_close_file:
	czWrap_CloseHandle(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if defined(_WIN32)
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

	ret = czWrap_CreateFileW(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	size_t offset = CZ_EOF;
	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return czWrap_CloseHandle(file);

err_close_file:
	czWrap_CloseHandle(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if defined(_WIN32)
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

	ret = czWrap_CreateFileW(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	ret = write_section_win32(file, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	czFree(wcPath);
	return czWrap_CloseHandle(file);

err_close_file:
	czWrap_CloseHandle(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if defined(_WIN32)
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

	ret = czWrap_CreateFileW(&file, wcPath, access, shareMode, security, disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_wcpath;

	LARGE_INTEGER fileSizeLarge;
	ret = czWrap_GetFileSizeEx(file, &fileSizeLarge);
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
	return czWrap_CloseHandle(file);

err_free_content:
	czFree(content);
err_close_file:
	czWrap_CloseHandle(file);
err_free_wcpath:
	czFree(wcPath);
	return ret;
}
#endif

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult truncate_write_file_posix(const char* restrict path, const void* restrict buffer, size_t size)
{
	int fd;
	int flags = O_WRONLY | O_CREAT | O_TRUNC | O_NOCTTY;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = czWrap_open(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_next_posix(fd, buffer, size);
	if CZ_NOEXPECT (ret) {
		czWrap_close(fd);
		return ret;
	}
	return czWrap_close(fd);
}
#endif

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult append_file_posix(const char* restrict path, const void* restrict buffer, size_t size)
{
	int fd;
	int flags = O_WRONLY | O_APPEND | O_CREAT | O_NOCTTY;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = czWrap_open(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_next_posix(fd, buffer, size);
	if CZ_NOEXPECT (ret) {
		czWrap_close(fd);
		return ret;
	}
	return czWrap_close(fd);
}
#endif

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult overwrite_file_posix(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	int fd;
	int flags = O_WRONLY | O_NOCTTY | (offset ? 0 : O_CREAT);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = czWrap_open(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_section_posix(fd, buffer, size, offset);
	if CZ_NOEXPECT (ret) {
		czWrap_close(fd);
		return ret;
	}
	return czWrap_close(fd);
}
#endif

#if CZ_POSIX_VERSION >= 200112L
static enum CzResult insert_file_posix(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	int fd;
	int flags = O_RDWR | O_NOCTTY | (offset ? 0 : O_CREAT);
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	enum CzResult ret = czWrap_open(&fd, path, flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	struct stat st;
	ret = czWrap_fstat(fd, &st);
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
	return czWrap_close(fd);

err_free_content:
	czFree(content);
err_close_file:
	czWrap_close(fd);
	return ret;
}
#endif

static enum CzResult truncate_write_file_other(const char* restrict path, const void* restrict buffer, size_t size)
{
	FILE* restrict file;
	const char* mode = "wb";
	enum CzResult ret = czWrap_fopen(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = czWrap_fwrite(NULL, buffer, sizeof(char), size, file);
	if CZ_NOEXPECT (ret) {
		czWrap_fclose(file);
		return ret;
	}
	return czWrap_fclose(file);
}

static enum CzResult append_file_other(const char* restrict path, const void* restrict buffer, size_t size)
{
	FILE* restrict file;
	const char* mode = "ab";
	enum CzResult ret = czWrap_fopen(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = czWrap_fwrite(NULL, buffer, sizeof(char), size, file);
	if CZ_NOEXPECT (ret) {
		czWrap_fclose(file);
		return ret;
	}
	return czWrap_fclose(file);
}

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
	size_t contentsSize = maxz(size + offset, fileSize);
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
	if CZ_NOEXPECT (ret)
		goto err_free_contents;

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
#elif CZ_POSIX_VERSION >= 200112L
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

static enum CzResult truncate_file_other(const char* restrict path)
{
	FILE* restrict file;
	const char* mode = "wb";
	enum CzResult ret = czWrap_fopen(&file, path, mode);
	if CZ_NOEXPECT (ret)
		return ret;
	return czWrap_fclose(file);
}

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
	if CZ_NOEXPECT (ret)
		goto err_free_content;

	ret = truncate_write_file_other(path, content, contentSize);
err_free_content:
	czFree(content);
	return ret;
}

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
