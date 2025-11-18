set(MACRO_DEFINITIONS_DARWIN
	# Enable Darwin extensions on macOS or iOS
	_DARWIN_C_SOURCE=1
	# Use 64-bit ino_t on macOS 10.5 or above
	_DARWIN_USE_64_BIT_INODE=1
)

set(MACRO_DEFINITIONS_MINGW
	# Enable warnings for using functions MSVC considers deprecated or unsafe
	"$<$<BOOL:${CZ_EXCESS_WARNINGS}>:__MINGW_MSVC_COMPAT_WARNINGS=1>"
)

set(MACRO_DEFINITIONS_UNIX
	# Enable .*at functions on glibc 2.4 or above
	_ATFILE_SOURCE=1
	# Enable miscellaneous BSD and System V extensions on glibc 2.19 or above
	_DEFAULT_SOURCE=1
	# Enable POSIX.1-2024 definitions if supported
	_POSIX_C_SOURCE=202405L
	# Enable SUSv5 definitions if supported
	_XOPEN_SOURCE=800

	# Use 64-bit file/IO functions and datatypes if supported
	_FILE_OFFSET_BITS=64
	# Use 64-bit time_t on glibc 2.34 or above
	_TIME_BITS=64
)

set(MACRO_DEFINITIONS_WINDOWS
	# Enable POSIX functions unprefixed with underscores
	_CRT_DECLARE_NONSTDC_NAMES=1
	# Disable warnings for using POSIX functions unprefixed with underscores
	_CRT_NONSTDC_NO_WARNINGS=1
	# Disable warnings for using functions MSVC considers unsafe
	_CRT_SECURE_NO_WARNINGS=1
)

set(MACRO_DEFINITIONS
	# Name of the project/repository
	CZ_NAME="Collatz Conjecture Simulator"
	# Name of the compiled executable file
	CZ_EXECUTABLE="$<TARGET_FILE_NAME:cltzExe>"
	# Copyright of the program/project
	"CZ_COPYRIGHT=\"Copyright (C) 2025 Seth McDonald\""
	# Licence governing the above copyright
	CZ_LICENCE="Licence GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>"

	# Version of the project
	"CZ_VERSION_MAJOR=${cltz_VERSION_MAJOR}"
	"CZ_VERSION_MINOR=${cltz_VERSION_MINOR}"
	"CZ_VERSION_PATCH=${cltz_VERSION_PATCH}"

	# Define CZ_DEBUG as 1 if in Debug config, and as 0 otherwise
	"CZ_DEBUG=$<CONFIG:DEBUG>"
	# Disable stdc assertions when not in Debug config
	"$<$<NOT:$<CONFIG:DEBUG>>:NDEBUG=1>"

	# Platform-specific definitions
	"$<${USING_DARWIN}:${MACRO_DEFINITIONS_DARWIN}>"
	"$<${USING_MINGW}:${MACRO_DEFINITIONS_MINGW}>"
	"$<${USING_UNIX}:${MACRO_DEFINITIONS_UNIX}>"
	"$<${USING_WINDOWS}:${MACRO_DEFINITIONS_WINDOWS}>"
)
