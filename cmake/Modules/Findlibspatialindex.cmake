# Script to find libspatialindex if installed
# libspatialindex has a target export in master branch since July 2020
# so this will eventually be obsolete
#
# Sets the following variables:
#
#     libspatialindex_FOUND
#     libspatialindex_LIBRARY
#     libspatialindex_INCLUDE_DIR
#
# And the following imported targets:
#
#     libspatialindex::spatialindex
#
# Authors: Josiah VanderZee - josiah_vanderzee@mediacombb.net

find_library(libspatialindex_LIBRARY
	NAMES spatialindex
)
message(STATUS "Lib: ${libspatialindex_LIBRARY}")
find_path(libspatialindex_INCLUDE_DIR
	NAMES SpatialIndex.h
	PATH_SUFFIXES spatialindex
)

mark_as_advanced(
	libspatialindex_FOUND
	libspatialindex_LIBRARY
	libspatialindex_INCLUDE_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libspatialindex
	REQUIRED_VARS libspatialindex_LIBRARY libspatialindex_INCLUDE_DIR
)

if(libspatialindex_FOUND AND NOT TARGET libspatialindex::libspatial)
	add_library(libspatialindex::libspatial INTERFACE IMPORTED)
	set_target_properties(libspatialindex::libspatial PROPERTIES
		INTERFACE_LINK_LIBRARIES "${libspatialindex_LIBRARY}"
		INTERFACE_INCLUDE_DIRS "${libspatialindex_INCLUDE_DIR}"
    )
endif()
