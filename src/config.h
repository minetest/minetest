/*
	If CMake is used, includes the cmake-generated cmake_config.h.
	Otherwise use default values
*/

#ifndef CONFIG_H
#define CONFIG_H

#define PROJECT_NAME "Minetest"
#define RUN_IN_PLACE 0
#define USE_GETTEXT 0
#define USE_SOUND 0
#define USE_CURL 0
#define USE_FREETYPE 0
#define STATIC_SHAREDIR ""
#define USE_LEVELDB 0

#ifdef USE_CMAKE_CONFIG_H
	#include "cmake_config.h"
	#undef PROJECT_NAME
	#define PROJECT_NAME CMAKE_PROJECT_NAME
	#undef RUN_IN_PLACE
	#define RUN_IN_PLACE CMAKE_RUN_IN_PLACE
	#undef USE_GETTEXT
	#define USE_GETTEXT CMAKE_USE_GETTEXT
	#undef USE_SOUND
	#define USE_SOUND CMAKE_USE_SOUND
	#undef USE_CURL
	#define USE_CURL CMAKE_USE_CURL
	#undef USE_FREETYPE
	#define USE_FREETYPE CMAKE_USE_FREETYPE
	#undef STATIC_SHAREDIR
	#define STATIC_SHAREDIR CMAKE_STATIC_SHAREDIR
	#undef USE_LEVELDB
	#define USE_LEVELDB CMAKE_USE_LEVELDB
#endif

#endif

