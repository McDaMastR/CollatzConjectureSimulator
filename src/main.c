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

#include "defs.h"

int main(void)
{
	BEGIN_FUNC

	Gpu gpu = {0}; // Stack-allocate majority of variables

	CHECK_RESULT(create_instance(&gpu))
	CHECK_RESULT(select_device(&gpu))
	CHECK_RESULT(create_device(&gpu))
	CHECK_RESULT(manage_memory(&gpu))
	CHECK_RESULT(create_buffers(&gpu))
	CHECK_RESULT(create_descriptors(&gpu))
	CHECK_RESULT(create_pipeline(&gpu))
	CHECK_RESULT(create_commands(&gpu))
	CHECK_RESULT(submit_commands(&gpu))

	destroy_gpu(&gpu);

	END_FUNC
	return EXIT_SUCCESS;
}
