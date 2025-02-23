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


typedef enum CliDatatype
{
	CLI_DATATYPE_NONE,
	CLI_DATATYPE_CHAR,
	CLI_DATATYPE_STRING,
	CLI_DATATYPE_FLOAT,
	CLI_DATATYPE_DOUBLE,
	CLI_DATATYPE_LDOUBLE,
	CLI_DATATYPE_LONG,
	CLI_DATATYPE_LLONG,
	CLI_DATATYPE_ULONG,
	CLI_DATATYPE_ULLONG
} CliDatatype;

typedef bool (*CliCallback)(void* data, void* arg);

typedef struct Cli_T* Cli;


// TODO Document these functions like the Dy* functions

void cli_destroy(Cli cli);

Cli cli_create(void* data, size_t count) FREE_FUNC(cli_destroy, 1) NO_ACCESS(1) USE_RET;

bool cli_parse(Cli cli, int argc, char** argv) NONNULL_ARGS_ALL;

bool cli_add(Cli cli, const char* name, CliDatatype type, CliCallback cb) NONNULL_ARGS_ALL NULTSTR_ARG(2);
