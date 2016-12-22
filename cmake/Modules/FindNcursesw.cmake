#.rst:
# FindNcursesw
# ------------
#
# Find the ncursesw (wide ncurses) include file and library.
#
# Based on FindCurses.cmake which comes with CMake.
#
# Checks for ncursesw first. If not found, it then executes the
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
# ``CURSES_HAVE_CURSES_H``
#   True if curses.h is available.
# ``CURSES_HAVE_NCURSES_H``
#   True if ncurses.h is available.
# ``CURSES_HAVE_NCURSES_NCURSES_H``
#   True if ``ncurses/ncurses.h`` is available.
# ``CURSES_HAVE_NCURSES_CURSES_H``
#   True if ``ncurses/curses.h`` is available.
# ``CURSES_HAVE_NCURSESW_NCURSES_H``
#   True if ``ncursesw/ncurses.h`` is available.
# ``CURSES_HAVE_NCURSESW_CURSES_H``
#   True if ``ncursesw/curses.h`` is available.
#
# Set ``CURSES_NEED_NCURSES`` to ``TRUE`` before the
# ``find_package(Ncursesw)`` call if NCurses functionality is required.
#
#=============================================================================
# Copyright 2001-2014 Kitware, Inc.
# modifications: Copyright 2015 kahrl <kahrl@gmx.net>
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
#
# ------------------------------------------------------------------------------
#
# The above copyright and license notice applies to distributions of
# CMake in source and binary form.  Some source files contain additional
# notices of original copyright by their contributors; see each source
# for details.  Third-party software packages supplied with CMake under
# compatible licenses provide their own copyright notices documented in
# corresponding subdirectories.
#
# ------------------------------------------------------------------------------
#
# CMake was initially developed by Kitware with the following sponsorship:
#
#  * National Library of Medicine at the National Institutes of Health
#    as part of the Insight Segmentation and Registration Toolkit (ITK).
#
#  * US National Labs (Los Alamos, Livermore, Sandia) ASC Parallel
#    Visualization Initiative.
#
#  * National Alliance for Medical Image Computing (NAMIC) is funded by the
#    National Institutes of Health through the NIH Roadmap for Medical Research,
#    Grant U54 EB005149.
#
#  * Kitware, Inc.
#=============================================================================

include(CheckLibraryExists)

find_library(CURSES_NCURSESW_LIBRARY NAMES ncursesw
  DOC "Path to libncursesw.so or .lib or .a")

set(CURSES_USE_NCURSES FALSE)
set(CURSES_USE_NCURSESW FALSE)

if(CURSES_NCURSESW_LIBRARY)
  set(CURSES_USE_NCURSES TRUE)
  set(CURSES_USE_NCURSESW TRUE)
endif()

if(CURSES_USE_NCURSESW)
  get_filename_component(_cursesLibDir "${CURSES_NCURSESW_LIBRARY}" PATH)
  get_filename_component(_cursesParentDir "${_cursesLibDir}" PATH)

  find_path(CURSES_INCLUDE_PATH
    NAMES ncursesw/ncurses.h ncursesw/curses.h ncurses.h curses.h
    HINTS "${_cursesParentDir}/include"
    )

  # Previous versions of FindCurses provided these values.
  if(NOT DEFINED CURSES_LIBRARY)
    set(CURSES_LIBRARY "${CURSES_NCURSESW_LIBRARY}")
  endif()

  CHECK_LIBRARY_EXISTS("${CURSES_NCURSESW_LIBRARY}"
    cbreak "" CURSES_NCURSESW_HAS_CBREAK)
  if(NOT CURSES_NCURSESW_HAS_CBREAK)
    find_library(CURSES_EXTRA_LIBRARY tinfo HINTS "${_cursesLibDir}"
      DOC "Path to libtinfo.so or .lib or .a")
    find_library(CURSES_EXTRA_LIBRARY tinfo )
  endif()

  # Report whether each possible header name exists in the include directory.
  if(NOT DEFINED CURSES_HAVE_NCURSESW_NCURSES_H)
    if(EXISTS "${CURSES_INCLUDE_PATH}/ncursesw/ncurses.h")
      set(CURSES_HAVE_NCURSESW_NCURSES_H "${CURSES_INCLUDE_PATH}/ncursesw/ncurses.h")
    else()
      set(CURSES_HAVE_NCURSESW_NCURSES_H "CURSES_HAVE_NCURSESW_NCURSES_H-NOTFOUND")
    endif()
  endif()
  if(NOT DEFINED CURSES_HAVE_NCURSESW_CURSES_H)
    if(EXISTS "${CURSES_INCLUDE_PATH}/ncursesw/curses.h")
      set(CURSES_HAVE_NCURSESW_CURSES_H "${CURSES_INCLUDE_PATH}/ncursesw/curses.h")
    else()
      set(CURSES_HAVE_NCURSESW_CURSES_H "CURSES_HAVE_NCURSESW_CURSES_H-NOTFOUND")
    endif()
  endif()
  if(NOT DEFINED CURSES_HAVE_NCURSES_H)
    if(EXISTS "${CURSES_INCLUDE_PATH}/ncurses.h")
      set(CURSES_HAVE_NCURSES_H "${CURSES_INCLUDE_PATH}/ncurses.h")
    else()
      set(CURSES_HAVE_NCURSES_H "CURSES_HAVE_NCURSES_H-NOTFOUND")
    endif()
  endif()
  if(NOT DEFINED CURSES_HAVE_CURSES_H)
    if(EXISTS "${CURSES_INCLUDE_PATH}/curses.h")
      set(CURSES_HAVE_CURSES_H "${CURSES_INCLUDE_PATH}/curses.h")
    else()
      set(CURSES_HAVE_CURSES_H "CURSES_HAVE_CURSES_H-NOTFOUND")
    endif()
  endif()


  find_library(CURSES_FORM_LIBRARY form HINTS "${_cursesLibDir}"
    DOC "Path to libform.so or .lib or .a")
  find_library(CURSES_FORM_LIBRARY form )

  # Need to provide the *_LIBRARIES
  set(CURSES_LIBRARIES ${CURSES_LIBRARY})

  if(CURSES_EXTRA_LIBRARY)
    set(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_EXTRA_LIBRARY})
  endif()

  if(CURSES_FORM_LIBRARY)
    set(CURSES_LIBRARIES ${CURSES_LIBRARIES} ${CURSES_FORM_LIBRARY})
  endif()

  # Provide the *_INCLUDE_DIRS result.
  set(CURSES_INCLUDE_DIRS ${CURSES_INCLUDE_PATH})
  set(CURSES_INCLUDE_DIR ${CURSES_INCLUDE_PATH}) # compatibility

  # handle the QUIETLY and REQUIRED arguments and set CURSES_FOUND to TRUE if
  # all listed variables are TRUE
  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Ncursesw DEFAULT_MSG
    CURSES_LIBRARY CURSES_INCLUDE_PATH)
  set(CURSES_FOUND ${NCURSESW_FOUND})

else()
  find_package(Curses)
  set(NCURSESW_FOUND FALSE)
endif()

mark_as_advanced(
  CURSES_INCLUDE_PATH
  CURSES_CURSES_LIBRARY
  CURSES_NCURSES_LIBRARY
  CURSES_NCURSESW_LIBRARY
  CURSES_EXTRA_LIBRARY
  CURSES_FORM_LIBRARY
  )
