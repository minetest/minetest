
cmake_minimum_required(VERSION 2.4 FATAL_ERROR)

project(lua C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(COMMON_CFLAGS)
set(COMMON_LDFLAGS)
set(LIBS)

if(APPLE)
	set(DEFAULT_POSIX TRUE)
	set(DEFAULT_DLOPEN ON)
	# use this on Mac OS X 10.3-
	option(LUA_USE_MACOSX "Mac OS X 10.3-" OFF)
elseif(CYGWIN)
	set(DEFAULT_POSIX TRUE)
elseif(UNIX)
	set(DEFAULT_POSIX TRUE)
elseif(WIN32)
	set(LUA_WIN TRUE)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_BUILD_AS_DLL")
else()
	set(DEFAULT_ANSI TRUE)
endif()

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -Wl,-E -lm")
	set(DEFAULT_DLOPEN ON)
endif()

set(BUILD_STATIC ON)

if(DEFAULT_DLOPEN)
	option(LUA_USE_DLOPEN "Enable dlopen support." ON)
else()
	option(LUA_USE_DLOPEN "Enable dlopen support." OFF)
endif()

mark_as_advanced(LUA_USE_DLOPEN)

if(DEFAULT_ANSI)
	option(LUA_ANSI "Disable non-ansi features." ON)
else()
	option(LUA_ANSI "Disable non-ansi features." OFF)
endif()

mark_as_advanced(LUA_ANSI)


# For "Mac OS X 10.3-"
if(LUA_USE_MACOSX)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_MACOSX")
	set(LUA_USE_DLOPEN FALSE)
endif(LUA_USE_MACOSX)

if(LUA_USE_DLOPEN)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_DLOPEN")
	if(NOT APPLE)
		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -ldl ")
	endif(NOT APPLE)
endif(LUA_USE_DLOPEN)

if(LUA_ANSI)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_ANSI")
endif(LUA_ANSI)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_CFLAGS}")

if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -Wshadow -W -pedantic --std=c99")
	set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}     -O2")
	set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}       -O0 -g")
	set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_C_FLAGS_PROFILE}     -O1 -g")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_WITHDEBINFO} -O2 -g")
endif(CMAKE_COMPILER_IS_GNUCC)

add_subdirectory(src build)

