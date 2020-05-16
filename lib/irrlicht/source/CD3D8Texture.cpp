// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_

#define _IRR_DONT_DO_MEMORY_DEBUGGING_HERE
#include "CD3D8Texture.h"
#include "CD3D8Driver.h"
#include "os.h"


#ifndef _IRR_COMPILE_WITH_DIRECT3D_9_
// The D3DXFilterTexture function seems to get linked wrong when
// compiling with both D3D8 and 9, causing it not to work in the D3D9 device.
// So mipmapgeneration is replaced with my own bad generation in d3d 8 when
// compiling with both D3D 8 and 9.
//#define _IRR_USE_D3DXFilterTexture_
#endif // _IRR_COMPILE_WITH_DIRECT3D_9_

#include <d3dx8tex.h>

#ifdef _IRR_USE_D3DXFilterTexture_
#pragma comment (lib, "d3dx8.lib")
#endif // _IRR_USE_D3DXFilterTexture_

namespace irr
{
namespace video
{

//! rendertarget constructor
CD3D8Texture::CD3D8Texture(CD3D8Driver* driver, const core::dimension2d<u32>& size, const io::path& name)
: ITexture(name), Texture(0), RTTSurface(0), Driver(driver),
	TextureSize(size), ImageSize(size), Pitch(0),
	HasMipMaps(false), IsRenderTarget(true)
{
	#ifdef _DEBUG
	setDebugName("CD3D8Texture");
	#endif

	Device=driver->getExposedVideoData().D3D8.D3DDev8;
	if (Device)
		Device->AddRef();

	createRenderTarget();
}


//! constructor
CD3D8Texture::CD3D8Texture(IImage* image, CD3D8Driver* driver,
			u32 flags, const io::path& name, void* mipmapData)
: ITexture(name), Texture(0), RTTSurface(0), Driver(driver),
TextureSize(0,0), ImageSize(0,0), Pitch(0),
HasMipMaps(false), IsRenderTarget(false)
{
	#ifdef _DEBUG
	setDebugName("CD3D8Texture");
	#endif

	HasMipMaps = Driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);

	Device=driver->getExposedVideoData().D3D8.D3DDev8;
	if (Device)
		Device->AddRef();

	if (image)
	{
		if (createTexture(flags, image))
		{
			if (copyTexture(image))
			{
				regenerateMipMapLevels(mipmapData);
			}
		}
		else
			os::Printer::log("Could not create DIRECT3D8 Texture.", ELL_WARNING);
	}
}


//! destructor
CD3D8Texture::~CD3D8Texture()
{
	if (Texture)
		Texture->Release();

	if (RTTSurface)
		RTTSurface->Release();

	if (Device)
		Device->Release();
}


//! creates the hardware texture
bool CD3D8Texture::createTexture(u32 flags, video::IImage* image)
{
	ImageSize = image->getDimension();

	core::dimension2d<u32> optSize = ImageSize.getOptimalSize(!Driver->queryFeature(EVDF_TEXTURE_NPOT), !Driver->queryFeature(EVDF_TEXTURE_NSQUARE), true, Driver->Caps.MaxTextureWidth);

	D3DFORMAT format = D3DFMT_A1R5G5B5;
	switch(getTextureFormatFromFlags(flags))
	{
	case ETCF_ALWAYS_16_BIT:
		format = D3DFMT_A1R5G5B5; break;
	case ETCF_ALWAYS_32_BIT:
		format = D3DFMT_A8R8G8B8; break;
	case ETCF_OPTIMIZED_FOR_QUALITY:
		{
			switch(image->getColorFormat())
			{
			case ECF_R8G8B8:
			case ECF_A8R8G8B8:
				format = D3DFMT_A8R8G8B8; break;
			case ECF_A1R5G5B5:
			case ECF_R5G6B5:
				format = D3DFMT_A1R5G5B5; break;
			}
		}
		break;
	case ETCF_OPTIMIZED_FOR_SPEED:
		format = D3DFMT_A1R5G5B5; break;
	}

	if (Driver->getTextureCreationFlag(video::ETCF_NO_ALPHA_CHANNEL))
	{
		if (format == D3DFMT_A8R8G8B8)

#ifdef _IRR_XBOX_PLATFORM_
			format = D3DFMT_X8R8G8B8;
#else
			format = D3DFMT_R8G8B8;
#endif

		else if (format == D3DFMT_A1R5G5B5)
			format = D3DFMT_R5G6B5;
	}

	const bool mipmaps = Driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);

	HRESULT hr = Device->CreateTexture(optSize.Width, optSize.Height,
		mipmaps ? 0 : 1, // number of mipmaplevels (0 = automatic all)
		0, format, D3DPOOL_MANAGED, &Texture);

	if (FAILED(hr))
	{
		// try brute force 16 bit
		if (format == D3DFMT_A8R8G8B8)
			format = D3DFMT_A1R5G5B5;
#ifdef _IRR_XBOX_PLATFORM_
		else if (format == D3DFMT_X8R8G8B8)
			format = D3DFMT_R5G6B5;
#else
		else if (format == D3DFMT_R8G8B8)
			format = D3DFMT_R5G6B5;
#endif
		else
			return false;

		hr = Device->CreateTexture(optSize.Width, optSize.Height,
			mipmaps ? 0 : 1, // number of mipmaplevels (0 = automatic all)
			0, format, D3DPOOL_MANAGED, &Texture);
	}

	ColorFormat = getColorFormatFromD3DFormat(format);
	return (SUCCEEDED(hr));
}


//! copies the image to the texture
bool CD3D8Texture::copyTexture(video::IImage* image)
{
	if (Texture && image)
	{
		D3DSURFACE_DESC desc;
		Texture->GetLevelDesc(0, &desc);

		TextureSize.Width = desc.Width;
		TextureSize.Height = desc.Height;

		D3DLOCKED_RECT rect;
		HRESULT hr = Texture->LockRect(0, &rect, 0, 0);
		if (FAILED(hr))
		{
			os::Printer::log("Could not lock D3D8 Texture.", ELL_ERROR);
			return false;
		}

		Pitch = rect.Pitch;
		image->copyToScaling(rect.pBits, TextureSize.Width, TextureSize.Height, ColorFormat, Pitch);

		hr = Texture->UnlockRect(0);
		if (FAILED(hr))
		{
			os::Printer::log("Could not unlock D3D8 Texture.", ELL_ERROR);
			return false;
		}
	}

	return true;
}


//! lock function
void* CD3D8Texture::lock(E_TEXTURE_LOCK_MODE mode, u32 mipmapLevel)
{
	if (!Texture)
		return 0;

	MipLevelLocked=mipmapLevel;
	HRESULT hr;
	D3DLOCKED_RECT rect;
	if(!IsRenderTarget)
	{
		hr = Texture->LockRect(mipmapLevel, &rect, 0, (mode==ETLM_READ_ONLY)?D3DLOCK_READONLY:0);
		if (FAILED(hr))
		{
			os::Printer::log("Could not lock DIRECT3D9 Texture.", ELL_ERROR);
			return 0;
		}
	}
	else
	{
		if (!RTTSurface)
		{
			// Make RTT surface large enough for all miplevels (including 0)
			D3DSURFACE_DESC desc;
			Texture->GetLevelDesc(0, &desc);
			hr = Device->CreateImageSurface(desc.Width, desc.Height, desc.Format, &RTTSurface);
			if (FAILED(hr))
			{
				os::Printer::log("Could not lock DIRECT3D8 Texture.", ELL_ERROR);
				return 0;
			}
		}

		IDirect3DSurface8 *surface = 0;
		hr = Texture->GetSurfaceLevel(mipmapLevel, &surface);
		if (FAILED(hr))
		{
			os::Printer::log("Could not lock DIRECT3D8 Texture.", "Could not get surface.", ELL_ERROR);
			return 0;
		}
		hr = Device->CopyRects(surface, 0, 0, RTTSurface, 0);
		surface->Release();
		if(FAILED(hr))
		{
			os::Printer::log("Could not lock DIRECT3D8 Texture.", "Data copy failed.", ELL_ERROR);
			return 0;
		}
		hr = RTTSurface->LockRect(&rect, 0, (mode==ETLM_READ_ONLY)?D3DLOCK_READONLY:0);
		if(FAILED(hr))
		{
			os::Printer::log("Could not lock DIRECT3D8 Texture.", "LockRect failed.", ELL_ERROR);
			return 0;
		}
	}
	return rect.pBits;
}


//! unlock function
void CD3D8Texture::unlock()
{
	if (!Texture)
		return;

	if (!IsRenderTarget)
		Texture->UnlockRect(MipLevelLocked);
	else if (RTTSurface)
		RTTSurface->UnlockRect();
}


//! Returns original size of the texture.
const core::dimension2d<u32>& CD3D8Texture::getOriginalSize() const
{
	return ImageSize;
}


//! Returns (=size) of the texture.
const core::dimension2d<u32>& CD3D8Texture::getSize() const
{
	return TextureSize;
}


//! returns driver type of texture (=the driver, who created the texture)
E_DRIVER_TYPE CD3D8Texture::getDriverType() const
{
	return EDT_DIRECT3D8;
}


//! returns color format of texture
ECOLOR_FORMAT CD3D8Texture::getColorFormat() const
{
	return ColorFormat;
}


//! returns pitch of texture (in bytes)
u32 CD3D8Texture::getPitch() const
{
	return Pitch;
}


//! returns the DIRECT3D8 Texture
IDirect3DTexture8* CD3D8Texture::getDX8Texture() const
{
	return Texture;
}


//! returns if texture has mipmap levels
bool CD3D8Texture::hasMipMaps() const
{
	return HasMipMaps;
}


// The D3DXFilterTexture function seems to get linked wrong when
// compiling with both D3D8 and 9, causing it not to work in the D3D9 device.
// So mipmapgeneration is replaced with my own bad generation in d3d 8 when
// compiling with both D3D 8 and 9.
bool CD3D8Texture::createMipMaps(u32 level)
{
	if (level==0)
		return true;

	IDirect3DSurface8* upperSurface = 0;
	IDirect3DSurface8* lowerSurface = 0;

	// get upper level
	HRESULT hr = Texture->GetSurfaceLevel(level-1, &upperSurface);
	if (FAILED(hr) || !upperSurface)
	{
		os::Printer::log("Could not get upper surface level for mip map generation", ELL_WARNING);
		return false;
	}

	// get lower level
	hr = Texture->GetSurfaceLevel(level, &lowerSurface);
	if (FAILED(hr) || !lowerSurface)
	{
		os::Printer::log("Could not get lower surface level for mip map generation", ELL_WARNING);
		upperSurface->Release();
		return false;
	}

	D3DSURFACE_DESC upperDesc, lowerDesc;
	upperSurface->GetDesc(&upperDesc);
	lowerSurface->GetDesc(&lowerDesc);

	D3DLOCKED_RECT upperlr;
	D3DLOCKED_RECT lowerlr;

	// lock upper surface
	if (FAILED(upperSurface->LockRect(&upperlr, NULL, 0)))
	{
		os::Printer::log("Could not lock upper texture for mip map generation", ELL_WARNING);
		upperSurface->Release();
		lowerSurface->Release();
		return false;
	}

	// lock lower surface
	if (FAILED(lowerSurface->LockRect(&lowerlr, NULL, 0)))
	{
		os::Printer::log("Could not lock lower texture for mip map generation", ELL_WARNING);
		upperSurface->UnlockRect();
		upperSurface->Release();
		lowerSurface->Release();
		return false;
	}

	if (upperDesc.Format != lowerDesc.Format)
	{
		os::Printer::log("Cannot copy mip maps with different formats.", ELL_WARNING);
	}
	else
	{
		if (upperDesc.Format == D3DFMT_A1R5G5B5)
			copy16BitMipMap((char*)upperlr.pBits, (char*)lowerlr.pBits,
							lowerDesc.Width, lowerDesc.Height,
							upperlr.Pitch, lowerlr.Pitch);
		else
		if (upperDesc.Format == D3DFMT_A8R8G8B8)
			copy32BitMipMap((char*)upperlr.pBits, (char*)lowerlr.pBits,
							lowerDesc.Width, lowerDesc.Height,
							upperlr.Pitch, lowerlr.Pitch);
		else
			os::Printer::log("Unsupported mipmap format, cannot copy.", ELL_WARNING);
	}

	bool result=true;
	// unlock
	if (FAILED(upperSurface->UnlockRect()))
		result=false;
	if (FAILED(lowerSurface->UnlockRect()))
		result=false;

	// release
	upperSurface->Release();
	lowerSurface->Release();

	if (!result || upperDesc.Width < 3 || upperDesc.Height < 3)
		return result; // stop generating levels

	// generate next level
	return createMipMaps(level+1);
}


ECOLOR_FORMAT CD3D8Texture::getColorFormatFromD3DFormat(D3DFORMAT format)
{
	switch(format)
	{
	case D3DFMT_X1R5G5B5:
	case D3DFMT_A1R5G5B5:
		Pitch = TextureSize.Width * 2;
		return ECF_A1R5G5B5;
		break;
	case D3DFMT_A8R8G8B8:
	case D3DFMT_X8R8G8B8:
		Pitch = TextureSize.Width * 4;
		return ECF_A8R8G8B8;
		break;
	case D3DFMT_R5G6B5:
		Pitch = TextureSize.Width * 2;
		return ECF_R5G6B5;
		break;
	default:
		return (ECOLOR_FORMAT)0;
	};
}



void CD3D8Texture::copy16BitMipMap(char* src, char* tgt,
				   s32 width, s32 height,
				   s32 pitchsrc, s32 pitchtgt) const
{
	u16 c;

	for (int x=0; x<width; ++x)
	{
		for (int y=0; y<height; ++y)
		{
			s32 a=0, r=0, g=0, b=0;

			for (int dx=0; dx<2; ++dx)
			{
				for (int dy=0; dy<2; ++dy)
				{
					int tgx = (x*2)+dx;
					int tgy = (y*2)+dy;

					c = *(u16*)((void*)&src[(tgx*2)+(tgy*pitchsrc)]);

					a += getAlpha(c);
					r += getRed(c);
					g += getGreen(c);
					b += getBlue(c);
				}
			}

			a /= 4;
			r /= 4;
			g /= 4;
			b /= 4;

			c = ((a & 0x1) <<15) | ((r & 0x1F)<<10) | ((g & 0x1F)<<5) | (b & 0x1F);
			*(u16*)((void*)&tgt[(x*2)+(y*pitchtgt)]) = c;
		}
	}
}


void CD3D8Texture::copy32BitMipMap(char* src, char* tgt,
				   s32 width, s32 height,
				   s32 pitchsrc, s32 pitchtgt) const
{
	SColor c;

	for (int x=0; x<width; ++x)
	{
		for (int y=0; y<height; ++y)
		{
			s32 a=0, r=0, g=0, b=0;

			for (int dx=0; dx<2; ++dx)
			{
				for (int dy=0; dy<2; ++dy)
				{
					int tgx = (x*2)+dx;
					int tgy = (y*2)+dy;

					c = *(u32*)((void*)&src[(tgx<<2)+(tgy*pitchsrc)]);

					a += c.getAlpha();
					r += c.getRed();
					g += c.getGreen();
					b += c.getBlue();
				}
			}

			a >>= 2;
			r >>= 2;
			g >>= 2;
			b >>= 2;

			c = ((a & 0xff)<<24) | ((r & 0xff)<<16) | ((g & 0xff)<<8) | (b & 0xff);
			*(u32*)((void*)&tgt[(x*4)+(y*pitchtgt)]) = c.color;
		}
	}
}


void CD3D8Texture::createRenderTarget()
{
	TextureSize = TextureSize.getOptimalSize(!Driver->queryFeature(EVDF_TEXTURE_NPOT), !Driver->queryFeature(EVDF_TEXTURE_NSQUARE), true, Driver->Caps.MaxTextureWidth);

	// get backbuffer format to create the render target in the
	// same format

	IDirect3DSurface8* bb;
	D3DFORMAT d3DFormat = D3DFMT_A8R8G8B8;

	if (!FAILED(Device->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &bb)))
	{
		D3DSURFACE_DESC desc;
		bb->GetDesc(&desc);
		d3DFormat = desc.Format;

		if (d3DFormat == D3DFMT_X8R8G8B8)
			d3DFormat = D3DFMT_A8R8G8B8;

		bb->Release();
	}
	else
	{
		os::Printer::log("Could not create RenderTarget texture: could not get BackBuffer.",
			ELL_WARNING);
		return;
	}

	// create texture
	HRESULT hr;

	hr = Device->CreateTexture(
		TextureSize.Width,
		TextureSize.Height,
		1, // mip map level count, we don't want mipmaps here
		D3DUSAGE_RENDERTARGET,
		d3DFormat,
		D3DPOOL_DEFAULT,
		&Texture);

	// get irrlicht format from D3D format
	ColorFormat = getColorFormatFromD3DFormat(d3DFormat);

	if (FAILED(hr))
		os::Printer::log("Could not create render target texture");
}


//! Regenerates the mip map levels of the texture. Useful after locking and
//! modifying the texture
void CD3D8Texture::regenerateMipMapLevels(void* mipmapData)
{
	if (mipmapData)
	{
		core::dimension2du size = TextureSize;
		u32 level=0;
		do
		{
			if (size.Width>1)
				size.Width /=2;
			if (size.Height>1)
				size.Height /=2;
			++level;
			IDirect3DSurface8* mipSurface = 0;
			HRESULT hr = Texture->GetSurfaceLevel(level, &mipSurface);
			if (FAILED(hr) || !mipSurface)
			{
				os::Printer::log("Could not get mipmap level", ELL_WARNING);
				return;
			}
			D3DSURFACE_DESC mipDesc;
			mipSurface->GetDesc(&mipDesc);
			D3DLOCKED_RECT miplr;

			// lock mipmap surface
			if (FAILED(mipSurface->LockRect(&miplr, NULL, 0)))
			{
				mipSurface->Release();
				os::Printer::log("Could not lock texture", ELL_WARNING);
				return;
			}

			memcpy(miplr.pBits, mipmapData, size.getArea()*getPitch()/TextureSize.Width);
			mipmapData = (u8*)mipmapData+size.getArea()*getPitch()/TextureSize.Width;
			// unlock
			mipSurface->UnlockRect();
			// release
			mipSurface->Release();
		} while (size.Width != 1 || size.Height != 1);
	}
	else if (HasMipMaps)
	{
		// create mip maps.
#ifndef _IRR_USE_D3DXFilterTexture_
		// The D3DXFilterTexture function seems to get linked wrong when
		// compiling with both D3D8 and 9, causing it not to work in the D3D9 device.
		// So mipmapgeneration is replaced with my own bad generation in d3d 8 when
		// compiling with both D3D 8 and 9.
		HRESULT hr  = D3DXFilterTexture(Texture, NULL, D3DX_DEFAULT , D3DX_DEFAULT );
		if (FAILED(hr))
#endif
		createMipMaps();
	}
}


//! returns if it is a render target
bool CD3D8Texture::isRenderTarget() const
{
	return IsRenderTarget;
}


//! Returns pointer to the render target surface
IDirect3DSurface8* CD3D8Texture::getRenderTargetSurface()
{
	if (!IsRenderTarget)
		return 0;

	IDirect3DSurface8 *pRTTSurface = 0;
	if (Texture)
		Texture->GetSurfaceLevel(0, &pRTTSurface);

	if (pRTTSurface)
		pRTTSurface->Release();

	return pRTTSurface;
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_8_

