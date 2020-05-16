// Copyright (C) 2012-2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#if defined(_IRR_COMPILE_WITH_OPENGL_) && defined(_IRR_COMPILE_WITH_CG_)

#include "COpenGLCgMaterialRenderer.h"
#include "COpenGLDriver.h"
#include "COpenGLTexture.h"

namespace irr
{
namespace video
{

COpenGLCgUniformSampler2D::COpenGLCgUniformSampler2D(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_SAMPLER2D;
}

void COpenGLCgUniformSampler2D::update(const void* data, const SMaterial& material) const
{
	s32* Data = (s32*)data;
	s32 LayerID = *Data;

	if (material.TextureLayer[LayerID].Texture)
	{
		int TextureID = reinterpret_cast<COpenGLTexture*>(material.TextureLayer[LayerID].Texture)->getOpenGLTextureName();

		cgGLSetTextureParameter(Parameter, TextureID);
		cgGLEnableTextureParameter(Parameter);
	}
}

COpenGLCgMaterialRenderer::COpenGLCgMaterialRenderer(COpenGLDriver* driver, s32& materialType,
	const c8* vertexProgram, const c8* vertexEntry, E_VERTEX_SHADER_TYPE vertexProfile,
	const c8* fragmentProgram, const c8* fragmentEntry, E_PIXEL_SHADER_TYPE fragmentProfile,
	const c8* geometryProgram, const c8* geometryEntry, E_GEOMETRY_SHADER_TYPE geometryProfile,
	scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 vertices,
	IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData) :
	Driver(driver), CCgMaterialRenderer(callback, baseMaterial, userData)
{
	#ifdef _DEBUG
	setDebugName("COpenGLCgMaterialRenderer");
	#endif

	init(materialType, vertexProgram, vertexEntry, vertexProfile, fragmentProgram, fragmentEntry, fragmentProfile,
		geometryProgram, geometryEntry, geometryProfile, inType, outType, vertices);
}

COpenGLCgMaterialRenderer::~COpenGLCgMaterialRenderer()
{
	if (VertexProgram)
	{
		cgGLUnloadProgram(VertexProgram);
		cgDestroyProgram(VertexProgram);
	}
	if (FragmentProgram)
	{
		cgGLUnloadProgram(FragmentProgram);
		cgDestroyProgram(FragmentProgram);
	}
	if (GeometryProgram)
	{
		cgGLUnloadProgram(GeometryProgram);
		cgDestroyProgram(GeometryProgram);
	}
}

void COpenGLCgMaterialRenderer::OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates, IMaterialRendererServices* services)
{
	Material = material;

	if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
	{
		if (VertexProgram)
		{
			cgGLEnableProfile(VertexProfile);
			cgGLBindProgram(VertexProgram);
		}

		if (FragmentProgram)
		{
			cgGLEnableProfile(FragmentProfile);
			cgGLBindProgram(FragmentProgram);
		}

		if (GeometryProgram)
		{
			cgGLEnableProfile(GeometryProfile);
			cgGLBindProgram(GeometryProgram);
		}

		if (BaseMaterial)
			BaseMaterial->OnSetMaterial(material, material, true, this);
	}

	if (CallBack)
		CallBack->OnSetMaterial(material);

	for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		Driver->setActiveTexture(i, material.getTexture(i));

	Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

bool COpenGLCgMaterialRenderer::OnRender(IMaterialRendererServices* services, E_VERTEX_TYPE vtxtype)
{
	if (CallBack && (VertexProgram || FragmentProgram || GeometryProgram))
		CallBack->OnSetConstants(this, UserData);

	return true;
}

void COpenGLCgMaterialRenderer::OnUnsetMaterial()
{
	if (VertexProgram)
	{
		cgGLUnbindProgram(VertexProfile);
		cgGLDisableProfile(VertexProfile);
	}
	if (FragmentProgram)
	{
		cgGLUnbindProgram(FragmentProfile);
		cgGLDisableProfile(FragmentProfile);
	}
	if (GeometryProgram)
	{
		cgGLUnbindProgram(GeometryProfile);
		cgGLDisableProfile(GeometryProfile);
	}

	if (BaseMaterial)
		BaseMaterial->OnUnsetMaterial();

	Material = IdentityMaterial;;
}

void COpenGLCgMaterialRenderer::setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates)
{
	Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

IVideoDriver* COpenGLCgMaterialRenderer::getVideoDriver()
{
	return Driver;
}

void COpenGLCgMaterialRenderer::init(s32& materialType,
	const c8* vertexProgram, const c8* vertexEntry, E_VERTEX_SHADER_TYPE vertexProfile,
	const c8* fragmentProgram, const c8* fragmentEntry, E_PIXEL_SHADER_TYPE fragmentProfile,
	const c8* geometryProgram, const c8* geometryEntry, E_GEOMETRY_SHADER_TYPE geometryProfile,
	scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 vertices)
{
	bool Status = true;
	CGerror Error = CG_NO_ERROR;
	materialType = -1;

	// TODO: add profile selection

	if (vertexProgram)
	{
		VertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);

		if (VertexProfile)
			VertexProgram = cgCreateProgram(Driver->getCgContext(), CG_SOURCE, vertexProgram, VertexProfile, vertexEntry, 0);

		if (!VertexProgram)
		{
			Error = cgGetError();
			os::Printer::log("Cg vertex program failed to compile:", ELL_ERROR);
			os::Printer::log(cgGetLastListing(Driver->getCgContext()), ELL_ERROR);

			Status = false;
		}
		else
			cgGLLoadProgram(VertexProgram);
	}

	if (fragmentProgram)
	{
		FragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);

		if (FragmentProfile)
			FragmentProgram = cgCreateProgram(Driver->getCgContext(), CG_SOURCE, fragmentProgram, FragmentProfile, fragmentEntry, 0);

		if (!FragmentProgram)
		{
			Error = cgGetError();
			os::Printer::log("Cg fragment program failed to compile:", ELL_ERROR);
			os::Printer::log(cgGetLastListing(Driver->getCgContext()), ELL_ERROR);

			Status = false;
		}
		else
			cgGLLoadProgram(FragmentProgram);
	}

	if (geometryProgram)
	{
		GeometryProfile = cgGLGetLatestProfile(CG_GL_GEOMETRY);

		if (GeometryProfile)
			GeometryProgram = cgCreateProgram(Driver->getCgContext(), CG_SOURCE, geometryProgram, GeometryProfile, geometryEntry, 0);

		if (!GeometryProgram)
		{
			Error = cgGetError();
			os::Printer::log("Cg geometry program failed to compile:", ELL_ERROR);
			os::Printer::log(cgGetLastListing(Driver->getCgContext()), ELL_ERROR);

			Status = false;
		}
		else
			cgGLLoadProgram(GeometryProgram);
	}

	getUniformList();

	// create OpenGL specifics sampler uniforms.
	for (unsigned int i = 0; i < UniformInfo.size(); ++i)
	{
		if (UniformInfo[i]->getType() == CG_SAMPLER2D)
		{
			bool IsGlobal = true;

			if (UniformInfo[i]->getSpace() == CG_PROGRAM)
				IsGlobal = false;

			CCgUniform* Uniform = new COpenGLCgUniformSampler2D(UniformInfo[i]->getParameter(), IsGlobal);
			delete UniformInfo[i];
			UniformInfo[i] = Uniform;
		}
	}

	if (Status)
		materialType = Driver->addMaterialRenderer(this);
}

}
}

#endif
