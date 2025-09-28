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
#include "config.h"


static bool version_option_callback(void* data, void* arg)
{
	(void) data;
	(void) arg;

	printf(
		CZ_NAME " %d.%d.%d\n"
		CZ_COPYRIGHT "\n"
		CZ_LICENCE "\n",
		CZ_VERSION_MAJOR, CZ_VERSION_MINOR, CZ_VERSION_PATCH);

	return false;
}

static bool help_option_callback(void* data, void* arg)
{
	(void) data;
	(void) arg;

	printf( // TO MAINTAIN 80 COLUMN LIMIT IN TERMINAL OUTPUT, DO NOT CROSS THIS LINE ---> |
		"Usage: " CZ_EXECUTABLE " [options]...\n"
		"\n"
		"Legend:\n"
		"  <arg>                       A single mandatory argument.\n"
		"  [arg]                       A single optional argument.\n"
		"  [arg]...                    A variable number of consecutive optional\n"
		"                              arguments.\n"
		"\n"
		"Options:\n"
		"  -V --version                Output version and licence information, then\n"
		"                              terminate.\n"
		"  -h --help                   Output this hopefully helpful CLI overview, then\n"
		"                              terminate.\n"
		"\n"
		"  -s --silent                 Output no diagnostic information.\n"
		"  -q --quiet                  Output minimal diagnostic information.\n"
		"  -v --verbose                Output maximal diagnostic information.\n"
		"\n"
		"  -n --no-colour              Do not use colour in terminal or file output.\n"
		"  -c --colour                 Use colour in terminal but not file output.\n"
		"                              (default)\n"
		"  -C --colour-all             Use colour in both terminal and file output.\n"
		"\n"
		"  -e --ext-layers             Enable the Khronos extension layers, if present.\n"
		"  -p --profile-layers         Enable the Khronos profiles layer, if present.\n"
		"  -d --validation             Enable the Khronos validation layer, if present.\n"
		"\n"
		"  -i --int16                  Prefer shaders using 16-bit integers where\n"
		"                              appropriate.\n"
		"  -I --int64                  Prefer shaders using 64-bit integers where\n"
		"                              appropriate.\n"
		"\n"
		"  -r --restart                Restart the simulation. Do not save progress nor\n"
		"                              overwrite previous progress.\n"
		"  -b --no-query-benchmarks    Do not benchmark GPU operations via Vulkan\n"
		"                              queries.\n"
		"\n"
		"  --log-allocations <path>    Log all memory allocations performed by Vulkan to\n"
		"                              the file located at <path>.\n"
		"  --capture-pipelines <path>  Output pipeline data captured via the\n"
		"                              VK_KHR_pipeline_executable_properties extension,\n"
		"                              if present, to the file located at <path>.\n"
		"\n"
		"  --iter-size <size>          Set the bit precision of the iterating value in\n"
		"                              shaders to <size>. Higher precision decreases the\n"
		"                              chance of integer overflow, but also decreases\n"
		"                              performance. Must be 128 or 256. Defaults to 128.\n"
		"  --max-loops <count>         Set the maximum number of main loop iterations to\n"
		"                              <count>. More iterations increase the number of\n"
		"                              tested starting values, but also increase\n"
		"                              execution time. Must be a nonnegative integer.\n"
		"                              Defaults to 2^64-1 (UINT64_MAX).\n"
		"  --max-memory <prop>         Limit the usable proportion of GPU heap memory to\n"
		"                              <prop>. Larger proportions may increase\n"
		"                              concurrency, but also increase execution time and\n"
		"                              memory usage. Must be within the interval (0, 1].\n"
		"                              Defaults to 0.4 (40%%).\n");

	return false;
}

static bool silent_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->outputLevel = CZ_OUTPUT_LEVEL_SILENT;
	return true;
}

static bool quiet_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->outputLevel = CZ_OUTPUT_LEVEL_QUIET;
	return true;
}

static bool verbose_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->outputLevel = CZ_OUTPUT_LEVEL_VERBOSE;
	return true;
}

static bool no_colour_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->colourLevel = CZ_COLOUR_LEVEL_NONE;
	return true;
}

static bool colour_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->colourLevel = CZ_COLOUR_LEVEL_TTY;
	return true;
}

static bool colour_all_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->colourLevel = CZ_COLOUR_LEVEL_ALL;
	return true;
}

static bool ext_layers_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->extensionLayers = true;
	return true;
}

static bool profile_layers_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->profileLayers = true;
	return true;
}

static bool validation_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->validationLayers = true;
	return true;
}

static bool int16_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->preferInt16 = true;
	return true;
}

static bool int64_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->preferInt64 = true;
	return true;
}

static bool restart_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->restart = true;
	return true;
}

static bool no_query_benchmarks_option_callback(void* data, void* arg)
{
	(void) arg;

	struct CzConfig* config = (struct CzConfig*) data;
	config->queryBenchmarks = false;
	return true;
}

static bool log_allocations_option_callback(void* data, void* arg)
{
	struct CzConfig* config = (struct CzConfig*) data;
	const char* allocLogPath = *(const char**) arg;

	config->allocLogPath = allocLogPath;
	return true;
}

static bool capture_pipelines_option_callback(void* data, void* arg)
{
	struct CzConfig* config = (struct CzConfig*) data;
	const char* capturePath = *(const char**) arg;

	config->capturePath = capturePath;
	return true;
}

static bool iter_size_option_callback(void* data, void* arg)
{
	struct CzConfig* config = (struct CzConfig*) data;
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
	struct CzConfig* config = (struct CzConfig*) data;
	unsigned long long maxLoops = *(unsigned long long*) arg;

	config->maxLoops = maxLoops;
	return true;
}

static bool max_memory_option_callback(void* data, void* arg)
{
	struct CzConfig* config = (struct CzConfig*) data;
	float maxMemory = *(float*) arg;

	if (maxMemory <= 0 || maxMemory > 1) {
		log_warning(stdout, "Ignoring invalid --max-memory argument %f", (double) maxMemory);
		return true;
	}

	config->maxMemory = maxMemory;
	return true;
}

static bool init_config(int argc, char** argv)
{
	size_t optCount = 20;
	CzCli cli = czCliCreate(&czgConfig, optCount);
	if NOEXPECT (!cli) { return false; }

	czCliAdd(cli, 'V', "version", CZ_CLI_DATATYPE_NONE, version_option_callback);
	czCliAdd(cli, 'h', "help",    CZ_CLI_DATATYPE_NONE, help_option_callback);

	czCliAdd(cli, 's', "silent",  CZ_CLI_DATATYPE_NONE, silent_option_callback);
	czCliAdd(cli, 'q', "quiet",   CZ_CLI_DATATYPE_NONE, quiet_option_callback);
	czCliAdd(cli, 'v', "verbose", CZ_CLI_DATATYPE_NONE, verbose_option_callback);

	czCliAdd(cli, 'n', "no-colour",  CZ_CLI_DATATYPE_NONE, no_colour_option_callback);
	czCliAdd(cli, 'c', "colour",     CZ_CLI_DATATYPE_NONE, colour_option_callback);
	czCliAdd(cli, 'C', "colour-all", CZ_CLI_DATATYPE_NONE, colour_all_option_callback);

	czCliAdd(cli, 'e', "ext-layers",     CZ_CLI_DATATYPE_NONE, ext_layers_option_callback);
	czCliAdd(cli, 'p', "profile-layers", CZ_CLI_DATATYPE_NONE, profile_layers_option_callback);
	czCliAdd(cli, 'd', "validation",     CZ_CLI_DATATYPE_NONE, validation_option_callback);

	czCliAdd(cli, 'i', "int16", CZ_CLI_DATATYPE_NONE, int16_option_callback);
	czCliAdd(cli, 'I', "int64", CZ_CLI_DATATYPE_NONE, int64_option_callback);

	czCliAdd(cli, 'r',  "restart",             CZ_CLI_DATATYPE_NONE, restart_option_callback);
	czCliAdd(cli, 'b',  "no-query-benchmarks", CZ_CLI_DATATYPE_NONE, no_query_benchmarks_option_callback);

	czCliAdd(cli, 0, "log-allocations",   CZ_CLI_DATATYPE_STRING, log_allocations_option_callback);
	czCliAdd(cli, 0, "capture-pipelines", CZ_CLI_DATATYPE_STRING, capture_pipelines_option_callback);

	czCliAdd(cli, 0, "iter-size",  CZ_CLI_DATATYPE_ULONG,  iter_size_option_callback);
	czCliAdd(cli, 0, "max-loops",  CZ_CLI_DATATYPE_ULLONG, max_loops_option_callback);
	czCliAdd(cli, 0, "max-memory", CZ_CLI_DATATYPE_FLOAT,  max_memory_option_callback);

	bool bres = czCliParse(cli, argc, argv);
	if NOEXPECT (!bres) { czCliDestroy(cli); return false; }

	czCliDestroy(cli);
	return true;
}

static bool init_env(void)
{
#if defined(_WIN32)
	HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);

	DWORD mode;
	BOOL bres = GetConsoleMode(output, &mode);
	if NOEXPECT (!bres) { return false; }

	// Enable ANSI escape codes (for pretty coloured output)
	mode |= ENABLE_PROCESSED_OUTPUT;
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;

	bres = SetConsoleMode(output, mode);
	if NOEXPECT (!bres) { return false; }

	// Prevent system from sleeping, but allow display to sleep
	EXECUTION_STATE flags = 0;
	flags |= ES_CONTINUOUS;
	flags |= ES_SYSTEM_REQUIRED;

	SetThreadExecutionState(flags);
#elif defined(__APPLE__)
	// Prevent system from sleeping, but allow display to sleep
	CFStringRef type = kIOPMAssertPreventUserIdleSystemSleep;
	IOPMAssertionLevel level = kIOPMAssertionLevelOn;
	CFStringRef name = CFSTR(CZ_NAME);
	IOPMAssertionID id;

	IOReturn iores = IOPMAssertionCreateWithName(type, level, name, &id);
	if NOEXPECT (iores != kIOReturnSuccess) { return false; }
#elif defined(__unix__)
	// TODO
#endif

	return true;
}

static bool init_gpu(struct Gpu* gpu)
{
	bool bres = create_instance(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = select_device(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = create_device(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = manage_memory(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = create_buffers(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = create_descriptors(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = create_pipeline(gpu);
	if NOEXPECT (!bres) { return false; }

	bres = create_commands(gpu);
	if NOEXPECT (!bres) { return false; }

	return true;
}

int main(int argc, char** argv)
{
	struct Gpu gpu = {0};

	bool bres = init_env();
	if NOEXPECT (!bres) { return EXIT_FAILURE; }

	bres = init_config(argc, argv);
	if (!bres) { return EXIT_SUCCESS; }

	bres = init_colour_level(czgConfig.colourLevel);
	if NOEXPECT (!bres) { return EXIT_FAILURE; }

	bres = init_gpu(&gpu);
	if NOEXPECT (!bres) { goto err_destroy_gpu; }

	bres = submit_commands(&gpu);
	if NOEXPECT (!bres) { goto err_destroy_gpu; }

	destroy_gpu(&gpu);
	return EXIT_SUCCESS;

err_destroy_gpu:
	destroy_gpu(&gpu);
	return EXIT_FAILURE;
}
