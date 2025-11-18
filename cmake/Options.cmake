set(COMPILE_OPTIONS_GNU
	-ftabstop=4

	"$<$<CONFIG:Debug>:-Og;-g>"
	"$<$<CONFIG:Release>:-O3>"
	"$<$<CONFIG:MinSizeRel>:-Os>"
	"$<$<CONFIG:RelWithDebInfo>:-O2;-g>"
)

set(COMPILE_OPTIONS_MSVC
	/utf-8

	"$<$<CONFIG:Debug>:/Od;/Z7>"
	"$<$<CONFIG:Release>:/GL;/GS-;/Gw;/O2>"
	"$<$<CONFIG:MinSizeRel>:/GS-;/Gw;/O1>"
	"$<$<CONFIG:RelWithDebInfo>:/O2;/Z7>"
)

set(COMPILE_WARNINGS_CLANG
	# Warnings ON
	-Weverything

	# Warnings OFF
	-Wno-bad-function-cast
	-Wno-c++-compat
	-Wno-c23-extensions
	-Wno-c2y-extensions
	-Wno-covered-switch-default
	-Wno-declaration-after-statement
	-Wno-gnu-conditional-omitted-operand
	-Wno-missing-field-initializers
	-Wno-padded
	-Wno-switch-enum
	-Wno-unsafe-buffer-usage
	-Wno-unused-function
)

set(COMPILE_WARNINGS_GNU
	# Warnings ON
	-Wall
	-Wextra
	-Warray-bounds=2
	-Wcast-qual
	-Wconversion
	-Wdate-time
	-Wdisabled-optimization
	-Wdouble-promotion
	-Wfloat-equal
	-Wformat=2
	-Wformat-signedness
	-Winit-self
	-Winvalid-pch
	-Wlogical-op
	-Wmissing-declarations
	-Wmissing-format-attribute
	-Wmissing-include-dirs
	-Wmissing-prototypes
	-Wnested-externs
	-Wold-style-definition
	-Wpacked
	-Wpointer-arith
	-Wredundant-decls
	-Wshadow
	-Wstack-protector
	-Wstrict-overflow=5
	-Wstrict-prototypes
	-Wswitch-default
	-Wsync-nand
	-Wtrampolines
	-Wundef
	-Wunsafe-loop-optimizations
	-Wvector-operation-performance
	-Wwrite-strings

	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,6>:-Wduplicated-cond>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,6>:-Wnull-dereference>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,6>:-Wshift-overflow=2>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,6>:-Wunused-const-variable=2>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Walloc-zero>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Walloca>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Wduplicated-branches>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Wformat-overflow=2>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Wformat-truncation=2>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Wimplicit-fallthrough=5>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Wstringop-overflow=4>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,7>:-Wunused-macros>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,8>:-Wcast-align=strict>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,9>:-Wattribute-alias=2>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,9>:-Wmissing-noreturn>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,12>:-Wbidi-chars=any,ucn>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,12>:-Wopenacc-parallelism>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,12>:-Wtrivial-auto-var-init>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,13>:-Winvalid-utf8>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,14>:-Wflex-array-member-not-at-end>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,14>:-Wmissing-variable-declarations>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,14>:-Wuse-after-free=3>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,15>:-Wc11-c23-compat>"
	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,15>:-Wleading-whitespace=blanks>"

	# Warnings OFF
	-Wno-missing-field-initializers
	-Wno-unused-function

	"$<$<VERSION_GREATER_EQUAL:$<C_COMPILER_VERSION>,15>:-Wno-free-labels>"
)

set(COMPILE_WARNINGS_MSVC
	# Warnings ON
	/Wall

	# Warnings OFF
	/wd4061 # Don't need to have an explicit switch case for every possible value of an enum
	/wd4062 # Don't need to handle every possible value of an enum in a switch statement
	/wd4820 # Don't care about struct padding
)

set(STATIC_ANALYSIS_GNU
	-fanalyzer
)

set(STATIC_ANALYSIS_MSVC
	/analyze
)

set(COMPILE_OPTIONS
	# Compiler-specific general options
	"$<$<C_COMPILER_ID:AppleClang,Clang,GNU>:${COMPILE_OPTIONS_GNU}>"
	"$<$<C_COMPILER_ID:MSVC>:${COMPILE_OPTIONS_MSVC}>"

	# Compiler-specific warning options
	"$<$<AND:$<BOOL:${CZ_EXCESS_WARNINGS}>,$<C_COMPILER_ID:AppleClang,Clang>>:${COMPILE_WARNINGS_CLANG}>"
	"$<$<AND:$<BOOL:${CZ_EXCESS_WARNINGS}>,$<C_COMPILER_ID:GNU>>:${COMPILE_WARNINGS_GNU}>"
	"$<$<AND:$<BOOL:${CZ_EXCESS_WARNINGS}>,$<C_COMPILER_ID:MSVC>>:${COMPILE_WARNINGS_MSVC}>"

	# Compiler-specific static analysis options
	"$<$<AND:$<BOOL:${CZ_STATIC_ANALYSIS}>,$<C_COMPILER_ID:GNU>>:${STATIC_ANALYSIS_GNU}>"
	"$<$<AND:$<BOOL:${CZ_STATIC_ANALYSIS}>,$<C_COMPILER_ID:MSVC>>:${STATIC_ANALYSIS_MSVC}>"
)
