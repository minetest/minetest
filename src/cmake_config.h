// Filled in by the build system

#ifndef CMAKE_CONFIG_H
#define CMAKE_CONFIG_H

#define CMAKE_PROJECT_NAME "minetest"
#define CMAKE_INSTALL_PREFIX "/usr/local"
#define CMAKE_VERSION_STRING "0.4.dev-20120408"
#ifdef NDEBUG
	#define CMAKE_BUILD_TYPE "Release"
#else
	#define CMAKE_BUILD_TYPE "Debug"
#endif
#define CMAKE_USE_GETTEXT 0
#define CMAKE_USE_SOUND 1
#define CMAKE_BUILD_INFO "VER=0.4.dev-20120408 BUILD_TYPE="CMAKE_BUILD_TYPE" RUN_IN_PLACE=1 USE_GETTEXT=0 USE_SOUND=1 INSTALL_PREFIX=/usr/local"

#endif

