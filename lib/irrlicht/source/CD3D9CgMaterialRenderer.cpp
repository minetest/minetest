// Copyright (C) 2012-2012 Patryk Nadrowski
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#if defined(_IRR_COMPILE_WITH_DIRECT3D_9_) && defined(_IRR_COMPILE_WITH_CG_)

#include "CD3D9CgMaterialRenderer.h"
#include "CD3D9Driver.h"
#include "CD3D9Texture.h"

namespace irr
{
namespace video
{

CD3D9CgUniformSampler2D::CD3D9CgUniformSampler2D(const CGparameter& parameter, bool global) : CCgUniform(parameter, global)
{
	Type = CG_SAMPLER2D;
}

void CD3D9CgUniformSampler2D::update(const void* data, const SMaterial& material) const
{
	s32* Data = (s32*)data;
	s32 LayerID = *Data;

	if (material.TextureLayer[LayerID].Texture)
	{
		IDirect3DBaseTexture9* Texture = reinterpret_cast<irr::video::CD3D9Texture*>(material.TextureLayer[LayerID].Texture)->getDX9Texture();

		cgD3D9SetTextureParameter(Parameter, Texture);
	}
}

CD3D9CgMaterialRenderer::CD3D9CgMaterialRenderer(CD3D9Driver* driver, s32& materialType,
	const c8* vertexProgram, const c8* vertexEntry, E_VERTEX_SHADER_TYPE vertexProfile,
	const c8* fragmentProgram, const c8* fragmentEntry, E_PIXEL_SHADER_TYPE fragmentProfile,
	const c8* geometryProgram, const c8* geometryEntry, E_GEOMETRY_SHADER_TYPE geometryProfile,
	scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType, u32 vertices,
	IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData) :
	Driver(driver), CCgMaterialRenderer(callback, baseMaterial, userData)
{
	#ifdef _DEBUG
	setDebugName("CD3D9CgMaterialRenderer");
	#endif

	init(materialType, vertexProgram, vertexEntry, vertexProfile, fragmentProgram, fragmentEntry, fragmentProfile,
		geometryProgram, geometryEntry, geometryProfile, inType, outType, vertices);
}

CD3D9CgMaterialRenderer::~CD3D9CgMaterialRenderer()
{
	if (VertexProgram)
	{
		cgD3D9UnloadProgram(VertexProgram);
		cgDestroyProgram(VertexProgram);
	}
	if (FragmentProgram)
	{
		cgD3D9UnloadProgram(FragmentProgram);
		cgDestroyProgram(FragmentProgram);
	}
	/*if (GeometryProgram)
	{
		cgD3D9UnloadProgram(GeometryProgram);
		cgDestroyProgram(GeometryProgram);
	}*/
}

void CD3D9CgMaterialRenderer::OnSetMaterial(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates, IMaterialRendererServices* services)
{
	Material = material;

	if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
	{
		if (VertexProgram)
			cgD3D9BindProgram(VertexProgram);

		if (FragmentProgram)
			cgD3D9BindProgram(FragmentProgram);

		/*if (GeometryProgram)
			cgD3D9BindProgram(GeometryProgram);*/

		if (BaseMaterial)
			BaseMaterial->OnSetMaterial(material, material, true, this);
	}

	if (CallBack)
		CallBack->OnSetMaterial(material);

	Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

bool CD3D9CgMaterialRenderer::OnRender(IMaterialRendererServices* services, E_VERTEX_TYPE vtxtype)
{
	if (CallBack && (VertexProgram || FragmentProgram || GeometryProgram))
		CallBack->OnSetConstants(this, UserData);

	return true;
}

void CD3D9CgMaterialRenderer::OnUnsetMaterial()
{
	if (VertexProgram)
		cgD3D9UnbindProgram(VertexProgram);
	if (FragmentProgram)
		cgD3D9UnbindProgram(FragmentProgram);
	/*if (GeometryProgram)
		cgD3D9UnbindProgram(GeometryProgram);*/

	if (BaseMaterial)
		BaseMaterial->OnUnsetMaterial();

	Material = IdentityMaterial;;
}

void CD3D9CgMaterialRenderer::setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial, bool resetAllRenderstates)
{
	Driver->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

IVideoDriver* CD3D9CgMaterialRenderer::getVideoDriver()
{
	return Driver;
}

void CD3D9CgMaterialRenderer::init(s32& materialType,
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
		VertexProfile = cgD3D9GetLatestVertexProfile();

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
			cgD3D9LoadProgram(VertexProgram, 0, 0);
	}

	if (fragmentProgram)
	{
		FragmentProfile = cgD3D9GetLatestPixelProfile();

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
			cgD3D9LoadProgram(FragmentProgram, 0, 0);
	}

	/*if (geometryProgram)
	{
		GeometryProfile = cgD3D9GetLatestGeometryProfile();

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
			cgD3D9LoadProgram(GeometryProgram, 0, 0);
	}*/

	getUniformList();

	// create D3D9 specifics sampler uniforms.
	for(unsigned int i = 0; i < UniformInfo.size(); ++i)
	{
		if (UniformInfo[i]->getType() == CG_SAMPLER2D)
		{
			bool IsGlobal = true;

			if (UniformInfo[i]->getSpace() == CG_PROGRAM)
				IsGlobal = false;

			CCgUniform* Uniform = new CD3D9CgUniformSampler2D(UniformInfo[i]->getParameter(), IsGlobal);
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
