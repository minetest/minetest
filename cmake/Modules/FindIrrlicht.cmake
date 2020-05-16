# Look for IRRLICHTCPP if asked to.
# We use a bundled version by default because some distros ship versions of
# IRRLICHTCPP that cause segfaults and other memory errors when we link with them.
# See https://github.com/minetest/minetest/issues/1793

mark_as_advanced(IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)
option(ENABLE_SYSTEM_IRRLICHT "Enable using a system-wide Irrlicht.  May cause irrlicht non fixed bugs!" FALSE)

if(ENABLE_SYSTEM_IRRLICHT)
	find_library(IRRLICHT_LIBRARY NAMES irrlicht)
	find_path(IRRLICHT_INCLUDE_DIR irrlicht/IrrCompileConfig.h PATH_SUFFIXES irrlicht)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(Irrlicht DEFAULT_MSG IRRLICHT_LIBRARY IRRLICHT_INCLUDE_DIR)

	if(IRRLICHT_FOUND)
		message(STATUS "Using system Irrlicht library.")
	endif()
endif()

if(NOT IRRLICHT_FOUND)
	message(STATUS "Using bundled Irrlicht library.")
	set(IRRLICHT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib/irrlicht/include)
	set(IRRLICHT_LIBRARY irrlicht)
	add_subdirectory(lib/irrlicht)
endif()

message(STATUS "Irrlicht include dir: ${IRRLICHT_INCLUDE_DIR}")
