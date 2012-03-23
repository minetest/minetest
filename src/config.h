/*
	If CMake is used, includes the cmake-generated cmake_config.h.
	Otherwise use default values
*/

#ifndef CONFIG_H
#define CONFIG_H

#define PROJECT_NAME "Minetest"
#define VERSION_STRING "unknown"
#define BUILD_TYPE "unknown"
#define USE_GETTEXT 0
#define USE_AUDIO 0
#define BUILD_INFO "non-cmake"

#ifdef USE_CMAKE_CONFIG_H
	#include "cmake_config.h"
	#undef PROJECT_NAME
	#define PROJECT_NAME CMAKE_PROJECT_NAME
	#undef VERSION_STRING
	#define VERSION_STRING CMAKE_VERSION_STRING
	#undef BUILD_INFO
	#define BUILD_INFO CMAKE_BUILD_INFO
	#undef USE_GETTEXT
	#define USE_GETTEXT CMAKE_USE_GETTEXT
	#undef USE_AUDIO
	#define USE_AUDIO CMAKE_USE_AUDIO
	#undef BUILD_INFO
	#define BUILD_INFO CMAKE_BUILD_INFO
#endif

#endif

