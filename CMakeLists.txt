cmake_minimum_required(VERSION 2.6)
if(${CMAKE_VERSION} STREQUAL "2.8.2")
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif(${CMAKE_VERSION} STREQUAL "2.8.2")

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

# This is done here so that IRRDIR is relative to the typical cmake call directory
find_package(Irrlicht)

# This way the CMakeLists.txt file in src/ is processed
add_subdirectory(src)

