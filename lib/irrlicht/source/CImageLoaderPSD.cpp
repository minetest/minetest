// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageLoaderPSD.h"

#ifdef _IRR_COMPILE_WITH_PSD_LOADER_

#include "IReadFile.h"
#include "os.h"
#include "CImage.h"
#include "irrString.h"


namespace irr
{
namespace video
{


//! constructor
CImageLoaderPSD::CImageLoaderPSD()
{
	#ifdef _DEBUG
	setDebugName("CImageLoaderPSD");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderPSD::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "psd" );
}



//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderPSD::isALoadableFileFormat(io::IReadFile* file) const
{
	if (!file)
		return false;

	u8 type[3];
	file->read(&type, sizeof(u8)*3);
	return (type[2]==2); // we currently only handle tgas of type 2.
}



//! creates a surface from the file
IImage* CImageLoaderPSD::loadImage(io::IReadFile* file) const
{
	u32* imageData = 0;

	PsdHeader header;
	file->read(&header, sizeof(PsdHeader));

#ifndef __BIG_ENDIAN__
	header.version = os::Byteswap::byteswap(header.version);
	header.channels = os::Byteswap::byteswap(header.channels);
	header.height = os::Byteswap::byteswap(header.height);
	header.width = os::Byteswap::byteswap(header.width);
	header.depth = os::Byteswap::byteswap(header.depth);
	header.mode = os::Byteswap::byteswap(header.mode);
#endif

	if (header.signature[0] != '8' ||
		header.signature[1] != 'B' ||
		header.signature[2] != 'P' ||
		header.signature[3] != 'S')
		return 0;

	if (header.version != 1)
	{
		os::Printer::log("Unsupported PSD file version", file->getFileName(), ELL_ERROR);
		return 0;
	}

	if (header.mode != 3 || header.depth != 8)
	{
		os::Printer::log("Unsupported PSD color mode or depth.\n", file->getFileName(), ELL_ERROR);
		return 0;
	}

	// skip color mode data

	u32 l;
	file->read(&l, sizeof(u32));
#ifndef __BIG_ENDIAN__
	l = os::Byteswap::byteswap(l);
#endif
	if (!file->seek(l, true))
	{
		os::Printer::log("Error seeking file pos to image resources.\n", file->getFileName(), ELL_ERROR);
		return 0;
	}

	// skip image resources

	file->read(&l, sizeof(u32));
#ifndef __BIG_ENDIAN__
	l = os::Byteswap::byteswap(l);
#endif
	if (!file->seek(l, true))
	{
		os::Printer::log("Error seeking file pos to layer and mask.\n", file->getFileName(), ELL_ERROR);
		return 0;
	}

	// skip layer & mask

	file->read(&l, sizeof(u32));
#ifndef __BIG_ENDIAN__
	l = os::Byteswap::byteswap(l);
#endif
	if (!file->seek(l, true))
	{
		os::Printer::log("Error seeking file pos to image data section.\n", file->getFileName(), ELL_ERROR);
		return 0;
	}

	// read image data

	u16 compressionType;
	file->read(&compressionType, sizeof(u16));
#ifndef __BIG_ENDIAN__
	compressionType = os::Byteswap::byteswap(compressionType);
#endif

	if (compressionType != 1 && compressionType != 0)
	{
		os::Printer::log("Unsupported psd compression mode.\n", file->getFileName(), ELL_ERROR);
		return 0;
	}

	// create image data block

	imageData = new u32[header.width * header.height];

	bool res = false;

	if (compressionType == 0)
		res = readRawImageData(file, header, imageData); // RAW image data
	else
		res = readRLEImageData(file, header, imageData); // RLE compressed data

	video::IImage* image = 0;

	if (res)
	{
		// create surface
		image = new CImage(ECF_A8R8G8B8,
			core::dimension2d<u32>(header.width, header.height), imageData);
	}

	if (!image)
		delete [] imageData;
	imageData = 0;

	return image;
}


bool CImageLoaderPSD::readRawImageData(io::IReadFile* file, const PsdHeader& header, u32* imageData) const
{
	u8* tmpData = new u8[header.width * header.height];

	for (s32 channel=0; channel<header.channels && channel < 3; ++channel)
	{
		if (!file->read(tmpData, sizeof(c8) * header.width * header.height))
		{
			os::Printer::log("Error reading color channel\n", file->getFileName(), ELL_ERROR);
			break;
		}

		s16 shift = getShiftFromChannel((c8)channel, header);
		if (shift != -1)
		{
			u32 mask = 0xff << shift;

			for (u32 x=0; x<header.width; ++x)
			{
				for (u32 y=0; y<header.height; ++y)
				{
					s32 index = x + y*header.width;
					imageData[index] = ~(~imageData[index] | mask);
					imageData[index] |= tmpData[index] << shift;
				}
			}
		}

	}

	delete [] tmpData;
	return true;
}


bool CImageLoaderPSD::readRLEImageData(io::IReadFile* file, const PsdHeader& header, u32* imageData) const
{
	/*	If the compression code is 1, the image data
		starts with the byte counts for all the scan lines in the channel
		(LayerBottom LayerTop), with each count stored as a two
		byte value. The RLE compressed data follows, with each scan line
		compressed separately. The RLE compression is the same compres-sion
		algorithm used by the Macintosh ROM routine PackBits, and
		the TIFF standard.
		If the Layer's Size, and therefore the data, is odd, a pad byte will
		be inserted at the end of the row.
	*/

	/*
	A pseudo code fragment to unpack might look like this:

	Loop until you get the number of unpacked bytes you are expecting:
		Read the next source byte into n.
		If n is between 0 and 127 inclusive, copy the next n+1 bytes literally.
		Else if n is between -127 and -1 inclusive, copy the next byte -n+1
		times.
		Else if n is -128, noop.
	Endloop

	In the inverse routine, it is best to encode a 2-byte repeat run as a replicate run
	except when preceded and followed by a literal run. In that case, it is best to merge
	the three runs into one literal run. Always encode 3-byte repeats as replicate runs.
	That is the essence of the algorithm. Here are some additional rules:
	- Pack each row separately. Do not compress across row boundaries.
	- The number of uncompressed bytes per row is defined to be (ImageWidth + 7)
	/ 8. If the uncompressed bitmap is required to have an even number of bytes per
	row, decompress into word-aligned buffers.
	- If a run is larger than 128 bytes, encode the remainder of the run as one or more
	additional replicate runs.
	When PackBits data is decompressed, the result should be interpreted as per com-pression
	type 1 (no compression).
	*/

	u8* tmpData = new u8[header.width * header.height];
	u16 *rleCount= new u16 [header.height * header.channels];

	s32 size=0;

	for (u32 y=0; y<header.height * header.channels; ++y)
	{
		if (!file->read(&rleCount[y], sizeof(u16)))
		{
			delete [] tmpData;
			delete [] rleCount;
			os::Printer::log("Error reading rle rows\n", file->getFileName(), ELL_ERROR);
			return false;
		}

#ifndef __BIG_ENDIAN__
		rleCount[y] = os::Byteswap::byteswap(rleCount[y]);
#endif
		size += rleCount[y];
	}

	s8 *buf = new s8[size];
	if (!file->read(buf, size))
	{
		delete [] rleCount;
		delete [] buf;
		delete [] tmpData;
		os::Printer::log("Error reading rle rows\n", file->getFileName(), ELL_ERROR);
		return false;
	}

	u16 *rcount=rleCount;

	s8 rh;
	u16 bytesRead;
	u8 *dest;
	s8 *pBuf = buf;

	// decompress packbit rle

	for (s32 channel=0; channel<header.channels; channel++)
	{
		for (u32 y=0; y<header.height; ++y, ++rcount)
		{
			bytesRead=0;
			dest = &tmpData[y*header.width];

			while (bytesRead < *rcount)
			{
				rh = *pBuf++;
				++bytesRead;

				if (rh >= 0)
				{
					++rh;

					while (rh--)
					{
						*dest = *pBuf++;
						++bytesRead;
						++dest;
					}
				}
				else
				if (rh > -128)
				{
					rh = -rh +1;

					while (rh--)
					{
						*dest = *pBuf;
						++dest;
					}

					++pBuf;
					++bytesRead;
				}
			}
		}

		s16 shift = getShiftFromChannel((c8)channel, header);

		if (shift != -1)
		{
			u32 mask = 0xff << shift;

			for (u32 x=0; x<header.width; ++x)
				for (u32 y=0; y<header.height; ++y)
				{
					s32 index = x + y*header.width;
					imageData[index] = ~(~imageData[index] | mask);
					imageData[index] |= tmpData[index] << shift;
				}
		}
	}

	delete [] rleCount;
	delete [] buf;
	delete [] tmpData;

	return true;
}


s16 CImageLoaderPSD::getShiftFromChannel(c8 channelNr, const PsdHeader& header) const
{
	switch(channelNr)
	{
	case 0:
		return 16;  // red
	case 1:
		return 8;   // green
	case 2:
		return 0;   // blue
	case 3:
		return header.channels == 4 ? 24 : -1;	// ?
	case 4:
		return 24;  // alpha
	default:
		return -1;
	}
}



//! creates a loader which is able to load tgas
IImageLoader* createImageLoaderPSD()
{
	return new CImageLoaderPSD();
}


} // end namespace video
} // end namespace irr

#endif

