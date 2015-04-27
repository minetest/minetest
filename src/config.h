/*
	If CMake is used, includes the cmake-generated cmake_config.h.
	Otherwise use default values
*/

#ifndef CONFIG_H
#define CONFIG_H

#define STRINGIFY(x) #x
#define STR(x) STRINGIFY(x)


#if defined USE_CMAKE_CONFIG_H
	#include "cmake_config.h"
#elif defined (__ANDROID__) || defined (ANDROID)
	#define PROJECT_NAME "minetest"
	#define PROJECT_NAME_C "Minetest"
	#define STATIC_SHAREDIR ""
	#include "android_version.h"
	#ifdef NDEBUG
		#define BUILD_TYPE "Release"
	#else
		#define BUILD_TYPE "Debug"
	#endif
#else
	#ifdef NDEBUG
		#define BUILD_TYPE "Release"
	#else
		#define BUILD_TYPE "Debug"
	#endif
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
