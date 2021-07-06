#-------------------------------------------------------------------
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find OpenGL ES 1
# Once done this will define
#
#  OpenGLES1_FOUND        - system has OpenGL ES
#  OpenGLES1_INCLUDE_DIR  - the GL include directory
#  OpenGLES1_LIBRARIES    - Link these to use OpenGL ES

# Win32 and Apple are not tested!
# Linux tested and works

if(WIN32)
	find_path(OpenGLES1_INCLUDE_DIR GLES/gl.h)
	find_library(OpenGLES1_LIBRARY libGLESv1_CM)
elseif(APPLE)
	create_search_paths(/Developer/Platforms)
	findpkg_framework(OpenGLES)
	set(OpenGLES1_LIBRARY "-framework OpenGLES")
else()
	# Unix
	find_path(OpenGLES1_INCLUDE_DIR GLES/gl.h
		PATHS /usr/openwin/share/include
			/opt/graphics/OpenGL/include
			/usr/X11R6/include
			/usr/include
	)

	find_library(OpenGLES1_LIBRARY
		NAMES GLESv1_CM
		PATHS /opt/graphics/OpenGL/lib
			/usr/openwin/lib
			/usr/X11R6/lib
			/usr/lib
	)

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(OpenGLES1 DEFAULT_MSG OpenGLES1_LIBRARY OpenGLES1_INCLUDE_DIR)
endif()

set(OpenGLES1_LIBRARIES ${OpenGLES1_LIBRARY})

mark_as_advanced(
	OpenGLES1_INCLUDE_DIR
	OpenGLES1_LIBRARY
)
