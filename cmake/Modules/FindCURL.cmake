# - Find CURL
# Find the native CURL includes and libraries
#
#  CURL_INCLUDE_DIR - where to find vorbis.h, etc.
#  CURL_LIBRARIES   - List of libraries when using vorbis(file).
#  CURL_FOUND       - True if vorbis found.
if( UNIX )
	FIND_PATH(CURL_INCLUDE_DIR NAMES curl.h
		PATHS
		/usr/local/include/curl
		/usr/include/curl
	)

	FIND_LIBRARY(CURL_LIBRARY NAMES libcurl.a curl
		PATHS
		/usr/local/lib
		/usr/lib
	)
else( UNIX )
	 if(NOT GP2XWIZ)
        if(CURL_INCLUDE_DIR)
            # Already in cache, be silent
            set(CURL_FIND_QUIETLY TRUE)
        endif(CURL_INCLUDE_DIR)
        find_library(CURL_LIBRARY NAMES curl)
        # Handle the QUIETLY and REQUIRED arguments and set VORBIS_FOUND
        # to TRUE if all listed variables are TRUE.
        include(FindPackageHandleStandardArgs)
        find_package_handle_standard_args(CURL DEFAULT_MSG CURL_INCLUDE_DIR CURL_LIBRARY)
    else(NOT GP2XWIZ)
        find_package_handle_standard_args(VORBIS DEFAULT_MSG
            CURL_INCLUDE_DIR CURL_LIBRARY)
    endif(NOT GP2XWIZ)
        
    if(CURL_FOUND)
      if(NOT GP2XWIZ)
         set(CURL_LIBRARY ${CURL_LIBRARY})
      else(NOT GP2XWIZ)
         set(CURL_LIBRARY ${CURL_LIBRARY})
      endif(NOT GP2XWIZ)
    else(CURL_FOUND)
      set(CURL_LIBRARY)
    endif(CURL_FOUND)
endif( UNIX )

MESSAGE(STATUS "CURL_SOURCE_DIR = ${CURL_SOURCE_DIR}")
MESSAGE(STATUS "CURL_INCLUDE_DIR = ${CURL_INCLUDE_DIR}")
MESSAGE(STATUS "CURL_LIBRARY = ${CURL_LIBRARY}")
