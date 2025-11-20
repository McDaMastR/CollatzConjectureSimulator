include(CheckIncludeFile)

check_include_file(stdcountof.h HAVE_STDCOUNTOF_H)
check_include_file(sys/file.h HAVE_SYS_FILE_H)
check_include_file(sys/mman.h HAVE_SYS_MMAN_H)
check_include_file(sys/param.h HAVE_SYS_PARAM_H)
check_include_file(sys/select.h HAVE_SYS_SELECT_H)
check_include_file(sys/syscall.h HAVE_SYS_SYSCALL_H)

set(INCLUDE_HEADERS_DARWIN
	<CoreFoundation/CoreFoundation.h$<ANGLE-R>
	<IOKit/pwr_mgt/IOPMLib.h$<ANGLE-R>
)

set(INCLUDE_HEADERS_UNIX
	<dirent.h$<ANGLE-R>
	<fcntl.h$<ANGLE-R>
	<unistd.h$<ANGLE-R>

	<sys/stat.h$<ANGLE-R>
	<sys/types.h$<ANGLE-R>
	<sys/uio.h$<ANGLE-R>
)

set(INCLUDE_HEADERS_WINDOWS
	<winbase.h$<ANGLE-R>
	<wincon.h$<ANGLE-R>
	<windef.h$<ANGLE-R>
	<winerror.h$<ANGLE-R>
	<winnt.h$<ANGLE-R>

	<basetsd.h$<ANGLE-R>
	<fileapi.h$<ANGLE-R>
	<handleapi.h$<ANGLE-R>
	<intrin.h$<ANGLE-R>
	<io.h$<ANGLE-R>
	<ioapiset.h$<ANGLE-R>
	<malloc.h$<ANGLE-R>
	<memoryapi.h$<ANGLE-R>
	<stringapiset.h$<ANGLE-R>
)

set(INCLUDE_HEADERS
	# Standard headers
	<stdalign.h>
	<stdarg.h>
	<stdatomic.h>
	<stdbool.h>
	<stddef.h>
	<stdint.h>
	<stdio.h>
	<stdlib.h>
	<stdnoreturn.h>

	<assert.h>
	<complex.h>
	<ctype.h>
	<errno.h>
	<fenv.h>
	<float.h>
	<inttypes.h>
	<limits.h>
	<locale.h>
	<math.h>
	<setjmp.h>
	<signal.h>
	<string.h>
	<time.h>
	<wchar.h>
	<wctype.h>

	# Required library headers
	<cwalk.h>
	<pthread.h>
	<volk.h>
	<vulkan/vk_enum_string_helper.h>
	<whereami.h>

	# Platform-specific headers
	"$<${USING_DARWIN}:${INCLUDE_HEADERS_DARWIN}>"
	"$<${USING_UNIX}:${INCLUDE_HEADERS_UNIX}>"
	"$<${USING_WINDOWS}:${INCLUDE_HEADERS_WINDOWS}>"

	# Optional headers
	"$<$<BOOL:${HAVE_STDCOUNTOF_H}>:<stdcountof.h$<ANGLE-R>>"
	"$<$<BOOL:${HAVE_SYS_FILE_H}>:<sys/file.h$<ANGLE-R>>"
	"$<$<BOOL:${HAVE_SYS_MMAN_H}>:<sys/mman.h$<ANGLE-R>>"
	"$<$<BOOL:${HAVE_SYS_PARAM_H}>:<sys/param.h$<ANGLE-R>>"
	"$<$<BOOL:${HAVE_SYS_SELECT_H}>:<sys/select.h$<ANGLE-R>>"
	"$<$<BOOL:${HAVE_SYS_SYSCALL_H}>:<sys/syscall.h$<ANGLE-R>>"
)
