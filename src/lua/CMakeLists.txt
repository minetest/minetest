#
# Lua 5.1.x
#
cmake_minimum_required(VERSION 2.4 FATAL_ERROR)

project(lua C)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(CustomMacros)

#enable_testing()

set(COMMON_CFLAGS)
set(COMMON_LDFLAGS)
set(LIBS)
#
# Detect system type
#
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
	set(DEFAULT_POSIX TRUE)
	set(DEFAULT_DLOPEN ON)
	set(DEFAULT_READLINE ON)
	set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -Wl,-E")
else(CMAKE_SYSTEM_NAME STREQUAL "Linux")
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
	else(APPLE)
		set(DEFAULT_ANSI TRUE)
	endif(APPLE)
endif(CMAKE_SYSTEM_NAME STREQUAL "Linux")

#
# setup config options
#
if(WIN32)
	set(BUILD_STATIC OFF)
else(WIN32)
	option(BUILD_STATIC "build static library" ON)
endif(WIN32)

if(DEFAULT_DLOPEN)
	option(LUA_USE_DLOPEN "Enable dlopen support." ON)
else(DEFAULT_DLOPEN)
	option(LUA_USE_DLOPEN "Enable dlopen support." OFF)
endif(DEFAULT_DLOPEN)

if(DEFAULT_POSIX)
	option(LUA_USE_CURSES "Enable Curses support." ON)
	option(LUA_USE_MKSTEMP "Use mkstemp." ON)
	option(LUA_USE_ISATTY "Enable isatty support." ON)
	option(LUA_USE_POPEN "Enable lua_popen support." ON)
	option(LUA_USE_ULONGJMP "Try using _longjmp/_setjmp (more efficient)" ON)
else(DEFAULT_POSIX)
	option(LUA_USE_CURSES "Enable Curses support." OFF)
	option(LUA_USE_MKSTEMP "Use mkstemp." OFF)
	option(LUA_USE_ISATTY "Enable isatty support." OFF)
	option(LUA_USE_POPEN "Enable lua_popen support." OFF)
	option(LUA_USE_ULONGJMP "Try using _longjmp/_setjmp (more efficient)" OFF)
endif(DEFAULT_POSIX)

if(DEFAULT_READLINE)
	option(LUA_USE_READLINE "Enable readline support." ON)
else(DEFAULT_READLINE)
	option(LUA_USE_READLINE "Enable readline support." OFF)
endif(DEFAULT_READLINE)

if(DEFAULT_ANSI)
	option(LUA_ANSI "Disable non-ansi features." ON)
else(DEFAULT_ANSI)
	option(LUA_ANSI "Disable non-ansi features." OFF)
endif(DEFAULT_ANSI)

option(LUA_USE_APICHECK "Enable API checks." OFF)

#
# Lua version
#
set(LUA_VERSION_MAJOR 5)
set(LUA_VERSION_MINOR 1)
set(LUA_VERSION_PATCH 4)
set(LUA_VERSION
				"${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}.${LUA_VERSION_PATCH}")
set(LUA_SOVERSION
				"${LUA_VERSION_MAJOR}")
#
# Lua package info.
#
set(CPACK_PACKAGE_VERSION_MAJOR ${LUA_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${LUA_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${LUA_VERSION_PATCH})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYRIGHT")
#set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README")
set(CPACK_PACKAGE_VENDOR "Lua.org, PUC-Rio.")
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
"/\\\\.;/\\\\.git.*/;~$;build/;CMakeFiles/;CMakeCache;Testing/;cmake_install;CPack;Dart;Makefile$")
set(CPACK_SOURCE_PACKAGE_FILE_NAME
	"${CMAKE_PROJECT_NAME}-${LUA_VERSION_MAJOR}.${LUA_VERSION_MINOR}.${LUA_VERSION_PATCH}")
# MUST be after CPACK_* variables.
include(CPack)

#
# libs & cflags
#
set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -lm ")

# For "Mac OS X 10.3-"
if(LUA_USE_MACOSX)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_MACOSX")
	set(LUA_USE_DLOPEN FALSE)
endif(LUA_USE_MACOSX)

# enable options
if(LUA_USE_DLOPEN)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_DLOPEN")
	if(NOT APPLE)
		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -ldl ")
	endif(NOT APPLE)
endif(LUA_USE_DLOPEN)
if(LUA_USE_MKSTEMP)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_MKSTEMP")
endif(LUA_USE_MKSTEMP)
if(LUA_USE_ISATTY)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_ISATTY")
endif(LUA_USE_ISATTY)
if(LUA_USE_POPEN)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_POPEN")
endif(LUA_USE_POPEN)
if(LUA_USE_ULONGJMP)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_ULONGJMP")
endif(LUA_USE_ULONGJMP)
if(LUA_USE_APICHECK)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_APICHECK")
endif(LUA_USE_APICHECK)
if(LUA_ANSI)
	set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_ANSI")
endif(LUA_ANSI)

# readline support
if(LUA_USE_READLINE)
	FIND_PATH(READLINE_INCLUDE_DIR readline/readline.h)
	FIND_LIBRARY(READLINE_LIBRARY NAMES readline)
	if(READLINE_LIBRARY)
		set(COMMON_CFLAGS "${COMMON_CFLAGS} -DLUA_USE_READLINE")
		set(COMMON_LDFLAGS "${COMMON_LDFLAGS} -lreadline -lhistory")
		include_directories(${READLINE_INCLUDE_DIR})
	endif(READLINE_LIBRARY)
endif(LUA_USE_READLINE)

# curses
if(LUA_USE_CURSES)
	include(FindCurses)
	if(CURSES_LIBRARY)
		include_directories(${CURSES_INCLUDE_DIR})
		set(LIBS ${LIBS} ${CURSES_LIBRARY})
	endif(CURSES_LIBRARY)
endif(LUA_USE_CURSES)

#
# standard flags to use for each build type.
#
if(CMAKE_COMPILER_IS_GNUCC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -Wall -Wextra -Wshadow -W -pedantic -std=gnu99")
	set(CMAKE_C_FLAGS_RELEASE        "${CMAKE_C_FLAGS_RELEASE}     -O2")
	set(CMAKE_C_FLAGS_DEBUG          "${CMAKE_C_FLAGS_DEBUG}       -O0 -g")
	set(CMAKE_C_FLAGS_PROFILE        "${CMAKE_C_FLAGS_PROFILE}     -O1 -g")
	set(CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_WITHDEBINFO} -O2 -g")
endif(CMAKE_COMPILER_IS_GNUCC)

#
# Install extra header files & manpages
#
#install(FILES
#	etc/lua.hpp
#	DESTINATION include)
#install(FILES
#	doc/lua.1
#	doc/luac.1
#	DESTINATION man/man1)

#
# sub-folders
#
ADD_SUBDIRECTORY(src build)
#ADD_SUBDIRECTORY(test)

