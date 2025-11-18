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
	PUBLIC FILE_SET HEADERS FILES
		alloc.h
		common.h
		debug.h
		def.h
		file.h
		util.h
		wrap.h
)
