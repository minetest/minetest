#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OpenGLES and EGL
# Once done this will define
#
#  OPENGLES2_FOUND        - system has OpenGLES
#  OPENGLES2_INCLUDE_DIR  - the GL include directory
#  OPENGLES2_LIBRARIES    - Link these to use OpenGLES
#
#  EGL_FOUND        - system has EGL
#  EGL_INCLUDE_DIR  - the EGL include directory
#  EGL_LIBRARIES    - Link these to use EGL

# Win32 and Apple are not tested!
# Linux tested and works

if(WIN32)
	find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h)
	find_library(OPENGLES2_LIBRARY libGLESv2)
elseif(APPLE)
	create_search_paths(/Developer/Platforms)
	findpkg_framework(OpenGLES2)
	set(OPENGLES2_LIBRARY "-framework OpenGLES")
else()
	# Unix
	find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h
		PATHS /usr/openwin/share/include
			/opt/graphics/OpenGL/include
			/usr/X11R6/include
			/usr/include
	)

	find_library(OPENGLES2_LIBRARY
		NAMES GLESv2
		PATHS /opt/graphics/OpenGL/lib
			/usr/openwin/lib
			/usr/X11R6/lib
			/usr/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(OPENGLES2 DEFAULT_MSG OPENGLES2_LIBRARY OPENGLES2_INCLUDE_DIR)

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

set(OPENGLES2_LIBRARIES ${OPENGLES2_LIBRARY})
set(EGL_LIBRARIES ${EGL_LIBRARY})

mark_as_advanced(
	OPENGLES2_INCLUDE_DIR
	OPENGLES2_LIBRARY
	EGL_INCLUDE_DIR
	EGL_LIBRARY
)
