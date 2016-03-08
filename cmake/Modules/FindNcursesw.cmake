# Find an ncurses include file and library.
#
# Based on FindCurses.cmake which comes with CMake.
#
# Checks for ncursesw first.  If not found, it then executes the
# regular old FindCurses.cmake to look for for ncurses (or curses).
#
#
# Result Variables
# ^^^^^^^^^^^^^^^^
#
# This module defines the following variables:
#
# ``CURSES_FOUND``
#   True if curses is found.
# ``NCURSESW_FOUND``
#   True if ncursesw is found.
# ``CURSES_INCLUDE_DIRS``
#   The include directories needed to use Curses.
# ``CURSES_LIBRARIES``
#   The libraries needed to use Curses.
# ``CURSES_HEADER``
#   Curses header file to use.
#
#=============================================================================
# Copyright 2001-2014 Kitware, Inc.
# Modifications copyright:
#	2015 kahrl <kahrl@gmx.net>
#	2016 ShadowNinja <shadowninja@minetest.net>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

find_library(CURSES_NCURSESW_LIBRARY NAMES ncursesw
	DOC "Path to libncursesw.so or .lib or .a")

if (CURSES_NCURSESW_LIBRARY)
	get_filename_component(_cursesLibDir "${CURSES_NCURSESW_LIBRARY}" PATH)
	get_filename_component(_cursesParentDir "${_cursesLibDir}" PATH)

	find_path(CURSES_INCLUDE_PATH
		NAMES ncursesw/ncurses.h ncursesw/curses.h
			ncurses/ncurses.h ncurses/curses.h
			ncurses.h curses.h
		HINTS "${_cursesParentDir}/include"
	)

	include(CheckLibraryExists)
	check_library_exists("${CURSES_NCURSESW_LIBRARY}"
		cbreak "" CURSES_NCURSESW_HAS_CBREAK)
	if (NOT CURSES_NCURSESW_HAS_CBREAK)
		find_library(CURSES_EXTRA_LIBRARY tinfo HINTS "${_cursesLibDir}"
			DOC "Path to libtinfo.so or .lib or .a")
	endif()

	# Find header to use
	if (EXISTS "${CURSES_INCLUDE_PATH}/ncursesw/ncurses.h")
		set(CURSES_HEADER "ncursesw/ncurses.h")
	elseif (EXISTS "${CURSES_INCLUDE_PATH}/ncursesw/curses.h")
		set(CURSES_HEADER "ncursesw/curses.h")
	elseif (EXISTS "${CURSES_INCLUDE_PATH}/ncurses/ncurses.h")
		set(CURSES_HEADER "ncurses/ncurses.h")
	elseif (EXISTS "${CURSES_INCLUDE_PATH}/ncurses/curses.h")
		set(CURSES_HEADER "ncurses/curses.h")
	elseif (EXISTS "${CURSES_INCLUDE_PATH}/ncurses.h")
		set(CURSES_HEADER "ncurses.h")
	elseif (EXISTS "${CURSES_INCLUDE_PATH}/curses.h")
		set(CURSES_HEADER "curses.h")
	endif()

	set(CURSES_INCLUDE_DIRS ${CURSES_INCLUDE_PATH})
	set(CURSES_LIBRARIES ${CURSES_NCURSESW_LIBRARY} ${CURSES_EXTRA_LIBRARY})

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(NcursesW DEFAULT_MSG
		CURSES_NCURSESW_LIBRARY CURSES_INCLUDE_PATH)
	set(CURSES_FOUND ${NCURSESW_FOUND})
else()
	set(NCURSESW_FOUND FALSE)
	find_package(Curses)
	if (CURSES_HAVE_NCURSES_NCURSES_H)
		set(CURSES_HEADER "ncurses/ncurses.h")
	elseif (CURSES_HAVE_NCURSES_CURSES_H)
		set(CURSES_HEADER "ncurses/curses.h")
	elseif (CURSES_HAVE_NCURSES_H)
		set(CURSES_HEADER "ncurses.h")
	elseif (CURSES_HAVE_CURSES_H)
		set(CURSES_HEADER "curses.h")
	endif()
endif()

mark_as_advanced(
	CURSES_INCLUDE_PATH
	CURSES_CURSES_LIBRARY
	CURSES_NCURSESW_LIBRARY
	CURSES_EXTRA_LIBRARY
)

