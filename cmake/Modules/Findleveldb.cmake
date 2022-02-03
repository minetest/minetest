# Script to find LevelDB
#
# Sets the following variables:
#
#     leveldb_FOUND
#     leveldb_LIBRARY
#     leveldb_INCLUDE_DIR
#
# And the following imported targets:
#
#     leveldb::leveldb
#
# Authors: Josiah VanderZee <josiah_vanderzee@mediacombb.net> 2022

find_library(leveldb_LIBRARY
	NAMES leveldb
)

find_path(leveldb_INCLUDE_DIR
	NAMES db.h
	PATH_SUFFIXES leveldb
)

mark_as_advanced(
	leveldb_FOUND
	leveldb_LIBRARY
	leveldb_INCLUDE_DIR
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(leveldb
	REQUIRED_VARS leveldb_LIBRARY leveldb_INCLUDE_DIR
)

if(leveldb_FOUND AND NOT TARGET leveldb::leveldb)
	add_library(leveldb::leveldb INTERFACE IMPORTED)
	set_target_properties(leveldb::leveldb PROPERTIES
		INTERFACE_LINK_LIBRARIES "${leveldb_LIBRARY}"
		INTERFACE_INCLUDE_DIRECTORIES "${leveldb_INCLUDE_DIR}"
	)
endif()

