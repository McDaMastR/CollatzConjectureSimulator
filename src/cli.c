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

#include "cli.h"
#include "debug.h"
#include "dyarray.h"


typedef union CliData
{
	char c;
	const char* s;
	float f;
	double d;
	long double ld;
	long l;
	long long ll;
	unsigned long ul;
	unsigned long long ull;
} CliData;

typedef struct CliOption
{
	char name[CLI_MAX_OPTION_LENGTH];
	CliDatatype type;
	CliCallback cb;
} CliOption;

typedef struct Cli_T
{
	DyArray opts;
	void*   data;
} Cli_T;


void cli_destroy(Cli cli)
{
	DyArray_destroy(cli->opts);
	free(cli);
}

Cli cli_create(void* data, size_t count)
{
	Cli cli = (Cli) malloc(sizeof(Cli_T));

	if EXPECT_FALSE (!cli) { MALLOC_FAILURE(cli, sizeof(Cli_T)); return NULL; }

	DyArray opts = DyArray_create(sizeof(CliOption), count);

	if EXPECT_FALSE (!opts) { free(cli); return NULL; }

	cli->opts = opts;
	cli->data = data;

	return cli;
}

static bool cli_parse_arg(CliDatatype type, const char* opt, const char* arg, CliData* res)
{
	char* end = NULL;

	memset(res, 0, sizeof(CliData));

	if (type == CLI_DATATYPE_NONE) return true;

	if (!arg) { log_warning(stdout, "Ignoring incomplete option '%s'", opt); return false; }

	switch (type) {
		case CLI_DATATYPE_CHAR:
			res->c = *arg;
			break;

		case CLI_DATATYPE_STRING:
			res->s = arg;
			break;

		case CLI_DATATYPE_FLOAT:;
			float f = strtof(arg, &end);

			if (*end != '\0') { 
				log_warning(stdout, "Partially interpreting argument '%s' as %f", arg, (double) f); }

			res->f = f;
			break;

		case CLI_DATATYPE_DOUBLE:;
			double d = strtod(arg, &end);

			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument '%s' as %f", arg, d); }

			res->d = d;
			break;
		
		case CLI_DATATYPE_LDOUBLE:;
			long double ld = strtold(arg, &end);

			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument '%s' as %Lf", arg, ld); }

			res->ld = ld;
			break;
		
		case CLI_DATATYPE_LONG:;
			long l = strtol(arg, &end, 0);

			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument '%s' as %ld", arg, l); }

			res->l = l;
			break;
		
		case CLI_DATATYPE_LLONG:;
			long long ll = strtoll(arg, &end, 0);

			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument '%s' as %lld", arg, ll); }

			res->ll = ll;
			break;
		
		case CLI_DATATYPE_ULONG:;
			unsigned long ul = strtoul(arg, &end, 0);

			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument '%s' as %lu", arg, ul); }

			res->ul = ul;
			break;
		
		case CLI_DATATYPE_ULLONG:;
			unsigned long long ull = strtoull(arg, &end, 0);

			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument '%s' as %llu", arg, ull); }

			res->ull = ull;
			break;

		default:
			log_error(stderr, "Invalid datatype for option '%s'", opt);
			return false;
	}

	return true;
}

bool cli_parse(Cli cli, int argc, char** argv)
{
	size_t optc = DyArray_size(cli->opts);
	void*  data = cli->data;

	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];

		if (!strncmp(arg, "--", 2)) {
			arg += 2;

			bool found = false;

			for (size_t j = 0; j < optc; j++) {
				CliOption opt;
				DyArray_get(cli->opts, &opt, j);

				if (!strncmp(arg, opt.name, CLI_MAX_OPTION_LENGTH)) {
					found = true;

					CliDatatype type = opt.type;
					CliData     next;

					if (type == CLI_DATATYPE_NONE) {
						bool bres = opt.cb(data, NULL);

						if (!bres) return false;

						break;
					}

					bool bres = cli_parse_arg(type, argv[i], argv[i + 1], &next);

					if (!bres) { i++; break; }

					bres = opt.cb(data, &next);

					if (!bres) return false;

					i++;
					break;
				}
			}

			if (!found) { log_warning(stdout, "Ignoring unknown option '--%s'", arg); }
		}
	}

	return true;
}

bool cli_add(Cli cli, const char* name, CliDatatype type, CliCallback cb)
{
	CliOption newopt = {0};

	strncpy(newopt.name, name, CLI_MAX_OPTION_LENGTH);

	newopt.type = type;
	newopt.cb   = cb;

	void* pres = DyArray_append(cli->opts, &newopt);
	if EXPECT_FALSE (!pres) return false;

	return true;
}
