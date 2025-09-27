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

#include "defs.h"


ProgramConfig g_config = {
	.allocLogPath = NULL,
	.capturePath = NULL,
	.outputLevel = OUTPUT_LEVEL_DEFAULT,
	.colourLevel = COLOUR_LEVEL_TTY,
	.iterSize = 128,
	.maxLoops = ULLONG_MAX,
	.maxMemory = .4f,
	.preferInt16 = false,
	.preferInt64 = false,
	.extensionLayers = false,
	.profileLayers = false,
	.validationLayers = false,
	.restart = false,
	.queryBenchmarks = true
};
