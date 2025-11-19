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

CZ_NONNULL_RET CZ_USE_RET
char* stime(void);
CZ_USE_RET
double program_time(void);

CZ_CONST CZ_USE_RET
enum CzEndianness get_endianness(void);

CZ_CONST CZ_USE_RET
CzU32 ceil_pow2(CzU32 x);
CZ_CONST CZ_USE_RET
CzU32 floor_pow2(CzU32 x);

CZ_CONST CZ_USE_RET
double get_benchmark(clock_t start, clock_t end);

CZ_NONNULL_ARGS(1) CZ_NULTERM_ARG(4) CZ_RD_ACCESS(4)
bool set_debug_name(VkDevice device, VkObjectType type, CzU64 handle, const char* name);

CZ_NONNULL_ARGS() CZ_WR_ACCESS(4)
bool get_buffer_requirements_noext(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements);

CZ_NONNULL_ARGS() CZ_WR_ACCESS(4)
bool get_buffer_requirements_main4(
	VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* requirements);

CZ_NONNULL_ARGS() CZ_NULTERM_ARG(3) CZ_RD_ACCESS(3)
bool save_pipeline_cache(VkDevice device, VkPipelineCache cache, const char* filename);

CZ_SCANF(2, 3) CZ_NONNULL_ARGS() CZ_NULTERM_ARG(1) CZ_NULTERM_ARG(2) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2)
bool read_text(const char* filename, const char* format, ...);

CZ_PRINTF(2, 3) CZ_NONNULL_ARGS(1, 2) CZ_NULTERM_ARG(1) CZ_NULTERM_ARG(2) CZ_RD_ACCESS(1) CZ_RD_ACCESS(2)
bool write_text(const char* filename, const char* format, ...);

// Unsigned integer maximum and minimum functions

CZ_CONST CZ_USE_RET
CzU32 maxu32(CzU32 x, CzU32 y);
CZ_CONST CZ_USE_RET
CzU32 minu32(CzU32 x, CzU32 y);

CZ_CONST CZ_USE_RET
CzU64 maxu64(CzU64 x, CzU64 y);
CZ_CONST CZ_USE_RET
CzU64 minu64(CzU64 x, CzU64 y);

CZ_CONST CZ_USE_RET
CzU32 maxu32v(CzU32 count, ...);
CZ_CONST CZ_USE_RET
CzU32 minu32v(CzU32 count, ...);

CZ_CONST CZ_USE_RET
CzU64 maxu64v(CzU32 count, ...);
CZ_CONST CZ_USE_RET
CzU64 minu64v(CzU32 count, ...);

#if SIZE_MAX == UINT32_MAX
#define maxz(x, y)    maxu32(x, y)
#define minz(x, y)    minu32(x, y)
#define maxzv(c, ...) maxu32v(c, __VA_ARGS__)
#define minzv(c, ...) minu32v(c, __VA_ARGS__)
#elif SIZE_MAX == UINT64_MAX
#define maxz(x, y)    maxu64(x, y)
#define minz(x, y)    minu64(x, y)
#define maxzv(c, ...) maxu64v(c, __VA_ARGS__)
#define minzv(c, ...) minu64v(c, __VA_ARGS__)
#else
#error "Unsupported size_t width"
#endif
