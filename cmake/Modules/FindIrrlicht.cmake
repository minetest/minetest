#FindIrrlicht.cmake

set(IRRLICHT_SOURCE_DIR "" CACHE PATH "Path to irrlicht source directory (optional)")

if( UNIX )
	# Unix
else( UNIX )
	# Windows
endif( UNIX )

# Find include directory

if(NOT IRRLICHT_SOURCE_DIR STREQUAL "")
	set(IRRLICHT_SOURCE_DIR_INCLUDE
		"${IRRLICHT_SOURCE_DIR}/include"
	)

	set(IRRLICHT_LIBRARY_NAMES libIrrlicht.a Irrlicht Irrlicht.lib)

	if(WIN32)
		if(MSVC)
			set(IRRLICHT_SOURCE_DIR_LIBS "${IRRLICHT_SOURCE_DIR}/lib/Win32-visualstudio")
			set(IRRLICHT_LIBRARY_NAMES Irrlicht.lib)
		else()
			set(IRRLICHT_SOURCE_DIR_LIBS "${IRRLICHT_SOURCE_DIR}/lib/Win32-gcc")
			set(IRRLICHT_LIBRARY_NAMES libIrrlicht.a libIrrlicht.dll.a)
		endif()
	else()
		set(IRRLICHT_SOURCE_DIR_LIBS "${IRRLICHT_SOURCE_DIR}/lib/Linux")
		set(IRRLICHT_LIBRARY_NAMES libIrrlicht.a)
	endif()

	FIND_PATH(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		PATHS
		${IRRLICHT_SOURCE_DIR_INCLUDE}
		NO_DEFAULT_PATH
	)

	FIND_LIBRARY(IRRLICHT_LIBRARY NAMES ${IRRLICHT_LIBRARY_NAMES}
		PATHS
		${IRRLICHT_SOURCE_DIR_LIBS}
		NO_DEFAULT_PATH
	)

else()

	FIND_PATH(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		PATHS
		/usr/local/include/irrlicht
		/usr/include/irrlicht
	)

	FIND_LIBRARY(IRRLICHT_LIBRARY NAMES libIrrlicht.a Irrlicht
		PATHS
		/usr/local/lib
		/usr/lib
	)
endif()

MESSAGE(STATUS "IRRLICHT_SOURCE_DIR = ${IRRLICHT_SOURCE_DIR}")
MESSAGE(STATUS "IRRLICHT_INCLUDE_DIR = ${IRRLICHT_INCLUDE_DIR}")
MESSAGE(STATUS "IRRLICHT_LIBRARY = ${IRRLICHT_LIBRARY}")

# On windows, find the dll for installation
if(WIN32)
	if(MSVC)
		FIND_FILE(IRRLICHT_DLL NAMES Irrlicht.dll
			PATHS
			"${IRRLICHT_SOURCE_DIR}/bin/Win32-VisualStudio"
			DOC "Path of the Irrlicht dll (for installation)"
		)
	else()
		FIND_FILE(IRRLICHT_DLL NAMES Irrlicht.dll
			PATHS
			"${IRRLICHT_SOURCE_DIR}/bin/Win32-gcc"
			DOC "Path of the Irrlicht dll (for installation)"
		)
	endif()
	MESSAGE(STATUS "IRRLICHT_DLL = ${IRRLICHT_DLL}")
endif(WIN32)

# handle the QUIETLY and REQUIRED arguments and set IRRLICHT_FOUND to TRUE if
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(IRRLICHT DEFAULT_MSG IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

IF(IRRLICHT_FOUND)
  SET(IRRLICHT_LIBRARIES ${IRRLICHT_LIBRARY})
ELSE(IRRLICHT_FOUND)
  SET(IRRLICHT_LIBRARIES)
ENDIF(IRRLICHT_FOUND)

MARK_AS_ADVANCED(IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR IRRLICHT_DLL) 

