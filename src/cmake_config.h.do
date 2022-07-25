#!/bin/sh
set -eu
cat <<EOF
// Filled in by the build system

#pragma once

#define PROJECT_NAME "minetest"
#define PROJECT_NAME_C "Minetest"
#define VERSION_MAJOR 5
#define VERSION_MINOR 6
#define VERSION_PATCH 0
#define VERSION_EXTRA ""
#define VERSION_STRING "5.6.0-dev"
#define PRODUCT_VERSION_STRING "5.6"
#define STATIC_SHAREDIR "."
#define STATIC_LOCALEDIR "locale"
#define BUILD_TYPE "Release"
#define ICON_DIR "unix/icons"
#define RUN_IN_PLACE 1
#define USE_GETTEXT 1
#define USE_CURL 1
#define USE_SOUND 1
#define USE_CURSES 1
#define USE_LEVELDB 1
#define USE_LUAJIT 1
#define USE_POSTGRESQL 0
#define USE_PROMETHEUS 0
#define USE_SPATIAL 0
#define USE_SYSTEM_GMP 1
#define USE_REDIS 0
#define ENABLE_GLES 0
#define HAVE_ENDIAN_H 1
#define CURSES_HAVE_CURSES_H 1
#define CURSES_HAVE_NCURSES_H 1
#define CURSES_HAVE_NCURSES_NCURSES_H 0
#define CURSES_HAVE_NCURSES_CURSES_H 0
#define CURSES_HAVE_NCURSESW_NCURSES_H 1
#define CURSES_HAVE_NCURSESW_CURSES_H 1
#define BUILD_UNITTESTS 1
#define BUILD_BENCHMARKS 0
EOF
