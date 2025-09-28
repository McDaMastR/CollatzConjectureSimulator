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
#include "dynamic.h"


union CliData
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
};

struct CliOption
{
	char fullName[CLTZ_CLI_MAX_OPTION_LENGTH];
	char shortName;

	CzCliCallback callback;
	enum CzCliDatatype type;
};

struct CliCallbackData
{
	CzCliCallback callback;
	enum CzCliDatatype type;
	union CliData data;
};

struct CzCli_
{
	DyArray options;
	void* config;
};


void czCliDestroy(struct CzCli_* cli)
{
	if NOEXPECT (!cli) { return; }

	dyarray_destroy(cli->options);
	free(cli);
}

struct CzCli_* czCliCreate(void* config, size_t count)
{
	size_t allocSize = sizeof(struct CzCli_);
	struct CzCli_* cli = malloc(allocSize);
	if NOEXPECT (!cli) { MALLOC_FAILURE(cli, allocSize); return NULL; }

	size_t elmSize = sizeof(struct CliOption);
	size_t elmCount = count;

	DyArray options = dyarray_create(elmSize, elmCount);
	if NOEXPECT (!options) { free(cli); return NULL; }

	cli->options = options;
	cli->config = config;

	return cli;
}

static void czCliParseArg(enum CzCliDatatype type, const char* option, const char* arg, union CliData* result)
{
	char* end = NULL;

	switch (type) {
	case CZ_CLI_DATATYPE_NONE:
		break;

	case CZ_CLI_DATATYPE_CHAR:
		result->c = *arg;
		break;

	case CZ_CLI_DATATYPE_STRING:
		result->s = arg;
		break;

	case CZ_CLI_DATATYPE_FLOAT:;
		float f = strtof(arg, &end);
		if (*end) { 
			log_warning(stdout, "Partially interpreting argument %s for option %s as %f", arg, option, (double) f);
		}

		result->f = f;
		break;

	case CZ_CLI_DATATYPE_DOUBLE:;
		double d = strtod(arg, &end);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %f", arg, option, d);
		}

		result->d = d;
		break;

	case CZ_CLI_DATATYPE_LDOUBLE:;
		long double ld = strtold(arg, &end);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %Lf", arg, option, ld);
		}

		result->ld = ld;
		break;

	case CZ_CLI_DATATYPE_LONG:;
		long l = strtol(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %ld", arg, option, l);
		}

		result->l = l;
		break;

	case CZ_CLI_DATATYPE_LLONG:;
		long long ll = strtoll(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %lld", arg, option, ll);
		}

		result->ll = ll;
		break;

	case CZ_CLI_DATATYPE_ULONG:;
		unsigned long ul = strtoul(arg, &end, 0);
		if (*end) {
			log_warning(stdout, "Partially interpreting argument %s for option %s as %lu", arg, option, ul);
		}

		result->ul = ul;
		break;

	case CZ_CLI_DATATYPE_ULLONG:;
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

bool czCliParse(struct CzCli_* cli, int argc, char** argv)
{
	size_t elmSize = sizeof(struct CliCallbackData);
	DyQueue callbacks = dyqueue_create(elmSize);
	if NOEXPECT (!callbacks) { return false; }

	DyArray options = cli->options;
	size_t optionCount = dyarray_size(options);
	void* config = cli->config;

	struct CliCallbackData callbackData = {0};
	enum CzCliDatatype argType = CZ_CLI_DATATYPE_NONE;
	const char* optionName = NULL;

	for (int i = 1; i < argc; i++) {
		const char* arg = argv[i];

		if (argType) {
			czCliParseArg(argType, optionName, arg, &callbackData.data);

			bool bres = dyqueue_enqueue(callbacks, &callbackData);
			if NOEXPECT (!bres) { dyqueue_destroy(callbacks); return false; }

			argType = CZ_CLI_DATATYPE_NONE;
			optionName = NULL;
		}

		else if (!strncmp(arg, "--", 2)) {
			for (size_t j = 0; j < optionCount; j++) {
				struct CliOption option;
				dyarray_get(options, &option, j);

				if (!strncmp(arg + 2, option.fullName, CLTZ_CLI_MAX_OPTION_LENGTH)) {
					argType = option.type;
					optionName = arg;

					callbackData.callback = option.callback;
					callbackData.type = option.type;
					callbackData.data = (union CliData) {0};

					break;
				}
			}

			if (optionName && !argType) {
				bool bres = dyqueue_enqueue(callbacks, &callbackData);
				if NOEXPECT (!bres) { dyqueue_destroy(callbacks); return false; }
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

					argType = CZ_CLI_DATATYPE_NONE;
					optionName = NULL;
				}

				for (size_t k = 0; k < optionCount; k++) {
					struct CliOption option;
					dyarray_get(options, &option, k);

					if (arg[j] == option.shortName) {
						argType = option.type;
						optionName = arg;

						callbackData.callback = option.callback;
						callbackData.type = option.type;
						callbackData.data = (union CliData) {0};

						break;
					}
				}

				if (optionName && !argType) {
					bool bres = dyqueue_enqueue(callbacks, &callbackData);
					if NOEXPECT (!bres) { dyqueue_destroy(callbacks); return false; }
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

bool czCliAdd(struct CzCli_* cli, char option, const char* name, enum CzCliDatatype type, CzCliCallback callback)
{
	struct CliOption newOption = {0};

	size_t length = strlen(name);
	if NOEXPECT (length > CLTZ_CLI_MAX_OPTION_LENGTH) { return false; }

	strncpy(newOption.fullName, name, CLTZ_CLI_MAX_OPTION_LENGTH);

	newOption.shortName = option;
	newOption.callback = callback;
	newOption.type = type;

	void* pres = dyarray_append(cli->options, &newOption);
	if NOEXPECT (!pres) { return false; }

	return true;
}
