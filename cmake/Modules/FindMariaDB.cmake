mark_as_advanced(MARIADB_LIBRARY MARIADB_INCLUDE_DIR)

find_path(MARIADB_INCLUDE_DIR conncpp.hpp PATH_SUFFIXES mariadb)

find_library(MARIADB_LIBRARY NAMES mariadbcpp)

include(FindPackageHandleStandardArgs)

if(NOT TARGET mariadbcpp::mariadbcpp)
  add_library(mariadbcpp::mariadbcpp INTERFACE IMPORTED)
  set_target_properties(mariadbcpp::mariadbcpp PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${MARIADB_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${MARIADB_LIBRARY}"
  )
endif()

find_package_handle_standard_args(MariaDB DEFAULT_MSG MARIADB_LIBRARY MARIADB_INCLUDE_DIR)