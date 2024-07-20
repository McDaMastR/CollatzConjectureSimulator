/* 
 * Collatz Conjecture Simulator
 * Copyright (C) 2024  Seth Isaiah McDonald <seth.i.mcdonald@gmail.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "defs.h"

int main(void)
{
	BEGIN_FUNC

	Gpu_t gpu = {0};

	CHECK_RESULT(create_instance)
	CHECK_RESULT(create_device)
	CHECK_RESULT(manage_memory)
	CHECK_RESULT(create_buffers)
	CHECK_RESULT(create_descriptors)
	CHECK_RESULT(create_pipeline)
	CHECK_RESULT(create_commands)
	CHECK_RESULT(submit_commands)

	destroy_gpu(&gpu);

	END_FUNC
	return EXIT_SUCCESS;
}
