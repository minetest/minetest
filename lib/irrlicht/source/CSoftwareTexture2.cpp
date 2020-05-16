// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BURNINGSVIDEO_

#include "SoftwareDriver2_compile_config.h"
#include "SoftwareDriver2_helper.h"
#include "CSoftwareTexture2.h"
#include "os.h"

namespace irr
{
namespace video
{

//! constructor
CSoftwareTexture2::CSoftwareTexture2(IImage* image, const io::path& name,
		u32 flags, void* mipmapData)
		: ITexture(name), MipMapLOD(0), Flags ( flags ), OriginalFormat(video::ECF_UNKNOWN)
{
	#ifdef _DEBUG
	setDebugName("CSoftwareTexture2");
	#endif

	#ifndef SOFTWARE_DRIVER_2_MIPMAPPING
		Flags &= ~GEN_MIPMAP;
	#endif

	memset32 ( MipMap, 0, sizeof ( MipMap ) );

	if (image)
	{
		OrigSize = image->getDimension();
		OriginalFormat = image->getColorFormat();

		core::setbit_cond(Flags,
				image->getColorFormat () == video::ECF_A8R8G8B8 ||
				image->getColorFormat () == video::ECF_A1R5G5B5,
				HAS_ALPHA);

		core::dimension2d<u32> optSize(
				OrigSize.getOptimalSize( 0 != ( Flags & NP2_SIZE ),
				false, false,
				( Flags & NP2_SIZE ) ? SOFTWARE_DRIVER_2_TEXTURE_MAXSIZE : 0)
			);

		if ( OrigSize == optSize )
		{
			MipMap[0] = new CImage(BURNINGSHADER_COLOR_FORMAT, image->getDimension());
			image->copyTo(MipMap[0]);
		}
		else
		{
			char buf[256];
			core::stringw showName ( name );
			snprintf ( buf, 256, "Burningvideo: Warning Texture %ls reformat %dx%d -> %dx%d,%d",
							showName.c_str(),
							OrigSize.Width, OrigSize.Height, optSize.Width, optSize.Height,
							BURNINGSHADER_COLOR_FORMAT
						);

			OrigSize = optSize;
			os::Printer::log ( buf, ELL_WARNING );
			MipMap[0] = new CImage(BURNINGSHADER_COLOR_FORMAT, optSize);
			image->copyToScalingBoxFilter ( MipMap[0],0, false );
		}

		OrigImageDataSizeInPixels = (f32) 0.3f * MipMap[0]->getImageDataSizeInPixels();
	}

	regenerateMipMapLevels(mipmapData);
}


//! destructor
CSoftwareTexture2::~CSoftwareTexture2()
{
	for ( s32 i = 0; i!= SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i )
	{
		if ( MipMap[i] )
			MipMap[i]->drop();
	}
}


//! Regenerates the mip map levels of the texture. Useful after locking and
//! modifying the texture
void CSoftwareTexture2::regenerateMipMapLevels(void* mipmapData)
{
	if ( !hasMipMaps () )
		return;

	s32 i;

	// release
	for ( i = 1; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i )
	{
		if ( MipMap[i] )
			MipMap[i]->drop();
	}

	core::dimension2d<u32> newSize;
	core::dimension2d<u32> origSize=OrigSize;

	for (i=1; i < SOFTWARE_DRIVER_2_MIPMAPPING_MAX; ++i)
	{
		newSize = MipMap[i-1]->getDimension();
		newSize.Width = core::s32_max ( 1, newSize.Width >> SOFTWARE_DRIVER_2_MIPMAPPING_SCALE );
		newSize.Height = core::s32_max ( 1, newSize.Height >> SOFTWARE_DRIVER_2_MIPMAPPING_SCALE );
		origSize.Width = core::s32_max(1, origSize.Width >> 1);
		origSize.Height = core::s32_max(1, origSize.Height >> 1);

		if (mipmapData)
		{
			if (OriginalFormat != BURNINGSHADER_COLOR_FORMAT)
			{
				IImage* tmpImage = new CImage(OriginalFormat, origSize, mipmapData, true, false);
				MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize);
				if (origSize==newSize)
					tmpImage->copyTo(MipMap[i]);
				else
					tmpImage->copyToScalingBoxFilter(MipMap[i]);
				tmpImage->drop();
			}
			else
			{
				if (origSize==newSize)
					MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize, mipmapData, false);
				else
				{
					MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize);
					IImage* tmpImage = new CImage(BURNINGSHADER_COLOR_FORMAT, origSize, mipmapData, true, false);
					tmpImage->copyToScalingBoxFilter(MipMap[i]);
					tmpImage->drop();
				}
			}
			mipmapData = (u8*)mipmapData+origSize.getArea()*IImage::getBitsPerPixelFromFormat(OriginalFormat)/8;
		}
		else
		{
			MipMap[i] = new CImage(BURNINGSHADER_COLOR_FORMAT, newSize);

			//static u32 color[] = { 0, 0xFFFF0000, 0xFF00FF00,0xFF0000FF,0xFFFFFF00,0xFFFF00FF,0xFF00FFFF,0xFF0F0F0F };
			MipMap[i]->fill ( 0 );
			MipMap[0]->copyToScalingBoxFilter( MipMap[i], 0, false );
		}
	}
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BURNINGSVIDEO_
