// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D9_SHADER_MATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D9_SHADER_MATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_
#if defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#include "irrMath.h"    // needed by borland for sqrtf define
#endif
#include <d3dx9shader.h>

#include "IMaterialRenderer.h"

namespace irr
{
namespace video
{

class IVideoDriver;
class IShaderConstantSetCallBack;
class IMaterialRenderer;

//! Class for using vertex and pixel shaders with D3D9
class CD3D9ShaderMaterialRenderer : public IMaterialRenderer
{
public:

	//! Public constructor
	CD3D9ShaderMaterialRenderer(IDirect3DDevice9* d3ddev, video::IVideoDriver* driver,
		s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram,
		IShaderConstantSetCallBack* callback, IMaterialRenderer* baseMaterial, s32 userData);

	//! Destructor
	~CD3D9ShaderMaterialRenderer();

	virtual void OnSetMaterial(const video::SMaterial& material, const video::SMaterial& lastMaterial,
		bool resetAllRenderstates, video::IMaterialRendererServices* services);

	virtual void OnUnsetMaterial();

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	//! Returns if the material is transparent.
	virtual bool isTransparent() const;

protected:

	//! constructor only for use by derived classes who want to
	//! create a fall back material for example.
	CD3D9ShaderMaterialRenderer(IDirect3DDevice9* d3ddev,
					video::IVideoDriver* driver,
					IShaderConstantSetCallBack* callback,
					IMaterialRenderer* baseMaterial,
					s32 userData=0);

	void init(s32& outMaterialTypeNr, const c8* vertexShaderProgram, const c8* pixelShaderProgram);
	bool createPixelShader(const c8* pxsh);
	bool createVertexShader(const char* vtxsh);

	HRESULT stubD3DXAssembleShader(LPCSTR pSrcData,  UINT SrcDataLen,
					CONST D3DXMACRO* pDefines, LPD3DXINCLUDE pInclude,
					DWORD Flags, LPD3DXBUFFER* ppShader,
					LPD3DXBUFFER* ppErrorMsgs);

	HRESULT stubD3DXAssembleShaderFromFile(LPCSTR pSrcFile,
				        CONST D3DXMACRO* pDefines, LPD3DXINCLUDE  pInclude, DWORD Flags,
					LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs);

	HRESULT stubD3DXCompileShader(LPCSTR pSrcData, UINT SrcDataLen, CONST D3DXMACRO* pDefines,
					LPD3DXINCLUDE pInclude, LPCSTR pFunctionName,
					LPCSTR pProfile, DWORD Flags, LPD3DXBUFFER* ppShader,
					LPD3DXBUFFER* ppErrorMsgs, LPD3DXCONSTANTTABLE* ppConstantTable);

	HRESULT stubD3DXCompileShaderFromFile(LPCSTR pSrcFile, CONST D3DXMACRO* pDefines,
					LPD3DXINCLUDE pInclude, LPCSTR pFunctionName,
					LPCSTR pProfile, DWORD Flags, LPD3DXBUFFER* ppShader, LPD3DXBUFFER* ppErrorMsgs,
					LPD3DXCONSTANTTABLE* ppConstantTable);

	IDirect3DDevice9* pID3DDevice;
	video::IVideoDriver* Driver;
	IShaderConstantSetCallBack* CallBack;
	IMaterialRenderer* BaseMaterial;

	IDirect3DVertexShader9* VertexShader;
	IDirect3DVertexShader9* OldVertexShader;
	IDirect3DPixelShader9* PixelShader;
	s32 UserData;
};


} // end namespace video
} // end namespace irr

#endif
#endif
#endif

