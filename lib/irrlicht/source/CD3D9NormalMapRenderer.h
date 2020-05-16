// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D9_NORMAL_MAPMATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D9_NORMAL_MAPMATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_
#if defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#include "irrMath.h"    // needed by borland for sqrtf define
#endif 
#include <d3d9.h>

#include "CD3D9ShaderMaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"

namespace irr
{
namespace video
{

//! Renderer for normal maps
class CD3D9NormalMapRenderer :
	public CD3D9ShaderMaterialRenderer, IShaderConstantSetCallBack
{
public:

	CD3D9NormalMapRenderer(
		IDirect3DDevice9* d3ddev, video::IVideoDriver* driver,
		s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial);

	~CD3D9NormalMapRenderer();

	//! Called by the engine when the vertex and/or pixel shader constants for an
	//! material renderer should be set.
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	//! Returns the render capability of the material.
	virtual s32 getRenderCapability() const;

private:

};

} // end namespace video
} // end namespace irr

#endif
#endif
#endif

