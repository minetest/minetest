// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SOFTWARE_TEXTURE_H_INCLUDED__
#define __C_SOFTWARE_TEXTURE_H_INCLUDED__

#include "ITexture.h"
#include "CImage.h"

namespace irr
{
namespace video
{

/*!
	interface for a Video Driver dependent Texture.
*/
class CSoftwareTexture : public ITexture
{
public:

	//! constructor
	CSoftwareTexture(IImage* surface, const io::path& name,
			bool renderTarget=false, void* mipmapData=0);

	//! destructor
	virtual ~CSoftwareTexture();

	//! lock function
	virtual void* lock(E_TEXTURE_LOCK_MODE mode=ETLM_READ_WRITE, u32 mipmapLevel=0);

	//! unlock function
	virtual void unlock();

	//! Returns original size of the texture.
	virtual const core::dimension2d<u32>& getOriginalSize() const;

	//! Returns (=size) of the texture.
	virtual const core::dimension2d<u32>& getSize() const;

	//! returns unoptimized surface
	virtual CImage* getImage();

	//! returns texture surface
	virtual CImage* getTexture();

	//! returns driver type of texture (=the driver, who created the texture)
	virtual E_DRIVER_TYPE getDriverType() const;

	//! returns color format of texture
	virtual ECOLOR_FORMAT getColorFormat() const;

	//! returns pitch of texture (in bytes)
	virtual u32 getPitch() const;

	//! Regenerates the mip map levels of the texture. Useful after locking and
	//! modifying the texture
	virtual void regenerateMipMapLevels(void* mipmapData=0);

	//! is it a render target?
	virtual bool isRenderTarget() const;

private:
	CImage* Image;
	CImage* Texture;
	core::dimension2d<u32> OrigSize;
	bool IsRenderTarget;
};


} // end namespace video
} // end namespace irr

#endif

