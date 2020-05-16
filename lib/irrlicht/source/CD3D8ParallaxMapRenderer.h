// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_D3D8_PARALLAX_MAPMATERIAL_RENDERER_H_INCLUDED__
#define __C_D3D8_PARALLAX_MAPMATERIAL_RENDERER_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_WINDOWS_API_

#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_
#include <d3d8.h>

#include "CD3D8ShaderMaterialRenderer.h"
#include "IShaderConstantSetCallBack.h"

namespace irr
{
namespace video
{

//! Renderer for parallax maps
class CD3D8ParallaxMapRenderer : public CD3D8ShaderMaterialRenderer, IShaderConstantSetCallBack
{
public:

	CD3D8ParallaxMapRenderer(
		IDirect3DDevice8* d3ddev, video::IVideoDriver* driver,
		s32& outMaterialTypeNr, IMaterialRenderer* baseMaterial);
	~CD3D8ParallaxMapRenderer();

	//! Called by the engine when the vertex and/or pixel shader constants for an
	//! material renderer should be set.
	virtual void OnSetConstants(IMaterialRendererServices* services, s32 userData);

	virtual bool OnRender(IMaterialRendererServices* service, E_VERTEX_TYPE vtxtype);

	virtual void OnSetMaterial(const SMaterial& material) { }
	virtual void OnSetMaterial(const video::SMaterial& material,
		const video::SMaterial& lastMaterial,
		bool resetAllRenderstates, video::IMaterialRendererServices* services);

	//! Returns the render capability of the material.
	virtual s32 getRenderCapability() const;

private:

	//! stores if this shader compiled the shaders and is
	//! allowed to delete them again. D3D8 lacks reference counting
	//! support for shaders.
	bool CompiledShaders;

	f32 CurrentScale;
};

} // end namespace video
} // end namespace irr

#endif
#endif
#endif

