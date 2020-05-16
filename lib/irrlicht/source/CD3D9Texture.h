// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_DIRECTX9_TEXTURE_H_INCLUDED__
#define __C_DIRECTX9_TEXTURE_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_9_

#include "ITexture.h"
#include "IImage.h"
#if defined(__BORLANDC__) || defined (__BCPLUSPLUS__)
#include "irrMath.h"    // needed by borland for sqrtf define
#endif
#include <d3d9.h>

namespace irr
{
namespace video
{

class CD3D9Driver;
// forward declaration for RTT depth buffer handling
struct SDepthSurface;
/*!
	interface for a Video Driver dependent Texture.
*/
class CD3D9Texture : public ITexture
{
public:

	//! constructor
	CD3D9Texture(IImage* image, CD3D9Driver* driver,
			u32 flags, const io::path& name, void* mipmapData=0);

	//! rendertarget constructor
	CD3D9Texture(CD3D9Driver* driver, const core::dimension2d<u32>& size, const io::path& name,
		const ECOLOR_FORMAT format = ECF_UNKNOWN);

	//! destructor
	virtual ~CD3D9Texture();

	//! lock function
	virtual void* lock(E_TEXTURE_LOCK_MODE mode=ETLM_READ_WRITE, u32 mipmapLevel=0);

	//! unlock function
	virtual void unlock();

	//! Returns original size of the texture.
	virtual const core::dimension2d<u32>& getOriginalSize() const;

	//! Returns (=size) of the texture.
	virtual const core::dimension2d<u32>& getSize() const;

	//! returns driver type of texture (=the driver, who created the texture)
	virtual E_DRIVER_TYPE getDriverType() const;

	//! returns color format of texture
	virtual ECOLOR_FORMAT getColorFormat() const;

	//! returns pitch of texture (in bytes)
	virtual u32 getPitch() const;

	//! returns the DIRECT3D9 Texture
	IDirect3DBaseTexture9* getDX9Texture() const;

	//! returns if texture has mipmap levels
	bool hasMipMaps() const;

	//! Regenerates the mip map levels of the texture. Useful after locking and
	//! modifying the texture
	virtual void regenerateMipMapLevels(void* mipmapData=0);

	//! returns if it is a render target
	virtual bool isRenderTarget() const;

	//! Returns pointer to the render target surface
	IDirect3DSurface9* getRenderTargetSurface();

private:
	friend class CD3D9Driver;

	void createRenderTarget(const ECOLOR_FORMAT format = ECF_UNKNOWN);

	//! creates the hardware texture
	bool createTexture(u32 flags, IImage * image);

	//! copies the image to the texture
	bool copyTexture(IImage * image);

	//! Helper function for mipmap generation.
	bool createMipMaps(u32 level=1);

	//! Helper function for mipmap generation.
	void copy16BitMipMap(char* src, char* tgt,
		s32 width, s32 height,  s32 pitchsrc, s32 pitchtgt) const;

	//! Helper function for mipmap generation.
	void copy32BitMipMap(char* src, char* tgt,
		s32 width, s32 height,  s32 pitchsrc, s32 pitchtgt) const;

	//! set Pitch based on the d3d format
	void setPitch(D3DFORMAT d3dformat);

	IDirect3DDevice9* Device;
	IDirect3DTexture9* Texture;
	IDirect3DSurface9* RTTSurface;
	CD3D9Driver* Driver;
	SDepthSurface* DepthSurface;
	core::dimension2d<u32> TextureSize;
	core::dimension2d<u32> ImageSize;
	s32 Pitch;
	u32 MipLevelLocked;
	ECOLOR_FORMAT ColorFormat;

	bool HasMipMaps;
	bool HardwareMipMaps;
	bool IsRenderTarget;
};


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_9_

#endif // __C_DIRECTX9_TEXTURE_H_INCLUDED__


