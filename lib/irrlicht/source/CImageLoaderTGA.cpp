// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageLoaderTGA.h"

#ifdef _IRR_COMPILE_WITH_TGA_LOADER_

#include "IReadFile.h"
#include "os.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "irrString.h"


namespace irr
{
namespace video
{


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderTGA::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "tga" );
}


//! loads a compressed tga.
u8 *CImageLoaderTGA::loadCompressedImage(io::IReadFile *file, const STGAHeader& header) const
{
	// This was written and sent in by Jon Pry, thank you very much!
	// I only changed the formatting a little bit.

	s32 bytesPerPixel = header.PixelDepth/8;
	s32 imageSize =  header.ImageHeight * header.ImageWidth * bytesPerPixel;
	u8* data = new u8[imageSize];
	s32 currentByte = 0;

	while(currentByte < imageSize)
	{
		u8 chunkheader = 0;
		file->read(&chunkheader, sizeof(u8)); // Read The Chunk's Header

		if(chunkheader < 128) // If The Chunk Is A 'RAW' Chunk
		{
			chunkheader++; // Add 1 To The Value To Get Total Number Of Raw Pixels

			file->read(&data[currentByte], bytesPerPixel * chunkheader);
			currentByte += bytesPerPixel * chunkheader;
		}
		else
		{
			// thnx to neojzs for some fixes with this code

			// If It's An RLE Header
			chunkheader -= 127; // Subtract 127 To Get Rid Of The ID Bit

			s32 dataOffset = currentByte;
			file->read(&data[dataOffset], bytesPerPixel);

			currentByte += bytesPerPixel;

			for(s32 counter = 1; counter < chunkheader; counter++)
			{
				for(s32 elementCounter=0; elementCounter < bytesPerPixel; elementCounter++)
					data[currentByte + elementCounter] = data[dataOffset + elementCounter];

				currentByte += bytesPerPixel;
			}
		}
	}

	return data;
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderTGA::isALoadableFileFormat(io::IReadFile* file) const
{
	if (!file)
		return false;

	STGAFooter footer;
	memset(&footer, 0, sizeof(STGAFooter));
	file->seek(file->getSize()-sizeof(STGAFooter));
	file->read(&footer, sizeof(STGAFooter));
	return (!strcmp(footer.Signature,"TRUEVISION-XFILE.")); // very old tgas are refused.
}



//! creates a surface from the file
IImage* CImageLoaderTGA::loadImage(io::IReadFile* file) const
{
	STGAHeader header;
	u32 *palette = 0;

	file->read(&header, sizeof(STGAHeader));

#ifdef __BIG_ENDIAN__
	header.ColorMapLength = os::Byteswap::byteswap(header.ColorMapLength);
	header.ImageWidth = os::Byteswap::byteswap(header.ImageWidth);
	header.ImageHeight = os::Byteswap::byteswap(header.ImageHeight);
#endif

	// skip image identification field
	if (header.IdLength)
		file->seek(header.IdLength, true);

	if (header.ColorMapType)
	{
		// create 32 bit palette
		palette = new u32[ header.ColorMapLength];

		// read color map
		u8 * colorMap = new u8[header.ColorMapEntrySize/8 * header.ColorMapLength];
		file->read(colorMap,header.ColorMapEntrySize/8 * header.ColorMapLength);

		// convert to 32-bit palette
		switch ( header.ColorMapEntrySize )
		{
			case 16:
				CColorConverter::convert_A1R5G5B5toA8R8G8B8(colorMap, header.ColorMapLength, palette);
				break;
			case 24:
				CColorConverter::convert_B8G8R8toA8R8G8B8(colorMap, header.ColorMapLength, palette);
				break;
			case 32:
				CColorConverter::convert_B8G8R8A8toA8R8G8B8(colorMap, header.ColorMapLength, palette);
				break;
		}
		delete [] colorMap;
	}

	// read image

	u8* data = 0;

	if (	header.ImageType == 1 || // Uncompressed, color-mapped images.
			header.ImageType == 2 || // Uncompressed, RGB images
			header.ImageType == 3 // Uncompressed, black and white images
		)
	{
		const s32 imageSize = header.ImageHeight * header.ImageWidth * header.PixelDepth/8;
		data = new u8[imageSize];
	  	file->read(data, imageSize);
	}
	else
	if(header.ImageType == 10)
	{
		// Runlength encoded RGB images
		data = loadCompressedImage(file, header);
	}
	else
	{
		os::Printer::log("Unsupported TGA file type", file->getFileName(), ELL_ERROR);
		delete [] palette;
		return 0;
	}

	IImage* image = 0;

	switch(header.PixelDepth)
	{
	case 8:
		{
			if (header.ImageType==3) // grey image
			{
				image = new CImage(ECF_R8G8B8,
					core::dimension2d<u32>(header.ImageWidth, header.ImageHeight));
				if (image)
					CColorConverter::convert8BitTo24Bit((u8*)data,
						(u8*)image->lock(),
						header.ImageWidth,header.ImageHeight,
						0, 0, (header.ImageDescriptor&0x20)==0);
			}
			else
			{
				image = new CImage(ECF_A1R5G5B5,
					core::dimension2d<u32>(header.ImageWidth, header.ImageHeight));
				if (image)
					CColorConverter::convert8BitTo16Bit((u8*)data,
						(s16*)image->lock(),
						header.ImageWidth,header.ImageHeight,
						(s32*) palette, 0,
						(header.ImageDescriptor&0x20)==0);
			}
		}
		break;
	case 16:
		image = new CImage(ECF_A1R5G5B5,
			core::dimension2d<u32>(header.ImageWidth, header.ImageHeight));
		if (image)
			CColorConverter::convert16BitTo16Bit((s16*)data,
				(s16*)image->lock(), header.ImageWidth,	header.ImageHeight, 0, (header.ImageDescriptor&0x20)==0);
		break;
	case 24:
			image = new CImage(ECF_R8G8B8,
				core::dimension2d<u32>(header.ImageWidth, header.ImageHeight));
			if (image)
				CColorConverter::convert24BitTo24Bit(
					(u8*)data, (u8*)image->lock(), header.ImageWidth, header.ImageHeight, 0, (header.ImageDescriptor&0x20)==0, true);
		break;
	case 32:
			image = new CImage(ECF_A8R8G8B8,
				core::dimension2d<u32>(header.ImageWidth, header.ImageHeight));
			if (image)
				CColorConverter::convert32BitTo32Bit((s32*)data,
					(s32*)image->lock(), header.ImageWidth, header.ImageHeight, 0, (header.ImageDescriptor&0x20)==0);
		break;
	default:
		os::Printer::log("Unsupported TGA format", file->getFileName(), ELL_ERROR);
		break;
	}
	if (image)
		image->unlock();

	delete [] data;
	delete [] palette;

	return image;
}


//! creates a loader which is able to load tgas
IImageLoader* createImageLoaderTGA()
{
	return new CImageLoaderTGA();
}


} // end namespace video
} // end namespace irr

#endif

