// Copyright (C) 2008 Christian Stehno
// Heavily based on the OpenGL driver implemented by Nikolaus Gebhardt
// 2017 modified by Michael Zeilfelder (unifying extension handlers)
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "COGLESExtensionHandler.h"

#ifdef _IRR_COMPILE_WITH_OGLES1_

#include "irrString.h"
#include "SMaterial.h"
#include "fast_atof.h"

#if defined(_IRR_COMPILE_WITH_WINDOWS_DEVICE_)
#include <EGL/egl.h>
#else
#include <GLES/egl.h>
#endif

namespace irr
{
namespace video
{

COGLES1ExtensionHandler::COGLES1ExtensionHandler() :
		COGLESCoreExtensionHandler(),
		MaxUserClipPlanes(0), MaxLights(0), pGlBlendEquationOES(0), pGlBlendFuncSeparateOES(0),
		pGlBindFramebufferOES(0), pGlDeleteFramebuffersOES(0),
		pGlGenFramebuffersOES(0), pGlCheckFramebufferStatusOES(0),
		pGlFramebufferTexture2DOES(0), pGlGenerateMipmapOES(0)
{
}

void COGLES1ExtensionHandler::initExtensions()
{
	getGLVersion();

	if (Version >= 100)
		os::Printer::log("OpenGL ES driver version is 1.1.", ELL_INFORMATION);
	else
		os::Printer::log("OpenGL ES driver version is 1.0.", ELL_WARNING);

	getGLExtensions();

	GLint val = 0;

	if (Version > 100 || FeatureAvailable[IRR_GL_IMG_user_clip_plane]) {
		glGetIntegerv(GL_MAX_CLIP_PLANES, &val);
		MaxUserClipPlanes = static_cast<u8>(val);
	}

	glGetIntegerv(GL_MAX_LIGHTS, &val);
	MaxLights = static_cast<u8>(val);

	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &val);
	Feature.MaxTextureUnits = static_cast<u8>(val);

#ifdef GL_EXT_texture_filter_anisotropic
	if (FeatureAvailable[IRR_GL_EXT_texture_filter_anisotropic]) {
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
		MaxAnisotropy = static_cast<u8>(val);
	}
#endif
#ifdef GL_MAX_ELEMENTS_INDICES
	glGetIntegerv(GL_MAX_ELEMENTS_INDICES, &val);
	MaxIndices = val;
#endif
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
	MaxTextureSize = static_cast<u32>(val);
#ifdef GL_EXT_texture_lod_bias
	if (FeatureAvailable[IRR_GL_EXT_texture_lod_bias])
		glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &MaxTextureLODBias);
#endif
	glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, DimAliasedLine);
	glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, DimAliasedPoint);

	Feature.MaxTextureUnits = core::min_(Feature.MaxTextureUnits, static_cast<u8>(MATERIAL_MAX_TEXTURES));
	Feature.ColorAttachment = 1;

	pGlBlendEquationOES = (PFNGLBLENDEQUATIONOESPROC)eglGetProcAddress("glBlendEquationOES");
	pGlBlendFuncSeparateOES = (PFNGLBLENDFUNCSEPARATEOESPROC)eglGetProcAddress("glBlendFuncSeparateOES");
	pGlBindFramebufferOES = (PFNGLBINDFRAMEBUFFEROESPROC)eglGetProcAddress("glBindFramebufferOES");
	pGlDeleteFramebuffersOES = (PFNGLDELETEFRAMEBUFFERSOESPROC)eglGetProcAddress("glDeleteFramebuffersOES");
	pGlGenFramebuffersOES = (PFNGLGENFRAMEBUFFERSOESPROC)eglGetProcAddress("glGenFramebuffersOES");
	pGlCheckFramebufferStatusOES = (PFNGLCHECKFRAMEBUFFERSTATUSOESPROC)eglGetProcAddress("glCheckFramebufferStatusOES");
	pGlFramebufferTexture2DOES = (PFNGLFRAMEBUFFERTEXTURE2DOESPROC)eglGetProcAddress("glFramebufferTexture2DOES");
	pGlGenerateMipmapOES = (PFNGLGENERATEMIPMAPOESPROC)eglGetProcAddress("glGenerateMipmapOES");
}

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_OGLES2_
