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
#include "dyqueue.h"


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
	char fullName[CLI_MAX_OPTION_LENGTH];
	char shortName;

	CliCallback cb;
	CliDatatype type;
} CliOption;

typedef struct CliCallbackData
{
	CliCallback cb;
	CliDatatype type;
	CliData     data;
} CliCallbackData;

typedef struct Cli_T
{
	DyArray opts;
	void*   config;
} Cli_T;


void cli_destroy(Cli cli)
{
	if EXPECT_FALSE (!cli) return;

	dyarray_destroy(cli->opts);
	free(cli);
}

Cli cli_create(void* config, size_t count)
{
	Cli cli = (Cli) malloc(sizeof(Cli_T));
	if EXPECT_FALSE (!cli) { MALLOC_FAILURE(cli, sizeof(Cli_T)); return NULL; }

	DyArray opts = dyarray_create(sizeof(CliOption), count);
	if EXPECT_FALSE (!opts) { free(cli); return NULL; }

	cli->opts   = opts;
	cli->config = config;

	return cli;
}

static void cli_parse_arg(CliDatatype type, const char* opt, const char* arg, CliData* res)
{
	char* end = NULL;

	switch (type) {
		case CLI_DATATYPE_NONE:
			break;

		case CLI_DATATYPE_CHAR:
			res->c = *arg;
			break;

		case CLI_DATATYPE_STRING:
			res->s = arg;
			break;

		case CLI_DATATYPE_FLOAT:;
			float f = strtof(arg, &end);
			if (*end != '\0') { 
				log_warning(stdout, "Partially interpreting argument %s for option %s as %f", arg, opt, (double) f); }

			res->f = f;
			break;

		case CLI_DATATYPE_DOUBLE:;
			double d = strtod(arg, &end);
			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument %s for option %s as %f", arg, opt, d); }

			res->d = d;
			break;

		case CLI_DATATYPE_LDOUBLE:;
			long double ld = strtold(arg, &end);
			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument %s for option %s as %Lf", arg, opt, ld); }

			res->ld = ld;
			break;

		case CLI_DATATYPE_LONG:;
			long l = strtol(arg, &end, 0);
			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument %s for option %s as %ld", arg, opt, l); }

			res->l = l;
			break;

		case CLI_DATATYPE_LLONG:;
			long long ll = strtoll(arg, &end, 0);
			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument %s for option %s as %lld", arg, opt, ll); }

			res->ll = ll;
			break;

		case CLI_DATATYPE_ULONG:;
			unsigned long ul = strtoul(arg, &end, 0);
			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument %s for option %s as %lu", arg, opt, ul); }

			res->ul = ul;
			break;

		case CLI_DATATYPE_ULLONG:;
			unsigned long long ull = strtoull(arg, &end, 0);
			if (*end != '\0') {
				log_warning(stdout, "Partially interpreting argument %s for option %s as %llu", arg, opt, ull); }

			res->ull = ull;
			break;

		default:
			log_error(stderr, "Invalid datatype for option %s", opt);
			break;
	}
}

bool cli_parse(Cli cli, int argc, char** argv)
{
	DyQueue callbacks = dyqueue_create(sizeof(CliCallbackData));
	if EXPECT_FALSE (!callbacks) return false;

	DyArray opts   = cli->opts;
	size_t  optc   = dyarray_size(opts);
	void*   config = cli->config;

	CliDatatype argType = CLI_DATATYPE_NONE;
	const char* optName = NULL;

	CliCallbackData cbd = {0};

	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];

		if (argType) {
			cli_parse_arg(argType, optName, arg, &cbd.data);

			void* pres = dyqueue_enqueue(callbacks, &cbd);
			if EXPECT_FALSE (!pres) { dyqueue_destroy(callbacks); return false; }

			argType = CLI_DATATYPE_NONE;
			optName = NULL;
		}

		else if (!strncmp(arg, "--", 2)) {
			for (size_t j = 0; j < optc; j++) {
				CliOption opt;
				dyarray_get(opts, &opt, j);

				if (!strncmp(arg + 2, opt.fullName, CLI_MAX_OPTION_LENGTH)) {
					argType = opt.type;
					optName = arg;

					cbd.cb   = opt.cb;
					cbd.type = opt.type;
					cbd.data = (CliData) {0};

					break;
				}
			}

			if (optName && !argType) {
				void* pres = dyqueue_enqueue(callbacks, &cbd);
				if EXPECT_FALSE (!pres) { dyqueue_destroy(callbacks); return false; }

				optName = NULL;
			}
			else if (!optName) { log_warning(stdout, "Ignoring unknown option %s", arg); }
		}

		else if (!strncmp(arg, "-", 1)) {
			for (size_t j = 1; arg[j] != '\0'; j++) {
				if (argType) {
					log_warning(stdout, "Ignoring incomplete option -%c", arg[j - 1]);

					argType = CLI_DATATYPE_NONE;
					optName = NULL;
				}

				for (size_t k = 0; k < optc; k++) {
					CliOption opt;
					dyarray_get(opts, &opt, k);

					if (arg[j] == opt.shortName) {
						argType = opt.type;
						optName = arg;

						cbd.cb   = opt.cb;
						cbd.type = opt.type;
						cbd.data = (CliData) {0};

						break;
					}
				}

				if (optName && !argType) {
					void* pres = dyqueue_enqueue(callbacks, &cbd);
					if EXPECT_FALSE (!pres) { dyqueue_destroy(callbacks); return false; }

					optName = NULL;
				}
				else if (!optName) { log_warning(stdout, "Ignoring unknown option -%c", arg[j]); }
			}
		}

		else { log_warning(stdout, "Ignoring unknown option %s", arg); }
	}

	if (argType) { log_warning(stdout, "Ignoring incomplete option %s", optName); }

	size_t cbc = dyqueue_size(callbacks);

	for (size_t i = 0; i < cbc; i++) {
		dyqueue_dequeue(callbacks, &cbd);

		bool bres = cbd.cb(config, cbd.type ? &cbd.data : NULL);
		if (!bres) { dyqueue_destroy(callbacks); return false; }
	}

	dyqueue_destroy(callbacks);

	return true;
}

bool cli_add(Cli cli, char opt, const char* name, CliDatatype type, CliCallback cb)
{
	CliOption newopt = {0};

	size_t len = strlen(name);
	if EXPECT_FALSE (len > CLI_MAX_OPTION_LENGTH) return false;

	strncpy(newopt.fullName, name, CLI_MAX_OPTION_LENGTH);

	newopt.shortName = opt;
	newopt.cb        = cb;
	newopt.type      = type;

	void* pres = dyarray_append(cli->opts, &newopt);
	if EXPECT_FALSE (!pres) return false;

	return true;
}
