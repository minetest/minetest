// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OPENGL_SHADER_MATERIAL_RENDERER_H_INCLUDED__
#define __C_OPENGL_SHADER_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_OPENGL_

#if defined(_IRR_OPENGL_USE_EXTPOINTER_)
	#define GL_GLEXT_LEGACY 1
#else
	#define GL_GLEXT_PROTOTYPES 1
#endif
#ifdef _IRR_WINDOWS_API_
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <GL/gl.h>
#elif defined(_IRR_OSX_PLATFORM_)
	#include <OpenGL/gl.h>
#elif defined(_IRR_COMPILE_WITH_SDL_DEVICE_)
	#define NO_SDL_GLEXT
	#include <SDL/SDL_video.h>
	#include <SDL/SDL_opengl.h>
#else
	#include <GL/gl.h>
#endif

#include "IMaterialRenderer.h"

namespace irr
{
namespace video
{

class COpenGLDriver;
class IShaderConstantSetCallBack;
class IMaterialRenderer;

//! Class for using vertex and pixel shaders with OpenGL
class COpenGLShaderMaterialRenderer : public IMaterialRenderer
{
public:

	//! Constructor
	COpenGLShaderMaterialRenderer(COpenGLDriver* driver,
		s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData);

	//! Destructor
	virtual ~COpenGLShaderMaterialRenderer();

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial,
		bool resetAllRenderstates, IMaterialRendererServices* services);

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	virtual void OnUnsetMaterial();

	//! Returns if the material is transparent.
	virtual bool isTransparent() const;

protected:

	//! constructor only for use by derived classes who want to
	//! create a fall back material for example.
	COpenGLShaderMaterialRenderer(COpenGLDriver* driver,
					IShaderConstantSetCallBack* callback,
					IMaterialRenderer* baseMaterial, s32 userData=0);

	// must not be called more than once!
	void init(s32& outMaterialTypeNr, const c8* vertexShaderProgram,
		const c8* pixelShaderProgram, E_VERTEX_TYPE type);

	bool createPixelShader(const c8* pxsh);
	bool createVertexShader(const c8* vtxsh);
	bool checkError(const irr::c8* type);

	COpenGLDriver* Driver;
	IShaderConstantSetCallBack* CallBack;
	IMaterialRenderer* BaseMaterial;

	GLuint VertexShader;
	// We have 4 values here, [0] is the non-fog version, the other three are
	// ARB_fog_linear, ARB_fog_exp, and ARB_fog_exp2 in that order
	core::array<GLuint> PixelShader;
	s32 UserData;
};


} // end namespace video
} // end namespace irr

#endif
#endif

