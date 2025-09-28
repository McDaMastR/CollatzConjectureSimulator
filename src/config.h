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

#pragma once

#include "common.h"


struct CzConfig
{
	const char* allocLogPath;
	const char* capturePath;

	enum CzOutputLevel outputLevel;
	enum CzColourLevel colourLevel;

	unsigned long iterSize;
	unsigned long long maxLoops;
	float maxMemory;

	bool preferInt16;
	bool preferInt64;

	bool extensionLayers;
	bool profileLayers;
	bool validationLayers;

	bool restart;
	bool queryBenchmarks;
};

extern struct CzConfig czgConfig;
