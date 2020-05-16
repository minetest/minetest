// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CD3D8ShaderMaterialRenderer.h"

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_
#include <d3d8.h>
#include <d3dx8core.h>
#pragma comment (lib, "d3dx8.lib")

#include "IShaderConstantSetCallBack.h"
#include "IMaterialRendererServices.h"
#include "IVideoDriver.h"
#include "os.h"

#ifndef _IRR_D3D_NO_SHADER_DEBUGGING
#include <stdio.h>
#endif

namespace irr
{
namespace video
{

//! Public constructor
CD3D8ShaderMaterialRenderer::CD3D8ShaderMaterialRenderer(IDirect3DDevice8* d3ddev, video::IVideoDriver* driver,
		s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData)
: pID3DDevice(d3ddev), Driver(driver), CallBack(callback), BaseMaterial(baseMaterial),
	VertexShader(0), OldVertexShader(0), PixelShader(0), UserData(userData)
{

	#ifdef _DEBUG
	setDebugName("CD3D8ShaderMaterialRenderer");
	#endif

	if (BaseMaterial)
		BaseMaterial->grab();

	if (CallBack)
		CallBack->grab();

	init(outMaterialTypeNr, vertexShaderProgram, pixelShaderProgram, EVT_STANDARD);
}

//! constructor only for use by derived classes who want to
//! create a fall back material for example.
CD3D8ShaderMaterialRenderer::CD3D8ShaderMaterialRenderer(IDirect3DDevice8* d3ddev,
														 video::IVideoDriver* driver,
														 IShaderConstantSetCallBack* callback,
														 IMaterialRenderer* baseMaterial,
														 s32 userData)
: pID3DDevice(d3ddev), Driver(driver), BaseMaterial(baseMaterial), CallBack(callback),
	VertexShader(0), PixelShader(0), UserData(userData)
{
	if (BaseMaterial)
		BaseMaterial->grab();

	if (CallBack)
		CallBack->grab();
}



//! Destructor
CD3D8ShaderMaterialRenderer::~CD3D8ShaderMaterialRenderer()
{
	if (CallBack)
		CallBack->drop();

	if (VertexShader)
		pID3DDevice->DeleteVertexShader(VertexShader);

	if (PixelShader)
		pID3DDevice->DeletePixelShader(PixelShader);

	if (BaseMaterial)
		BaseMaterial->drop ();
}


void CD3D8ShaderMaterialRenderer::init(s32& outMaterialTypeNr, const c8* vertexShaderProgram,
									   const c8* pixelShaderProgram, E_VERTEX_TYPE type)
{
	outMaterialTypeNr = -1;

	// create vertex shader
	if (!createVertexShader(vertexShaderProgram, type))
		return;

	// create pixel shader
	if (!createPixelShader(pixelShaderProgram))
		return;

	// register myself as new material
	outMaterialTypeNr = Driver->addMaterialRenderer(this);
}


bool CD3D8ShaderMaterialRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
{
	// call callback to set shader constants
	if (CallBack && (VertexShader || PixelShader))
		CallBack->OnSetConstants(service, UserData);

	return true;
}


void CD3D8ShaderMaterialRenderer::OnSetMaterial(const video::SMaterial& material, const video::SMaterial& lastMaterial,
	bool resetAllRenderstates, video::IMaterialRendererServices* services)
{
	if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
	{
		if (VertexShader)
		{
			// We do not need to save and reset the old vertex shader, because
			// in D3D8, this is mixed up with the fvf, and this is set by the driver
			// every time.
			//pID3DDevice->GetVertexShader(&OldVertexShader);

			// set new vertex shader
			if (FAILED(pID3DDevice->SetVertexShader(VertexShader)))
				os::Printer::log("Could not set vertex shader.", ELL_ERROR);
		}

		// set new pixel shader
		if (PixelShader)
		{
			if (FAILED(pID3DDevice->SetPixelShader(PixelShader)))
				os::Printer::log("Could not set pixel shader.", ELL_ERROR);
		}

		if (BaseMaterial)
			BaseMaterial->OnSetMaterial(material, material, true, services);
	}

	//let callback know used material
	if (CallBack)
		CallBack->OnSetMaterial(material);

	services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}

void CD3D8ShaderMaterialRenderer::OnUnsetMaterial()
{
	// We do not need to save and reset the old vertex shader, because
	// in D3D8, this is mixed up with the fvf, and this is set by the driver
	// every time.
	//	if (VertexShader)
	//		pID3DDevice->SetVertexShader(OldVertexShader);

	if (PixelShader)
		pID3DDevice->SetPixelShader(0);

	if (BaseMaterial)
		BaseMaterial->OnUnsetMaterial();
}


//! Returns if the material is transparent. The scene managment needs to know this
//! for being able to sort the materials by opaque and transparent.
bool CD3D8ShaderMaterialRenderer::isTransparent()  const
{
	return BaseMaterial ? BaseMaterial->isTransparent() : false;
}

bool CD3D8ShaderMaterialRenderer::createPixelShader(const c8* pxsh)
{
	if (!pxsh)
		return true;

#if defined( _IRR_XBOX_PLATFORM_)
	return false;
#else
	// compile shader

	LPD3DXBUFFER code = 0;
	LPD3DXBUFFER errors = 0;

	#ifdef _IRR_D3D_NO_SHADER_DEBUGGING

		// compile shader without debug info
		D3DXAssembleShader(pxsh, (UINT)strlen(pxsh), 0, 0, &code, &errors);

	#else

		// compile shader and emitt some debug informations to
		// make it possible to debug the shader in visual studio

		static int irr_dbg_file_nr = 0;
		++irr_dbg_file_nr;
		char tmp[32];
		sprintf(tmp, "irr_d3d8_dbg_shader_%d.psh", irr_dbg_file_nr);

		FILE* f = fopen(tmp, "wb");
		fwrite(pxsh, strlen(pxsh), 1, f);
		fflush(f);
		fclose(f);

		D3DXAssembleShaderFromFile(tmp, D3DXASM_DEBUG, 0, &code, &errors);
	#endif
	if (errors)
	{
		// print out compilation errors.
		os::Printer::log("Pixel shader compilation failed:", ELL_ERROR);
		os::Printer::log((c8*)errors->GetBufferPointer(), ELL_ERROR);

		if (code)
			code->Release();

		errors->Release();
		return false;
	}

	if (FAILED(pID3DDevice->CreatePixelShader((DWORD*)code->GetBufferPointer(), &PixelShader)))
	{
		os::Printer::log("Could not create pixel shader.", ELL_ERROR);
		code->Release();
		return false;
	}

	code->Release();
	return true;
#endif

}



bool CD3D8ShaderMaterialRenderer::createVertexShader(const char* vtxsh, E_VERTEX_TYPE type)
{
	if (!vtxsh)
		return true;

	// compile shader
#if defined( _IRR_XBOX_PLATFORM_)
	return false;
#else

	LPD3DXBUFFER code = 0;
	LPD3DXBUFFER errors = 0;

	#ifdef _IRR_D3D_NO_SHADER_DEBUGGING

		// compile shader without debug info
		D3DXAssembleShader(vtxsh, (UINT)strlen(vtxsh), 0, 0, &code, &errors);

	#else

		// compile shader and emitt some debug informations to
		// make it possible to debug the shader in visual studio
		static int irr_dbg_file_nr = 0;
		++irr_dbg_file_nr;
		char tmp[32];
		sprintf(tmp, "irr_d3d8_dbg_shader_%d.vsh", irr_dbg_file_nr);

		FILE* f = fopen(tmp, "wb");
		fwrite(vtxsh, strlen(vtxsh), 1, f);
		fflush(f);
		fclose(f);

		D3DXAssembleShaderFromFile(tmp, D3DXASM_DEBUG, 0, &code, &errors);

	#endif


	if (errors)
	{
		// print out compilation errors.
		os::Printer::log("Vertex shader compilation failed:", ELL_ERROR);
		os::Printer::log((c8*)errors->GetBufferPointer(), ELL_ERROR);

		if (code)
			code->Release();

		errors->Release();
		return false;
	}

	DWORD* decl = 0;

	DWORD dwStdDecl[] =
	{
		D3DVSD_STREAM(0),
		D3DVSD_REG(0, D3DVSDT_FLOAT3),   // position 0
		D3DVSD_REG(1, D3DVSDT_FLOAT3),   // normal 1
		D3DVSD_REG(2, D3DVSDT_D3DCOLOR ),// color 2
		D3DVSD_REG(3, D3DVSDT_FLOAT2 ),  // tex1 3
		D3DVSD_REG(4, D3DVSDT_FLOAT2 ),  // tex2 4
		D3DVSD_END()
	};

	DWORD dwTngtDecl[] =
	{
		D3DVSD_STREAM(0),
		D3DVSD_REG(0 , D3DVSDT_FLOAT3),   // position 0
		D3DVSD_REG(1 , D3DVSDT_FLOAT3),   // normal 1
		D3DVSD_REG(2 , D3DVSDT_D3DCOLOR ),// color 2
		D3DVSD_REG(3 , D3DVSDT_FLOAT2 ),  // tex1 3
		D3DVSD_REG(4, D3DVSDT_FLOAT3 ),  // tangent 4
		D3DVSD_REG(5, D3DVSDT_FLOAT3 ),  // binormal 5
		D3DVSD_END()
	};

	if (type == EVT_TANGENTS)
		decl = dwTngtDecl;
	else
		decl = dwStdDecl;

	if (FAILED(pID3DDevice->CreateVertexShader(decl,
		(DWORD*)code->GetBufferPointer(), &VertexShader, 0)))
	{
		os::Printer::log("Could not create vertex shader.", ELL_ERROR);
		code->Release();
		return false;
	}

	code->Release();
	return true;
#endif
}



} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_8_

