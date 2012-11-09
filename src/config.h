/*
	If CMake is used, includes the cmake-generated cmake_config.h.
	Otherwise use default values
*/

#ifndef CONFIG_H
#define CONFIG_H

#define PROJECT_NAME "Minetest"
#define VERSION_STRING "unknown"
#define RUN_IN_PLACE 0
#define USE_GETTEXT 0
#define USE_SOUND 0
#define STATIC_SHAREDIR ""
#define BUILD_INFO "non-cmake"

#ifdef USE_CMAKE_CONFIG_H
	#include "cmake_config.h"
	#undef PROJECT_NAME
	#define PROJECT_NAME CMAKE_PROJECT_NAME
	#undef VERSION_STRING
	#define VERSION_STRING CMAKE_VERSION_STRING
	#undef RUN_IN_PLACE
	#define RUN_IN_PLACE CMAKE_RUN_IN_PLACE
	#undef USE_GETTEXT
	#define USE_GETTEXT CMAKE_USE_GETTEXT
	#undef USE_SOUND
	#define USE_SOUND CMAKE_USE_SOUND
	#undef STATIC_SHAREDIR
	#define STATIC_SHAREDIR CMAKE_STATIC_SHAREDIR
	#undef BUILD_INFO
	#define BUILD_INFO CMAKE_BUILD_INFO
#endif

#endif

