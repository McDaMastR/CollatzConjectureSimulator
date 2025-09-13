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
	char fullName[CLTZ_CLI_MAX_OPTION_LENGTH];
	char shortName;

	CltzCliCallback callback;
	CltzCliDatatype type;
} CliOption;

typedef struct CliCallbackData
{
	CltzCliCallback callback;
	CltzCliDatatype type;
	CliData data;
} CliCallbackData;

typedef struct CltzCli_T
{
	DyArray options;
	void* config;
} CltzCli_T;


void cltzCliDestroy(CltzCli cli)
{
	if EXPECT_FALSE (!cli) { return; }

	dyarray_destroy(cli->options);
	free(cli);
}

CltzCli cltzCliCreate(void* config, size_t count)
{
	size_t allocSize = sizeof(CltzCli_T);
	CltzCli cli = malloc(allocSize);
	if EXPECT_FALSE (!cli) { MALLOC_FAILURE(cli, allocSize); return NULL; }

	size_t elmSize = sizeof(CliOption);
	size_t elmCount = count;

	DyArray options = dyarray_create(elmSize, elmCount);
	if EXPECT_FALSE (!options) { free(cli); return NULL; }

	cli->options = options;
	cli->config = config;

	return cli;
}

static void cltzCliParseArg(CltzCliDatatype type, const char* option, const char* arg, CliData* result)
{
	char* end = NULL;

	switch (type) {
	case CLTZ_CLI_DATATYPE_NONE:
		break;

	case CLTZ_CLI_DATATYPE_CHAR:
		result->c = *arg;
		break;

	case CLTZ_CLI_DATATYPE_STRING:
		result->s = arg;
		break;

	case CLTZ_CLI_DATATYPE_FLOAT:;
		float f = strtof(arg, &end);
		if (*end) { 
			log_warning(stdout, "Partially interpreting argument %s for option %s as %f", arg, option, (double) f);
		}

		result->f = f;
		break;

	case CLTZ_CLI_DATATYPE_DOUBLE:;
		double d = strtod(arg, &end);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %f", arg, option, d);
		}

		result->d = d;
		break;

	case CLTZ_CLI_DATATYPE_LDOUBLE:;
		long double ld = strtold(arg, &end);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %Lf", arg, option, ld);
		}

		result->ld = ld;
		break;

	case CLTZ_CLI_DATATYPE_LONG:;
		long l = strtol(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %ld", arg, option, l);
		}

		result->l = l;
		break;

	case CLTZ_CLI_DATATYPE_LLONG:;
		long long ll = strtoll(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %lld", arg, option, ll);
		}

		result->ll = ll;
		break;

	case CLTZ_CLI_DATATYPE_ULONG:;
		unsigned long ul = strtoul(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %lu", arg, option, ul);
		}

		result->ul = ul;
		break;

	case CLTZ_CLI_DATATYPE_ULLONG:;
		unsigned long long ull = strtoull(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %llu", arg, option, ull);
		}

		result->ull = ull;
		break;

	default:
		log_error(stderr, "Invalid datatype for option %s", option);
		break;
	}
}

bool cltzCliParse(CltzCli cli, int argc, char** argv)
{
	size_t elmSize = sizeof(CliCallbackData);
	DyQueue callbacks = dyqueue_create(elmSize);
	if EXPECT_FALSE (!callbacks) { return false; }

	DyArray options = cli->options;
	size_t optionCount = dyarray_size(options);
	void* config = cli->config;

	CliCallbackData callbackData = {0};
	CltzCliDatatype argType = CLTZ_CLI_DATATYPE_NONE;
	const char* optionName = NULL;

	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];

		if (argType) {
			cltzCliParseArg(argType, optionName, arg, &callbackData.data);

			bool bres = dyqueue_enqueue(callbacks, &callbackData);
			if EXPECT_FALSE (!bres) { dyqueue_destroy(callbacks); return false; }

			argType = CLTZ_CLI_DATATYPE_NONE;
			optionName = NULL;
		}

		else if (!strncmp(arg, "--", 2)) {
			for (size_t j = 0; j < optionCount; j++) {
				CliOption option;
				dyarray_get(options, &option, j);

				if (!strncmp(arg + 2, option.fullName, CLTZ_CLI_MAX_OPTION_LENGTH)) {
					argType = option.type;
					optionName = arg;

					callbackData.callback = option.callback;
					callbackData.type = option.type;
					callbackData.data = (CliData) {0};

					break;
				}
			}

			if (optionName && !argType) {
				bool bres = dyqueue_enqueue(callbacks, &callbackData);
				if EXPECT_FALSE (!bres) { dyqueue_destroy(callbacks); return false; }
				optionName = NULL;
			}
			else if (!optionName) {
				log_warning(stdout, "Ignoring unknown option %s", arg);
			}
		}

		else if (!strncmp(arg, "-", 1)) {
			for (size_t j = 1; arg[j]; j++) {
				if (argType) {
					log_warning(stdout, "Ignoring incomplete option -%c", arg[j - 1]);

					argType = CLTZ_CLI_DATATYPE_NONE;
					optionName = NULL;
				}

				for (size_t k = 0; k < optionCount; k++) {
					CliOption option;
					dyarray_get(options, &option, k);

					if (arg[j] == option.shortName) {
						argType = option.type;
						optionName = arg;

						callbackData.callback = option.callback;
						callbackData.type = option.type;
						callbackData.data = (CliData) {0};

						break;
					}
				}

				if (optionName && !argType) {
					bool bres = dyqueue_enqueue(callbacks, &callbackData);
					if EXPECT_FALSE (!bres) { dyqueue_destroy(callbacks); return false; }
					optionName = NULL;
				}
				else if (!optionName) {
					log_warning(stdout, "Ignoring unknown option -%c", arg[j]);
				}
			}
		}

		else {
			log_warning(stdout, "Ignoring unknown option %s", arg);
		}
	}

	if (argType) {
		log_warning(stdout, "Ignoring incomplete option %s", optionName);
	}

	size_t callbackCount = dyqueue_size(callbacks);

	for (size_t i = 0; i < callbackCount; i++) {
		dyqueue_dequeue(callbacks, &callbackData);

		bool bres = callbackData.callback(config, callbackData.type ? &callbackData.data : NULL);
		if (!bres) { dyqueue_destroy(callbacks); return false; }
	}

	dyqueue_destroy(callbacks);
	return true;
}

bool cltzCliAdd(CltzCli cli, char option, const char* name, CltzCliDatatype type, CltzCliCallback callback)
{
	CliOption newOption = {0};

	size_t length = strlen(name);
	if EXPECT_FALSE (length > CLTZ_CLI_MAX_OPTION_LENGTH) { return false; }

	strncpy(newOption.fullName, name, CLTZ_CLI_MAX_OPTION_LENGTH);

	newOption.shortName = option;
	newOption.callback = callback;
	newOption.type = type;

	void* pres = dyarray_append(cli->options, &newOption);
	if EXPECT_FALSE (!pres) { return false; }

	return true;
}
