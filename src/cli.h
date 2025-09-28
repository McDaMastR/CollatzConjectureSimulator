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

#define CLTZ_CLI_MAX_OPTION_LENGTH 64


typedef struct CzCli_* CzCli;

typedef bool (*CzCliCallback)(void* config, void* arg);

enum CzCliDatatype
{
	CZ_CLI_DATATYPE_NONE    = 0,
	CZ_CLI_DATATYPE_CHAR    = 1,
	CZ_CLI_DATATYPE_STRING  = 2,
	CZ_CLI_DATATYPE_FLOAT   = 3,
	CZ_CLI_DATATYPE_DOUBLE  = 4,
	CZ_CLI_DATATYPE_LDOUBLE = 5,
	CZ_CLI_DATATYPE_LONG    = 6,
	CZ_CLI_DATATYPE_LLONG   = 7,
	CZ_CLI_DATATYPE_ULONG   = 8,
	CZ_CLI_DATATYPE_ULLONG  = 9
};


// TODO Document these functions like the Dy* functions

void czCliDestroy(CzCli cli);

CzCli czCliCreate(void* config, size_t count) FREE_FUNC(czCliDestroy, 1) NO_ACCESS(1) USE_RET;

bool czCliParse(CzCli cli, int argc, char** argv) NONNULL_ALL;

bool czCliAdd(CzCli cli, char option, const char* name, enum CzCliDatatype type, CzCliCallback callback)
	NONNULL_ALL NULTSTR_ARG(3);
