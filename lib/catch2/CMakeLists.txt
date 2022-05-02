# Downloaded from:
#
# https://github.com/catchorg/Catch2/releases/download/v2.13.9/catch.hpp
#
# No library required, because it is just a header.

add_library(catch2 INTERFACE)
target_include_directories(catch2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
