set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(LINK_LIBRARIES_DARWIN
	"$<LINK_LIBRARY:FRAMEWORK,CoreFoundation>"
	"$<LINK_LIBRARY:FRAMEWORK,IOKit>"
)

set(LINK_LIBRARIES
	Threads::Threads
	cwalk::cwalk
	wai::wai

	Vulkan::Headers
	Vulkan::UtilityHeaders
	volk::volk

	# Platform-specific libraries
	"$<${USING_DARWIN}:${LINK_LIBRARIES_DARWIN}>"
)
