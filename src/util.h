/* 
 * Copyright (C) 2024 Seth McDonald <seth.i.mcdonald@gmail.com>
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

#pragma once

#include "defs.h"


char*   stime(void) USE_RET;
clock_t program_time(void) USE_RET;

Endianness get_endianness(void) CONST_FUNC USE_RET;

uint32_t ceil_pow2(uint32_t x) CONST_FUNC USE_RET;
uint32_t floor_pow2(uint32_t x) CONST_FUNC USE_RET;

float get_benchmark(clock_t start, clock_t end) CONST_FUNC USE_RET;

bool set_debug_name(VkDevice device, VkObjectType type, uint64_t handle, const char* name)
	NONNULL_ARGS(1) NULTSTR_ARG(4) RE_ACCESS(4);

bool get_buffer_requirements_noext(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* memoryRequirements)
	NONNULL_ARGS_ALL WR_ACCESS(4);
bool get_buffer_requirements_main4(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* memoryRequirements)
	NONNULL_ARGS_ALL WR_ACCESS(4);

bool file_size(const char* filename, size_t* size) NONNULL_ARGS_ALL NULTSTR_ARG(1) RE_ACCESS(1) WR_ACCESS(2);
bool read_file(const char* filename, void* data, size_t size) NONNULL_ARGS_ALL NULTSTR_ARG(1) RE_ACCESS(1) WR_ACCESS(2);
bool write_file(const char* filename, const void* data, size_t size)
	NONNULL_ARGS_ALL NULTSTR_ARG(1) RE_ACCESS(1) RE_ACCESS(2);

bool read_text(const char* filename, const char* format, ...)
	SCANF_FUNC(2, 3) NONNULL_ARGS_ALL NULTSTR_ARG(1) NULTSTR_ARG(2) RE_ACCESS(1) RE_ACCESS(2);
bool write_text(const char* filename, const char* format, ...)
	PRINTF_FUNC(2, 3) NONNULL_ARGS(1, 2) NULTSTR_ARG(1) NULTSTR_ARG(2) RE_ACCESS(1) RE_ACCESS(2);

void* aligned_malloc(size_t size, size_t alignment) MALLOC_FUNC ALLOC_ARG(1) ALIGN_ARG(2) USE_RET;
void* aligned_realloc(void* memory, size_t size, size_t alignment) NONNULL_ARGS_ALL ALLOC_ARG(2) ALIGN_ARG(3) USE_RET;
void* aligned_free(void* memory) NONNULL_ARGS_ALL;

size_t aligned_size(const void* memory) NONNULL_ARGS_ALL RE_ACCESS(1) USE_RET;


// Unsigned integer maximum and minimum functions

uint8_t maxu8(uint8_t x, uint8_t y) CONST_FUNC USE_RET;
uint8_t minu8(uint8_t x, uint8_t y) CONST_FUNC USE_RET;

uint16_t maxu16(uint16_t x, uint16_t y) CONST_FUNC USE_RET;
uint16_t minu16(uint16_t x, uint16_t y) CONST_FUNC USE_RET;

uint32_t maxu32(uint32_t x, uint32_t y) CONST_FUNC USE_RET;
uint32_t minu32(uint32_t x, uint32_t y) CONST_FUNC USE_RET;

uint64_t maxu64(uint64_t x, uint64_t y) CONST_FUNC USE_RET;
uint64_t minu64(uint64_t x, uint64_t y) CONST_FUNC USE_RET;

uint8_t maxu8v(size_t count, ...) CONST_FUNC USE_RET;
uint8_t minu8v(size_t count, ...) CONST_FUNC USE_RET;

uint16_t maxu16v(size_t count, ...) CONST_FUNC USE_RET;
uint16_t minu16v(size_t count, ...) CONST_FUNC USE_RET;

uint32_t maxu32v(size_t count, ...) CONST_FUNC USE_RET;
uint32_t minu32v(size_t count, ...) CONST_FUNC USE_RET;

uint64_t maxu64v(size_t count, ...) CONST_FUNC USE_RET;
uint64_t minu64v(size_t count, ...) CONST_FUNC USE_RET;


#if SIZE_MAX == UINT8_MAX
	#define maxz(x, y)    maxu8(x, y)
	#define minz(x, y)    minu8(x, y)
	#define maxzv(c, ...) maxu8v(c, __VA_ARGS__)
	#define minzv(c, ...) minu8v(c, __VA_ARGS__)
#elif SIZE_MAX == UINT16_MAX
	#define maxz(x, y)    maxu16(x, y)
	#define minz(x, y)    minu16(x, y)
	#define maxzv(c, ...) maxu16v(c, __VA_ARGS__)
	#define minzv(c, ...) minu16v(c, __VA_ARGS__)
#elif SIZE_MAX == UINT32_MAX
	#define maxz(x, y)    maxu32(x, y)
	#define minz(x, y)    minu32(x, y)
	#define maxzv(c, ...) maxu32v(c, __VA_ARGS__)
	#define minzv(c, ...) minu32v(c, __VA_ARGS__)
#elif SIZE_MAX == UINT64_MAX
	#define maxz(x, y)    maxu64(x, y)
	#define minz(x, y)    minu64(x, y)
	#define maxzv(c, ...) maxu64v(c, __VA_ARGS__)
	#define minzv(c, ...) minu64v(c, __VA_ARGS__)
#endif
