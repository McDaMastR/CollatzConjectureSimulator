add_library(cltzCommon OBJECT)
add_library(cltz::common ALIAS cltzCommon)

cltz_configure(cltzCommon)

target_sources(cltzCommon
	PRIVATE
		alloc.c
		debug.c
		file.c
		util.c
		wrap.c
		wrap_posix.c
		wrap_stdc.c
		wrap_win32.c
	PUBLIC FILE_SET HEADERS FILES
		alloc.h
		common.h
		debug.h
		def.h
		file.h
		util.h
		wrap.h
		wrap_posix.h
		wrap_stdc.h
		wrap_win32.h
)
