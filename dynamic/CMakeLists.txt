add_library(cltzDynamic OBJECT)

set_target_properties(
	cltzDynamic

	PROPERTIES

	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	C_EXTENSIONS ON
)

target_link_libraries(cltzDynamic
	PRIVATE
		cltzCommon
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
