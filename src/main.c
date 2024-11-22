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

#include "gpu.h"
#include "debug.h"


#define CHECK_RESULT(func)                  \
	do {                                    \
		bres = (func)(&gpu);                \
		if EXPECT_FALSE (!bres) {           \
			destroy_gpu(&gpu);              \
			puts("EXIT FAILURE AT " #func); \
			return EXIT_FAILURE;            \
		}                                   \
	} while (0)


int main(int argc, char** argv)
{
	Gpu gpu = {0};

	bool bres = parse_cmdline(&gpu, argc, argv);
	if (!bres) return EXIT_SUCCESS;

	CHECK_RESULT(create_instance);
	CHECK_RESULT(select_device);
	CHECK_RESULT(create_device);
	CHECK_RESULT(manage_memory);
	CHECK_RESULT(create_buffers);
	CHECK_RESULT(create_descriptors);
	CHECK_RESULT(create_pipeline);
	CHECK_RESULT(create_commands);
	CHECK_RESULT(submit_commands);

	destroy_gpu(&gpu);

	return EXIT_SUCCESS;
}
