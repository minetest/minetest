// Copyright (C) 2008 Christian Stehno
// Heavily based on the OpenGL driver implemented by Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#pragma once

#ifdef _IRR_COMPILE_WITH_OGLES1_

#include "EDriverFeatures.h"
#include "irrTypes.h"
#include "os.h"

#include "COGLESCommon.h"

#include "COGLESCoreExtensionHandler.h"

namespace irr
{
namespace video
{

class COGLES1ExtensionHandler : public COGLESCoreExtensionHandler
{
public:
	COGLES1ExtensionHandler();

	void initExtensions();

	bool queryFeature(video::E_VIDEO_DRIVER_FEATURE feature) const
	{
		switch (feature) {
		case EVDF_RENDER_TO_TARGET:
		case EVDF_HARDWARE_TL:
		case EVDF_MULTITEXTURE:
		case EVDF_BILINEAR_FILTER:
		case EVDF_MIP_MAP:
		case EVDF_TEXTURE_NSQUARE:
		case EVDF_STENCIL_BUFFER:
		case EVDF_ALPHA_TO_COVERAGE:
		case EVDF_COLOR_MASK:
		case EVDF_POLYGON_OFFSET:
		case EVDF_TEXTURE_MATRIX:
			return true;
		case EVDF_TEXTURE_NPOT:
			return FeatureAvailable[IRR_GL_APPLE_texture_2D_limited_npot] || FeatureAvailable[IRR_GL_OES_texture_npot];
		case EVDF_MIP_MAP_AUTO_UPDATE:
			return Version > 100;
		case EVDF_BLEND_OPERATIONS:
			return FeatureAvailable[IRR_GL_OES_blend_subtract];
		case EVDF_BLEND_SEPARATE:
			return FeatureAvailable[IRR_GL_OES_blend_func_separate];
		case EVDF_FRAMEBUFFER_OBJECT:
			return FeatureAvailable[IRR_GL_OES_framebuffer_object];
		case EVDF_VERTEX_BUFFER_OBJECT:
			return Version > 100;
		default:
			return true;
		};
	}

	inline void irrGlActiveTexture(GLenum texture)
	{
		glActiveTexture(texture);
	}

	inline void irrGlCompressedTexImage2D(GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border,
			GLsizei imageSize, const void *data)
	{
		glCompressedTexImage2D(target, level, internalformat, width, height, border, imageSize, data);
	}

	inline void irrGlCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
			GLenum format, GLsizei imageSize, const void *data)
	{
		glCompressedTexSubImage2D(target, level, xoffset, yoffset, width, height, format, imageSize, data);
	}

	inline void irrGlUseProgram(GLuint prog)
	{
	}

	inline void irrGlBindFramebuffer(GLenum target, GLuint framebuffer)
	{
		if (pGlBindFramebufferOES)
			pGlBindFramebufferOES(target, framebuffer);
	}

	inline void irrGlDeleteFramebuffers(GLsizei n, const GLuint *framebuffers)
	{
		if (pGlDeleteFramebuffersOES)
			pGlDeleteFramebuffersOES(n, framebuffers);
	}

	inline void irrGlGenFramebuffers(GLsizei n, GLuint *framebuffers)
	{
		if (pGlGenFramebuffersOES)
			pGlGenFramebuffersOES(n, framebuffers);
	}

	inline GLenum irrGlCheckFramebufferStatus(GLenum target)
	{
		if (pGlCheckFramebufferStatusOES)
			return pGlCheckFramebufferStatusOES(target);
		else
			return 0;
	}

	inline void irrGlFramebufferTexture2D(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level)
	{
		if (pGlFramebufferTexture2DOES)
			pGlFramebufferTexture2DOES(target, attachment, textarget, texture, level);
	}

	inline void irrGlGenerateMipmap(GLenum target)
	{
		if (pGlGenerateMipmapOES)
			pGlGenerateMipmapOES(target);
	}

	inline void irrGlActiveStencilFace(GLenum face)
	{
	}

	inline void irrGlDrawBuffer(GLenum mode)
	{
	}

	inline void irrGlDrawBuffers(GLsizei n, const GLenum *bufs)
	{
	}

	inline void irrGlBlendFuncSeparate(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
	{
		if (pGlBlendFuncSeparateOES)
			pGlBlendFuncSeparateOES(srcRGB, dstRGB, srcAlpha, dstAlpha);
	}

	inline void irrGlBlendEquation(GLenum mode)
	{
		if (pGlBlendEquationOES)
			pGlBlendEquationOES(mode);
	}

	inline void irrGlEnableIndexed(GLenum target, GLuint index)
	{
	}

	inline void irrGlDisableIndexed(GLenum target, GLuint index)
	{
	}

	inline void irrGlColorMaskIndexed(GLuint buf, GLboolean r, GLboolean g, GLboolean b, GLboolean a)
	{
	}

	inline void irrGlBlendFuncIndexed(GLuint buf, GLenum src, GLenum dst)
	{
	}

	inline void irrGlBlendFuncSeparateIndexed(GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha)
	{
	}

	inline void irrGlBlendEquationIndexed(GLuint buf, GLenum mode)
	{
	}

	inline void irrGlBlendEquationSeparateIndexed(GLuint buf, GLenum modeRGB, GLenum modeAlpha)
	{
	}

protected:
	u8 MaxUserClipPlanes;
	u8 MaxLights;

	PFNGLBLENDEQUATIONOESPROC pGlBlendEquationOES;
	PFNGLBLENDFUNCSEPARATEOESPROC pGlBlendFuncSeparateOES;
	PFNGLBINDFRAMEBUFFEROESPROC pGlBindFramebufferOES;
	PFNGLDELETEFRAMEBUFFERSOESPROC pGlDeleteFramebuffersOES;
	PFNGLGENFRAMEBUFFERSOESPROC pGlGenFramebuffersOES;
	PFNGLCHECKFRAMEBUFFERSTATUSOESPROC pGlCheckFramebufferStatusOES;
	PFNGLFRAMEBUFFERTEXTURE2DOESPROC pGlFramebufferTexture2DOES;
	PFNGLGENERATEMIPMAPOESPROC pGlGenerateMipmapOES;
};

}
}

#endif
