# Script to find THST
#
# Sets the following variables:
#
#     THST_FOUND
#     THST_LIBRARY
#
# And the following imported targets:
#
#     THST::THST
#
# Authors: Josiah VanderZee - josiah_vanderzee@mediacombb.net

find_path(THST_INCLUDE_DIR
	NAMES RTree.h
	PATHS "${CMAKE_SOURCE_DIR}/lib/THST/"
	NO_DEFAULT_PATH
)

mark_as_advanced(
	libspatialindex_FOUND
	libspatialindex_INCLUDE_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(THST
	REQUIRED_VARS THST_INCLUDE_DIR
)

if(THST_FOUND AND NOT TARGET THST::THST)
	add_library(THST::THST INTERFACE IMPORTED)
	set_target_properties(THST::THST PROPERTIES
		INTERFACE_INCLUDE_DIRECTORIES ${THST_INCLUDE_DIR}
	)
endif()

