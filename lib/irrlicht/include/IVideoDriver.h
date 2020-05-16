// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_I_VIDEO_DRIVER_H_INCLUDED__
#define __IRR_I_VIDEO_DRIVER_H_INCLUDED__

#include "rect.h"
#include "SColor.h"
#include "ITexture.h"
#include "irrArray.h"
#include "matrix4.h"
#include "plane3d.h"
#include "dimension2d.h"
#include "position2d.h"
#include "SMaterial.h"
#include "IMeshBuffer.h"
#include "triangle3d.h"
#include "EDriverTypes.h"
#include "EDriverFeatures.h"
#include "SExposedVideoData.h"

namespace irr
{
namespace io
{
	class IAttributes;
	struct SAttributeReadWriteOptions;
	class IReadFile;
	class IWriteFile;
} // end namespace io
namespace scene
{
	class IMeshBuffer;
	class IMesh;
	class IMeshManipulator;
	class ISceneNode;
} // end namespace scene

namespace video
{
	struct S3DVertex;
	struct S3DVertex2TCoords;
	struct S3DVertexTangents;
	struct SLight;
	class IImageLoader;
	class IImageWriter;
	class IMaterialRenderer;
	class IGPUProgrammingServices;

	//! enumeration for geometry transformation states
	enum E_TRANSFORMATION_STATE
	{
		//! View transformation
		ETS_VIEW = 0,
		//! World transformation
		ETS_WORLD,
		//! Projection transformation
		ETS_PROJECTION,
		//! Texture transformation
		ETS_TEXTURE_0,
		//! Texture transformation
		ETS_TEXTURE_1,
		//! Texture transformation
		ETS_TEXTURE_2,
		//! Texture transformation
		ETS_TEXTURE_3,
#if _IRR_MATERIAL_MAX_TEXTURES_>4
		//! Texture transformation
		ETS_TEXTURE_4,
#if _IRR_MATERIAL_MAX_TEXTURES_>5
		//! Texture transformation
		ETS_TEXTURE_5,
#if _IRR_MATERIAL_MAX_TEXTURES_>6
		//! Texture transformation
		ETS_TEXTURE_6,
#if _IRR_MATERIAL_MAX_TEXTURES_>7
		//! Texture transformation
		ETS_TEXTURE_7,
#endif
#endif
#endif
#endif
		//! Not used
		ETS_COUNT
	};

	//! enumeration for signaling resources which were lost after the last render cycle
	/** These values can be signaled by the driver, telling the app that some resources
	were lost and need to be recreated. Irrlicht will sometimes recreate the actual objects,
	but the content needs to be recreated by the application. */
	enum E_LOST_RESOURCE
	{
		//! The whole device/driver is lost
		ELR_DEVICE = 1,
		//! All texture are lost, rare problem
		ELR_TEXTURES = 2,
		//! The Render Target Textures are lost, typical problem for D3D
		ELR_RTTS = 4,
		//! The HW buffers are lost, will be recreated automatically, but might require some more time this frame
		ELR_HW_BUFFERS = 8
	};

	//! Special render targets, which usually map to dedicated hardware
	/** These render targets (besides 0 and 1) need not be supported by gfx cards */
	enum E_RENDER_TARGET
	{
		//! Render target is the main color frame buffer
		ERT_FRAME_BUFFER=0,
		//! Render target is a render texture
		ERT_RENDER_TEXTURE,
		//! Multi-Render target textures
		ERT_MULTI_RENDER_TEXTURES,
		//! Render target is the main color frame buffer
		ERT_STEREO_LEFT_BUFFER,
		//! Render target is the right color buffer (left is the main buffer)
		ERT_STEREO_RIGHT_BUFFER,
		//! Render to both stereo buffers at once
		ERT_STEREO_BOTH_BUFFERS,
		//! Auxiliary buffer 0
		ERT_AUX_BUFFER0,
		//! Auxiliary buffer 1
		ERT_AUX_BUFFER1,
		//! Auxiliary buffer 2
		ERT_AUX_BUFFER2,
		//! Auxiliary buffer 3
		ERT_AUX_BUFFER3,
		//! Auxiliary buffer 4
		ERT_AUX_BUFFER4
	};

	//! Enum for the types of fog distributions to choose from
	enum E_FOG_TYPE
	{
		EFT_FOG_EXP=0,
		EFT_FOG_LINEAR,
		EFT_FOG_EXP2
	};

	const c8* const FogTypeNames[] =
	{
		"FogExp",
		"FogLinear",
		"FogExp2",
		0
	};

	struct SOverrideMaterial
	{
		//! The Material values
		SMaterial Material;
		//! Which values are taken for override
		/** OR'ed values from E_MATERIAL_FLAGS. */
		u32 EnableFlags;
		//! Set in which render passes the material override is active.
		/** OR'ed values from E_SCENE_NODE_RENDER_PASS. */
		u16 EnablePasses;
		//! Global enable flag, overwritten by the SceneManager in each pass
		/** The Scenemanager uses the EnablePass array and sets Enabled to
		true if the Override material is enabled in the current pass. */
		bool Enabled;

		//! Default constructor
		SOverrideMaterial() : EnableFlags(0), EnablePasses(0), Enabled(false) {}

		//! Apply the enabled overrides
		void apply(SMaterial& material)
		{
			if (Enabled)
			{
				for (u32 i=0; i<32; ++i)
				{
					const u32 num=(1<<i);
					if (EnableFlags & num)
					{
						switch (num)
						{
						case EMF_WIREFRAME: material.Wireframe = Material.Wireframe; break;
						case EMF_POINTCLOUD: material.PointCloud = Material.PointCloud; break;
						case EMF_GOURAUD_SHADING: material.GouraudShading = Material.GouraudShading; break;
						case EMF_LIGHTING: material.Lighting = Material.Lighting; break;
						case EMF_ZBUFFER: material.ZBuffer = Material.ZBuffer; break;
						case EMF_ZWRITE_ENABLE: material.ZWriteEnable = Material.ZWriteEnable; break;
						case EMF_BACK_FACE_CULLING: material.BackfaceCulling = Material.BackfaceCulling; break;
						case EMF_FRONT_FACE_CULLING: material.FrontfaceCulling = Material.FrontfaceCulling; break;
						case EMF_BILINEAR_FILTER: material.TextureLayer[0].BilinearFilter = Material.TextureLayer[0].BilinearFilter; break;
						case EMF_TRILINEAR_FILTER: material.TextureLayer[0].TrilinearFilter = Material.TextureLayer[0].TrilinearFilter; break;
						case EMF_ANISOTROPIC_FILTER: material.TextureLayer[0].AnisotropicFilter = Material.TextureLayer[0].AnisotropicFilter; break;
						case EMF_FOG_ENABLE: material.FogEnable = Material.FogEnable; break;
						case EMF_NORMALIZE_NORMALS: material.NormalizeNormals = Material.NormalizeNormals; break;
						case EMF_TEXTURE_WRAP:
							material.TextureLayer[0].TextureWrapU = Material.TextureLayer[0].TextureWrapU;
							material.TextureLayer[0].TextureWrapV = Material.TextureLayer[0].TextureWrapV;
							break;
						case EMF_ANTI_ALIASING: material.AntiAliasing = Material.AntiAliasing; break;
						case EMF_COLOR_MASK: material.ColorMask = Material.ColorMask; break;
						case EMF_COLOR_MATERIAL: material.ColorMaterial = Material.ColorMaterial; break;
						case EMF_USE_MIP_MAPS: material.UseMipMaps = Material.UseMipMaps; break;
						case EMF_BLEND_OPERATION: material.BlendOperation = Material.BlendOperation; break;
						case EMF_POLYGON_OFFSET:
							material.PolygonOffsetDirection = Material.PolygonOffsetDirection;
							material.PolygonOffsetFactor = Material.PolygonOffsetFactor; break;
						}
					}
				}
			}
		}

	};

	struct IRenderTarget
	{
		IRenderTarget(ITexture* texture,
				E_COLOR_PLANE colorMask=ECP_ALL,
				E_BLEND_FACTOR blendFuncSrc=EBF_ONE,
				E_BLEND_FACTOR blendFuncDst=EBF_ONE_MINUS_SRC_ALPHA,
				E_BLEND_OPERATION blendOp=EBO_NONE) :
			RenderTexture(texture),
			TargetType(ERT_RENDER_TEXTURE), ColorMask(colorMask),
			BlendFuncSrc(blendFuncSrc), BlendFuncDst(blendFuncDst),
			BlendOp(blendOp) {}
		IRenderTarget(E_RENDER_TARGET target,
				E_COLOR_PLANE colorMask=ECP_ALL,
				E_BLEND_FACTOR blendFuncSrc=EBF_ONE,
				E_BLEND_FACTOR blendFuncDst=EBF_ONE_MINUS_SRC_ALPHA,
				E_BLEND_OPERATION blendOp=EBO_NONE) :
			RenderTexture(0),
			TargetType(target), ColorMask(colorMask),
			BlendFuncSrc(blendFuncSrc), BlendFuncDst(blendFuncDst),
			BlendOp(blendOp) {}
		bool operator!=(const IRenderTarget& other) const
		{
			return ((RenderTexture != other.RenderTexture) ||
				(TargetType != other.TargetType) ||
				(ColorMask != other.ColorMask) ||
				(BlendFuncSrc != other.BlendFuncSrc) ||
				(BlendFuncDst != other.BlendFuncDst) ||
				(BlendOp != other.BlendOp));
		}
		ITexture* RenderTexture;
		E_RENDER_TARGET TargetType:8;
		E_COLOR_PLANE ColorMask:8;
		E_BLEND_FACTOR BlendFuncSrc:4;
		E_BLEND_FACTOR BlendFuncDst:4;
		E_BLEND_OPERATION BlendOp:4;
	};

	//! Interface to driver which is able to perform 2d and 3d graphics functions.
	/** This interface is one of the most important interfaces of
	the Irrlicht Engine: All rendering and texture manipulation is done with
	this interface. You are able to use the Irrlicht Engine by only
	invoking methods of this interface if you like to, although the
	irr::scene::ISceneManager interface provides a lot of powerful classes
	and methods to make the programmer's life easier.
	*/
	class IVideoDriver : public virtual IReferenceCounted
	{
	public:

		//! Applications must call this method before performing any rendering.
		/** This method can clear the back- and the z-buffer.
		\param backBuffer Specifies if the back buffer should be
		cleared, which means that the screen is filled with the color
		specified. If this parameter is false, the back buffer will
		not be cleared and the color parameter is ignored.
		\param zBuffer Specifies if the depth buffer (z buffer) should
		be cleared. It is not nesesarry to do so if only 2d drawing is
		used.
		\param color The color used for back buffer clearing
		\param videoData Handle of another window, if you want the
		bitmap to be displayed on another window. If this is an empty
		element, everything will be displayed in the default window.
		Note: This feature is not fully implemented for all devices.
		\param sourceRect Pointer to a rectangle defining the source
		rectangle of the area to be presented. Set to null to present
		everything. Note: not implemented in all devices.
		\return False if failed. */
		virtual bool beginScene(bool backBuffer=true, bool zBuffer=true,
				SColor color=SColor(255,0,0,0),
				const SExposedVideoData& videoData=SExposedVideoData(),
				core::rect<s32>* sourceRect=0) =0;

		//! Presents the rendered image to the screen.
		/** Applications must call this method after performing any
		rendering.
		\return False if failed and true if succeeded. */
		virtual bool endScene() =0;

		//! Queries the features of the driver.
		/** Returns true if a feature is available
		\param feature Feature to query.
		\return True if the feature is available, false if not. */
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const =0;

		//! Disable a feature of the driver.
		/** Can also be used to enable the features again. It is not
		possible to enable unsupported features this way, though.
		\param feature Feature to disable.
		\param flag When true the feature is disabled, otherwise it is enabled. */
		virtual void disableFeature(E_VIDEO_DRIVER_FEATURE feature, bool flag=true) =0;

		//! Get attributes of the actual video driver
		/** The following names can be queried for the given types:
		MaxTextures (int) The maximum number of simultaneous textures supported by the driver. This can be less than the supported number of textures of the driver. Use _IRR_MATERIAL_MAX_TEXTURES_ to adapt the number.
		MaxSupportedTextures (int) The maximum number of simultaneous textures supported by the fixed function pipeline of the (hw) driver. The actual supported number of textures supported by the engine can be lower.
		MaxLights (int) Number of hardware lights supported in the fixed function pipieline of the driver, typically 6-8. Use light manager or deferred shading for more.
		MaxAnisotropy (int) Number of anisotropy levels supported for filtering. At least 1, max is typically at 16 or 32.
		MaxUserClipPlanes (int) Number of additional clip planes, which can be set by the user via dedicated driver methods.
		MaxAuxBuffers (int) Special render buffers, which are currently not really usable inside Irrlicht. Only supported by OpenGL
		MaxMultipleRenderTargets (int) Number of render targets which can be bound simultaneously. Rendering to MRTs is done via shaders.
		MaxIndices (int) Number of indices which can be used in one render call (i.e. one mesh buffer).
		MaxTextureSize (int) Dimension that a texture may have, both in width and height.
		MaxGeometryVerticesOut (int) Number of vertices the geometry shader can output in one pass. Only OpenGL so far.
		MaxTextureLODBias (float) Maximum value for LOD bias. Is usually at around 16, but can be lower on some systems.
		Version (int) Version of the driver. Should be Major*100+Minor
		ShaderLanguageVersion (int) Version of the high level shader language. Should be Major*100+Minor.
		AntiAlias (int) Number of Samples the driver uses for each pixel. 0 and 1 means anti aliasing is off, typical values are 2,4,8,16,32
		*/
		virtual const io::IAttributes& getDriverAttributes() const=0;

		//! Check if the driver was recently reset.
		/** For d3d devices you will need to recreate the RTTs if the
		driver was reset. Should be queried right after beginScene().
		*/
		virtual bool checkDriverReset() =0;

		//! Sets transformation matrices.
		/** \param state Transformation type to be set, e.g. view,
		world, or projection.
		\param mat Matrix describing the transformation. */
		virtual void setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat) =0;

		//! Returns the transformation set by setTransform
		/** \param state Transformation type to query
		\return Matrix describing the transformation. */
		virtual const core::matrix4& getTransform(E_TRANSFORMATION_STATE state) const =0;

		//! Retrieve the number of image loaders
		/** \return Number of image loaders */
		virtual u32 getImageLoaderCount() const = 0;

		//! Retrieve the given image loader
		/** \param n The index of the loader to retrieve. This parameter is an 0-based
		array index.
		\return A pointer to the specified loader, 0 if the index is incorrect. */
		virtual IImageLoader* getImageLoader(u32 n) = 0;

		//! Retrieve the number of image writers
		/** \return Number of image writers */
		virtual u32 getImageWriterCount() const = 0;

		//! Retrieve the given image writer
		/** \param n The index of the writer to retrieve. This parameter is an 0-based
		array index.
		\return A pointer to the specified writer, 0 if the index is incorrect. */
		virtual IImageWriter* getImageWriter(u32 n) = 0;

		//! Sets a material.
		/** All 3d drawing functions will draw geometry using this material thereafter.
		\param material: Material to be used from now on. */
		virtual void setMaterial(const SMaterial& material) =0;

		//! Get access to a named texture.
		/** Loads the texture from disk if it is not
		already loaded and generates mipmap levels if desired.
		Texture loading can be influenced using the
		setTextureCreationFlag() method. The texture can be in several
		imageformats, such as BMP, JPG, TGA, PCX, PNG, and PSD.
		\param filename Filename of the texture to be loaded.
		\return Pointer to the texture, or 0 if the texture
		could not be loaded. This pointer should not be dropped. See
		IReferenceCounted::drop() for more information. */
		virtual ITexture* getTexture(const io::path& filename) = 0;

		//! Get access to a named texture.
		/** Loads the texture from disk if it is not
		already loaded and generates mipmap levels if desired.
		Texture loading can be influenced using the
		setTextureCreationFlag() method. The texture can be in several
		imageformats, such as BMP, JPG, TGA, PCX, PNG, and PSD.
		\param file Pointer to an already opened file.
		\return Pointer to the texture, or 0 if the texture
		could not be loaded. This pointer should not be dropped. See
		IReferenceCounted::drop() for more information. */
		virtual ITexture* getTexture(io::IReadFile* file) =0;

		//! Returns a texture by index
		/** \param index: Index of the texture, must be smaller than
		getTextureCount() Please note that this index might change when
		adding or removing textures
		\return Pointer to the texture, or 0 if the texture was not
		set or index is out of bounds. This pointer should not be
		dropped. See IReferenceCounted::drop() for more information. */
		virtual ITexture* getTextureByIndex(u32 index) =0;

		//! Returns amount of textures currently loaded
		/** \return Amount of textures currently loaded */
		virtual u32 getTextureCount() const = 0;

		//! Renames a texture
		/** \param texture Pointer to the texture to rename.
		\param newName New name for the texture. This should be a unique name. */
		virtual void renameTexture(ITexture* texture, const io::path& newName) = 0;

		//! Creates an empty texture of specified size.
		/** \param size: Size of the texture.
		\param name A name for the texture. Later calls to
		getTexture() with this name will return this texture
		\param format Desired color format of the texture. Please note
		that the driver may choose to create the texture in another
		color format.
		\return Pointer to the newly created texture. This pointer
		should not be dropped. See IReferenceCounted::drop() for more
		information. */
		virtual ITexture* addTexture(const core::dimension2d<u32>& size,
			const io::path& name, ECOLOR_FORMAT format = ECF_A8R8G8B8) = 0;

		//! Creates a texture from an IImage.
		/** \param name A name for the texture. Later calls of
		getTexture() with this name will return this texture
		\param image Image the texture is created from.
		\param mipmapData Optional pointer to a set of images which
		build up the whole mipmap set. Must be images of the same color
		type as image. If this parameter is not given, the mipmaps are
		derived from image.
		\return Pointer to the newly created texture. This pointer
		should not be dropped. See IReferenceCounted::drop() for more
		information. */
		virtual ITexture* addTexture(const io::path& name, IImage* image, void* mipmapData=0) = 0;

		//! Adds a new render target texture to the texture cache.
		/** \param size Size of the texture, in pixels. Width and
		height should be a power of two (e.g. 64, 128, 256, 512, ...)
		and it should not be bigger than the backbuffer, because it
		shares the zbuffer with the screen buffer.
		\param name An optional name for the RTT.
		\param format The color format of the render target. Floating point formats are supported.
		\return Pointer to the created texture or 0 if the texture
		could not be created. This pointer should not be dropped. See
		IReferenceCounted::drop() for more information. */
		virtual ITexture* addRenderTargetTexture(const core::dimension2d<u32>& size,
				const io::path& name = "rt", const ECOLOR_FORMAT format = ECF_UNKNOWN) =0;

		//! Removes a texture from the texture cache and deletes it.
		/** This method can free a lot of memory!
		Please note that after calling this, the pointer to the
		ITexture may no longer be valid, if it was not grabbed before
		by other parts of the engine for storing it longer. So it is a
		good idea to set all materials which are using this texture to
		0 or another texture first.
		\param texture Texture to delete from the engine cache. */
		virtual void removeTexture(ITexture* texture) =0;

		//! Removes all textures from the texture cache and deletes them.
		/** This method can free a lot of memory!
		Please note that after calling this, the pointer to the
		ITexture may no longer be valid, if it was not grabbed before
		by other parts of the engine for storing it longer. So it is a
		good idea to set all materials which are using this texture to
		0 or another texture first. */
		virtual void removeAllTextures() =0;

		//! Remove hardware buffer
		virtual void removeHardwareBuffer(const scene::IMeshBuffer* mb) =0;

		//! Remove all hardware buffers
		virtual void removeAllHardwareBuffers() =0;

		//! Create occlusion query.
		/** Use node for identification and mesh for occlusion test. */
		virtual void addOcclusionQuery(scene::ISceneNode* node,
				const scene::IMesh* mesh=0) =0;

		//! Remove occlusion query.
		virtual void removeOcclusionQuery(scene::ISceneNode* node) =0;

		//! Remove all occlusion queries.
		virtual void removeAllOcclusionQueries() =0;

		//! Run occlusion query. Draws mesh stored in query.
		/** If the mesh shall not be rendered visible, use
		overrideMaterial to disable the color and depth buffer. */
		virtual void runOcclusionQuery(scene::ISceneNode* node, bool visible=false) =0;

		//! Run all occlusion queries. Draws all meshes stored in queries.
		/** If the meshes shall not be rendered visible, use
		overrideMaterial to disable the color and depth buffer. */
		virtual void runAllOcclusionQueries(bool visible=false) =0;

		//! Update occlusion query. Retrieves results from GPU.
		/** If the query shall not block, set the flag to false.
		Update might not occur in this case, though */
		virtual void updateOcclusionQuery(scene::ISceneNode* node, bool block=true) =0;

		//! Update all occlusion queries. Retrieves results from GPU.
		/** If the query shall not block, set the flag to false.
		Update might not occur in this case, though */
		virtual void updateAllOcclusionQueries(bool block=true) =0;

		//! Return query result.
		/** Return value is the number of visible pixels/fragments.
		The value is a safe approximation, i.e. can be larger than the
		actual value of pixels. */
		virtual u32 getOcclusionQueryResult(scene::ISceneNode* node) const =0;

		//! Sets a boolean alpha channel on the texture based on a color key.
		/** This makes the texture fully transparent at the texels where
		this color key can be found when using for example draw2DImage
		with useAlphachannel==true.  The alpha of other texels is not modified.
		\param texture Texture whose alpha channel is modified.
		\param color Color key color. Every texel with this color will
		become fully transparent as described above. Please note that the
		colors of a texture may be converted when loading it, so the
		color values may not be exactly the same in the engine and for
		example in picture edit programs. To avoid this problem, you
		could use the makeColorKeyTexture method, which takes the
		position of a pixel instead a color value.
		\param zeroTexels \deprecated If set to true, then any texels that match
		the color key will have their color, as well as their alpha, set to zero
		(i.e. black). This behavior matches the legacy (buggy) behavior prior
		to release 1.5 and is provided for backwards compatibility only.
		This parameter may be removed by Irrlicht 1.9. */
		virtual void makeColorKeyTexture(video::ITexture* texture,
						video::SColor color,
						bool zeroTexels = false) const =0;

		//! Sets a boolean alpha channel on the texture based on the color at a position.
		/** This makes the texture fully transparent at the texels where
		the color key can be found when using for example draw2DImage
		with useAlphachannel==true.  The alpha of other texels is not modified.
		\param texture Texture whose alpha channel is modified.
		\param colorKeyPixelPos Position of a pixel with the color key
		color. Every texel with this color will become fully transparent as
		described above.
		\param zeroTexels \deprecated If set to true, then any texels that match
		the color key will have their color, as well as their alpha, set to zero
		(i.e. black). This behavior matches the legacy (buggy) behavior prior
		to release 1.5 and is provided for backwards compatibility only.
		This parameter may be removed by Irrlicht 1.9. */
		virtual void makeColorKeyTexture(video::ITexture* texture,
				core::position2d<s32> colorKeyPixelPos,
				bool zeroTexels = false) const =0;

		//! Creates a normal map from a height map texture.
		/** If the target texture has 32 bit, the height value is
		stored in the alpha component of the texture as addition. This
		value is used by the video::EMT_PARALLAX_MAP_SOLID material and
		similar materials.
		\param texture Texture whose alpha channel is modified.
		\param amplitude Constant value by which the height
		information is multiplied.*/
		virtual void makeNormalMapTexture(video::ITexture* texture, f32 amplitude=1.0f) const =0;

		//! Sets a new render target.
		/** This will only work if the driver supports the
		EVDF_RENDER_TO_TARGET feature, which can be queried with
		queryFeature(). Usually, rendering to textures is done in this
		way:
		\code
		// create render target
		ITexture* target = driver->addRenderTargetTexture(core::dimension2d<u32>(128,128), "rtt1");

		// ...

		driver->setRenderTarget(target); // set render target
		// .. draw stuff here
		driver->setRenderTarget(0); // set previous render target
		\endcode
		Please note that you cannot render 3D or 2D geometry with a
		render target as texture on it when you are rendering the scene
		into this render target at the same time. It is usually only
		possible to render into a texture between the
		IVideoDriver::beginScene() and endScene() method calls.
		\param texture New render target. Must be a texture created with
		IVideoDriver::addRenderTargetTexture(). If set to 0, it sets
		the previous render target which was set before the last
		setRenderTarget() call.
		\param clearBackBuffer Clears the backbuffer of the render
		target with the color parameter
		\param clearZBuffer Clears the zBuffer of the rendertarget.
		Note that because the frame buffer may share the zbuffer with
		the rendertarget, its zbuffer might be partially cleared too
		by this.
		\param color The background color for the render target.
		\return True if sucessful and false if not. */
		virtual bool setRenderTarget(video::ITexture* texture,
			bool clearBackBuffer=true, bool clearZBuffer=true,
			SColor color=video::SColor(0,0,0,0)) =0;

		//! set or reset special render targets
		/** This method enables access to special color buffers such as
		stereoscopic buffers or auxiliary buffers.
		\param target Enum value for the render target
		\param clearTarget Clears the target buffer with the color
		parameter
		\param clearZBuffer Clears the zBuffer of the rendertarget.
		Note that because the main frame buffer may share the zbuffer with
		the rendertarget, its zbuffer might be partially cleared too
		by this.
		\param color The background color for the render target.
		\return True if sucessful and false if not. */
		virtual bool setRenderTarget(E_RENDER_TARGET target, bool clearTarget=true,
					bool clearZBuffer=true,
					SColor color=video::SColor(0,0,0,0)) =0;

		//! Sets new multiple render targets.
		virtual bool setRenderTarget(const core::array<video::IRenderTarget>& texture,
			bool clearBackBuffer=true, bool clearZBuffer=true,
			SColor color=video::SColor(0,0,0,0)) =0;

		//! Sets a new viewport.
		/** Every rendering operation is done into this new area.
		\param area: Rectangle defining the new area of rendering
		operations. */
		virtual void setViewPort(const core::rect<s32>& area) =0;

		//! Gets the area of the current viewport.
		/** \return Rectangle of the current viewport. */
		virtual const core::rect<s32>& getViewPort() const =0;

		//! Draws a vertex primitive list
		/** Note that, depending on the index type, some vertices might be not
		accessible through the index list. The limit is at 65535 vertices for 16bit
		indices. Please note that currently not all primitives are available for
		all drivers, and some might be emulated via triangle renders.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices. These define the vertices used
		for each primitive. Depending on the pType, indices are interpreted as single
		objects (for point like primitives), pairs (for lines), triplets (for
		triangles), or quads.
		\param primCount Amount of Primitives
		\param vType Vertex type, e.g. video::EVT_STANDARD for S3DVertex.
		\param pType Primitive type, e.g. scene::EPT_TRIANGLE_FAN for a triangle fan.
		\param iType Index type, e.g. video::EIT_16BIT for 16bit indices. */
		virtual void drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primCount,
				E_VERTEX_TYPE vType=EVT_STANDARD,
				scene::E_PRIMITIVE_TYPE pType=scene::EPT_TRIANGLES,
				E_INDEX_TYPE iType=EIT_16BIT) =0;

		//! Draws a vertex primitive list in 2d
		/** Compared to the general (3d) version of this method, this
		one sets up a 2d render mode, and uses only x and y of vectors.
		Note that, depending on the index type, some vertices might be
		not accessible through the index list. The limit is at 65535
		vertices for 16bit indices. Please note that currently not all
		primitives are available for all drivers, and some might be
		emulated via triangle renders. This function is not available
		for the sw drivers.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices. These define the
		vertices used for each primitive. Depending on the pType,
		indices are interpreted as single objects (for point like
		primitives), pairs (for lines), triplets (for triangles), or
		quads.
		\param primCount Amount of Primitives
		\param vType Vertex type, e.g. video::EVT_STANDARD for S3DVertex.
		\param pType Primitive type, e.g. scene::EPT_TRIANGLE_FAN for a triangle fan.
		\param iType Index type, e.g. video::EIT_16BIT for 16bit indices. */
		virtual void draw2DVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primCount,
				E_VERTEX_TYPE vType=EVT_STANDARD,
				scene::E_PRIMITIVE_TYPE pType=scene::EPT_TRIANGLES,
				E_INDEX_TYPE iType=EIT_16BIT) =0;

		//! Draws an indexed triangle list.
		/** Note that there may be at maximum 65536 vertices, because
		the index list is an array of 16 bit values each with a maximum
		value of 65536. If there are more than 65536 vertices in the
		list, results of this operation are not defined.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices.
		\param triangleCount Amount of Triangles. Usually amount of indices / 3. */
		void drawIndexedTriangleList(const S3DVertex* vertices,
			u32 vertexCount, const u16* indexList, u32 triangleCount)
		{
			drawVertexPrimitiveList(vertices, vertexCount, indexList, triangleCount, EVT_STANDARD, scene::EPT_TRIANGLES, EIT_16BIT);
		}

		//! Draws an indexed triangle list.
		/** Note that there may be at maximum 65536 vertices, because
		the index list is an array of 16 bit values each with a maximum
		value of 65536. If there are more than 65536 vertices in the
		list, results of this operation are not defined.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices.
		\param triangleCount Amount of Triangles. Usually amount of indices / 3. */
		void drawIndexedTriangleList(const S3DVertex2TCoords* vertices,
			u32 vertexCount, const u16* indexList, u32 triangleCount)
		{
			drawVertexPrimitiveList(vertices, vertexCount, indexList, triangleCount, EVT_2TCOORDS, scene::EPT_TRIANGLES, EIT_16BIT);
		}

		//! Draws an indexed triangle list.
		/** Note that there may be at maximum 65536 vertices, because
		the index list is an array of 16 bit values each with a maximum
		value of 65536. If there are more than 65536 vertices in the
		list, results of this operation are not defined.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices.
		\param triangleCount Amount of Triangles. Usually amount of indices / 3. */
		void drawIndexedTriangleList(const S3DVertexTangents* vertices,
			u32 vertexCount, const u16* indexList, u32 triangleCount)
		{
			drawVertexPrimitiveList(vertices, vertexCount, indexList, triangleCount, EVT_TANGENTS, scene::EPT_TRIANGLES, EIT_16BIT);
		}

		//! Draws an indexed triangle fan.
		/** Note that there may be at maximum 65536 vertices, because
		the index list is an array of 16 bit values each with a maximum
		value of 65536. If there are more than 65536 vertices in the
		list, results of this operation are not defined.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices.
		\param triangleCount Amount of Triangles. Usually amount of indices - 2. */
		void drawIndexedTriangleFan(const S3DVertex* vertices,
			u32 vertexCount, const u16* indexList, u32 triangleCount)
		{
			drawVertexPrimitiveList(vertices, vertexCount, indexList, triangleCount, EVT_STANDARD, scene::EPT_TRIANGLE_FAN, EIT_16BIT);
		}

		//! Draws an indexed triangle fan.
		/** Note that there may be at maximum 65536 vertices, because
		the index list is an array of 16 bit values each with a maximum
		value of 65536. If there are more than 65536 vertices in the
		list, results of this operation are not defined.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices.
		\param triangleCount Amount of Triangles. Usually amount of indices - 2. */
		void drawIndexedTriangleFan(const S3DVertex2TCoords* vertices,
			u32 vertexCount, const u16* indexList, u32 triangleCount)
		{
			drawVertexPrimitiveList(vertices, vertexCount, indexList, triangleCount, EVT_2TCOORDS, scene::EPT_TRIANGLE_FAN, EIT_16BIT);
		}

		//! Draws an indexed triangle fan.
		/** Note that there may be at maximum 65536 vertices, because
		the index list is an array of 16 bit values each with a maximum
		value of 65536. If there are more than 65536 vertices in the
		list, results of this operation are not defined.
		\param vertices Pointer to array of vertices.
		\param vertexCount Amount of vertices in the array.
		\param indexList Pointer to array of indices.
		\param triangleCount Amount of Triangles. Usually amount of indices - 2. */
		void drawIndexedTriangleFan(const S3DVertexTangents* vertices,
			u32 vertexCount, const u16* indexList, u32 triangleCount)
		{
			drawVertexPrimitiveList(vertices, vertexCount, indexList, triangleCount, EVT_TANGENTS, scene::EPT_TRIANGLE_FAN, EIT_16BIT);
		}

		//! Draws a 3d line.
		/** For some implementations, this method simply calls
		drawVertexPrimitiveList for some triangles.
		Note that the line is drawn using the current transformation
		matrix and material. So if you need to draw the 3D line
		independently of the current transformation, use
		\code
		driver->setMaterial(someMaterial);
		driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
		\endcode
		for some properly set up material before drawing the line.
		Some drivers support line thickness set in the material.
		\param start Start of the 3d line.
		\param end End of the 3d line.
		\param color Color of the line. */
		virtual void draw3DLine(const core::vector3df& start,
			const core::vector3df& end, SColor color = SColor(255,255,255,255)) =0;

		//! Draws a 3d triangle.
		/** This method calls drawVertexPrimitiveList for some triangles.
		This method works with all drivers because it simply calls
		drawVertexPrimitiveList, but it is hence not very fast.
		Note that the triangle is drawn using the current
		transformation matrix and material. So if you need to draw it
		independently of the current transformation, use
		\code
		driver->setMaterial(someMaterial);
		driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
		\endcode
		for some properly set up material before drawing the triangle.
		\param triangle The triangle to draw.
		\param color Color of the line. */
		virtual void draw3DTriangle(const core::triangle3df& triangle,
			SColor color = SColor(255,255,255,255)) =0;

		//! Draws a 3d axis aligned box.
		/** This method simply calls draw3DLine for the edges of the
		box. Note that the box is drawn using the current transformation
		matrix and material. So if you need to draw it independently of
		the current transformation, use
		\code
		driver->setMaterial(someMaterial);
		driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
		\endcode
		for some properly set up material before drawing the box.
		\param box The axis aligned box to draw
		\param color Color to use while drawing the box. */
		virtual void draw3DBox(const core::aabbox3d<f32>& box,
			SColor color = SColor(255,255,255,255)) =0;

		//! Draws a 2d image without any special effects
		/** \param texture Pointer to texture to use.
		\param destPos Upper left 2d destination position where the
		image will be drawn. */
		virtual void draw2DImage(const video::ITexture* texture,
			const core::position2d<s32>& destPos) =0;

		//! Draws a 2d image using a color
		/** (if color is other than
		Color(255,255,255,255)) and the alpha channel of the texture.
		\param texture Texture to be drawn.
		\param destPos Upper left 2d destination position where the
		image will be drawn.
		\param sourceRect Source rectangle in the image.
		\param clipRect Pointer to rectangle on the screen where the
		image is clipped to.
		If this pointer is NULL the image is not clipped.
		\param color Color with which the image is drawn. If the color
		equals Color(255,255,255,255) it is ignored. Note that the
		alpha component is used: If alpha is other than 255, the image
		will be transparent.
		\param useAlphaChannelOfTexture: If true, the alpha channel of
		the texture is used to draw the image.*/
		virtual void draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect =0,
			SColor color=SColor(255,255,255,255), bool useAlphaChannelOfTexture=false) =0;

		//! Draws a set of 2d images, using a color and the alpha channel of the texture.
		/** The images are drawn beginning at pos and concatenated in
		one line. All drawings are clipped against clipRect (if != 0).
		The subtextures are defined by the array of sourceRects and are
		chosen by the indices given.
		\param texture Texture to be drawn.
		\param pos Upper left 2d destination position where the image
		will be drawn.
		\param sourceRects Source rectangles of the image.
		\param indices List of indices which choose the actual
		rectangle used each time.
		\param kerningWidth Offset to Position on X
		\param clipRect Pointer to rectangle on the screen where the
		image is clipped to.
		If this pointer is 0 then the image is not clipped.
		\param color Color with which the image is drawn.
		Note that the alpha component is used. If alpha is other than
		255, the image will be transparent.
		\param useAlphaChannelOfTexture: If true, the alpha channel of
		the texture is used to draw the image. */
		virtual void draw2DImageBatch(const video::ITexture* texture,
				const core::position2d<s32>& pos,
				const core::array<core::rect<s32> >& sourceRects,
				const core::array<s32>& indices,
				s32 kerningWidth=0,
				const core::rect<s32>* clipRect=0,
				SColor color=SColor(255,255,255,255),
				bool useAlphaChannelOfTexture=false) =0;

		//! Draws a set of 2d images, using a color and the alpha channel of the texture.
		/** All drawings are clipped against clipRect (if != 0).
		The subtextures are defined by the array of sourceRects and are
		positioned using the array of positions.
		\param texture Texture to be drawn.
		\param positions Array of upper left 2d destinations where the
		images will be drawn.
		\param sourceRects Source rectangles of the image.
		\param clipRect Pointer to rectangle on the screen where the
		images are clipped to.
		If this pointer is 0 then the image is not clipped.
		\param color Color with which the image is drawn.
		Note that the alpha component is used. If alpha is other than
		255, the image will be transparent.
		\param useAlphaChannelOfTexture: If true, the alpha channel of
		the texture is used to draw the image. */
		virtual void draw2DImageBatch(const video::ITexture* texture,
				const core::array<core::position2d<s32> >& positions,
				const core::array<core::rect<s32> >& sourceRects,
				const core::rect<s32>* clipRect=0,
				SColor color=SColor(255,255,255,255),
				bool useAlphaChannelOfTexture=false) =0;

		//! Draws a part of the texture into the rectangle. Note that colors must be an array of 4 colors if used.
		/** Suggested and first implemented by zola.
		\param texture The texture to draw from
		\param destRect The rectangle to draw into
		\param sourceRect The rectangle denoting a part of the texture
		\param clipRect Clips the destination rectangle (may be 0)
		\param colors Array of 4 colors denoting the color values of
		the corners of the destRect
		\param useAlphaChannelOfTexture True if alpha channel will be
		blended. */
		virtual void draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect =0,
			const video::SColor * const colors=0, bool useAlphaChannelOfTexture=false) =0;

		//! Draws a 2d rectangle.
		/** \param color Color of the rectangle to draw. The alpha
		component will not be ignored and specifies how transparent the
		rectangle will be.
		\param pos Position of the rectangle.
		\param clip Pointer to rectangle against which the rectangle
		will be clipped. If the pointer is null, no clipping will be
		performed. */
		virtual void draw2DRectangle(SColor color, const core::rect<s32>& pos,
			const core::rect<s32>* clip =0) =0;

		//! Draws a 2d rectangle with a gradient.
		/** \param colorLeftUp Color of the upper left corner to draw.
		The alpha component will not be ignored and specifies how
		transparent the rectangle will be.
		\param colorRightUp Color of the upper right corner to draw.
		The alpha component will not be ignored and specifies how
		transparent the rectangle will be.
		\param colorLeftDown Color of the lower left corner to draw.
		The alpha component will not be ignored and specifies how
		transparent the rectangle will be.
		\param colorRightDown Color of the lower right corner to draw.
		The alpha component will not be ignored and specifies how
		transparent the rectangle will be.
		\param pos Position of the rectangle.
		\param clip Pointer to rectangle against which the rectangle
		will be clipped. If the pointer is null, no clipping will be
		performed. */
		virtual void draw2DRectangle(const core::rect<s32>& pos,
				SColor colorLeftUp, SColor colorRightUp,
				SColor colorLeftDown, SColor colorRightDown,
				const core::rect<s32>* clip =0) =0;

		//! Draws the outline of a 2D rectangle.
		/** \param pos Position of the rectangle.
		\param color Color of the rectangle to draw. The alpha component
		specifies how transparent the rectangle outline will be. */
		virtual void draw2DRectangleOutline(const core::recti& pos,
				SColor color=SColor(255,255,255,255)) =0;

		//! Draws a 2d line. Both start and end will be included in coloring.
		/** \param start Screen coordinates of the start of the line
		in pixels.
		\param end Screen coordinates of the start of the line in
		pixels.
		\param color Color of the line to draw. */
		virtual void draw2DLine(const core::position2d<s32>& start,
					const core::position2d<s32>& end,
					SColor color=SColor(255,255,255,255)) =0;

		//! Draws a pixel.
		/** \param x The x-position of the pixel.
		\param y The y-position of the pixel.
		\param color Color of the pixel to draw. */
		virtual void drawPixel(u32 x, u32 y, const SColor& color) =0;

		//! Draws a non filled concyclic regular 2d polygon.
		/** This method can be used to draw circles, but also
		triangles, tetragons, pentagons, hexagons, heptagons, octagons,
		enneagons, decagons, hendecagons, dodecagon, triskaidecagons,
		etc. I think you'll got it now. And all this by simply
		specifying the vertex count. Welcome to the wonders of
		geometry.
		\param center Position of center of circle (pixels).
		\param radius Radius of circle in pixels.
		\param color Color of the circle.
		\param vertexCount Amount of vertices of the polygon. Specify 2
		to draw a line, 3 to draw a triangle, 4 for tetragons and a lot
		(>10) for nearly a circle. */
		virtual void draw2DPolygon(core::position2d<s32> center,
				f32 radius,
				video::SColor color=SColor(100,255,255,255),
				s32 vertexCount=10) =0;

		//! Draws a shadow volume into the stencil buffer.
		/** To draw a stencil shadow, do this: First, draw all geometry.
		Then use this method, to draw the shadow volume. Then, use
		IVideoDriver::drawStencilShadow() to visualize the shadow.
		Please note that the code for the opengl version of the method
		is based on free code sent in by Philipp Dortmann, lots of
		thanks go to him!
		\param triangles Array of 3d vectors, specifying the shadow
		volume.
		\param zfail If set to true, zfail method is used, otherwise
		zpass.
		\param debugDataVisible The debug data that is enabled for this
		shadow node
		*/
		virtual void drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail=true, u32 debugDataVisible=0) =0;

		//! Fills the stencil shadow with color.
		/** After the shadow volume has been drawn into the stencil
		buffer using IVideoDriver::drawStencilShadowVolume(), use this
		to draw the color of the shadow.
		Please note that the code for the opengl version of the method
		is based on free code sent in by Philipp Dortmann, lots of
		thanks go to him!
		\param clearStencilBuffer Set this to false, if you want to
		draw every shadow with the same color, and only want to call
		drawStencilShadow() once after all shadow volumes have been
		drawn. Set this to true, if you want to paint every shadow with
		its own color.
		\param leftUpEdge Color of the shadow in the upper left corner
		of screen.
		\param rightUpEdge Color of the shadow in the upper right
		corner of screen.
		\param leftDownEdge Color of the shadow in the lower left
		corner of screen.
		\param rightDownEdge Color of the shadow in the lower right
		corner of screen. */
		virtual void drawStencilShadow(bool clearStencilBuffer=false,
			video::SColor leftUpEdge = video::SColor(255,0,0,0),
			video::SColor rightUpEdge = video::SColor(255,0,0,0),
			video::SColor leftDownEdge = video::SColor(255,0,0,0),
			video::SColor rightDownEdge = video::SColor(255,0,0,0)) =0;

		//! Draws a mesh buffer
		/** \param mb Buffer to draw */
		virtual void drawMeshBuffer(const scene::IMeshBuffer* mb) =0;

		//! Draws normals of a mesh buffer
		/** \param mb Buffer to draw the normals of
		\param length length scale factor of the normals
		\param color Color the normals are rendered with
		*/
		virtual void drawMeshBufferNormals(const scene::IMeshBuffer* mb, f32 length=10.f, SColor color=0xffffffff) =0;

		//! Sets the fog mode.
		/** These are global values attached to each 3d object rendered,
		which has the fog flag enabled in its material.
		\param color Color of the fog
		\param fogType Type of fog used
		\param start Only used in linear fog mode (linearFog=true).
		Specifies where fog starts.
		\param end Only used in linear fog mode (linearFog=true).
		Specifies where fog ends.
		\param density Only used in exponential fog mode
		(linearFog=false). Must be a value between 0 and 1.
		\param pixelFog Set this to false for vertex fog, and true if
		you want per-pixel fog.
		\param rangeFog Set this to true to enable range-based vertex
		fog. The distance from the viewer is used to compute the fog,
		not the z-coordinate. This is better, but slower. This might not
		be available with all drivers and fog settings. */
		virtual void setFog(SColor color=SColor(0,255,255,255),
				E_FOG_TYPE fogType=EFT_FOG_LINEAR,
				f32 start=50.0f, f32 end=100.0f, f32 density=0.01f,
				bool pixelFog=false, bool rangeFog=false) =0;

		//! Gets the fog mode.
		virtual void getFog(SColor& color, E_FOG_TYPE& fogType,
				f32& start, f32& end, f32& density,
				bool& pixelFog, bool& rangeFog) = 0;

		//! Get the current color format of the color buffer
		/** \return Color format of the color buffer. */
		virtual ECOLOR_FORMAT getColorFormat() const =0;

		//! Get the size of the screen or render window.
		/** \return Size of screen or render window. */
		virtual const core::dimension2d<u32>& getScreenSize() const =0;

		//! Get the size of the current render target
		/** This method will return the screen size if the driver
		doesn't support render to texture, or if the current render
		target is the screen.
		\return Size of render target or screen/window */
		virtual const core::dimension2d<u32>& getCurrentRenderTargetSize() const =0;

		//! Returns current frames per second value.
		/** This value is updated approximately every 1.5 seconds and
		is only intended to provide a rough guide to the average frame
		rate. It is not suitable for use in performing timing
		calculations or framerate independent movement.
		\return Approximate amount of frames per second drawn. */
		virtual s32 getFPS() const =0;

		//! Returns amount of primitives (mostly triangles) which were drawn in the last frame.
		/** Together with getFPS() very useful method for statistics.
		\param mode Defines if the primitives drawn are accumulated or
		counted per frame.
		\return Amount of primitives drawn in the last frame. */
		virtual u32 getPrimitiveCountDrawn( u32 mode =0 ) const =0;

		//! Deletes all dynamic lights which were previously added with addDynamicLight().
		virtual void deleteAllDynamicLights() =0;

		//! adds a dynamic light, returning an index to the light
		//! \param light: the light data to use to create the light
		//! \return An index to the light, or -1 if an error occurs
		virtual s32 addDynamicLight(const SLight& light) =0;

		//! Returns the maximal amount of dynamic lights the device can handle
		/** \return Maximal amount of dynamic lights. */
		virtual u32 getMaximalDynamicLightAmount() const =0;

		//! Returns amount of dynamic lights currently set
		/** \return Amount of dynamic lights currently set */
		virtual u32 getDynamicLightCount() const =0;

		//! Returns light data which was previously set by IVideoDriver::addDynamicLight().
		/** \param idx Zero based index of the light. Must be 0 or
		greater and smaller than IVideoDriver::getDynamicLightCount.
		\return Light data. */
		virtual const SLight& getDynamicLight(u32 idx) const =0;

		//! Turns a dynamic light on or off
		//! \param lightIndex: the index returned by addDynamicLight
		//! \param turnOn: true to turn the light on, false to turn it off
		virtual void turnLightOn(s32 lightIndex, bool turnOn) =0;

		//! Gets name of this video driver.
		/** \return Returns the name of the video driver, e.g. in case
		of the Direct3D8 driver, it would return "Direct3D 8.1". */
		virtual const wchar_t* getName() const =0;

		//! Adds an external image loader to the engine.
		/** This is useful if the Irrlicht Engine should be able to load
		textures of currently unsupported file formats (e.g. gif). The
		IImageLoader only needs to be implemented for loading this file
		format. A pointer to the implementation can be passed to the
		engine using this method.
		\param loader Pointer to the external loader created. */
		virtual void addExternalImageLoader(IImageLoader* loader) =0;

		//! Adds an external image writer to the engine.
		/** This is useful if the Irrlicht Engine should be able to
		write textures of currently unsupported file formats (e.g
		.gif). The IImageWriter only needs to be implemented for
		writing this file format. A pointer to the implementation can
		be passed to the engine using this method.
		\param writer: Pointer to the external writer created. */
		virtual void addExternalImageWriter(IImageWriter* writer) =0;

		//! Returns the maximum amount of primitives
		/** (mostly vertices) which the device is able to render with
		one drawVertexPrimitiveList call.
		\return Maximum amount of primitives. */
		virtual u32 getMaximalPrimitiveCount() const =0;

		//! Enables or disables a texture creation flag.
		/** These flags define how textures should be created. By
		changing this value, you can influence for example the speed of
		rendering a lot. But please note that the video drivers take
		this value only as recommendation. It could happen that you
		enable the ETCF_ALWAYS_16_BIT mode, but the driver still creates
		32 bit textures.
		\param flag Texture creation flag.
		\param enabled Specifies if the given flag should be enabled or
		disabled. */
		virtual void setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag, bool enabled=true) =0;

		//! Returns if a texture creation flag is enabled or disabled.
		/** You can change this value using setTextureCreationFlag().
		\param flag Texture creation flag.
		\return The current texture creation flag enabled mode. */
		virtual bool getTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag) const =0;

		//! Creates a software image from a file.
		/** No hardware texture will be created for this image. This
		method is useful for example if you want to read a heightmap
		for a terrain renderer.
		\param filename Name of the file from which the image is
		created.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		virtual IImage* createImageFromFile(const io::path& filename) = 0;

		//! Creates a software image from a file.
		/** No hardware texture will be created for this image. This
		method is useful for example if you want to read a heightmap
		for a terrain renderer.
		\param file File from which the image is created.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		virtual IImage* createImageFromFile(io::IReadFile* file) =0;

		//! Writes the provided image to a file.
		/** Requires that there is a suitable image writer registered
		for writing the image.
		\param image Image to write.
		\param filename Name of the file to write.
		\param param Control parameter for the backend (e.g. compression
		level).
		\return True on successful write. */
		virtual bool writeImageToFile(IImage* image, const io::path& filename, u32 param = 0) = 0;

		//! Writes the provided image to a file.
		/** Requires that there is a suitable image writer registered
		for writing the image.
		\param image Image to write.
		\param file  An already open io::IWriteFile object. The name
		will be used to determine the appropriate image writer to use.
		\param param Control parameter for the backend (e.g. compression
		level).
		\return True on successful write. */
		virtual bool writeImageToFile(IImage* image, io::IWriteFile* file, u32 param =0) =0;

		//! Creates a software image from a byte array.
		/** No hardware texture will be created for this image. This
		method is useful for example if you want to read a heightmap
		for a terrain renderer.
		\param format Desired color format of the texture
		\param size Desired size of the image
		\param data A byte array with pixel color information
		\param ownForeignMemory If true, the image will use the data
		pointer directly and own it afterwards. If false, the memory
		will by copied internally.
		\param deleteMemory Whether the memory is deallocated upon
		destruction.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		virtual IImage* createImageFromData(ECOLOR_FORMAT format,
			const core::dimension2d<u32>& size, void *data,
			bool ownForeignMemory=false,
			bool deleteMemory = true) =0;

		//! Creates an empty software image.
		/**
		\param format Desired color format of the image.
		\param size Size of the image to create.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		virtual IImage* createImage(ECOLOR_FORMAT format, const core::dimension2d<u32>& size) =0;

		//! Creates a software image by converting it to given format from another image.
		/** \deprecated Create an empty image and use copyTo(). This method may be removed by Irrlicht 1.9.
		\param format Desired color format of the image.
		\param imageToCopy Image to copy to the new image.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		_IRR_DEPRECATED_ virtual IImage* createImage(ECOLOR_FORMAT format, IImage *imageToCopy) =0;

		//! Creates a software image from a part of another image.
		/** \deprecated Create an empty image and use copyTo(). This method may be removed by Irrlicht 1.9.
		\param imageToCopy Image to copy to the new image in part.
		\param pos Position of rectangle to copy.
		\param size Extents of rectangle to copy.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		_IRR_DEPRECATED_ virtual IImage* createImage(IImage* imageToCopy,
				const core::position2d<s32>& pos,
				const core::dimension2d<u32>& size) =0;

		//! Creates a software image from a part of a texture.
		/**
		\param texture Texture to copy to the new image in part.
		\param pos Position of rectangle to copy.
		\param size Extents of rectangle to copy.
		\return The created image.
		If you no longer need the image, you should call IImage::drop().
		See IReferenceCounted::drop() for more information. */
		virtual IImage* createImage(ITexture* texture,
				const core::position2d<s32>& pos,
				const core::dimension2d<u32>& size) =0;

		//! Event handler for resize events. Only used by the engine internally.
		/** Used to notify the driver that the window was resized.
		Usually, there is no need to call this method. */
		virtual void OnResize(const core::dimension2d<u32>& size) =0;

		//! Adds a new material renderer to the video device.
		/** Use this method to extend the VideoDriver with new material
		types. To extend the engine using this method do the following:
		Derive a class from IMaterialRenderer and override the methods
		you need. For setting the right renderstates, you can try to
		get a pointer to the real rendering device using
		IVideoDriver::getExposedVideoData(). Add your class with
		IVideoDriver::addMaterialRenderer(). To use an object being
		displayed with your new material, set the MaterialType member of
		the SMaterial struct to the value returned by this method.
		If you simply want to create a new material using vertex and/or
		pixel shaders it would be easier to use the
		video::IGPUProgrammingServices interface which you can get
		using the getGPUProgrammingServices() method.
		\param renderer A pointer to the new renderer.
		\param name Optional name for the material renderer entry.
		\return The number of the material type which can be set in
		SMaterial::MaterialType to use the renderer. -1 is returned if
		an error occured. For example if you tried to add an material
		renderer to the software renderer or the null device, which do
		not accept material renderers. */
		virtual s32 addMaterialRenderer(IMaterialRenderer* renderer, const c8* name =0) =0;

		//! Get access to a material renderer by index.
		/** \param idx Id of the material renderer. Can be a value of
		the E_MATERIAL_TYPE enum or a value which was returned by
		addMaterialRenderer().
		\return Pointer to material renderer or null if not existing. */
		virtual IMaterialRenderer* getMaterialRenderer(u32 idx) =0;

		//! Get amount of currently available material renderers.
		/** \return Amount of currently available material renderers. */
		virtual u32 getMaterialRendererCount() const =0;

		//! Get name of a material renderer
		/** This string can, e.g., be used to test if a specific
		renderer already has been registered/created, or use this
		string to store data about materials: This returned name will
		be also used when serializing materials.
		\param idx Id of the material renderer. Can be a value of the
		E_MATERIAL_TYPE enum or a value which was returned by
		addMaterialRenderer().
		\return String with the name of the renderer, or 0 if not
		exisiting */
		virtual const c8* getMaterialRendererName(u32 idx) const =0;

		//! Sets the name of a material renderer.
		/** Will have no effect on built-in material renderers.
		\param idx: Id of the material renderer. Can be a value of the
		E_MATERIAL_TYPE enum or a value which was returned by
		addMaterialRenderer().
		\param name: New name of the material renderer. */
		virtual void setMaterialRendererName(s32 idx, const c8* name) =0;

		//! Creates material attributes list from a material
		/** This method is useful for serialization and more.
		Please note that the video driver will use the material
		renderer names from getMaterialRendererName() to write out the
		material type name, so they should be set before.
		\param material The material to serialize.
		\param options Additional options which might influence the
		serialization.
		\return The io::IAttributes container holding the material
		properties. */
		virtual io::IAttributes* createAttributesFromMaterial(const video::SMaterial& material,
			io::SAttributeReadWriteOptions* options=0) =0;

		//! Fills an SMaterial structure from attributes.
		/** Please note that for setting material types of the
		material, the video driver will need to query the material
		renderers for their names, so all non built-in materials must
		have been created before calling this method.
		\param outMaterial The material to set the properties for.
		\param attributes The attributes to read from. */
		virtual void fillMaterialStructureFromAttributes(video::SMaterial& outMaterial, io::IAttributes* attributes) =0;

		//! Returns driver and operating system specific data about the IVideoDriver.
		/** This method should only be used if the engine should be
		extended without having to modify the source of the engine.
		\return Collection of device dependent pointers. */
		virtual const SExposedVideoData& getExposedVideoData() =0;

		//! Get type of video driver
		/** \return Type of driver. */
		virtual E_DRIVER_TYPE getDriverType() const =0;

		//! Gets the IGPUProgrammingServices interface.
		/** \return Pointer to the IGPUProgrammingServices. Returns 0
		if the video driver does not support this. For example the
		Software driver and the Null driver will always return 0. */
		virtual IGPUProgrammingServices* getGPUProgrammingServices() =0;

		//! Returns a pointer to the mesh manipulator.
		virtual scene::IMeshManipulator* getMeshManipulator() =0;

		//! Clears the ZBuffer.
		/** Note that you usually need not to call this method, as it
		is automatically done in IVideoDriver::beginScene() or
		IVideoDriver::setRenderTarget() if you enable zBuffer. But if
		you have to render some special things, you can clear the
		zbuffer during the rendering process with this method any time.
		*/
		virtual void clearZBuffer() =0;

		//! Make a screenshot of the last rendered frame.
		/** \return An image created from the last rendered frame. */
		virtual IImage* createScreenShot(video::ECOLOR_FORMAT format=video::ECF_UNKNOWN, video::E_RENDER_TARGET target=video::ERT_FRAME_BUFFER) =0;

		//! Check if the image is already loaded.
		/** Works similar to getTexture(), but does not load the texture
		if it is not currently loaded.
		\param filename Name of the texture.
		\return Pointer to loaded texture, or 0 if not found. */
		virtual video::ITexture* findTexture(const io::path& filename) = 0;

		//! Set or unset a clipping plane.
		/** There are at least 6 clipping planes available for the user
		to set at will.
		\param index The plane index. Must be between 0 and
		MaxUserClipPlanes.
		\param plane The plane itself.
		\param enable If true, enable the clipping plane else disable
		it.
		\return True if the clipping plane is usable. */
		virtual bool setClipPlane(u32 index, const core::plane3df& plane, bool enable=false) =0;

		//! Enable or disable a clipping plane.
		/** There are at least 6 clipping planes available for the user
		to set at will.
		\param index The plane index. Must be between 0 and
		MaxUserClipPlanes.
		\param enable If true, enable the clipping plane else disable
		it. */
		virtual void enableClipPlane(u32 index, bool enable) =0;

		//! Set the minimum number of vertices for which a hw buffer will be created
		/** \param count Number of vertices to set as minimum. */
		virtual void setMinHardwareBufferVertexCount(u32 count) =0;

		//! Get the global Material, which might override local materials.
		/** Depending on the enable flags, values from this Material
		are used to override those of local materials of some
		meshbuffer being rendered.
		\return Reference to the Override Material. */
		virtual SOverrideMaterial& getOverrideMaterial() =0;

		//! Get the 2d override material for altering its values
		/** The 2d override materual allows to alter certain render
		states of the 2d methods. Not all members of SMaterial are
		honored, especially not MaterialType and Textures. Moreover,
		the zbuffer is always ignored, and lighting is always off. All
		other flags can be changed, though some might have to effect
		in most cases.
		Please note that you have to enable/disable this effect with
		enableInitMaterial2D(). This effect is costly, as it increases
		the number of state changes considerably. Always reset the
		values when done.
		\return Material reference which should be altered to reflect
		the new settings.
		*/
		virtual SMaterial& getMaterial2D() =0;

		//! Enable the 2d override material
		/** \param enable Flag which tells whether the material shall be
		enabled or disabled. */
		virtual void enableMaterial2D(bool enable=true) =0;

		//! Get the graphics card vendor name.
		virtual core::stringc getVendorInfo() =0;

		//! Only used by the engine internally.
		/** The ambient color is set in the scene manager, see
		scene::ISceneManager::setAmbientLight().
		\param color New color of the ambient light. */
		virtual void setAmbientLight(const SColorf& color) =0;

		//! Only used by the engine internally.
		/** Passes the global material flag AllowZWriteOnTransparent.
		Use the SceneManager attribute to set this value from your app.
		\param flag Default behavior is to disable ZWrite, i.e. false. */
		virtual void setAllowZWriteOnTransparent(bool flag) =0;

		//! Get the maximum texture size supported.
		virtual core::dimension2du getMaxTextureSize() const =0;

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
		virtual void convertColor(const void* sP, ECOLOR_FORMAT sF, s32 sN,
				void* dP, ECOLOR_FORMAT dF) const =0;
	};

} // end namespace video
} // end namespace irr


#endif
