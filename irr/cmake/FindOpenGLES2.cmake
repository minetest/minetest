#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# Try to find OpenGL ES 2 and EGL

if(WIN32)
	find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h)
	find_library(OPENGLES2_LIBRARY libGLESv2)
elseif(APPLE)
	find_library(OPENGLES2_LIBRARY OpenGLES REQUIRED) # framework
else()
	# Unix
	find_path(OPENGLES2_INCLUDE_DIR GLES2/gl2.h
		PATHS /usr/X11R6/include /usr/include
	)
	find_library(OPENGLES2_LIBRARY
		NAMES GLESv2
		PATHS /usr/X11R6/lib /usr/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(OpenGLES2 DEFAULT_MSG OPENGLES2_LIBRARY OPENGLES2_INCLUDE_DIR)

	find_path(EGL_INCLUDE_DIR EGL/egl.h
		PATHS /usr/X11R6/include /usr/include
	)
	find_library(EGL_LIBRARY
		NAMES EGL
		PATHS /usr/X11R6/lib /usr/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(EGL REQUIRED_VARS EGL_LIBRARY EGL_INCLUDE_DIR NAME_MISMATCHED)
endif()

set(OPENGLES2_LIBRARIES ${OPENGLES2_LIBRARY} ${EGL_LIBRARY})
