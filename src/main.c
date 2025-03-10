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

#include "gpu.h"
#include "cli.h"
#include "debug.h"


#define CHECK_RESULT(func, ...)             \
	do {                                    \
		bres = (func)(__VA_ARGS__);         \
		if EXPECT_FALSE (!bres) {           \
			puts("EXIT FAILURE AT " #func); \
			destroy_gpu(&gpu);              \
			return EXIT_FAILURE;            \
		}                                   \
	}                                       \
	while (0)


static bool version_option_callback(void* data, void* arg)
{
	(void) data;
	(void) arg;

	printf(
		"%s %" PRIu32 ".%" PRIu32 ".%" PRIu32 "\n%s\n%s\n",
		PROGRAM_NAME, PROGRAM_VER_MAJOR, PROGRAM_VER_MINOR, PROGRAM_VER_PATCH, PROGRAM_COPYRIGHT, PROGRAM_LICENCE);

	return false;
}

static bool help_option_callback(void* data, void* arg)
{
	(void) data;
	(void) arg;

	printf(
		"Usage: %s [options]\n"
		"\n"
		"Options:\n"
		"  -V --version                Output version and licence information, then terminate.\n"
		"  -h --help                   Output this hopefully helpful CLI overview, then terminate.\n"
		"\n"
		"  -s --silent                 Output no diagnostic information.\n"
		"  -q --quiet                  Output minimal diagnostic information.\n"
		"  -v --verbose                Output maximal diagnostic information.\n"
		"\n"
		"  -n --no-colour              Do not use colour in terminal and file output.\n"
		"  -c --colour                 Use colour in terminal output, but not file output. (default)\n"
		"  -C --colour-all             Use colour in both terminal and file output.\n"
		"\n"
		"  -e --ext-layers             Enable the Khronos extension layers, if present.\n"
		"  -p --profile-layers         Enable the Khronos profiles layer, if present.\n"
		"  -d --validation             Enable the Khronos validation layer, if present.\n"
		"\n"
		"  --int16                     Prefer shaders using 16-bit integers where appropriate.\n"
		"  --int64                     Prefer shaders using 64-bit integers where appropriate.\n"
		"\n"
		"  -r --restart                Restart the simulation. Do not overwrite previous progress.\n"
		"  -b --no-query-benchmarks    Do not benchmark Vulkan commands via queries.\n"
		"  --log-allocations           Log all memory allocations performed by Vulkan to %s.\n"
		"  --capture-pipelines         Output pipeline data captured via %s, if present.\n"
		"\n"
		"  --iter-size <value>         Bit precision of the iterating value in shaders. Must be 128 or 256.\n"
		"  --max-loops <value>         Maximum number of iterations of the main loop. Must be a non-negative integer.\n"
		"  --max-memory <value>        Maximum proportion of available GPU heap memory to be allocated. Must be within "
			"(0, 1].\n",
		PROGRAM_EXE, ALLOC_LOG_NAME, VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME);

	return false;
}

static bool silent_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->outputLevel = OUTPUT_LEVEL_SILENT;

	return true;
}

static bool quiet_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->outputLevel = OUTPUT_LEVEL_QUIET;

	return true;
}

static bool verbose_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->outputLevel = OUTPUT_LEVEL_VERBOSE;

	return true;
}

static bool no_colour_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->colourLevel = COLOUR_LEVEL_NONE;

	return true;
}

static bool colour_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->colourLevel = COLOUR_LEVEL_TTY;

	return true;
}

static bool colour_all_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->colourLevel = COLOUR_LEVEL_ALL;

	return true;
}

static bool ext_layers_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->extensionLayers = true;

	return true;
}

static bool profile_layers_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->profileLayers = true;

	return true;
}

static bool validation_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->validationLayers = true;

	return true;
}

static bool int16_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->preferInt16 = true;

	return true;
}

static bool int64_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->preferInt64 = true;

	return true;
}

static bool restart_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->restartCount = true;

	return true;
}

static bool no_query_benchmarks_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->queryBenchmarking = false;

	return true;
}

static bool log_allocations_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->logAllocations = true;

	return true;
}

static bool capture_pipelines_option_callback(void* data, void* arg)
{
	(void) arg;

	ProgramConfig* config = (ProgramConfig*) data;

	config->capturePipelines = true;

	return true;
}

static bool iter_size_option_callback(void* data, void* arg)
{
	ProgramConfig* config  = (ProgramConfig*) data;
	unsigned long iterSize = *(unsigned long*) arg;

	if (iterSize != 128 && iterSize != 256) {
		log_warning(stdout, "Ignoring invalid --iter-size argument %lu", iterSize);
		return true;
	}

	config->iterSize = iterSize;

	return true;
}

static bool max_loops_option_callback(void* data, void* arg)
{
	ProgramConfig*     config   = (ProgramConfig*) data;
	unsigned long long maxLoops = *(unsigned long long*) arg;

	config->maxLoops = maxLoops;

	return true;
}

static bool max_memory_option_callback(void* data, void* arg)
{
	ProgramConfig* config    = (ProgramConfig*) data;
	float          maxMemory = *(float*) arg;

	if (maxMemory <= 0 || maxMemory > 1) {
		log_warning(stdout, "Ignoring invalid --max-memory argument %f", (double) maxMemory);
		return true;
	}

	config->maxMemory = maxMemory;

	return true;
}

static bool init_config(int argc, char** argv)
{
	Cli cli = cli_create(&g_config, 20);

	if EXPECT_FALSE (!cli) return false;

	cli_add(cli, 'V', "version", CLI_DATATYPE_NONE, version_option_callback);
	cli_add(cli, 'h', "help",    CLI_DATATYPE_NONE, help_option_callback);

	cli_add(cli, 's', "silent",  CLI_DATATYPE_NONE, silent_option_callback);
	cli_add(cli, 'q', "quiet",   CLI_DATATYPE_NONE, quiet_option_callback);
	cli_add(cli, 'v', "verbose", CLI_DATATYPE_NONE, verbose_option_callback);

	cli_add(cli, 'n', "no-colour",  CLI_DATATYPE_NONE, no_colour_option_callback);
	cli_add(cli, 'c', "colour",     CLI_DATATYPE_NONE, colour_option_callback);
	cli_add(cli, 'C', "colour-all", CLI_DATATYPE_NONE, colour_all_option_callback);

	cli_add(cli, 'e', "ext-layers",     CLI_DATATYPE_NONE, ext_layers_option_callback);
	cli_add(cli, 'p', "profile-layers", CLI_DATATYPE_NONE, profile_layers_option_callback);
	cli_add(cli, 'd', "validation",     CLI_DATATYPE_NONE, validation_option_callback);

	cli_add(cli, '\0', "int16", CLI_DATATYPE_NONE, int16_option_callback);
	cli_add(cli, '\0', "int64", CLI_DATATYPE_NONE, int64_option_callback);

	cli_add(cli, 'r',  "restart",             CLI_DATATYPE_NONE, restart_option_callback);
	cli_add(cli, 'b',  "no-query-benchmarks", CLI_DATATYPE_NONE, no_query_benchmarks_option_callback);
	cli_add(cli, '\0', "log-allocations",     CLI_DATATYPE_NONE, log_allocations_option_callback);
	cli_add(cli, '\0', "capture-pipelines",   CLI_DATATYPE_NONE, capture_pipelines_option_callback);

	cli_add(cli, '\0', "iter-size",  CLI_DATATYPE_ULONG,  iter_size_option_callback);
	cli_add(cli, '\0', "max-loops",  CLI_DATATYPE_ULLONG, max_loops_option_callback);
	cli_add(cli, '\0', "max-memory", CLI_DATATYPE_FLOAT,  max_memory_option_callback);

	bool bres = cli_parse(cli, argc, argv);

	if (!bres) { cli_destroy(cli); return false; }

	cli_destroy(cli);

	return true;
}


static bool init_env(void)
{
#ifdef _WIN32
	HANDLE hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD  dwMode;

	WINBOOL wbres = GetConsoleMode(hOutput, &dwMode);

	if (!wbres) return false;

	// Enable ANSI escape codes
	dwMode |= ENABLE_PROCESSED_OUTPUT;
	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	wbres = SetConsoleMode(hOutput, dwMode);

	if (!wbres) return false;
#endif

	return true;
}


int main(int argc, char** argv)
{
	Gpu gpu = {0};

	bool bres = init_env();

	if (!bres) return EXIT_FAILURE;

	bres = init_config(argc, argv);

	if (!bres) return EXIT_SUCCESS;

	CHECK_RESULT(create_instance, &gpu);
	CHECK_RESULT(select_device, &gpu);
	CHECK_RESULT(create_device, &gpu);
	CHECK_RESULT(manage_memory, &gpu);
	CHECK_RESULT(create_buffers, &gpu);
	CHECK_RESULT(create_descriptors, &gpu);
	CHECK_RESULT(create_pipeline, &gpu);
	CHECK_RESULT(create_commands, &gpu);
	CHECK_RESULT(submit_commands, &gpu);

	destroy_gpu(&gpu);

	return EXIT_SUCCESS;
}
