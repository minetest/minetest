// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SOFTWARE_2_TEXTURE_H_INCLUDED__
#define __C_SOFTWARE_2_TEXTURE_H_INCLUDED__

#include "SoftwareDriver2_compile_config.h"

#include "ITexture.h"
#include "CImage.h"

namespace irr
{
namespace video
{

/*!
	interface for a Video Driver dependent Texture.
*/
class CSoftwareTexture2 : public ITexture
{
public:

	//! constructor
	enum eTex2Flags
	{
		GEN_MIPMAP	= 1,
		IS_RENDERTARGET	= 2,
		NP2_SIZE	= 4,
		HAS_ALPHA	= 8
	};
	CSoftwareTexture2(IImage* surface, const io::path& name, u32 flags, void* mipmapData=0);

	//! destructor
	virtual ~CSoftwareTexture2();

	//! lock function
	virtual void* lock(E_TEXTURE_LOCK_MODE mode=ETLM_READ_WRITE, u32 mipmapLevel=0)
	{
		if (Flags & GEN_MIPMAP)
			MipMapLOD=mipmapLevel;
		return MipMap[MipMapLOD]->lock();
	}

	//! unlock function
	virtual void unlock()
	{
		MipMap[MipMapLOD]->unlock();
	}

	//! Returns original size of the texture.
	virtual const core::dimension2d<u32>& getOriginalSize() const
	{
		//return MipMap[0]->getDimension();
		return OrigSize;
	}

	//! Returns the size of the largest mipmap.
	f32 getLODFactor( const f32 texArea ) const
	{
		return OrigImageDataSizeInPixels * texArea;
		//return MipMap[0]->getImageDataSizeInPixels () * texArea;
	}

	//! Returns (=size) of the texture.
	virtual const core::dimension2d<u32>& getSize() const
	{
		return MipMap[MipMapLOD]->getDimension();
	}

	//! returns unoptimized surface
	virtual CImage* getImage() const
	{
		return MipMap[0];
	}

	//! returns texture surface
	virtual CImage* getTexture() const
	{
		return MipMap[MipMapLOD];
	}


	//! returns driver type of texture (=the driver, who created the texture)
	virtual E_DRIVER_TYPE getDriverType() const
	{
		return EDT_BURNINGSVIDEO;
	}

	//! returns color format of texture
	virtual ECOLOR_FORMAT getColorFormat() const
	{
		return BURNINGSHADER_COLOR_FORMAT;
	}

	//! returns pitch of texture (in bytes)
	virtual u32 getPitch() const
	{
		return MipMap[MipMapLOD]->getPitch();
	}

	//! Regenerates the mip map levels of the texture. Useful after locking and
	//! modifying the texture
	virtual void regenerateMipMapLevels(void* mipmapData=0);

	//! support mipmaps
	virtual bool hasMipMaps() const
	{
		return (Flags & GEN_MIPMAP ) != 0;
	}

	//! Returns if the texture has an alpha channel
	virtual bool hasAlpha() const
	{
		return (Flags & HAS_ALPHA ) != 0;
	}

	//! is a render target
	virtual bool isRenderTarget() const
	{
		return (Flags & IS_RENDERTARGET) != 0;
	}

private:
	f32 OrigImageDataSizeInPixels;
	core::dimension2d<u32> OrigSize;

	CImage * MipMap[SOFTWARE_DRIVER_2_MIPMAPPING_MAX];

	u32 MipMapLOD;
	u32 Flags;
	ECOLOR_FORMAT OriginalFormat;
};


} // end namespace video
} // end namespace irr

#endif

