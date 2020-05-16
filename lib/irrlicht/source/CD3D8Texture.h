// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_DIRECTX8_TEXTURE_H_INCLUDED__
#define __C_DIRECTX8_TEXTURE_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_

#include "ITexture.h"
#include "IImage.h"

#include <d3d8.h>

namespace irr
{
namespace video
{

class CD3D8Driver;

/*!
	interface for a Video Driver dependent Texture.
*/
class CD3D8Texture : public ITexture
{
public:

	//! constructor
	CD3D8Texture(IImage* image, CD3D8Driver* driver,
		u32 flags, const io::path& name, void* mipmapData=0);

	//! rendertarget constructor
	CD3D8Texture(CD3D8Driver* driver, const core::dimension2d<u32>& size, const io::path& name);

	//! destructor
	virtual ~CD3D8Texture();

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

	//! returns the DIRECT3D8 Texture
	IDirect3DTexture8* getDX8Texture() const;

	//! returns if texture has mipmap levels
	bool hasMipMaps() const;

	//! Regenerates the mip map levels of the texture. Useful after locking and
	//! modifying the texture
	virtual void regenerateMipMapLevels(void* mipmapData=0);

	//! returns if it is a render target
	virtual bool isRenderTarget() const;

	//! Returns pointer to the render target surface
	IDirect3DSurface8* getRenderTargetSurface();

private:
	friend class CD3D8Driver;

	void createRenderTarget();

	//! creates the hardware texture
	bool createTexture(u32 flags, IImage* Image);

	//! copies the image to the texture
	bool copyTexture(IImage* Image);

	//! convert color formats
	ECOLOR_FORMAT getColorFormatFromD3DFormat(D3DFORMAT format);

	bool createMipMaps(u32 level=1);

	void copy16BitMipMap(char* src, char* tgt,
		s32 width, s32 height, s32 pitchsrc, s32 pitchtgt) const;

	void copy32BitMipMap(char* src, char* tgt,
		s32 width, s32 height, s32 pitchsrc, s32 pitchtgt) const;

	IDirect3DDevice8* Device;
	IDirect3DTexture8* Texture;
	IDirect3DSurface8* RTTSurface;
	CD3D8Driver* Driver;
	core::dimension2d<u32> TextureSize;
	core::dimension2d<u32> ImageSize;
	s32 Pitch;
	u32 MipLevelLocked;
	ECOLOR_FORMAT ColorFormat;

	bool HasMipMaps;
	bool IsRenderTarget;
};


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_8_

#endif // __C_DIRECTX8_TEXTURE_H_INCLUDED__

