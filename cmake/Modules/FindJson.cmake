# Look for JSONCPP if asked to.
# We use a bundled version by default because some distros ship versions of
# JSONCPP that cause segfaults and other memory errors when we link with them.
# See https://github.com/minetest/minetest/issues/1793

mark_as_advanced(JSON_LIBRARY JSON_INCLUDE_DIR)
option(ENABLE_SYSTEM_JSONCPP "Enable using a system-wide JSONCPP.  May cause segfaults and other memory errors!" FALSE)

if(ENABLE_SYSTEM_JSONCPP)
	find_library(JSON_LIBRARY NAMES jsoncpp)
	find_path(JSON_INCLUDE_DIR json/features.h PATH_SUFFIXES jsoncpp)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(JSONCPP DEFAULT_MSG JSON_LIBRARY JSON_INCLUDE_DIR)

	if(JSONCPP_FOUND)
		message(STATUS "Using system JSONCPP library.")
	endif()
endif()

if(NOT JSONCPP_FOUND)
	message(STATUS "Using bundled JSONCPP library.")
	set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/jsoncpp)
	set(JSON_LIBRARY jsoncpp)
	add_subdirectory(lib/jsoncpp)
endif()
