
# Find script for libspatialindex
# libspatialindex has a target export in master branch since July 2020
# so this will eventually be obsolete
#
# Defines the following variables:
#
#       SPATIAL_LIBRARY
#       SPATIAL_INCLUDE_DIR
#
# And the following imported targets:
#
#       libspatialindex::spatialindex
#
# Author Josiah VanderZee

find_library(SPATIAL_LIBRARY spatialindex)
find_path(SPATIAL_INCLUDE_DIR spatialindex/SpatialIndex.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libspatialindex
    REQUIRED_VARS
        SPATIAL_LIBRARY SPATIAL_INCLUDE_DIR
)

if(NOT TARGET libspatialindex::libspatial)
    add_library(libspatialindex::libspatial INTERFACE IMPORTED)
    set_target_properties(libspatialindex::libspatial PROPERTIES
        INTERFACE_INCLUDE_DIRS
            ${SPATIAL_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES
            ${SPATIAL_LIBRARY}
    )
endif()