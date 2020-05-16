// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D8_SHADER_MATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D8_SHADER_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_API_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_
#include <d3d8.h>
#include <d3dx8core.h>

#include "IMaterialRenderer.h"
#include "S3DVertex.h"

namespace irr
{
namespace video
{

class IVideoDriver;
class IShaderConstantSetCallBack;
class IMaterialRenderer;

//! Class for using vertex and pixel shaders with D3D8
class CD3D8ShaderMaterialRenderer : public IMaterialRenderer
{
public:

	//! Public constructor
	CD3D8ShaderMaterialRenderer(IDirect3DDevice8* d3ddev, video::IVideoDriver* driver,
		s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData);

	//! Destructor
	~CD3D8ShaderMaterialRenderer();

	virtual void OnSetMaterial(const video::SMaterial& material, const video::SMaterial& lastMaterial,
		bool resetAllRenderstates, video::IMaterialRendererServices* services);

	virtual void OnUnsetMaterial();

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	//! Returns if the material is transparent.
	virtual bool isTransparent() const;

protected:

	//! constructor only for use by derived classes who want to
	//! create a fall back material for example.
	CD3D8ShaderMaterialRenderer(IDirect3DDevice8* d3ddev,
								video::IVideoDriver* driver,
								IShaderConstantSetCallBack* callback,
								IMaterialRenderer* baseMaterial, s32 userData=0);

	void init(s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram,
		E_VERTEX_TYPE type);
	bool createPixelShader(const c8* pxsh);
	bool createVertexShader(const char* vtxsh, E_VERTEX_TYPE type);

	IDirect3DDevice8* pID3DDevice;
	video::IVideoDriver* Driver;
	IShaderConstantSetCallBack* CallBack;
	IMaterialRenderer* BaseMaterial;

	DWORD VertexShader;
	DWORD OldVertexShader;
	DWORD PixelShader;
	s32 UserData;
};


} // end namespace video
} // end namespace irr

#endif
#endif
#endif

