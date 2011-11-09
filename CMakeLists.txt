cmake_minimum_required(VERSION 2.6)
if(${CMAKE_VERSION} STREQUAL "2.8.2")
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif(${CMAKE_VERSION} STREQUAL "2.8.2")

# This can be read from ${PROJECT_NAME} after project() is called
project(minetest)

# Also remember to set PROTOCOL_VERSION in clientserver.h when releasing
set(VERSION_MAJOR 0)
set(VERSION_MINOR 3)
set(VERSION_PATCH 1)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

MESSAGE(STATUS "*** Will build version ${VERSION_STRING} ***")

# Configuration options

if(WIN32)
	set(RUN_IN_PLACE 1 CACHE BOOL "Run directly in source directory structure")
else()
	set(RUN_IN_PLACE 0 CACHE BOOL "Run directly in source directory structure")
endif()

set(BUILD_CLIENT 1 CACHE BOOL "Build client")
if(WIN32)
	set(BUILD_SERVER 0 CACHE BOOL "Build server")
else()
	set(BUILD_SERVER 1 CACHE BOOL "Build server")
endif()

set(WARN_ALL 1 CACHE BOOL "Enable -Wall for Release build")

if(NOT CMAKE_BUILD_TYPE)
	# Default to release
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type: Debug or Release" FORCE)
endif()

# Included stuff
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(${CMAKE_SOURCE_DIR}/cmake/Modules/misc.cmake)

# This is done here so that relative search paths are more reasnable
find_package(Irrlicht)

#
# Installation
#

if(WIN32)
	set(DATADIR "data")
	set(BINDIR "bin")
	set(DOCDIR "doc")
	set(EXAMPLE_CONF_DIR ".")
elseif(APPLE)
	# random placeholders
	set(DATADIR "share/${PROJECT_NAME}")
	set(BINDIR "bin")
	set(DOCDIR "share/doc/${PROJECT_NAME}")
	set(EXAMPLE_CONF_DIR ".")
elseif(UNIX) # Linux, BSD etc
	set(DATADIR "share/${PROJECT_NAME}")
	set(BINDIR "bin")
	set(DOCDIR "share/doc/${PROJECT_NAME}")
	set(EXAMPLE_CONF_DIR "share/doc/${PROJECT_NAME}")
endif()

install(FILES "README.txt" DESTINATION "${DOCDIR}")
install(FILES "doc/changelog.txt" DESTINATION "${DOCDIR}")
install(FILES "minetest.conf.example" DESTINATION "${DOCDIR}")

#
# Subdirectories
# Be sure to add all relevant definitions above this
#

add_subdirectory(src)

# CPack

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "An InfiniMiner/Minecraft inspired game")
set(CPACK_PACKAGE_VERSION_MAJOR ${VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${VERSION_PATCH})
set(CPACK_PACKAGE_VENDOR "celeron55")
set(CPACK_PACKAGE_CONTACT "Perttu Ahola <celeron55@gmail.com>")

if(WIN32)
	# For some reason these aren't copied otherwise
	# NOTE: For some reason now it seems to work without these
	#if(BUILD_CLIENT)
	#	install(FILES bin/minetest.exe DESTINATION bin)
	#endif()
	#if(BUILD_SERVER)
	#	install(FILES bin/minetestserver.exe DESTINATION bin)
	#endif()

	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-win32")

	set(CPACK_GENERATOR ZIP)

	# This might be needed for some installer
	#set(CPACK_PACKAGE_EXECUTABLES bin/minetest.exe "Minetest" bin/minetestserver.exe "Minetest Server")
elseif(APPLE)
	# TODO
	# see http://cmake.org/Wiki/CMake:CPackPackageGenerators#Bundle_.28OSX_only.29
	#
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-osx")
	set(CPACK_PACKAGE_ICON "")
	set(CPACK_BUNDLE_NAME ${PROJECT_NAME})
	set(CPACK_BUNDLE_ICON "")
	set(CPACK_BUNDLE_PLIST "")
	set(CPACK_BUNDLE_STARTUP_COMMAND "Contents/MacOS/${PROJECT_NAME}")
	set(CPACK_GENERATOR "Bundle")
else()
	set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${VERSION_STRING}-linux")
	set(CPACK_GENERATOR TGZ)
	set(CPACK_SOURCE_GENERATOR TGZ)
endif()

include(CPack)

