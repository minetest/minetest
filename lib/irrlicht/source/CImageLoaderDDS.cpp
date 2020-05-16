// Copyright (C) 2002-2012 Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

/*
	Based on Code from Copyright (c) 2003 Randy Reddig
	Based on code from Nvidia's DDS example:
	http://www.nvidia.com/object/dxtc_decompression_code.html

	mainly c to cpp
*/


#include "CImageLoaderDDS.h"

#ifdef _IRR_COMPILE_WITH_DDS_LOADER_

#include "IReadFile.h"
#include "os.h"
#include "CColorConverter.h"
#include "CImage.h"
#include "irrString.h"


namespace irr
{

namespace video
{

namespace
{

/*!
	DDSDecodePixelFormat()
	determines which pixel format the dds texture is in
*/
void DDSDecodePixelFormat( ddsBuffer *dds, eDDSPixelFormat *pf )
{
	/* dummy check */
	if(	dds == NULL || pf == NULL )
		return;

	/* extract fourCC */
	const u32 fourCC = dds->pixelFormat.fourCC;

	/* test it */
	if( fourCC == 0 )
		*pf = DDS_PF_ARGB8888;
	else if( fourCC == *((u32*) "DXT1") )
		*pf = DDS_PF_DXT1;
	else if( fourCC == *((u32*) "DXT2") )
		*pf = DDS_PF_DXT2;
	else if( fourCC == *((u32*) "DXT3") )
		*pf = DDS_PF_DXT3;
	else if( fourCC == *((u32*) "DXT4") )
		*pf = DDS_PF_DXT4;
	else if( fourCC == *((u32*) "DXT5") )
		*pf = DDS_PF_DXT5;
	else
		*pf = DDS_PF_UNKNOWN;
}


/*!
DDSGetInfo()
extracts relevant info from a dds texture, returns 0 on success
*/
s32 DDSGetInfo( ddsBuffer *dds, s32 *width, s32 *height, eDDSPixelFormat *pf )
{
	/* dummy test */
	if( dds == NULL )
		return -1;

	/* test dds header */
	if( *((s32*) dds->magic) != *((s32*) "DDS ") )
		return -1;
	if( DDSLittleLong( dds->size ) != 124 )
		return -1;

	/* extract width and height */
	if( width != NULL )
		*width = DDSLittleLong( dds->width );
	if( height != NULL )
		*height = DDSLittleLong( dds->height );

	/* get pixel format */
	DDSDecodePixelFormat( dds, pf );

	/* return ok */
	return 0;
}


/*!
	DDSGetColorBlockColors()
	extracts colors from a dds color block
*/
void DDSGetColorBlockColors( ddsColorBlock *block, ddsColor colors[ 4 ] )
{
	u16		word;


	/* color 0 */
	word = DDSLittleShort( block->colors[ 0 ] );
	colors[ 0 ].a = 0xff;

	/* extract rgb bits */
	colors[ 0 ].b = (u8) word;
	colors[ 0 ].b <<= 3;
	colors[ 0 ].b |= (colors[ 0 ].b >> 5);
	word >>= 5;
	colors[ 0 ].g = (u8) word;
	colors[ 0 ].g <<= 2;
	colors[ 0 ].g |= (colors[ 0 ].g >> 5);
	word >>= 6;
	colors[ 0 ].r = (u8) word;
	colors[ 0 ].r <<= 3;
	colors[ 0 ].r |= (colors[ 0 ].r >> 5);

	/* same for color 1 */
	word = DDSLittleShort( block->colors[ 1 ] );
	colors[ 1 ].a = 0xff;

	/* extract rgb bits */
	colors[ 1 ].b = (u8) word;
	colors[ 1 ].b <<= 3;
	colors[ 1 ].b |= (colors[ 1 ].b >> 5);
	word >>= 5;
	colors[ 1 ].g = (u8) word;
	colors[ 1 ].g <<= 2;
	colors[ 1 ].g |= (colors[ 1 ].g >> 5);
	word >>= 6;
	colors[ 1 ].r = (u8) word;
	colors[ 1 ].r <<= 3;
	colors[ 1 ].r |= (colors[ 1 ].r >> 5);

	/* use this for all but the super-freak math method */
	if( block->colors[ 0 ] > block->colors[ 1 ] )
	{
		/* four-color block: derive the other two colors.
		00 = color 0, 01 = color 1, 10 = color 2, 11 = color 3
		these two bit codes correspond to the 2-bit fields
		stored in the 64-bit block. */

		word = ((u16) colors[ 0 ].r * 2 + (u16) colors[ 1 ].r ) / 3;
		/* no +1 for rounding */
		/* as bits have been shifted to 888 */
		colors[ 2 ].r = (u8) word;
		word = ((u16) colors[ 0 ].g * 2 + (u16) colors[ 1 ].g) / 3;
		colors[ 2 ].g = (u8) word;
		word = ((u16) colors[ 0 ].b * 2 + (u16) colors[ 1 ].b) / 3;
		colors[ 2 ].b = (u8) word;
		colors[ 2 ].a = 0xff;

		word = ((u16) colors[ 0 ].r + (u16) colors[ 1 ].r * 2) / 3;
		colors[ 3 ].r = (u8) word;
		word = ((u16) colors[ 0 ].g + (u16) colors[ 1 ].g * 2) / 3;
		colors[ 3 ].g = (u8) word;
		word = ((u16) colors[ 0 ].b + (u16) colors[ 1 ].b * 2) / 3;
		colors[ 3 ].b = (u8) word;
		colors[ 3 ].a = 0xff;
	}
	else
	{
		/* three-color block: derive the other color.
		00 = color 0, 01 = color 1, 10 = color 2,
		11 = transparent.
		These two bit codes correspond to the 2-bit fields
		stored in the 64-bit block */

		word = ((u16) colors[ 0 ].r + (u16) colors[ 1 ].r) / 2;
		colors[ 2 ].r = (u8) word;
		word = ((u16) colors[ 0 ].g + (u16) colors[ 1 ].g) / 2;
		colors[ 2 ].g = (u8) word;
		word = ((u16) colors[ 0 ].b + (u16) colors[ 1 ].b) / 2;
		colors[ 2 ].b = (u8) word;
		colors[ 2 ].a = 0xff;

		/* random color to indicate alpha */
		colors[ 3 ].r = 0x00;
		colors[ 3 ].g = 0xff;
		colors[ 3 ].b = 0xff;
		colors[ 3 ].a = 0x00;
	}
}


/*
DDSDecodeColorBlock()
decodes a dds color block
fixme: make endian-safe
*/

void DDSDecodeColorBlock( u32 *pixel, ddsColorBlock *block, s32 width, u32 colors[ 4 ] )
{
	s32				r, n;
	u32	bits;
	u32	masks[] = { 3, 12, 3 << 4, 3 << 6 };	/* bit masks = 00000011, 00001100, 00110000, 11000000 */
	s32				shift[] = { 0, 2, 4, 6 };


	/* r steps through lines in y */
	for( r = 0; r < 4; r++, pixel += (width - 4) )	/* no width * 4 as u32 ptr inc will * 4 */
	{
		/* width * 4 bytes per pixel per line, each j dxtc row is 4 lines of pixels */

		/* n steps through pixels */
		for( n = 0; n < 4; n++ )
		{
			bits = block->row[ r ] & masks[ n ];
			bits >>= shift[ n ];

			switch( bits )
			{
			case 0:
				*pixel = colors[ 0 ];
				pixel++;
				break;

			case 1:
				*pixel = colors[ 1 ];
				pixel++;
				break;

			case 2:
				*pixel = colors[ 2 ];
				pixel++;
				break;

			case 3:
				*pixel = colors[ 3 ];
				pixel++;
				break;

			default:
				/* invalid */
				pixel++;
				break;
			}
		}
	}
}


/*
DDSDecodeAlphaExplicit()
decodes a dds explicit alpha block
*/
void DDSDecodeAlphaExplicit( u32 *pixel, ddsAlphaBlockExplicit *alphaBlock, s32 width, u32 alphaZero )
{
	s32				row, pix;
	u16	word;
	ddsColor		color;


	/* clear color */
	color.r = 0;
	color.g = 0;
	color.b = 0;

	/* walk rows */
	for( row = 0; row < 4; row++, pixel += (width - 4) )
	{
		word = DDSLittleShort( alphaBlock->row[ row ] );

		/* walk pixels */
		for( pix = 0; pix < 4; pix++ )
		{
			/* zero the alpha bits of image pixel */
			*pixel &= alphaZero;
			color.a = word & 0x000F;
			color.a = color.a | (color.a << 4);
			*pixel |= *((u32*) &color);
			word >>= 4;		/* move next bits to lowest 4 */
			pixel++;		/* move to next pixel in the row */
		}
	}
}



/*
DDSDecodeAlpha3BitLinear()
decodes interpolated alpha block
*/
void DDSDecodeAlpha3BitLinear( u32 *pixel, ddsAlphaBlock3BitLinear *alphaBlock, s32 width, u32 alphaZero )
{

	s32 row, pix;
	u32 stuff;
	u8 bits[ 4 ][ 4 ];
	u16 alphas[ 8 ];
	ddsColor aColors[ 4 ][ 4 ];

	/* get initial alphas */
	alphas[ 0 ] = alphaBlock->alpha0;
	alphas[ 1 ] = alphaBlock->alpha1;

	/* 8-alpha block */
	if( alphas[ 0 ] > alphas[ 1 ] )
	{
		/* 000 = alpha_0, 001 = alpha_1, others are interpolated */
		alphas[ 2 ] = ( 6 * alphas[ 0 ] +     alphas[ 1 ]) / 7;	/* bit code 010 */
		alphas[ 3 ] = ( 5 * alphas[ 0 ] + 2 * alphas[ 1 ]) / 7;	/* bit code 011 */
		alphas[ 4 ] = ( 4 * alphas[ 0 ] + 3 * alphas[ 1 ]) / 7;	/* bit code 100 */
		alphas[ 5 ] = ( 3 * alphas[ 0 ] + 4 * alphas[ 1 ]) / 7;	/* bit code 101 */
		alphas[ 6 ] = ( 2 * alphas[ 0 ] + 5 * alphas[ 1 ]) / 7;	/* bit code 110 */
		alphas[ 7 ] = (     alphas[ 0 ] + 6 * alphas[ 1 ]) / 7;	/* bit code 111 */
	}

	/* 6-alpha block */
	else
	{
		/* 000 = alpha_0, 001 = alpha_1, others are interpolated */
		alphas[ 2 ] = (4 * alphas[ 0 ] +     alphas[ 1 ]) / 5;	/* bit code 010 */
		alphas[ 3 ] = (3 * alphas[ 0 ] + 2 * alphas[ 1 ]) / 5;	/* bit code 011 */
		alphas[ 4 ] = (2 * alphas[ 0 ] + 3 * alphas[ 1 ]) / 5;	/* bit code 100 */
		alphas[ 5 ] = (    alphas[ 0 ] + 4 * alphas[ 1 ]) / 5;	/* bit code 101 */
		alphas[ 6 ] = 0;										/* bit code 110 */
		alphas[ 7 ] = 255;										/* bit code 111 */
	}

	/* decode 3-bit fields into array of 16 bytes with same value */

	/* first two rows of 4 pixels each */
	stuff = *((u32*) &(alphaBlock->stuff[ 0 ]));

	bits[ 0 ][ 0 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 0 ][ 1 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 0 ][ 2 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 0 ][ 3 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 1 ][ 0 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 1 ][ 1 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 1 ][ 2 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 1 ][ 3 ] = (u8) (stuff & 0x00000007);

	/* last two rows */
	stuff = *((u32*) &(alphaBlock->stuff[ 3 ])); /* last 3 bytes */

	bits[ 2 ][ 0 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 2 ][ 1 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 2 ][ 2 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 2 ][ 3 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 3 ][ 0 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 3 ][ 1 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 3 ][ 2 ] = (u8) (stuff & 0x00000007);
	stuff >>= 3;
	bits[ 3 ][ 3 ] = (u8) (stuff & 0x00000007);

	/* decode the codes into alpha values */
	for( row = 0; row < 4; row++ )
	{
		for( pix=0; pix < 4; pix++ )
		{
			aColors[ row ][ pix ].r = 0;
			aColors[ row ][ pix ].g = 0;
			aColors[ row ][ pix ].b = 0;
			aColors[ row ][ pix ].a = (u8) alphas[ bits[ row ][ pix ] ];
		}
	}

	/* write out alpha values to the image bits */
	for( row = 0; row < 4; row++, pixel += width-4 )
	{
		for( pix = 0; pix < 4; pix++ )
		{
			/* zero the alpha bits of image pixel */
			*pixel &= alphaZero;

			/* or the bits into the prev. nulled alpha */
			*pixel |= *((u32*) &(aColors[ row ][ pix ]));
			pixel++;
		}
	}
}


/*
DDSDecompressDXT1()
decompresses a dxt1 format texture
*/
s32 DDSDecompressDXT1( ddsBuffer *dds, s32 width, s32 height, u8 *pixels )
{
	s32 x, y, xBlocks, yBlocks;
	u32 *pixel;
	ddsColorBlock *block;
	ddsColor colors[ 4 ];

	/* setup */
	xBlocks = width / 4;
	yBlocks = height / 4;

	/* walk y */
	for( y = 0; y < yBlocks; y++ )
	{
		/* 8 bytes per block */
		block = (ddsColorBlock*) (dds->data + y * xBlocks * 8);

		/* walk x */
		for( x = 0; x < xBlocks; x++, block++ )
		{
			DDSGetColorBlockColors( block, colors );
			pixel = (u32*) (pixels + x * 16 + (y * 4) * width * 4);
			DDSDecodeColorBlock( pixel, block, width, (u32*) colors );
		}
	}

	/* return ok */
	return 0;
}


/*
DDSDecompressDXT3()
decompresses a dxt3 format texture
*/

s32 DDSDecompressDXT3( ddsBuffer *dds, s32 width, s32 height, u8 *pixels )
{
	s32 x, y, xBlocks, yBlocks;
	u32 *pixel, alphaZero;
	ddsColorBlock *block;
	ddsAlphaBlockExplicit *alphaBlock;
	ddsColor colors[ 4 ];

	/* setup */
	xBlocks = width / 4;
	yBlocks = height / 4;

	/* create zero alpha */
	colors[ 0 ].a = 0;
	colors[ 0 ].r = 0xFF;
	colors[ 0 ].g = 0xFF;
	colors[ 0 ].b = 0xFF;
	alphaZero = *((u32*) &colors[ 0 ]);

	/* walk y */
	for( y = 0; y < yBlocks; y++ )
	{
		/* 8 bytes per block, 1 block for alpha, 1 block for color */
		block = (ddsColorBlock*) (dds->data + y * xBlocks * 16);

		/* walk x */
		for( x = 0; x < xBlocks; x++, block++ )
		{
			/* get alpha block */
			alphaBlock = (ddsAlphaBlockExplicit*) block;

			/* get color block */
			block++;
			DDSGetColorBlockColors( block, colors );

			/* decode color block */
			pixel = (u32*) (pixels + x * 16 + (y * 4) * width * 4);
			DDSDecodeColorBlock( pixel, block, width, (u32*) colors );

			/* overwrite alpha bits with alpha block */
			DDSDecodeAlphaExplicit( pixel, alphaBlock, width, alphaZero );
		}
	}

	/* return ok */
	return 0;
}


/*
DDSDecompressDXT5()
decompresses a dxt5 format texture
*/
s32 DDSDecompressDXT5( ddsBuffer *dds, s32 width, s32 height, u8 *pixels )
{
	s32 x, y, xBlocks, yBlocks;
	u32 *pixel, alphaZero;
	ddsColorBlock *block;
	ddsAlphaBlock3BitLinear *alphaBlock;
	ddsColor colors[ 4 ];

	/* setup */
	xBlocks = width / 4;
	yBlocks = height / 4;

	/* create zero alpha */
	colors[ 0 ].a = 0;
	colors[ 0 ].r = 0xFF;
	colors[ 0 ].g = 0xFF;
	colors[ 0 ].b = 0xFF;
	alphaZero = *((u32*) &colors[ 0 ]);

	/* walk y */
	for( y = 0; y < yBlocks; y++ )
	{
		/* 8 bytes per block, 1 block for alpha, 1 block for color */
		block = (ddsColorBlock*) (dds->data + y * xBlocks * 16);

		/* walk x */
		for( x = 0; x < xBlocks; x++, block++ )
		{
			/* get alpha block */
			alphaBlock = (ddsAlphaBlock3BitLinear*) block;

			/* get color block */
			block++;
			DDSGetColorBlockColors( block, colors );

			/* decode color block */
			pixel = (u32*) (pixels + x * 16 + (y * 4) * width * 4);
			DDSDecodeColorBlock( pixel, block, width, (u32*) colors );

			/* overwrite alpha bits with alpha block */
			DDSDecodeAlpha3BitLinear( pixel, alphaBlock, width, alphaZero );
		}
	}

	/* return ok */
	return 0;
}


/*
DDSDecompressDXT2()
decompresses a dxt2 format texture (fixme: un-premultiply alpha)
*/
s32 DDSDecompressDXT2( ddsBuffer *dds, s32 width, s32 height, u8 *pixels )
{
	/* decompress dxt3 first */
	const s32 r = DDSDecompressDXT3( dds, width, height, pixels );

	/* return to sender */
	return r;
}


/*
DDSDecompressDXT4()
decompresses a dxt4 format texture (fixme: un-premultiply alpha)
*/
s32 DDSDecompressDXT4( ddsBuffer *dds, s32 width, s32 height, u8 *pixels )
{
	/* decompress dxt5 first */
	const s32 r = DDSDecompressDXT5( dds, width, height, pixels );

	/* return to sender */
	return r;
}


/*
DDSDecompressARGB8888()
decompresses an argb 8888 format texture
*/
s32 DDSDecompressARGB8888( ddsBuffer *dds, s32 width, s32 height, u8 *pixels )
{
	/* setup */
	u8* in = dds->data;
	u8* out = pixels;

	/* walk y */
	for(s32 y = 0; y < height; y++)
	{
		/* walk x */
		for(s32 x = 0; x < width; x++)
		{
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
			*out++ = *in++;
		}
	}

	/* return ok */
	return 0;
}


/*
DDSDecompress()
decompresses a dds texture into an rgba image buffer, returns 0 on success
*/
s32 DDSDecompress( ddsBuffer *dds, u8 *pixels )
{
	s32 width, height;
	eDDSPixelFormat pf;

	/* get dds info */
	s32 r = DDSGetInfo( dds, &width, &height, &pf );
	if ( r )
		return r;

	/* decompress */
	switch( pf )
	{
	case DDS_PF_ARGB8888:
		/* fixme: support other [a]rgb formats */
		r = DDSDecompressARGB8888( dds, width, height, pixels );
		break;

	case DDS_PF_DXT1:
		r = DDSDecompressDXT1( dds, width, height, pixels );
		break;

	case DDS_PF_DXT2:
		r = DDSDecompressDXT2( dds, width, height, pixels );
		break;

	case DDS_PF_DXT3:
		r = DDSDecompressDXT3( dds, width, height, pixels );
		break;

	case DDS_PF_DXT4:
		r = DDSDecompressDXT4( dds, width, height, pixels );
		break;

	case DDS_PF_DXT5:
		r = DDSDecompressDXT5( dds, width, height, pixels );
		break;

	default:
	case DDS_PF_UNKNOWN:
		memset( pixels, 0xFF, width * height * 4 );
		r = -1;
		break;
	}

	/* return to sender */
	return r;
}

} // end anonymous namespace


//! returns true if the file maybe is able to be loaded by this class
//! based on the file extension (e.g. ".tga")
bool CImageLoaderDDS::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "dds" );
}


//! returns true if the file maybe is able to be loaded by this class
bool CImageLoaderDDS::isALoadableFileFormat(io::IReadFile* file) const
{
	if (!file)
		return false;

	ddsBuffer header;
	file->read(&header, sizeof(header));

	s32 width, height;
	eDDSPixelFormat pixelFormat;

	return (0 == DDSGetInfo( &header, &width, &height, &pixelFormat));
}


//! creates a surface from the file
IImage* CImageLoaderDDS::loadImage(io::IReadFile* file) const
{
	u8 *memFile = new u8 [ file->getSize() ];
	file->read ( memFile, file->getSize() );

	ddsBuffer *header = (ddsBuffer*) memFile;
	IImage* image = 0;
	s32 width, height;
	eDDSPixelFormat pixelFormat;

	if ( 0 == DDSGetInfo( header, &width, &height, &pixelFormat) )
	{
		image = new CImage(ECF_A8R8G8B8, core::dimension2d<u32>(width, height));

		if ( DDSDecompress( header, (u8*) image->lock() ) == -1)
		{
			image->unlock();
			image->drop();
			image = 0;
		}
	}

	delete [] memFile;
	if ( image )
		image->unlock();

	return image;
}


//! creates a loader which is able to load dds images
IImageLoader* createImageLoaderDDS()
{
	return new CImageLoaderDDS();
}


} // end namespace video
} // end namespace irr

#endif

