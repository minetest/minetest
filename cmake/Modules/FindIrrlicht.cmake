
mark_as_advanced(IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR IRRLICHT_DLL)
set(IRRLICHT_SOURCE_DIR "" CACHE PATH "Path to irrlicht source directory (optional)")


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

	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		PATHS
		${IRRLICHT_SOURCE_DIR_INCLUDE}
		NO_DEFAULT_PATH
	)

	find_library(IRRLICHT_LIBRARY NAMES ${IRRLICHT_LIBRARY_NAMES}
		PATHS
		${IRRLICHT_SOURCE_DIR_LIBS}
		NO_DEFAULT_PATH
	)

else()
	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		PATHS
		/usr/local/include/irrlicht
		/usr/include/irrlicht
		/system/develop/headers/irrlicht #Haiku
		PATH_SUFFIXES "include/irrlicht"
	)

	find_library(IRRLICHT_LIBRARY NAMES libIrrlicht.so libIrrlicht.a Irrlicht
		PATHS
		/usr/local/lib
		/usr/lib
		/system/develop/lib # Haiku
	)
endif()


# On Windows, find the DLL for installation
if(WIN32)
	# If VCPKG_APPLOCAL_DEPS is ON, dll's are automatically handled by VCPKG
	if(NOT VCPKG_APPLOCAL_DEPS)
		if(MSVC)
			set(IRRLICHT_COMPILER "VisualStudio")
		else()
			set(IRRLICHT_COMPILER "gcc")
		endif()
		find_file(IRRLICHT_DLL NAMES Irrlicht.dll
			PATHS
			"${IRRLICHT_SOURCE_DIR}/bin/Win32-${IRRLICHT_COMPILER}"
			DOC "Path of the Irrlicht dll (for installation)"
		)
	endif()
endif(WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Irrlicht DEFAULT_MSG IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

