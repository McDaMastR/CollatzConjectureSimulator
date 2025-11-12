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
 * @brief Buffer-based and (mostly) cross-platform file and IO management.
 * 
 * A file and IO API that allows access of system resources in a way that analogises dynamic memory buffers. In
 * particular, regular files are abstracted as blocks of memory that can be read from and written to.
 */

#pragma once

#include "def.h"

/**
 * @brief Denotes end-of-file.
 * 
 * A value representing end-of-file. Can be used as an offset where explicitly stated.
 */
#define CZ_EOF SIZE_MAX

/**
 * @brief Specifies the behaviour of file and IO functions.
 * 
 * A set of flags specifying the desired behaviour of @ref czFileSize, @ref czReadFile, @ref czWriteFile,
 * @ref czInsertFile, @ref czClearFile, or @ref czTrimFile.
 */
struct CzFileFlags
{
	/**
	 * @brief Whether to interpret relative filepaths as relative to the executable.
	 */
	bool relativeToExe : 1;

	/**
	 * @brief Whether to examine symbolic links rather than follow them.
	 */
	bool openSymLink : 1;
};

/**
 * @brief Determines if an IO stream is a terminal.
 * 
 * Determines whether @p stream refers to a terminal or terminal emulator (abbreviated TTY) and synchronously writes the
 * result to @p istty. If @p stream does refer to a TTY, @p istty is set to true. Otherwise, @p istty is set to false.
 * If the platform does not support this ability, failure occurs. On failure, the contents of @p istty are unchanged.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czStreamIsTerminal if the following conditions are
 * satisfied.
 * - For any concurrent invocation @b B to @ref czStreamIsTerminal, the @p istty arguments of @b A and @b B are
 *   nonoverlapping in memory. If overlap does occur, the contents of the overlapping memory are undefined.
 * 
 * @param[in] stream The open IO stream.
 * @param[out] istty The memory to write the result to.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_STREAM @p stream was an invalid IO stream.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p stream is nonnull.
 * @pre @p istty is nonnull.
 * @pre @p stream and @p istty do not overlap in memory.
 */
CZ_REPRODUCIBLE CZ_NONNULL_ARGS() CZ_WR_ACCESS(2)
enum CzResult czStreamIsTerminal(FILE* stream, bool* istty);

/**
 * @brief Obtains the size of a file.
 * 
 * Determines the size of the file located at the filepath @p path and synchronously writes the file size to @p size.
 * The file size is measured in bytes. If the filepath style of @p path is not POSIX or Windows style, or is unsupported
 * by the platform, failure occurs. If @p path is an invalid filepath or locates a nonexistent or invalid resource,
 * failure occurs. If @p path locates a symbolic link, the behaviour is dependent on @p flags.openSymLink. If @p path
 * locates any other non-regular file resource, such as a directory, pipe, or socket, the behaviour is platform
 * dependent. On failure, the contents of @p size are unchanged.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - If @p flags.openSymLink is set and @p path locates a symbolic link, the file examined is the symbolic link itself.
 *   Otherwise if @p path locates a symbolic link, it is followed recursively to the linked file.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czFileSize if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czFileSize, the @p size arguments of @b A and @b B are nonoverlapping in
 *   memory. If overlap does occur, the contents of the overlapping memory are undefined.
 * - For any concurrent invocation @b C to @ref czWriteFile, @ref czInsertFile, or @ref czTrimFile, the @p path
 *   arguments of @b A and @b C locate distinct system resources. If they locate the same resource, the behaviour is
 *   undefined.
 * 
 * @param[in] path The path to the file.
 * @param[out] size The memory to write the size to.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to access the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file did not exist.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p size is nonnull.
 * @pre @p path and @p size do not overlap in memory.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_WR_ACCESS(2)
enum CzResult czFileSize(const char* path, size_t* size, struct CzFileFlags flags);

/**
 * @brief Reads from a file.
 * 
 * Reads the contents of the file located at the filepath @p path and synchronously writes the contents to @p buffer. If
 * the filepath style of @p path is not POSIX or Windows style, or is unsupported by the platform, failure occurs. If
 * @p path is an invalid filepath or locates a nonexistent or invalid resource, failure occurs. If @p path locates a
 * symbolic link, the link is followed recursively. If @p path locates any other non-regular file resource, such as a
 * directory, pipe, or socket, the behaviour is platform dependent.
 * 
 * Let @e fileSize denote the size of the file located at @p path as measured in bytes, let @e minSize denote the
 * minimum of @p size and (@e fileSize - @p offset), and let @e maxSize denote the maximum of (@e fileSize - @p size)
 * and zero. The file contents read are a contiguous block of memory whose size and offset within the file are
 * dependent on @p size and @p offset.
 * - If @p offset is @c CZ_EOF, the read block includes exactly (@e fileSize - @e maxSize) bytes starting from the byte
 *   at the zero-based index @e maxSize. That is, all bytes whose indices lie within the interval [@e maxSize,
 *   @e fileSize).
 * - If @p offset is not @c CZ_EOF, the read block includes exactly @e minSize bytes starting from the byte at the
 *   zero-based index @p offset. That is, all bytes whose indices lie within the interval [@p offset,
 *   @e minSize + @p offset).
 * 
 * If @p size is zero or @p offset is not @c CZ_EOF and not less than @e fileSize, failure occurs. On failure, the
 * contents of @p buffer are undefined.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - @p flags.openSymLink is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czReadFile if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czReadFile, the @p buffer arguments of @b A and @b B are nonoverlapping
 *   in memory. If overlap does occur, the contents of the overlapping memory are undefined.
 * - For any concurrent invocation @b C to @ref czWriteFile, @ref czInsertFile, @ref czClearFile, or @ref czTrimFile,
 *   the @p path arguments of @b A and @b C locate distinct system resources. If they locate the same resource, the
 *   behaviour is undefined.
 * 
 * @param[in] path The path to the file.
 * @param[out] buffer The memory to write the file contents to.
 * @param[in] size The maximum number of bytes to read.
 * @param[in] offset The first byte to read from.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero, not @c CZ_EOF, and greater than or equal to @e fileSize.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_FILE The file did not exist or was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * @retval CZ_RESULT_TIMEOUT A system operation timed out.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p buffer is nonnull.
 * @pre @p path and @p buffer do not overlap in memory.
 * @pre @p size is less than or equal to the size of @p buffer.
 * 
 * Example 1: @e fileSize = 80, @p size = 16, @p offset = 48.
@verbatim
                                            offset      size+offset         fileSize
                                                 │                │                │
┌────────────────────────────────────────────────┬────────────────┬────────────────┐
│                    48 bytes                    │    16 bytes    │    16 bytes    │
└────────────────────────────────────────────────┴────────────────┴────────────────┘
                                                 └────────────────┘
                                                        Read
@endverbatim
 * Example 2: @e fileSize = 80, @p size = 48, @p offset = 48.
@verbatim
                                            offset                         fileSize      size+offset
                                                 │                                │                │
┌────────────────────────────────────────────────┬────────────────────────────────┬────────────────┐
│                    48 bytes                    │            32 bytes            │    16 bytes    │
└────────────────────────────────────────────────┴────────────────────────────────┴────────────────┘
                                                 └────────────────────────────────┘
                                                                Read
@endverbatim
 * Example 3: @e fileSize = 80, @p size = 1024, @p offset = 0.
@verbatim
                                                                          fileSize             size
                                                                                 │                │
┌────────────────────────────────────────────────────────────────────────────────┬──────....──────┐
│                                    80 bytes                                    │                │
└────────────────────────────────────────────────────────────────────────────────┴──────....──────┘
└────────────────────────────────────────────────────────────────────────────────┘
                                       Read
@endverbatim
 * Example 4: @e fileSize = 80, @p size = 64, @p offset = CZ_EOF.
@verbatim
                 fileSize-size                                             fileSize
                 │                                                                │
┌────────────────┬────────────────────────────────────────────────────────────────┐
│    16 bytes    │                            64 bytes                            │
└────────────────┴────────────────────────────────────────────────────────────────┘
                 └────────────────────────────────────────────────────────────────┘
                                                Read
@endverbatim
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_WR_ACCESS(2, 3)
enum CzResult czReadFile(const char* path, void* buffer, size_t size, size_t offset, struct CzFileFlags flags);

/**
 * @brief Writes to a file.
 * 
 * Reads the contents of @p buffer and synchronously writes the contents to the file located at the filepath @p path. If
 * the filepath style of @p path is not POSIX or Windows style, or is unsupported by the platform, failure occurs. If
 * @p path is an invalid filepath or locates an invalid resource, failure occurs. If @p path locates a nonexistent
 * resource, a regular file is created at the location. If @p path locates a symbolic link, the link is followed
 * recursively. If @p path locates any other non-regular file resource, such as a directory, pipe, or socket, the
 * behaviour is platform dependent.
 * 
 * Let @e fileSize denote the size of the file located at @p path as measured in bytes if it exists, and zero otherwise.
 * Let @e maxSize denote the maximum of (@p size + @p offset) and @e fileSize. The file contents written include exactly
 * @p size contiguous bytes starting from the byte at the zero-based index @p offset. That is, all bytes whose indices
 * lie within the interval [@p offset, @p size + @p offset). Any overwritten file contents are destroyed.
 * 
 * If @p offset is @e fileSize or @c CZ_EOF, the contents of @p buffer are appended to the file. If @p offset is less
 * than @e fileSize and @e maxSize is greater than @e fileSize, the file size is increased to @e maxSize to accommodate
 * the contents of @p buffer. If @p size is zero or @p offset is not @c CZ_EOF and greater than @e fileSize, failure
 * occurs. On failure, the contents of the file are undefined.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - @p flags.openSymLink is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czWriteFile if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czWriteFile, @ref czInsertFile, @ref czClearFile, or @ref czTrimFile,
 *   the @p path arguments of @b A and @b B locate distinct system resources. If they locate the same resource, the
 *   behaviour is undefined.
 * 
 * @param[in] path The path to the file.
 * @param[in] buffer The memory to read the new file contents from.
 * @param[in] size The number of bytes to write.
 * @param[in] offset The first byte to write to.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET The file did exist and @p offset was not @c CZ_EOF and greater than @e fileSize.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist and @p offset was nonzero and not @c CZ_EOF.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p buffer is nonnull.
 * @pre @p path and @p buffer do not overlap in memory.
 * @pre @p size is less than or equal to the size of @p buffer.
 * 
 * @warning Invalid usage can result in permanent loss of file data.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2, 3)
enum CzResult czWriteFile(const char* path, const void* buffer, size_t size, size_t offset, struct CzFileFlags flags);

/**
 * @brief Inserts into a file.
 * 
 * Reads the contents of @p buffer and synchronously writes the contents to the file located at the filepath @p path. If
 * the filepath style of @p path is not POSIX or Windows style, or is unsupported by the platform, failure occurs. If
 * @p path is an invalid filepath or locates an invalid resource, failure occurs. If @p path locates a nonexistent
 * resource, a regular file is created at the location. If @p path locates a symbolic link, the link is followed
 * recursively. If @p path locates any other non-regular file resource, such as a directory, pipe, or socket, the
 * behaviour is platform dependent.
 * 
 * Let @e fileSize denote the size of the file located at @p path as measured in bytes if it exists, and zero otherwise.
 * Let @e maxSize denote the maximum of (@p size + @p offset) and @e fileSize. The file contents written include exactly
 * @p size contiguous bytes starting from the byte at the zero-based index @p offset. That is, all bytes whose indices
 * lie within the interval [@p offset, @p size + @p offset).
 * 
 * If @p offset is @e fileSize or @c CZ_EOF, the contents of @p buffer are appended to the file. If @p offset is less
 * than @e fileSize, any previous file contents in the interval [@p offset, @e fileSize) are moved to the memory in the
 * interval [@p size + @p offset, @e fileSize + @p size). If @p size is zero or @p offset is not @c CZ_EOF and greater
 * than @e fileSize, failure occurs. On failure, the contents of the file are undefined.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - @p flags.openSymLink is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czInsertFile if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czWriteFile, @ref czInsertFile, @ref czClearFile, or @ref czTrimFile,
 *   the @p path arguments of @b A and @b B locate distinct system resources. If they locate the same resource, the
 *   behaviour is undefined.
 * 
 * @param[in] path The path to the file.
 * @param[in] buffer The memory to read the new file contents from.
 * @param[in] size The number of bytes to write.
 * @param[in] offset The first byte to write to.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from or write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET The file did exist and @p offset was not @c CZ_EOF and greater than @e fileSize.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist and @p offset was nonzero and not @c CZ_EOF.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p buffer is nonnull.
 * @pre @p path and @p buffer do not overlap in memory.
 * @pre @p size is less than or equal to the size of @p buffer.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2, 3)
enum CzResult czInsertFile(const char* path, const void* buffer, size_t size, size_t offset, struct CzFileFlags flags);

/**
 * @brief Overwrites an entire file.
 * 
 * Reads the contents of @p buffer and synchronously writes the contents to the file located at the filepath @p path. If
 * the filepath style of @p path is not POSIX or Windows style, or is unsupported by the platform, failure occurs. If
 * @p path is an invalid filepath or locates an invalid resource, failure occurs. If @p path locates a nonexistent
 * resource, a regular file is created at the location. If @p path locates a symbolic link, the link is followed
 * recursively. If @p path locates any other non-regular file resource, such as a directory, pipe, or socket, the
 * behaviour is platform dependent.
 * 
 * The file contents written include exactly the first @p size contiguous bytes. That is, all bytes whose indices lie
 * within the interval [0, @p size). The file size is changed to @p size and any previous file contents are destroyed.
 * If @p size is zero, failure occurs. On failure, the contents of the file are undefined.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - @p flags.openSymLink is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czRewriteFile if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czWriteFile, @ref czInsertFile, @ref czRewriteFile, @ref czClearFile, or
 *   @ref czTrimFile, the @p path arguments of @b A and @b B locate distinct system resources. If they locate the same
 *   resource, the behaviour is undefined.
 * 
 * @param[in] path The path to the file.
 * @param[in] buffer The memory to read the new file contents from.
 * @param[in] size The number of bytes to write.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path or @p buffer was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * @pre @p buffer is nonnull.
 * @pre @p path and @p buffer do not overlap in memory.
 * @pre @p size is less than or equal to the size of @p buffer.
 * 
 * @warning Invalid usage can result in permanent loss of file data.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2, 3)
enum CzResult czRewriteFile(const char* path, const void* buffer, size_t size, struct CzFileFlags flags);

/**
 * @brief Zeros out a file.
 * 
 * Synchronously zeros the contents of the file located at the filepath @p path. If the filepath style of @p path is not
 * POSIX or Windows style, or is unsupported by the platform, failure occurs. If @p path is an invalid filepath or
 * locates a nonexistent or invalid resource, failure occurs. If @p path locates a symbolic link, the link is followed
 * recursively. If @p path locates any other non-regular file resource, such as a directory, pipe, or socket, the
 * behaviour is platform dependent.
 * 
 * Let @e fileSize denote the size of the file located at @p path as measured in bytes, let @e minSize denote the
 * minimum of @p size and (@e fileSize - @p offset), and let @e maxSize denote the maximum of (@e fileSize - @p size)
 * and zero. The file contents cleared are a contiguous block of memory whose size and offset within the file are
 * dependent on @p size and @p offset.
 * - If @p offset is @c CZ_EOF, the cleared block includes exactly (@e fileSize - @e maxSize) bytes starting from the
 *   byte at the zero-based index @e maxSize. That is, all bytes whose indices lie within the interval [@e maxSize,
 *   @e fileSize).
 * - If @p offset is not @c CZ_EOF, the cleared block includes exactly @e minSize bytes starting from the byte at the
 *   zero-based index @p offset. That is, all bytes whose indices lie within the interval [@p offset,
 *   @e minSize + @p offset).
 * 
 * If @p size is zero or @p offset is not @c CZ_EOF and not less than @e fileSize, failure occurs. On failure, the
 * contents of the file are undefined.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - @p flags.openSymLink is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czClearFile if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czWriteFile, @ref czInsertFile, or @ref czTrimFile, the @p path
 *   arguments of @b A and @b B locate distinct system resources. If they locate the same resource, the behaviour is
 *   undefined.
 * 
 * @param[in] path The path to the file.
 * @param[in] size The number of bytes to clear.
 * @param[in] offset The first byte to clear.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from or write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero, not @c CZ_EOF, and greater than or equal to @e fileSize.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist or was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @warning Invalid usage can result in permanent loss of file data.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czClearFile(const char* path, size_t size, size_t offset, struct CzFileFlags flags);

/**
 * @brief Trims a file.
 * 
 * Synchronously trims the contents of the file located at the filepath @p path. If the filepath style of @p path is not
 * POSIX or Windows style, or is unsupported by the platform, failure occurs. If @p path is an invalid filepath or
 * locates a nonexistent or invalid resource, failure occurs. If @p path locates a symbolic link, the link is followed
 * recursively. If @p path locates any other non-regular file resource, such as a directory, pipe, or socket, the
 * behaviour is platform dependent.
 * 
 * Let @e fileSize denote the size of the file located at @p path as measured in bytes, let @e minSize denote the
 * minimum of @p size and (@e fileSize - @p offset), and let @e maxSize denote the maximum of (@e fileSize - @p size)
 * and zero. The file contents trimmed are a contiguous block of memory whose size and offset within the file are
 * dependent on @p size and @p offset.
 * - If @p offset is @c CZ_EOF, the trimmed block includes exactly (@e fileSize - @e maxSize) bytes starting from the
 *   byte at the zero-based index @e maxSize. That is, all bytes whose indices lie within the interval [@e maxSize,
 *   @e fileSize). The file size is decreased to @e maxSize.
 * - If @p offset is not @c CZ_EOF, the trimmed block includes exactly @e minSize bytes starting from the byte at the
 *   zero-based index @p offset. That is, all bytes whose indices lie within the interval [@p offset,
 *   @e minSize + @p offset). Any previous file contents in the interval [@e minSize + @p offset, @e fileSize) are moved
 *   to the memory in the interval [@p offset, @e fileSize - @e minSize). The file size is decreased by @e minSize.
 * 
 * If @p size is zero or @p offset is not @c CZ_EOF and not less than @e fileSize, failure occurs. On failure, the
 * contents of the file are undefined.
 * 
 * The members of @p flags can optionally specify the following behaviour.
 * - If @p flags.relativeToExe is set and @p path is a relative filepath, @p path is interpreted as relative to the
 *   executable file of the program. Otherwise if @p path is relative, it is interpreted as relative to the current
 *   working directory of the program.
 * - @p flags.openSymLink is ignored.
 * 
 * Thread-safety is guaranteed for an invocation @b A to @ref czTrimFile if the following conditions are satisfied.
 * - For any concurrent invocation @b B to @ref czWriteFile, @ref czInsertFile, @ref czClearFile, or @ref czTrimFile,
 *   the @p path arguments of @b A and @b B locate distinct system resources. If they locate the same resource, the
 *   behaviour is undefined.
 * 
 * @param[in] path The path to the file.
 * @param[in] size The number of bytes to trim.
 * @param[in] offset The first byte to trim.
 * @param[in] flags Binary flags describing additional behaviour.
 * 
 * @retval CZ_RESULT_SUCCESS The operation was successful.
 * @retval CZ_RESULT_INTERNAL_ERROR An unexpected or unintended internal event occurred.
 * @retval CZ_RESULT_BAD_ACCESS Permission to read from or write to the file was denied.
 * @retval CZ_RESULT_BAD_ADDRESS @p path was an invalid pointer.
 * @retval CZ_RESULT_BAD_FILE The file was too large or the file type was invalid or unsupported.
 * @retval CZ_RESULT_BAD_OFFSET @p offset was nonzero, not @c CZ_EOF, and greater than or equal to @e fileSize.
 * @retval CZ_RESULT_BAD_PATH @p path was an invalid or unsupported filepath.
 * @retval CZ_RESULT_BAD_SIZE @p size was zero.
 * @retval CZ_RESULT_IN_USE The file was already in use by the system.
 * @retval CZ_RESULT_INTERRUPT An interruption occured due to a signal or IO cancellation.
 * @retval CZ_RESULT_NO_CONNECTION The file was a disconnected FIFO, pipe, or socket.
 * @retval CZ_RESULT_NO_DISK The filesystem or secondary storage unit was full.
 * @retval CZ_RESULT_NO_FILE The file did not exist or was empty.
 * @retval CZ_RESULT_NO_MEMORY Sufficient memory was unable to be allocated.
 * @retval CZ_RESULT_NO_OPEN The maximum number of open files was reached.
 * @retval CZ_RESULT_NO_QUOTA The block or inode quota was exhausted.
 * @retval CZ_RESULT_NO_SUPPORT The operation was unsupported by the platform.
 * 
 * @pre @p path is nonnull and NUL-terminated.
 * 
 * @warning Invalid usage can result in permanent loss of file data.
 */
CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_RD_ACCESS(1)
enum CzResult czTrimFile(const char* path, size_t size, size_t offset, struct CzFileFlags flags);
