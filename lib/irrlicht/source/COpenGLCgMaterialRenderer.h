// Copyright (C) 2012-2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OPENGL_CG_MATERIAL_RENDERER_H_INCLUDED__
#define __C_OPENGL_CG_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#if defined(_IRR_COMPILE_WITH_OPENGL_) && defined(_IRR_COMPILE_WITH_CG_)

#ifdef _IRR_WINDOWS_API_
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
	#include <GL/gl.h>
	#include "glext.h"
#else
#if defined(_IRR_OPENGL_USE_EXTPOINTER_)
	#define GL_GLEXT_LEGACY 1
#else
	#define GL_GLEXT_PROTOTYPES 1
#endif
#if defined(_IRR_OSX_PLATFORM_)
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif
#if defined(_IRR_OPENGL_USE_EXTPOINTER_)
	#include "glext.h"
#endif
#endif

#include "CCgMaterialRenderer.h"
#include "Cg/cgGL.h"

#ifdef _MSC_VER
	#pragma comment(lib, "cgGL.lib")
#endif

namespace irr
{
namespace video
{

class COpenGLDriver;
class IShaderConstantSetCallBack;

class COpenGLCgUniformSampler2D : public CCgUniform
{
public:
	COpenGLCgUniformSampler2D(const CGparameter& parameter, bool global);

	void update(const void* data, const SMaterial& material) const;
};

class COpenGLCgMaterialRenderer : public CCgMaterialRenderer
{
public:
	COpenGLCgMaterialRenderer(COpenGLDriver* driver, s32& materialType,
		const c8* vertexProgram = 0, const c8* vertexEntry = "main",
		E_VERTEX_SHADER_TYPE vertexProfile = video::EVST_VS_1_1,
		const c8* fragmentProgram = 0, const c8* fragmentEntry = "main",
		E_PIXEL_SHADER_TYPE fragmentProfile = video::EPST_PS_1_1,
		const c8* geometryProgram = 0, const c8* geometryEntry = "main",
		E_GEOMETRY_SHADER_TYPE geometryProfile = video::EGST_GS_4_0,
		scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
		scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
		u32 vertices = 0, IShaderConstantSetCallBack* callback = 0,
		IMaterialRenderer* baseMaterial = 0, s32 userData = 0);

	virtual ~COpenGLCgMaterialRenderer();

	virtual void OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates, IMaterialRendererServices* services);
	virtual bool OnRender(IMaterialRendererServices* services, E_VERTEX_TYPE vtxtype);
	virtual void OnUnsetMaterial();

	virtual void setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates);
	virtual IVideoDriver* getVideoDriver();

protected:
	void init(s32& materialType,
		const c8* vertexProgram = 0, const c8* vertexEntry = "main",
		E_VERTEX_SHADER_TYPE vertexProfile = video::EVST_VS_1_1,
		const c8* fragmentProgram = 0, const c8* fragmentEntry = "main",
		E_PIXEL_SHADER_TYPE fragmentProfile = video::EPST_PS_1_1,
		const c8* geometryProgram = 0, const c8* geometryEntry = "main",
		E_GEOMETRY_SHADER_TYPE geometryProfile = video::EGST_GS_4_0,
		scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
		scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
		u32 vertices = 0);

	COpenGLDriver* Driver;
};

}
}

#endif
#endif

