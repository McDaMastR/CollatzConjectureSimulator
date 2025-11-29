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

/**
 * @file
 * 
 * @brief Thin wrappers for common Windows API functions.
 * 
 * A non-comprehensive set of thin wrapper functions over the Windows API (WinAPI) to provide consistent error
 * management. These wrappers are intended for use within cz* API implementations rather than for general use. Each
 * wrapper function wraps exactly one WinAPI function, though may also call other WinAPI functions to aid in error
 * reporting.
 * 
 * Each wrapper function is prefixed with 'czWrap_' and suffixed with the name of the respective wrapped function. For
 * example, the function @c czWrap_ReadFile wraps the @c ReadFile WinAPI function. The only exception is if the wrapped
 * function begins with one or more underscores. In this case, the additional leading underscores are ignored when
 * naming the wrapper function. For example, the function @c _recalloc is wrapped with @c czWrap_recalloc.
 * 
 * Wrapper functions are accompanied by a preprocessor macro of the same name, but in screaming snake case. For example,
 * the wrapper @c czWrap_ReadFile is accompanied by the macro @c CZ_WRAP_READ_FILE. For any wrapper function, the
 * corresponding macro is defined as a nonzero value if and only if the wrapped function is available on the target
 * platform. And the declaration and definition of the wrapper function are visible if and only if the macro is nonzero.
 * 
 * Each wrapper guarantees that on return, the calling thread's last-error code (as returned by @c GetLastError) is the
 * same value as the last-error code immediately following the wrapped function's return. So the wrapper's observable
 * effect on the last-error code is functionally identical to the wrapped function.
 * 
 * Due to differences in error categorisation between WinAPI and this API (@c CzResult enumeration values), as well as
 * the abysmal documentation of WinAPI error codes and error reporting, the documented return values for wrappers are
 * @b not guarantees. If failure occurs in a wrapped function, the wrapper will attempt to return the corresponding
 * error value, but cannot always be certain of the exact reason for failure.
 */

#pragma once

#include "def.h"

/**
 * @def CZ_WRAP_RECALLOC
 * 
 * @brief Specifies whether @c _recalloc is defined.
 */
#if !defined(CZ_WRAP_RECALLOC)
#if CZ_WIN32
#define CZ_WRAP_RECALLOC 1
#else
#define CZ_WRAP_RECALLOC 0
#endif
#endif

#if CZ_WRAP_RECALLOC
/**
 * @brief Wraps @c _recalloc.
 * 
 * Calls @c _recalloc with @p memblock, @p num, and @p size. On success, the returned @c void* is synchronously written
 * to @p res. On failure, the contents of @p res are unchanged and the call is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] memblock The first argument to pass to @c _recalloc.
 * @param[in] num The second argument to pass to @c _recalloc.
 * @param[in] size The third argument to pass to @c _recalloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_RECALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_recalloc(void* restrict* res, void* memblock, size_t num, size_t size);
#endif

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_MALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_malloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_MALLOC)
#if CZ_WIN32
#define CZ_WRAP_ALIGNED_OFFSET_MALLOC 1
#else
#define CZ_WRAP_ALIGNED_OFFSET_MALLOC 0
#endif
#endif

#if CZ_WRAP_ALIGNED_OFFSET_MALLOC
/**
 * @brief Wraps @c _aligned_offset_malloc.
 * 
 * Calls @c _aligned_offset_malloc with @p size, @p alignment, and @p offset. On success, the returned @c void* is
 * synchronously written to @p res. On failure, the contents of @p res are unchanged and the call is logged to
 * @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] size The first argument to pass to @c _aligned_offset_malloc.
 * @param[in] alignment The second argument to pass to @c _aligned_offset_malloc.
 * @param[in] offset The third argument to pass to @c _aligned_offset_malloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to @p size.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_MALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_malloc(void* restrict* res, size_t size, size_t alignment, size_t offset);
#endif

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_REALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_realloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_REALLOC)
#if CZ_WIN32
#define CZ_WRAP_ALIGNED_OFFSET_REALLOC 1
#else
#define CZ_WRAP_ALIGNED_OFFSET_REALLOC 0
#endif
#endif

#if CZ_WRAP_ALIGNED_OFFSET_REALLOC
/**
 * @brief Wraps @c _aligned_offset_realloc.
 * 
 * Calls @c _aligned_offset_realloc with @p memblock, @p size, @p alignment, and @p offset. On success, the returned
 * @c void* is synchronously written to @p res. On failure, the contents of @p res are unchanged and the call is logged
 * to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] memblock The first argument to pass to @c _aligned_offset_realloc.
 * @param[in] size The second argument to pass to @c _aligned_offset_realloc.
 * @param[in] alignment The third argument to pass to @c _aligned_offset_realloc.
 * @param[in] offset The fourth argument to pass to @c _aligned_offset_realloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to @p size.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_REALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_realloc(
	void* restrict* res, void* memblock, size_t size, size_t alignment, size_t offset);
#endif

/**
 * @def CZ_WRAP_ALIGNED_OFFSET_RECALLOC
 * 
 * @brief Specifies whether @c _aligned_offset_recalloc is defined.
 */
#if !defined(CZ_WRAP_ALIGNED_OFFSET_RECALLOC)
#if CZ_WIN32
#define CZ_WRAP_ALIGNED_OFFSET_RECALLOC 1
#else
#define CZ_WRAP_ALIGNED_OFFSET_RECALLOC 0
#endif
#endif

#if CZ_WRAP_ALIGNED_OFFSET_RECALLOC
/**
 * @brief Wraps @c _aligned_offset_recalloc.
 * 
 * Calls @c _aligned_offset_recalloc with @p memblock, @p num, @p size, @p alignment, and @p offset. On success, the
 * returned @c void* is synchronously written to @p res. On failure, the contents of @p res are unchanged and the call
 * is logged to @c stderr.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] memblock The first argument to pass to @c _aligned_offset_recalloc.
 * @param[in] num The second argument to pass to @c _aligned_offset_recalloc.
 * @param[in] size The third argument to pass to @c _aligned_offset_recalloc.
 * @param[in] alignment The fourth argument to pass to @c _aligned_offset_recalloc.
 * @param[in] offset The fifth argument to pass to @c _aligned_offset_recalloc.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ALIGNMENT @p alignment was not a power of two.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero and greater than or equal to (@p num * @p size).
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_ALIGNED_OFFSET_RECALLOC is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1)
enum CzResult czWrap_aligned_offset_recalloc(
	void* restrict* res, void* memblock, size_t num, size_t size, size_t alignment, size_t offset);
#endif

/**
 * @def CZ_WRAP_GET_OSFHANDLE
 * 
 * @brief Specifies whether @c _get_osfhandle is defined.
 */
#if !defined(CZ_WRAP_GET_OSFHANDLE)
#if CZ_WIN32
#define CZ_WRAP_GET_OSFHANDLE 1
#else
#define CZ_WRAP_GET_OSFHANDLE 0
#endif
#endif

#if CZ_WRAP_GET_OSFHANDLE
/**
 * @brief Wraps @c _get_osfhandle.
 * 
 * Calls @c _get_osfhandle with @p fd. On success, the returned @c intptr_t is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] fd The argument to pass to @c _get_osfhandle.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p fd was an invalid open file descriptor.
 * 
 * @pre @p res is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_OSFHANDLE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(1) CZ_FILDES(2)
enum CzResult czWrap_get_osfhandle(intptr_t* res, int fd);
#endif

/**
 * @def CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR
 * 
 * @brief Specifies whether @c MultiByteToWideChar is defined.
 */
#if !defined(CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR)
#if CZ_WIN32
#define CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR 1
#else
#define CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR 0
#endif
#endif

#if CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR
/**
 * @brief Wraps @c MultiByteToWideChar.
 * 
 * Calls @c MultiByteToWideChar with @p CodePage, @p dwFlags, @p lpMultiByteStr, @p cbMultiByte, @p lpWideCharStr, and
 * @p cchWideChar. If @p res is nonnull, the returned @c INT is synchronously written to @p res.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] CodePage The first argument to pass to @c MultiByteToWideChar.
 * @param[in] dwFlags The second argument to pass to @c MultiByteToWideChar.
 * @param[in] lpMultiByteStr The third argument to pass to @c MultiByteToWideChar.
 * @param[in] cbMultiByte The fourth argument to pass to @c MultiByteToWideChar.
 * @param[out] lpWideCharStr The fifth argument to pass to @c MultiByteToWideChar.
 * @param[in] cchWideChar The sixth argument to pass to @c MultiByteToWideChar.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS @p CodePage or @p dwFlags was invalid.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpMultiByteStr or @p lpWideCharStr was an invalid pointer.
 * @retval CZ_RESULT_BAD_PATH @p lpMultiByteStr was an invalid multibyte string.
 * @retval CZ_RESULT_BAD_SIZE @p cbMultiByte or @p cchWideChar was invalid.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MULTI_BYTE_TO_WIDE_CHAR is defined as a nonzero value.
 */
CZ_WR_ACCESS(1) CZ_RD_ACCESS(4, 5) CZ_WR_ACCESS(6, 7)
enum CzResult czWrap_MultiByteToWideChar(
	LPINT res,
	UINT CodePage,
	DWORD dwFlags,
	LPCCH lpMultiByteStr,
	INT cbMultiByte,
	LPWSTR lpWideCharStr,
	INT cchWideChar);
#endif

/**
 * @def CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W
 * 
 * @brief Specifies whether @c GetFileAttributesExW is defined.
 */
#if !defined(CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W)
#if CZ_WIN32
#define CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W 1
#else
#define CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W 0
#endif
#endif

#if CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W
/**
 * @brief Wraps @c GetFileAttributesExW.
 * 
 * Calls @c GetFileAttributesExW with @p lpFileName, @p fInfoLevelId, and @p lpFileInformation.
 * 
 * @param[in] lpFileName The first argument to pass to @c GetFileAttributesExW.
 * @param[in] fInfoLevelId The second argument to pass to @c GetFileAttributesExW.
 * @param[out] lpFileInformation The third argument to pass to @c GetFileAttributesExW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileName or @p lpFileInformation was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_PATH @p lpFileName was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p lpFileName is nonnull and NUL-terminated.
 * @pre @p lpFileInformation is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_FILE_ATTRIBUTES_EX_W is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 3) CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_WR_ACCESS(3)
enum CzResult czWrap_GetFileAttributesExW(
	LPCWSTR lpFileName, GET_FILEEX_INFO_LEVELS fInfoLevelId, LPVOID lpFileInformation);
#endif

/**
 * @def CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX
 * 
 * @brief Specifies whether @c GetFileInformationByHandleEx is defined.
 */
#if !defined(CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX)
#if CZ_WIN32
#define CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX 1
#else
#define CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX 0
#endif
#endif

#if CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX
/**
 * @brief Wraps @c GetFileInformationByHandleEx.
 * 
 * Calls @c GetFileInformationByHandleEx with @p hFile, @p FileInformationClass, @p lpFileInformation, and
 * @p dwBufferSize.
 * 
 * @param[in] hFile The first argument to pass to @c GetFileInformationByHandleEx.
 * @param[in] FileInformationClass The second argument to pass to @c GetFileInformationByHandleEx.
 * @param[out] lpFileInformation The third argument to pass to @c GetFileInformationByHandleEx.
 * @param[in] dwBufferSize The fourth argument to pass to @c GetFileInformationByHandleEx.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileInformation was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * @pre @p lpFileInformation is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_FILE_INFORMATION_BY_HANDLE_EX is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 3) CZ_WR_ACCESS(3)
enum CzResult czWrap_GetFileInformationByHandleEx(
	HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
#endif

/**
 * @def CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE
 * 
 * @brief Specifies whether @c SetFileInformationByHandle is defined.
 */
#if !defined(CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE)
#if CZ_WIN32
#define CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE 1
#else
#define CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE 0
#endif
#endif

#if CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE
/**
 * @brief Wraps @c SetFileInformationByHandle.
 * 
 * Calls @c SetFileInformationByHandle with @p hFile, @p FileInformationClass, @p lpFileInformation, and
 * @p dwBufferSize.
 * 
 * @param[in] hFile The first argument to pass to @c SetFileInformationByHandle.
 * @param[in] FileInformationClass The second argument to pass to @c SetFileInformationByHandle.
 * @param[in] lpFileInformation The third argument to pass to @c SetFileInformationByHandle.
 * @param[in] dwBufferSize The fourth argument to pass to @c SetFileInformationByHandle.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileInformation was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * @pre @p lpFileInformation is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_SET_FILE_INFORMATION_BY_HANDLE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 3) CZ_RD_ACCESS(3)
enum CzResult czWrap_SetFileInformationByHandle(
	HANDLE hFile, FILE_INFO_BY_HANDLE_CLASS FileInformationClass, LPVOID lpFileInformation, DWORD dwBufferSize);
#endif

/**
 * @def CZ_WRAP_GET_FILE_SIZE_EX
 * 
 * @brief Specifies whether @c GetFileSizeEx is defined.
 */
#if !defined(CZ_WRAP_GET_FILE_SIZE_EX)
#if CZ_WIN32
#define CZ_WRAP_GET_FILE_SIZE_EX 1
#else
#define CZ_WRAP_GET_FILE_SIZE_EX 0
#endif
#endif

#if CZ_WRAP_GET_FILE_SIZE_EX
/**
 * @brief Wraps @c GetFileSizeEx.
 * 
 * Calls @c GetFileSizeEx with @p hFile and @p lpFileSize.
 * 
 * @param[in] hFile The first argument to pass to @c GetFileSizeEx.
 * @param[out] lpFileSize The second argument to pass to @c GetFileSizeEx.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileSize was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * @pre @p lpFileSize is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_FILE_SIZE_EX is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(2)
enum CzResult czWrap_GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize);
#endif

/**
 * @def CZ_WRAP_GET_FILE_TYPE
 * 
 * @brief Specifies whether @c GetFileType is defined.
 */
#if !defined(CZ_WRAP_GET_FILE_TYPE)
#if CZ_WIN32
#define CZ_WRAP_GET_FILE_TYPE 1
#else
#define CZ_WRAP_GET_FILE_TYPE 0
#endif
#endif

#if CZ_WRAP_GET_FILE_TYPE
/**
 * @brief Wraps @c GetFileType.
 * 
 * Calls @c GetFileType with @p hFile. On success, the returned @c DWORD is synchronously written to @p res. On failure,
 * the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] hFile The argument to pass to @c GetFileType.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p hFile is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_GET_FILE_TYPE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(1)
enum CzResult czWrap_GetFileType(PDWORD res, HANDLE hFile);
#endif

/**
 * @def CZ_WRAP_CREATE_FILE_W
 * 
 * @brief Specifies whether @c CreateFileW is defined.
 */
#if !defined(CZ_WRAP_CREATE_FILE_W)
#if CZ_WIN32
#define CZ_WRAP_CREATE_FILE_W 1
#else
#define CZ_WRAP_CREATE_FILE_W 0
#endif
#endif

#if CZ_WRAP_CREATE_FILE_W
/**
 * @brief Wraps @c CreateFileW.
 * 
 * Calls @c CreateFileW with @p lpFileName, @p dwDesiredAccess, @p dwShareMode, @p lpSecurityAttributes,
 * @p dwCreationDisposition, @p dwFlagsAndAttributes, and @p hTemplateFile. On success, the returned @c HANDLE is
 * synchronously written to @p res. On failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] lpFileName The first argument to pass to @c CreateFileW.
 * @param[in] dwDesiredAccess The second argument to pass to @c CreateFileW.
 * @param[in] dwShareMode The third argument to pass to @c CreateFileW.
 * @param[in] lpSecurityAttributes The fourth argument to pass to @c CreateFileW.
 * @param[in] dwCreationDisposition The fifth argument to pass to @c CreateFileW.
 * @param[in] dwFlagsAndAttributes The sixth argument to pass to @c CreateFileW.
 * @param[in] hTemplateFile The seventh argument to pass to @c CreateFileW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to open the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileName or @p lpSecurityAttributes was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when creating, reading from, or writing to the file.
 * @retval CZ_RESULT_BAD_PATH @p lpFileName was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p lpFileName is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CREATE_FILE_W is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_NULTERM_ARG(2) CZ_WR_ACCESS(1) CZ_RD_ACCESS(2) CZ_RD_ACCESS(5)
enum CzResult czWrap_CreateFileW(
	LPHANDLE res,
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile);
#endif

/**
 * @def CZ_WRAP_CLOSE_HANDLE
 * 
 * @brief Specifies whether @c CloseHandle is defined.
 */
#if !defined(CZ_WRAP_CLOSE_HANDLE)
#if CZ_WIN32
#define CZ_WRAP_CLOSE_HANDLE 1
#else
#define CZ_WRAP_CLOSE_HANDLE 0
#endif
#endif

#if CZ_WRAP_CLOSE_HANDLE
/**
 * @brief Wraps @c CloseHandle.
 * 
 * Calls @c CloseHandle with @p hObject.
 * 
 * @param[in] hObject The argument to pass to @c CloseHandle.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hObject is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_CLOSE_HANDLE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1)
enum CzResult czWrap_CloseHandle(HANDLE hObject);
#endif

/**
 * @def CZ_WRAP_SET_END_OF_FILE
 * 
 * @brief Specifies whether @c SetEndOfFile is defined.
 */
#if !defined(CZ_WRAP_SET_END_OF_FILE)
#if CZ_WIN32
#define CZ_WRAP_SET_END_OF_FILE 1
#else
#define CZ_WRAP_SET_END_OF_FILE 0
#endif
#endif

#if CZ_WRAP_SET_END_OF_FILE
/**
 * @brief Wraps @c SetEndOfFile.
 * 
 * Calls @c SetEndOfFile with @p hFile.
 * 
 * @param[in] hFile The argument to pass to @c SetEndOfFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_SET_END_OF_FILE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1)
enum CzResult czWrap_SetEndOfFile(HANDLE hFile);
#endif

/**
 * @def CZ_WRAP_SET_FILE_POINTER_EX
 * 
 * @brief Specifies whether @c SetFilePointerEx is defined.
 */
#if !defined(CZ_WRAP_SET_FILE_POINTER_EX)
#if CZ_WIN32
#define CZ_WRAP_SET_FILE_POINTER_EX 1
#else
#define CZ_WRAP_SET_FILE_POINTER_EX 0
#endif
#endif

#if CZ_WRAP_SET_FILE_POINTER_EX
/**
 * @brief Wraps @c SetFilePointerEx.
 * 
 * Calls @c SetFilePointerEx with @p hFile, @p liDistanceToMove, @p lpNewFilePointer, and @p dwMoveMethod.
 * 
 * @param[in] hFile The first argument to pass to @c SetFilePointerEx.
 * @param[in] liDistanceToMove The second argument to pass to @c SetFilePointerEx.
 * @param[out] lpNewFilePointer The third argument to pass to @c SetFilePointerEx.
 * @param[in] dwMoveMethod The fourth argument to pass to @c SetFilePointerEx.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpNewFilePointer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_OFFSET The resultant file offset was invalid.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_SET_FILE_POINTER_EX is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_WR_ACCESS(3)
enum CzResult czWrap_SetFilePointerEx(
	HANDLE hFile, LARGE_INTEGER liDistanceToMove, PLARGE_INTEGER lpNewFilePointer, DWORD dwMoveMethod);
#endif

/**
 * @def CZ_WRAP_READ_FILE
 * 
 * @brief Specifies whether @c ReadFile is defined.
 */
#if !defined(CZ_WRAP_READ_FILE)
#if CZ_WIN32
#define CZ_WRAP_READ_FILE 1
#else
#define CZ_WRAP_READ_FILE 0
#endif
#endif

#if CZ_WRAP_READ_FILE
/**
 * @brief Wraps @c ReadFile.
 * 
 * Calls @c ReadFile with @p hFile, @p lpBuffer, @p nNumberOfBytesToRead, @p lpNumberOfBytesRead, and @p lpOverlapped.
 * 
 * @param[in] hFile The first argument to pass to @c ReadFile.
 * @param[out] lpBuffer The second argument to pass to @c ReadFile.
 * @param[in] nNumberOfBytesToRead The third argument to pass to @c ReadFile.
 * @param[out] lpNumberOfBytesRead The fourth argument to pass to @c ReadFile.
 * @param[in,out] lpOverlapped The fifth argument to pass to @c ReadFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpBuffer or @p lpNumberOfBytesRead was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from the file.
 * @retval CZ_RESULT_BAD_OFFSET The file was already at @c EOF or @p lpOverlapped had an invalid file offset.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * @pre @p lpBuffer is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_READ_FILE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(2, 3) CZ_WR_ACCESS(4) CZ_RW_ACCESS(5)
enum CzResult czWrap_ReadFile(
	HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped);
#endif

/**
 * @def CZ_WRAP_WRITE_FILE
 * 
 * @brief Specifies whether @c WriteFile is defined.
 */
#if !defined(CZ_WRAP_WRITE_FILE)
#if CZ_WIN32
#define CZ_WRAP_WRITE_FILE 1
#else
#define CZ_WRAP_WRITE_FILE 0
#endif
#endif

#if CZ_WRAP_WRITE_FILE
/**
 * @brief Wraps @c WriteFile.
 * 
 * Calls @c WriteFile with @p hFile, @p lpBuffer, @p nNumberOfBytesToWrite, @p lpNumberOfBytesWritten, and
 * @p lpOverlapped.
 * 
 * @param[in] hFile The first argument to pass to @c WriteFile.
 * @param[in] lpBuffer The second argument to pass to @c WriteFile.
 * @param[in] nNumberOfBytesToWrite The third argument to pass to @c WriteFile.
 * @param[out] lpNumberOfBytesWritten The fourth argument to pass to @c WriteFile.
 * @param[in,out] lpOverlapped The fifth argument to pass to @c WriteFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpBuffer or @p lpNumberOfBytesWritten was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when writing to the file.
 * @retval CZ_RESULT_BAD_OFFSET @p lpOverlapped had an invalid file offset.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * @pre @p lpBuffer is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_WRITE_FILE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_RD_ACCESS(2, 3) CZ_WR_ACCESS(4) CZ_RW_ACCESS(5)
enum CzResult czWrap_WriteFile(
	HANDLE hFile,
	LPCVOID lpBuffer,
	DWORD nNumberOfBytesToWrite,
	LPDWORD lpNumberOfBytesWritten,
	LPOVERLAPPED lpOverlapped);
#endif

/**
 * @def CZ_WRAP_DELETE_FILE_W
 * 
 * @brief Specifies whether @c DeleteFileW is defined.
 */
#if !defined(CZ_WRAP_DELETE_FILE_W)
#if CZ_WIN32
#define CZ_WRAP_DELETE_FILE_W 1
#else
#define CZ_WRAP_DELETE_FILE_W 0
#endif
#endif

#if CZ_WRAP_DELETE_FILE_W
/**
 * @brief Wraps @c DeleteFileW.
 * 
 * Calls @c DeleteFileW with @p lpFileName.
 * 
 * @param[in] lpFileName The argument to pass to @c DeleteFileW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to delete the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileName was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_BAD_PATH @p lpFileName was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p lpFileName is nonnull and NUL-terminated.
 * 
 * @note This function is only defined if @ref CZ_WRAP_DELETE_FILE_W is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_RD_ACCESS(1)
enum CzResult czWrap_DeleteFileW(LPCWSTR lpFileName);
#endif

/**
 * @def CZ_WRAP_CREATE_FILE_MAPPING_W
 * 
 * @brief Specifies whether @c CreateFileMappingW is defined.
 */
#if !defined(CZ_WRAP_CREATE_FILE_MAPPING_W)
#if CZ_WIN32
#define CZ_WRAP_CREATE_FILE_MAPPING_W 1
#else
#define CZ_WRAP_CREATE_FILE_MAPPING_W 0
#endif
#endif

#if CZ_WRAP_CREATE_FILE_MAPPING_W
/**
 * @brief Wraps @c CreateFileMappingW.
 * 
 * Calls @c CreateFileMappingW with @p hFile, @p lpFileMappingAttributes, @p flProtect, @p dwMaximumSizeHigh,
 * @p dwMaximumSizeLow, and @p lpName. On success, the returned @c HANDLE is synchronously written to @p res. On
 * failure, the contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] hFile The first argument to pass to @c CreateFileMappingW.
 * @param[in] lpFileMappingAttributes The second argument to pass to @c CreateFileMappingW.
 * @param[in] flProtect The third argument to pass to @c CreateFileMappingW.
 * @param[in] dwMaximumSizeHigh The fourth argument to pass to @c CreateFileMappingW.
 * @param[in] dwMaximumSizeLow The fifth argument to pass to @c CreateFileMappingW.
 * @param[in] lpName The sixth argument to pass to @c CreateFileMappingW.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to create a mapping object for the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpFileMappingAttributes or @p lpName was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p hFile is nonnull.
 * @pre @p lpName is NUL-terminated,
 * 
 * @note This function is only defined if @ref CZ_WRAP_CREATE_FILE_MAPPING_W is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_NULTERM_ARG(7) CZ_WR_ACCESS(1) CZ_RD_ACCESS(3) CZ_RD_ACCESS(7)
enum CzResult czWrap_CreateFileMappingW(
	LPHANDLE res,
	HANDLE hFile,
	LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
	DWORD flProtect,
	DWORD dwMaximumSizeHigh,
	DWORD dwMaximumSizeLow,
	LPCWSTR lpName);
#endif

/**
 * @def CZ_WRAP_MAP_VIEW_OF_FILE
 * 
 * @brief Specifies whether @c MapViewOfFile is defined.
 */
#if !defined(CZ_WRAP_MAP_VIEW_OF_FILE)
#if CZ_WIN32
#define CZ_WRAP_MAP_VIEW_OF_FILE 1
#else
#define CZ_WRAP_MAP_VIEW_OF_FILE 0
#endif
#endif

#if CZ_WRAP_MAP_VIEW_OF_FILE
/**
 * @brief Wraps @c MapViewOfFile.
 * 
 * Calls @c MapViewOfFile with @p hFileMappingObject, @p dwDesiredAccess, @p dwFileOffsetHigh, @p dwFileOffsetLow, and
 * @p dwNumberOfBytesToMap. On success, the returned @c LPVOID is synchronously written to @p res. On failure, the
 * contents of @p res are unchanged.
 * 
 * @param[out] res The memory to write the return value to.
 * @param[in] hFileMappingObject The first argument to pass to @c MapViewOfFile.
 * @param[in] dwDesiredAccess The second argument to pass to @c MapViewOfFile.
 * @param[in] dwFileOffsetHigh The third argument to pass to @c MapViewOfFile.
 * @param[in] dwFileOffsetLow The fourth argument to pass to @c MapViewOfFile.
 * @param[in] dwNumberOfBytesToMap The fifth argument to pass to @c MapViewOfFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to map a view of @p hFileMappingObject was denied.
 * @retval CZ_RESULT_BAD_FILE @p hFileMappingObject or the corresponding file was invalid.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_BAD_OFFSET The file offset was not a multiple of the allocation granularity.
 * @retval CZ_RESULT_IN_USE @p hFileMappingObject or the corresponding file was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p res is nonnull.
 * @pre @p hFileMappingObject is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_MAP_VIEW_OF_FILE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1, 2) CZ_WR_ACCESS(1)
enum CzResult czWrap_MapViewOfFile(
	LPVOID* res,
	HANDLE hFileMappingObject,
	DWORD dwDesiredAccess,
	DWORD dwFileOffsetHigh,
	DWORD dwFileOffsetLow,
	SIZE_T dwNumberOfBytesToMap);
#endif

/**
 * @def CZ_WRAP_UNMAP_VIEW_OF_FILE
 * 
 * @brief Specifies whether @c UnmapViewOfFile is defined.
 */
#if !defined(CZ_WRAP_UNMAP_VIEW_OF_FILE)
#if CZ_WIN32
#define CZ_WRAP_UNMAP_VIEW_OF_FILE 1
#else
#define CZ_WRAP_UNMAP_VIEW_OF_FILE 0
#endif
#endif

#if CZ_WRAP_UNMAP_VIEW_OF_FILE
/**
 * @brief Wraps @c UnmapViewOfFile.
 * 
 * Calls @c UnmapViewOfFile with @p lpBaseAddress.
 * 
 * @param[in] lpBaseAddress The argument to pass to @c UnmapViewOfFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpBaseAddress was an invalid pointer or mapping view.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_IN_USE The mapping view was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p lpBaseAddress is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_UNMAP_VIEW_OF_FILE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1)
enum CzResult czWrap_UnmapViewOfFile(LPCVOID lpBaseAddress);
#endif

/**
 * @def CZ_WRAP_FLUSH_VIEW_OF_FILE
 * 
 * @brief Specifies whether @c FlushViewOfFile is defined.
 */
#if !defined(CZ_WRAP_FLUSH_VIEW_OF_FILE)
#if CZ_WIN32
#define CZ_WRAP_FLUSH_VIEW_OF_FILE 1
#else
#define CZ_WRAP_FLUSH_VIEW_OF_FILE 0
#endif
#endif

#if CZ_WRAP_FLUSH_VIEW_OF_FILE
/**
 * @brief Wraps @c FlushViewOfFile.
 * 
 * Calls @c FlushViewOfFile with @p lpBaseAddress and @p dwNumberOfBytesToFlush.
 * 
 * @param[in] lpBaseAddress The first argument to pass to @c FlushViewOfFile.
 * @param[in] dwNumberOfBytesToFlush The second argument to pass to @c FlushViewOfFile.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ADDRESS The address range was invalid.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_IN_USE The mapping view was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p lpBaseAddress is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FLUSH_VIEW_OF_FILE is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_RD_ACCESS(1, 2)
enum CzResult czWrap_FlushViewOfFile(LPCVOID lpBaseAddress, SIZE_T dwNumberOfBytesToFlush);
#endif

/**
 * @def CZ_WRAP_FLUSH_FILE_BUFFERS
 * 
 * @brief Specifies whether @c FlushFileBuffers is defined.
 */
#if !defined(CZ_WRAP_FLUSH_FILE_BUFFERS)
#if CZ_WIN32
#define CZ_WRAP_FLUSH_FILE_BUFFERS 1
#else
#define CZ_WRAP_FLUSH_FILE_BUFFERS 0
#endif
#endif

#if CZ_WRAP_FLUSH_FILE_BUFFERS
/**
 * @brief Wraps @c FlushFileBuffers.
 * 
 * Calls @c FlushFileBuffers with @p hFile.
 * 
 * @param[in] hFile The argument to pass to @c FlushFileBuffers.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when flushing the file.
 * @retval CZ_RESULT_IN_USE The mapping view was already in use by the system.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hFile is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_FLUSH_FILE_BUFFERS is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1)
enum CzResult czWrap_FlushFileBuffers(HANDLE hFile);
#endif

/**
 * @def CZ_WRAP_DEVICE_IO_CONTROL
 * 
 * @brief Specifies whether @c DeviceIoControl is defined.
 */
#if !defined(CZ_WRAP_DEVICE_IO_CONTROL)
#if CZ_WIN32
#define CZ_WRAP_DEVICE_IO_CONTROL 1
#else
#define CZ_WRAP_DEVICE_IO_CONTROL 0
#endif
#endif

#if CZ_WRAP_DEVICE_IO_CONTROL
/**
 * @brief Wraps @c DeviceIoControl.
 * 
 * Calls @c DeviceIoControl with @p hDevice, @p dwIoControlCode, @p lpInBuffer, @p nInBufferSize, @p lpOutBuffer,
 * @p nOutBufferSize, @p lpBytesReturned, and @p lpOverlapped.
 * 
 * @param[in] hDevice The first argument to pass to @c DeviceIoControl.
 * @param[in] dwIoControlCode The second argument to pass to @c DeviceIoControl.
 * @param[in] lpInBuffer The third argument to pass to @c DeviceIoControl.
 * @param[in] nInBufferSize The fourth argument to pass to @c DeviceIoControl.
 * @param[out] lpOutBuffer The fifth argument to pass to @c DeviceIoControl.
 * @param[in] nOutBufferSize The sixth argument to pass to @c DeviceIoControl.
 * @param[out] lpBytesReturned The seventh argument to pass to @c DeviceIoControl.
 * @param[in,out] lpOverlapped The eighth argument to pass to @c DeviceIoControl.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p lpInBuffer, @p lpOutBuffer, @p lpBytesReturned, or @p lpOverlapped was an invalid
 *   pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was unsupported.
 * @retval CZ_RESULT_BAD_IO A low-level IO operation failed when reading from or writing to the file.
 * @retval CZ_RESULT_BAD_SIZE @p nOutBufferSize was too small.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p hDevice is nonnull.
 * 
 * @note This function is only defined if @ref CZ_WRAP_DEVICE_IO_CONTROL is defined as a nonzero value.
 */
CZ_NONNULL_ARGS(1) CZ_RD_ACCESS(3, 4) CZ_WR_ACCESS(5, 6) CZ_WR_ACCESS(7), CZ_RW_ACCESS(8)
enum CzResult czWrap_DeviceIoControl(
	HANDLE hDevice,
	DWORD dwIoControlCode,
	LPVOID lpInBuffer,
	DWORD nInBufferSize,
	LPVOID lpOutBuffer,
	DWORD nOutBufferSize,
	LPDWORD lpBytesReturned,
	LPOVERLAPPED lpOverlapped);
#endif
