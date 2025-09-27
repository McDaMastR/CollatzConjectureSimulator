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

#include "defs.h"


#define CLTZ_CLI_MAX_OPTION_LENGTH 64ULL


typedef struct CltzCli_T* CltzCli;

typedef bool (*CltzCliCallback)(void* config, void* arg);

typedef enum CltzCliDatatype
{
	CLTZ_CLI_DATATYPE_NONE    = 0,
	CLTZ_CLI_DATATYPE_CHAR    = 1,
	CLTZ_CLI_DATATYPE_STRING  = 2,
	CLTZ_CLI_DATATYPE_FLOAT   = 3,
	CLTZ_CLI_DATATYPE_DOUBLE  = 4,
	CLTZ_CLI_DATATYPE_LDOUBLE = 5,
	CLTZ_CLI_DATATYPE_LONG    = 6,
	CLTZ_CLI_DATATYPE_LLONG   = 7,
	CLTZ_CLI_DATATYPE_ULONG   = 8,
	CLTZ_CLI_DATATYPE_ULLONG  = 9
} CltzCliDatatype;


// TODO Document these functions like the Dy* functions

void cltzCliDestroy(CltzCli cli);

CltzCli cltzCliCreate(void* config, size_t count) FREE_FUNC(cltzCliDestroy, 1) NO_ACCESS(1) USE_RET;

bool cltzCliParse(CltzCli cli, int argc, char** argv) NONNULL_ALL;

bool cltzCliAdd(CltzCli cli, char option, const char* name, CltzCliDatatype type, CltzCliCallback callback)
	NONNULL_ALL NULTSTR_ARG(3);
