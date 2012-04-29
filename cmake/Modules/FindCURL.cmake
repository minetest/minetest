#FindCURL.cmake

set(CURL_SOURCE_DIR "" CACHE PATH "Path to curl source directory (optional)")

if( UNIX )
	# Unix
else( UNIX )
	# Windows
endif( UNIX )

# Find include directory

if(NOT CURL_SOURCE_DIR STREQUAL "")
	set(CURL_SOURCE_DIR_INCLUDE
		"${CURL_SOURCE_DIR}/include"
	)

	set(CURL_LIBRARY_NAMES libcurl.a)

	if(WIN32)
		if(MSVC)
			set(CURL_SOURCE_DIR_LIBS "${CURL_SOURCE_DIR}/lib/Release")
			set(CURL_LIBRARY_NAMES curllib.lib)
		else()
			set(CURL_SOURCE_DIR_LIBS "${CURL_SOURCE_DIR}/lib")
			set(CURL_LIBRARY_NAMES libcurl.a libcurl.dll.a)
		endif()
	else()
		set(CURL_SOURCE_DIR_LIBS "${CURL_SOURCE_DIR}/lib/Linux")
		set(CURL_LIBRARY_NAMES libcurl.a)
	endif()

	FIND_PATH(CURL_INCLUDE_DIR NAMES curl.h
		PATHS
		${CURL_SOURCE_DIR_INCLUDE}
		NO_DEFAULT_PATH
	)

	FIND_LIBRARY(CURL_LIBRARY NAMES ${CURL_LIBRARY_NAMES}
		PATHS
		${CURL_SOURCE_DIR_LIBS}
		NO_DEFAULT_PATH
	)

else()

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
endif()

MESSAGE(STATUS "CURL_SOURCE_DIR = ${CURL_SOURCE_DIR}")
MESSAGE(STATUS "CURL_INCLUDE_DIR = ${CURL_INCLUDE_DIR}")
MESSAGE(STATUS "CURL_LIBRARY = ${CURL_LIBRARY}")

# On windows, find the dll for installation
if(WIN32)
	if(MSVC)
		FIND_FILE(CURL_DLL NAMES libcurl.dll
			PATHS
			"${CURL_SOURCE_DIR}/bin/"
			DOC "Path of the libcurl dll (for installation)"
		)
	else()
		FIND_FILE(CURL_DLL NAMES curllib.dll
			PATHS
			"${CURL_SOURCE_DIR}/lib/Release"
			DOC "Path of the curllib dll (for installation)"
		)
	endif()
	MESSAGE(STATUS "CURL_DLL = ${CURL_DLL}")
endif(WIN32)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CURL DEFAULT_MSG CURL_LIBRARY CURL_INCLUDE_DIR)

IF(CURL_FOUND)
  SET(CURL_LIBRARIES ${CURL_LIBRARY})
ELSE(CURL_FOUND)
  SET(CURL_LIBRARIES)
ENDIF(CURL_FOUND)

MARK_AS_ADVANCED(CURL_LIBRARY CURL_INCLUDE_DIR CURL_DLL) 

