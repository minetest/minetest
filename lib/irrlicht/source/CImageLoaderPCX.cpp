// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageLoaderPCX.h"

#ifdef _IRR_COMPILE_WITH_PCX_LOADER_

#include "IReadFile.h"
#include "SColor.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "os.h"
#include "irrString.h"


namespace irr
{
namespace video
{


//! constructor
CImageLoaderPCX::CImageLoaderPCX()
{
	#ifdef _DEBUG
	setDebugName("CImageLoaderPCX");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderPCX::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "pcx" );
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderPCX::isALoadableFileFormat(io::IReadFile* file) const
{
	u8 headerID;
	file->read(&headerID, sizeof(headerID));
	return headerID == 0x0a;
}


//! creates a image from the file
IImage* CImageLoaderPCX::loadImage(io::IReadFile* file) const
{
	SPCXHeader header;
	s32* paletteData = 0;

	file->read(&header, sizeof(header));
	#ifdef __BIG_ENDIAN__
		header.XMin = os::Byteswap::byteswap(header.XMin);
		header.YMin = os::Byteswap::byteswap(header.YMin);
		header.XMax = os::Byteswap::byteswap(header.XMax);
		header.YMax = os::Byteswap::byteswap(header.YMax);
		header.HorizDPI = os::Byteswap::byteswap(header.HorizDPI);
		header.VertDPI = os::Byteswap::byteswap(header.VertDPI);
		header.BytesPerLine = os::Byteswap::byteswap(header.BytesPerLine);
		header.PaletteType = os::Byteswap::byteswap(header.PaletteType);
		header.HScrSize = os::Byteswap::byteswap(header.HScrSize);
		header.VScrSize = os::Byteswap::byteswap(header.VScrSize);
	#endif

	//! return if the header is wrong
	if (header.Manufacturer != 0x0a && header.Encoding != 0x01)
		return 0;

	// return if this isn't a supported type
	if ((header.BitsPerPixel != 8) && (header.BitsPerPixel != 4) && (header.BitsPerPixel != 1))
	{
		os::Printer::log("Unsupported bits per pixel in PCX file.",
			file->getFileName(), irr::ELL_WARNING);
		return 0;
	}

	// read palette
	if( (header.BitsPerPixel == 8) && (header.Planes == 1) )
	{
		// the palette indicator (usually a 0x0c is found infront of the actual palette data)
		// is ignored because some exporters seem to forget to write it. This would result in
		// no image loaded before, now only wrong colors will be set.
		const long pos = file->getPos();
		file->seek( file->getSize()-256*3, false );

		u8 *tempPalette = new u8[768];
		paletteData = new s32[256];
		file->read( tempPalette, 768 );

		for( s32 i=0; i<256; i++ )
		{
			paletteData[i] = (0xff000000 |
					(tempPalette[i*3+0] << 16) |
					(tempPalette[i*3+1] << 8) |
					(tempPalette[i*3+2]));
		}

		delete [] tempPalette;

		file->seek(pos);
	}
	else if( header.BitsPerPixel == 4 )
	{
		paletteData = new s32[16];
		for( s32 i=0; i<16; i++ )
		{
			paletteData[i] = (0xff000000 |
					(header.Palette[i*3+0] << 16) |
					(header.Palette[i*3+1] << 8) |
					(header.Palette[i*3+2]));
		}
	}

	// read image data
	const s32 width = header.XMax - header.XMin + 1;
	const s32 height = header.YMax - header.YMin + 1;
	const s32 imagebytes = header.BytesPerLine * header.Planes * header.BitsPerPixel * height / 8;
	u8* PCXData = new u8[imagebytes];

	u8 cnt, value;
	s32 lineoffset=0, linestart=0, nextmode=1;
	for(s32 offset = 0; offset < imagebytes; offset += cnt)
	{
		file->read(&cnt, 1);
		if( !((cnt & 0xc0) == 0xc0) )
		{
			value = cnt;
			cnt = 1;
		}
		else
		{
			cnt &= 0x3f;
			file->read(&value, 1);
		}
		if (header.Planes==1)
			memset(PCXData+offset, value, cnt);
		else
		{
			for (u8 i=0; i<cnt; ++i)
			{
				PCXData[linestart+lineoffset]=value;
				lineoffset += 3;
				if (lineoffset>=3*header.BytesPerLine)
				{
					lineoffset=nextmode;
					if (++nextmode==3)
						nextmode=0;
					if (lineoffset==0)
						linestart += 3*header.BytesPerLine;
				}
			}
		}
	}

	// create image
	video::IImage* image = 0;
	s32 pad = (header.BytesPerLine - width * header.BitsPerPixel / 8) * header.Planes;

	if (pad < 0)
		pad = -pad;

	if (header.BitsPerPixel==8)
	{
		switch(header.Planes) // TODO: Other formats
		{
		case 1:
			image = new CImage(ECF_A1R5G5B5, core::dimension2d<u32>(width, height));
			if (image)
				CColorConverter::convert8BitTo16Bit(PCXData, (s16*)image->lock(), width, height, paletteData, pad);
			break;
		case 3:
			image = new CImage(ECF_R8G8B8, core::dimension2d<u32>(width, height));
			if (image)
				CColorConverter::convert24BitTo24Bit(PCXData, (u8*)image->lock(), width, height, pad);
			break;
		}
	}
	else if (header.BitsPerPixel==4)
	{
		if (header.Planes==1)
		{
			image = new CImage(ECF_A1R5G5B5, core::dimension2d<u32>(width, height));
			if (image)
				CColorConverter::convert4BitTo16Bit(PCXData, (s16*)image->lock(), width, height, paletteData, pad);
		}
	}
	else if (header.BitsPerPixel==1)
	{
		if (header.Planes==4)
		{
			image = new CImage(ECF_A1R5G5B5, core::dimension2d<u32>(width, height));
			if (image)
				CColorConverter::convert4BitTo16Bit(PCXData, (s16*)image->lock(), width, height, paletteData, pad);
		}
		else if (header.Planes==1)
		{
			image = new CImage(ECF_A1R5G5B5, core::dimension2d<u32>(width, height));
			if (image)
				CColorConverter::convert1BitTo16Bit(PCXData, (s16*)image->lock(), width, height, pad);
		}
	}
	if (image)
		image->unlock();

	// clean up

	delete [] paletteData;
	delete [] PCXData;

	return image;
}


//! creates a loader which is able to load pcx images
IImageLoader* createImageLoaderPCX()
{
	return new CImageLoaderPCX();
}



} // end namespace video
} // end namespace irr

#endif

