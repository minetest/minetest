// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageWriterBMP.h"

#ifdef _IRR_COMPILE_WITH_BMP_WRITER_

#include "CImageLoaderBMP.h"
#include "IWriteFile.h"
#include "CColorConverter.h"
#include "irrString.h"

namespace irr
{
namespace video
{

IImageWriter* createImageWriterBMP()
{
	return new CImageWriterBMP;
}

CImageWriterBMP::CImageWriterBMP()
{
#ifdef _DEBUG
	setDebugName("CImageWriterBMP");
#endif
}

bool CImageWriterBMP::isAWriteableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "bmp" );
}

bool CImageWriterBMP::writeImage(io::IWriteFile* file, IImage* image, u32 param) const
{
	// we always write 24-bit color because nothing really reads 32-bit

	SBMPHeader imageHeader;
	imageHeader.Id = 0x4d42;
	imageHeader.Reserved = 0;
	imageHeader.BitmapDataOffset = sizeof(imageHeader);
	imageHeader.BitmapHeaderSize = 0x28;
	imageHeader.Width = image->getDimension().Width;
	imageHeader.Height = image->getDimension().Height;
	imageHeader.Planes = 1;
	imageHeader.BPP = 24;
	imageHeader.Compression = 0;
	imageHeader.PixelPerMeterX = 0;
	imageHeader.PixelPerMeterY = 0;
	imageHeader.Colors = 0;
	imageHeader.ImportantColors = 0;

	// data size is rounded up to next larger 4 bytes boundary
	imageHeader.BitmapDataSize = imageHeader.Width * imageHeader.BPP / 8;
	imageHeader.BitmapDataSize = (imageHeader.BitmapDataSize + 3) & ~3;
	imageHeader.BitmapDataSize *= imageHeader.Height;

	// file size is data size plus offset to data
	imageHeader.FileSize = imageHeader.BitmapDataOffset + imageHeader.BitmapDataSize;

	// bitmaps are stored upside down and padded so we always do this
	void (*CColorConverter_convertFORMATtoFORMAT)(const void*, s32, void*) = 0;
	switch(image->getColorFormat())
	{
	case ECF_R8G8B8:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_R8G8B8toR8G8B8;
		break;
	case ECF_A8R8G8B8:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_A8R8G8B8toB8G8R8;
		break;
	case ECF_A1R5G5B5:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_A1R5G5B5toR8G8B8;
		break;
	case ECF_R5G6B5:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_R5G6B5toR8G8B8;
		break;
#ifndef _DEBUG
	default:
		break;
#endif
	}

	// couldn't find a color converter
	if (!CColorConverter_convertFORMATtoFORMAT)
		return false;

	// write the bitmap header
	if (file->write(&imageHeader, sizeof(imageHeader)) != sizeof(imageHeader))
		return false;

	u8* scan_lines = (u8*)image->lock();
	if (!scan_lines)
		return false;

	// size of one pixel in bytes
	u32 pixel_size = image->getBytesPerPixel();

	// length of one row of the source image in bytes
	u32 row_stride = (pixel_size * imageHeader.Width);

	// length of one row in bytes, rounded up to nearest 4-byte boundary
	s32 row_size = ((3 * imageHeader.Width) + 3) & ~3;

	// allocate and clear memory for our scan line
	u8* row_pointer = new u8[row_size];
	memset(row_pointer, 0, row_size);

	// convert the image to 24-bit BGR and flip it over
	s32 y;
	for (y = imageHeader.Height - 1; 0 <= y; --y)
	{
		if (image->getColorFormat()==ECF_R8G8B8)
			CColorConverter::convert24BitTo24Bit(&scan_lines[y * row_stride], row_pointer, imageHeader.Width, 1, 0, false, true);
		else
			// source, length [pixels], destination
			CColorConverter_convertFORMATtoFORMAT(&scan_lines[y * row_stride], imageHeader.Width, row_pointer);
		if (file->write(row_pointer, row_size) < row_size)
			break;
	}

	// clean up our scratch area
	delete [] row_pointer;

	// give back image handle
	image->unlock();

	return y < 0;
}

} // namespace video
} // namespace irr

#endif

