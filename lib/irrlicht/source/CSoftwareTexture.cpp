// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_SOFTWARE_

#include "CSoftwareTexture.h"
#include "os.h"

namespace irr
{
namespace video
{

//! constructor
CSoftwareTexture::CSoftwareTexture(IImage* image, const io::path& name,
		bool renderTarget, void* mipmapData)
: ITexture(name), Texture(0), IsRenderTarget(renderTarget)
{
	#ifdef _DEBUG
	setDebugName("CSoftwareTexture");
	#endif

	if (image)
	{
		OrigSize = image->getDimension();
		core::dimension2d<u32> optSize=OrigSize.getOptimalSize();

		Image = new CImage(ECF_A1R5G5B5, OrigSize);
		image->copyTo(Image);

		if (optSize == OrigSize)
		{
			Texture = Image;
			Texture->grab();
		}
		else
		{
			Texture = new CImage(ECF_A1R5G5B5, optSize);
			Image->copyToScaling(Texture);
		}
	}
}



//! destructor
CSoftwareTexture::~CSoftwareTexture()
{
	if (Image)
		Image->drop();

	if (Texture)
		Texture->drop();
}



//! lock function
void* CSoftwareTexture::lock(E_TEXTURE_LOCK_MODE mode, u32 mipmapLevel)
{
	return Image->lock();
}



//! unlock function
void CSoftwareTexture::unlock()
{
	if (Image != Texture)
	{
		os::Printer::log("Performance warning, slow unlock of non power of 2 texture.", ELL_WARNING);
		Image->copyToScaling(Texture);
	}

	Image->unlock();
}


//! Returns original size of the texture.
const core::dimension2d<u32>& CSoftwareTexture::getOriginalSize() const
{
	return OrigSize;
}


//! Returns (=size) of the texture.
const core::dimension2d<u32>& CSoftwareTexture::getSize() const
{
	return Image->getDimension();
}


//! returns unoptimized surface
CImage* CSoftwareTexture::getImage()
{
	return Image;
}



//! returns texture surface
CImage* CSoftwareTexture::getTexture()
{
	return Texture;
}



//! returns driver type of texture (=the driver, who created the texture)
E_DRIVER_TYPE CSoftwareTexture::getDriverType() const
{
	return EDT_SOFTWARE;
}



//! returns color format of texture
ECOLOR_FORMAT CSoftwareTexture::getColorFormat() const
{
	return ECF_A1R5G5B5;
}



//! returns pitch of texture (in bytes)
u32 CSoftwareTexture::getPitch() const
{
	return Image->getDimension().Width * 2;
}


//! Regenerates the mip map levels of the texture. Useful after locking and
//! modifying the texture
void CSoftwareTexture::regenerateMipMapLevels(void* mipmapData)
{
	// our software textures don't have mip maps
}

bool CSoftwareTexture::isRenderTarget() const
{
	return IsRenderTarget;
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_SOFTWARE_

