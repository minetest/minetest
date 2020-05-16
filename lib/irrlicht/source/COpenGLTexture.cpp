// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_OPENGL_

#include "irrTypes.h"
#include "COpenGLTexture.h"
#include "COpenGLDriver.h"
#include "os.h"
#include "CColorConverter.h"

#include "irrString.h"

namespace irr
{
namespace video
{

//! constructor for usual textures
COpenGLTexture::COpenGLTexture(IImage* origImage, const io::path& name, void* mipmapData, COpenGLDriver* driver)
	: ITexture(name), ColorFormat(ECF_A8R8G8B8), Driver(driver), Image(0), MipImage(0),
	TextureName(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
	PixelType(GL_UNSIGNED_BYTE), MipLevelStored(0), MipmapLegacyMode(true),
	IsRenderTarget(false), AutomaticMipmapUpdate(false),
	ReadOnlyLock(false), KeepImage(true)
{
	#ifdef _DEBUG
	setDebugName("COpenGLTexture");
	#endif

	HasMipMaps = Driver->getTextureCreationFlag(ETCF_CREATE_MIP_MAPS);
	getImageValues(origImage);

	glGenTextures(1, &TextureName);

	if (ImageSize==TextureSize)
	{
		Image = Driver->createImage(ColorFormat, ImageSize);
		origImage->copyTo(Image);
	}
	else
	{
		Image = Driver->createImage(ColorFormat, TextureSize);
		// scale texture
		origImage->copyToScaling(Image);
	}
	uploadTexture(true, mipmapData);
	if (!KeepImage)
	{
		Image->drop();
		Image=0;
	}
}


//! constructor for basic setup (only for derived classes)
COpenGLTexture::COpenGLTexture(const io::path& name, COpenGLDriver* driver)
	: ITexture(name), ColorFormat(ECF_A8R8G8B8), Driver(driver), Image(0), MipImage(0),
	TextureName(0), InternalFormat(GL_RGBA), PixelFormat(GL_BGRA_EXT),
	PixelType(GL_UNSIGNED_BYTE), MipLevelStored(0), HasMipMaps(true),
	MipmapLegacyMode(true), IsRenderTarget(false), AutomaticMipmapUpdate(false),
	ReadOnlyLock(false), KeepImage(true)
{
	#ifdef _DEBUG
	setDebugName("COpenGLTexture");
	#endif
}


//! destructor
COpenGLTexture::~COpenGLTexture()
{
	if (TextureName)
		glDeleteTextures(1, &TextureName);
	if (Image)
		Image->drop();
}


//! Choose best matching color format, based on texture creation flags
ECOLOR_FORMAT COpenGLTexture::getBestColorFormat(ECOLOR_FORMAT format)
{
	ECOLOR_FORMAT destFormat = ECF_A8R8G8B8;
	switch (format)
	{
		case ECF_A1R5G5B5:
			if (!Driver->getTextureCreationFlag(ETCF_ALWAYS_32_BIT))
				destFormat = ECF_A1R5G5B5;
		break;
		case ECF_R5G6B5:
			if (!Driver->getTextureCreationFlag(ETCF_ALWAYS_32_BIT))
				destFormat = ECF_A1R5G5B5;
		break;
		case ECF_A8R8G8B8:
			if (Driver->getTextureCreationFlag(ETCF_ALWAYS_16_BIT) ||
					Driver->getTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED))
				destFormat = ECF_A1R5G5B5;
		break;
		case ECF_R8G8B8:
			if (Driver->getTextureCreationFlag(ETCF_ALWAYS_16_BIT) ||
					Driver->getTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED))
				destFormat = ECF_A1R5G5B5;
		default:
		break;
	}
	if (Driver->getTextureCreationFlag(ETCF_NO_ALPHA_CHANNEL))
	{
		switch (destFormat)
		{
			case ECF_A1R5G5B5:
				destFormat = ECF_R5G6B5;
			break;
			case ECF_A8R8G8B8:
				destFormat = ECF_R8G8B8;
			break;
			default:
			break;
		}
	}
	return destFormat;
}


//! Get opengl values for the GPU texture storage
GLint COpenGLTexture::getOpenGLFormatAndParametersFromColorFormat(ECOLOR_FORMAT format,
				GLint& filtering,
				GLenum& colorformat,
				GLenum& type)
{
	// default
	filtering = GL_LINEAR;
	colorformat = GL_RGBA;
	type = GL_UNSIGNED_BYTE;
	GLenum internalformat = GL_RGBA;

	switch(format)
	{
		case ECF_A1R5G5B5:
			colorformat=GL_BGRA_EXT;
			type=GL_UNSIGNED_SHORT_1_5_5_5_REV;
			internalformat =  GL_RGBA;
			break;
		case ECF_R5G6B5:
			colorformat=GL_RGB;
			type=GL_UNSIGNED_SHORT_5_6_5;
			internalformat =  GL_RGB;
			break;
		case ECF_R8G8B8:
			colorformat=GL_BGR;
			type=GL_UNSIGNED_BYTE;
			internalformat =  GL_RGB;
			break;
		case ECF_A8R8G8B8:
			colorformat=GL_BGRA_EXT;
			if (Driver->Version > 101)
				type=GL_UNSIGNED_INT_8_8_8_8_REV;
			internalformat =  GL_RGBA;
			break;
		// Floating Point texture formats. Thanks to Patryk "Nadro" Nadrowski.
		case ECF_R16F:
		{
#ifdef GL_ARB_texture_rg
			filtering = GL_NEAREST;
			colorformat = GL_RED;
			type = GL_FLOAT;

			internalformat =  GL_R16F;
#else
			ColorFormat = ECF_A8R8G8B8;
			internalformat =  GL_RGB8;
#endif
		}
			break;
		case ECF_G16R16F:
		{
#ifdef GL_ARB_texture_rg
			filtering = GL_NEAREST;
			colorformat = GL_RG;
			type = GL_FLOAT;

			internalformat =  GL_RG16F;
#else
			ColorFormat = ECF_A8R8G8B8;
			internalformat =  GL_RGB8;
#endif
		}
			break;
		case ECF_A16B16G16R16F:
		{
#ifdef GL_ARB_texture_rg
			filtering = GL_NEAREST;
			colorformat = GL_RGBA;
			type = GL_FLOAT;

			internalformat =  GL_RGBA16F_ARB;
#else
			ColorFormat = ECF_A8R8G8B8;
			internalformat =  GL_RGBA8;
#endif
		}
			break;
		case ECF_R32F:
		{
#ifdef GL_ARB_texture_rg
			filtering = GL_NEAREST;
			colorformat = GL_RED;
			type = GL_FLOAT;

			internalformat =  GL_R32F;
#else
			ColorFormat = ECF_A8R8G8B8;
			internalformat =  GL_RGB8;
#endif
		}
			break;
		case ECF_G32R32F:
		{
#ifdef GL_ARB_texture_rg
			filtering = GL_NEAREST;
			colorformat = GL_RG;
			type = GL_FLOAT;

			internalformat =  GL_RG32F;
#else
			ColorFormat = ECF_A8R8G8B8;
			internalformat =  GL_RGB8;
#endif
		}
			break;
		case ECF_A32B32G32R32F:
		{
#ifdef GL_ARB_texture_float
			filtering = GL_NEAREST;
			colorformat = GL_RGBA;
			type = GL_FLOAT;

			internalformat =  GL_RGBA32F_ARB;
#else
			ColorFormat = ECF_A8R8G8B8;
			internalformat =  GL_RGBA8;
#endif
		}
			break;
		default:
		{
			os::Printer::log("Unsupported texture format", ELL_ERROR);
			internalformat =  GL_RGBA8;
		}
	}
#if defined(GL_ARB_framebuffer_sRGB) || defined(GL_EXT_framebuffer_sRGB)
	if (Driver->Params.HandleSRGB)
	{
		if (internalformat==GL_RGBA)
			internalformat=GL_SRGB_ALPHA_EXT;
		else if (internalformat==GL_RGB)
			internalformat=GL_SRGB_EXT;
	}
#endif
	return internalformat;
}


// prepare values ImageSize, TextureSize, and ColorFormat based on image
void COpenGLTexture::getImageValues(IImage* image)
{
	if (!image)
	{
		os::Printer::log("No image for OpenGL texture.", ELL_ERROR);
		return;
	}

	ImageSize = image->getDimension();

	if ( !ImageSize.Width || !ImageSize.Height)
	{
		os::Printer::log("Invalid size of image for OpenGL Texture.", ELL_ERROR);
		return;
	}

	const f32 ratio = (f32)ImageSize.Width/(f32)ImageSize.Height;
	if ((ImageSize.Width>Driver->MaxTextureSize) && (ratio >= 1.0f))
	{
		ImageSize.Width = Driver->MaxTextureSize;
		ImageSize.Height = (u32)(Driver->MaxTextureSize/ratio);
	}
	else if (ImageSize.Height>Driver->MaxTextureSize)
	{
		ImageSize.Height = Driver->MaxTextureSize;
		ImageSize.Width = (u32)(Driver->MaxTextureSize*ratio);
	}
	TextureSize=ImageSize.getOptimalSize(!Driver->queryFeature(EVDF_TEXTURE_NPOT));

	ColorFormat = getBestColorFormat(image->getColorFormat());
}


//! copies the the texture into an open gl texture.
void COpenGLTexture::uploadTexture(bool newTexture, void* mipmapData, u32 level)
{
	// check which image needs to be uploaded
	IImage* image = level?MipImage:Image;
	if (!image)
	{
		os::Printer::log("No image for OpenGL texture to upload", ELL_ERROR);
		return;
	}

	// get correct opengl color data values
	GLenum oldInternalFormat = InternalFormat;
	GLint filtering;
	InternalFormat = getOpenGLFormatAndParametersFromColorFormat(ColorFormat, filtering, PixelFormat, PixelType);
	// make sure we don't change the internal format of existing images
	if (!newTexture)
		InternalFormat=oldInternalFormat;

	Driver->setActiveTexture(0, this);
	if (Driver->testGLError())
		os::Printer::log("Could not bind Texture", ELL_ERROR);

	// mipmap handling for main texture
	if (!level && newTexture)
	{
#ifndef DISABLE_MIPMAPPING
#ifdef GL_SGIS_generate_mipmap
		// auto generate if possible and no mipmap data is given
		if (HasMipMaps && !mipmapData && Driver->queryFeature(EVDF_MIP_MAP_AUTO_UPDATE))
		{
			if (Driver->getTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED))
				glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_FASTEST);
			else if (Driver->getTextureCreationFlag(ETCF_OPTIMIZED_FOR_QUALITY))
				glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_NICEST);
			else
				glHint(GL_GENERATE_MIPMAP_HINT_SGIS, GL_DONT_CARE);

			AutomaticMipmapUpdate=true;

			if (!Driver->queryFeature(EVDF_FRAMEBUFFER_OBJECT))
			{
				glTexParameteri( GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE );
				MipmapLegacyMode=true;
			}
			else
				MipmapLegacyMode=false;
		}
		else
#endif
		{
			// Either generate manually due to missing capability
			// or use predefined mipmap data
			AutomaticMipmapUpdate=false;
			regenerateMipMapLevels(mipmapData);
		}
		if (HasMipMaps) // might have changed in regenerateMipMapLevels
		{
			// enable bilinear mipmap filter
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
#else
			HasMipMaps=false;
			os::Printer::log("Did not create OpenGL texture mip maps.", ELL_INFORMATION);
#endif
		{
			// enable bilinear filter without mipmaps
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
	}

	// now get image data and upload to GPU
	void* source = image->lock();
	if (newTexture)
		glTexImage2D(GL_TEXTURE_2D, level, InternalFormat, image->getDimension().Width,
			image->getDimension().Height, 0, PixelFormat, PixelType, source);
	else
		glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, image->getDimension().Width,
			image->getDimension().Height, PixelFormat, PixelType, source);
	image->unlock();

	if (!MipmapLegacyMode && AutomaticMipmapUpdate)
	{
		glEnable(GL_TEXTURE_2D);
		Driver->extGlGenerateMipmap(GL_TEXTURE_2D);
	}

	if (Driver->testGLError())
		os::Printer::log("Could not glTexImage2D", ELL_ERROR);
}


//! lock function
void* COpenGLTexture::lock(E_TEXTURE_LOCK_MODE mode, u32 mipmapLevel)
{
	// store info about which image is locked
	IImage* image = (mipmapLevel==0)?Image:MipImage;
	ReadOnlyLock |= (mode==ETLM_READ_ONLY);
	MipLevelStored = mipmapLevel;
	if (!ReadOnlyLock && mipmapLevel)
	{
#ifdef GL_SGIS_generate_mipmap
		if (Driver->queryFeature(EVDF_MIP_MAP_AUTO_UPDATE))
		{
			// do not automatically generate and update mipmaps
			glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
		}
#endif
		AutomaticMipmapUpdate=false;
	}

	// if data not available or might have changed on GPU download it
	if (!image || IsRenderTarget)
	{
		// prepare the data storage if necessary
		if (!image)
		{
			if (mipmapLevel)
			{
				u32 i=0;
				u32 width = TextureSize.Width;
				u32 height = TextureSize.Height;
				do
				{
					if (width>1)
						width>>=1;
					if (height>1)
						height>>=1;
					++i;
				}
				while (i != mipmapLevel);
				MipImage = image = Driver->createImage(ECF_A8R8G8B8, core::dimension2du(width,height));
			}
			else
				Image = image = Driver->createImage(ECF_A8R8G8B8, ImageSize);
			ColorFormat = ECF_A8R8G8B8;
		}
		if (!image)
			return 0;

		if (mode != ETLM_WRITE_ONLY)
		{
			u8* pixels = static_cast<u8*>(image->lock());
			if (!pixels)
				return 0;

			// we need to keep the correct texture bound later on
			GLint tmpTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &tmpTexture);
			glBindTexture(GL_TEXTURE_2D, TextureName);

			// we need to flip textures vertical
			// however, it seems that this does not hold for mipmap
			// textures, for unknown reasons.

			// allows to read pixels in top-to-bottom order
#ifdef GL_MESA_pack_invert
			if (!mipmapLevel && Driver->queryOpenGLFeature(COpenGLExtensionHandler::IRR_MESA_pack_invert))
				glPixelStorei(GL_PACK_INVERT_MESA, GL_TRUE);
#endif

			// download GPU data as ARGB8 to pixels;
			glGetTexImage(GL_TEXTURE_2D, mipmapLevel, GL_BGRA_EXT, GL_UNSIGNED_BYTE, pixels);

			if (!mipmapLevel)
			{
#ifdef GL_MESA_pack_invert
				if (Driver->queryOpenGLFeature(COpenGLExtensionHandler::IRR_MESA_pack_invert))
					glPixelStorei(GL_PACK_INVERT_MESA, GL_FALSE);
				else
#endif
				{
					// opengl images are horizontally flipped, so we have to fix that here.
					const s32 pitch=image->getPitch();
					u8* p2 = pixels + (image->getDimension().Height - 1) * pitch;
					u8* tmpBuffer = new u8[pitch];
					for (u32 i=0; i < image->getDimension().Height; i += 2)
					{
						memcpy(tmpBuffer, pixels, pitch);
						memcpy(pixels, p2, pitch);
						memcpy(p2, tmpBuffer, pitch);
						pixels += pitch;
						p2 -= pitch;
					}
					delete [] tmpBuffer;
				}
			}
			image->unlock();

			//reset old bound texture
			glBindTexture(GL_TEXTURE_2D, tmpTexture);
		}
	}
	return image->lock();
}


//! unlock function
void COpenGLTexture::unlock()
{
	// test if miplevel or main texture was locked
	IImage* image = MipImage?MipImage:Image;
	if (!image)
		return;
	// unlock image to see changes
	image->unlock();
	// copy texture data to GPU
	if (!ReadOnlyLock)
		uploadTexture(false, 0, MipLevelStored);
	ReadOnlyLock = false;
	// cleanup local image
	if (MipImage)
	{
		MipImage->drop();
		MipImage=0;
	}
	else if (!KeepImage)
	{
		Image->drop();
		Image=0;
	}
	// update information
	if (Image)
		ColorFormat=Image->getColorFormat();
	else
		ColorFormat=ECF_A8R8G8B8;
}


//! Returns size of the original image.
const core::dimension2d<u32>& COpenGLTexture::getOriginalSize() const
{
	return ImageSize;
}


//! Returns size of the texture.
const core::dimension2d<u32>& COpenGLTexture::getSize() const
{
	return TextureSize;
}


//! returns driver type of texture, i.e. the driver, which created the texture
E_DRIVER_TYPE COpenGLTexture::getDriverType() const
{
	return EDT_OPENGL;
}


//! returns color format of texture
ECOLOR_FORMAT COpenGLTexture::getColorFormat() const
{
	return ColorFormat;
}


//! returns pitch of texture (in bytes)
u32 COpenGLTexture::getPitch() const
{
	if (Image)
		return Image->getPitch();
	else
		return 0;
}


//! return open gl texture name
GLuint COpenGLTexture::getOpenGLTextureName() const
{
	return TextureName;
}


//! Returns whether this texture has mipmaps
bool COpenGLTexture::hasMipMaps() const
{
	return HasMipMaps;
}


//! Regenerates the mip map levels of the texture. Useful after locking and
//! modifying the texture
void COpenGLTexture::regenerateMipMapLevels(void* mipmapData)
{
	if (AutomaticMipmapUpdate || !HasMipMaps || !Image)
		return;
	if ((Image->getDimension().Width==1) && (Image->getDimension().Height==1))
		return;

	// Manually create mipmaps or use prepared version
	u32 width=Image->getDimension().Width;
	u32 height=Image->getDimension().Height;
	u32 i=0;
	u8* target = static_cast<u8*>(mipmapData);
	do
	{
		if (width>1)
			width>>=1;
		if (height>1)
			height>>=1;
		++i;
		if (!target)
			target = new u8[width*height*Image->getBytesPerPixel()];
		// create scaled version if no mipdata available
		if (!mipmapData)
			Image->copyToScaling(target, width, height, Image->getColorFormat());
		glTexImage2D(GL_TEXTURE_2D, i, InternalFormat, width, height,
				0, PixelFormat, PixelType, target);
		// get next prepared mipmap data if available
		if (mipmapData)
		{
			mipmapData = static_cast<u8*>(mipmapData)+width*height*Image->getBytesPerPixel();
			target = static_cast<u8*>(mipmapData);
		}
	}
	while (width!=1 || height!=1);
	// cleanup
	if (!mipmapData)
		delete [] target;
}


bool COpenGLTexture::isRenderTarget() const
{
	return IsRenderTarget;
}


void COpenGLTexture::setIsRenderTarget(bool isTarget)
{
	IsRenderTarget = isTarget;
}


bool COpenGLTexture::isFrameBufferObject() const
{
	return false;
}


//! Bind Render Target Texture
void COpenGLTexture::bindRTT()
{
}


//! Unbind Render Target Texture
void COpenGLTexture::unbindRTT()
{
	Driver->setActiveTexture(0, this);

	// Copy Our ViewPort To The Texture
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, getSize().Width, getSize().Height);
}


/* FBO Textures */

// helper function for render to texture
static bool checkFBOStatus(COpenGLDriver* Driver);

//! RTT ColorFrameBuffer constructor
COpenGLFBOTexture::COpenGLFBOTexture(const core::dimension2d<u32>& size,
					const io::path& name, COpenGLDriver* driver,
					ECOLOR_FORMAT format)
	: COpenGLTexture(name, driver), DepthTexture(0), ColorFrameBuffer(0)
{
	#ifdef _DEBUG
	setDebugName("COpenGLTexture_FBO");
	#endif

	ImageSize = size;
	TextureSize = size;

	if (ECF_UNKNOWN == format)
		format = getBestColorFormat(driver->getColorFormat());

	ColorFormat = format;

	GLint FilteringType;
	InternalFormat = getOpenGLFormatAndParametersFromColorFormat(format, FilteringType, PixelFormat, PixelType);

	HasMipMaps = false;
	IsRenderTarget = true;

#ifdef GL_EXT_framebuffer_object
	// generate frame buffer
	Driver->extGlGenFramebuffers(1, &ColorFrameBuffer);
	bindRTT();

	// generate color texture
	glGenTextures(1, &TextureName);
	Driver->setActiveTexture(0, this);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, FilteringType);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, InternalFormat, ImageSize.Width,
		ImageSize.Height, 0, PixelFormat, PixelType, 0);
#ifdef _DEBUG
	driver->testGLError();
#endif

	// attach color texture to frame buffer
	Driver->extGlFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
						GL_COLOR_ATTACHMENT0_EXT,
						GL_TEXTURE_2D,
						TextureName,
						0);
#ifdef _DEBUG
	checkFBOStatus(Driver);
#endif

#endif
	unbindRTT();
}


//! destructor
COpenGLFBOTexture::~COpenGLFBOTexture()
{
	if (DepthTexture)
		if (DepthTexture->drop())
			Driver->removeDepthTexture(DepthTexture);
	if (ColorFrameBuffer)
		Driver->extGlDeleteFramebuffers(1, &ColorFrameBuffer);
}


bool COpenGLFBOTexture::isFrameBufferObject() const
{
	return true;
}


//! Bind Render Target Texture
void COpenGLFBOTexture::bindRTT()
{
#ifdef GL_EXT_framebuffer_object
	if (ColorFrameBuffer != 0)
		Driver->extGlBindFramebuffer(GL_FRAMEBUFFER_EXT, ColorFrameBuffer);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
#endif
}


//! Unbind Render Target Texture
void COpenGLFBOTexture::unbindRTT()
{
#ifdef GL_EXT_framebuffer_object
	if (ColorFrameBuffer != 0)
		Driver->extGlBindFramebuffer(GL_FRAMEBUFFER_EXT, 0);
#endif
}


/* FBO Depth Textures */

//! RTT DepthBuffer constructor
COpenGLFBODepthTexture::COpenGLFBODepthTexture(
		const core::dimension2d<u32>& size,
		const io::path& name,
		COpenGLDriver* driver,
		bool useStencil)
	: COpenGLTexture(name, driver), DepthRenderBuffer(0),
	StencilRenderBuffer(0), UseStencil(useStencil)
{
#ifdef _DEBUG
	setDebugName("COpenGLTextureFBO_Depth");
#endif

	ImageSize = size;
	TextureSize = size;
	InternalFormat = GL_RGBA;
	PixelFormat = GL_RGBA;
	PixelType = GL_UNSIGNED_BYTE;
	HasMipMaps = false;

	if (useStencil)
	{
		glGenTextures(1, &DepthRenderBuffer);
		glBindTexture(GL_TEXTURE_2D, DepthRenderBuffer);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
#ifdef GL_EXT_packed_depth_stencil
		if (Driver->queryOpenGLFeature(COpenGLExtensionHandler::IRR_EXT_packed_depth_stencil))
		{
			// generate packed depth stencil texture
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_STENCIL_EXT, ImageSize.Width,
				ImageSize.Height, 0, GL_DEPTH_STENCIL_EXT, GL_UNSIGNED_INT_24_8_EXT, 0);
			StencilRenderBuffer = DepthRenderBuffer; // stencil is packed with depth
		}
		else // generate separate stencil and depth textures
#endif
		{
			// generate depth texture
			glTexImage2D(GL_TEXTURE_2D, 0, Driver->getZBufferBits(), ImageSize.Width,
				ImageSize.Height, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, 0);

			// generate stencil texture
			glGenTextures(1, &StencilRenderBuffer);
			glBindTexture(GL_TEXTURE_2D, StencilRenderBuffer);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_STENCIL_INDEX, ImageSize.Width,
				ImageSize.Height, 0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, 0);
		}
	}
#ifdef GL_EXT_framebuffer_object
	else
	{
		// generate depth buffer
		Driver->extGlGenRenderbuffers(1, &DepthRenderBuffer);
		Driver->extGlBindRenderbuffer(GL_RENDERBUFFER_EXT, DepthRenderBuffer);
		Driver->extGlRenderbufferStorage(GL_RENDERBUFFER_EXT,
				Driver->getZBufferBits(), ImageSize.Width,
				ImageSize.Height);
	}
#endif
}


//! destructor
COpenGLFBODepthTexture::~COpenGLFBODepthTexture()
{
	if (DepthRenderBuffer && UseStencil)
		glDeleteTextures(1, &DepthRenderBuffer);
	else
		Driver->extGlDeleteRenderbuffers(1, &DepthRenderBuffer);
	if (StencilRenderBuffer && StencilRenderBuffer != DepthRenderBuffer)
		glDeleteTextures(1, &StencilRenderBuffer);
}


//combine depth texture and rtt
bool COpenGLFBODepthTexture::attach(ITexture* renderTex)
{
	if (!renderTex)
		return false;
	video::COpenGLFBOTexture* rtt = static_cast<video::COpenGLFBOTexture*>(renderTex);
	rtt->bindRTT();
#ifdef GL_EXT_framebuffer_object
	if (UseStencil)
	{
		// attach stencil texture to stencil buffer
		Driver->extGlFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
						GL_STENCIL_ATTACHMENT_EXT,
						GL_TEXTURE_2D,
						StencilRenderBuffer,
						0);

		// attach depth texture to depth buffer
		Driver->extGlFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
						GL_DEPTH_ATTACHMENT_EXT,
						GL_TEXTURE_2D,
						DepthRenderBuffer,
						0);
	}
	else
	{
		// attach depth renderbuffer to depth buffer
		Driver->extGlFramebufferRenderbuffer(GL_FRAMEBUFFER_EXT,
						GL_DEPTH_ATTACHMENT_EXT,
						GL_RENDERBUFFER_EXT,
						DepthRenderBuffer);
	}
#endif
	// check the status
	if (!checkFBOStatus(Driver))
	{
		os::Printer::log("FBO incomplete");
		return false;
	}
	rtt->DepthTexture=this;
	grab(); // grab the depth buffer, not the RTT
	rtt->unbindRTT();
	return true;
}


//! Bind Render Target Texture
void COpenGLFBODepthTexture::bindRTT()
{
}


//! Unbind Render Target Texture
void COpenGLFBODepthTexture::unbindRTT()
{
}


bool checkFBOStatus(COpenGLDriver* Driver)
{
#ifdef GL_EXT_framebuffer_object
	GLenum status = Driver->extGlCheckFramebufferStatus(GL_FRAMEBUFFER_EXT);

	switch (status)
	{
		//Our FBO is perfect, return true
		case GL_FRAMEBUFFER_COMPLETE_EXT:
			return true;

		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
			os::Printer::log("FBO has invalid read buffer", ELL_ERROR);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
			os::Printer::log("FBO has invalid draw buffer", ELL_ERROR);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
			os::Printer::log("FBO has one or several incomplete image attachments", ELL_ERROR);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
			os::Printer::log("FBO has one or several image attachments with different internal formats", ELL_ERROR);
			break;

		case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
			os::Printer::log("FBO has one or several image attachments with different dimensions", ELL_ERROR);
			break;

// not part of fbo_object anymore, but won't harm as it is just a return value
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT
		case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT:
			os::Printer::log("FBO has a duplicate image attachment", ELL_ERROR);
			break;
#endif

		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
			os::Printer::log("FBO missing an image attachment", ELL_ERROR);
			break;

#ifdef GL_EXT_framebuffer_multisample
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
			os::Printer::log("FBO wrong multisample setup", ELL_ERROR);
			break;
#endif

		case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
			os::Printer::log("FBO format unsupported", ELL_ERROR);
			break;

		default:
			break;
	}
#endif
	os::Printer::log("FBO error", ELL_ERROR);
//	_IRR_DEBUG_BREAK_IF(true);
	return false;
}


} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_OPENGL_

