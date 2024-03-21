if(NOT ANDROID AND NOT APPLE)
	set(DEFAULT_SDL2 ON)
endif()

option(BUILD_SHARED_LIBS "Build shared library" TRUE)
option(USE_SDL2 "Use the SDL2 backend" ${DEFAULT_SDL2})

# Compiler flags

add_definitions(-DIRRLICHT_EXPORTS)
if(BUILD_SHARED_LIBS)
	if(WIN32)
		set(API_IMPORT "__declspec(dllimport)")
		set(API_EXPORT "__declspec(dllexport)")
	elseif(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
		set(API_EXPORT "__attribute__ ((visibility(\"default\")))") # only necessary if default visibility is set to hidden
	endif()
else()
	add_definitions(-D_IRR_STATIC_LIB_)
endif()
add_definitions("-DIRRLICHT_API=${API_EXPORT}")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	add_definitions(-D_DEBUG)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)

if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
	set(CMAKE_CXX_FLAGS_RELEASE "-O3")
	set(CMAKE_CXX_FLAGS_DEBUG "-g")

	add_compile_options(-Wall -pipe -fno-exceptions)

	# Enable SSE for floating point math on 32-bit x86 by default
	# reasoning see minetest issue #11810 and https://gcc.gnu.org/wiki/FloatingPointMath
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		include(CheckCXXSourceCompiles)
		check_cxx_source_compiles("#ifndef __i686__\n#error\n#endif\nint main(){}" IS_I686)
		if(IS_I686)
			message(STATUS "Detected Intel x86: using SSE instead of x87 FPU")
			add_compile_options(-mfpmath=sse -msse)
		endif()
	endif()

elseif(MSVC)
	string(APPEND CMAKE_CXX_STANDARD_LIBRARIES " msvcrt.lib") # ???? fuck off

	add_compile_options(/GR- /Zl)

	# Enable SSE for floating point math on 32-bit x86 by default
	# reasoning see minetest issue #11810 and https://gcc.gnu.org/wiki/FloatingPointMath
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_compile_options(/arch:SSE)
	endif()

	add_compile_options(/D_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
endif()

# Sanity-check version

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${PROJECT_SOURCE_DIR}/include)
unset(REVISION_SANITY_CHECK CACHE)
check_cxx_source_compiles("#include <IrrCompileConfig.h>\n\
#if IRRLICHT_VERSION_MT_REVISION != ${IRRLICHTMT_REVISION}\n\
#error\n\
#endif\n\
int main() {}" REVISION_SANITY_CHECK)
if(NOT REVISION_SANITY_CHECK)
	message(FATAL_ERROR "IrrlichtMt revision number mismatches between CMake and headers.")
endif()

# Platform-independent configuration (hard-coded currently)
add_definitions(
	-DIRR_ENABLE_BUILTIN_FONT
)

# Platform-specific configuration

if(CMAKE_SYSTEM_NAME STREQUAL "SunOS")
	set(SOLARIS TRUE)
endif()

# Device

if(WIN32)
	add_definitions(-D_IRR_WINDOWS_ -D_IRR_WINDOWS_API_)
	set(DEVICE "WINDOWS")
elseif(APPLE)
	add_definitions(-D_IRR_OSX_PLATFORM_)
	set(DEVICE "OSX")
elseif(ANDROID)
	add_definitions(-D_IRR_ANDROID_PLATFORM_)
	if(USE_SDL2)
		message(FATAL_ERROR "SDL2 device is not (yet) supported on Android")
	endif()
	set(DEVICE "ANDROID")
elseif(EMSCRIPTEN)
	add_definitions(-D_IRR_EMSCRIPTEN_PLATFORM_ -D_IRR_COMPILE_WITH_EGL_MANAGER_)
	set(LINUX_PLATFORM TRUE)
	set(DEVICE "SDL")
elseif(SOLARIS)
	add_definitions(-D_IRR_SOLARIS_PLATFORM_ -D_IRR_POSIX_API_)
	set(DEVICE "X11")
else()
	add_definitions(-D_IRR_POSIX_API_)
	set(LINUX_PLATFORM TRUE)
	set(DEVICE "X11")
endif()

if(LINUX_PLATFORM)
	add_definitions(-D_IRR_LINUX_PLATFORM_)
endif()

if(USE_SDL2)
	set(DEVICE "SDL")
elseif(DEVICE STREQUAL "SDL")
	message(FATAL_ERROR "SDL was used but not enabled?!")
endif()

add_definitions("-D_IRR_COMPILE_WITH_${DEVICE}_DEVICE_")

# X11

if(DEVICE STREQUAL "X11")
	option(USE_X11 "Use X11" TRUE)
else()
	set(USE_X11 FALSE)
endif()

if(LINUX_PLATFORM AND USE_X11)
	option(USE_XINPUT2 "Use XInput2" TRUE)
	option(USE_XCURSOR "Use XCursor" FALSE)
else()
	set(USE_XINPUT2 FALSE)
	set(USE_XCURSOR FALSE)
endif()

# Joystick

if(NOT (BSD OR SOLARIS OR EMSCRIPTEN))
	add_definitions(-D_IRR_COMPILE_WITH_JOYSTICK_EVENTS_)
endif()

# OpenGL

if(USE_SDL2)
	option(ENABLE_OPENGL3 "Enable OpenGL 3+" TRUE)
else()
	set(ENABLE_OPENGL3 FALSE)
endif()

if(ANDROID OR EMSCRIPTEN)
	set(ENABLE_OPENGL FALSE)
else()
	option(ENABLE_OPENGL "Enable OpenGL" TRUE)
endif()

if(EMSCRIPTEN OR APPLE)
	set(ENABLE_GLES1 FALSE)
else()
	if(ANDROID)
		set(DEFAULT_GLES1 TRUE)
	endif()
	option(ENABLE_GLES1 "Enable OpenGL ES" ${DEFAULT_GLES1})
endif()

if(APPLE)
	set(ENABLE_GLES2 FALSE)
	set(ENABLE_WEBGL1 FALSE)
else()
	if(ANDROID OR EMSCRIPTEN)
		set(DEFAULT_GLES2 TRUE)
	endif()
	if(EMSCRIPTEN)
		set(DEFAULT_WEBGL1 TRUE)
	endif()
	option(ENABLE_GLES2 "Enable OpenGL ES 2+" ${DEFAULT_GLES2})
	#option(ENABLE_WEBGL1 "Enable WebGL (requires GLES2)" ${DEFAULT_WEBGL1})
	set(ENABLE_WEBGL1 FALSE) # not working currently
	if(ENABLE_WEBGL1)
		set(ENABLE_GLES2 TRUE)
	endif()
endif()

if(ENABLE_OPENGL)
	add_definitions(-D_IRR_COMPILE_WITH_OPENGL_)
	set(OPENGL_DIRECT_LINK TRUE) # driver relies on this
	if(DEVICE STREQUAL "WINDOWS")
		add_definitions(-D_IRR_COMPILE_WITH_WGL_MANAGER_)
	elseif(DEVICE STREQUAL "X11")
		add_definitions(-D_IRR_COMPILE_WITH_GLX_MANAGER_)
	elseif(DEVICE STREQUAL "OSX")
		add_definitions(-D_IRR_COMPILE_WITH_NSOGL_MANAGER_)
	endif()
endif()

if(ENABLE_OPENGL3)
	if (NOT USE_SDL2)
		message(FATAL_ERROR "OpenGL 3 driver requires SDL2")
	endif()
endif()

if(ENABLE_GLES1)
	if (USE_SDL2)
		message(FATAL_ERROR "OpenGL ES 1 is not supported with SDL2")
	endif()
	add_definitions(-D_IRR_COMPILE_WITH_OGLES1_)
	set(OPENGLES_DIRECT_LINK TRUE)
	if(DEVICE MATCHES "^(WINDOWS|X11|ANDROID)$")
		add_definitions(-D_IRR_COMPILE_WITH_EGL_MANAGER_)
	endif()
endif()

if(ENABLE_GLES2)
	add_definitions(-D_IRR_COMPILE_WITH_OGLES2_)
	if(DEVICE MATCHES "^(WINDOWS|X11|ANDROID)$" OR EMSCRIPTEN)
		add_definitions(-D_IRR_COMPILE_WITH_EGL_MANAGER_)
	endif()
endif()

if(ENABLE_WEBGL1)
	add_definitions(-D_IRR_COMPILE_WITH_WEBGL1_)
endif()

# Misc

include(TestBigEndian)
TEST_BIG_ENDIAN(BIG_ENDIAN)
if(BIG_ENDIAN)
	add_definitions(-D__BIG_ENDIAN__)
endif()

# Configuration report

message(STATUS "Device: ${DEVICE}")
message(STATUS "OpenGL: ${ENABLE_OPENGL}")
message(STATUS "OpenGL 3: ${ENABLE_OPENGL3}")
message(STATUS "OpenGL ES: ${ENABLE_GLES1}")
if (ENABLE_GLES2)
	message(STATUS "OpenGL ES 2: ON (unified)")
else()
	message(STATUS "OpenGL ES 2: OFF")
endif()
#message(STATUS "WebGL: ${ENABLE_WEBGL1}")

# Required libs

find_package(ZLIB REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)


if(ENABLE_GLES1)
	# only tested on Android, probably works on Linux (is this needed anywhere else?)
	find_library(OPENGLES_LIBRARY NAMES GLESv1_CM REQUIRED)
	find_library(EGL_LIBRARY NAMES EGL REQUIRED)

	message(STATUS "Found OpenGLES: ${OPENGLES_LIBRARY}")
endif()
if(ENABLE_GLES2)
	find_package(OpenGLES2 REQUIRED)
endif()
if(ENABLE_OPENGL OR ENABLE_OPENGL3)
	find_package(OpenGL REQUIRED)
endif()
if(USE_SDL2)
	find_package(SDL2 REQUIRED)
	message(STATUS "Found SDL2: ${SDL2_LIBRARIES}")

	# unfortunately older SDL does not provide its version to cmake, so check header.
	set(CMAKE_REQUIRED_INCLUDES ${SDL2_INCLUDE_DIRS})
	unset(CHECK_SDL_VERSION CACHE)
	check_cxx_source_compiles("#include <SDL_version.h>\n\
#if !SDL_VERSION_ATLEAST(2, 0, 10)\n\
#error\n\
#endif\n\
int main() {}" CHECK_SDL_VERSION)
	if(NOT CHECK_SDL_VERSION)
		message(FATAL_ERROR "SDL2 is too old, required is at least 2.0.10!")
	endif()

	# ...no target either.
	if(NOT TARGET SDL2::SDL2)
		add_library(SDL2::SDL2 SHARED IMPORTED)
		set_target_properties(SDL2::SDL2 PROPERTIES
			IMPORTED_LOCATION "${SDL2_LIBRARIES}")
	endif()
endif()

# More special config

if(ENABLE_OPENGL AND DEVICE STREQUAL "SDL")
	# The legacy GL driver requires some symbols from GL 4.5 to compile,
	# which SDL only provides since 2.26.0 (Nov 2022).
	# We have a fallback in case this isn't satisfied so test for it.
	set(CMAKE_REQUIRED_INCLUDES ${SDL2_INCLUDE_DIRS})
	unset(CHECK_GL_VERSION_4_5 CACHE)
	check_cxx_source_compiles("#include <SDL_opengl.h>\n\
#ifndef GL_VERSION_4_5\n\
#error\n\
#endif\n\
int main() {}" CHECK_GL_VERSION_4_5)
	if(CHECK_GL_VERSION_4_5)
		add_definitions(-DIRR_PREFER_SDL_GL_HEADER)
	endif()
endif()

# Platform-specific libs

if(ANDROID)
	enable_language(C)
elseif(APPLE)
	find_library(COCOA_LIB Cocoa REQUIRED)
	find_library(IOKIT_LIB IOKit REQUIRED)

	add_definitions(-DGL_SILENCE_DEPRECATION)
elseif(NOT USE_SDL2)
	# Unix probably
	find_package(X11 REQUIRED)
	if(USE_XINPUT2 AND NOT X11_Xi_FOUND)
		message(FATAL_ERROR "XInput not found")
	endif()
endif()

set(link_includes
	"${PROJECT_SOURCE_DIR}/include"
	"${CMAKE_CURRENT_SOURCE_DIR}"

	"${ZLIB_INCLUDE_DIR}"
	"${JPEG_INCLUDE_DIR}"
	"${PNG_INCLUDE_DIR}"
	"$<$<BOOL:${USE_SDL2}>:${SDL2_INCLUDE_DIRS}>"

	${OPENGL_INCLUDE_DIR}
	${OPENGLES2_INCLUDE_DIR}
	${EGL_INCLUDE_DIR}

	"$<$<PLATFORM_ID:Android>:${ANDROID_NDK}/sources/android/native_app_glue>"
	"$<$<BOOL:${USE_X11}>:${X11_INCLUDE_DIR}>"
)

# Source files

set(IRRMESHLOADER
	CB3DMeshFileLoader.cpp
	COBJMeshFileLoader.cpp
	CXMeshFileLoader.cpp
)

add_library(IRRMESHOBJ OBJECT
	CSkinnedMesh.cpp
	CBoneSceneNode.cpp
	CMeshSceneNode.cpp
	CAnimatedMeshSceneNode.cpp
	${IRRMESHLOADER}
)

add_library(IRROBJ OBJECT
	CBillboardSceneNode.cpp
	CCameraSceneNode.cpp
	CDummyTransformationSceneNode.cpp
	CEmptySceneNode.cpp
	CMeshManipulator.cpp
	CSceneCollisionManager.cpp
	CSceneManager.cpp
	CMeshCache.cpp
)

set(IRRDRVROBJ
	CNullDriver.cpp
	CGLXManager.cpp
	CWGLManager.cpp
	CEGLManager.cpp
	CSDLManager.cpp
	mt_opengl_loader.cpp
)

# the two legacy drivers

if(ENABLE_OPENGL)
	set(IRRDRVROBJ
		${IRRDRVROBJ}
		COpenGLCacheHandler.cpp
		COpenGLDriver.cpp
		COpenGLSLMaterialRenderer.cpp
		COpenGLExtensionHandler.cpp
	)
endif()

if(ENABLE_GLES1)
	set(IRRDRVROBJ
		${IRRDRVROBJ}
		COGLESDriver.cpp
		COGLESExtensionHandler.cpp
	)
endif()

# the unified drivers

if(ENABLE_OPENGL3 OR ENABLE_GLES2)
	set(IRRDRVROBJ
		${IRRDRVROBJ}
		OpenGL/Driver.cpp
		OpenGL/ExtensionHandler.cpp
		OpenGL/FixedPipelineRenderer.cpp
		OpenGL/MaterialRenderer.cpp
		OpenGL/Renderer2D.cpp
	)
endif()

if(ENABLE_OPENGL3)
	set(IRRDRVROBJ
		${IRRDRVROBJ}
		OpenGL3/Driver.cpp
	)
endif()

if(ENABLE_GLES2)
	set(IRRDRVROBJ
		${IRRDRVROBJ}
		OpenGLES2/Driver.cpp
	)
endif()

set(IRRIMAGEOBJ
	CColorConverter.cpp
	CImage.cpp
	CImageLoaderBMP.cpp
	CImageLoaderJPG.cpp
	CImageLoaderPNG.cpp
	CImageLoaderTGA.cpp
	CImageWriterJPG.cpp
	CImageWriterPNG.cpp
)

add_library(IRRVIDEOOBJ OBJECT
	CFPSCounter.cpp
	${IRRDRVROBJ}
	${IRRIMAGEOBJ}
)

add_library(IRRIOOBJ OBJECT
	CFileList.cpp
	CFileSystem.cpp
	CLimitReadFile.cpp
	CMemoryFile.cpp
	CReadFile.cpp
	CWriteFile.cpp
	CZipReader.cpp
	CAttributes.cpp
)

add_library(IRROTHEROBJ OBJECT
	CIrrDeviceSDL.cpp
	CIrrDeviceLinux.cpp
	CIrrDeviceStub.cpp
	CIrrDeviceWin32.cpp
	CLogger.cpp
	COSOperator.cpp
	Irrlicht.cpp
	os.cpp
)

if(ENABLE_OPENGL3)
	target_compile_definitions(IRROTHEROBJ PRIVATE ENABLE_OPENGL3)
endif()

if(ANDROID)
	target_sources(IRROTHEROBJ PRIVATE
		Android/CIrrDeviceAndroid.cpp
		Android/CAndroidAssetReader.cpp
		Android/CAndroidAssetFileArchive.cpp
		Android/CKeyEventWrapper.cpp
	)
elseif(APPLE)
	# Build all IRROTHEROBJ sources as objc++, including the .cpp's
	set_target_properties(IRROTHEROBJ PROPERTIES COMPILE_OPTIONS "-xobjective-c++")
	target_sources(IRROTHEROBJ PRIVATE
		CIrrDeviceOSX.mm
		CNSOGLManager.mm
	)
endif()

if(USE_X11)
	target_compile_definitions(IRROTHEROBJ PRIVATE _IRR_COMPILE_WITH_X11_)
endif()

if(USE_XINPUT2)
	target_compile_definitions(IRROTHEROBJ PRIVATE _IRR_LINUX_X11_XINPUT2_)
endif()

if(USE_XCURSOR)
	target_compile_definitions(IRROTHEROBJ PRIVATE _IRR_LINUX_XCURSOR_)
endif()

add_library(IRRGUIOBJ OBJECT
	CGUIButton.cpp
	CGUICheckBox.cpp
	CGUIComboBox.cpp
	CGUIEditBox.cpp
	CGUIEnvironment.cpp
	CGUIFileOpenDialog.cpp
	CGUIFont.cpp
	CGUIImage.cpp
	CGUIListBox.cpp
	CGUIScrollBar.cpp
	CGUISkin.cpp
	CGUIStaticText.cpp
	CGUITabControl.cpp
	CGUISpriteBank.cpp
	CGUIImageList.cpp
)

# Library

add_library(IrrlichtMt)
foreach(object_lib
	IRRMESHOBJ IRROBJ IRRVIDEOOBJ
	IRRIOOBJ IRROTHEROBJ IRRGUIOBJ)
	# Set include directories for object library compilation
	target_include_directories(${object_lib} PRIVATE ${link_includes})
	# Add objects from object library to main library
	target_sources(IrrlichtMt PRIVATE $<TARGET_OBJECTS:${object_lib}>)
endforeach()

# Alias target provides add_submodule compatibility
add_library(IrrlichtMt::IrrlichtMt ALIAS IrrlichtMt)

target_include_directories(IrrlichtMt
	PUBLIC
		"$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include/>"
		"$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
		"$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/irrlichtmt>"
	PRIVATE
		${link_includes}
)

# this needs to be here and not in a variable (like link_includes) due to issues
# with the generator expressions on at least CMake 3.22, but not 3.28 or later
target_link_libraries(IrrlichtMt PRIVATE
	${ZLIB_LIBRARY}
	${JPEG_LIBRARY}
	${PNG_LIBRARY}
	"$<$<BOOL:${USE_SDL2}>:SDL2::SDL2>"

	"$<$<BOOL:${OPENGL_DIRECT_LINK}>:${OPENGL_LIBRARIES}>"
	"$<$<BOOL:${OPENGLES_DIRECT_LINK}>:${OPENGLES_LIBRARY}>"
	${EGL_LIBRARY}

	"$<$<PLATFORM_ID:Android>:-landroid -llog>"
	${COCOA_LIB}
	${IOKIT_LIB}
	"$<$<PLATFORM_ID:Windows>:gdi32>"
	"$<$<PLATFORM_ID:Windows>:winmm>"
	"$<$<BOOL:${USE_X11}>:${X11_X11_LIB}>"
	"$<$<BOOL:${USE_X11}>:${X11_Xi_LIB}>"
)

if(WIN32)
	target_compile_definitions(IrrlichtMt INTERFACE _IRR_WINDOWS_API_) # used in _IRR_DEBUG_BREAK_IF definition in a public header
endif()
target_compile_definitions(IrrlichtMt INTERFACE "IRRLICHT_API=${API_IMPORT}")
if(APPLE OR ANDROID OR EMSCRIPTEN)
	target_compile_definitions(IrrlichtMt PUBLIC IRR_MOBILE_PATHS)
endif()

set_target_properties(IrrlichtMt PROPERTIES
	VERSION ${PROJECT_VERSION}
)

if(WIN32)
	set_target_properties(IrrlichtMt PROPERTIES PREFIX "") # for DLL name
endif()

# Installation of library
install(TARGETS IrrlichtMt
	EXPORT IrrlichtMt-export
	DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
