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
	FIND_LIBRARY(GETTEXT_LIBRARY
		NAMES libintl.lib intl.lib libintl3.lib intl3.lib
		PATHS "${CUSTOM_GETTEXT_PATH}/lib"
		DOC "gettext *intl*.lib")
	FIND_LIBRARY(GETTEXT_DLL
		NAMES libintl.dll intl.dll libintl3.dll intl3.dll
		PATHS "${CUSTOM_GETTEXT_PATH}/bin" "${CUSTOM_GETTEXT_PATH}/lib" 
		DOC "gettext *intl*.dll")
	FIND_LIBRARY(GETTEXT_ICONV_DLL
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
		SET(GETTEXT_FOUND TRUE)
	ENDIF(WIN32)
ENDIF()


