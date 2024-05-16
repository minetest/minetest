// Copyright (C) 2015 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#ifdef _IRR_COMPILE_WITH_OGLES1_

#if defined(_IRR_COMPILE_WITH_IOS_DEVICE_)
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#elif defined(_IRR_OGLES1_USE_KHRONOS_API_HEADERS_)
#include <khronos-api/GLES/gl.h>
#include <EGL/eglplatform.h>
typedef char GLchar;
#else // or only when defined(_IRR_COMPILE_WITH_ANDROID_DEVICE_) ?
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/eglplatform.h>
#endif

#ifndef GL_BGRA
#define GL_BGRA 0x80E1;
#endif

// Blending definitions.

#if defined(GL_OES_blend_subtract)
#define GL_FUNC_ADD GL_FUNC_ADD_OES
#else
#define GL_FUNC_ADD 0
#endif

// FBO definitions.

#ifdef GL_OES_framebuffer_object
#define GL_NONE 0 // iOS has missing definition of GL_NONE_OES
#define GL_FRAMEBUFFER GL_FRAMEBUFFER_OES
#define GL_DEPTH_COMPONENT16 GL_DEPTH_COMPONENT16_OES
#define GL_COLOR_ATTACHMENT0 GL_COLOR_ATTACHMENT0_OES
#define GL_DEPTH_ATTACHMENT GL_DEPTH_ATTACHMENT_OES
#define GL_STENCIL_ATTACHMENT GL_STENCIL_ATTACHMENT_OES
#define GL_FRAMEBUFFER_COMPLETE GL_FRAMEBUFFER_COMPLETE_OES
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 1
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 2
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS GL_FRAMEBUFFER_INCOMPLETE_FORMATS_OES
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_OES
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES
#define GL_FRAMEBUFFER_UNSUPPORTED GL_FRAMEBUFFER_UNSUPPORTED_OES
#else
#define GL_NONE 0
#define GL_FRAMEBUFFER 0
#define GL_DEPTH_COMPONENT16 0
#define GL_COLOR_ATTACHMENT0 0
#define GL_DEPTH_ATTACHMENT 0
#define GL_STENCIL_ATTACHMENT 0
#define GL_FRAMEBUFFER_COMPLETE 0
#define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER 1
#define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER 2
#define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT 3
#define GL_FRAMEBUFFER_INCOMPLETE_FORMATS 4
#define GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS 5
#define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT 6
#define GL_FRAMEBUFFER_UNSUPPORTED 7
#endif

#define GL_DEPTH_COMPONENT 0x1902

// Texture definitions.

#ifdef GL_OES_texture_cube_map
#define GL_TEXTURE_CUBE_MAP GL_TEXTURE_CUBE_MAP_OES
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X GL_TEXTURE_CUBE_MAP_POSITIVE_X_OES
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X GL_TEXTURE_CUBE_MAP_NEGATIVE_X_OES
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y GL_TEXTURE_CUBE_MAP_POSITIVE_Y_OES
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_OES
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z GL_TEXTURE_CUBE_MAP_POSITIVE_Z_OES
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_OES
#else
#define GL_TEXTURE_CUBE_MAP 0
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X 0
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X 0
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y 0
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y 0
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z 0
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z 0
#endif

// to check if this header is in the current compile unit (different GL implementation used different "GLCommon" headers in Irrlicht
#define IRR_COMPILE_GLES_COMMON

// macro used with COGLES1Driver
#define TEST_GL_ERROR(cls) (cls)->testGLError(__LINE__)

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

class COGLES1Driver;
typedef COpenGLCoreTexture<COGLES1Driver> COGLES1Texture;
typedef COpenGLCoreRenderTarget<COGLES1Driver, COGLES1Texture> COGLES1RenderTarget;
typedef COpenGLCoreCacheHandler<COGLES1Driver, COGLES1Texture> COGLES1CacheHandler;

}
}

#endif
