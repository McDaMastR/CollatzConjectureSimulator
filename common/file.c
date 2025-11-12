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

/* 
 * The maximum size of a single access (read/write operation) to a file.
 * - On Windows, access size is generally specifed with DWORD - a 32-bit unsigned integer - meaning any such access can
 *   be at most UINT32_MAX bytes.
 * - On Darwin, many IO system calls are documented to fail with EINVAL if the access size is greater than INT_MAX.
 * - On GNU/Linux, IO system calls akin to read/write are documented to transfer at most 0x7ffff000 bytes.
 * - POSIX.1-1990 (and later) specifies the result of read/write with access sizes greater than SSIZE_MAX are
 *   implementation-defined.
 * - POSIX.1-1988 specifies the result of read/write with access sizes greater than INT_MAX are implementation-defined.
 * - Standard C fseek/ftell use long integers to specify file position, so any access must not cause the file position
 *   indicator to extend past LONG_MAX.
 */
#if CZ_WIN32
#define MAX_ACCESS_SIZE ( (size_t) (UINT32_MAX) )
#elif CZ_DARWIN
#define MAX_ACCESS_SIZE ( (size_t) (INT_MAX) )
#elif CZ_GNU_LINUX
#define MAX_ACCESS_SIZE ( (size_t) (0x7ffff000) )
#elif CZ_POSIX_VERSION >= CZ_POSIX_1990
#define MAX_ACCESS_SIZE ( (size_t) (SSIZE_MAX) )
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
#define MAX_ACCESS_SIZE ( (size_t) (INT_MAX) )
#else
#define MAX_ACCESS_SIZE ( (size_t) (LONG_MAX) )
#endif

/* 
 * The maximum size of a file we can handle.
 * - Windows expresses file sizes with LARGE_INTEGER which represents signed 64-bit integers, so any file size greater
 *   than INT64_MAX cannot be handled.
 * - POSIX expresses file sizes with off_t, so any file size greater than the maximum value of off_t cannot be handled.
 * - Standard C fseek/ftell use long integers, and since we use fseek/ftell to find file sizes, any file size greater
 *   than LONG_MAX cannot be handled.
 */
#if CZ_WIN32
#define MAX_FILE_SIZE ( (size_t) (INT64_MAX) )
#elif CZ_DARWIN && CZ_DARWIN_C_SOURCE
#define MAX_FILE_SIZE ( (size_t) (OFF_MAX) )
#elif CZ_POSIX_VERSION >= CZ_POSIX_1988
#define MAX_FILE_SIZE ( (size_t) ((1ULL << (sizeof(off_t) * CHAR_BIT - 1)) - 1) )
#else
#define MAX_FILE_SIZE ( (size_t) (LONG_MAX) )
#endif

/* 
 * If a function is being repeatedly interrupted by signals, it should be called at most MAX_INTERRUPT_RETRY times
 * before we give up and return CZ_RESULT_INTERRUPT.
 */
#define MAX_INTERRUPT_RETRY 64

/* 
 * Resolves 'path' with respect to the directory containing the executable. If path is already absolute, 'resolvedPath'
 * is set to NULL. Otherwise, 'resolvedPath' is set to a newly allocated string with the resolved path, which should be
 * freed with czFree().
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2)
static enum CzResult alloc_resolve_path(char* restrict* restrict resolvedPath, const char* restrict path)
{
	if (cwk_path_is_absolute(path)) {
		*resolvedPath = NULL;
		return CZ_RESULT_SUCCESS;
	}

	int exePathSize;
	enum CzResult ret = czWrap_getExecutablePath(&exePathSize, NULL, 0, NULL);
	if CZ_NOEXPECT (ret)
		return ret;

	char* absPath;
	size_t absPathSize = (size_t) exePathSize + strlen(path);
	struct CzAllocFlags flags = {0};

	ret = czAlloc((void**) &absPath, absPathSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	int dirLen;
	ret = czWrap_getExecutablePath(NULL, absPath, exePathSize, &dirLen);
	if CZ_NOEXPECT (ret) {
		czFree(absPath);
		return ret;
	}

	absPath[dirLen] = 0;
	cwk_path_get_absolute(absPath, path, absPath, absPathSize);
	*resolvedPath = absPath;
	return CZ_RESULT_SUCCESS;
}

/**********************************************************************************************************************
 * Windows implementation                                                                                             *
 **********************************************************************************************************************/

#define HAVE_FileInfoWin32 CZ_WIN32

#if HAVE_FileInfoWin32
/* 
 * File info passed around the Windows implementation.
 * - 'file' is the one and only accessible open handle to the file.
 *   - Nonnull if open; NULL if closed.
 * - 'path' is the filepath with which 'fildes' was opened.
 *   - Nonnull, NUL-terminated, nonempty, and wide (UTF-16) if open; undefined if closed.
 * - 'access' is the access mode of the file.
 *   - Nonzero if open; undefined if closed.
 *   - Must include at least one of GENERIC_READ or GENERIC_WRITE.
 * - 'disposition' is the creation disposition of the file.
 *   - Nonzero if open; undefined if closed.
 *   - One of CREATE_NEW, CREATE_ALWAYS, OPEN_EXISTING, OPEN_ALWAYS, or TRUNCATE_EXISTING.
 *     - CREATE_NEW (fail if exists, create if not exists),
 *     - CREATE_ALWAYS (truncate if exists, create if not exists),
 *     - OPEN_EXISTING (open if exists, fail if not exists),
 *     - OPEN_ALWAYS (open if exists, create if not exists), or
 *     - TRUNCATE_EXISTING (truncate if exists, fail if not exists).
 * - 'flags' are the flags and attributes of the file.
 *   - Undefined if closed.
 * - 'fileSize' is the current size of the file in bytes.
 *   - At most MAX_FILE_SIZE (maximum value of LARGE_INTEGER) if open; undefined if closed.
 *   - Updated before appropriate (potentially fileSize-modifying) functions return.
 * - 'map' is the file mapping object to the file, if any.
 *   - Nonnull if mapped; NULL if open and unmapped; undefined if closed.
 * - 'mapProt' is the page protection with which the mapping was created.
 *   - Nonzero if mapped; undefined if unmapped.
 *   - Must include exactly one of PAGE_READONLY or PAGE_READWRITE.
 *   - May include PAGE_READONLY if 'flags' includes GENERIC_READ.
 *   - May include PAGE_READWRITE if 'flags' includes GENERIC_READ and GENERIC_WRITE.
 * - 'mapSize' is the size of the mapping in bytes.
 *   - Nonzero and at most 'fileSize' if mapped; undefined if unmapped.
 * - 'view' is the view into the file mapping.
 *   - Nonnull if viewed; NULL if mapped and unviewed; undefined if unmapped.
 * - 'viewAccess' is the access mode of the view.
 *   - Nonzero if viewed; undefined if unviewed.
 *   - Must include at least one of FILE_MAP_READ or FILE_MAP_WRITE.
 *   - May include FILE_MAP_READ if 'mapProt' includes PAGE_READONLY or PAGE_READWRITE.
 *   - May include FILE_MAP_WRITE if 'mapProt' includes PAGE_READWRITE.
 * - 'viewSize' is the size of the view in bytes.
 *   -  Nonzero and at most 'mapSize' if viewed; undefined if unviewed.
 * - 'viewOffset' is the offset into the mapping at which the view begins.
 *   - Multiple of the allocation granularity and less than 'mapSize' if viewed; undefined if unviewed.
 * - 'pageSize' is the page size of the platform.
 *   - Nonzero power-of-two if open; undefined if closed.
 * - 'allocationGranularity' is the virtual allocation granularity of the platform.
 *   - Nonzero power-of-two if open; undefined if closed.
 */
struct FileInfoWin32
{
	HANDLE file;
	PWSTR path;
	DWORD access;
	DWORD disposition;
	DWORD attributes;
	DWORD type;
	SIZE_T fileSize;

	HANDLE map;
	DWORD mapProt;
	SIZE_T mapSize;

	PVOID view;
	DWORD viewAccess;
	SIZE_T viewSize;
	SIZE_T viewOffset;

	DWORD pageSize;
	DWORD allocationGranularity;
};
#endif

#define HAVE_widen_path_win32 ( CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR )

#if HAVE_widen_path_win32
/* 
 * Translates 'path' from UTF-8 to UTF-16, and sets 'widePath' to point to newly allocated memory containing this
 * translated path. This memory should eventually be freed with czFree().
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2)
static enum CzResult widen_path_win32(PWSTR* restrict widePath, PCSTR path)
{
	UINT codePage = CP_UTF8;
	DWORD flags = MB_ERR_INVALID_CHARS;
	INT pathSize = -1; // path is NUL-terminated

	INT wcSize;
	enum CzResult ret = czWrap_MultiByteToWideChar(&wcSize, codePage, flags, path, pathSize, NULL, 0);
	if CZ_NOEXPECT (ret)
		return ret;

	PWSTR wcStr;
	SIZE_T allocSize = (SIZE_T) wcSize * sizeof(WCHAR);
	struct CzAllocFlags allocFlags = {0};

	ret = czAlloc((PVOID*) &wcStr, allocSize, allocFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = czWrap_MultiByteToWideChar(NULL, codePage, flags, path, pathSize, wcStr, wcSize);
	if CZ_NOEXPECT (ret) {
		czFree(wcStr);
		return ret;
	}

	*widePath = wcStr;
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_open_file_win32 (                   \
	CZ_WRAP_CREATE_FILE_W &&                     \
	CZ_WRAP_CLOSE_HANDLE &&                      \
	CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX && \
	CZ_WRAP_GET_FILE_SIZE_EX &&                  \
	CZ_WRAP_GET_FILE_TYPE &&                     \
	HAVE_widen_path_win32 &&                     \
	HAVE_FileInfoWin32 )

#if HAVE_open_file_win32
/* 
 * Opens a file and initialises a corresponding FileInfoWin32 instance. The file should not already be open. The
 * 'access' and 'disposition'  members of 'info' should already be set correctly. The 'path' argument need not remain
 * valid after the function returns.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(2) CZ_RW_ACCESS(1) CZ_RD_ACCESS(2)
static enum CzResult open_file_win32(struct FileInfoWin32* restrict info, PCSTR path, DWORD flags)
{
	CZ_ASSUME(info->file == NULL);

	PWSTR widePath;
	enum CzResult ret = widen_path_win32(&widePath, path);
	if CZ_NOEXPECT (ret)
		return ret;

	HANDLE file;
	DWORD shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	PSECURITY_ATTRIBUTES security = NULL;
	HANDLE template = NULL;

	ret = czWrap_CreateFileW(&file, widePath, info->access, shareMode, security, info->disposition, flags, template);
	if CZ_NOEXPECT (ret)
		goto err_free_path;

	FILE_BASIC_INFO basicInfo;
	ret = czWrap_GetFileInformationByHandleEx(file, FileBasicInfo, &basicInfo, sizeof(basicInfo));
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	LARGE_INTEGER fileSize;
	ret = czWrap_GetFileSizeEx(file, &fileSize);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	DWORD fileType;
	ret = czWrap_GetFileType(&fileType, file);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);

	info->file = file;
	info->path = widePath;
	info->attributes = basicInfo.FileAttributes;
	info->type = fileType;
	info->fileSize = (SIZE_T) fileSize.QuadPart;
	info->map = NULL;
	info->view = NULL;
	info->pageSize = sysInfo.dwPageSize;
	info->allocationGranularity = sysInfo.dwAllocationGranularity;
	return CZ_RESULT_SUCCESS;

err_close_file:
	CloseHandle(file);
err_free_path:
	czFree(widePath);
	return ret;
}
#endif

#define HAVE_close_file_win32 (   \
	CZ_WRAP_CLOSE_HANDLE &&       \
	CZ_WRAP_FLUSH_FILE_BUFFERS && \
	HAVE_FileInfoWin32 )

#if HAVE_close_file_win32
/* 
 * Flushes and closes the file. The file must currently be open and unmapped, and this should be called once all IO
 * operations on the file have executed (on the application side at least).
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult close_file_win32(struct FileInfoWin32* restrict info)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map == NULL);

	HANDLE file = info->file;
	info->file = NULL;
	czFree(info->path);

	BOOL canFlush = info->access & GENERIC_WRITE;
	BOOL shouldFlush = info->type == FILE_TYPE_DISK || info->type == FILE_TYPE_CHAR;
	if (canFlush && shouldFlush) {
		enum CzResult ret = czWrap_FlushFileBuffers(file);
		if CZ_NOEXPECT (ret) {
			CloseHandle(file);
			return ret;
		}
	}
	return czWrap_CloseHandle(file);
}
#endif

#define HAVE_truncate_all_win32 (             \
	CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE && \
	HAVE_FileInfoWin32 )

#if HAVE_truncate_all_win32
/* 
 * Shrinks or extends the file size to 'size'. Controlled failure occurs if:
 * - 'size' extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must include GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult truncate_all_win32(struct FileInfoWin32* restrict info, SIZE_T size)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (size > MAX_FILE_SIZE)
		return CZ_RESULT_BAD_OFFSET;

	LARGE_INTEGER eof = {.QuadPart = (LONGLONG) size};
	FILE_END_OF_FILE_INFO eofInfo = {0};
	eofInfo.EndOfFile = eof;

	enum CzResult ret = czWrap_SetFileInformationByHandle(info->file, FileEndOfFileInfo, &eofInfo, sizeof(eofInfo));
	if CZ_NOEXPECT (ret)
		return ret;

	info->fileSize = size;
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_map_section_win32 (     \
	CZ_WRAP_CREATE_FILE_MAPPING_W && \
	HAVE_FileInfoWin32 )

#if HAVE_map_section_win32
/* 
 * Creates a file mapping to the file. The mapping begins at the start of the file and extends for 'size' bytes. If
 * 'size' is greater than the current file size, the file size is increased to 'size'. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'size' extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open and unmapped.
 * If 'prot' includes PAGE_READONLY, the file access mode must include GENERIC_READ.
 * If 'prot' includes PAGE_READWRITE, the file access mode must include GENERIC_READ and GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult map_section_win32(struct FileInfoWin32* restrict info, SIZE_T size, DWORD prot)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE)
		return CZ_RESULT_BAD_OFFSET;

	PSECURITY_ATTRIBUTES attr = NULL;
	ULARGE_INTEGER mapSize = {.QuadPart = (ULONGLONG) size};
	PCWSTR name = NULL;

	enum CzResult ret = czWrap_CreateFileMappingW(
		&info->map, info->file, attr, prot, mapSize.HighPart, mapSize.LowPart, name);
	if CZ_NOEXPECT (ret)
		return ret;

	info->mapProt = prot;
	info->mapSize = size;
	if (size > info->fileSize)
		info->fileSize = size;
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_unmap_section_win32 ( \
	CZ_WRAP_CLOSE_HANDLE &&        \
	HAVE_FileInfoWin32 )

#if HAVE_unmap_section_win32
/* 
 * Unmaps the file mapping to the file. This should be called once all views to the mapped section have closed. The file
 * must currently be open, mapped, and unviewed.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult unmap_section_win32(struct FileInfoWin32* restrict info)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map != NULL);
	CZ_ASSUME(info->view == NULL);

	HANDLE map = info->map;
	info->map = NULL;
	return czWrap_CloseHandle(map);
}
#endif

#define HAVE_view_section_win32 ( \
	CZ_WRAP_MAP_VIEW_OF_FILE &&   \
	HAVE_FileInfoWin32 )

#if HAVE_view_section_win32
/* 
 * Creates a view into the file mapping of the file. The view contains at least the section starting at 'offset' and
 * extending for 'size' bytes. The actual view begins at 'offset' rounded down to a multiple of the allocation
 * granularity, and the difference is added to the size of the view. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the mapping.
 * - The range to view extends past the mapping.
 * 
 * The file must currently be open, mapped, and unviewed.
 * If 'access' includes FILE_MAP_READ, the mapping protection must include PAGE_READONLY or PAGE_READWRITE.
 * If 'access' includes FILE_MAP_WRITE, the mapping protection must include PAGE_READWRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult view_section_win32(struct FileInfoWin32* restrict info, SIZE_T size, SIZE_T offset, DWORD access)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map != NULL);
	CZ_ASSUME(info->view == NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->mapSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > info->mapSize - offset)
		return CZ_RESULT_BAD_OFFSET;

	info->viewAccess = access;
	info->viewSize = size + (offset & (info->allocationGranularity - 1));
	info->viewOffset = size + (offset & ~(info->allocationGranularity - 1));

	ULARGE_INTEGER viewOffset = {.QuadPart = (ULONGLONG) info->viewOffset};
	return czWrap_MapViewOfFile(
		&info->view, info->map, access, viewOffset.HighPart, viewOffset.LowPart, info->viewSize);
}
#endif

#define HAVE_unview_section_win32 ( \
	CZ_WRAP_FLUSH_VIEW_OF_FILE &&   \
	CZ_WRAP_UNMAP_VIEW_OF_FILE &&   \
	HAVE_FileInfoWin32 )

#if HAVE_unview_section_win32
/* 
 * Unmaps and flushes the view into the file mapping. This should be called once all IO operations pertaining to the
 * viewed section have executed. The file must currently be open, mapped, and viewed.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
enum CzResult unview_section_win32(struct FileInfoWin32* restrict info)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map != NULL);
	CZ_ASSUME(info->view != NULL);

	PVOID view = info->view;
	info->view = NULL;

	enum CzResult ret = czWrap_FlushViewOfFile(view, info->viewSize);
	if CZ_NOEXPECT (ret) {
		UnmapViewOfFile(view);
		return ret;
	}
	return czWrap_UnmapViewOfFile(view);
}
#endif

#define HAVE_read_section_win32 ( \
	CZ_WRAP_READ_FILE &&          \
	HAVE_FileInfoWin32 )

#if HAVE_read_section_win32
/* 
 * Reads from a section of the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file is not modified. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open.
 * The file access mode must include GENERIC_READ.
 */
CZ_NONNULL_ARGS() CZ_RD_ACCESS(1) CZ_WR_ACCESS(2, 3)
static enum CzResult read_section_win32(
	const struct FileInfoWin32* restrict info, PVOID buffer, SIZE_T size, SIZE_T offset)
{
	CZ_ASSUME(info->file != NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	DWORD lastRead = 0;
	for (SIZE_T total = 0; total < size; total += (SIZE_T) lastRead) {
		PVOID readBuffer = (PCHAR) buffer + total;
		DWORD readSize = (DWORD) ((size - total) % (MAX_ACCESS_SIZE + 1));
		ULARGE_INTEGER readOffset = {.QuadPart = (ULONGLONG) (offset + total)};

		OVERLAPPED overlapped = {0};
		overlapped.Offset = readOffset.LowPart;
		overlapped.OffsetHigh = readOffset.HighPart;

		enum CzResult ret = czWrap_ReadFile(info->file, readBuffer, readSize, &lastRead, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_write_section_win32 ( \
	CZ_WRAP_WRITE_FILE &&          \
	HAVE_FileInfoWin32 )

#if HAVE_write_section_win32
/* 
 * Writes to a section of the file starting at 'offset' and extending for 'size' bytes. If needed, the file size is
 * increased. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the file, excluding EOF.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open.
 * The file access mode must include GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult write_section_win32(
	struct FileInfoWin32* restrict info, LPCVOID buffer, SIZE_T size, SIZE_T offset)
{
	CZ_ASSUME(info->file != NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset > info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - offset)
		return CZ_RESULT_BAD_OFFSET;

	for (SIZE_T total = 0; total < size;) {
		LPCVOID writeBuffer = (LPCSTR) buffer + total;
		DWORD writeSize = (DWORD) ((size - total) % (MAX_ACCESS_SIZE + 1));
		ULARGE_INTEGER writeOffset = {.QuadPart = (ULONGLONG) (offset + total)};

		OVERLAPPED overlapped = {0};
		overlapped.Offset = writeOffset.LowPart;
		overlapped.OffsetHigh = writeOffset.HighPart;

		DWORD lastWrote;
		enum CzResult ret = czWrap_WriteFile(info->file, writeBuffer, writeSize, &lastWrote, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;

		total += (SIZE_T) lastWrote;
		if (total + offset > info->fileSize)
			info->fileSize = total + offset;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_append_section_win32 ( \
	CZ_WRAP_WRITE_FILE &&           \
	HAVE_FileInfoWin32 )

#if HAVE_append_section_win32
/* 
 * Writes to the file starting at EOF and extending for 'size' bytes. The file size is increased by 'size'. Controlled
 * failure occurs if:
 * - 'size' is zero.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open.
 * The file access mode must include GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult append_section_win32(struct FileInfoWin32* restrict info, LPCVOID buffer, SIZE_T size)
{
	CZ_ASSUME(info->file != NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - info->fileSize)
		return CZ_RESULT_BAD_OFFSET;

	for (SIZE_T total = 0; total < size;) {
		LPCVOID writeBuffer = (LPCSTR) buffer + total;
		DWORD writeSize = (DWORD) ((size - total) % (MAX_ACCESS_SIZE + 1));
		ULARGE_INTEGER writeOffset = {.QuadPart = (ULONGLONG) info->fileSize};

		OVERLAPPED overlapped = {0};
		overlapped.Offset = writeOffset.LowPart;
		overlapped.OffsetHigh = writeOffset.HighPart;

		DWORD lastWrote;
		enum CzResult ret = czWrap_WriteFile(info->file, writeBuffer, writeSize, &lastWrote, &overlapped);
		if CZ_NOEXPECT (ret)
			return ret;

		total += (SIZE_T) lastWrote;
		info->fileSize += (SIZE_T) lastWrote;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_zero_section_win32 ( \
	CZ_WRAP_DEVICE_IO_CONTROL &&  \
	HAVE_FileInfoWin32 )

#if HAVE_zero_section_win32
/* 
 * Zeros out a section of the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file size is not modified. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open.
 * The file access mode must include GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult zero_section_win32(struct FileInfoWin32* restrict info, SIZE_T size, SIZE_T offset)
{
	CZ_ASSUME(info->file != NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	FILE_ZERO_DATA_INFORMATION zeroInfo = {0};
	zeroInfo.FileOffset.QuadPart = (LONGLONG) offset;
	zeroInfo.BeyondFinalZero.QuadPart = (LONGLONG) (offset + size);

	DWORD controlCode = FSCTL_SET_ZERO_DATA;
	PVOID outBuffer = NULL;
	DWORD outBufferSize = 0;
	PDWORD bytesReturned = NULL;
	OVERLAPPED overlapped = {0};

	return czWrap_DeviceIoControl(
		info->file, controlCode, &zeroInfo, sizeof(zeroInfo), outBuffer, outBufferSize, bytesReturned, &overlapped);
}
#endif

#define HAVE_insert_section_win32 ( \
	HAVE_map_section_win32 &&       \
	HAVE_unmap_section_win32 &&     \
	HAVE_view_section_win32 &&      \
	HAVE_unview_section_win32 &&    \
	HAVE_FileInfoWin32 )

#if HAVE_insert_section_win32
/* 
 * Inserts a section into the file starting at 'offset' and extending for 'size' bytes. The file size is increased by
 * 'size'. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the file, excluding EOF.
 * - The resultant file size would exceed MAX_FILE_SIZE.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must include GENERIC_READ and GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult insert_section_win32(
	struct FileInfoWin32* restrict info, LPCVOID buffer, SIZE_T size, SIZE_T offset)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset > info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (offset == info->fileSize)
		return append_section_win32(info, buffer, size);

	SIZE_T oldSize = info->fileSize;
	SIZE_T newSize = oldSize + size;
	enum CzResult ret = map_section_win32(info, newSize, PAGE_READWRITE);
	if CZ_NOEXPECT (ret)
		return ret;

	SIZE_T viewSize = newSize - offset;
	ret = view_section_win32(info, viewSize, offset, FILE_MAP_READ | FILE_MAP_WRITE);
	if CZ_NOEXPECT (ret)
		goto err_unmap_file;

	PVOID mvDst = (PCHAR) info->view + (size + offset - info->viewOffset);
	LPCVOID mvSrc = (PCHAR) info->view + (offset - info->viewOffset);
	SIZE_T mvSize = oldSize - offset;

	memmove(mvDst, mvSrc, mvSize);

	PVOID cpDst = (PCHAR) info->view + (offset - info->viewOffset);
	LPCVOID cpSrc = buffer;
	SIZE_T cpSize = size;

	memcpy(cpDst, cpSrc, cpSize);

	ret = unview_section_win32(info);
	if CZ_NOEXPECT (ret)
		goto err_unmap_file;

	return unmap_section_win32(info);

err_unmap_file:
	unmap_section_win32(info);
	return ret;
}
#endif

#define HAVE_remove_section_win32 ( \
	HAVE_map_section_win32 &&       \
	HAVE_unmap_section_win32 &&     \
	HAVE_view_section_win32 &&      \
	HAVE_unview_section_win32 &&    \
	HAVE_truncate_all_win32 &&      \
	HAVE_FileInfoWin32 )

#if HAVE_remove_section_win32
/* 
 * Removes a section from the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file size is decreased by the size of the removed section. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must include GENERIC_READ and GENERIC_WRITE.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult remove_section_win32(struct FileInfoWin32* restrict info, SIZE_T size, SIZE_T offset)
{
	CZ_ASSUME(info->file != NULL);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (!offset && size >= info->fileSize)
		return truncate_all_win32(info, 0);
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	SIZE_T mapSize = info->fileSize;
	enum CzResult ret = map_section_win32(info, mapSize, PAGE_READWRITE);
	if CZ_NOEXPECT (ret)
		return ret;

	SIZE_T viewSize = mapSize - offset;
	ret = view_section_win32(info, viewSize, offset, FILE_MAP_READ | FILE_MAP_WRITE);
	if CZ_NOEXPECT (ret)
		goto err_unmap_file;

	PVOID mvDst = (PCHAR) info->view + (offset - info->viewOffset);
	LPCVOID mvSrc = (PCHAR) info->view + (size + offset - info->viewOffset);
	SIZE_T mvSize = mapSize - (size + offset);

	memmove(mvDst, mvSrc, mvSize);

	ret = unview_section_win32(info);
	if CZ_NOEXPECT (ret)
		goto err_unmap_file;

	ret = unmap_section_win32(info);
	if CZ_NOEXPECT (ret)
		return ret;

	SIZE_T newSize = mapSize - size;
	return truncate_all_win32(info, newSize);

err_unmap_file:
	unmap_section_win32(info);
	return ret;
}
#endif

/**********************************************************************************************************************
 * POSIX implementation                                                                                               *
 **********************************************************************************************************************/

#define HAVE_FileInfoPosix ( CZ_POSIX_VERSION >= CZ_POSIX_1988 )

#if HAVE_FileInfoPosix
/* 
 * File info passed around the POSIX implementation.
 * - 'fildes' is the one and only accessible open file descriptor to the file.
 *   - Nonnegative if open; -1 if closed.
 * - 'flags' is the flag bitfield with which 'fildes' was opened.
 *   - Must include exactly one of O_RDONLY, O_WRONLY, or O_RDWR.
 *   - Must include O_NOCTTY.
 *   - May include O_CREAT, O_NOFOLLOW, or O_TRUNC.
 * - 'mode' is the file mode as given in stat::st_mode.
 *   - Nonzero if open; undefined if closed.
 * - 'fileSize' is the current size of the file in bytes.
 *   - At most MAX_FILE_SIZE (maximum value of off_t) if open; undefined if closed.
 *   - Updated before appropriate (potentially fileSize-modifying) functions return.
 * - 'path' is the filepath with which 'fildes' was opened.
 *   - Nonnull, NUL-terminated, and nonempty.
 * - 'map' is the memory block mapped to the file, if any.
 *   - Nonnull if mapped; NULL if open and unmapped; undefined if closed.
 * - 'mapSize' is the size of the mapping in bytes.
 *   - Nonzero and at most 'fileSize' if mapped; undefined if unmapped.
 * - 'mapOffset' is the offset into the file at which the mapping begins.
 *   - Multiple of the page size and less than 'fileSize' if mapped; undefined if unmapped.
 * - 'mapProt' is the memory protection with which the mapping was created.
 *   - Nonzero if mapped; undefined if unmapped.
 *   - May include PROT_READ if 'flags' includes O_RDONLY or O_RDWR.
 *   - May include PROT_WRITE if 'flags' includes O_RDWR.
 * - 'pageSize' is the page size of the platform.
 *   - Nonzero power-of-two if open; undefined if closed.
 */
struct FileInfoPosix
{
	int fildes;
	int flags;
	mode_t mode;
	size_t fileSize;
	const char* path;

	void* map;
	int mapProt;
	size_t mapSize;
	size_t mapOffset;
	size_t pageSize;
};
#endif

#if CZ_WRAP_FSYNC
/* 
 * Repeatedly calls fsync() until it executes without being interrupted by a signal, or until it has been called
 * MAX_INTERRUPT_RETRY times.
 */
CZ_COPY_ATTR(czWrap_fsync)
static enum CzResult loop_fsync(int fd)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	for (int i = 0; i < MAX_INTERRUPT_RETRY; i++) {
		ret = czWrap_fsync(fd);
		if CZ_EXPECT (ret != CZ_RESULT_INTERRUPT)
			break;
	}
	return ret;
}
#endif

#if CZ_WRAP_FTRUNCATE
/* 
 * Repeatedly calls ftruncate() until it executes without being interrupted by a signal, or until it has been called
 * MAX_INTERRUPT_RETRY times.
 */
CZ_COPY_ATTR(czWrap_ftruncate)
static enum CzResult loop_ftruncate(int fd, off_t size)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	for (int i = 0; i < MAX_INTERRUPT_RETRY; i++) {
		ret = czWrap_ftruncate(fd, size);
		if CZ_EXPECT (ret != CZ_RESULT_INTERRUPT)
			break;
	}
	return ret;
}
#endif

#if CZ_WRAP_PREAD
/* 
 * Repeatedly calls pread() until it executes without being interrupted by a signal, or until it has been called
 * MAX_INTERRUPT_RETRY times.
 */
CZ_COPY_ATTR(czWrap_pread)
static enum CzResult loop_pread(ssize_t* restrict res, int fd, void* restrict buffer, size_t size, off_t offset)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	for (int i = 0; i < MAX_INTERRUPT_RETRY; i++) {
		ret = czWrap_pread(res, fd, buffer, size, offset);
		if CZ_EXPECT (ret != CZ_RESULT_INTERRUPT)
			break;
	}
	return ret;
}
#endif

#if CZ_WRAP_PWRITE
/* 
 * Repeatedly calls pwrite() until it executes without being interrupted by a signal, or until it has been called
 * MAX_INTERRUPT_RETRY times.
 */
CZ_COPY_ATTR(czWrap_pwrite)
static enum CzResult loop_pwrite(ssize_t* restrict res, int fd, const void* restrict buffer, size_t size, off_t offset)
{
	enum CzResult ret = CZ_RESULT_INTERNAL_ERROR;
	for (int i = 0; i < MAX_INTERRUPT_RETRY; i++) {
		ret = czWrap_pwrite(res, fd, buffer, size, offset);
		if CZ_EXPECT (ret != CZ_RESULT_INTERRUPT)
			break;
	}
	return ret;
}
#endif

#define HAVE_open_file_posix ( \
	CZ_WRAP_OPEN &&            \
	CZ_WRAP_CLOSE &&           \
	CZ_WRAP_FSTAT &&           \
	CZ_WRAP_SYSCONF &&         \
	HAVE_FileInfoPosix )

#if HAVE_open_file_posix
/* 
 * Opens a file and initialises a corresponding FileInfoPosix instance. The file should not already be open. The 'path'
 * and 'flags' members of 'info' should already be set correctly, and 'path' should remain valid until the file is
 * closed with close_file_posix().
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult open_file_posix(struct FileInfoPosix* restrict info)
{
	CZ_ASSUME(info->fildes == -1);

	int fd;
	mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	enum CzResult ret = czWrap_open(&fd, info->path, info->flags, mode);
	if CZ_NOEXPECT (ret)
		return ret;

	struct stat st;
	ret = czWrap_fstat(fd, &st);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	long pageSize;
	ret = czWrap_sysconf(&pageSize, _SC_PAGESIZE);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	info->fildes = fd;
	info->mode = st.st_mode;
	info->fileSize = (size_t) st.st_size;
	info->map = NULL;
	info->pageSize = (size_t) pageSize;
	return CZ_RESULT_SUCCESS;

err_close_file:
#if CZ_WRAP_POSIX_CLOSE
	posix_close(fd, 0);
#else
	close(fd);
#endif
	return ret;
}
#endif

#define HAVE_close_file_posix ( \
	CZ_WRAP_CLOSE &&            \
	CZ_WRAP_FSYNC &&            \
	HAVE_FileInfoPosix )

#if HAVE_close_file_posix
/* 
 * Flushes and closes the file. The file must currently be open and unmapped, and this should be called once all IO
 * operations on the file have executed (on the application side at least).
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult close_file_posix(struct FileInfoPosix* restrict info)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map == NULL);

	int fd = info->fildes;
	info->fildes = -1;

	bool canSync = S_ISREG(info->mode) || S_ISLNK(info->mode);
	if (canSync) {
		enum CzResult ret = loop_fsync(fd);
		if CZ_NOEXPECT (ret) {
#if CZ_WRAP_POSIX_CLOSE
			posix_close(fd, 0);
#else
			close(fd);
#endif
			return ret;
		}
	}

#if CZ_WRAP_POSIX_CLOSE
	return czWrap_posix_close(fd, 0);
#else
	return czWrap_close(fd);
#endif
}
#endif

#define HAVE_truncate_all_posix ( \
	CZ_WRAP_FTRUNCATE &&          \
	HAVE_FileInfoPosix )

#if HAVE_truncate_all_posix
/* 
 * Shrinks or extends the file size to 'size'. Controlled failure occurs if:
 * - 'size' extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must be O_WRONLY or O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult truncate_all_posix(struct FileInfoPosix* restrict info, size_t size)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (size > MAX_FILE_SIZE)
		return CZ_RESULT_BAD_OFFSET;

	enum CzResult ret = loop_ftruncate(info->fildes, (off_t) size);
	if CZ_NOEXPECT (ret)
		return ret;

	info->fileSize = size;
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_map_section_posix ( \
	CZ_WRAP_MMAP &&              \
	HAVE_FileInfoPosix )

#if HAVE_map_section_posix
/* 
 * Creates a memory mapping to the file. The mapping contains at least the section starting at 'offset' and extending
 * either for 'size' bytes or until EOF is reached, whichever happens first. The actual mapping begins at 'offset'
 * rounded down to a multiple of the page size, and the difference is added to the size of the mapping. Controlled
 * failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open and unmapped; only one mapping at a time can be handled.
 * If 'prot' includes PROT_READ, the file access mode must be O_RDONLY or O_RDWR.
 * If 'prot' includes PROT_WRITE, the file access mode must be O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult map_section_posix(struct FileInfoPosix* restrict info, size_t size, size_t offset, int prot)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	void* map;
	size_t mapSize = size + (offset & (info->pageSize - 1));
	off_t mapOffset = (off_t) (offset & ~(info->pageSize - 1));

	enum CzResult ret = czWrap_mmap(&map, NULL, mapSize, prot, MAP_SHARED, info->fildes, mapOffset);
	if CZ_NOEXPECT (ret)
		return ret;

	info->map = map;
	info->mapProt = prot;
	info->mapSize = mapSize;
	info->mapOffset = (size_t) mapOffset;
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_unmap_section_posix ( \
	CZ_WRAP_MSYNC &&               \
	CZ_WRAP_MUNMAP &&              \
	HAVE_FileInfoPosix )

#if HAVE_unmap_section_posix
/* 
 * Flushes and unmaps the memory mapping to the file. The file must currently be open and mapped, and this should be
 * called once all IO operations pertaining to the mapped section have executed.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult unmap_section_posix(struct FileInfoPosix* restrict info)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map != NULL);

	void* map = info->map;
	info->map = NULL;

	enum CzResult ret = czWrap_msync(map, info->mapSize, MS_SYNC);
	if CZ_NOEXPECT (ret) {
		munmap(map, info->mapSize);
		return ret;
	}
	return czWrap_munmap(map, info->mapSize);
}
#endif

#define HAVE_read_section_posix ( \
	CZ_WRAP_PREAD &&              \
	HAVE_FileInfoPosix )

#if HAVE_read_section_posix
/* 
 * Reads from a section of the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file is not modified. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open.
 * The file access mode must be O_RDONLY or O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RD_ACCESS(1) CZ_WR_ACCESS(2, 3)
static enum CzResult read_section_posix(
	const struct FileInfoPosix* restrict info, void* restrict buffer, size_t size, size_t offset)
{
	CZ_ASSUME(info->fildes >= 0);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	ssize_t lastRead = 0;
	for (size_t total = 0; total < size; total += (size_t) lastRead) {
		void* readBuffer = (char*) buffer + total;
		size_t readSize = (size - total) % (MAX_ACCESS_SIZE + 1);
		off_t readOffset = (off_t) (offset + total);

		enum CzResult ret = loop_pread(&lastRead, info->fildes, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			return ret;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_write_section_posix ( \
	CZ_WRAP_PWRITE &&              \
	HAVE_FileInfoPosix )

#if HAVE_write_section_posix
/* 
 * Writes to a section of the file starting at 'offset' and extending for 'size' bytes. If needed, the file size is
 * increased. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the file, excluding EOF.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open.
 * The file access mode must be O_WRONLY or O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult write_section_posix(
	struct FileInfoPosix* restrict info, const void* restrict buffer, size_t size, size_t offset)
{
	CZ_ASSUME(info->fildes >= 0);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset > info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - offset)
		return CZ_RESULT_BAD_OFFSET;

	for (size_t total = 0; total < size;) {
		const void* writeBuffer = (const char*) buffer + total;
		size_t writeSize = (size - total) % (MAX_ACCESS_SIZE + 1);
		off_t writeOffset = (off_t) (offset + total);

		ssize_t lastWrote;
		enum CzResult ret = loop_pwrite(&lastWrote, info->fildes, writeBuffer, writeSize, writeOffset);
		if CZ_NOEXPECT (ret)
			return ret;

		total += (size_t) lastWrote;
		if (total + offset > info->fileSize)
			info->fileSize = total + offset;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_append_section_posix ( \
	CZ_WRAP_PWRITE &&               \
	HAVE_FileInfoPosix )

#if HAVE_append_section_posix
/* 
 * Writes to the file starting at EOF and extending for 'size' bytes. The file size is increased by 'size'. Controlled
 * failure occurs if:
 * - 'size' is zero.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * The file must currently be open.
 * The file access mode must be O_WRONLY or O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult append_section_posix(struct FileInfoPosix* restrict info, const void* restrict buffer, size_t size)
{
	CZ_ASSUME(info->fildes >= 0);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - info->fileSize)
		return CZ_RESULT_BAD_OFFSET;

	for (size_t total = 0; total < size;) {
		const void* writeBuffer = (const char*) buffer + total;
		size_t writeSize = (size - total) % (MAX_ACCESS_SIZE + 1);
		off_t writeOffset = (off_t) info->fileSize;

		ssize_t lastWrote;
		enum CzResult ret = loop_pwrite(&lastWrote, info->fildes, writeBuffer, writeSize, writeOffset);
		if CZ_NOEXPECT (ret)
			return ret;

		total += (size_t) lastWrote;
		info->fileSize += (size_t) lastWrote;
	}
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_zero_section_posix ( \
	HAVE_map_section_posix &&     \
	HAVE_unmap_section_posix &&   \
	HAVE_FileInfoPosix )

#if HAVE_zero_section_posix
/* 
 * Zeros out a section of the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file size is not modified. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must be O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult zero_section_posix(struct FileInfoPosix* restrict info, size_t size, size_t offset)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	enum CzResult ret = map_section_posix(info, size, offset, PROT_WRITE);
	if CZ_NOEXPECT (ret)
		return ret;

	void* memory = (char*) info->map + (offset - info->mapOffset);
	memset(memory, 0, size);
	return unmap_section_posix(info);
}
#endif

#define HAVE_insert_section_posix ( \
	HAVE_append_section_posix &&    \
	HAVE_truncate_all_posix &&      \
	HAVE_map_section_posix &&       \
	HAVE_unmap_section_posix &&     \
	HAVE_FileInfoPosix )

#if HAVE_insert_section_posix
/* 
 * Inserts a section into the file starting at 'offset' and extending for 'size' bytes. The file size is increased by
 * 'size'. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the file, excluding EOF.
 * - The resultant file size would exceed MAX_FILE_SIZE.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must be O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult insert_section_posix(
	struct FileInfoPosix* restrict info, const void* restrict buffer, size_t size, size_t offset)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset > info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (offset == info->fileSize)
		return append_section_posix(info, buffer, size);

	size_t oldSize = info->fileSize;
	size_t newSize = oldSize + size;
	enum CzResult ret = truncate_all_posix(info, newSize);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t mapSize = newSize - offset;
	ret = map_section_posix(info, mapSize, offset, PROT_READ | PROT_WRITE);
	if CZ_NOEXPECT (ret)
		return ret;

	void* mvDst = (char*) info->map + (size + offset - info->mapOffset);
	const void* mvSrc = (char*) info->map + (offset - info->mapOffset);
	size_t mvSize = oldSize - offset;

	memmove(mvDst, mvSrc, mvSize);

	void* cpDst = (char*) info->map + (offset - info->mapOffset);
	const void* cpSrc = buffer;
	size_t cpSize = size;

	memcpy(cpDst, cpSrc, cpSize);

	return unmap_section_posix(info);
}
#endif

#define HAVE_remove_section_posix ( \
	HAVE_truncate_all_posix &&      \
	HAVE_map_section_posix &&       \
	HAVE_unmap_section_posix &&     \
	HAVE_FileInfoPosix )

#if HAVE_remove_section_posix
/* 
 * Removes a section from the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file size is decreased by the size of the removed section. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file must currently be open and unmapped.
 * The file access mode must be O_RDWR.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult remove_section_posix(struct FileInfoPosix* restrict info, size_t size, size_t offset)
{
	CZ_ASSUME(info->fildes >= 0);
	CZ_ASSUME(info->map == NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (!offset && size >= info->fileSize)
		return truncate_all_posix(info, 0);
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	size_t mapSize = info->fileSize - offset;
	enum CzResult ret = map_section_posix(info, mapSize, offset, PROT_READ | PROT_WRITE);
	if CZ_NOEXPECT (ret)
		return ret;

	void* mvDst = (char*) info->map + (offset - info->mapOffset);
	const void* mvSrc = (char*) info->map + (size + offset - info->mapOffset);
	size_t mvSize = info->fileSize - (size + offset);

	memmove(mvDst, mvSrc, mvSize);

	ret = unmap_section_posix(info);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t newSize = info->fileSize - size;
	return truncate_all_posix(info, newSize);
}
#endif

/**********************************************************************************************************************
 * Standard C implementation                                                                                          *
 **********************************************************************************************************************/

/* 
 * File info passed around the stdc implementation.
 * - 'stream' is the one and only accessible open IO stream to the file.
 *   - Nonnull if open; NULL if closed.
 * - 'path' is the filepath with which 'stream' was opened.
 *   - Nonnull, NUL-terminated, and nonempty.
 * - 'mode' is the access mode with which 'stream' was opened.
 *   - One of
 *     - "rb" (read-only),
 *     - "wb" (truncate, write-only),
 *     - "r+b" (read-write), or
 *     - "w+b" (truncate, read-write).
 * - 'fileSize' is the current size of the file in bytes.
 *   - At most MAX_FILE_SIZE (LONG_MAX) if open; undefined if closed.
 *   - Updated before appropriate (potentially fileSize-modifying) functions return.
 */
struct FileInfoStdc
{
	FILE* stream;
	const char* path;
	const char* mode;
	size_t fileSize;
};

/* 
 * Opens a file and initialises a corresponding FileInfoStdc instance. The file should not already be open. The 'path'
 * and 'mode' members of 'info' should already be set correctly, and should remain valid until the file is closed with
 * close_file_stdc().
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult open_file_stdc(struct FileInfoStdc* restrict info)
{
	CZ_ASSUME(info->stream == NULL);

	FILE* stream;
	enum CzResult ret = czWrap_fopen(&stream, info->path, info->mode);
	if CZ_NOEXPECT (ret)
		return ret;

	long offset = 0;
	ret = czWrap_fseek(stream, offset, SEEK_END);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	long pos;
	ret = czWrap_ftell(&pos, stream);
	if CZ_NOEXPECT (ret)
		goto err_close_file;

	info->stream = stream;
	info->fileSize = (size_t) pos;
	return CZ_RESULT_SUCCESS;

err_close_file:
	fclose(stream);
	return ret;
}

/* 
 * Flushes and closes the file. The file should currently be open, and this should be called once all IO operations on
 * the file have executed (on the application side at least).
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult close_file_stdc(struct FileInfoStdc* restrict info)
{
	CZ_ASSUME(info->stream != NULL);

	FILE* stream = info->stream;
	info->stream = NULL;

	enum CzResult ret = czWrap_fflush(stream);
	if CZ_NOEXPECT (ret) {
		fclose(stream);
		return ret;
	}
	return czWrap_fclose(stream);
}

/* 
 * Reads from a section of the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file is not modified. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file access mode must be "rb", "r+b", or "w+b".
 */
CZ_NONNULL_ARGS() CZ_RD_ACCESS(1) CZ_WR_ACCESS(2, 3)
static enum CzResult read_section_stdc(
	const struct FileInfoStdc* restrict info, void* restrict buffer, size_t size, size_t offset)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	enum CzResult ret = czWrap_fseek(info->stream, (long) offset, SEEK_SET);
	return ret ?: czWrap_fread(NULL, buffer, sizeof(char), size, info->stream);
}

/* 
 * Writes to a section of the file starting at 'offset' and extending for 'size' bytes. If needed, the file size is
 * increased. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the file, excluding EOF.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * The file access mode must be "wb", "r+b", or "w+b".
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult write_section_stdc(
	struct FileInfoStdc* restrict info, const void* restrict buffer, size_t size, size_t offset)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset > info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - offset)
		return CZ_RESULT_BAD_OFFSET;

	enum CzResult ret = czWrap_fseek(info->stream, (long) offset, SEEK_SET);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t wrote;
	ret = czWrap_fwrite(&wrote, buffer, sizeof(char), size, info->stream);
	if (wrote + offset > info->fileSize)
		info->fileSize = wrote + offset;
	return ret;
}

/* 
 * Writes to the file starting at EOF and extending for 'size' bytes. The file size is increased by 'size'. Controlled
 * failure occurs if:
 * - 'size' is zero.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * The file access mode must be "wb", "r+b", or "w+b".
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult append_section_stdc(struct FileInfoStdc* restrict info, const void* restrict buffer, size_t size)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - info->fileSize)
		return CZ_RESULT_BAD_OFFSET;

	enum CzResult ret = czWrap_fseek(info->stream, (long) info->fileSize, SEEK_SET);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t wrote;
	ret = czWrap_fwrite(&wrote, buffer, sizeof(char), size, info->stream);
	info->fileSize += wrote;
	return ret;
}

/* 
 * Destroys the entire file. The file size is set to zero.
 * 
 * If the file access mode is "rb" or "r+b", it is changed to "w+b".
 * If the file access mode is "wb" or "w+b", it is not changed.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult remove_all_stdc(struct FileInfoStdc* restrict info)
{
	CZ_ASSUME(info->stream != NULL);

	if (info->mode[0] == 'r')
		info->mode = "w+b";

	enum CzResult ret = czWrap_freopen(info->path, info->mode, info->stream);
	if CZ_NOEXPECT (ret) {
		info->stream = NULL;
		return ret;
	}

	info->fileSize = 0;
	return CZ_RESULT_SUCCESS;
}

/* 
 * Overwrites the entire file. The file size is set to 'size'. Controlled failure occurs if:
 * - 'size' is zero.
 * - The range to write extends past MAX_FILE_SIZE.
 * 
 * If the file access mode is "rb" or "r+b", it is changed to "w+b".
 * If the file access mode is "wb" or "w+b", it is not changed.
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult write_all_stdc(struct FileInfoStdc* restrict info, const void* restrict buffer, size_t size)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE)
		return CZ_RESULT_BAD_OFFSET;

	enum CzResult ret = remove_all_stdc(info);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t wrote;
	ret = czWrap_fwrite(&wrote, buffer, sizeof(char), size, info->stream);
	info->fileSize = wrote;
	return ret;
}

/* 
 * Zeros out a section of the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file size is not modified. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file access mode must be "wb", "r+b", or "w+b".
 */
CZ_NONNULL_ARGS() CZ_RD_ACCESS(1)
static enum CzResult zero_section_stdc(struct FileInfoStdc* restrict info, size_t size, size_t offset)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	void* restrict buffer;
	struct CzAllocFlags flags = {0};
	flags.zeroInitialise = true;

	enum CzResult ret = czAlloc(&buffer, size, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = write_section_stdc(info, buffer, size, offset);
	czFree(buffer);
	return ret;
}

/* 
 * Inserts a section into the file starting at 'offset' and extending for 'size' bytes. The file size is increased by
 * 'size'. Controlled failure occurs if:
 * - 'size' is zero.
 * - 'offset' is outside the file, excluding EOF.
 * - The resultant file size would exceed MAX_FILE_SIZE.
 * 
 * The file access mode must be "r+b" or "w+b".
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1) CZ_RD_ACCESS(2, 3)
static enum CzResult insert_section_stdc(
	struct FileInfoStdc* restrict info, const void* restrict buffer, size_t size, size_t offset)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset > info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if CZ_NOEXPECT (size > MAX_FILE_SIZE - info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (offset == info->fileSize)
		return append_section_stdc(info, buffer, size);

	void* restrict content;
	size_t contentSize = info->fileSize - offset;
	struct CzAllocFlags flags = {0};

	enum CzResult ret = czAlloc(&content, contentSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = read_section_stdc(info, content, contentSize, offset);
	if CZ_NOEXPECT (ret)
		goto out_free_content;

	ret = write_section_stdc(info, buffer, size, offset);
	if CZ_NOEXPECT (ret)
		goto out_free_content;

	ret = write_section_stdc(info, content, contentSize, size + offset);
out_free_content:
	czFree(content);
	return ret;
}

/* 
 * Removes a section from the file starting at 'offset' and extending either for 'size' bytes or until EOF is reached,
 * whichever happens first. The file size is decreased by the size of the removed section. Controlled failure occurs if:
 * - The file is empty.
 * - 'size' is zero.
 * - 'offset' is outside the file, including EOF.
 * 
 * The file access mode must be "rb", "r+b", or "w+b", and is changed to "w+b".
 */
CZ_NONNULL_ARGS() CZ_RW_ACCESS(1)
static enum CzResult remove_section_stdc(struct FileInfoStdc* restrict info, size_t size, size_t offset)
{
	CZ_ASSUME(info->stream != NULL);

	if CZ_NOEXPECT (!info->fileSize)
		return CZ_RESULT_NO_FILE;
	if CZ_NOEXPECT (!size)
		return CZ_RESULT_BAD_SIZE;
	if CZ_NOEXPECT (offset >= info->fileSize)
		return CZ_RESULT_BAD_OFFSET;
	if (!offset && size >= info->fileSize)
		return remove_all_stdc(info);
	if (size > info->fileSize - offset)
		size = info->fileSize - offset;

	void* restrict content;
	size_t contentSize = info->fileSize - size;
	struct CzAllocFlags flags = {0};

	enum CzResult ret = czAlloc(&content, contentSize, flags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset) {
		void* readBuffer = content;
		size_t readSize = offset;
		size_t readOffset = 0;

		ret = read_section_stdc(info, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto out_free_content;
	}
	if (size + offset < info->fileSize) {
		void* readBuffer = (char*) content + offset;
		size_t readSize = info->fileSize - (size + offset);
		size_t readOffset = size + offset;

		ret = read_section_stdc(info, readBuffer, readSize, readOffset);
		if CZ_NOEXPECT (ret)
			goto out_free_content;
	}

	ret = write_all_stdc(info, content, contentSize);
out_free_content:
	czFree(content);
	return ret;
}

/**********************************************************************************************************************
 * API function definitions                                                                                           *
 **********************************************************************************************************************/

#define HAVE_czFileSize_win32 ( \
	HAVE_open_file_win32 &&     \
	HAVE_close_file_win32 &&    \
	HAVE_FileInfoWin32 )

#if HAVE_czFileSize_win32
CZ_COPY_ATTR(czFileSize)
static enum CzResult czFileSize_win32(PCSTR path, PSIZE_T size, struct CzFileFlags flags)
{
	struct FileInfoWin32 info = {0};
	info.access = GENERIC_READ;
	info.disposition = OPEN_EXISTING;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL;
	if (flags.openSymLink)
		openFlags |= FILE_FLAG_OPEN_REPARSE_POINT;

	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	SIZE_T fileSize = info.fileSize;
	ret = close_file_win32(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	*size = fileSize;
	return CZ_RESULT_SUCCESS;
}
#endif

#define HAVE_czFileSize_posix ( \
	CZ_WRAP_LSTAT &&            \
	CZ_WRAP_STAT )

#if HAVE_czFileSize_posix
CZ_COPY_ATTR(czFileSize)
static enum CzResult czFileSize_posix(const char* restrict path, size_t* restrict size, struct CzFileFlags flags)
{
	struct stat st;
	enum CzResult ret;
	if (flags.openSymLink)
		ret = czWrap_lstat(path, &st);
	else
		ret = czWrap_stat(path, &st);

	if CZ_NOEXPECT (ret)
		return ret;

	*size = (size_t) st.st_size;
	return CZ_RESULT_SUCCESS;
}
#endif

CZ_COPY_ATTR(czFileSize)
static enum CzResult czFileSize_stdc(const char* restrict path, size_t* restrict size, struct CzFileFlags flags)
{
	if (flags.openSymLink)
		return CZ_RESULT_NO_SUPPORT;

	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "rb";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	size_t fileSize = info.fileSize;
	ret = close_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	*size = fileSize;
	return CZ_RESULT_SUCCESS;
}

enum CzResult czFileSize(const char* restrict path, size_t* restrict size, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czFileSize_win32
	ret = czFileSize_win32(resolvedPath, size, flags);
#elif HAVE_czFileSize_posix
	ret = czFileSize_posix(resolvedPath, size, flags);
#else
	ret = czFileSize_stdc(resolvedPath, size, flags);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czReadFile_win32 ( \
	HAVE_open_file_win32 &&     \
	HAVE_close_file_win32 &&    \
	HAVE_read_section_win32 &&  \
	HAVE_FileInfoWin32 )

#if HAVE_czReadFile_win32
CZ_COPY_ATTR(czReadFile)
static enum CzResult czReadFile_win32(PCSTR path, PVOID buffer, SIZE_T size, SIZE_T offset)
{
	struct FileInfoWin32 info = {0};
	info.access = GENERIC_READ;
	info.disposition = OPEN_EXISTING;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = read_section_win32(&info, buffer, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_win32(&info);
		return ret;
	}
	return close_file_win32(&info);
}
#endif

#define HAVE_czReadFile_posix ( \
	HAVE_open_file_posix &&     \
	HAVE_close_file_posix &&    \
	HAVE_read_section_posix &&  \
	HAVE_FileInfoPosix )

#if HAVE_czReadFile_posix
CZ_COPY_ATTR(czReadFile)
static enum CzResult czReadFile_posix(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	struct FileInfoPosix info = {0};
	info.fildes = -1;
	info.flags = O_NOCTTY | O_RDONLY;
	info.path = path;

	enum CzResult ret = open_file_posix(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = read_section_posix(&info, buffer, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_posix(&info);
		return ret;
	}
	return close_file_posix(&info);
}
#endif

CZ_COPY_ATTR(czReadFile)
static enum CzResult czReadFile_stdc(const char* restrict path, void* restrict buffer, size_t size, size_t offset)
{
	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "rb";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = read_section_stdc(&info, buffer, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_stdc(&info);
		return ret;
	}
	return close_file_stdc(&info);
}

enum CzResult czReadFile(
	const char* restrict path, void* restrict buffer, size_t size, size_t offset, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czReadFile_win32
	ret = czReadFile_win32(resolvedPath, buffer, size, offset);
#elif HAVE_czReadFile_posix
	ret = czReadFile_posix(resolvedPath, buffer, size, offset);
#else
	ret = czReadFile_stdc(resolvedPath, buffer, size, offset);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czWriteFile_win32 ( \
	HAVE_open_file_win32 &&      \
	HAVE_close_file_win32 &&     \
	HAVE_append_section_win32 && \
	HAVE_write_section_win32 &&  \
	HAVE_FileInfoWin32 )

#if HAVE_czWriteFile_win32
CZ_COPY_ATTR(czWriteFile)
static enum CzResult czWriteFile_win32(PCSTR path, LPCVOID buffer, SIZE_T size, SIZE_T offset)
{
	struct FileInfoWin32 info = {0};
	info.access = GENERIC_WRITE;
	info.disposition = (!offset || offset == CZ_EOF) ? OPEN_ALWAYS : OPEN_EXISTING;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		ret = append_section_win32(&info, buffer, size);
	else
		ret = write_section_win32(&info, buffer, size, offset);

	if CZ_NOEXPECT (ret) {
		close_file_win32(&info);
		return ret;
	}
	return close_file_win32(&info);
}
#endif

#define HAVE_czWriteFile_posix ( \
	HAVE_open_file_posix &&      \
	HAVE_close_file_posix &&     \
	HAVE_append_section_posix && \
	HAVE_write_section_posix &&  \
	HAVE_FileInfoPosix )

#if HAVE_czWriteFile_posix
CZ_COPY_ATTR(czWriteFile)
static enum CzResult czWriteFile_posix(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	int openFlags = O_NOCTTY | O_WRONLY;
	if (!offset || offset == CZ_EOF)
		openFlags |= O_CREAT;

	struct FileInfoPosix info = {0};
	info.fildes = -1;
	info.flags = openFlags;
	info.path = path;

	enum CzResult ret = open_file_posix(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		ret = append_section_posix(&info, buffer, size);
	else
		ret = write_section_posix(&info, buffer, size, offset);

	if CZ_NOEXPECT (ret) {
		close_file_posix(&info);
		return ret;
	}
	return close_file_posix(&info);
}
#endif

CZ_COPY_ATTR(czWriteFile)
static enum CzResult czWriteFile_stdc(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "r+b";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		ret = append_section_stdc(&info, buffer, size);
	else
		ret = write_section_stdc(&info, buffer, size, offset);

	if CZ_NOEXPECT (ret) {
		close_file_stdc(&info);
		return ret;
	}
	return close_file_stdc(&info);
}

enum CzResult czWriteFile(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czWriteFile_win32
	ret = czWriteFile_win32(resolvedPath, buffer, size, offset);
#elif HAVE_czWriteFile_posix
	ret = czWriteFile_posix(resolvedPath, buffer, size, offset);
#else
	ret = czWriteFile_stdc(resolvedPath, buffer, size, offset);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czInsertFile_win32 ( \
	HAVE_open_file_win32 &&       \
	HAVE_close_file_win32 &&      \
	HAVE_append_section_win32 &&  \
	HAVE_insert_section_win32 &&  \
	HAVE_FileInfoWin32 )

#if HAVE_czInsertFile_win32
CZ_COPY_ATTR(czInsertFile)
static enum CzResult czInsertFile_win32(PCSTR path, LPCVOID buffer, SIZE_T size, SIZE_T offset)
{
	DWORD access = GENERIC_WRITE;
	if (offset != CZ_EOF)
		access |= GENERIC_READ;

	struct FileInfoWin32 info = {0};
	info.access = access;
	info.disposition = (!offset || offset == CZ_EOF) ? OPEN_ALWAYS : OPEN_EXISTING;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL;
	if (offset == CZ_EOF)
		openFlags |= FILE_FLAG_SEQUENTIAL_SCAN;
	else
		openFlags |= FILE_FLAG_RANDOM_ACCESS;

	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		ret = append_section_win32(&info, buffer, size);
	else
		ret = insert_section_win32(&info, buffer, size, offset);

	if CZ_NOEXPECT (ret) {
		close_file_win32(&info);
		return ret;
	}
	return close_file_win32(&info);
}
#endif

#define HAVE_czInsertFile_posix ( \
	HAVE_open_file_posix &&       \
	HAVE_close_file_posix &&      \
	HAVE_append_section_posix &&  \
	HAVE_insert_section_posix &&  \
	HAVE_FileInfoPosix )

#if HAVE_czInsertFile_posix
CZ_COPY_ATTR(czInsertFile)
static enum CzResult czInsertFile_posix(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	int openFlags = O_NOCTTY;
	openFlags |= (offset == CZ_EOF) ? O_WRONLY : O_RDWR;
	if (!offset || offset == CZ_EOF)
		openFlags |= O_CREAT;

	struct FileInfoPosix info = {0};
	info.fildes = -1;
	info.flags = openFlags;
	info.path = path;

	enum CzResult ret = open_file_posix(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		ret = append_section_posix(&info, buffer, size);
	else
		ret = insert_section_posix(&info, buffer, size, offset);

	if CZ_NOEXPECT (ret) {
		close_file_posix(&info);
		return ret;
	}
	return close_file_posix(&info);
}
#endif

CZ_COPY_ATTR(czInsertFile)
static enum CzResult czInsertFile_stdc(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset)
{
	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "r+b";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		ret = append_section_stdc(&info, buffer, size);
	else
		ret = insert_section_stdc(&info, buffer, size, offset);

	if CZ_NOEXPECT (ret) {
		close_file_stdc(&info);
		return ret;
	}
	return close_file_stdc(&info);
}

enum CzResult czInsertFile(
	const char* restrict path, const void* restrict buffer, size_t size, size_t offset, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czInsertFile_win32
	ret = czInsertFile_win32(resolvedPath, buffer, size, offset);
#elif HAVE_czInsertFile_posix
	ret = czInsertFile_posix(resolvedPath, buffer, size, offset);
#else
	ret = czInsertFile_stdc(resolvedPath, buffer, size, offset);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czRewriteFile_win32 ( \
	HAVE_open_file_win32 &&        \
	HAVE_close_file_win32 &&       \
	HAVE_append_section_win32 &&   \
	HAVE_FileInfoWin32 )

#if HAVE_czRewriteFile_win32
CZ_COPY_ATTR(czRewriteFile)
static enum CzResult czRewriteFile_win32(PCSTR path, LPCVOID buffer, SIZE_T size)
{
	struct FileInfoWin32 info = {0};
	info.access = GENERIC_WRITE;
	info.disposition = CREATE_ALWAYS;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = append_section_win32(&info, buffer, size);
	if CZ_NOEXPECT (ret) {
		close_file_win32(&info);
		return ret;
	}
	return close_file_win32(&info);
}
#endif

#define HAVE_czRewriteFile_posix ( \
	HAVE_open_file_posix &&        \
	HAVE_close_file_posix &&       \
	HAVE_append_section_posix &&   \
	HAVE_FileInfoPosix )

#if HAVE_czRewriteFile_posix
CZ_COPY_ATTR(czRewriteFile)
static enum CzResult czRewriteFile_posix(const char* restrict path, const void* restrict buffer, size_t size)
{
	struct FileInfoPosix info = {0};
	info.fildes = -1;
	info.flags = O_NOCTTY | O_WRONLY | O_CREAT | O_TRUNC;
	info.path = path;

	enum CzResult ret = open_file_posix(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = append_section_posix(&info, buffer, size);
	if CZ_NOEXPECT (ret) {
		close_file_posix(&info);
		return ret;
	}
	return close_file_posix(&info);
}
#endif

CZ_COPY_ATTR(czRewriteFile)
static enum CzResult czRewriteFile_stdc(const char* restrict path, const void* restrict buffer, size_t size)
{
	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "wb";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	ret = append_section_stdc(&info, buffer, size);
	if CZ_NOEXPECT (ret) {
		close_file_stdc(&info);
		return ret;
	}
	return close_file_stdc(&info);
}

enum CzResult czRewriteFile(const char* path, const void* buffer, size_t size, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czRewriteFile_win32
	ret = czRewriteFile_win32(resolvedPath, buffer, size);
#elif HAVE_czRewriteFile_posix
	ret = czRewriteFile_posix(resolvedPath, buffer, size);
#else
	ret = czRewriteFile_stdc(resolvedPath, buffer, size);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czClearFile_win32 ( \
	HAVE_open_file_win32 &&      \
	HAVE_close_file_win32 &&     \
	HAVE_zero_section_win32 &&   \
	HAVE_FileInfoWin32 )

#if HAVE_czClearFile_win32
CZ_COPY_ATTR(czClearFile)
static enum CzResult czClearFile_win32(PCSTR path, SIZE_T size, SIZE_T offset)
{
	struct FileInfoWin32 info = {0};
	info.access = GENERIC_WRITE;
	info.disposition = OPEN_EXISTING;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN;
	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = zero_section_win32(&info, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_win32(&info);
		return ret;
	}
	return close_file_win32(&info);
}
#endif

#define HAVE_czClearFile_posix ( \
	HAVE_open_file_posix &&      \
	HAVE_close_file_posix &&     \
	HAVE_zero_section_posix &&   \
	HAVE_FileInfoPosix )

#if HAVE_czClearFile_posix
CZ_COPY_ATTR(czClearFile)
static enum CzResult czClearFile_posix(const char* restrict path, size_t size, size_t offset)
{
	struct FileInfoPosix info = {0};
	info.fildes = -1;
	info.flags = O_NOCTTY | O_RDWR;
	info.path = path;

	enum CzResult ret = open_file_posix(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = zero_section_posix(&info, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_posix(&info);
		return ret;
	}
	return close_file_posix(&info);
}
#endif

CZ_COPY_ATTR(czClearFile)
static enum CzResult czClearFile_stdc(const char* restrict path, size_t size, size_t offset)
{
	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "r+b";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = zero_section_stdc(&info, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_stdc(&info);
		return ret;
	}
	return close_file_stdc(&info);
}

enum CzResult czClearFile(const char* restrict path, size_t size, size_t offset, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czClearFile_win32
	ret = czClearFile_win32(resolvedPath, size, offset);
#elif HAVE_czClearFile_posix
	ret = czClearFile_posix(resolvedPath, size, offset);
#else
	ret = czClearFile_stdc(resolvedPath, size, offset);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czTrimFile_win32 (  \
	HAVE_open_file_win32 &&      \
	HAVE_close_file_win32 &&     \
	HAVE_remove_section_win32 && \
	HAVE_FileInfoWin32 )

#if HAVE_czTrimFile_win32
CZ_COPY_ATTR(czTrimFile)
static enum CzResult czTrimFile_win32(PCSTR path, SIZE_T size, SIZE_T offset)
{
	struct FileInfoWin32 info = {0};
	info.access = GENERIC_READ | GENERIC_WRITE;
	info.disposition = OPEN_EXISTING;

	DWORD openFlags = FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS;
	enum CzResult ret = open_file_win32(&info, path, openFlags);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = remove_section_win32(&info, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_win32(&info);
		return ret;
	}
	return close_file_win32(&info);
}
#endif

#define HAVE_czTrimFile_posix (  \
	HAVE_open_file_posix &&      \
	HAVE_close_file_posix &&     \
	HAVE_remove_section_posix && \
	HAVE_FileInfoPosix )

#if HAVE_czTrimFile_posix
CZ_COPY_ATTR(czTrimFile)
static enum CzResult czTrimFile_posix(const char* restrict path, size_t size, size_t offset)
{
	struct FileInfoPosix info = {0};
	info.fildes = -1;
	info.flags = O_NOCTTY | O_RDWR;
	info.path = path;

	enum CzResult ret = open_file_posix(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = remove_section_posix(&info, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_posix(&info);
		return ret;
	}
	return close_file_posix(&info);
}
#endif

CZ_COPY_ATTR(czTrimFile)
static enum CzResult czTrimFile_stdc(const char* restrict path, size_t size, size_t offset)
{
	struct FileInfoStdc info = {0};
	info.path = path;
	info.mode = "rb";

	enum CzResult ret = open_file_stdc(&info);
	if CZ_NOEXPECT (ret)
		return ret;

	if (offset == CZ_EOF)
		offset = (info.fileSize > size) ? info.fileSize - size : 0;

	ret = remove_section_stdc(&info, size, offset);
	if CZ_NOEXPECT (ret) {
		close_file_stdc(&info);
		return ret;
	}
	return close_file_stdc(&info);
}

enum CzResult czTrimFile(const char* restrict path, size_t size, size_t offset, struct CzFileFlags flags)
{
	enum CzResult ret;
	const char* resolvedPath = path;
	char* allocPath = NULL;

	if (flags.relativeToExe) {
		ret = alloc_resolve_path(&allocPath, path);
		if CZ_NOEXPECT (ret)
			return ret;
		if (allocPath)
			resolvedPath = allocPath;
	}

#if HAVE_czTrimFile_win32
	ret = czTrimFile_win32(resolvedPath, size, offset);
#elif HAVE_czTrimFile_posix
	ret = czTrimFile_posix(resolvedPath, size, offset);
#else
	ret = czTrimFile_stdc(resolvedPath, size, offset);
#endif
	czFree(allocPath);
	return ret;
}

#define HAVE_czStreamIsTerminal_win32 ( \
	CZ_WRAP_FILENO &&                   \
	CZ_WRAP_GET_OSFHANDLE )

#if HAVE_czStreamIsTerminal_win32
static enum CzResult czStreamIsTerminal_win32(FILE* restrict stream, bool* restrict istty)
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

#define HAVE_czStreamIsTerminal_posix ( \
	CZ_WRAP_FILENO &&                   \
	CZ_WRAP_ISATTY )

#if HAVE_czStreamIsTerminal_posix
static enum CzResult czStreamIsTerminal_posix(FILE* restrict stream, bool* restrict istty)
{
	int fd;
	enum CzResult ret = czWrap_fileno(&fd, stream);
	if CZ_NOEXPECT (ret)
		return ret;

	int tty;
	ret = czWrap_isatty(&tty, fd);
	if CZ_NOEXPECT (ret)
		return ret;

	*istty = (bool) tty;
	return CZ_RESULT_SUCCESS;
}
#endif

static enum CzResult czStreamIsTerminal_stdc(FILE* restrict stream, bool* restrict istty)
{
	(void) stream;
	(void) istty;
	return CZ_RESULT_NO_SUPPORT;
}

enum CzResult czStreamIsTerminal(FILE* restrict stream, bool* restrict istty)
{
#if HAVE_czStreamIsTerminal_win32
	return czStreamIsTerminal_win32(stream, istty);
#elif HAVE_czStreamIsTerminal_posix
	return czStreamIsTerminal_posix(stream, istty);
#else
	return czStreamIsTerminal_stdc(stream, istty);
#endif
}
