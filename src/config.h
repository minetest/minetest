/*
	If CMake is used, includes the cmake-generated cmake_config.h.
	Otherwise use default values
*/

#ifndef CONFIG_H
#define CONFIG_H

#ifdef USE_CMAKE_CONFIG_H
	#include "cmake_config.h"
#else
	#define PROJECT_NAME "minetest"

	//#define INSTALL_PREFIX ""
	#define VERSION_STRING "unknown"
	#ifdef NDEBUG
		#define BUILD_TYPE "Release"
	#else
		#define BUILD_TYPE "Debug"
	#endif
	#ifdef RUN_IN_PLACE
		#define RUN_IN_PLACE_BOOLSTRING "1"
	#else
		#define RUN_IN_PLACE_BOOLSTRING "0"
	#endif
	#if USE_GETTEXT
		#define USE_GETTEXT_BOOLSTRING "1"
	#else
		#define USE_GETTEXT_BOOLSTRING "0"
	#endif

	#define BUILD_INFO "NON-CMAKE RUN_IN_PLACE="RUN_IN_PLACE_BOOLSTRING" USE_GETTEXT="USE_GETTEXT_BOOLSTRING" BUILD_TYPE="BUILD_TYPE
#endif
#endif

