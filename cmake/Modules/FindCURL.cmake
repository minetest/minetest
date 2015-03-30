mark_as_advanced(CURL_LIBRARY CURL_INCLUDE_DIR)

find_library(CURL_LIBRARY NAMES curl)
find_path(CURL_INCLUDE_DIR NAMES curl/curl.h)

set(CURL_REQUIRED_VARS CURL_LIBRARY CURL_INCLUDE_DIR)

if(WIN32)
	find_file(CURL_DLL NAMES libcurl-4.dll
		PATHS
		"C:/Windows/System32"
		DOC "Path to the cURL DLL (for installation)")
	mark_as_advanced(CURL_DLL)
	set(CURL_REQUIRED_VARS ${CURL_REQUIRED_VARS} CURL_DLL)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CURL DEFAULT_MSG ${CURL_REQUIRED_VARS})

