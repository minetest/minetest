cmake_minimum_required(VERSION 2.6)
if(${CMAKE_VERSION} STREQUAL "2.8.2")
  # bug http://vtk.org/Bug/view.php?id=11020
  message( WARNING "CMake/CPack version 2.8.2 will not create working .deb packages!")
endif(${CMAKE_VERSION} STREQUAL "2.8.2")

# This can be read from ${PROJECT_NAME} after project() is called
project(minetest)

set(VERSION_EXTRA "" CACHE STRING "Stuff to append to version string")

# Also remember to set PROTOCOL_VERSION in clientserver.h when releasing
set(VERSION_MAJOR 0)
set(VERSION_MINOR 4)
set(VERSION_PATCH 3)
if(VERSION_EXTRA)
	set(VERSION_PATCH ${VERSION_PATCH}-${VERSION_EXTRA})
endif()
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
	set(SHAREDIR ".")
	set(BINDIR "bin")
	set(DOCDIR "doc")
	set(EXAMPLE_CONF_DIR ".")
	set(LOCALEDIR "locale")
elseif(APPLE)
	# Random placeholders; this isn't usually used and may not work
	# See https://github.com/toabi/minetest-mac/
	set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
	set(BINDIR "bin")
	set(DOCDIR "share/doc/${PROJECT_NAME}")
	set(EXAMPLE_CONF_DIR ${DOCDIR})
	set(LOCALEDIR "locale")
elseif(UNIX) # Linux, BSD etc
	if(RUN_IN_PLACE)
		set(SHAREDIR ".")
		set(BINDIR "bin")
		set(DOCDIR "doc")
		set(EXAMPLE_CONF_DIR ".")
		set(MANDIR "unix/man")
		set(XDG_APPS_DIR "unix/applications")
		set(ICONDIR "unix/icons")
		set(LOCALEDIR "locale")
	else()
		set(SHAREDIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")
		set(BINDIR "${CMAKE_INSTALL_PREFIX}/bin")
		set(DOCDIR "${CMAKE_INSTALL_PREFIX}/share/doc/${PROJECT_NAME}")
		set(MANDIR "${CMAKE_INSTALL_PREFIX}/share/man")
		set(EXAMPLE_CONF_DIR ${DOCDIR})
		set(XDG_APPS_DIR "${CMAKE_INSTALL_PREFIX}/share/applications")
		set(ICONDIR "${CMAKE_INSTALL_PREFIX}/share/icons")
		set(LOCALEDIR "${CMAKE_INSTALL_PREFIX}/share/locale")
	endif()
endif()

set(CUSTOM_SHAREDIR "" CACHE STRING "Directory to install data files into")
if(NOT CUSTOM_SHAREDIR STREQUAL "")
	set(SHAREDIR "${CUSTOM_SHAREDIR}")
	message(STATUS "Using SHAREDIR=${SHAREDIR}")
endif()
set(CUSTOM_BINDIR "" CACHE STRING "Directory to install binaries into")
if(NOT CUSTOM_BINDIR STREQUAL "")
	set(BINDIR "${CUSTOM_BINDIR}")
	message(STATUS "Using BINDIR=${BINDIR}")
endif()
set(CUSTOM_DOCDIR "" CACHE STRING "Directory to install documentation into")
if(NOT CUSTOM_DOCDIR STREQUAL "")
	set(DOCDIR "${CUSTOM_DOCDIR}")
	message(STATUS "Using DOCDIR=${DOCDIR}")
endif()
set(CUSTOM_MANDIR "" CACHE STRING "Directory to install manpages into")
if(NOT CUSTOM_MANDIR STREQUAL "")
	set(MANDIR "${CUSTOM_MANDIR}")
	message(STATUS "Using MANDIR=${MANDIR}")
endif()
set(CUSTOM_EXAMPLE_CONF_DIR "" CACHE STRING "Directory to install example config file into")
if(NOT CUSTOM_EXAMPLE_CONF_DIR STREQUAL "")
	set(EXAMPLE_CONF_DIR "${CUSTOM_EXAMPLE_CONF_DIR}")
	message(STATUS "Using EXAMPLE_CONF_DIR=${EXAMPLE_CONF_DIR}")
endif()
set(CUSTOM_XDG_APPS_DIR "" CACHE STRING "Directory to install .desktop files into")
if(NOT CUSTOM_XDG_APPS_DIR STREQUAL "")
	set(XDG_APPS_DIR "${CUSTOM_XDG_APPS_DIR}")
	message(STATUS "Using XDG_APPS_DIR=${XDG_APPS_DIR}")
endif()
set(CUSTOM_ICONDIR "" CACHE STRING "Directory to install icons into")
if(NOT CUSTOM_ICONDIR STREQUAL "")
	set(ICONDIR "${CUSTOM_ICONDIR}")
	message(STATUS "Using ICONDIR=${ICONDIR}")
endif()
set(CUSTOM_LOCALEDIR "" CACHE STRING "Directory to install l10n files into")		
if(NOT CUSTOM_LOCALEDIR STREQUAL "")
	set(LOCALEDIR "${CUSTOM_LOCALEDIR}")
	message(STATUS "Using LOCALEDIR=${LOCALEDIR}")
endif()

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/builtin" DESTINATION "${SHAREDIR}")
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/games/minimal" DESTINATION "${SHAREDIR}/games")
set(MINETEST_GAME_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/games/minetest_game")
if(EXISTS ${MINETEST_GAME_SOURCE} AND IS_DIRECTORY ${MINETEST_GAME_SOURCE})
	install(FILES ${MINETEST_GAME_SOURCE}/game.conf DESTINATION "${SHAREDIR}/games/minetest_game/")
	install(FILES ${MINETEST_GAME_SOURCE}/README.txt DESTINATION "${SHAREDIR}/games/minetest_game/")
	install(DIRECTORY ${MINETEST_GAME_SOURCE}/mods DESTINATION "${SHAREDIR}/games/minetest_game")
endif()
if(BUILD_CLIENT)
	#install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/sounds/base/pack" DESTINATION "${SHAREDIR}/sounds/base")
	install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/textures/base/pack" DESTINATION "${SHAREDIR}/textures/base")
endif()
if(RUN_IN_PLACE)
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/mods/minetest/mods_here.txt" DESTINATION "${SHAREDIR}/mods/minetest")
	install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/textures/all/textures_here.txt" DESTINATION "${SHAREDIR}/textures/all")
endif()

install(FILES "README.txt" DESTINATION "${DOCDIR}")
install(FILES "doc/lua_api.txt" DESTINATION "${DOCDIR}")
install(FILES "doc/mapformat.txt" DESTINATION "${DOCDIR}")
install(FILES "minetest.conf.example" DESTINATION "${EXAMPLE_CONF_DIR}")

if(UNIX)
	install(FILES "doc/minetest.6" "doc/minetestserver.6" DESTINATION "${MANDIR}/man6")
	install(FILES "misc/minetest.desktop" DESTINATION "${XDG_APPS_DIR}")
	install(FILES "misc/minetest-icon.svg" DESTINATION "${ICONDIR}/hicolor/scalable/apps")
endif()

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

