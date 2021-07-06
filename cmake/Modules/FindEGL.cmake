#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find EGL
# Once done this will define
#
#  EGL_FOUND        - system has EGL
#  EGL_INCLUDE_DIR  - the EGL include directory
#  EGL_LIBRARIES    - Link these to use EGL

# Win32 and Apple are not tested!
# Linux tested and works

if(NOT WIN32 AND NOT APPLE)
	# Unix
	find_path(EGL_INCLUDE_DIR EGL/egl.h
		PATHS /usr/openwin/share/include
			/opt/graphics/OpenGL/include
			/usr/X11R6/include
			/usr/include
	)

	find_library(EGL_LIBRARY
		NAMES EGL
		PATHS /opt/graphics/OpenGL/lib
			/usr/openwin/lib
			/usr/X11R6/lib
			/usr/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(EGL DEFAULT_MSG EGL_LIBRARY EGL_INCLUDE_DIR)
endif()

set(EGL_LIBRARIES ${EGL_LIBRARY})

mark_as_advanced(
	EGL_INCLUDE_DIR
	EGL_LIBRARY
)
