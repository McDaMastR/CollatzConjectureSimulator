/* 
 * Copyright (C) 2024-2025 Seth McDonald
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

#include "def.h"


bool fisatty(FILE* stream) CZ_PURE CZ_USE_RET;

char* stime(void) CZ_NONNULL_RET CZ_USE_RET;
double program_time(void) CZ_USE_RET;

enum CzEndianness get_endianness(void) CZ_CONST CZ_USE_RET;

uint32_t ceil_pow2(uint32_t x) CZ_CONST CZ_USE_RET;
uint32_t floor_pow2(uint32_t x) CZ_CONST CZ_USE_RET;

double get_benchmark(clock_t start, clock_t end) CZ_CONST CZ_USE_RET;

bool set_debug_name(VkDevice device, VkObjectType type, uint64_t handle, const char* name)
	CZ_NONNULL_ARG(1) CZ_NULLTERM_ARG(4) CZ_RE_ACCESS(4);

bool get_buffer_requirements_noext(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements)
	CZ_NONNULL_ARGS CZ_WR_ACCESS(4);
bool get_buffer_requirements_main4(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements)
	CZ_NONNULL_ARGS CZ_WR_ACCESS(4);

bool save_pipeline_cache(VkDevice device, VkPipelineCache cache, const char* filename)
	CZ_NONNULL_ARGS CZ_NULLTERM_ARG(3) CZ_RE_ACCESS(3);

bool file_size(const char* filename, size_t* size) CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1) CZ_RE_ACCESS(1) CZ_WR_ACCESS(2);
bool read_file(const char* filename, void* data, size_t size)
	CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1) CZ_RE_ACCESS(1) CZ_WR_ACCESS(2);
bool write_file(const char* filename, const void* data, size_t size)
	CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1) CZ_RE_ACCESS(1) CZ_RE_ACCESS(2);

bool read_text(const char* filename, const char* format, ...)
	CZ_SCANF(2, 3) CZ_NONNULL_ARGS CZ_NULLTERM_ARG(1) CZ_NULLTERM_ARG(2) CZ_RE_ACCESS(1) CZ_RE_ACCESS(2);
bool write_text(const char* filename, const char* format, ...)
	CZ_PRINTF(2, 3) CZ_NONNULL_ARG(1, 2) CZ_NULLTERM_ARG(1) CZ_NULLTERM_ARG(2) CZ_RE_ACCESS(1) CZ_RE_ACCESS(2);

void* aligned_malloc(size_t size, size_t alignment) CZ_MALLOC CZ_ALLOC_ARG(1) CZ_ALIGN_ARG(2) CZ_USE_RET;
void* aligned_realloc(void* memory, size_t size, size_t alignment)
	CZ_NONNULL_ARGS CZ_ALLOC_ARG(2) CZ_ALIGN_ARG(3) CZ_USE_RET;
void aligned_free(void* memory) CZ_NONNULL_ARGS;

size_t aligned_size(const void* memory) CZ_PURE CZ_NONNULL_ARGS CZ_USE_RET CZ_RE_ACCESS(1);


// Unsigned integer maximum and minimum functions

uint8_t maxu8(uint8_t x, uint8_t y) CZ_CONST CZ_USE_RET;
uint8_t minu8(uint8_t x, uint8_t y) CZ_CONST CZ_USE_RET;

uint16_t maxu16(uint16_t x, uint16_t y) CZ_CONST CZ_USE_RET;
uint16_t minu16(uint16_t x, uint16_t y) CZ_CONST CZ_USE_RET;

uint32_t maxu32(uint32_t x, uint32_t y) CZ_CONST CZ_USE_RET;
uint32_t minu32(uint32_t x, uint32_t y) CZ_CONST CZ_USE_RET;

uint64_t maxu64(uint64_t x, uint64_t y) CZ_CONST CZ_USE_RET;
uint64_t minu64(uint64_t x, uint64_t y) CZ_CONST CZ_USE_RET;

uint8_t maxu8v(size_t count, ...) CZ_CONST CZ_USE_RET;
uint8_t minu8v(size_t count, ...) CZ_CONST CZ_USE_RET;

uint16_t maxu16v(size_t count, ...) CZ_CONST CZ_USE_RET;
uint16_t minu16v(size_t count, ...) CZ_CONST CZ_USE_RET;

uint32_t maxu32v(size_t count, ...) CZ_CONST CZ_USE_RET;
uint32_t minu32v(size_t count, ...) CZ_CONST CZ_USE_RET;

uint64_t maxu64v(size_t count, ...) CZ_CONST CZ_USE_RET;
uint64_t minu64v(size_t count, ...) CZ_CONST CZ_USE_RET;


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
