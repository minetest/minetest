// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageWriterTGA.h"

#ifdef _IRR_COMPILE_WITH_TGA_WRITER_

#include "CImageLoaderTGA.h"
#include "IWriteFile.h"
#include "CColorConverter.h"
#include "irrString.h"

namespace irr
{
namespace video
{

IImageWriter* createImageWriterTGA()
{
	return new CImageWriterTGA;
}

CImageWriterTGA::CImageWriterTGA()
{
#ifdef _DEBUG
	setDebugName("CImageWriterTGA");
#endif
}

bool CImageWriterTGA::isAWriteableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "tga" );
}

bool CImageWriterTGA::writeImage(io::IWriteFile *file, IImage *image,u32 param) const
{
	STGAHeader imageHeader;
	imageHeader.IdLength = 0;
	imageHeader.ColorMapType = 0;
	imageHeader.ImageType = 2;
	imageHeader.FirstEntryIndex[0] = 0;
	imageHeader.FirstEntryIndex[1] = 0;
	imageHeader.ColorMapLength = 0;
	imageHeader.ColorMapEntrySize = 0;
	imageHeader.XOrigin[0] = 0;
	imageHeader.XOrigin[1] = 0;
	imageHeader.YOrigin[0] = 0;
	imageHeader.YOrigin[1] = 0;
	imageHeader.ImageWidth = image->getDimension().Width;
	imageHeader.ImageHeight = image->getDimension().Height;

	// top left of image is the top. the image loader needs to
	// be fixed to only swap/flip
	imageHeader.ImageDescriptor = (1 << 5);

   // chances are good we'll need to swizzle data, so i'm going
	// to convert and write one scan line at a time. it's also
	// a bit cleaner this way
	void (*CColorConverter_convertFORMATtoFORMAT)(const void*, s32, void*) = 0;
	switch(image->getColorFormat())
	{
	case ECF_A8R8G8B8:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_A8R8G8B8toA8R8G8B8;
		imageHeader.PixelDepth = 32;
		imageHeader.ImageDescriptor |= 8;
		break;
	case ECF_A1R5G5B5:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_A1R5G5B5toA1R5G5B5;
		imageHeader.PixelDepth = 16;
		imageHeader.ImageDescriptor |= 1;
		break;
	case ECF_R5G6B5:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_R5G6B5toA1R5G5B5;
		imageHeader.PixelDepth = 16;
		imageHeader.ImageDescriptor |= 1;
		break;
	case ECF_R8G8B8:
		CColorConverter_convertFORMATtoFORMAT
			= CColorConverter::convert_R8G8B8toR8G8B8;
		imageHeader.PixelDepth = 24;
		imageHeader.ImageDescriptor |= 0;
		break;
#ifndef _DEBUG
	default:
		break;
#endif
	}

	// couldn't find a color converter
	if (!CColorConverter_convertFORMATtoFORMAT)
		return false;

	if (file->write(&imageHeader, sizeof(imageHeader)) != sizeof(imageHeader))
		return false;

	u8* scan_lines = (u8*)image->lock();
	if (!scan_lines)
		return false;

	// size of one pixel in bytes
	u32 pixel_size = image->getBytesPerPixel();

	// length of one row of the source image in bytes
	u32 row_stride = (pixel_size * imageHeader.ImageWidth);

	// length of one output row in bytes
	s32 row_size = ((imageHeader.PixelDepth / 8) * imageHeader.ImageWidth);

	// allocate a row do translate data into
	u8* row_pointer = new u8[row_size];

	u32 y;
	for (y = 0; y < imageHeader.ImageHeight; ++y)
	{
		// source, length [pixels], destination
		if (image->getColorFormat()==ECF_R8G8B8)
			CColorConverter::convert24BitTo24Bit(&scan_lines[y * row_stride], row_pointer, imageHeader.ImageWidth, 1, 0, 0, true);
		else
			CColorConverter_convertFORMATtoFORMAT(&scan_lines[y * row_stride], imageHeader.ImageWidth, row_pointer);
		if (file->write(row_pointer, row_size) != row_size)
			break;
	}

	delete [] row_pointer;

	image->unlock();

	STGAFooter imageFooter;
	imageFooter.ExtensionOffset = 0;
	imageFooter.DeveloperOffset = 0;
	strncpy(imageFooter.Signature, "TRUEVISION-XFILE.", 18);

	if (file->write(&imageFooter, sizeof(imageFooter)) < (s32)sizeof(imageFooter))
		return false;

	return imageHeader.ImageHeight <= y;
}

} // namespace video
} // namespace irr

#endif

