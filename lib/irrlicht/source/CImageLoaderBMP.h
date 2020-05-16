// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IMAGE_LOADER_BMP_H_INCLUDED__
#define __C_IMAGE_LOADER_BMP_H_INCLUDED__

#include "IrrCompileConfig.h"

#include "IImageLoader.h"


namespace irr
{
namespace video
{

#if defined(_IRR_COMPILE_WITH_BMP_LOADER_) || defined(_IRR_COMPILE_WITH_BMP_WRITER_)


// byte-align structures
#include "irrpack.h"

	struct SBMPHeader
	{
		u16	Id;					//	BM - Windows 3.1x, 95, NT, 98, 2000, ME, XP
											//	BA - OS/2 Bitmap Array
											//	CI - OS/2 Color Icon
											//	CP - OS/2 Color Pointer
											//	IC - OS/2 Icon
											//	PT - OS/2 Pointer
		u32	FileSize;
		u32	Reserved;
		u32	BitmapDataOffset;
		u32	BitmapHeaderSize;	// should be 28h for windows bitmaps or
											// 0Ch for OS/2 1.x or F0h for OS/2 2.x
		u32 Width;
		u32 Height;
		u16 Planes;
		u16 BPP;					// 1: Monochrome bitmap
											// 4: 16 color bitmap
											// 8: 256 color bitmap
											// 16: 16bit (high color) bitmap
											// 24: 24bit (true color) bitmap
											// 32: 32bit (true color) bitmap

		u32  Compression;			// 0: none (Also identified by BI_RGB)
											// 1: RLE 8-bit / pixel (Also identified by BI_RLE4)
											// 2: RLE 4-bit / pixel (Also identified by BI_RLE8)
											// 3: Bitfields  (Also identified by BI_BITFIELDS)

		u32  BitmapDataSize;		// Size of the bitmap data in bytes. This number must be rounded to the next 4 byte boundary.
		u32  PixelPerMeterX;
		u32  PixelPerMeterY;
		u32  Colors;
		u32  ImportantColors;
	} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"

#endif // defined with loader or writer

#ifdef _IRR_COMPILE_WITH_BMP_LOADER_

/*!
	Surface Loader for Windows bitmaps
*/
class CImageLoaderBMP : public IImageLoader
{
public:

	//! constructor
	CImageLoaderBMP();

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! returns true if the file maybe is able to be loaded by this class
	virtual bool isALoadableFileFormat(io::IReadFile* file) const;

	//! creates a surface from the file
	virtual IImage* loadImage(io::IReadFile* file) const;

private:

	void decompress8BitRLE(u8*& BmpData, s32 size, s32 width, s32 height, s32 pitch) const;

	void decompress4BitRLE(u8*& BmpData, s32 size, s32 width, s32 height, s32 pitch) const;
};


#endif // compiled with loader

} // end namespace video
} // end namespace irr

#endif

