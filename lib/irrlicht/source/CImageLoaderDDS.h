// Copyright (C) 2002-2012 Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_IMAGE_LOADER_DDS_H_INCLUDED__
#define __C_IMAGE_LOADER_DDS_H_INCLUDED__

#include "IrrCompileConfig.h"

#if defined(_IRR_COMPILE_WITH_DDS_LOADER_)

#include "IImageLoader.h"

namespace irr
{
namespace video
{

/* dependencies */
/* dds definition */
enum eDDSPixelFormat
{
	DDS_PF_ARGB8888,
	DDS_PF_DXT1,
	DDS_PF_DXT2,
	DDS_PF_DXT3,
	DDS_PF_DXT4,
	DDS_PF_DXT5,
	DDS_PF_UNKNOWN
};

/* 16bpp stuff */
#define DDS_LOW_5		0x001F;
#define DDS_MID_6		0x07E0;
#define DDS_HIGH_5		0xF800;
#define DDS_MID_555		0x03E0;
#define DDS_HI_555		0x7C00;


// byte-align structures
#include "irrpack.h"

/* structures */
struct ddsColorKey
{
	u32		colorSpaceLowValue;
	u32		colorSpaceHighValue;
} PACK_STRUCT;

struct ddsCaps
{
	u32		caps1;
	u32		caps2;
	u32		caps3;
	u32		caps4;
} PACK_STRUCT;

struct ddsMultiSampleCaps
{
	u16		flipMSTypes;
	u16		bltMSTypes;
} PACK_STRUCT;


struct ddsPixelFormat
{
	u32		size;
	u32		flags;
	u32		fourCC;
	union
	{
		u32	rgbBitCount;
		u32	yuvBitCount;
		u32	zBufferBitDepth;
		u32	alphaBitDepth;
		u32	luminanceBitCount;
		u32	bumpBitCount;
		u32	privateFormatBitCount;
	};
	union
	{
		u32	rBitMask;
		u32	yBitMask;
		u32	stencilBitDepth;
		u32	luminanceBitMask;
		u32	bumpDuBitMask;
		u32	operations;
	};
	union
	{
		u32	gBitMask;
		u32	uBitMask;
		u32	zBitMask;
		u32	bumpDvBitMask;
		ddsMultiSampleCaps	multiSampleCaps;
	};
	union
	{
		u32	bBitMask;
		u32	vBitMask;
		u32	stencilBitMask;
		u32	bumpLuminanceBitMask;
	};
	union
	{
		u32	rgbAlphaBitMask;
		u32	yuvAlphaBitMask;
		u32	luminanceAlphaBitMask;
		u32	rgbZBitMask;
		u32	yuvZBitMask;
	};
} PACK_STRUCT;


struct ddsBuffer
{
	/* magic: 'dds ' */
	c8				magic[ 4 ];

	/* directdraw surface */
	u32		size;
	u32		flags;
	u32		height;
	u32		width;
	union
	{
		s32				pitch;
		u32	linearSize;
	};
	u32		backBufferCount;
	union
	{
		u32	mipMapCount;
		u32	refreshRate;
		u32	srcVBHandle;
	};
	u32		alphaBitDepth;
	u32		reserved;
	void				*surface;
	union
	{
		ddsColorKey	ckDestOverlay;
		u32	emptyFaceColor;
	};
	ddsColorKey		ckDestBlt;
	ddsColorKey		ckSrcOverlay;
	ddsColorKey		ckSrcBlt;
	union
	{
		ddsPixelFormat	pixelFormat;
		u32	fvf;
	};
	ddsCaps			caps;
	u32		textureStage;

	/* data (Varying size) */
	u8		data[ 4 ];
} PACK_STRUCT;


struct ddsColorBlock
{
	u16		colors[ 2 ];
	u8		row[ 4 ];
} PACK_STRUCT;


struct ddsAlphaBlockExplicit
{
	u16		row[ 4 ];
} PACK_STRUCT;


struct ddsAlphaBlock3BitLinear
{
	u8		alpha0;
	u8		alpha1;
	u8		stuff[ 6 ];
} PACK_STRUCT;


struct ddsColor
{
	u8		r, g, b, a;
} PACK_STRUCT;

// Default alignment
#include "irrunpack.h"


/* endian tomfoolery */
typedef union
{
	f32	f;
	c8	c[ 4 ];
}
floatSwapUnion;


#ifndef __BIG_ENDIAN__
#ifdef _SGI_SOURCE
#define	__BIG_ENDIAN__
#endif
#endif


#ifdef __BIG_ENDIAN__

	s32   DDSBigLong( s32 src ) { return src; }
	s16 DDSBigShort( s16 src ) { return src; }
	f32 DDSBigFloat( f32 src ) { return src; }

	s32 DDSLittleLong( s32 src )
	{
		return ((src & 0xFF000000) >> 24) |
			((src & 0x00FF0000) >> 8) |
			((src & 0x0000FF00) << 8) |
			((src & 0x000000FF) << 24);
	}

	s16 DDSLittleShort( s16 src )
	{
		return ((src & 0xFF00) >> 8) |
			((src & 0x00FF) << 8);
	}

	f32 DDSLittleFloat( f32 src )
	{
		floatSwapUnion in,out;
		in.f = src;
		out.c[ 0 ] = in.c[ 3 ];
		out.c[ 1 ] = in.c[ 2 ];
		out.c[ 2 ] = in.c[ 1 ];
		out.c[ 3 ] = in.c[ 0 ];
		return out.f;
	}

#else /*__BIG_ENDIAN__*/

	s32   DDSLittleLong( s32 src ) { return src; }
	s16 DDSLittleShort( s16 src ) { return src; }
	f32 DDSLittleFloat( f32 src ) { return src; }

	s32 DDSBigLong( s32 src )
	{
		return ((src & 0xFF000000) >> 24) |
			((src & 0x00FF0000) >> 8) |
			((src & 0x0000FF00) << 8) |
			((src & 0x000000FF) << 24);
	}

	s16 DDSBigShort( s16 src )
	{
		return ((src & 0xFF00) >> 8) |
			((src & 0x00FF) << 8);
	}

	f32 DDSBigFloat( f32 src )
	{
		floatSwapUnion in,out;
		in.f = src;
		out.c[ 0 ] = in.c[ 3 ];
		out.c[ 1 ] = in.c[ 2 ];
		out.c[ 2 ] = in.c[ 1 ];
		out.c[ 3 ] = in.c[ 0 ];
		return out.f;
	}

#endif /*__BIG_ENDIAN__*/


/*!
	Surface Loader for DDS images
*/
class CImageLoaderDDS : public IImageLoader
{
public:

	//! returns true if the file maybe is able to be loaded by this class
	//! based on the file extension (e.g. ".tga")
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! returns true if the file maybe is able to be loaded by this class
	virtual bool isALoadableFileFormat(io::IReadFile* file) const;

	//! creates a surface from the file
	virtual IImage* loadImage(io::IReadFile* file) const;
};


} // end namespace video
} // end namespace irr

#endif // compiled with DDS loader
#endif

