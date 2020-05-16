// Copyright (C) 2007-2012 Christian Stehno
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CImageLoaderPPM.h"

#ifdef _IRR_COMPILE_WITH_PPM_LOADER_

#include "IReadFile.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "os.h"
#include "fast_atof.h"
#include "coreutil.h"

namespace irr
{
namespace video
{


//! constructor
CImageLoaderPPM::CImageLoaderPPM()
{
	#ifdef _DEBUG
	setDebugName("CImageLoaderPPM");
	#endif
}


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderPPM::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "ppm", "pgm", "pbm" );
}


//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderPPM::isALoadableFileFormat(io::IReadFile* file) const
{
	c8 id[2]={0};
	file->read(&id, 2);
	return (id[0]=='P' && id[1]>'0' && id[1]<'7');
}


//! creates a surface from the file
IImage* CImageLoaderPPM::loadImage(io::IReadFile* file) const
{
	IImage* image;

	if (file->getSize() < 12)
		return 0;

	c8 id[2];
	file->read(&id, 2);

	if (id[0]!='P' || id[1]<'1' || id[1]>'6')
		return 0;

	const u8 format = id[1] - '0';
	const bool binary = format>3;

	core::stringc token;
	getNextToken(file, token);
	const u32 width = core::strtoul10(token.c_str());

	getNextToken(file, token);
	const u32 height = core::strtoul10(token.c_str());

	u8* data = 0;
	const u32 size = width*height;
	if (format==1 || format==4)
	{
		skipToNextToken(file); // go to start of data

		const u32 bytesize = size/8+(size & 3)?1:0;
		if (binary)
		{
			if (file->getSize()-file->getPos() < (long)bytesize)
				return 0;
			data = new u8[bytesize];
			file->read(data, bytesize);
		}
		else
		{
			if (file->getSize()-file->getPos() < (long)(2*size)) // optimistic test
				return 0;
			data = new u8[bytesize];
			memset(data, 0, bytesize);
			u32 shift=0;
			for (u32 i=0; i<size; ++i)
			{
				getNextToken(file, token);
				if (token == "1")
					data[i/8] |= (0x01 << shift);
				if (++shift == 8)
					shift=0;
			}
		}
		image = new CImage(ECF_A1R5G5B5, core::dimension2d<u32>(width, height));
		if (image)
			CColorConverter::convert1BitTo16Bit(data, (s16*)image->lock(), width, height);
	}
	else
	{
		getNextToken(file, token);
		const u32 maxDepth = core::strtoul10(token.c_str());
		if (maxDepth > 255) // no double bytes yet
			return 0;

		skipToNextToken(file); // go to start of data

		if (format==2 || format==5)
		{
			if (binary)
			{
				if (file->getSize()-file->getPos() < (long)size)
					return 0;
				data = new u8[size];
				file->read(data, size);
				image = new CImage(ECF_A8R8G8B8, core::dimension2d<u32>(width, height));
				if (image)
				{
					u8* ptr = (u8*)image->lock();
					for (u32 i=0; i<size; ++i)
					{
						*ptr++ = data[i];
						*ptr++ = data[i];
						*ptr++ = data[i];
						*ptr++ = 255;
					}
				}
			}
			else
			{
				if (file->getSize()-file->getPos() < (long)(2*size)) // optimistic test
					return 0;
				image = new CImage(ECF_A8R8G8B8, core::dimension2d<u32>(width, height));
				if (image)
				{
					u8* ptr = (u8*)image->lock();
					for (u32 i=0; i<size; ++i)
					{
						getNextToken(file, token);
						const u8 num = (u8)core::strtoul10(token.c_str());
						*ptr++ = num;
						*ptr++ = num;
						*ptr++ = num;
						*ptr++ = 255;
					}
				}
			}
		}
		else
		{
			const u32 bytesize = 3*size;
			if (binary)
			{
				if (file->getSize()-file->getPos() < (long)bytesize)
					return 0;
				data = new u8[bytesize];
				file->read(data, bytesize);
				image = new CImage(ECF_A8R8G8B8, core::dimension2d<u32>(width, height));
				if (image)
				{
					u8* ptr = (u8*)image->lock();
					for (u32 i=0; i<size; ++i)
					{
						*ptr++ = data[3*i];
						*ptr++ = data[3*i+1];
						*ptr++ = data[3*i+2];
						*ptr++ = 255;
					}
				}
			}
			else
			{
				if (file->getSize()-file->getPos() < (long)(2*bytesize)) // optimistic test
					return 0;
				image = new CImage(ECF_A8R8G8B8, core::dimension2d<u32>(width, height));
				if (image)
				{
					u8* ptr = (u8*)image->lock();
					for (u32 i=0; i<size; ++i)
					{
						getNextToken(file, token);
						*ptr++ = (u8)core::strtoul10(token.c_str());
						getNextToken(file, token);
						*ptr++ = (u8)core::strtoul10(token.c_str());
						getNextToken(file, token);
						*ptr++ = (u8)core::strtoul10(token.c_str());
						*ptr++ = 255;
					}
				}
			}
		}
	}

	if (image)
		image->unlock();

	delete [] data;

	return image;
}


//! read the next token from file
void CImageLoaderPPM::getNextToken(io::IReadFile* file, core::stringc& token) const
{
	token = "";
	c8 c;
	while(file->getPos()<file->getSize())
	{
		file->read(&c, 1);
		if (c=='#')
		{
			while (c!='\n' && c!='\r' && (file->getPos()<file->getSize()))
				file->read(&c, 1);
		}
		else if (!core::isspace(c))
		{
			token.append(c);
			break;
		}
	}
	while(file->getPos()<file->getSize())
	{
		file->read(&c, 1);
		if (c=='#')
		{
			while (c!='\n' && c!='\r' && (file->getPos()<file->getSize()))
				file->read(&c, 1);
		}
		else if (!core::isspace(c))
			token.append(c);
		else
			break;
	}
}


//! skip to next token (skip whitespace)
void CImageLoaderPPM::skipToNextToken(io::IReadFile* file) const
{
	c8 c;
	while(file->getPos()<file->getSize())
	{
		file->read(&c, 1);
		if (c=='#')
		{
			while (c!='\n' && c!='\r' && (file->getPos()<file->getSize()))
				file->read(&c, 1);
		}
		else if (!core::isspace(c))
		{
			file->seek(-1, true); // put back
			break;
		}
	}
}


//! creates a loader which is able to load windows bitmaps
IImageLoader* createImageLoaderPPM()
{
	return new CImageLoaderPPM;
}


} // end namespace video
} // end namespace irr

#endif

