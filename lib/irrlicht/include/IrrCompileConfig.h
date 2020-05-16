// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_COMPILE_CONFIG_H_INCLUDED__
#define __IRR_COMPILE_CONFIG_H_INCLUDED__

//! Irrlicht SDK Version
#define IRRLICHT_VERSION_MAJOR 1
#define IRRLICHT_VERSION_MINOR 8
#define IRRLICHT_VERSION_REVISION 4
// This flag will be defined only in SVN, the official release code will have
// it undefined
//#define IRRLICHT_VERSION_SVN -alpha
#define IRRLICHT_SDK_VERSION "1.8.4"

#include <stdio.h> // TODO: Although included elsewhere this is required at least for mingw

//! The defines for different operating system are:
//! _IRR_XBOX_PLATFORM_ for XBox
//! _IRR_WINDOWS_ for all irrlicht supported Windows versions
//! _IRR_WINDOWS_CE_PLATFORM_ for Windows CE
//! _IRR_WINDOWS_API_ for Windows or XBox
//! _IRR_LINUX_PLATFORM_ for Linux (it is defined here if no other os is defined)
//! _IRR_SOLARIS_PLATFORM_ for Solaris
//! _IRR_OSX_PLATFORM_ for Apple systems running OSX
//! _IRR_POSIX_API_ for Posix compatible systems
//! Note: PLATFORM defines the OS specific layer, API can group several platforms

//! DEVICE is the windowing system used, several PLATFORMs support more than one DEVICE
//! Irrlicht can be compiled with more than one device
//! _IRR_COMPILE_WITH_WINDOWS_DEVICE_ for Windows API based device
//! _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_ for Windows CE API based device
//! _IRR_COMPILE_WITH_OSX_DEVICE_ for Cocoa native windowing on OSX
//! _IRR_COMPILE_WITH_X11_DEVICE_ for Linux X11 based device
//! _IRR_COMPILE_WITH_SDL_DEVICE_ for platform independent SDL framework
//! _IRR_COMPILE_WITH_CONSOLE_DEVICE_ for no windowing system, used as a fallback
//! _IRR_COMPILE_WITH_FB_DEVICE_ for framebuffer systems

//! Passing defines to the compiler which have NO in front of the _IRR definename is an alternative
//! way which can be used to disable defines (instead of outcommenting them in this header).
//! So defines can be controlled from Makefiles or Projectfiles which allows building
//! different library versions without having to change the sources.
//! Example: NO_IRR_COMPILE_WITH_X11_ would disable X11


//! Uncomment this line to compile with the SDL device
//#define _IRR_COMPILE_WITH_SDL_DEVICE_
#ifdef NO_IRR_COMPILE_WITH_SDL_DEVICE_
#undef _IRR_COMPILE_WITH_SDL_DEVICE_
#endif

//! Comment this line to compile without the fallback console device.
#define _IRR_COMPILE_WITH_CONSOLE_DEVICE_
#ifdef NO_IRR_COMPILE_WITH_CONSOLE_DEVICE_
#undef _IRR_COMPILE_WITH_CONSOLE_DEVICE_
#endif

//! WIN32 for Windows32
//! WIN64 for Windows64
// The windows platform and API support SDL and WINDOW device
#if defined(_WIN32) || defined(_WIN64) || defined(WIN32) || defined(WIN64)
#define _IRR_WINDOWS_
#define _IRR_WINDOWS_API_
#define _IRR_COMPILE_WITH_WINDOWS_DEVICE_
#endif

//! WINCE is a very restricted environment for mobile devices
#if defined(_WIN32_WCE)
#define _IRR_WINDOWS_
#define _IRR_WINDOWS_API_
#define _IRR_WINDOWS_CE_PLATFORM_
#define _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_
#endif

#if defined(_MSC_VER) && (_MSC_VER < 1300)
#  error "Only Microsoft Visual Studio 7.0 and later are supported."
#endif

// XBox only suppots the native Window stuff
#if defined(_XBOX)
	#undef _IRR_WINDOWS_
	#define _IRR_XBOX_PLATFORM_
	#define _IRR_WINDOWS_API_
	//#define _IRR_COMPILE_WITH_WINDOWS_DEVICE_
	#undef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
	//#define _IRR_COMPILE_WITH_SDL_DEVICE_

	#include <xtl.h>
#endif

#if defined(__APPLE__) || defined(MACOSX)
#if !defined(MACOSX)
#define MACOSX // legacy support
#endif
#define _IRR_OSX_PLATFORM_
#define _IRR_COMPILE_WITH_OSX_DEVICE_
#endif

#if !defined(_IRR_WINDOWS_API_) && !defined(_IRR_OSX_PLATFORM_)
#ifndef _IRR_SOLARIS_PLATFORM_
#define _IRR_LINUX_PLATFORM_
#endif
#define _IRR_POSIX_API_
#define _IRR_COMPILE_WITH_X11_DEVICE_
#endif


//! Define _IRR_COMPILE_WITH_JOYSTICK_SUPPORT_ if you want joystick events.
#define _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#ifdef NO_IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#undef _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#endif


//! Maximum number of texture an SMaterial can have, up to 8 are supported by Irrlicht.
#define _IRR_MATERIAL_MAX_TEXTURES_ 4

//! Define _IRR_COMPILE_WITH_DIRECT3D_8_ and _IRR_COMPILE_WITH_DIRECT3D_9_ to
//! compile the Irrlicht engine with Direct3D8 and/or DIRECT3D9.
/** If you only want to use the software device or opengl you can disable those defines.
This switch is mostly disabled because people do not get the g++ compiler compile
directX header files, and directX is only available on Windows platforms. If you
are using Dev-Cpp, and want to compile this using a DX dev pack, you can define
_IRR_COMPILE_WITH_DX9_DEV_PACK_. So you simply need to add something like this
to the compiler settings: -DIRR_COMPILE_WITH_DX9_DEV_PACK
and this to the linker settings: -ld3dx9 -ld3dx8

Microsoft have chosen to remove D3D8 headers from their recent DXSDKs, and
so D3D8 support is now disabled by default.  If you really want to build
with D3D8 support, then you will have to source a DXSDK with the appropriate
headers, e.g. Summer 2004.  This is a Microsoft issue, not an Irrlicht one.
*/
#if defined(_IRR_WINDOWS_API_) && (!defined(__GNUC__) || defined(IRR_COMPILE_WITH_DX9_DEV_PACK))

//! Define _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_ if you want to use DirectInput for joystick handling.
/** This only applies to Windows devices, currently only supported under Win32 device.
If not defined, Windows Multimedia library is used, which offers also broad support for joystick devices. */
#define _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
#ifdef NO_IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
#undef _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
#endif
// can't get this to compile currently under borland, can be removed if someone has a better solution
#if defined(__BORLANDC__)
#undef _IRR_COMPILE_WITH_DIRECTINPUT_JOYSTICK_
#endif

//! Only define _IRR_COMPILE_WITH_DIRECT3D_8_ if you have an appropriate DXSDK, e.g. Summer 2004
// #define _IRR_COMPILE_WITH_DIRECT3D_8_
#define _IRR_COMPILE_WITH_DIRECT3D_9_

#ifdef NO_IRR_COMPILE_WITH_DIRECT3D_8_
#undef _IRR_COMPILE_WITH_DIRECT3D_8_
#endif
#ifdef NO_IRR_COMPILE_WITH_DIRECT3D_9_
#undef _IRR_COMPILE_WITH_DIRECT3D_9_
#endif

#endif

//! Define _IRR_COMPILE_WITH_OPENGL_ to compile the Irrlicht engine with OpenGL.
/** If you do not wish the engine to be compiled with OpenGL, comment this
define out. */
#define _IRR_COMPILE_WITH_OPENGL_
#ifdef NO_IRR_COMPILE_WITH_OPENGL_
#undef _IRR_COMPILE_WITH_OPENGL_
#endif

//! Define _IRR_COMPILE_WITH_SOFTWARE_ to compile the Irrlicht engine with software driver
/** If you do not need the software driver, or want to use Burning's Video instead,
comment this define out */
#define _IRR_COMPILE_WITH_SOFTWARE_
#ifdef NO_IRR_COMPILE_WITH_SOFTWARE_
#undef _IRR_COMPILE_WITH_SOFTWARE_
#endif

//! Define _IRR_COMPILE_WITH_BURNINGSVIDEO_ to compile the Irrlicht engine with Burning's video driver
/** If you do not need this software driver, you can comment this define out. */
#define _IRR_COMPILE_WITH_BURNINGSVIDEO_
#ifdef NO_IRR_COMPILE_WITH_BURNINGSVIDEO_
#undef _IRR_COMPILE_WITH_BURNINGSVIDEO_
#endif

//! Define _IRR_COMPILE_WITH_X11_ to compile the Irrlicht engine with X11 support.
/** If you do not wish the engine to be compiled with X11, comment this
define out. */
// Only used in LinuxDevice.
#define _IRR_COMPILE_WITH_X11_
#ifdef NO_IRR_COMPILE_WITH_X11_
#undef _IRR_COMPILE_WITH_X11_
#endif

//! Define _IRR_OPENGL_USE_EXTPOINTER_ if the OpenGL renderer should use OpenGL extensions via function pointers.
/** On some systems there is no support for the dynamic extension of OpenGL
	via function pointers such that this has to be undef'ed. */
#if !defined(_IRR_OSX_PLATFORM_) && !defined(_IRR_SOLARIS_PLATFORM_)
#define _IRR_OPENGL_USE_EXTPOINTER_
#endif

//! On some Linux systems the XF86 vidmode extension or X11 RandR are missing. Use these flags
//! to remove the dependencies such that Irrlicht will compile on those systems, too.
//! If you don't need colored cursors you can also disable the Xcursor extension
#if defined(_IRR_LINUX_PLATFORM_) && defined(_IRR_COMPILE_WITH_X11_)
#define _IRR_LINUX_X11_VIDMODE_
//#define _IRR_LINUX_X11_RANDR_
#ifdef NO_IRR_LINUX_X11_VIDMODE_
#undef _IRR_LINUX_X11_VIDMODE_
#endif
#ifdef NO_IRR_LINUX_X11_RANDR_
#undef _IRR_LINUX_X11_RANDR_
#endif

//! X11 has by default only monochrome cursors, but using the Xcursor library we can also get color cursor support.
//! If you have the need for custom color cursors on X11 then enable this and make sure you also link
//! to the Xcursor library in your Makefile/Projectfile.
//#define _IRR_LINUX_XCURSOR_
#ifdef NO_IRR_LINUX_XCURSOR_
#undef _IRR_LINUX_XCURSOR_
#endif

#endif

//! Define _IRR_COMPILE_WITH_GUI_ to compile the engine with the built-in GUI
/** Disable this if you are using an external library to draw the GUI. If you disable this then
you will not be able to use anything provided by the GUI Environment, including loading fonts. */
#define _IRR_COMPILE_WITH_GUI_
#ifdef NO_IRR_COMPILE_WITH_GUI_
#undef _IRR_COMPILE_WITH_GUI_
#endif

//! Define _IRR_WCHAR_FILESYSTEM to enable unicode filesystem support for the engine.
/** This enables the engine to read/write from unicode filesystem. If you
disable this feature, the engine behave as before (ansi). This is currently only supported
for Windows based systems. You also have to set #define UNICODE for this to compile.
*/
//#define _IRR_WCHAR_FILESYSTEM
#ifdef NO_IRR_WCHAR_FILESYSTEM
#undef _IRR_WCHAR_FILESYSTEM
#endif

//! Define _IRR_COMPILE_WITH_JPEGLIB_ to enable compiling the engine using libjpeg.
/** This enables the engine to read jpeg images. If you comment this out,
the engine will no longer read .jpeg images. */
#define _IRR_COMPILE_WITH_LIBJPEG_
#ifdef NO_IRR_COMPILE_WITH_LIBJPEG_
#undef _IRR_COMPILE_WITH_LIBJPEG_
#endif

//! Define _IRR_USE_NON_SYSTEM_JPEG_LIB_ to let irrlicht use the jpeglib which comes with irrlicht.
/** If this is commented out, Irrlicht will try to compile using the jpeg lib installed in the system.
	This is only used when _IRR_COMPILE_WITH_LIBJPEG_ is defined. */
#define _IRR_USE_NON_SYSTEM_JPEG_LIB_
#ifdef NO_IRR_USE_NON_SYSTEM_JPEG_LIB_
#undef _IRR_USE_NON_SYSTEM_JPEG_LIB_
#endif

//! Define _IRR_COMPILE_WITH_LIBPNG_ to enable compiling the engine using libpng.
/** This enables the engine to read png images. If you comment this out,
the engine will no longer read .png images. */
#define _IRR_COMPILE_WITH_LIBPNG_
#ifdef NO_IRR_COMPILE_WITH_LIBPNG_
#undef _IRR_COMPILE_WITH_LIBPNG_
#endif

//! Define _IRR_USE_NON_SYSTEM_LIBPNG_ to let irrlicht use the libpng which comes with irrlicht.
/** If this is commented out, Irrlicht will try to compile using the libpng installed in the system.
	This is only used when _IRR_COMPILE_WITH_LIBPNG_ is defined. */
#define _IRR_USE_NON_SYSTEM_LIB_PNG_
#ifdef NO_IRR_USE_NON_SYSTEM_LIB_PNG_
#undef _IRR_USE_NON_SYSTEM_LIB_PNG_
#endif

//! Define _IRR_D3D_NO_SHADER_DEBUGGING to disable shader debugging in D3D9
/** If _IRR_D3D_NO_SHADER_DEBUGGING is undefined in IrrCompileConfig.h,
it is possible to debug all D3D9 shaders in VisualStudio. All shaders
(which have been generated in memory or read from archives for example) will be emitted
into a temporary file at runtime for this purpose. To debug your shaders, choose
Debug->Direct3D->StartWithDirect3DDebugging in Visual Studio, and for every shader a
file named 'irr_dbg_shader_%%.vsh' or 'irr_dbg_shader_%%.psh' will be created. Drag'n'drop
the file you want to debug into visual studio. That's it. You can now set breakpoints and
watch registers, variables etc. This works with ASM, HLSL, and both with pixel and vertex shaders.
Note that the engine will run in D3D REF for this, which is a lot slower than HAL. */
#define _IRR_D3D_NO_SHADER_DEBUGGING
#ifdef NO_IRR_D3D_NO_SHADER_DEBUGGING
#undef _IRR_D3D_NO_SHADER_DEBUGGING
#endif

//! Define _IRR_D3D_USE_LEGACY_HLSL_COMPILER to enable the old HLSL compiler in recent DX SDKs
/** This enables support for ps_1_x shaders for recent DX SDKs. Otherwise, support
for this shader model is not available anymore in SDKs after Oct2006. You need to
distribute the OCT2006_d3dx9_31_x86.cab or OCT2006_d3dx9_31_x64.cab though, in order
to provide the user with the proper DLL. That's why it's disabled by default. */
//#define _IRR_D3D_USE_LEGACY_HLSL_COMPILER
#ifdef NO_IRR_D3D_USE_LEGACY_HLSL_COMPILER
#undef _IRR_D3D_USE_LEGACY_HLSL_COMPILER
#endif

//! Define _IRR_COMPILE_WITH_CG_ to enable Cg Shading Language support
//#define _IRR_COMPILE_WITH_CG_
#ifdef NO_IRR_COMPILE_WITH_CG_
#undef _IRR_COMPILE_WITH_CG_
#endif
#if !defined(_IRR_COMPILE_WITH_OPENGL_) && !defined(_IRR_COMPILE_WITH_DIRECT3D_9_)
#undef _IRR_COMPILE_WITH_CG_
#endif

//! Define _IRR_USE_NVIDIA_PERFHUD_ to opt-in to using the nVidia PerHUD tool
/** Enable, by opting-in, to use the nVidia PerfHUD performance analysis driver
tool <http://developer.nvidia.com/object/nvperfhud_home.html>. */
#undef _IRR_USE_NVIDIA_PERFHUD_

//! Define one of the three setting for Burning's Video Software Rasterizer
/** So if we were marketing guys we could say Irrlicht has 4 Software-Rasterizers.
	In a Nutshell:
		All Burnings Rasterizers use 32 Bit Backbuffer, 32Bit Texture & 32 Bit Z or WBuffer,
		16 Bit/32 Bit can be adjusted on a global flag.

		BURNINGVIDEO_RENDERER_BEAUTIFUL
			32 Bit + Vertexcolor + Lighting + Per Pixel Perspective Correct + SubPixel/SubTexel Correct +
			Bilinear Texturefiltering + WBuffer

		BURNINGVIDEO_RENDERER_FAST
			32 Bit + Per Pixel Perspective Correct + SubPixel/SubTexel Correct + WBuffer +
			Bilinear Dithering TextureFiltering + WBuffer

		BURNINGVIDEO_RENDERER_ULTRA_FAST
			16Bit + SubPixel/SubTexel Correct + ZBuffer
*/

#define BURNINGVIDEO_RENDERER_BEAUTIFUL
//#define BURNINGVIDEO_RENDERER_FAST
//#define BURNINGVIDEO_RENDERER_ULTRA_FAST
//#define BURNINGVIDEO_RENDERER_CE

//! Uncomment the following line if you want to ignore the deprecated warnings
//#define IGNORE_DEPRECATED_WARNING

//! Define _IRR_COMPILE_WITH_IRR_SCENE_LOADER_ if you want to be able to load
/** .irr scenes using ISceneManager::loadScene */
#define _IRR_COMPILE_WITH_IRR_SCENE_LOADER_
#ifdef NO_IRR_COMPILE_WITH_IRR_SCENE_LOADER_
#undef _IRR_COMPILE_WITH_IRR_SCENE_LOADER_
#endif

//! Define _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_ if you want to use bone based
/** animated meshes. If you compile without this, you will be unable to load
B3D, MS3D or X meshes */
#define _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_
#ifdef NO_IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_
#undef _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_
#endif

#ifdef _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_
//! Define _IRR_COMPILE_WITH_B3D_LOADER_ if you want to use Blitz3D files
#define _IRR_COMPILE_WITH_B3D_LOADER_
#ifdef NO_IRR_COMPILE_WITH_B3D_LOADER_
#undef _IRR_COMPILE_WITH_B3D_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_MS3D_LOADER_ if you want to Milkshape files
#define _IRR_COMPILE_WITH_MS3D_LOADER_
#ifdef NO_IRR_COMPILE_WITH_MS3D_LOADER_
#undef _IRR_COMPILE_WITH_MS3D_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_X_LOADER_ if you want to use Microsoft X files
#define _IRR_COMPILE_WITH_X_LOADER_
#ifdef NO_IRR_COMPILE_WITH_X_LOADER_
#undef _IRR_COMPILE_WITH_X_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_OGRE_LOADER_ if you want to load Ogre 3D files
#define _IRR_COMPILE_WITH_OGRE_LOADER_
#ifdef NO_IRR_COMPILE_WITH_OGRE_LOADER_
#undef _IRR_COMPILE_WITH_OGRE_LOADER_
#endif
#endif	// _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_

//! Define _IRR_COMPILE_WITH_IRR_MESH_LOADER_ if you want to load Irrlicht Engine .irrmesh files
#define _IRR_COMPILE_WITH_IRR_MESH_LOADER_
#ifdef NO_IRR_COMPILE_WITH_IRR_MESH_LOADER_
#undef _IRR_COMPILE_WITH_IRR_MESH_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_HALFLIFE_LOADER_ if you want to load Halflife animated files
#define _IRR_COMPILE_WITH_HALFLIFE_LOADER_
#ifdef NO_IRR_COMPILE_WITH_HALFLIFE_LOADER_
#undef _IRR_COMPILE_WITH_HALFLIFE_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_MD2_LOADER_ if you want to load Quake 2 animated files
#define _IRR_COMPILE_WITH_MD2_LOADER_
#ifdef NO_IRR_COMPILE_WITH_MD2_LOADER_
#undef _IRR_COMPILE_WITH_MD2_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_MD3_LOADER_ if you want to load Quake 3 animated files
#define _IRR_COMPILE_WITH_MD3_LOADER_
#ifdef NO_IRR_COMPILE_WITH_MD3_LOADER_
#undef _IRR_COMPILE_WITH_MD3_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_3DS_LOADER_ if you want to load 3D Studio Max files
#define _IRR_COMPILE_WITH_3DS_LOADER_
#ifdef NO_IRR_COMPILE_WITH_3DS_LOADER_
#undef _IRR_COMPILE_WITH_3DS_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_COLLADA_LOADER_ if you want to load Collada files
#define _IRR_COMPILE_WITH_COLLADA_LOADER_
#ifdef NO_IRR_COMPILE_WITH_COLLADA_LOADER_
#undef _IRR_COMPILE_WITH_COLLADA_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_CSM_LOADER_ if you want to load Cartography Shop files
#define _IRR_COMPILE_WITH_CSM_LOADER_
#ifdef NO_IRR_COMPILE_WITH_CSM_LOADER_
#undef _IRR_COMPILE_WITH_CSM_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_BSP_LOADER_ if you want to load Quake 3 BSP files
#define _IRR_COMPILE_WITH_BSP_LOADER_
#ifdef NO_IRR_COMPILE_WITH_BSP_LOADER_
#undef _IRR_COMPILE_WITH_BSP_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_DMF_LOADER_ if you want to load DeleD files
#define _IRR_COMPILE_WITH_DMF_LOADER_
#ifdef NO_IRR_COMPILE_WITH_DMF_LOADER_
#undef _IRR_COMPILE_WITH_DMF_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_LMTS_LOADER_ if you want to load LMTools files
#define _IRR_COMPILE_WITH_LMTS_LOADER_
#ifdef NO_IRR_COMPILE_WITH_LMTS_LOADER_
#undef _IRR_COMPILE_WITH_LMTS_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_MY3D_LOADER_ if you want to load MY3D files
#define _IRR_COMPILE_WITH_MY3D_LOADER_
#ifdef NO_IRR_COMPILE_WITH_MY3D_LOADER_
#undef _IRR_COMPILE_WITH_MY3D_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_OBJ_LOADER_ if you want to load Wavefront OBJ files
#define _IRR_COMPILE_WITH_OBJ_LOADER_
#ifdef NO_IRR_COMPILE_WITH_OBJ_LOADER_
#undef _IRR_COMPILE_WITH_OBJ_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_OCT_LOADER_ if you want to load FSRad OCT files
#define _IRR_COMPILE_WITH_OCT_LOADER_
#ifdef NO_IRR_COMPILE_WITH_OCT_LOADER_
#undef _IRR_COMPILE_WITH_OCT_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_LWO_LOADER_ if you want to load Lightwave3D files
#define _IRR_COMPILE_WITH_LWO_LOADER_
#ifdef NO_IRR_COMPILE_WITH_LWO_LOADER_
#undef _IRR_COMPILE_WITH_LWO_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_STL_LOADER_ if you want to load stereolithography files
#define _IRR_COMPILE_WITH_STL_LOADER_
#ifdef NO_IRR_COMPILE_WITH_STL_LOADER_
#undef _IRR_COMPILE_WITH_STL_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_PLY_LOADER_ if you want to load Polygon (Stanford Triangle) files
#define _IRR_COMPILE_WITH_PLY_LOADER_
#ifdef NO_IRR_COMPILE_WITH_PLY_LOADER_
#undef _IRR_COMPILE_WITH_PLY_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_SMF_LOADER_ if you want to load 3D World Studio mesh files
#define _IRR_COMPILE_WITH_SMF_LOADER_
#ifdef NO_IRR_COMPILE_WITH_SMF_LOADER_
#undef _IRR_COMPILE_WITH_SMF_LOADER_
#endif

//! Define _IRR_COMPILE_WITH_IRR_WRITER_ if you want to write static .irrMesh files
#define _IRR_COMPILE_WITH_IRR_WRITER_
#ifdef NO_IRR_COMPILE_WITH_IRR_WRITER_
#undef _IRR_COMPILE_WITH_IRR_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_COLLADA_WRITER_ if you want to write Collada files
#define _IRR_COMPILE_WITH_COLLADA_WRITER_
#ifdef NO_IRR_COMPILE_WITH_COLLADA_WRITER_
#undef _IRR_COMPILE_WITH_COLLADA_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_STL_WRITER_ if you want to write .stl files
#define _IRR_COMPILE_WITH_STL_WRITER_
#ifdef NO_IRR_COMPILE_WITH_STL_WRITER_
#undef _IRR_COMPILE_WITH_STL_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_OBJ_WRITER_ if you want to write .obj files
#define _IRR_COMPILE_WITH_OBJ_WRITER_
#ifdef NO_IRR_COMPILE_WITH_OBJ_WRITER_
#undef _IRR_COMPILE_WITH_OBJ_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_PLY_WRITER_ if you want to write .ply files
#define _IRR_COMPILE_WITH_PLY_WRITER_
#ifdef NO_IRR_COMPILE_WITH_PLY_WRITER_
#undef _IRR_COMPILE_WITH_PLY_WRITER_
#endif

//! Define _IRR_COMPILE_WITH_BMP_LOADER_ if you want to load .bmp files
//! Disabling this loader will also disable the built-in font
#define _IRR_COMPILE_WITH_BMP_LOADER_
#ifdef NO_IRR_COMPILE_WITH_BMP_LOADER_
#undef _IRR_COMPILE_WITH_BMP_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_JPG_LOADER_ if you want to load .jpg files
#define _IRR_COMPILE_WITH_JPG_LOADER_
#ifdef NO_IRR_COMPILE_WITH_JPG_LOADER_
#undef _IRR_COMPILE_WITH_JPG_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_PCX_LOADER_ if you want to load .pcx files
#define _IRR_COMPILE_WITH_PCX_LOADER_
#ifdef NO_IRR_COMPILE_WITH_PCX_LOADER_
#undef _IRR_COMPILE_WITH_PCX_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_PNG_LOADER_ if you want to load .png files
#define _IRR_COMPILE_WITH_PNG_LOADER_
#ifdef NO_IRR_COMPILE_WITH_PNG_LOADER_
#undef _IRR_COMPILE_WITH_PNG_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_PPM_LOADER_ if you want to load .ppm/.pgm/.pbm files
#define _IRR_COMPILE_WITH_PPM_LOADER_
#ifdef NO_IRR_COMPILE_WITH_PPM_LOADER_
#undef _IRR_COMPILE_WITH_PPM_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_PSD_LOADER_ if you want to load .psd files
#define _IRR_COMPILE_WITH_PSD_LOADER_
#ifdef NO_IRR_COMPILE_WITH_PSD_LOADER_
#undef _IRR_COMPILE_WITH_PSD_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_DDS_LOADER_ if you want to load .dds files
// Outcommented because
// a) it doesn't compile on 64-bit currently
// b) anyone enabling it should be aware that S3TC compression algorithm which might be used in that loader
// is patented in the US by S3 and they do collect license fees when it's used in applications.
// So if you are unfortunate enough to develop applications for US market and their broken patent system be careful.
// #define _IRR_COMPILE_WITH_DDS_LOADER_
#ifdef NO_IRR_COMPILE_WITH_DDS_LOADER_
#undef _IRR_COMPILE_WITH_DDS_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_TGA_LOADER_ if you want to load .tga files
#define _IRR_COMPILE_WITH_TGA_LOADER_
#ifdef NO_IRR_COMPILE_WITH_TGA_LOADER_
#undef _IRR_COMPILE_WITH_TGA_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_WAL_LOADER_ if you want to load .wal files
#define _IRR_COMPILE_WITH_WAL_LOADER_
#ifdef NO_IRR_COMPILE_WITH_WAL_LOADER_
#undef _IRR_COMPILE_WITH_WAL_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_LMP_LOADER_ if you want to load .lmp files
#define _IRR_COMPILE_WITH_LMP_LOADER_
#ifdef NO_IRR_COMPILE_WITH_LMP_LOADER_
#undef _IRR_COMPILE_WITH_LMP_LOADER_
#endif
//! Define _IRR_COMPILE_WITH_RGB_LOADER_ if you want to load Silicon Graphics .rgb/.rgba/.sgi/.int/.inta/.bw files
#define _IRR_COMPILE_WITH_RGB_LOADER_
#ifdef NO_IRR_COMPILE_WITH_RGB_LOADER_
#undef _IRR_COMPILE_WITH_RGB_LOADER_
#endif

//! Define _IRR_COMPILE_WITH_BMP_WRITER_ if you want to write .bmp files
#define _IRR_COMPILE_WITH_BMP_WRITER_
#ifdef NO_IRR_COMPILE_WITH_BMP_WRITER_
#undef _IRR_COMPILE_WITH_BMP_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_JPG_WRITER_ if you want to write .jpg files
#define _IRR_COMPILE_WITH_JPG_WRITER_
#ifdef NO_IRR_COMPILE_WITH_JPG_WRITER_
#undef _IRR_COMPILE_WITH_JPG_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_PCX_WRITER_ if you want to write .pcx files
#define _IRR_COMPILE_WITH_PCX_WRITER_
#ifdef NO_IRR_COMPILE_WITH_PCX_WRITER_
#undef _IRR_COMPILE_WITH_PCX_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_PNG_WRITER_ if you want to write .png files
#define _IRR_COMPILE_WITH_PNG_WRITER_
#ifdef NO_IRR_COMPILE_WITH_PNG_WRITER_
#undef _IRR_COMPILE_WITH_PNG_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_PPM_WRITER_ if you want to write .ppm files
#define _IRR_COMPILE_WITH_PPM_WRITER_
#ifdef NO_IRR_COMPILE_WITH_PPM_WRITER_
#undef _IRR_COMPILE_WITH_PPM_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_PSD_WRITER_ if you want to write .psd files
#define _IRR_COMPILE_WITH_PSD_WRITER_
#ifdef NO_IRR_COMPILE_WITH_PSD_WRITER_
#undef _IRR_COMPILE_WITH_PSD_WRITER_
#endif
//! Define _IRR_COMPILE_WITH_TGA_WRITER_ if you want to write .tga files
#define _IRR_COMPILE_WITH_TGA_WRITER_
#ifdef NO_IRR_COMPILE_WITH_TGA_WRITER_
#undef _IRR_COMPILE_WITH_TGA_WRITER_
#endif

//! Define __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_ if you want to open ZIP and GZIP archives
/** ZIP reading has several more options below to configure. */
#define __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
#endif
#ifdef __IRR_COMPILE_WITH_ZIP_ARCHIVE_LOADER_
//! Define _IRR_COMPILE_WITH_ZLIB_ to enable compiling the engine using zlib.
/** This enables the engine to read from compressed .zip archives. If you
disable this feature, the engine can still read archives, but only uncompressed
ones. */
#define _IRR_COMPILE_WITH_ZLIB_
#ifdef NO_IRR_COMPILE_WITH_ZLIB_
#undef _IRR_COMPILE_WITH_ZLIB_
#endif
//! Define _IRR_USE_NON_SYSTEM_ZLIB_ to let irrlicht use the zlib which comes with irrlicht.
/** If this is commented out, Irrlicht will try to compile using the zlib
installed on the system. This is only used when _IRR_COMPILE_WITH_ZLIB_ is
defined. */
#define _IRR_USE_NON_SYSTEM_ZLIB_
#ifdef NO_IRR_USE_NON_SYSTEM_ZLIB_
#undef _IRR_USE_NON_SYSTEM_ZLIB_
#endif
//! Define _IRR_COMPILE_WITH_ZIP_ENCRYPTION_ if you want to read AES-encrypted ZIP archives
#define _IRR_COMPILE_WITH_ZIP_ENCRYPTION_
#ifdef NO_IRR_COMPILE_WITH_ZIP_ENCRYPTION_
#undef _IRR_COMPILE_WITH_ZIP_ENCRYPTION_
#endif
//! Define _IRR_COMPILE_WITH_BZIP2_ if you want to support bzip2 compressed zip archives
/** bzip2 is superior to the original zip file compression modes, but requires
a certain amount of memory for decompression and adds several files to the
library. */
#define _IRR_COMPILE_WITH_BZIP2_
#ifdef NO_IRR_COMPILE_WITH_BZIP2_
#undef _IRR_COMPILE_WITH_BZIP2_
#endif
//! Define _IRR_USE_NON_SYSTEM_BZLIB_ to let irrlicht use the bzlib which comes with irrlicht.
/** If this is commented out, Irrlicht will try to compile using the bzlib
installed on the system. This is only used when _IRR_COMPILE_WITH_BZLIB_ is
defined. */
#define _IRR_USE_NON_SYSTEM_BZLIB_
#ifdef NO_IRR_USE_NON_SYSTEM_BZLIB_
#undef _IRR_USE_NON_SYSTEM_BZLIB_
#endif
//! Define _IRR_COMPILE_WITH_LZMA_ if you want to use LZMA compressed zip files.
/** LZMA is a very efficient compression code, known from 7zip. Irrlicht
currently only supports zip archives, though. */
#define _IRR_COMPILE_WITH_LZMA_
#ifdef NO_IRR_COMPILE_WITH_LZMA_
#undef _IRR_COMPILE_WITH_LZMA_
#endif
#endif

//! Define __IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_ if you want to mount folders as archives
#define __IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_MOUNT_ARCHIVE_LOADER_
#endif
//! Define __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_ if you want to open ID software PAK archives
#define __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_PAK_ARCHIVE_LOADER_
#endif
//! Define __IRR_COMPILE_WITH_NPK_ARCHIVE_LOADER_ if you want to open Nebula Device NPK archives
#define __IRR_COMPILE_WITH_NPK_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_NPK_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_NPK_ARCHIVE_LOADER_
#endif
//! Define __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_ if you want to open TAR archives
#define __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_TAR_ARCHIVE_LOADER_
#endif
//! Define __IRR_COMPILE_WITH_WAD_ARCHIVE_LOADER_ if you want to open WAD archives
#define __IRR_COMPILE_WITH_WAD_ARCHIVE_LOADER_
#ifdef NO__IRR_COMPILE_WITH_WAD_ARCHIVE_LOADER_
#undef __IRR_COMPILE_WITH_WAD_ARCHIVE_LOADER_
#endif

//! Set FPU settings
/** Irrlicht should use approximate float and integer fpu techniques
precision will be lower but speed higher. currently X86 only
*/
#if !defined(_IRR_OSX_PLATFORM_) && !defined(_IRR_SOLARIS_PLATFORM_)
	//#define IRRLICHT_FAST_MATH
	#ifdef NO_IRRLICHT_FAST_MATH
	#undef IRRLICHT_FAST_MATH
	#endif
#endif

// Some cleanup and standard stuff

#ifdef _IRR_WINDOWS_API_

// To build Irrlicht as a static library, you must define _IRR_STATIC_LIB_ in both the
// Irrlicht build, *and* in the user application, before #including <irrlicht.h>
#ifndef _IRR_STATIC_LIB_
#ifdef IRRLICHT_EXPORTS
#define IRRLICHT_API __declspec(dllexport)
#else
#define IRRLICHT_API __declspec(dllimport)
#endif // IRRLICHT_EXPORT
#else
#define IRRLICHT_API
#endif // _IRR_STATIC_LIB_

// Declare the calling convention.
#if defined(_STDCALL_SUPPORTED)
#define IRRCALLCONV __stdcall
#else
#define IRRCALLCONV __cdecl
#endif // STDCALL_SUPPORTED

#else // _IRR_WINDOWS_API_

// Force symbol export in shared libraries built with gcc.
#if (__GNUC__ >= 4) && !defined(_IRR_STATIC_LIB_) && defined(IRRLICHT_EXPORTS)
#define IRRLICHT_API __attribute__ ((visibility("default")))
#else
#define IRRLICHT_API
#endif

#define IRRCALLCONV

#endif // _IRR_WINDOWS_API_

// We need to disable DIRECT3D9 support for Visual Studio 6.0 because
// those $%&$!! disabled support for it since Dec. 2004 and users are complaining
// about linker errors. Comment this out only if you are knowing what you are
// doing. (Which means you have an old DX9 SDK and VisualStudio6).
#ifdef _MSC_VER
#if (_MSC_VER < 1300 && !defined(__GNUC__))
#undef _IRR_COMPILE_WITH_DIRECT3D_9_
#pragma message("Compiling Irrlicht with Visual Studio 6.0, support for DX9 is disabled.")
#endif
#endif

// XBox does not have OpenGL or DirectX9
#if defined(_IRR_XBOX_PLATFORM_)
	#undef _IRR_COMPILE_WITH_OPENGL_
	#undef _IRR_COMPILE_WITH_DIRECT3D_9_
#endif

//! WinCE does not have OpenGL or DirectX9. use minimal loaders
#if defined(_WIN32_WCE)
	#undef _IRR_COMPILE_WITH_OPENGL_
	#undef _IRR_COMPILE_WITH_DIRECT3D_8_
	#undef _IRR_COMPILE_WITH_DIRECT3D_9_

	#undef BURNINGVIDEO_RENDERER_BEAUTIFUL
	#undef BURNINGVIDEO_RENDERER_FAST
	#undef BURNINGVIDEO_RENDERER_ULTRA_FAST
	#define BURNINGVIDEO_RENDERER_CE

	#undef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
	#define _IRR_COMPILE_WITH_WINDOWS_CE_DEVICE_
	//#define _IRR_WCHAR_FILESYSTEM

	#undef _IRR_COMPILE_WITH_IRR_MESH_LOADER_
	//#undef _IRR_COMPILE_WITH_MD2_LOADER_
	#undef _IRR_COMPILE_WITH_MD3_LOADER_
	#undef _IRR_COMPILE_WITH_3DS_LOADER_
	#undef _IRR_COMPILE_WITH_COLLADA_LOADER_
	#undef _IRR_COMPILE_WITH_CSM_LOADER_
	#undef _IRR_COMPILE_WITH_BSP_LOADER_
	#undef _IRR_COMPILE_WITH_DMF_LOADER_
	#undef _IRR_COMPILE_WITH_LMTS_LOADER_
	#undef _IRR_COMPILE_WITH_MY3D_LOADER_
	#undef _IRR_COMPILE_WITH_OBJ_LOADER_
	#undef _IRR_COMPILE_WITH_OCT_LOADER_
	#undef _IRR_COMPILE_WITH_OGRE_LOADER_
	#undef _IRR_COMPILE_WITH_LWO_LOADER_
	#undef _IRR_COMPILE_WITH_STL_LOADER_
	#undef _IRR_COMPILE_WITH_IRR_WRITER_
	#undef _IRR_COMPILE_WITH_COLLADA_WRITER_
	#undef _IRR_COMPILE_WITH_STL_WRITER_
	#undef _IRR_COMPILE_WITH_OBJ_WRITER_
	//#undef _IRR_COMPILE_WITH_BMP_LOADER_
	//#undef _IRR_COMPILE_WITH_JPG_LOADER_
	#undef _IRR_COMPILE_WITH_PCX_LOADER_
	//#undef _IRR_COMPILE_WITH_PNG_LOADER_
	#undef _IRR_COMPILE_WITH_PPM_LOADER_
	#undef _IRR_COMPILE_WITH_PSD_LOADER_
	//#undef _IRR_COMPILE_WITH_TGA_LOADER_
	#undef _IRR_COMPILE_WITH_WAL_LOADER_
	#undef _IRR_COMPILE_WITH_BMP_WRITER_
	#undef _IRR_COMPILE_WITH_JPG_WRITER_
	#undef _IRR_COMPILE_WITH_PCX_WRITER_
	#undef _IRR_COMPILE_WITH_PNG_WRITER_
	#undef _IRR_COMPILE_WITH_PPM_WRITER_
	#undef _IRR_COMPILE_WITH_PSD_WRITER_
	#undef _IRR_COMPILE_WITH_TGA_WRITER_

#endif

#ifndef _IRR_WINDOWS_API_
	#undef _IRR_WCHAR_FILESYSTEM
#endif

#if defined(__sparc__) || defined(__sun__)
#define __BIG_ENDIAN__
#endif

#if defined(_IRR_SOLARIS_PLATFORM_)
	#undef _IRR_COMPILE_WITH_JOYSTICK_EVENTS_
#endif

//! Define __IRR_HAS_S64 if the irr::s64 type should be enable (needs long long, available on most platforms, but not part of ISO C++ 98)
#define __IRR_HAS_S64
#ifdef NO__IRR_HAS_S64
#undef __IRR_HAS_S64
#endif

#if defined(__BORLANDC__)
	#include <tchar.h>

	// Borland 5.5.1 does not have _strcmpi defined
	#if __BORLANDC__ == 0x551
	//    #define _strcmpi strcmpi
		#undef _tfinddata_t
		#undef _tfindfirst
		#undef _tfindnext

		#define _tfinddata_t __tfinddata_t
		#define _tfindfirst  __tfindfirst
		#define _tfindnext   __tfindnext
		typedef long intptr_t;
	#endif

#endif

#ifdef _DEBUG
	//! A few attributes are written in CSceneManager when _IRR_SCENEMANAGER_DEBUG is enabled
	// NOTE: Those attributes were used always until 1.8.0 and became a global define for 1.8.1
	// which is only enabled in debug because it had a large (sometimes >5%) impact on speed.
	// A better solution in the long run is to break the interface and remove _all_ attribute
	// access in functions like CSceneManager::drawAll and instead put that information in some
	// own struct/class or in CSceneManager.
	// See http://irrlicht.sourceforge.net/forum/viewtopic.php?f=2&t=48211 for the discussion.
	#define _IRR_SCENEMANAGER_DEBUG
	#ifdef NO_IRR_SCENEMANAGER_DEBUG
		#undef _IRR_SCENEMANAGER_DEBUG
	#endif
#endif

#endif // __IRR_COMPILE_CONFIG_H_INCLUDED__

