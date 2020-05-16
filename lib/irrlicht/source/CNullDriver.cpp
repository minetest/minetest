// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CNullDriver.h"
#include "os.h"
#include "CImage.h"
#include "CAttributes.h"
#include "IReadFile.h"
#include "IWriteFile.h"
#include "IImageLoader.h"
#include "IImageWriter.h"
#include "IMaterialRenderer.h"
#include "IAnimatedMeshSceneNode.h"
#include "CMeshManipulator.h"
#include "CColorConverter.h"
#include "IAttributeExchangingObject.h"


namespace irr
{
namespace video
{

//! creates a loader which is able to load windows bitmaps
IImageLoader* createImageLoaderBMP();

//! creates a loader which is able to load jpeg images
IImageLoader* createImageLoaderJPG();

//! creates a loader which is able to load targa images
IImageLoader* createImageLoaderTGA();

//! creates a loader which is able to load psd images
IImageLoader* createImageLoaderPSD();

//! creates a loader which is able to load dds images
IImageLoader* createImageLoaderDDS();

//! creates a loader which is able to load pcx images
IImageLoader* createImageLoaderPCX();

//! creates a loader which is able to load png images
IImageLoader* createImageLoaderPNG();

//! creates a loader which is able to load WAL images
IImageLoader* createImageLoaderWAL();

//! creates a loader which is able to load halflife images
IImageLoader* createImageLoaderHalfLife();

//! creates a loader which is able to load lmp images
IImageLoader* createImageLoaderLMP();

//! creates a loader which is able to load ppm/pgm/pbm images
IImageLoader* createImageLoaderPPM();

//! creates a loader which is able to load rgb images
IImageLoader* createImageLoaderRGB();


//! creates a writer which is able to save bmp images
IImageWriter* createImageWriterBMP();

//! creates a writer which is able to save jpg images
IImageWriter* createImageWriterJPG();

//! creates a writer which is able to save tga images
IImageWriter* createImageWriterTGA();

//! creates a writer which is able to save psd images
IImageWriter* createImageWriterPSD();

//! creates a writer which is able to save pcx images
IImageWriter* createImageWriterPCX();

//! creates a writer which is able to save png images
IImageWriter* createImageWriterPNG();

//! creates a writer which is able to save ppm images
IImageWriter* createImageWriterPPM();

//! constructor
CNullDriver::CNullDriver(io::IFileSystem* io, const core::dimension2d<u32>& screenSize)
: FileSystem(io), MeshManipulator(0), ViewPort(0,0,0,0), ScreenSize(screenSize),
	PrimitivesDrawn(0), MinVertexCountForVBO(500), TextureCreationFlags(0),
	OverrideMaterial2DEnabled(false), AllowZWriteOnTransparent(false)
{
	#ifdef _DEBUG
	setDebugName("CNullDriver");
	#endif

	DriverAttributes = new io::CAttributes();
	DriverAttributes->addInt("MaxTextures", _IRR_MATERIAL_MAX_TEXTURES_);
	DriverAttributes->addInt("MaxSupportedTextures", _IRR_MATERIAL_MAX_TEXTURES_);
	DriverAttributes->addInt("MaxLights", getMaximalDynamicLightAmount());
	DriverAttributes->addInt("MaxAnisotropy", 1);
//	DriverAttributes->addInt("MaxUserClipPlanes", 0);
//	DriverAttributes->addInt("MaxAuxBuffers", 0);
	DriverAttributes->addInt("MaxMultipleRenderTargets", 1);
	DriverAttributes->addInt("MaxIndices", -1);
	DriverAttributes->addInt("MaxTextureSize", -1);
//	DriverAttributes->addInt("MaxGeometryVerticesOut", 0);
//	DriverAttributes->addFloat("MaxTextureLODBias", 0.f);
	DriverAttributes->addInt("Version", 1);
//	DriverAttributes->addInt("ShaderLanguageVersion", 0);
//	DriverAttributes->addInt("AntiAlias", 0);

	setFog();

	setTextureCreationFlag(ETCF_ALWAYS_32_BIT, true);
	setTextureCreationFlag(ETCF_CREATE_MIP_MAPS, true);

	ViewPort = core::rect<s32>(core::position2d<s32>(0,0), core::dimension2di(screenSize));

	// create manipulator
	MeshManipulator = new scene::CMeshManipulator();

	if (FileSystem)
		FileSystem->grab();

	// create surface loader

#ifdef _IRR_COMPILE_WITH_WAL_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderHalfLife());
	SurfaceLoader.push_back(video::createImageLoaderWAL());
#endif
#ifdef _IRR_COMPILE_WITH_LMP_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderLMP());
#endif
#ifdef _IRR_COMPILE_WITH_PPM_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderPPM());
#endif
#ifdef _IRR_COMPILE_WITH_RGB_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderRGB());
#endif
#ifdef _IRR_COMPILE_WITH_PSD_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderPSD());
#endif
#ifdef _IRR_COMPILE_WITH_DDS_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderDDS());
#endif
#ifdef _IRR_COMPILE_WITH_PCX_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderPCX());
#endif
#ifdef _IRR_COMPILE_WITH_TGA_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderTGA());
#endif
#ifdef _IRR_COMPILE_WITH_PNG_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderPNG());
#endif
#ifdef _IRR_COMPILE_WITH_JPG_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderJPG());
#endif
#ifdef _IRR_COMPILE_WITH_BMP_LOADER_
	SurfaceLoader.push_back(video::createImageLoaderBMP());
#endif


#ifdef _IRR_COMPILE_WITH_PPM_WRITER_
	SurfaceWriter.push_back(video::createImageWriterPPM());
#endif
#ifdef _IRR_COMPILE_WITH_PCX_WRITER_
	SurfaceWriter.push_back(video::createImageWriterPCX());
#endif
#ifdef _IRR_COMPILE_WITH_PSD_WRITER_
	SurfaceWriter.push_back(video::createImageWriterPSD());
#endif
#ifdef _IRR_COMPILE_WITH_TGA_WRITER_
	SurfaceWriter.push_back(video::createImageWriterTGA());
#endif
#ifdef _IRR_COMPILE_WITH_JPG_WRITER_
	SurfaceWriter.push_back(video::createImageWriterJPG());
#endif
#ifdef _IRR_COMPILE_WITH_PNG_WRITER_
	SurfaceWriter.push_back(video::createImageWriterPNG());
#endif
#ifdef _IRR_COMPILE_WITH_BMP_WRITER_
	SurfaceWriter.push_back(video::createImageWriterBMP());
#endif


	// set ExposedData to 0
	memset(&ExposedData, 0, sizeof(ExposedData));
	for (u32 i=0; i<video::EVDF_COUNT; ++i)
		FeatureEnabled[i]=true;

	InitMaterial2D.AntiAliasing=video::EAAM_OFF;
	InitMaterial2D.Lighting=false;
	InitMaterial2D.ZWriteEnable=false;
	InitMaterial2D.ZBuffer=video::ECFN_NEVER;
	InitMaterial2D.UseMipMaps=false;
	for (u32 i=0; i<video::MATERIAL_MAX_TEXTURES; ++i)
	{
		InitMaterial2D.TextureLayer[i].BilinearFilter=false;
		InitMaterial2D.TextureLayer[i].TextureWrapU=video::ETC_REPEAT;
		InitMaterial2D.TextureLayer[i].TextureWrapV=video::ETC_REPEAT;
	}
	OverrideMaterial2D=InitMaterial2D;
}


//! destructor
CNullDriver::~CNullDriver()
{
	if (DriverAttributes)
		DriverAttributes->drop();

	if (FileSystem)
		FileSystem->drop();

	if (MeshManipulator)
		MeshManipulator->drop();
	deleteAllTextures();

	u32 i;
	for (i=0; i<SurfaceLoader.size(); ++i)
		SurfaceLoader[i]->drop();

	for (i=0; i<SurfaceWriter.size(); ++i)
		SurfaceWriter[i]->drop();

	// delete material renderers
	deleteMaterialRenders();

	// delete hardware mesh buffers
	removeAllHardwareBuffers();
}


//! Adds an external surface loader to the engine.
void CNullDriver::addExternalImageLoader(IImageLoader* loader)
{
	if (!loader)
		return;

	loader->grab();
	SurfaceLoader.push_back(loader);
}


//! Adds an external surface writer to the engine.
void CNullDriver::addExternalImageWriter(IImageWriter* writer)
{
	if (!writer)
		return;

	writer->grab();
	SurfaceWriter.push_back(writer);
}


//! Retrieve the number of image loaders
u32 CNullDriver::getImageLoaderCount() const
{
	return SurfaceLoader.size();
}


//! Retrieve the given image loader
IImageLoader* CNullDriver::getImageLoader(u32 n)
{
	if (n < SurfaceLoader.size())
		return SurfaceLoader[n];
	return 0;
}


//! Retrieve the number of image writers
u32 CNullDriver::getImageWriterCount() const
{
	return SurfaceWriter.size();
}


//! Retrieve the given image writer
IImageWriter* CNullDriver::getImageWriter(u32 n)
{
	if (n < SurfaceWriter.size())
		return SurfaceWriter[n];
	return 0;
}


//! deletes all textures
void CNullDriver::deleteAllTextures()
{
	// we need to remove previously set textures which might otherwise be kept in the
	// last set material member. Could be optimized to reduce state changes.
	setMaterial(SMaterial());

	for (u32 i=0; i<Textures.size(); ++i)
		Textures[i].Surface->drop();

	Textures.clear();
}



//! applications must call this method before performing any rendering. returns false if failed.
bool CNullDriver::beginScene(bool backBuffer, bool zBuffer, SColor color,
		const SExposedVideoData& videoData, core::rect<s32>* sourceRect)
{
	core::clearFPUException();
	PrimitivesDrawn = 0;
	return true;
}


//! applications must call this method after performing any rendering. returns false if failed.
bool CNullDriver::endScene()
{
	FPSCounter.registerFrame(os::Timer::getRealTime(), PrimitivesDrawn);
	updateAllHardwareBuffers();
	updateAllOcclusionQueries();
	return true;
}


//! Disable a feature of the driver.
void CNullDriver::disableFeature(E_VIDEO_DRIVER_FEATURE feature, bool flag)
{
	FeatureEnabled[feature]=!flag;
}


//! queries the features of the driver, returns true if feature is available
bool CNullDriver::queryFeature(E_VIDEO_DRIVER_FEATURE feature) const
{
	return false;
}


//! Get attributes of the actual video driver
const io::IAttributes& CNullDriver::getDriverAttributes() const
{
	return *DriverAttributes;
}


//! sets transformation
void CNullDriver::setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat)
{
}


//! Returns the transformation set by setTransform
const core::matrix4& CNullDriver::getTransform(E_TRANSFORMATION_STATE state) const
{
	return TransformationMatrix;
}


//! sets a material
void CNullDriver::setMaterial(const SMaterial& material)
{
}


//! Removes a texture from the texture cache and deletes it, freeing lot of
//! memory.
void CNullDriver::removeTexture(ITexture* texture)
{
	if (!texture)
		return;

	for (u32 i=0; i<Textures.size(); ++i)
	{
		if (Textures[i].Surface == texture)
		{
			texture->drop();
			Textures.erase(i);
		}
	}
}


//! Removes all texture from the texture cache and deletes them, freeing lot of
//! memory.
void CNullDriver::removeAllTextures()
{
	setMaterial ( SMaterial() );
	deleteAllTextures();
}


//! Returns a texture by index
ITexture* CNullDriver::getTextureByIndex(u32 i)
{
	if ( i < Textures.size() )
		return Textures[i].Surface;

	return 0;
}


//! Returns amount of textures currently loaded
u32 CNullDriver::getTextureCount() const
{
	return Textures.size();
}


//! Renames a texture
void CNullDriver::renameTexture(ITexture* texture, const io::path& newName)
{
	// we can do a const_cast here safely, the name of the ITexture interface
	// is just readonly to prevent the user changing the texture name without invoking
	// this method, because the textures will need resorting afterwards

	io::SNamedPath& name = const_cast<io::SNamedPath&>(texture->getName());
	name.setPath(newName);

	Textures.sort();
}


//! loads a Texture
ITexture* CNullDriver::getTexture(const io::path& filename)
{
	// Identify textures by their absolute filenames if possible.
	const io::path absolutePath = FileSystem->getAbsolutePath(filename);

	ITexture* texture = findTexture(absolutePath);
	if (texture)
		return texture;

	// Then try the raw filename, which might be in an Archive
	texture = findTexture(filename);
	if (texture)
		return texture;

	// Now try to open the file using the complete path.
	io::IReadFile* file = FileSystem->createAndOpenFile(absolutePath);

	if (!file)
	{
		// Try to open it using the raw filename.
		file = FileSystem->createAndOpenFile(filename);
	}

	if (file)
	{
		// Re-check name for actual archive names
		texture = findTexture(file->getFileName());
		if (texture)
		{
			file->drop();
			return texture;
		}

		texture = loadTextureFromFile(file);
		file->drop();

		if (texture)
		{
			addTexture(texture);
			texture->drop(); // drop it because we created it, one grab too much
		}
		else
			os::Printer::log("Could not load texture", filename, ELL_ERROR);
		return texture;
	}
	else
	{
		os::Printer::log("Could not open file of texture", filename, ELL_WARNING);
		return 0;
	}
}


//! loads a Texture
ITexture* CNullDriver::getTexture(io::IReadFile* file)
{
	ITexture* texture = 0;

	if (file)
	{
		texture = findTexture(file->getFileName());

		if (texture)
			return texture;

		texture = loadTextureFromFile(file);

		if (texture)
		{
			addTexture(texture);
			texture->drop(); // drop it because we created it, one grab too much
		}

		if (!texture)
			os::Printer::log("Could not load texture", file->getFileName(), ELL_WARNING);
	}

	return texture;
}


//! opens the file and loads it into the surface
video::ITexture* CNullDriver::loadTextureFromFile(io::IReadFile* file, const io::path& hashName )
{
	ITexture* texture = 0;
	IImage* image = createImageFromFile(file);

	if (image)
	{
		// create texture from surface
		texture = createDeviceDependentTexture(image, hashName.size() ? hashName : file->getFileName() );
		os::Printer::log("Loaded texture", file->getFileName());
		image->drop();
	}

	return texture;
}


//! adds a surface, not loaded or created by the Irrlicht Engine
void CNullDriver::addTexture(video::ITexture* texture)
{
	if (texture)
	{
		SSurface s;
		s.Surface = texture;
		texture->grab();

		Textures.push_back(s);

		// the new texture is now at the end of the texture list. when searching for
		// the next new texture, the texture array will be sorted and the index of this texture
		// will be changed. to let the order be more consistent to the user, sort
		// the textures now already although this isn't necessary:

		Textures.sort();
	}
}


//! looks if the image is already loaded
video::ITexture* CNullDriver::findTexture(const io::path& filename)
{
	SSurface s;
	SDummyTexture dummy(filename);
	s.Surface = &dummy;

	s32 index = Textures.binary_search(s);
	if (index != -1)
		return Textures[index].Surface;

	return 0;
}


//! Creates a texture from a loaded IImage.
ITexture* CNullDriver::addTexture(const io::path& name, IImage* image, void* mipmapData)
{
	if ( 0 == name.size() || !image)
		return 0;

	ITexture* t = createDeviceDependentTexture(image, name, mipmapData);
	if (t)
	{
		addTexture(t);
		t->drop();
	}
	return t;
}


//! creates a Texture
ITexture* CNullDriver::addTexture(const core::dimension2d<u32>& size,
				  const io::path& name, ECOLOR_FORMAT format)
{
	if(IImage::isRenderTargetOnlyFormat(format))
	{
		os::Printer::log("Could not create ITexture, format only supported for render target textures.", ELL_WARNING);
		return 0;
	}

	if ( 0 == name.size () )
		return 0;

	IImage* image = new CImage(format, size);
	ITexture* t = createDeviceDependentTexture(image, name);
	image->drop();
	addTexture(t);

	if (t)
		t->drop();

	return t;
}



//! returns a device dependent texture from a software surface (IImage)
//! THIS METHOD HAS TO BE OVERRIDDEN BY DERIVED DRIVERS WITH OWN TEXTURES
ITexture* CNullDriver::createDeviceDependentTexture(IImage* surface, const io::path& name, void* mipmapData)
{
	return new SDummyTexture(name);
}


//! set or reset special render targets
bool CNullDriver::setRenderTarget(video::E_RENDER_TARGET target, bool clearTarget,
			bool clearZBuffer, SColor color)
{
	if (ERT_FRAME_BUFFER==target)
		return setRenderTarget(0,clearTarget, clearZBuffer, color);
	else
		return false;
}


//! sets a render target
bool CNullDriver::setRenderTarget(video::ITexture* texture, bool clearBackBuffer,
					bool clearZBuffer, SColor color)
{
	return false;
}


//! Sets multiple render targets
bool CNullDriver::setRenderTarget(const core::array<video::IRenderTarget>& texture,
				bool clearBackBuffer, bool clearZBuffer, SColor color)
{
	return false;
}


//! sets a viewport
void CNullDriver::setViewPort(const core::rect<s32>& area)
{
}


//! gets the area of the current viewport
const core::rect<s32>& CNullDriver::getViewPort() const
{
	return ViewPort;
}


//! draws a vertex primitive list
void CNullDriver::drawVertexPrimitiveList(const void* vertices, u32 vertexCount, const void* indexList, u32 primitiveCount, E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType)
{
	if ((iType==EIT_16BIT) && (vertexCount>65536))
		os::Printer::log("Too many vertices for 16bit index type, render artifacts may occur.");
	PrimitivesDrawn += primitiveCount;
}


//! draws a vertex primitive list in 2d
void CNullDriver::draw2DVertexPrimitiveList(const void* vertices, u32 vertexCount, const void* indexList, u32 primitiveCount, E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType)
{
	if ((iType==EIT_16BIT) && (vertexCount>65536))
		os::Printer::log("Too many vertices for 16bit index type, render artifacts may occur.");
	PrimitivesDrawn += primitiveCount;
}


//! Draws a 3d line.
void CNullDriver::draw3DLine(const core::vector3df& start,
				const core::vector3df& end, SColor color)
{
}


//! Draws a 3d triangle.
void CNullDriver::draw3DTriangle(const core::triangle3df& triangle, SColor color)
{
	S3DVertex vertices[3];
	vertices[0].Pos=triangle.pointA;
	vertices[0].Color=color;
	vertices[0].Normal=triangle.getNormal().normalize();
	vertices[0].TCoords.set(0.f,0.f);
	vertices[1].Pos=triangle.pointB;
	vertices[1].Color=color;
	vertices[1].Normal=vertices[0].Normal;
	vertices[1].TCoords.set(0.5f,1.f);
	vertices[2].Pos=triangle.pointC;
	vertices[2].Color=color;
	vertices[2].Normal=vertices[0].Normal;
	vertices[2].TCoords.set(1.f,0.f);
	const u16 indexList[] = {0,1,2};
	drawVertexPrimitiveList(vertices, 3, indexList, 1, EVT_STANDARD, scene::EPT_TRIANGLES, EIT_16BIT);
}


//! Draws a 3d axis aligned box.
void CNullDriver::draw3DBox(const core::aabbox3d<f32>& box, SColor color)
{
	core::vector3df edges[8];
	box.getEdges(edges);

	// TODO: optimize into one big drawIndexPrimitive call.

	draw3DLine(edges[5], edges[1], color);
	draw3DLine(edges[1], edges[3], color);
	draw3DLine(edges[3], edges[7], color);
	draw3DLine(edges[7], edges[5], color);
	draw3DLine(edges[0], edges[2], color);
	draw3DLine(edges[2], edges[6], color);
	draw3DLine(edges[6], edges[4], color);
	draw3DLine(edges[4], edges[0], color);
	draw3DLine(edges[1], edges[0], color);
	draw3DLine(edges[3], edges[2], color);
	draw3DLine(edges[7], edges[6], color);
	draw3DLine(edges[5], edges[4], color);
}



//! draws an 2d image
void CNullDriver::draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos)
{
	if (!texture)
		return;

	draw2DImage(texture,destPos, core::rect<s32>(core::position2d<s32>(0,0),
												core::dimension2di(texture->getOriginalSize())));
}



//! draws a set of 2d images, using a color and the alpha channel of the
//! texture if desired. The images are drawn beginning at pos and concatenated
//! in one line. All drawings are clipped against clipRect (if != 0).
//! The subtextures are defined by the array of sourceRects and are chosen
//! by the indices given.
void CNullDriver::draw2DImageBatch(const video::ITexture* texture,
				const core::position2d<s32>& pos,
				const core::array<core::rect<s32> >& sourceRects,
				const core::array<s32>& indices,
				s32 kerningWidth,
				const core::rect<s32>* clipRect, SColor color,
				bool useAlphaChannelOfTexture)
{
	core::position2d<s32> target(pos);

	for (u32 i=0; i<indices.size(); ++i)
	{
		draw2DImage(texture, target, sourceRects[indices[i]],
				clipRect, color, useAlphaChannelOfTexture);
		target.X += sourceRects[indices[i]].getWidth();
		target.X += kerningWidth;
	}
}

//! draws a set of 2d images, using a color and the alpha channel of the
//! texture if desired.
void CNullDriver::draw2DImageBatch(const video::ITexture* texture,
				const core::array<core::position2d<s32> >& positions,
				const core::array<core::rect<s32> >& sourceRects,
				const core::rect<s32>* clipRect,
				SColor color,
				bool useAlphaChannelOfTexture)
{
	const irr::u32 drawCount = core::min_<u32>(positions.size(), sourceRects.size());

	for (u32 i=0; i<drawCount; ++i)
	{
		draw2DImage(texture, positions[i], sourceRects[i],
				clipRect, color, useAlphaChannelOfTexture);
	}
}


//! Draws a part of the texture into the rectangle.
void CNullDriver::draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
	const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect,
	const video::SColor* const colors, bool useAlphaChannelOfTexture)
{
	if (destRect.isValid())
		draw2DImage(texture, core::position2d<s32>(destRect.UpperLeftCorner),
				sourceRect, clipRect, colors?colors[0]:video::SColor(0xffffffff),
				useAlphaChannelOfTexture);
}


//! Draws a 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
void CNullDriver::draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
				const core::rect<s32>& sourceRect,
				const core::rect<s32>* clipRect, SColor color,
				bool useAlphaChannelOfTexture)
{
}


//! Draws the outline of a 2d rectangle
void CNullDriver::draw2DRectangleOutline(const core::recti& pos, SColor color)
{
	draw2DLine(pos.UpperLeftCorner, core::position2di(pos.LowerRightCorner.X, pos.UpperLeftCorner.Y), color);
	draw2DLine(core::position2di(pos.LowerRightCorner.X, pos.UpperLeftCorner.Y), pos.LowerRightCorner, color);
	draw2DLine(pos.LowerRightCorner, core::position2di(pos.UpperLeftCorner.X, pos.LowerRightCorner.Y), color);
	draw2DLine(core::position2di(pos.UpperLeftCorner.X, pos.LowerRightCorner.Y), pos.UpperLeftCorner, color);
}


//! Draw a 2d rectangle
void CNullDriver::draw2DRectangle(SColor color, const core::rect<s32>& pos, const core::rect<s32>* clip)
{
	draw2DRectangle(pos, color, color, color, color, clip);
}



//! Draws a 2d rectangle with a gradient.
void CNullDriver::draw2DRectangle(const core::rect<s32>& pos,
	SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
	const core::rect<s32>* clip)
{
}



//! Draws a 2d line.
void CNullDriver::draw2DLine(const core::position2d<s32>& start,
				const core::position2d<s32>& end, SColor color)
{
}

//! Draws a pixel
void CNullDriver::drawPixel(u32 x, u32 y, const SColor & color)
{
}


//! Draws a non filled concyclic regular 2d polygon.
void CNullDriver::draw2DPolygon(core::position2d<s32> center,
	f32 radius, video::SColor color, s32 count)
{
	if (count < 2)
		return;

	core::position2d<s32> first;
	core::position2d<s32> a,b;

	for (s32 j=0; j<count; ++j)
	{
		b = a;

		f32 p = j / (f32)count * (core::PI*2);
		a = center + core::position2d<s32>((s32)(sin(p)*radius), (s32)(cos(p)*radius));

		if (j==0)
			first = a;
		else
			draw2DLine(a, b, color);
	}

	draw2DLine(a, first, color);
}


//! returns color format
ECOLOR_FORMAT CNullDriver::getColorFormat() const
{
	return ECF_R5G6B5;
}


//! returns screen size
const core::dimension2d<u32>& CNullDriver::getScreenSize() const
{
	return ScreenSize;
}


//! returns the current render target size,
//! or the screen size if render targets are not implemented
const core::dimension2d<u32>& CNullDriver::getCurrentRenderTargetSize() const
{
	return ScreenSize;
}


// returns current frames per second value
s32 CNullDriver::getFPS() const
{
	return FPSCounter.getFPS();
}



//! returns amount of primitives (mostly triangles) were drawn in the last frame.
//! very useful method for statistics.
u32 CNullDriver::getPrimitiveCountDrawn( u32 param ) const
{
	return (0 == param) ? FPSCounter.getPrimitive() : (1 == param) ? FPSCounter.getPrimitiveAverage() : FPSCounter.getPrimitiveTotal();
}



//! Sets the dynamic ambient light color. The default color is
//! (0,0,0,0) which means it is dark.
//! \param color: New color of the ambient light.
void CNullDriver::setAmbientLight(const SColorf& color)
{
}



//! \return Returns the name of the video driver. Example: In case of the DIRECT3D8
//! driver, it would return "Direct3D8".

const wchar_t* CNullDriver::getName() const
{
	return L"Irrlicht NullDevice";
}



//! Draws a shadow volume into the stencil buffer. To draw a stencil shadow, do
//! this: Frist, draw all geometry. Then use this method, to draw the shadow
//! volume. Then, use IVideoDriver::drawStencilShadow() to visualize the shadow.
void CNullDriver::drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible)
{
}


//! Fills the stencil shadow with color. After the shadow volume has been drawn
//! into the stencil buffer using IVideoDriver::drawStencilShadowVolume(), use this
//! to draw the color of the shadow.
void CNullDriver::drawStencilShadow(bool clearStencilBuffer,
		video::SColor leftUpEdge, video::SColor rightUpEdge,
		video::SColor leftDownEdge, video::SColor rightDownEdge)
{
}


//! deletes all dynamic lights there are
void CNullDriver::deleteAllDynamicLights()
{
	Lights.set_used(0);
}


//! adds a dynamic light
s32 CNullDriver::addDynamicLight(const SLight& light)
{
	Lights.push_back(light);
	return Lights.size() - 1;
}

//! Turns a dynamic light on or off
//! \param lightIndex: the index returned by addDynamicLight
//! \param turnOn: true to turn the light on, false to turn it off
void CNullDriver::turnLightOn(s32 lightIndex, bool turnOn)
{
	// Do nothing
}


//! returns the maximal amount of dynamic lights the device can handle
u32 CNullDriver::getMaximalDynamicLightAmount() const
{
	return 0;
}


//! Returns current amount of dynamic lights set
//! \return Current amount of dynamic lights set
u32 CNullDriver::getDynamicLightCount() const
{
	return Lights.size();
}


//! Returns light data which was previously set by IVideoDriver::addDynamicLight().
//! \param idx: Zero based index of the light. Must be greater than 0 and smaller
//! than IVideoDriver()::getDynamicLightCount.
//! \return Light data.
const SLight& CNullDriver::getDynamicLight(u32 idx) const
{
	if ( idx < Lights.size() )
		return Lights[idx];
	else
		return *((SLight*)0);
}


//! Creates a boolean alpha channel of the texture based of an color key.
void CNullDriver::makeColorKeyTexture(video::ITexture* texture,
									video::SColor color,
									bool zeroTexels) const
{
	if (!texture)
		return;

	if (texture->getColorFormat() != ECF_A1R5G5B5 &&
		texture->getColorFormat() != ECF_A8R8G8B8 )
	{
		os::Printer::log("Error: Unsupported texture color format for making color key channel.", ELL_ERROR);
		return;
	}

	if (texture->getColorFormat() == ECF_A1R5G5B5)
	{
		u16 *p = (u16*)texture->lock();

		if (!p)
		{
			os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
			return;
		}

		const core::dimension2d<u32> dim = texture->getSize();
		const u32 pitch = texture->getPitch() / 2;

		// color with alpha disabled (i.e. fully transparent)
		const u16 refZeroAlpha = (0x7fff & color.toA1R5G5B5());

		const u32 pixels = pitch * dim.Height;

		for (u32 pixel = 0; pixel < pixels; ++ pixel)
		{
			// If the color matches the reference color, ignoring alphas,
			// set the alpha to zero.
			if(((*p) & 0x7fff) == refZeroAlpha)
			{
				if(zeroTexels)
					(*p) = 0;
				else
					(*p) = refZeroAlpha;
			}

			++p;
		}

		texture->unlock();
	}
	else
	{
		u32 *p = (u32*)texture->lock();

		if (!p)
		{
			os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
			return;
		}

		core::dimension2d<u32> dim = texture->getSize();
		u32 pitch = texture->getPitch() / 4;

		// color with alpha disabled (fully transparent)
		const u32 refZeroAlpha = 0x00ffffff & color.color;

		const u32 pixels = pitch * dim.Height;
		for (u32 pixel = 0; pixel < pixels; ++ pixel)
		{
			// If the color matches the reference color, ignoring alphas,
			// set the alpha to zero.
			if(((*p) & 0x00ffffff) == refZeroAlpha)
			{
				if(zeroTexels)
					(*p) = 0;
				else
					(*p) = refZeroAlpha;
			}

			++p;
		}

		texture->unlock();
	}
	texture->regenerateMipMapLevels();
}



//! Creates an boolean alpha channel of the texture based of an color key position.
void CNullDriver::makeColorKeyTexture(video::ITexture* texture,
					core::position2d<s32> colorKeyPixelPos,
					bool zeroTexels) const
{
	if (!texture)
		return;

	if (texture->getColorFormat() != ECF_A1R5G5B5 &&
		texture->getColorFormat() != ECF_A8R8G8B8 )
	{
		os::Printer::log("Error: Unsupported texture color format for making color key channel.", ELL_ERROR);
		return;
	}

	SColor colorKey;

	if (texture->getColorFormat() == ECF_A1R5G5B5)
	{
		u16 *p = (u16*)texture->lock(ETLM_READ_ONLY);

		if (!p)
		{
			os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
			return;
		}

		u32 pitch = texture->getPitch() / 2;

		const u16 key16Bit = 0x7fff & p[colorKeyPixelPos.Y*pitch + colorKeyPixelPos.X];

		colorKey = video::A1R5G5B5toA8R8G8B8(key16Bit);
	}
	else
	{
		u32 *p = (u32*)texture->lock(ETLM_READ_ONLY);

		if (!p)
		{
			os::Printer::log("Could not lock texture for making color key channel.", ELL_ERROR);
			return;
		}

		u32 pitch = texture->getPitch() / 4;
		colorKey = 0x00ffffff & p[colorKeyPixelPos.Y*pitch + colorKeyPixelPos.X];
	}

	texture->unlock();
	makeColorKeyTexture(texture, colorKey, zeroTexels);
}



//! Creates a normal map from a height map texture.
//! \param amplitude: Constant value by which the height information is multiplied.
void CNullDriver::makeNormalMapTexture(video::ITexture* texture, f32 amplitude) const
{
	if (!texture)
		return;

	if (texture->getColorFormat() != ECF_A1R5G5B5 &&
		texture->getColorFormat() != ECF_A8R8G8B8 )
	{
		os::Printer::log("Error: Unsupported texture color format for making normal map.", ELL_ERROR);
		return;
	}

	core::dimension2d<u32> dim = texture->getSize();
	amplitude = amplitude / 255.0f;
	f32 vh = dim.Height / (f32)dim.Width;
	f32 hh = dim.Width / (f32)dim.Height;

	if (texture->getColorFormat() == ECF_A8R8G8B8)
	{
		// ECF_A8R8G8B8 version

		s32 *p = (s32*)texture->lock();

		if (!p)
		{
			os::Printer::log("Could not lock texture for making normal map.", ELL_ERROR);
			return;
		}

		// copy texture

		u32 pitch = texture->getPitch() / 4;

		s32* in = new s32[dim.Height * pitch];
		memcpy(in, p, dim.Height * pitch * 4);

		for (s32 x=0; x < s32(pitch); ++x)
			for (s32 y=0; y < s32(dim.Height); ++y)
			{
				// TODO: this could be optimized really a lot

				core::vector3df h1((x-1)*hh, nml32(x-1, y, pitch, dim.Height, in)*amplitude, y*vh);
				core::vector3df h2((x+1)*hh, nml32(x+1, y, pitch, dim.Height, in)*amplitude, y*vh);
				//core::vector3df v1(x*hh, nml32(x, y-1, pitch, dim.Height, in)*amplitude, (y-1)*vh);
				//core::vector3df v2(x*hh, nml32(x, y+1, pitch, dim.Height, in)*amplitude, (y+1)*vh);
				core::vector3df v1(x*hh, nml32(x, y+1, pitch, dim.Height, in)*amplitude, (y-1)*vh);
				core::vector3df v2(x*hh, nml32(x, y-1, pitch, dim.Height, in)*amplitude, (y+1)*vh);

				core::vector3df v = v1-v2;
				core::vector3df h = h1-h2;

				core::vector3df n = v.crossProduct(h);
				n.normalize();
				n *= 0.5f;
				n += core::vector3df(0.5f,0.5f,0.5f); // now between 0 and 1
				n *= 255.0f;

				s32 height = (s32)nml32(x, y, pitch, dim.Height, in);
				p[y*pitch + x] = video::SColor(
					height, // store height in alpha
					(s32)n.X, (s32)n.Z, (s32)n.Y).color;
			}

		delete [] in;
		texture->unlock();
	}
	else
	{
		// ECF_A1R5G5B5 version

		s16 *p = (s16*)texture->lock();

		if (!p)
		{
			os::Printer::log("Could not lock texture for making normal map.", ELL_ERROR);
			return;
		}

		u32 pitch = texture->getPitch() / 2;

		// copy texture

		s16* in = new s16[dim.Height * pitch];
		memcpy(in, p, dim.Height * pitch * 2);

		for (s32 x=0; x < s32(pitch); ++x)
			for (s32 y=0; y < s32(dim.Height); ++y)
			{
				// TODO: this could be optimized really a lot

				core::vector3df h1((x-1)*hh, nml16(x-1, y, pitch, dim.Height, in)*amplitude, y*vh);
				core::vector3df h2((x+1)*hh, nml16(x+1, y, pitch, dim.Height, in)*amplitude, y*vh);
				core::vector3df v1(x*hh, nml16(x, y-1, pitch, dim.Height, in)*amplitude, (y-1)*vh);
				core::vector3df v2(x*hh, nml16(x, y+1, pitch, dim.Height, in)*amplitude, (y+1)*vh);

				core::vector3df v = v1-v2;
				core::vector3df h = h1-h2;

				core::vector3df n = v.crossProduct(h);
				n.normalize();
				n *= 0.5f;
				n += core::vector3df(0.5f,0.5f,0.5f); // now between 0 and 1
				n *= 255.0f;

				p[y*pitch + x] = video::RGBA16((u32)n.X, (u32)n.Z, (u32)n.Y);
			}

		delete [] in;
		texture->unlock();
	}

	texture->regenerateMipMapLevels();
}


//! Returns the maximum amount of primitives (mostly vertices) which
//! the device is able to render with one drawIndexedTriangleList
//! call.
u32 CNullDriver::getMaximalPrimitiveCount() const
{
	return 0xFFFFFFFF;
}


//! checks triangle count and print warning if wrong
bool CNullDriver::checkPrimitiveCount(u32 prmCount) const
{
	const u32 m = getMaximalPrimitiveCount();

	if (prmCount > m)
	{
		char tmp[1024];
		sprintf(tmp,"Could not draw triangles, too many primitives(%u), maxium is %u.", prmCount, m);
		os::Printer::log(tmp, ELL_ERROR);
		return false;
	}

	return true;
}

//! Enables or disables a texture creation flag.
void CNullDriver::setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag, bool enabled)
{
	if (enabled && ((flag == ETCF_ALWAYS_16_BIT) || (flag == ETCF_ALWAYS_32_BIT)
		|| (flag == ETCF_OPTIMIZED_FOR_QUALITY) || (flag == ETCF_OPTIMIZED_FOR_SPEED)))
	{
		// disable other formats
		setTextureCreationFlag(ETCF_ALWAYS_16_BIT, false);
		setTextureCreationFlag(ETCF_ALWAYS_32_BIT, false);
		setTextureCreationFlag(ETCF_OPTIMIZED_FOR_QUALITY, false);
		setTextureCreationFlag(ETCF_OPTIMIZED_FOR_SPEED, false);
	}

	// set flag
	TextureCreationFlags = (TextureCreationFlags & (~flag)) |
		((((u32)!enabled)-1) & flag);
}


//! Returns if a texture creation flag is enabled or disabled.
bool CNullDriver::getTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag) const
{
	return (TextureCreationFlags & flag)!=0;
}


//! Creates a software image from a file.
IImage* CNullDriver::createImageFromFile(const io::path& filename)
{
	if (!filename.size())
		return 0;

	IImage* image = 0;
	io::IReadFile* file = FileSystem->createAndOpenFile(filename);

	if (file)
	{
		image = createImageFromFile(file);
		file->drop();
	}
	else
		os::Printer::log("Could not open file of image", filename, ELL_WARNING);

	return image;
}


//! Creates a software image from a file.
IImage* CNullDriver::createImageFromFile(io::IReadFile* file)
{
	if (!file)
		return 0;

	IImage* image = 0;

	s32 i;

	// try to load file based on file extension
	for (i=SurfaceLoader.size()-1; i>=0; --i)
	{
		if (SurfaceLoader[i]->isALoadableFileExtension(file->getFileName()))
		{
			// reset file position which might have changed due to previous loadImage calls
			file->seek(0);
			image = SurfaceLoader[i]->loadImage(file);
			if (image)
				return image;
		}
	}

	// try to load file based on what is in it
	for (i=SurfaceLoader.size()-1; i>=0; --i)
	{
		// dito
		file->seek(0);
		if (SurfaceLoader[i]->isALoadableFileFormat(file))
		{
			file->seek(0);
			image = SurfaceLoader[i]->loadImage(file);
			if (image)
				return image;
		}
	}

	return 0; // failed to load
}


//! Writes the provided image to disk file
bool CNullDriver::writeImageToFile(IImage* image, const io::path& filename,u32 param)
{
	io::IWriteFile* file = FileSystem->createAndWriteFile(filename);
	if(!file)
		return false;

	bool result = writeImageToFile(image, file, param);
	file->drop();

	return result;
}

//! Writes the provided image to a file.
bool CNullDriver::writeImageToFile(IImage* image, io::IWriteFile * file, u32 param)
{
	if(!file)
		return false;

	for (s32 i=SurfaceWriter.size()-1; i>=0; --i)
	{
		if (SurfaceWriter[i]->isAWriteableFileExtension(file->getFileName()))
		{
			bool written = SurfaceWriter[i]->writeImage(file, image, param);
			if (written)
				return true;
		}
	}
	return false; // failed to write
}


//! Creates a software image from a byte array.
IImage* CNullDriver::createImageFromData(ECOLOR_FORMAT format,
					const core::dimension2d<u32>& size,
					void *data, bool ownForeignMemory,
					bool deleteMemory)
{
	if(IImage::isRenderTargetOnlyFormat(format))
	{
		os::Printer::log("Could not create IImage, format only supported for render target textures.", ELL_WARNING);
		return 0;
	}

	return new CImage(format, size, data, ownForeignMemory, deleteMemory);
}


//! Creates an empty software image.
IImage* CNullDriver::createImage(ECOLOR_FORMAT format, const core::dimension2d<u32>& size)
{
	if(IImage::isRenderTargetOnlyFormat(format))
	{
		os::Printer::log("Could not create IImage, format only supported for render target textures.", ELL_WARNING);
		return 0;
	}

	return new CImage(format, size);
}


//! Creates a software image from another image.
IImage* CNullDriver::createImage(ECOLOR_FORMAT format, IImage *imageToCopy)
{
	os::Printer::log("Deprecated method, please create an empty image instead and use copyTo().", ELL_WARNING);
	if(IImage::isRenderTargetOnlyFormat(format))
	{
		os::Printer::log("Could not create IImage, format only supported for render target textures.", ELL_WARNING);
		return 0;
	}

	CImage* tmp = new CImage(format, imageToCopy->getDimension());
	imageToCopy->copyTo(tmp);
	return tmp;
}


//! Creates a software image from part of another image.
IImage* CNullDriver::createImage(IImage* imageToCopy, const core::position2d<s32>& pos, const core::dimension2d<u32>& size)
{
	os::Printer::log("Deprecated method, please create an empty image instead and use copyTo().", ELL_WARNING);
	CImage* tmp = new CImage(imageToCopy->getColorFormat(), imageToCopy->getDimension());
	imageToCopy->copyTo(tmp, core::position2di(0,0), core::recti(pos,size));
	return tmp;
}


//! Creates a software image from part of a texture.
IImage* CNullDriver::createImage(ITexture* texture, const core::position2d<s32>& pos, const core::dimension2d<u32>& size)
{
	if ((pos==core::position2di(0,0)) && (size == texture->getSize()))
	{
		IImage* image = new CImage(texture->getColorFormat(), size, texture->lock(ETLM_READ_ONLY), false);
		texture->unlock();
		return image;
	}
	else
	{
		// make sure to avoid buffer overruns
		// make the vector a separate variable for g++ 3.x
		const core::vector2d<u32> leftUpper(core::clamp(static_cast<u32>(pos.X), 0u, texture->getSize().Width),
					core::clamp(static_cast<u32>(pos.Y), 0u, texture->getSize().Height));
		const core::rect<u32> clamped(leftUpper,
					core::dimension2du(core::clamp(static_cast<u32>(size.Width), 0u, texture->getSize().Width),
					core::clamp(static_cast<u32>(size.Height), 0u, texture->getSize().Height)));
		if (!clamped.isValid())
			return 0;
		u8* src = static_cast<u8*>(texture->lock(ETLM_READ_ONLY));
		if (!src)
			return 0;
		IImage* image = new CImage(texture->getColorFormat(), clamped.getSize());
		u8* dst = static_cast<u8*>(image->lock());
		src += clamped.UpperLeftCorner.Y * texture->getPitch() + image->getBytesPerPixel() * clamped.UpperLeftCorner.X;
		for (u32 i=0; i<clamped.getHeight(); ++i)
		{
			video::CColorConverter::convert_viaFormat(src, texture->getColorFormat(), clamped.getWidth(), dst, image->getColorFormat());
			src += texture->getPitch();
			dst += image->getPitch();
		}
		image->unlock();
		texture->unlock();
		return image;
	}
}


//! Sets the fog mode.
void CNullDriver::setFog(SColor color, E_FOG_TYPE fogType, f32 start, f32 end,
		f32 density, bool pixelFog, bool rangeFog)
{
	FogColor = color;
	FogType = fogType;
	FogStart = start;
	FogEnd = end;
	FogDensity = density;
	PixelFog = pixelFog;
	RangeFog = rangeFog;
}

//! Gets the fog mode.
void CNullDriver::getFog(SColor& color, E_FOG_TYPE& fogType, f32& start, f32& end,
		f32& density, bool& pixelFog, bool& rangeFog)
{
	color = FogColor;
	fogType = FogType;
	start = FogStart;
	end = FogEnd;
	density = FogDensity;
	pixelFog = PixelFog;
	rangeFog = RangeFog;
}

//! Draws a mesh buffer
void CNullDriver::drawMeshBuffer(const scene::IMeshBuffer* mb)
{
	if (!mb)
		return;

	//IVertexBuffer and IIndexBuffer later
	SHWBufferLink *HWBuffer=getBufferLink(mb);

	if (HWBuffer)
		drawHardwareBuffer(HWBuffer);
	else
		drawVertexPrimitiveList(mb->getVertices(), mb->getVertexCount(), mb->getIndices(), mb->getIndexCount()/3, mb->getVertexType(), scene::EPT_TRIANGLES, mb->getIndexType());
}


//! Draws the normals of a mesh buffer
void CNullDriver::drawMeshBufferNormals(const scene::IMeshBuffer* mb, f32 length, SColor color)
{
	const u32 count = mb->getVertexCount();
	const bool normalize = mb->getMaterial().NormalizeNormals;

	for (u32 i=0; i < count; ++i)
	{
		core::vector3df normalizedNormal = mb->getNormal(i);
		if (normalize)
			normalizedNormal.normalize();

		const core::vector3df& pos = mb->getPosition(i);
		draw3DLine(pos, pos + (normalizedNormal * length), color);
	}
}


CNullDriver::SHWBufferLink *CNullDriver::getBufferLink(const scene::IMeshBuffer* mb)
{
	if (!mb || !isHardwareBufferRecommend(mb))
		return 0;

	//search for hardware links
	core::map< const scene::IMeshBuffer*,SHWBufferLink* >::Node* node = HWBufferMap.find(mb);
	if (node)
		return node->getValue();

	return createHardwareBuffer(mb); //no hardware links, and mesh wants one, create it
}


//! Update all hardware buffers, remove unused ones
void CNullDriver::updateAllHardwareBuffers()
{
	core::map<const scene::IMeshBuffer*,SHWBufferLink*>::ParentFirstIterator Iterator=HWBufferMap.getParentFirstIterator();

	for (;!Iterator.atEnd();Iterator++)
	{
		SHWBufferLink *Link=Iterator.getNode()->getValue();

		Link->LastUsed++;
		if (Link->LastUsed>20000)
		{
			deleteHardwareBuffer(Link);

			// todo: needs better fix
			Iterator = HWBufferMap.getParentFirstIterator();
		}
	}
}


void CNullDriver::deleteHardwareBuffer(SHWBufferLink *HWBuffer)
{
	if (!HWBuffer)
		return;
	HWBufferMap.remove(HWBuffer->MeshBuffer);
	delete HWBuffer;
}


//! Remove hardware buffer
void CNullDriver::removeHardwareBuffer(const scene::IMeshBuffer* mb)
{
	core::map<const scene::IMeshBuffer*,SHWBufferLink*>::Node* node = HWBufferMap.find(mb);
	if (node)
		deleteHardwareBuffer(node->getValue());
}


//! Remove all hardware buffers
void CNullDriver::removeAllHardwareBuffers()
{
	while (HWBufferMap.size())
		deleteHardwareBuffer(HWBufferMap.getRoot()->getValue());
}


bool CNullDriver::isHardwareBufferRecommend(const scene::IMeshBuffer* mb)
{
	if (!mb || (mb->getHardwareMappingHint_Index()==scene::EHM_NEVER && mb->getHardwareMappingHint_Vertex()==scene::EHM_NEVER))
		return false;

	if (mb->getVertexCount()<MinVertexCountForVBO)
		return false;

	return true;
}


//! Create occlusion query.
/** Use node for identification and mesh for occlusion test. */
void CNullDriver::addOcclusionQuery(scene::ISceneNode* node, const scene::IMesh* mesh)
{
	if (!node)
		return;
	if (!mesh)
	{
		if ((node->getType() != scene::ESNT_MESH) && (node->getType() != scene::ESNT_ANIMATED_MESH))
			return;
		else if (node->getType() == scene::ESNT_MESH)
			mesh = static_cast<scene::IMeshSceneNode*>(node)->getMesh();
		else
			mesh = static_cast<scene::IAnimatedMeshSceneNode*>(node)->getMesh()->getMesh(0);
		if (!mesh)
			return;
	}

	//search for query
	s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index != -1)
	{
		if (OcclusionQueries[index].Mesh != mesh)
		{
			OcclusionQueries[index].Mesh->drop();
			OcclusionQueries[index].Mesh = mesh;
			mesh->grab();
		}
	}
	else
	{
		OcclusionQueries.push_back(SOccQuery(node, mesh));
		node->setAutomaticCulling(node->getAutomaticCulling() | scene::EAC_OCC_QUERY);
	}
}


//! Remove occlusion query.
void CNullDriver::removeOcclusionQuery(scene::ISceneNode* node)
{
	//search for query
	s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index != -1)
	{
		node->setAutomaticCulling(node->getAutomaticCulling() & ~scene::EAC_OCC_QUERY);
		OcclusionQueries.erase(index);
	}
}


//! Remove all occlusion queries.
void CNullDriver::removeAllOcclusionQueries()
{
	for (s32 i=OcclusionQueries.size()-1; i>=0; --i)
	{
		removeOcclusionQuery(OcclusionQueries[i].Node);
	}
}


//! Run occlusion query. Draws mesh stored in query.
/** If the mesh shall be rendered visible, use
flag to enable the proper material setting. */
void CNullDriver::runOcclusionQuery(scene::ISceneNode* node, bool visible)
{
	if(!node)
		return;
	s32 index = OcclusionQueries.linear_search(SOccQuery(node));
	if (index==-1)
		return;
	OcclusionQueries[index].Run=0;
	if (!visible)
	{
		SMaterial mat;
		mat.Lighting=false;
		mat.AntiAliasing=0;
		mat.ColorMask=ECP_NONE;
		mat.GouraudShading=false;
		mat.ZWriteEnable=false;
		setMaterial(mat);
	}
	setTransform(video::ETS_WORLD, node->getAbsoluteTransformation());
	const scene::IMesh* mesh = OcclusionQueries[index].Mesh;
	for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		if (visible)
			setMaterial(mesh->getMeshBuffer(i)->getMaterial());
		drawMeshBuffer(mesh->getMeshBuffer(i));
	}
}


//! Run all occlusion queries. Draws all meshes stored in queries.
/** If the meshes shall not be rendered visible, use
overrideMaterial to disable the color and depth buffer. */
void CNullDriver::runAllOcclusionQueries(bool visible)
{
	for (u32 i=0; i<OcclusionQueries.size(); ++i)
		runOcclusionQuery(OcclusionQueries[i].Node, visible);
}


//! Update occlusion query. Retrieves results from GPU.
/** If the query shall not block, set the flag to false.
Update might not occur in this case, though */
void CNullDriver::updateOcclusionQuery(scene::ISceneNode* node, bool block)
{
}


//! Update all occlusion queries. Retrieves results from GPU.
/** If the query shall not block, set the flag to false.
Update might not occur in this case, though */
void CNullDriver::updateAllOcclusionQueries(bool block)
{
	for (u32 i=0; i<OcclusionQueries.size(); ++i)
	{
		if (OcclusionQueries[i].Run==u32(~0))
			continue;
		updateOcclusionQuery(OcclusionQueries[i].Node, block);
		++OcclusionQueries[i].Run;
		if (OcclusionQueries[i].Run>1000)
			removeOcclusionQuery(OcclusionQueries[i].Node);
	}
}


//! Return query result.
/** Return value is the number of visible pixels/fragments.
The value is a safe approximation, i.e. can be larger then the
actual value of pixels. */
u32 CNullDriver::getOcclusionQueryResult(scene::ISceneNode* node) const
{
	return ~0;
}


//! Only used by the internal engine. Used to notify the driver that
//! the window was resized.
void CNullDriver::OnResize(const core::dimension2d<u32>& size)
{
	if (ViewPort.getWidth() == (s32)ScreenSize.Width &&
		ViewPort.getHeight() == (s32)ScreenSize.Height)
		ViewPort = core::rect<s32>(core::position2d<s32>(0,0),
									core::dimension2di(size));

	ScreenSize = size;
}


// adds a material renderer and drops it afterwards. To be used for internal creation
s32 CNullDriver::addAndDropMaterialRenderer(IMaterialRenderer* m)
{
	s32 i = addMaterialRenderer(m);

	if (m)
		m->drop();

	return i;
}


//! Adds a new material renderer to the video device.
s32 CNullDriver::addMaterialRenderer(IMaterialRenderer* renderer, const char* name)
{
	if (!renderer)
		return -1;

	SMaterialRenderer r;
	r.Renderer = renderer;
	r.Name = name;

	if (name == 0 && (MaterialRenderers.size() < (sizeof(sBuiltInMaterialTypeNames) / sizeof(char*))-1 ))
	{
		// set name of built in renderer so that we don't have to implement name
		// setting in all available renderers.
		r.Name = sBuiltInMaterialTypeNames[MaterialRenderers.size()];
	}

	MaterialRenderers.push_back(r);
	renderer->grab();

	return MaterialRenderers.size()-1;
}


//! Sets the name of a material renderer.
void CNullDriver::setMaterialRendererName(s32 idx, const char* name)
{
	if (idx < s32(sizeof(sBuiltInMaterialTypeNames) / sizeof(char*))-1 ||
		idx >= (s32)MaterialRenderers.size())
		return;

	MaterialRenderers[idx].Name = name;
}


//! Creates material attributes list from a material, usable for serialization and more.
io::IAttributes* CNullDriver::createAttributesFromMaterial(const video::SMaterial& material,
	io::SAttributeReadWriteOptions* options)
{
	io::CAttributes* attr = new io::CAttributes(this);

	attr->addEnum("Type", material.MaterialType, sBuiltInMaterialTypeNames);

	attr->addColor("Ambient", material.AmbientColor);
	attr->addColor("Diffuse", material.DiffuseColor);
	attr->addColor("Emissive", material.EmissiveColor);
	attr->addColor("Specular", material.SpecularColor);

	attr->addFloat("Shininess", material.Shininess);
	attr->addFloat("Param1", material.MaterialTypeParam);
	attr->addFloat("Param2", material.MaterialTypeParam2);

	core::stringc prefix="Texture";
	u32 i;
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
	{
		if (options && (options->Flags&io::EARWF_USE_RELATIVE_PATHS) && options->Filename && material.getTexture(i))
		{
			io::path path = FileSystem->getRelativeFilename(
				FileSystem->getAbsolutePath(material.getTexture(i)->getName()), options->Filename);
			attr->addTexture((prefix+core::stringc(i+1)).c_str(), material.getTexture(i), path);
		}
		else
			attr->addTexture((prefix+core::stringc(i+1)).c_str(), material.getTexture(i));
	}

	attr->addBool("Wireframe", material.Wireframe);
	attr->addBool("GouraudShading", material.GouraudShading);
	attr->addBool("Lighting", material.Lighting);
	attr->addBool("ZWriteEnable", material.ZWriteEnable);
	attr->addInt("ZBuffer", material.ZBuffer);
	attr->addBool("BackfaceCulling", material.BackfaceCulling);
	attr->addBool("FrontfaceCulling", material.FrontfaceCulling);
	attr->addBool("FogEnable", material.FogEnable);
	attr->addBool("NormalizeNormals", material.NormalizeNormals);
	attr->addBool("UseMipMaps", material.UseMipMaps);
	attr->addInt("AntiAliasing", material.AntiAliasing);
	attr->addInt("ColorMask", material.ColorMask);
	attr->addInt("ColorMaterial", material.ColorMaterial);
	attr->addInt("PolygonOffsetFactor", material.PolygonOffsetFactor);
	attr->addEnum("PolygonOffsetDirection", material.PolygonOffsetDirection, video::PolygonOffsetDirectionNames);

	prefix = "BilinearFilter";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		attr->addBool((prefix+core::stringc(i+1)).c_str(), material.TextureLayer[i].BilinearFilter);
	prefix = "TrilinearFilter";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		attr->addBool((prefix+core::stringc(i+1)).c_str(), material.TextureLayer[i].TrilinearFilter);
	prefix = "AnisotropicFilter";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		attr->addInt((prefix+core::stringc(i+1)).c_str(), material.TextureLayer[i].AnisotropicFilter);
	prefix="TextureWrapU";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		attr->addEnum((prefix+core::stringc(i+1)).c_str(), material.TextureLayer[i].TextureWrapU, aTextureClampNames);
	prefix="TextureWrapV";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		attr->addEnum((prefix+core::stringc(i+1)).c_str(), material.TextureLayer[i].TextureWrapV, aTextureClampNames);
	prefix="LODBias";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		attr->addInt((prefix+core::stringc(i+1)).c_str(), material.TextureLayer[i].LODBias);

	return attr;
}


//! Fills an SMaterial structure from attributes.
void CNullDriver::fillMaterialStructureFromAttributes(video::SMaterial& outMaterial, io::IAttributes* attr)
{
	outMaterial.MaterialType = video::EMT_SOLID;

	core::stringc name = attr->getAttributeAsString("Type");

	u32 i;

	for ( i=0; i < MaterialRenderers.size(); ++i)
		if ( name == MaterialRenderers[i].Name )
		{
			outMaterial.MaterialType = (video::E_MATERIAL_TYPE)i;
			break;
		}

	outMaterial.AmbientColor = attr->getAttributeAsColor("Ambient");
	outMaterial.DiffuseColor = attr->getAttributeAsColor("Diffuse");
	outMaterial.EmissiveColor = attr->getAttributeAsColor("Emissive");
	outMaterial.SpecularColor = attr->getAttributeAsColor("Specular");

	outMaterial.Shininess = attr->getAttributeAsFloat("Shininess");
	outMaterial.MaterialTypeParam = attr->getAttributeAsFloat("Param1");
	outMaterial.MaterialTypeParam2 = attr->getAttributeAsFloat("Param2");

	core::stringc prefix="Texture";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		outMaterial.setTexture(i, attr->getAttributeAsTexture((prefix+core::stringc(i+1)).c_str()));

	outMaterial.Wireframe = attr->getAttributeAsBool("Wireframe");
	outMaterial.GouraudShading = attr->getAttributeAsBool("GouraudShading");
	outMaterial.Lighting = attr->getAttributeAsBool("Lighting");
	outMaterial.ZWriteEnable = attr->getAttributeAsBool("ZWriteEnable");
	outMaterial.ZBuffer = (u8)attr->getAttributeAsInt("ZBuffer");
	outMaterial.BackfaceCulling = attr->getAttributeAsBool("BackfaceCulling");
	outMaterial.FrontfaceCulling = attr->getAttributeAsBool("FrontfaceCulling");
	outMaterial.FogEnable = attr->getAttributeAsBool("FogEnable");
	outMaterial.NormalizeNormals = attr->getAttributeAsBool("NormalizeNormals");
	if (attr->existsAttribute("UseMipMaps")) // legacy
		outMaterial.UseMipMaps = attr->getAttributeAsBool("UseMipMaps");
	else
		outMaterial.UseMipMaps = true;

	// default 0 is ok
	outMaterial.AntiAliasing = attr->getAttributeAsInt("AntiAliasing");
	if (attr->existsAttribute("ColorMask"))
		outMaterial.ColorMask = attr->getAttributeAsInt("ColorMask");
	if (attr->existsAttribute("ColorMaterial"))
		outMaterial.ColorMaterial = attr->getAttributeAsInt("ColorMaterial");
	if (attr->existsAttribute("PolygonOffsetFactor"))
		outMaterial.PolygonOffsetFactor = attr->getAttributeAsInt("PolygonOffsetFactor");
	if (attr->existsAttribute("PolygonOffsetDirection"))
		outMaterial.PolygonOffsetDirection = (video::E_POLYGON_OFFSET)attr->getAttributeAsEnumeration("PolygonOffsetDirection", video::PolygonOffsetDirectionNames);
	prefix = "BilinearFilter";
	if (attr->existsAttribute(prefix.c_str())) // legacy
		outMaterial.setFlag(EMF_BILINEAR_FILTER, attr->getAttributeAsBool(prefix.c_str()));
	else
		for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
			outMaterial.TextureLayer[i].BilinearFilter = attr->getAttributeAsBool((prefix+core::stringc(i+1)).c_str());

	prefix = "TrilinearFilter";
	if (attr->existsAttribute(prefix.c_str())) // legacy
		outMaterial.setFlag(EMF_TRILINEAR_FILTER, attr->getAttributeAsBool(prefix.c_str()));
	else
		for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
			outMaterial.TextureLayer[i].TrilinearFilter = attr->getAttributeAsBool((prefix+core::stringc(i+1)).c_str());

	prefix = "AnisotropicFilter";
	if (attr->existsAttribute(prefix.c_str())) // legacy
		outMaterial.setFlag(EMF_ANISOTROPIC_FILTER, attr->getAttributeAsBool(prefix.c_str()));
	else
		for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
			outMaterial.TextureLayer[i].AnisotropicFilter = attr->getAttributeAsInt((prefix+core::stringc(i+1)).c_str());

	prefix = "TextureWrap";
	if (attr->existsAttribute(prefix.c_str())) // legacy
	{
		for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		{
			outMaterial.TextureLayer[i].TextureWrapU = (E_TEXTURE_CLAMP)attr->getAttributeAsEnumeration((prefix+core::stringc(i+1)).c_str(), aTextureClampNames);
			outMaterial.TextureLayer[i].TextureWrapV = outMaterial.TextureLayer[i].TextureWrapU;
		}
	}
	else
	{
		for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		{
			outMaterial.TextureLayer[i].TextureWrapU = (E_TEXTURE_CLAMP)attr->getAttributeAsEnumeration((prefix+"U"+core::stringc(i+1)).c_str(), aTextureClampNames);
			outMaterial.TextureLayer[i].TextureWrapV = (E_TEXTURE_CLAMP)attr->getAttributeAsEnumeration((prefix+"V"+core::stringc(i+1)).c_str(), aTextureClampNames);
		}
	}

	// default 0 is ok
	prefix="LODBias";
	for (i=0; i<MATERIAL_MAX_TEXTURES; ++i)
		outMaterial.TextureLayer[i].LODBias = attr->getAttributeAsInt((prefix+core::stringc(i+1)).c_str());
}


//! Returns driver and operating system specific data about the IVideoDriver.
const SExposedVideoData& CNullDriver::getExposedVideoData()
{
	return ExposedData;
}


//! Returns type of video driver
E_DRIVER_TYPE CNullDriver::getDriverType() const
{
	return EDT_NULL;
}


//! deletes all material renderers
void CNullDriver::deleteMaterialRenders()
{
	// delete material renderers
	for (u32 i=0; i<MaterialRenderers.size(); ++i)
		if (MaterialRenderers[i].Renderer)
			MaterialRenderers[i].Renderer->drop();

	MaterialRenderers.clear();
}


//! Returns pointer to material renderer or null
IMaterialRenderer* CNullDriver::getMaterialRenderer(u32 idx)
{
	if ( idx < MaterialRenderers.size() )
		return MaterialRenderers[idx].Renderer;
	else
		return 0;
}


//! Returns amount of currently available material renderers.
u32 CNullDriver::getMaterialRendererCount() const
{
	return MaterialRenderers.size();
}


//! Returns name of the material renderer
const char* CNullDriver::getMaterialRendererName(u32 idx) const
{
	if ( idx < MaterialRenderers.size() )
		return MaterialRenderers[idx].Name.c_str();

	return 0;
}


//! Returns pointer to the IGPUProgrammingServices interface.
IGPUProgrammingServices* CNullDriver::getGPUProgrammingServices()
{
	return this;
}


//! Adds a new material renderer to the VideoDriver, based on a high level shading language.
s32 CNullDriver::addHighLevelShaderMaterial(
	const c8* vertexShaderProgram,
	const c8* vertexShaderEntryPointName,
	E_VERTEX_SHADER_TYPE vsCompileTarget,
	const c8* pixelShaderProgram,
	const c8* pixelShaderEntryPointName,
	E_PIXEL_SHADER_TYPE psCompileTarget,
	const c8* geometryShaderProgram,
	const c8* geometryShaderEntryPointName,
	E_GEOMETRY_SHADER_TYPE gsCompileTarget,
	scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType,
	u32 verticesOut,
	IShaderConstantSetCallBack* callback,
	E_MATERIAL_TYPE baseMaterial,
	s32 userData, E_GPU_SHADING_LANGUAGE shadingLang)
{
	os::Printer::log("High level shader materials not available (yet) in this driver, sorry");
	return -1;
}


//! Like IGPUProgrammingServices::addShaderMaterial() (look there for a detailed description),
//! but tries to load the programs from files.
s32 CNullDriver::addHighLevelShaderMaterialFromFiles(
		const io::path& vertexShaderProgramFileName,
		const c8* vertexShaderEntryPointName,
		E_VERTEX_SHADER_TYPE vsCompileTarget,
		const io::path& pixelShaderProgramFileName,
		const c8* pixelShaderEntryPointName,
		E_PIXEL_SHADER_TYPE psCompileTarget,
		const io::path& geometryShaderProgramFileName,
		const c8* geometryShaderEntryPointName,
		E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType,
		u32 verticesOut,
		IShaderConstantSetCallBack* callback,
		E_MATERIAL_TYPE baseMaterial,
		s32 userData, E_GPU_SHADING_LANGUAGE shadingLang)
{
	io::IReadFile* vsfile = 0;
	io::IReadFile* psfile = 0;
	io::IReadFile* gsfile = 0;

	if (vertexShaderProgramFileName.size() )
	{
		vsfile = FileSystem->createAndOpenFile(vertexShaderProgramFileName);
		if (!vsfile)
		{
			os::Printer::log("Could not open vertex shader program file",
				vertexShaderProgramFileName, ELL_WARNING);
		}
	}

	if (pixelShaderProgramFileName.size() )
	{
		psfile = FileSystem->createAndOpenFile(pixelShaderProgramFileName);
		if (!psfile)
		{
			os::Printer::log("Could not open pixel shader program file",
				pixelShaderProgramFileName, ELL_WARNING);
		}
	}

	if (geometryShaderProgramFileName.size() )
	{
		gsfile = FileSystem->createAndOpenFile(geometryShaderProgramFileName);
		if (!gsfile)
		{
			os::Printer::log("Could not open geometry shader program file",
				geometryShaderProgramFileName, ELL_WARNING);
		}
	}

	s32 result = addHighLevelShaderMaterialFromFiles(
		vsfile, vertexShaderEntryPointName, vsCompileTarget,
		psfile, pixelShaderEntryPointName, psCompileTarget,
		gsfile, geometryShaderEntryPointName, gsCompileTarget,
		inType, outType, verticesOut,
		callback, baseMaterial, userData, shadingLang);

	if (psfile)
		psfile->drop();

	if (vsfile)
		vsfile->drop();

	if (gsfile)
		gsfile->drop();

	return result;
}


//! Like IGPUProgrammingServices::addShaderMaterial() (look there for a detailed description),
//! but tries to load the programs from files.
s32 CNullDriver::addHighLevelShaderMaterialFromFiles(
		io::IReadFile* vertexShaderProgram,
		const c8* vertexShaderEntryPointName,
		E_VERTEX_SHADER_TYPE vsCompileTarget,
		io::IReadFile* pixelShaderProgram,
		const c8* pixelShaderEntryPointName,
		E_PIXEL_SHADER_TYPE psCompileTarget,
		io::IReadFile* geometryShaderProgram,
		const c8* geometryShaderEntryPointName,
		E_GEOMETRY_SHADER_TYPE gsCompileTarget,
		scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType,
		u32 verticesOut,
		IShaderConstantSetCallBack* callback,
		E_MATERIAL_TYPE baseMaterial,
		s32 userData, E_GPU_SHADING_LANGUAGE shadingLang)
{
	c8* vs = 0;
	c8* ps = 0;
	c8* gs = 0;

	if (vertexShaderProgram)
	{
		const long size = vertexShaderProgram->getSize();
		if (size)
		{
			vs = new c8[size+1];
			vertexShaderProgram->read(vs, size);
			vs[size] = 0;
		}
	}

	if (pixelShaderProgram)
	{
		const long size = pixelShaderProgram->getSize();
		if (size)
		{
			// if both handles are the same we must reset the file
			if (pixelShaderProgram==vertexShaderProgram)
				pixelShaderProgram->seek(0);
			ps = new c8[size+1];
			pixelShaderProgram->read(ps, size);
			ps[size] = 0;
		}
	}

	if (geometryShaderProgram)
	{
		const long size = geometryShaderProgram->getSize();
		if (size)
		{
			// if both handles are the same we must reset the file
			if ((geometryShaderProgram==vertexShaderProgram) ||
					(geometryShaderProgram==pixelShaderProgram))
				geometryShaderProgram->seek(0);
			gs = new c8[size+1];
			geometryShaderProgram->read(gs, size);
			gs[size] = 0;
		}
	}

	s32 result = this->addHighLevelShaderMaterial(
		vs, vertexShaderEntryPointName, vsCompileTarget,
		ps, pixelShaderEntryPointName, psCompileTarget,
		gs, geometryShaderEntryPointName, gsCompileTarget,
		inType, outType, verticesOut,
		callback, baseMaterial, userData, shadingLang);

	delete [] vs;
	delete [] ps;
	delete [] gs;

	return result;
}


//! Adds a new material renderer to the VideoDriver, using pixel and/or
//! vertex shaders to render geometry.
s32 CNullDriver::addShaderMaterial(const c8* vertexShaderProgram,
	const c8* pixelShaderProgram,
	IShaderConstantSetCallBack* callback,
	E_MATERIAL_TYPE baseMaterial,
	s32 userData)
{
	os::Printer::log("Shader materials not implemented yet in this driver, sorry.");
	return -1;
}


//! Like IGPUProgrammingServices::addShaderMaterial(), but tries to load the
//! programs from files.
s32 CNullDriver::addShaderMaterialFromFiles(io::IReadFile* vertexShaderProgram,
	io::IReadFile* pixelShaderProgram,
	IShaderConstantSetCallBack* callback,
	E_MATERIAL_TYPE baseMaterial,
	s32 userData)
{
	c8* vs = 0;
	c8* ps = 0;

	if (vertexShaderProgram)
	{
		const long size = vertexShaderProgram->getSize();
		if (size)
		{
			vs = new c8[size+1];
			vertexShaderProgram->read(vs, size);
			vs[size] = 0;
		}
	}

	if (pixelShaderProgram)
	{
		const long size = pixelShaderProgram->getSize();
		if (size)
		{
			ps = new c8[size+1];
			pixelShaderProgram->read(ps, size);
			ps[size] = 0;
		}
	}

	s32 result = addShaderMaterial(vs, ps, callback, baseMaterial, userData);

	delete [] vs;
	delete [] ps;

	return result;
}


//! Like IGPUProgrammingServices::addShaderMaterial(), but tries to load the
//! programs from files.
s32 CNullDriver::addShaderMaterialFromFiles(const io::path& vertexShaderProgramFileName,
	const io::path& pixelShaderProgramFileName,
	IShaderConstantSetCallBack* callback,
	E_MATERIAL_TYPE baseMaterial,
	s32 userData)
{
	io::IReadFile* vsfile = 0;
	io::IReadFile* psfile = 0;

	if (vertexShaderProgramFileName.size())
	{
		vsfile = FileSystem->createAndOpenFile(vertexShaderProgramFileName);
		if (!vsfile)
		{
			os::Printer::log("Could not open vertex shader program file",
				vertexShaderProgramFileName, ELL_WARNING);
			return -1;
		}
	}

	if (pixelShaderProgramFileName.size())
	{
		psfile = FileSystem->createAndOpenFile(pixelShaderProgramFileName);
		if (!psfile)
		{
			os::Printer::log("Could not open pixel shader program file",
				pixelShaderProgramFileName, ELL_WARNING);
			if (vsfile)
				vsfile->drop();
			return -1;
		}
	}

	s32 result = addShaderMaterialFromFiles(vsfile, psfile, callback,
		baseMaterial, userData);

	if (psfile)
		psfile->drop();

	if (vsfile)
		vsfile->drop();

	return result;
}


//! Creates a render target texture.
ITexture* CNullDriver::addRenderTargetTexture(const core::dimension2d<u32>& size,
		const io::path&name, const ECOLOR_FORMAT format)
{
	return 0;
}


//! Clears the ZBuffer.
void CNullDriver::clearZBuffer()
{
}


//! Returns a pointer to the mesh manipulator.
scene::IMeshManipulator* CNullDriver::getMeshManipulator()
{
	return MeshManipulator;
}


//! Returns an image created from the last rendered frame.
IImage* CNullDriver::createScreenShot(video::ECOLOR_FORMAT format, video::E_RENDER_TARGET target)
{
	return 0;
}


// prints renderer version
void CNullDriver::printVersion()
{
	core::stringw namePrint = L"Using renderer: ";
	namePrint += getName();
	os::Printer::log(namePrint.c_str(), ELL_INFORMATION);
}


//! creates a video driver
IVideoDriver* createNullDriver(io::IFileSystem* io, const core::dimension2d<u32>& screenSize)
{
	CNullDriver* nullDriver = new CNullDriver(io, screenSize);

	// create empty material renderers
	for(u32 i=0; sBuiltInMaterialTypeNames[i]; ++i)
	{
		IMaterialRenderer* imr = new IMaterialRenderer();
		nullDriver->addMaterialRenderer(imr);
		imr->drop();
	}

	return nullDriver;
}


//! Set/unset a clipping plane.
//! There are at least 6 clipping planes available for the user to set at will.
//! \param index: The plane index. Must be between 0 and MaxUserClipPlanes.
//! \param plane: The plane itself.
//! \param enable: If true, enable the clipping plane else disable it.
bool CNullDriver::setClipPlane(u32 index, const core::plane3df& plane, bool enable)
{
	return false;
}


//! Enable/disable a clipping plane.
void CNullDriver::enableClipPlane(u32 index, bool enable)
{
	// not necessary
}


ITexture* CNullDriver::createRenderTargetTexture(const core::dimension2d<u32>& size,
		const c8* name)
{
	os::Printer::log("createRenderTargetTexture is deprecated, use addRenderTargetTexture instead");
	ITexture* tex = addRenderTargetTexture(size, name);
	tex->grab();
	return tex;
}


void CNullDriver::setMinHardwareBufferVertexCount(u32 count)
{
	MinVertexCountForVBO = count;
}


SOverrideMaterial& CNullDriver::getOverrideMaterial()
{
	return OverrideMaterial;
}


//! Get the 2d override material for altering its values
SMaterial& CNullDriver::getMaterial2D()
{
	return OverrideMaterial2D;
}


//! Enable the 2d override material
void CNullDriver::enableMaterial2D(bool enable)
{
	OverrideMaterial2DEnabled=enable;
}


core::dimension2du CNullDriver::getMaxTextureSize() const
{
	return core::dimension2du(0x10000,0x10000); // maybe large enough
}


//! Color conversion convenience function
/** Convert an image (as array of pixels) from source to destination
array, thereby converting the color format. The pixel size is
determined by the color formats.
\param sP Pointer to source
\param sF Color format of source
\param sN Number of pixels to convert, both array must be large enough
\param dP Pointer to destination
\param dF Color format of destination
*/
void CNullDriver::convertColor(const void* sP, ECOLOR_FORMAT sF, s32 sN,
		void* dP, ECOLOR_FORMAT dF) const
{
	video::CColorConverter::convert_viaFormat(sP, sF, sN, dP, dF);
}


} // end namespace
} // end namespace
