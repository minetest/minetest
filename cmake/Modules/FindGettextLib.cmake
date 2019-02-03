
set(CUSTOM_GETTEXT_PATH "${PROJECT_SOURCE_DIR}/../../gettext"
	CACHE FILEPATH "path to custom gettext")

find_path(GETTEXT_INCLUDE_DIR
	NAMES libintl.h
	PATHS "${CUSTOM_GETTEXT_PATH}/include"
	DOC "GetText include directory")

find_program(GETTEXT_MSGFMT
	NAMES msgfmt
	PATHS "${CUSTOM_GETTEXT_PATH}/bin"
	DOC "Path to msgfmt")

set(GETTEXT_REQUIRED_VARS GETTEXT_INCLUDE_DIR GETTEXT_MSGFMT)

if(APPLE)
	find_library(GETTEXT_LIBRARY
		NAMES libintl.a
		PATHS "${CUSTOM_GETTEXT_PATH}/lib"
		DOC "GetText library")

	find_library(ICONV_LIBRARY
		NAMES libiconv.dylib
		PATHS "/usr/lib"
		DOC "IConv library")
	set(GETTEXT_REQUIRED_VARS ${GETTEXT_REQUIRED_VARS} GETTEXT_LIBRARY ICONV_LIBRARY)
endif(APPLE)

# Modern Linux, as well as OSX, does not require special linking because
# GetText is part of glibc.
# TODO: check the requirements on other BSDs and older Linux
if(WIN32)
	if(MSVC)
		set(GETTEXT_LIB_NAMES
			libintl.lib intl.lib libintl3.lib intl3.lib)
	else()
		set(GETTEXT_LIB_NAMES
			libintl.dll.a intl.dll.a libintl3.dll.a intl3.dll.a)
	endif()
	find_library(GETTEXT_LIBRARY
		NAMES ${GETTEXT_LIB_NAMES}
		PATHS "${CUSTOM_GETTEXT_PATH}/lib"
		DOC "GetText library")
	find_file(GETTEXT_DLL
		NAMES libintl.dll intl.dll libintl3.dll intl3.dll
		PATHS "${CUSTOM_GETTEXT_PATH}/bin" "${CUSTOM_GETTEXT_PATH}/lib" 
		DOC "gettext *intl*.dll")
	find_file(GETTEXT_ICONV_DLL
		NAMES libiconv2.dll
		PATHS "${CUSTOM_GETTEXT_PATH}/bin" "${CUSTOM_GETTEXT_PATH}/lib"
		DOC "gettext *iconv*.lib")
	set(GETTEXT_REQUIRED_VARS ${GETTEXT_REQUIRED_VARS} GETTEXT_DLL GETTEXT_ICONV_DLL)
endif(WIN32)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GetText DEFAULT_MSG ${GETTEXT_REQUIRED_VARS})


if(GETTEXT_FOUND)
	# BSD variants require special linkage as they don't use glibc
	if(${CMAKE_SYSTEM_NAME} MATCHES "BSD|DragonFly")
		set(GETTEXT_LIBRARY "intl")
	endif()

	set(GETTEXT_PO_PATH ${CMAKE_SOURCE_DIR}/po)
	set(GETTEXT_MO_BUILD_PATH ${CMAKE_BINARY_DIR}/locale/<locale>/LC_MESSAGES)
	set(GETTEXT_MO_DEST_PATH ${LOCALEDIR}/<locale>/LC_MESSAGES)
	file(GLOB GETTEXT_AVAILABLE_LOCALES RELATIVE ${GETTEXT_PO_PATH} "${GETTEXT_PO_PATH}/*")
	list(REMOVE_ITEM GETTEXT_AVAILABLE_LOCALES minetest.pot)
	list(REMOVE_ITEM GETTEXT_AVAILABLE_LOCALES timestamp)
	macro(SET_MO_PATHS _buildvar _destvar _locale)
		string(REPLACE "<locale>" ${_locale} ${_buildvar} ${GETTEXT_MO_BUILD_PATH})
		string(REPLACE "<locale>" ${_locale} ${_destvar} ${GETTEXT_MO_DEST_PATH})
	endmacro()
endif()

