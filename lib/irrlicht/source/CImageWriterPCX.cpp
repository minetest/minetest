// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageWriterPCX.h"

#ifdef _IRR_COMPILE_WITH_PCX_WRITER_

#include "CImageLoaderPCX.h"
#include "IWriteFile.h"
#include "os.h" // for logging
#include "irrString.h"

namespace irr
{
namespace video
{

IImageWriter* createImageWriterPCX()
{
	return new CImageWriterPCX;
}

CImageWriterPCX::CImageWriterPCX()
{
#ifdef _DEBUG
	setDebugName("CImageWriterPCX");
#endif
}

bool CImageWriterPCX::isAWriteableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "pcx" );
}

bool CImageWriterPCX::writeImage(io::IWriteFile *file, IImage *image,u32 param) const
{
	if (!file || !image)
		return false;

	u8 d1;
	u16 d2;
	u32 i;

	d1 = 10; // Manufacturer
	file->write(&d1, 1);
	d1 = 5; // Version
	file->write(&d1, 1);
	d1 = 1; // Encoding
	file->write(&d1, 1);
	d1 = 8; // Bits per Pixel
	file->write(&d1, 1);
	d2 = 0; // pixel origin
	file->write(&d2, 2);
	file->write(&d2, 2);
	d2 = image->getDimension().Width-1; // width
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	d2 = image->getDimension().Height-1; // height
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	d2 = 300; // dpi
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	file->write(&d2, 2);
	d2 = 0; // palette (not used)
	for (i=0; i<24; ++i)
	{
		file->write(&d2, 2);
	}
	d1 = 0; // reserved
	file->write(&d1, 1);
	d1 = 3; // planes
	file->write(&d1, 1);
	d2 = image->getDimension().Width; // pitch
	if (d2&0x0001) // must be even
		++d2;
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	d2 = 1; // color mode
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	d2 = 800; // screen width
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	d2 = 600; // screen height
#ifdef __BIG_ENDIAN__
	d2 = os::Byteswap::byteswap(d2);
#endif
	file->write(&d2, 2);
	d2 = 0; // filler (not used)
	for (i=0; i<27; ++i)
	{
		file->write(&d2, 2);
	}

	u8 cnt, value;
	for (i=0; i<image->getDimension().Height; ++i)
	{
		cnt = 0;
		value = 0;
		for (u32 j=0; j<3; ++j) // color planes
		{
			for (u32 k=0; k<image->getDimension().Width; ++k)
			{
				const SColor pix = image->getPixel(k,i);
				if ((cnt!=0) && (cnt<63) &&
					(((j==0) && (value==pix.getRed())) ||
					((j==1) && (value==pix.getGreen())) ||
					((j==2) && (value==pix.getBlue()))))
				{
					++cnt;
				}
				else
				{
					if (cnt!=0)
					{
						if ((cnt>1) || ((value&0xc0)==0xc0))
						{
							cnt |= 0xc0;
							file->write(&cnt, 1);
						}
						file->write(&value, 1);
					}
					cnt=1;
					if (j==0)
						value=(u8)pix.getRed();
					else if (j==1)
						value=(u8)pix.getGreen();
					else if (j==2)
						value=(u8)pix.getBlue();
				}
			}
		}
		if ((cnt>1) || ((value&0xc0)==0xc0))
		{
			cnt |= 0xc0;
			file->write(&cnt, 1);
		}
		file->write(&value, 1);
	}

	return true;
}

} // namespace video
} // namespace irr

#endif

