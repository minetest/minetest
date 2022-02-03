# Script to find HiRedis
#
# Sets the following variables:
#
#     hiredis_FOUND
#     hiredis_LIBRARY
#     hiredis_INCLUDE_DIR
#
# And the following imported targets:
#
#     hiredis::hiredis
#
# Authors: Josiah VanderZee <josiah_vanderzee@mediacombb.net> 2022

find_library(hiredis_LIBRARY
	NAMES hiredis
)

find_path(hiredis_INCLUDE_DIR
	NAMES hiredis.h
	PATH_SUFFIXES hiredis
)

mark_as_advanced(
	hiredis_FOUND
	hiredis_LIBRARY
	hiredis_INCLUDE_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(hiredis
	REQUIRED_VARS hiredis_LIBRARY hiredis_INCLUDE_DIR
)

if(hiredis_FOUND AND NOT TARGET hiredis::hiredis)
	add_library(hiredis::hiredis INTERFACE IMPORTED)
	set_target_properties(hiredis::hiredis PROPERTIES
		INTERFACE_LINK_LIBRARIES "${hiredis_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${hiredis_INCLUDE_DIR}"
	)
endif()

