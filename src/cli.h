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


#define CLI_MAX_OPTION_LENGTH 64ULL


typedef struct Cli_T* Cli;

typedef bool (*CliCallback)(void* config, void* arg);

typedef enum CliDatatype
{
	CLI_DATATYPE_NONE    = 0,
	CLI_DATATYPE_CHAR    = 1,
	CLI_DATATYPE_STRING  = 2,
	CLI_DATATYPE_FLOAT   = 3,
	CLI_DATATYPE_DOUBLE  = 4,
	CLI_DATATYPE_LDOUBLE = 5,
	CLI_DATATYPE_LONG    = 6,
	CLI_DATATYPE_LLONG   = 7,
	CLI_DATATYPE_ULONG   = 8,
	CLI_DATATYPE_ULLONG  = 9
} CliDatatype;


// TODO Document these functions like the Dy* functions

void cli_destroy(Cli cli);

Cli cli_create(void* config, size_t count) FREE_FUNC(cli_destroy, 1) NO_ACCESS(1) USE_RET;

bool cli_parse(Cli cli, int argc, char** argv) NONNULL_ARGS_ALL;

bool cli_add(Cli cli, char option, const char* name, CliDatatype type, CliCallback callback)
	NONNULL_ARGS_ALL NULTSTR_ARG(3);
