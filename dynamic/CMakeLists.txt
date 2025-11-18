add_library(cltzDynamic OBJECT)
add_library(cltz::dynamic ALIAS cltzDynamic)

cltz_configure(cltzDynamic)

target_link_libraries(cltzDynamic
	PRIVATE
		cltz::common
)

target_sources(cltzDynamic
	PRIVATE
		dyarray.c
		dynamic.c
		dyqueue.c
		dyrecord.c
		dystring.c
	PUBLIC FILE_SET HEADERS FILES
		dyarray.h
		dynamic.h
		dyqueue.h
		dyrecord.h
		dystring.h
)
