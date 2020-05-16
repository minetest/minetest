// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

#include "CD3D9ShaderMaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"
#include "IMaterialRendererServices.h"
#include "IVideoDriver.h"
#include "os.h"
#include "irrString.h"

#ifndef _IRR_D3D_NO_SHADER_DEBUGGING
#include <stdio.h>
#endif


namespace irr
{
namespace video
{

//! Public constructor
CD3D9ShaderMaterialRenderer::CD3D9ShaderMaterialRenderer(IDirect3DDevice9* d3ddev, video::IVideoDriver* driver,
		s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData)
: pID3DDevice(d3ddev), Driver(driver), CallBack(callback), BaseMaterial(baseMaterial),
	VertexShader(0), OldVertexShader(0), PixelShader(0), UserData(userData)
{
	#ifdef _DEBUG
	setDebugName("CD3D9ShaderMaterialRenderer");
	#endif

	if (BaseMaterial)
		BaseMaterial->grab();

	if (CallBack)
		CallBack->grab();

	init(outMaterialTypeNr, vertexShaderProgram, pixelShaderProgram);
}


//! constructor only for use by derived classes who want to
//! create a fall back material for example.
CD3D9ShaderMaterialRenderer::CD3D9ShaderMaterialRenderer(IDirect3DDevice9* d3ddev,
			video::IVideoDriver* driver,
			IShaderConstantSetCallBack* callback,
			IMaterialRenderer* baseMaterial, s32 userData)
: pID3DDevice(d3ddev), Driver(driver), CallBack(callback), BaseMaterial(baseMaterial),
	VertexShader(0), OldVertexShader(0), PixelShader(0), UserData(userData)
{
	#ifdef _DEBUG
	setDebugName("CD3D9ShaderMaterialRenderer");
	#endif

	if (BaseMaterial)
		BaseMaterial->grab();

	if (CallBack)
		CallBack->grab();
}


void CD3D9ShaderMaterialRenderer::init(s32& outMaterialTypeNr,
		const c8* vertexShaderProgram, const c8* pixelShaderProgram)
{
	outMaterialTypeNr = -1;

	// create vertex shader
	if (!createVertexShader(vertexShaderProgram))
		return;

	// create pixel shader
	if (!createPixelShader(pixelShaderProgram))
		return;

	// register myself as new material
	outMaterialTypeNr = Driver->addMaterialRenderer(this);
}


//! Destructor
CD3D9ShaderMaterialRenderer::~CD3D9ShaderMaterialRenderer()
{
	if (CallBack)
		CallBack->drop();

	if (VertexShader)
		VertexShader->Release();

	if (PixelShader)
		PixelShader->Release();

	if (BaseMaterial)
		BaseMaterial->drop();
}


bool CD3D9ShaderMaterialRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
{
	// call callback to set shader constants
	if (CallBack && (VertexShader || PixelShader))
		CallBack->OnSetConstants(service, UserData);

	return true;
}


void CD3D9ShaderMaterialRenderer::OnSetMaterial(const video::SMaterial& material, const video::SMaterial& lastMaterial,
	bool resetAllRenderstates, video::IMaterialRendererServices* services)
{
	if (material.MaterialType != lastMaterial.MaterialType || resetAllRenderstates)
	{
		if (VertexShader)
		{
			// save old vertex shader
			pID3DDevice->GetVertexShader(&OldVertexShader);

			// set new vertex shader
			if (FAILED(pID3DDevice->SetVertexShader(VertexShader)))
				os::Printer::log("Could not set vertex shader.", ELL_WARNING);
		}

		// set new pixel shader
		if (PixelShader)
		{
			if (FAILED(pID3DDevice->SetPixelShader(PixelShader)))
				os::Printer::log("Could not set pixel shader.", ELL_WARNING);
		}

		if (BaseMaterial)
			BaseMaterial->OnSetMaterial(material, material, true, services);
	}

	//let callback know used material
	if (CallBack)
		CallBack->OnSetMaterial(material);

	services->setBasicRenderStates(material, lastMaterial, resetAllRenderstates);
}


void CD3D9ShaderMaterialRenderer::OnUnsetMaterial()
{
	if (VertexShader)
		pID3DDevice->SetVertexShader(OldVertexShader);

	if (PixelShader)
		pID3DDevice->SetPixelShader(0);

	if (BaseMaterial)
		BaseMaterial->OnUnsetMaterial();
}


//! Returns if the material is transparent. The scene managment needs to know this
//! for being able to sort the materials by opaque and transparent.
bool CD3D9ShaderMaterialRenderer::isTransparent() const
{
	return BaseMaterial ? BaseMaterial->isTransparent() : false;
}


bool CD3D9ShaderMaterialRenderer::createPixelShader(const c8* pxsh)
{
	if (!pxsh)
		return true;

	// compile shader

	LPD3DXBUFFER code = 0;
	LPD3DXBUFFER errors = 0;

	#ifdef _IRR_D3D_NO_SHADER_DEBUGGING

		// compile shader without debug info
		stubD3DXAssembleShader(pxsh, (UINT)strlen(pxsh), 0, 0, 0, &code, &errors);
	#else

		// compile shader and emitt some debug informations to
		// make it possible to debug the shader in visual studio

		static int irr_dbg_file_nr = 0;
		++irr_dbg_file_nr;
		char tmp[32];
		sprintf(tmp, "irr_d3d9_dbg_shader_%d.psh", irr_dbg_file_nr);

		FILE* f = fopen(tmp, "wb");
		fwrite(pxsh, strlen(pxsh), 1, f);
		fflush(f);
		fclose(f);

		stubD3DXAssembleShaderFromFile(tmp, 0, 0, D3DXSHADER_DEBUG, &code, &errors);

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
}


bool CD3D9ShaderMaterialRenderer::createVertexShader(const char* vtxsh)
{
	if (!vtxsh)
		return true;

	// compile shader

	LPD3DXBUFFER code = 0;
	LPD3DXBUFFER errors = 0;

	#ifdef _IRR_D3D_NO_SHADER_DEBUGGING

		// compile shader without debug info
		stubD3DXAssembleShader(vtxsh, (UINT)strlen(vtxsh), 0, 0, 0, &code, &errors);

	#else

		// compile shader and emitt some debug informations to
		// make it possible to debug the shader in visual studio

		static int irr_dbg_file_nr = 0;
		++irr_dbg_file_nr;
		char tmp[32];
		sprintf(tmp, "irr_d3d9_dbg_shader_%d.vsh", irr_dbg_file_nr);

		FILE* f = fopen(tmp, "wb");
		fwrite(vtxsh, strlen(vtxsh), 1, f);
		fflush(f);
		fclose(f);

        stubD3DXAssembleShaderFromFile(tmp, 0, 0, D3DXSHADER_DEBUG, &code, &errors);

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

	if (!code || FAILED(pID3DDevice->CreateVertexShader((DWORD*)code->GetBufferPointer(), &VertexShader)))
	{
		os::Printer::log("Could not create vertex shader.", ELL_ERROR);
		if (code)
			code->Release();
		return false;
	}

	code->Release();
	return true;
}


HRESULT CD3D9ShaderMaterialRenderer::stubD3DXAssembleShader(LPCSTR pSrcData,
		UINT SrcDataLen, CONST D3DXMACRO* pDefines,
		LPD3DXINCLUDE pInclude, DWORD Flags, LPD3DXBUFFER* ppShader,
		LPD3DXBUFFER* ppErrorMsgs)
{
	// Because Irrlicht needs to be able to start up even without installed d3d dlls, it
	// needs to load external d3d dlls manually. examples for the dlls are:
	// SDK              dll name     D3DX_SDK_VERSION
	// Summer 2004:     no dll        22
	// February 2005:	d3dx9_24.dll  24
	// April 2005:		d3dx9_25.dll  25
	// June 2005:		d3dx9_26.dll  26
	// August 2005:		d3dx9_27.dll  27
	// October 2005,
	// December 2005:	d3dx9_28.dll  28

	#if ( D3DX_SDK_VERSION < 24 )
		// directly link functions, old d3d sdks didn't try to load external dlls
		// when linking to the d3dx9.lib
		#ifdef _MSC_VER
		#pragma comment (lib, "d3dx9.lib")
		#endif

		// invoke static linked function
		return D3DXAssembleShader(pSrcData, SrcDataLen, pDefines, pInclude,
						  Flags, ppShader, ppErrorMsgs);
	#else
	{
		// try to load shader functions from the dll and print error if failed.

		// D3DXAssembleShader signature
		typedef HRESULT (WINAPI *AssembleShaderFunction)(LPCSTR pSrcData,  UINT SrcDataLen,
					CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude,
					DWORD Flags, LPD3DXBUFFER* ppShader,
					LPD3DXBUFFER* ppErrorMsgs);

		static bool LoadFailed = false;
		static AssembleShaderFunction pFn = 0;

		if (!pFn && !LoadFailed)
		{
			// try to load dll
			io::path strDllName = "d3dx9_";
			strDllName += (int)D3DX_SDK_VERSION;
			strDllName += ".dll";

			HMODULE hMod = LoadLibrary(strDllName.c_str());
			if (hMod)
				 pFn = (AssembleShaderFunction)GetProcAddress(hMod, "D3DXAssembleShader");

			if (!pFn)
			{
				LoadFailed = true;
				os::Printer::log("Could not load shader function D3DXAssembleShader from dll, shaders disabled",
					strDllName.c_str(), ELL_ERROR);
			}
		}

		if (pFn)
		{
			// call already loaded function
			return (*pFn)(pSrcData, SrcDataLen, pDefines, pInclude, Flags, ppShader, ppErrorMsgs);
		}
	}
	#endif // D3DX_SDK_VERSION < 24

	return 0;
}


HRESULT CD3D9ShaderMaterialRenderer::stubD3DXAssembleShaderFromFile(LPCSTR pSrcFile,
		        CONST D3DXMACRO* pDefines, LPD3DXINCLUDE  pInclude, DWORD Flags,
			LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs)
{
	// wondering what I'm doing here?
	// see comment in CD3D9ShaderMaterialRenderer::stubD3DXAssembleShader()

	#if ( D3DX_SDK_VERSION < 24 )
		// directly link functions, old d3d sdks didn't try to load external dlls
		// when linking to the d3dx9.lib
		#ifdef _MSC_VER
		#pragma comment (lib, "d3dx9.lib")
		#endif

		// invoke static linked function
		return D3DXAssembleShaderFromFileA(pSrcFile, pDefines, pInclude, Flags,
										  ppShader, ppErrorMsgs);
	#else
	{
		// try to load shader functions from the dll and print error if failed.

		// D3DXAssembleShaderFromFileA signature
		typedef HRESULT (WINAPI *AssembleShaderFromFileFunction)(LPCSTR pSrcFile,
				CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, DWORD Flags,
				LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs);

		static bool LoadFailed = false;
		static AssembleShaderFromFileFunction pFn = 0;

		if (!pFn && !LoadFailed)
		{
			// try to load dll
			io::path strDllName = "d3dx9_";
			strDllName += (int)D3DX_SDK_VERSION;
			strDllName += ".dll";

			HMODULE hMod = LoadLibrary(strDllName.c_str());
			if (hMod)
				 pFn = (AssembleShaderFromFileFunction)GetProcAddress(hMod, "D3DXAssembleShaderFromFileA");

			if (!pFn)
			{
				LoadFailed = true;
				os::Printer::log("Could not load shader function D3DXAssembleShaderFromFileA from dll, shaders disabled",
					strDllName.c_str(), ELL_ERROR);
			}
		}

		if (pFn)
		{
			// call already loaded function
			return (*pFn)(pSrcFile, pDefines, pInclude, Flags, ppShader, ppErrorMsgs);
		}
	}
	#endif // D3DX_SDK_VERSION < 24

	return 0;
}


HRESULT CD3D9ShaderMaterialRenderer::stubD3DXCompileShader(LPCSTR pSrcData, UINT SrcDataLen, CONST D3DXMACRO* pDefines,
				LPD3DXINCLUDE pInclude, LPCSTR pFunctionName,
				LPCSTR pProfile, DWORD Flags, LPD3DXBUFFER* ppShader,
				LPD3DXBUFFER* ppErrorMsgs, LPD3DXCONSTANTTABLE* ppConstantTable)
{
	// wondering what I'm doing here?
	// see comment in CD3D9ShaderMaterialRenderer::stubD3DXAssembleShader()

	#if ( D3DX_SDK_VERSION < 24 )
		// directly link functions, old d3d sdks didn't try to load external dlls
		// when linking to the d3dx9.lib
		#ifdef _MSC_VER
		#pragma comment (lib, "d3dx9.lib")
		#endif

		// invoke static linked function
		return D3DXCompileShader(pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pProfile, Flags, ppShader, ppErrorMsgs, ppConstantTable);
	#else
	{
		// try to load shader functions from the dll and print error if failed.

		// D3DXCompileShader
		typedef HRESULT (WINAPI *D3DXCompileShaderFunction)(LPCSTR pSrcData, UINT SrcDataLen, CONST D3DXMACRO* pDefines,
				LPD3DXINCLUDE pInclude, LPCSTR pFunctionName,
				LPCSTR pProfile, DWORD Flags, LPD3DXBUFFER* ppShader,
				LPD3DXBUFFER* ppErrorMsgs, LPD3DXCONSTANTTABLE* ppConstantTable);

		static bool LoadFailed = false;
		static D3DXCompileShaderFunction pFn = 0;

		if (!pFn && !LoadFailed)
		{
			// try to load dll
			io::path strDllName = "d3dx9_";
			strDllName += (int)D3DX_SDK_VERSION;
			strDllName += ".dll";

			HMODULE hMod = LoadLibrary(strDllName.c_str());
			if (hMod)
				 pFn = (D3DXCompileShaderFunction)GetProcAddress(hMod, "D3DXCompileShader");

			if (!pFn)
			{
				LoadFailed = true;
				os::Printer::log("Could not load shader function D3DXCompileShader from dll, shaders disabled",
					strDllName.c_str(), ELL_ERROR);
			}
		}

		if (pFn)
		{
			// call already loaded function
			return (*pFn)(pSrcData, SrcDataLen, pDefines, pInclude, pFunctionName, pProfile, Flags, ppShader, ppErrorMsgs, ppConstantTable);
		}
	}
	#endif // D3DX_SDK_VERSION < 24

	return 0;
}

HRESULT CD3D9ShaderMaterialRenderer::stubD3DXCompileShaderFromFile(LPCSTR pSrcFile, CONST D3DXMACRO* pDefines,
				LPD3DXINCLUDE pInclude, LPCSTR pFunctionName,
				LPCSTR pProfile, DWORD Flags, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs,
				LPD3DXCONSTANTTABLE* ppConstantTable)
{
	// wondering what I'm doing here?
	// see comment in CD3D9ShaderMaterialRenderer::stubD3DXAssembleShader()

	#if ( D3DX_SDK_VERSION < 24 )
		// directly link functions, old d3d sdks didn't try to load external dlls
		// when linking to the d3dx9.lib
		#ifdef _MSC_VER
		#pragma comment (lib, "d3dx9.lib")
		#endif

		// invoke static linked function
		return D3DXCompileShaderFromFileA(pSrcFile, pDefines, pInclude, pFunctionName, pProfile, Flags, ppShader, ppErrorMsgs, ppConstantTable);
	#else
	{
		// try to load shader functions from the dll and print error if failed.

		// D3DXCompileShaderFromFileA
		typedef HRESULT (WINAPI *D3DXCompileShaderFromFileFunction)(LPCSTR pSrcFile,
			CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude, LPCSTR pFunctionName,
			LPCSTR pProfile, DWORD Flags, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs,
			LPD3DXCONSTANTTABLE* ppConstantTable);

		static bool LoadFailed = false;
		static D3DXCompileShaderFromFileFunction pFn = 0;

		if (!pFn && !LoadFailed)
		{
			// try to load dll
			io::path strDllName = "d3dx9_";
			strDllName += (int)D3DX_SDK_VERSION;
			strDllName += ".dll";

			HMODULE hMod = LoadLibrary(strDllName.c_str());
			if (hMod)
				 pFn = (D3DXCompileShaderFromFileFunction)GetProcAddress(hMod, "D3DXCompileShaderFromFileA");

			if (!pFn)
			{
				LoadFailed = true;
				os::Printer::log("Could not load shader function D3DXCompileShaderFromFileA from dll, shaders disabled",
					strDllName.c_str(), ELL_ERROR);
			}
		}

		if (pFn)
		{
			// call already loaded function
			return (*pFn)(pSrcFile, pDefines, pInclude, pFunctionName, pProfile, Flags, ppShader, ppErrorMsgs, ppConstantTable);
		}
	}
	#endif // D3DX_SDK_VERSION < 24

	return 0;
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_9_
