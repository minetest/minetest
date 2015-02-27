/*
	If CMake is used, includes the cmake-generated cmake_config.h.
	Otherwise use default values
*/

#ifndef CONFIG_H
#define CONFIG_H

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)


#ifdef USE_CMAKE_CONFIG_H
	#include "cmake_config.h"
#else
	#define PROJECT_NAME "Minetest"
	#define RUN_IN_PLACE 0
	#define USE_CURL 0
	#define USE_FREETYPE 0
	#define USE_GETTEXT 0
	#define USE_LEVELDB 0
	#define USE_LUAJIT 0
	#define USE_REDIS 0
	#define USE_SOUND 0
	#define HAVE_ENDIAN_H 0
	#define STATIC_SHAREDIR ""
	#ifdef NDEBUG
		#define BUILD_TYPE "Release"
	#else
		#define BUILD_TYPE "Debug"
	#endif
#endif

#ifdef __ANDROID__
	#include "android_version.h"
#endif

#define BUILD_INFO "BUILD_TYPE=" BUILD_TYPE \
		" RUN_IN_PLACE=" STR(RUN_IN_PLACE) \
		" USE_GETTEXT=" STR(USE_GETTEXT) \
		" USE_SOUND=" STR(USE_SOUND) \
		" USE_CURL=" STR(USE_CURL) \
		" USE_FREETYPE=" STR(USE_FREETYPE) \
		" USE_LUAJIT=" STR(USE_LUAJIT) \
		" STATIC_SHAREDIR=" STR(STATIC_SHAREDIR)

#endif

