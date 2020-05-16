// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

#include "CD3D9HLSLMaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"
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
CD3D9HLSLMaterialRenderer::CD3D9HLSLMaterialRenderer(IDirect3DDevice9* d3ddev,
	video::IVideoDriver* driver, s32& outMaterialTypeNr,
	const c8* vertexShaderProgram,
	const c8* vertexShaderEntryPointName,
	E_VERTEX_SHADER_TYPE vsCompileTarget,
	const c8* pixelShaderProgram,
	const c8* pixelShaderEntryPointName,
	E_PIXEL_SHADER_TYPE psCompileTarget,
	IShaderConstantSetCallBack* callback,
	IMaterialRenderer* baseMaterial,
	s32 userData)
	: CD3D9ShaderMaterialRenderer(d3ddev, driver, callback, baseMaterial, userData),
	VSConstantsTable(0), PSConstantsTable(0)
{

	#ifdef _DEBUG
	setDebugName("CD3D9HLSLMaterialRenderer");
	#endif

	outMaterialTypeNr = -1;

	// now create shaders

	if (vsCompileTarget < 0 || vsCompileTarget > EVST_COUNT)
	{
		os::Printer::log("Invalid HLSL vertex shader compilation target", ELL_ERROR);
		return;
	}

	if (!createHLSLVertexShader(vertexShaderProgram,
		vertexShaderEntryPointName, VERTEX_SHADER_TYPE_NAMES[vsCompileTarget]))
		return;

	if (!createHLSLPixelShader(pixelShaderProgram,
		pixelShaderEntryPointName, PIXEL_SHADER_TYPE_NAMES[psCompileTarget]))
		return;

	// register myself as new material
	outMaterialTypeNr = Driver->addMaterialRenderer(this);
}


//! Destructor
CD3D9HLSLMaterialRenderer::~CD3D9HLSLMaterialRenderer()
{
	if (VSConstantsTable)
		VSConstantsTable->Release();

	if (PSConstantsTable)
		PSConstantsTable->Release();
}


bool CD3D9HLSLMaterialRenderer::createHLSLVertexShader(const char* vertexShaderProgram,
			const char* shaderEntryPointName,
			const char* shaderTargetName)
{
	if (!vertexShaderProgram)
		return true;

	LPD3DXBUFFER buffer = 0;
	LPD3DXBUFFER errors = 0;

#ifdef _IRR_D3D_NO_SHADER_DEBUGGING

	// compile without debug info
	HRESULT h = stubD3DXCompileShader(
		vertexShaderProgram,
		strlen(vertexShaderProgram),
		0, // macros
		0, // no includes
		shaderEntryPointName,
		shaderTargetName,
		0, // no flags
		&buffer,
		&errors,
		&VSConstantsTable);

#else

	// compile shader and emitt some debug informations to
	// make it possible to debug the shader in visual studio

	static int irr_dbg_hlsl_file_nr = 0;
	++irr_dbg_hlsl_file_nr;
	char tmp[32];
	sprintf(tmp, "irr_d3d9_dbg_hlsl_%d.vsh", irr_dbg_hlsl_file_nr);

	FILE* f = fopen(tmp, "wb");
	fwrite(vertexShaderProgram, strlen(vertexShaderProgram), 1, f);
	fflush(f);
	fclose(f);

	HRESULT h = stubD3DXCompileShaderFromFile(
		tmp,
		0, // macros
		0, // no includes
		shaderEntryPointName,
		shaderTargetName,
		D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION,
		&buffer,
		&errors,
		&VSConstantsTable);

#endif

	if (FAILED(h))
	{
		os::Printer::log("HLSL vertex shader compilation failed:", ELL_ERROR);
		if (errors)
		{
			os::Printer::log((c8*)errors->GetBufferPointer(), ELL_ERROR);
			errors->Release();
			if (buffer)
				buffer->Release();
		}
		return false;
	}

	if (errors)
		errors->Release();

	if (buffer)
	{
		if (FAILED(pID3DDevice->CreateVertexShader((DWORD*)buffer->GetBufferPointer(),
			&VertexShader)))
		{
			os::Printer::log("Could not create hlsl vertex shader.", ELL_ERROR);
			buffer->Release();
			return false;
		}

		buffer->Release();
		return true;
	}

	return false;
}


bool CD3D9HLSLMaterialRenderer::createHLSLPixelShader(const char* pixelShaderProgram,
		const char* shaderEntryPointName,
		const char* shaderTargetName)
{
	if (!pixelShaderProgram)
		return true;

	LPD3DXBUFFER buffer = 0;
	LPD3DXBUFFER errors = 0;

	DWORD flags = 0;

#ifdef D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY
	if (Driver->queryFeature(video::EVDF_VERTEX_SHADER_2_0) || Driver->queryFeature(video::EVDF_VERTEX_SHADER_3_0))
		// this one's for newer DX SDKs which don't support ps_1_x anymore
		// instead they'll silently compile 1_x as 2_x when using this flag
		flags |= D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY;
#endif
#if defined(_IRR_D3D_USE_LEGACY_HLSL_COMPILER) && defined(D3DXSHADER_USE_LEGACY_D3DX9_31_DLL)
#ifdef D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY
	else
#endif
		flags |= D3DXSHADER_USE_LEGACY_D3DX9_31_DLL;
#endif

#ifdef _IRR_D3D_NO_SHADER_DEBUGGING

	// compile without debug info
	HRESULT h = stubD3DXCompileShader(
		pixelShaderProgram,
		strlen(pixelShaderProgram),
		0, // macros
		0, // no includes
		shaderEntryPointName,
		shaderTargetName,
		flags,
		&buffer,
		&errors,
		&PSConstantsTable);

#else

	// compile shader and emitt some debug informations to
	// make it possible to debug the shader in visual studio

	static int irr_dbg_hlsl_file_nr = 0;
	++irr_dbg_hlsl_file_nr;
	char tmp[32];
	sprintf(tmp, "irr_d3d9_dbg_hlsl_%d.psh", irr_dbg_hlsl_file_nr);

	FILE* f = fopen(tmp, "wb");
	fwrite(pixelShaderProgram, strlen(pixelShaderProgram), 1, f);
	fflush(f);
	fclose(f);

	HRESULT h = stubD3DXCompileShaderFromFile(
		tmp,
		0, // macros
		0, // no includes
		shaderEntryPointName,
		shaderTargetName,
		flags | D3DXSHADER_DEBUG | D3DXSHADER_SKIPOPTIMIZATION,
		&buffer,
		&errors,
		&PSConstantsTable);

#endif

	if (FAILED(h))
	{
		os::Printer::log("HLSL pixel shader compilation failed:", ELL_ERROR);
		if (errors)
		{
			os::Printer::log((c8*)errors->GetBufferPointer(), ELL_ERROR);
			errors->Release();
			if (buffer)
				buffer->Release();
		}
		return false;
	}

	if (errors)
		errors->Release();

	if (buffer)
	{
		if (FAILED(pID3DDevice->CreatePixelShader((DWORD*)buffer->GetBufferPointer(),
			&PixelShader)))
		{
			os::Printer::log("Could not create hlsl pixel shader.", ELL_ERROR);
			buffer->Release();
			return false;
		}

		buffer->Release();
		return true;
	}

	return false;
}


bool CD3D9HLSLMaterialRenderer::setVariable(bool vertexShader, const c8* name,
					const f32* floats, int count)
{
	LPD3DXCONSTANTTABLE tbl = vertexShader ? VSConstantsTable : PSConstantsTable;
	if (!tbl)
		return false;

	// currently we only support top level parameters.
	// Should be enough for the beginning. (TODO)

	D3DXHANDLE hndl = tbl->GetConstantByName(NULL, name);
	if (!hndl)
	{
		core::stringc s = "HLSL Variable to set not found: '";
		s += name;
		s += "'. Available variables are:";
		os::Printer::log(s.c_str(), ELL_WARNING);
		printHLSLVariables(tbl);
		return false;
	}

	D3DXCONSTANT_DESC Description;
	UINT ucount = 1;
    tbl->GetConstantDesc(hndl, &Description, &ucount);

	if(Description.RegisterSet != D3DXRS_SAMPLER)
	{
		HRESULT hr = tbl->SetFloatArray(pID3DDevice, hndl, floats, count);
		if (FAILED(hr))
		{
			os::Printer::log("Error setting float array for HLSL variable", ELL_WARNING);
			return false;
		}
	}

	return true;
}


bool CD3D9HLSLMaterialRenderer::setVariable(bool vertexShader, const c8* name,
					const bool* bools, int count)
{
	LPD3DXCONSTANTTABLE tbl = vertexShader ? VSConstantsTable : PSConstantsTable;
	if (!tbl)
		return false;

	// currently we only support top level parameters.
	// Should be enough for the beginning. (TODO)

	D3DXHANDLE hndl = tbl->GetConstantByName(NULL, name);
	if (!hndl)
	{
		core::stringc s = "HLSL Variable to set not found: '";
		s += name;
		s += "'. Available variables are:";
		os::Printer::log(s.c_str(), ELL_WARNING);
		printHLSLVariables(tbl);
		return false;
	}

	D3DXCONSTANT_DESC Description;
	UINT ucount = 1;
    tbl->GetConstantDesc(hndl, &Description, &ucount);

	if(Description.RegisterSet != D3DXRS_SAMPLER)
	{
		HRESULT hr = tbl->SetBoolArray(pID3DDevice, hndl, (BOOL*)bools, count);
		if (FAILED(hr))
		{
			os::Printer::log("Error setting bool array for HLSL variable", ELL_WARNING);
			return false;
		}
	}

	return true;
}


bool CD3D9HLSLMaterialRenderer::setVariable(bool vertexShader, const c8* name,
					const s32* ints, int count)
{
	LPD3DXCONSTANTTABLE tbl = vertexShader ? VSConstantsTable : PSConstantsTable;
	if (!tbl)
		return false;

	// currently we only support top level parameters.
	// Should be enough for the beginning. (TODO)

	D3DXHANDLE hndl = tbl->GetConstantByName(NULL, name);
	if (!hndl)
	{
		core::stringc s = "HLSL Variable to set not found: '";
		s += name;
		s += "'. Available variables are:";
		os::Printer::log(s.c_str(), ELL_WARNING);
		printHLSLVariables(tbl);
		return false;
	}

	D3DXCONSTANT_DESC Description;
	UINT ucount = 1;
    tbl->GetConstantDesc(hndl, &Description, &ucount);

	if(Description.RegisterSet != D3DXRS_SAMPLER)
	{
		HRESULT hr = tbl->SetIntArray(pID3DDevice, hndl, ints, count);
		if (FAILED(hr))
		{
			os::Printer::log("Error setting int array for HLSL variable", ELL_WARNING);
			return false;
		}
	}

	return true;
}


bool CD3D9HLSLMaterialRenderer::OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype)
{
	if (VSConstantsTable)
		VSConstantsTable->SetDefaults(pID3DDevice);

	return CD3D9ShaderMaterialRenderer::OnRender(service, vtxtype);
}


void CD3D9HLSLMaterialRenderer::printHLSLVariables(LPD3DXCONSTANTTABLE table)
{
	// currently we only support top level parameters.
	// Should be enough for the beginning. (TODO)

	// print out constant names
	D3DXCONSTANTTABLE_DESC tblDesc;
	HRESULT hr = table->GetDesc(&tblDesc);
	if (!FAILED(hr))
	{
		for (int i=0; i<(int)tblDesc.Constants; ++i)
		{
			D3DXCONSTANT_DESC d;
			UINT n = 1;
			D3DXHANDLE cHndl = table->GetConstant(NULL, i);
			if (!FAILED(table->GetConstantDesc(cHndl, &d, &n)))
			{
				core::stringc s = "  '";
				s += d.Name;
				s += "' Registers:[begin:";
				s += (int)d.RegisterIndex;
				s += ", count:";
				s += (int)d.RegisterCount;
				s += "]";
				os::Printer::log(s.c_str());
			}
		}
	}
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_9_
