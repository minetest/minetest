// Copyright (C) 2015 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#ifdef _IRR_COMPILE_WITH_OGLES2_

#if defined(_IRR_COMPILE_WITH_IOS_DEVICE_)
#include <OpenGLES/ES2/gl.h>
#include <OpenGLES/ES2/glext.h>
#elif defined(_IRR_COMPILE_WITH_ANDROID_DEVICE_)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglplatform.h>
#elif defined(_IRR_EMSCRIPTEN_PLATFORM_)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <EGL/eglplatform.h>
#else
#if defined(_IRR_OGLES2_USE_EXTPOINTER_)
	#define GL_GLEXT_PROTOTYPES 1
	#define GLX_GLXEXT_PROTOTYPES 1
#endif
#include <GLES2/gl2.h>
#include <EGL/eglplatform.h>
typedef char GLchar;
#if defined(_IRR_OGLES2_USE_EXTPOINTER_)
#include <GLES2/gl2ext.h>
#endif
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1;
#endif

// FBO definitions.

#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 1
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 2
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 3

// to check if this header is in the current compile unit (different GL implementation used different "GLCommon" headers in Irrlicht
#define IRR_COMPILE_GLES2_COMMON

namespace irr
{
namespace video
{

	// Forward declarations.

	class COpenGLCoreFeature;

	template <class TOpenGLDriver>
	class COpenGLCoreTexture;

	template <class TOpenGLDriver, class TOpenGLTexture>
	class COpenGLCoreRenderTarget;

	template <class TOpenGLDriver, class TOpenGLTexture>
	class COpenGLCoreCacheHandler;

	class COGLES2Driver;
	typedef COpenGLCoreTexture<COGLES2Driver> COGLES2Texture;
	typedef COpenGLCoreRenderTarget<COGLES2Driver, COGLES2Texture> COGLES2RenderTarget;
	typedef COpenGLCoreCacheHandler<COGLES2Driver, COGLES2Texture> COGLES2CacheHandler;

}
}

#endif
