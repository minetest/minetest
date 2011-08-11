# Package finder for gettext libs and include files

SET(CUSTOM_GETTEXT_PATH "${PROJECT_SOURCE_DIR}/../../gettext"
	CACHE FILEPATH "path to custom gettext")

# by default
SET(GETTEXT_FOUND FALSE)

FIND_PATH(GETTEXT_INCLUDE_DIR
	NAMES libintl.h
	PATHS "${CUSTOM_GETTEXT_PATH}/include"
	DOC "gettext include directory")

FIND_PROGRAM(GETTEXT_MSGFMT
	NAMES msgfmt
	PATHS "${CUSTOM_GETTEXT_PATH}/bin"
	DOC "path to msgfmt")

# modern Linux, as well as Mac, seem to not need require special linking
# they do not because gettext is part of glibc
# TODO check the requirements on other BSDs and older Linux
IF (WIN32)
	IF(MSVC)
		SET(GETTEXT_LIB_NAMES
			libintl.lib intl.lib libintl3.lib intl3.lib)
	ELSE()
		SET(GETTEXT_LIB_NAMES
			libintl.dll.a intl.dll.a libintl3.dll.a intl3.dll.a)
	ENDIF()
	FIND_LIBRARY(GETTEXT_LIBRARY
		NAMES ${GETTEXT_LIB_NAMES}
		PATHS "${CUSTOM_GETTEXT_PATH}/lib"
		DOC "gettext *intl*.lib")
	FIND_FILE(GETTEXT_DLL
		NAMES libintl.dll intl.dll libintl3.dll intl3.dll
		PATHS "${CUSTOM_GETTEXT_PATH}/bin" "${CUSTOM_GETTEXT_PATH}/lib" 
		DOC "gettext *intl*.dll")
	FIND_FILE(GETTEXT_ICONV_DLL
		NAMES libiconv2.dll
		PATHS "${CUSTOM_GETTEXT_PATH}/bin" "${CUSTOM_GETTEXT_PATH}/lib"
		DOC "gettext *iconv*.lib")
ENDIF(WIN32)

IF(GETTEXT_INCLUDE_DIR AND GETTEXT_MSGFMT)
	IF (WIN32)
		# in the Win32 case check also for the extra linking requirements
		IF(GETTEXT_LIBRARY AND GETTEXT_DLL AND GETTEXT_ICONV_DLL)
			SET(GETTEXT_FOUND TRUE)
		ENDIF()
	ELSE(WIN32)
		# *BSD variants require special linkage as they don't use glibc
		IF(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
			SET(GETTEXT_LIBRARY "intl")
		ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "BSD")
		SET(GETTEXT_FOUND TRUE)
	ENDIF(WIN32)
ENDIF()

IF(GETTEXT_FOUND)
	SET(GETTEXT_PO_PATH ${CMAKE_SOURCE_DIR}/po)
	SET(GETTEXT_MO_BUILD_PATH ${CMAKE_BINARY_DIR}/locale/<locale>/LC_MESSAGES)
	SET(GETTEXT_MO_DEST_PATH ${DATADIR}/../locale/<locale>/LC_MESSAGES)
	FILE(GLOB GETTEXT_AVAILABLE_LOCALES RELATIVE ${GETTEXT_PO_PATH} "${GETTEXT_PO_PATH}/*")
	LIST(REMOVE_ITEM GETTEXT_AVAILABLE_LOCALES minetest.pot)
	MACRO(SET_MO_PATHS _buildvar _destvar _locale)
		STRING(REPLACE "<locale>" ${_locale} ${_buildvar} ${GETTEXT_MO_BUILD_PATH})
		STRING(REPLACE "<locale>" ${_locale} ${_destvar} ${GETTEXT_MO_DEST_PATH})
	ENDMACRO(SET_MO_PATHS)
ELSE()
	SET(GETTEXT_INCLUDE_DIR "")
	SET(GETTEXT_LIBRARY "")
ENDIF()
