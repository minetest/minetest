
mark_as_advanced(IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR IRRLICHT_DLL)

# Find include directory and libraries

if(TRUE)
	find_path(IRRLICHT_INCLUDE_DIR NAMES irrlicht.h
		DOC "Path to the directory with Irrlicht includes"
		PATHS
		/usr/local/include/irrlicht
		/usr/include/irrlicht
		/system/develop/headers/irrlicht #Haiku
		PATH_SUFFIXES "include/irrlicht"
	)

	find_library(IRRLICHT_LIBRARY NAMES libIrrlicht Irrlicht
		DOC "Path to the Irrlicht library file"
		PATHS
		/usr/local/lib
		/usr/lib
		/system/develop/lib # Haiku
	)
endif()

# Users will likely need to edit these
mark_as_advanced(CLEAR IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

# On Windows, find the DLL for installation
if(WIN32)
	# If VCPKG_APPLOCAL_DEPS is ON, dll's are automatically handled by VCPKG
	if(NOT VCPKG_APPLOCAL_DEPS)
		find_file(IRRLICHT_DLL NAMES Irrlicht.dll
			DOC "Path of the Irrlicht dll (for installation)"
		)
	endif()
endif(WIN32)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Irrlicht DEFAULT_MSG IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

