cmake_minimum_required(VERSION 2.6)
if(${CMAKE_VERSION} STREQUAL "2.8.2")
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif(${CMAKE_VERSION} STREQUAL "2.8.2")

# This can be read from ${PROJECT_NAME} after project() is called
project(minetest)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(${CMAKE_SOURCE_DIR}/cmake/Modules/misc.cmake)

# Default to Release
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

# Configuration

set(RUN_IN_PLACE 0 CACHE BOOL "Run directly in source directory structure")

set(BUILD_CLIENT 1 CACHE BOOL "Build client")
set(BUILD_SERVER 1 CACHE BOOL "Build server")

# Get date and time
GETDATETIME(BUILD_DATE)
MESSAGE(STATUS "BUILD_DATE = ${BUILD_DATE}")

# This is done here so that relative search paths are more reasnable
find_package(Irrlicht)

# This way the CMakeLists.txt file in src/ is processed
add_subdirectory(src)

install(FILES "doc/README.txt" DESTINATION "share/minetest/doc")

# CPack

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An InfiniMiner/Minecraft inspired game")
set(CPACK_PACKAGE_VERSION_MAJOR 0)
set(CPACK_PACKAGE_VERSION_MINOR 0)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPACK_PACKAGE_VENDOR "celeron55")
set(CPACK_PACKAGE_FILE_NAME "minetest-${BUILD_DATE}")

if(WIN32)
	# For some reason these aren't copied otherwise
	if(BUILD_CLIENT)
		install(FILES bin/minetest.exe DESTINATION bin)
	endif()
	if(BUILD_SERVER)
		install(FILES bin/minetestserver.exe DESTINATION bin)
	endif()

	set(CPACK_GENERATOR ZIP)

	# This might be needed for some installer
	#set(CPACK_PACKAGE_EXECUTABLES bin/minetest.exe "Minetest" bin/minetestserver.exe "Minetest Server")
elseif(APPLE)
	# TODO
	# see http://cmake.org/Wiki/CMake:CPackPackageGenerators#Bundle_.28OSX_only.29
	set(CPACK_PACKAGE_FILE_NAME ${PROJECT_NAME})
	set(CPACK_PACKAGE_ICON "")
	set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
	set(CPACK_BUNDLE_ICON "")
	set(CPACK_BUNDLE_PLIST "")
	set(CPACK_BUNDLE_STARTUP_COMMAND "Contents/MacOS/minetest")
	set(CPACK_GENERATOR BUNDLE)
else()
	set(CPACK_GENERATOR TGZ)
	set(CPACK_SOURCE_GENERATOR TGZ)
endif()

include(CPack)

