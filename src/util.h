/* 
 * Copyright (C) 2024  Seth McDonald <seth.i.mcdonald@gmail.com>
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

#include "config.h"


char*   stime(void)        WARN_UNUSED_RET;
clock_t program_time(void) WARN_UNUSED_RET;

uint32_t clz(uint32_t x)        CONST_FUNC WARN_UNUSED_RET;
uint32_t floor_pow2(uint32_t x) CONST_FUNC WARN_UNUSED_RET;

float get_benchmark(clock_t start, clock_t end) CONST_FUNC WARN_UNUSED_RET;

bool set_debug_name(VkDevice device, VkObjectType type, uint64_t handle, const char* name) NULTSTR_ARG(4) RE_ACCESS(4);

bool get_buffer_requirements_noext(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* memoryRequirements) NONNULL_ARGS(4) WR_ACCESS(4);
bool get_buffer_requirements_main4(VkDevice device, VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryRequirements* memoryRequirements) NONNULL_ARGS(4) WR_ACCESS(4);

bool file_size (const char* filename, size_t* size)                  NONNULL_ARGS(1, 2) NULTSTR_ARG(1) RE_ACCESS(1) WR_ACCESS(2);
bool read_file (const char* filename, void*       data, size_t size) NONNULL_ARGS(1, 2) NULTSTR_ARG(1) RE_ACCESS(1) WR_ACCESS(2);
bool write_file(const char* filename, const void* data, size_t size) NONNULL_ARGS(1, 2) NULTSTR_ARG(1) RE_ACCESS(1) RE_ACCESS(2);
