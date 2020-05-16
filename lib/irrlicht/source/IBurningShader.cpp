// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

#include "SoftwareDriver2_compile_config.h"
#include "IBurningShader.h"
#include "CSoftwareDriver2.h"

namespace irr
{
namespace video
{

	const tFixPointu IBurningShader::dithermask[] =
	{
		0x00,0x80,0x20,0xa0,
		0xc0,0x40,0xe0,0x60,
		0x30,0xb0,0x10,0x90,
		0xf0,0x70,0xd0,0x50
	};

	IBurningShader::IBurningShader(CBurningVideoDriver* driver)
	{
		#ifdef _DEBUG
		setDebugName("IBurningShader");
		#endif

		for ( u32 i = 0; i != BURNING_MATERIAL_MAX_TEXTURES; ++i )
		{
			IT[i].Texture = 0;
		}

		Driver = driver;
		RenderTarget = 0;
		ColorMask = COLOR_BRIGHT_WHITE;
		DepthBuffer = (CDepthBuffer*) driver->getDepthBuffer ();
		if ( DepthBuffer )
			DepthBuffer->grab();

		Stencil = (CStencilBuffer*) driver->getStencilBuffer ();
		if ( Stencil )
			Stencil->grab();

	}


	//! destructor
	IBurningShader::~IBurningShader()
	{
		if (RenderTarget)
			RenderTarget->drop();

		if (DepthBuffer)
			DepthBuffer->drop();

		if (Stencil)
			Stencil->drop();

		for ( u32 i = 0; i != BURNING_MATERIAL_MAX_TEXTURES; ++i )
		{
			if ( IT[i].Texture )
				IT[i].Texture->drop();
		}
	}

	//! sets a render target
	void IBurningShader::setRenderTarget(video::IImage* surface, const core::rect<s32>& viewPort)
	{
		if (RenderTarget)
			RenderTarget->drop();

		RenderTarget = (video::CImage* ) surface;

		if (RenderTarget)
		{
			RenderTarget->grab();

			//(tVideoSample*)RenderTarget->lock() = (tVideoSample*)RenderTarget->lock();
			//(fp24*) DepthBuffer->lock() = DepthBuffer->lock();
		}
	}


	//! sets the Texture
	void IBurningShader::setTextureParam( u32 stage, video::CSoftwareTexture2* texture, s32 lodLevel)
	{
		sInternalTexture *it = &IT[stage];

		if ( it->Texture)
			it->Texture->drop();

		it->Texture = texture;

		if ( it->Texture)
		{
			it->Texture->grab();

			// select mignify and magnify ( lodLevel )
			//SOFTWARE_DRIVER_2_MIPMAPPING_LOD_BIAS
			it->lodLevel = lodLevel;
			it->data = (tVideoSample*) it->Texture->lock(ETLM_READ_ONLY,
				core::s32_clamp ( lodLevel + SOFTWARE_DRIVER_2_MIPMAPPING_LOD_BIAS, 0, SOFTWARE_DRIVER_2_MIPMAPPING_MAX - 1 ));

			// prepare for optimal fixpoint
			it->pitchlog2 = s32_log2_s32 ( it->Texture->getPitch() );

			const core::dimension2d<u32> &dim = it->Texture->getSize();
			it->textureXMask = s32_to_fixPoint ( dim.Width - 1 ) & FIX_POINT_UNSIGNED_MASK;
			it->textureYMask = s32_to_fixPoint ( dim.Height - 1 ) & FIX_POINT_UNSIGNED_MASK;
		}
	}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
