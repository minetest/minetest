# Look for JsonCpp, with fallback to bundeled version

mark_as_advanced(JSON_LIBRARY JSON_INCLUDE_DIR)
option(ENABLE_SYSTEM_JSONCPP "Enable using a system-wide JsonCpp" TRUE)
set(USE_SYSTEM_JSONCPP FALSE)

if(ENABLE_SYSTEM_JSONCPP)
	find_library(JSON_LIBRARY NAMES jsoncpp)
	find_path(JSON_INCLUDE_DIR json/allocator.h PATH_SUFFIXES jsoncpp)

	if(JSON_LIBRARY AND JSON_INCLUDE_DIR)
		message(STATUS "Using JsonCpp provided by system.")
		set(USE_SYSTEM_JSONCPP TRUE)
	endif()
endif()

if(NOT USE_SYSTEM_JSONCPP)
	message(STATUS "Using bundled JsonCpp library.")
	set(JSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/jsoncpp)
	set(JSON_LIBRARY jsoncpp)
	add_subdirectory(lib/jsoncpp)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Json DEFAULT_MSG JSON_LIBRARY JSON_INCLUDE_DIR)
