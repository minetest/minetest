# - Find curl
# Find the native CURL headers and libraries.
#
#  CURL_INCLUDE_DIR - where to find curl/curl.h, etc.
#  CURL_LIBRARY    - List of libraries when using curl.
#  CURL_FOUND        - True if curl found.

if( UNIX )
  FIND_PATH(CURL_INCLUDE_DIR NAMES curl.h
    PATHS
    /usr/local/include/curl
    /usr/include/curl
  )

  FIND_LIBRARY(CURL_LIBRARY NAMES curl
    PATHS
    /usr/local/lib
    /usr/lib
  )
else( UNIX )
  FIND_PATH(CURL_INCLUDE_DIR NAMES curl/curl.h) # Look for the header file.
  FIND_LIBRARY(CURL_LIBRARY NAMES curl) # Look for the library.
  FIND_FILE(CURL_DLL NAMES libcurl.dll
			PATHS
			"c:/windows/system32"
			DOC "Path of the cURL dll (for installation)")
  INCLUDE(FindPackageHandleStandardArgs) # handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(CURL DEFAULT_MSG CURL_LIBRARY CURL_INCLUDE_DIR) # all listed variables are TRUE
endif( UNIX )

if( WIN32 )
  if( CURL_LIBRARY AND CURL_INCLUDE_DIR AND CURL_DLL ) # libcurl.dll is required on Windows
    SET(CURL_FOUND TRUE)
  else( CURL_LIBRARY AND CURL_INCLUDE_DIR AND CURL_DLL )
    SET(CURL_FOUND FALSE)
  endif( CURL_LIBRARY AND CURL_INCLUDE_DIR AND CURL_DLL )
else ( WIN32 )
  if( CURL_LIBRARY AND CURL_INCLUDE_DIR )
    SET(CURL_FOUND TRUE)
  else( CURL_LIBRARY AND CURL_INCLUDE_DIR )
    SET(CURL_FOUND FALSE)
  endif( CURL_LIBRARY AND CURL_INCLUDE_DIR )
endif ( WIN32 )

MESSAGE(STATUS "CURL_INCLUDE_DIR = ${CURL_INCLUDE_DIR}")
MESSAGE(STATUS "CURL_LIBRARY = ${CURL_LIBRARY}")
MESSAGE(STATUS "CURL_DLL = ${CURL_DLL}")
