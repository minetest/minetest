// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_VIDEO_2_SOFTWARE_H_INCLUDED__
#define __C_VIDEO_2_SOFTWARE_H_INCLUDED__

#include "SoftwareDriver2_compile_config.h"
#include "IBurningShader.h"
#include "CNullDriver.h"
#include "CImage.h"
#include "os.h"
#include "irrString.h"
#include "SIrrCreationParameters.h"

namespace irr
{
namespace video
{
	class CBurningVideoDriver : public CNullDriver
	{
	public:

		//! constructor
		CBurningVideoDriver(const irr::SIrrlichtCreationParameters& params, io::IFileSystem* io, video::IImagePresenter* presenter);

		//! destructor
		virtual ~CBurningVideoDriver();

		//! queries the features of the driver, returns true if feature is available
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const;

		//! sets transformation
		virtual void setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat);

		//! sets a material
		virtual void setMaterial(const SMaterial& material);

		virtual bool setRenderTarget(video::ITexture* texture, bool clearBackBuffer,
						bool clearZBuffer, SColor color);

		//! sets a viewport
		virtual void setViewPort(const core::rect<s32>& area);

		//! clears the zbuffer
		virtual bool beginScene(bool backBuffer=true, bool zBuffer=true,
				SColor color=SColor(255,0,0,0),
				const SExposedVideoData& videoData=SExposedVideoData(),
				core::rect<s32>* sourceRect=0);

		//! presents the rendered scene on the screen, returns false if failed
		virtual bool endScene();

		//! Only used by the internal engine. Used to notify the driver that
		//! the window was resized.
		virtual void OnResize(const core::dimension2d<u32>& size);

		//! returns size of the current render target
		virtual const core::dimension2d<u32>& getCurrentRenderTargetSize() const;

		//! deletes all dynamic lights there are
		virtual void deleteAllDynamicLights();

		//! adds a dynamic light, returning an index to the light
		//! \param light: the light data to use to create the light
		//! \return An index to the light, or -1 if an error occurs
		virtual s32 addDynamicLight(const SLight& light);

		//! Turns a dynamic light on or off
		//! \param lightIndex: the index returned by addDynamicLight
		//! \param turnOn: true to turn the light on, false to turn it off
		virtual void turnLightOn(s32 lightIndex, bool turnOn);

		//! returns the maximal amount of dynamic lights the device can handle
		virtual u32 getMaximalDynamicLightAmount() const;

		//! Sets the dynamic ambient light color. The default color is
		//! (0,0,0,0) which means it is dark.
		//! \param color: New color of the ambient light.
		virtual void setAmbientLight(const SColorf& color);

		//! draws a vertex primitive list
		void drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType);

		//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
		virtual void draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
			SColor color=SColor(255,255,255,255), bool useAlphaChannelOfTexture=false);

	//! Draws a part of the texture into the rectangle.
		virtual void draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
				const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
				const video::SColor* const colors=0, bool useAlphaChannelOfTexture=false);

		//! Draws a 3d line.
		virtual void draw3DLine(const core::vector3df& start,
			const core::vector3df& end, SColor color = SColor(255,255,255,255));

		//! draw an 2d rectangle
		virtual void draw2DRectangle(SColor color, const core::rect<s32>& pos,
			const core::rect<s32>* clip = 0);

		//!Draws an 2d rectangle with a gradient.
		virtual void draw2DRectangle(const core::rect<s32>& pos,
			SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
			const core::rect<s32>* clip = 0);

		//! Draws a 2d line.
		virtual void draw2DLine(const core::position2d<s32>& start,
					const core::position2d<s32>& end,
					SColor color=SColor(255,255,255,255));

		//! Draws a single pixel
		virtual void drawPixel(u32 x, u32 y, const SColor & color);

		//! \return Returns the name of the video driver. Example: In case of the DirectX8
		//! driver, it would return "Direct3D8.1".
		virtual const wchar_t* getName() const;

		//! Returns type of video driver
		virtual E_DRIVER_TYPE getDriverType() const;

		//! get color format of the current color buffer
		virtual ECOLOR_FORMAT getColorFormat() const;

		//! Returns the transformation set by setTransform
		virtual const core::matrix4& getTransform(E_TRANSFORMATION_STATE state) const;

		//! Creates a render target texture.
		virtual ITexture* addRenderTargetTexture(const core::dimension2d<u32>& size,
			const io::path& name, const ECOLOR_FORMAT format = ECF_UNKNOWN);

		//! Clears the DepthBuffer.
		virtual void clearZBuffer();

		//! Returns an image created from the last rendered frame.
		virtual IImage* createScreenShot(video::ECOLOR_FORMAT format=video::ECF_UNKNOWN, video::E_RENDER_TARGET target=video::ERT_FRAME_BUFFER);

		//! Returns the maximum amount of primitives (mostly vertices) which
		//! the device is able to render with one drawIndexedTriangleList
		//! call.
		virtual u32 getMaximalPrimitiveCount() const;

		//! Draws a shadow volume into the stencil buffer. To draw a stencil shadow, do
		//! this: First, draw all geometry. Then use this method, to draw the shadow
		//! volume. Then, use IVideoDriver::drawStencilShadow() to visualize the shadow.
		virtual void drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail=true, u32 debugDataVisible=0);

		//! Fills the stencil shadow with color. After the shadow volume has been drawn
		//! into the stencil buffer using IVideoDriver::drawStencilShadowVolume(), use this
		//! to draw the color of the shadow.
		virtual void drawStencilShadow(bool clearStencilBuffer=false,
			video::SColor leftUpEdge = video::SColor(0,0,0,0),
			video::SColor rightUpEdge = video::SColor(0,0,0,0),
			video::SColor leftDownEdge = video::SColor(0,0,0,0),
			video::SColor rightDownEdge = video::SColor(0,0,0,0));

		//! Returns the graphics card vendor name.
		virtual core::stringc getVendorInfo();

		//! Returns the maximum texture size supported.
		virtual core::dimension2du getMaxTextureSize() const;

		virtual IDepthBuffer * getDepthBuffer () { return DepthBuffer; }
		virtual IStencilBuffer * getStencilBuffer () { return StencilBuffer; }

	protected:


		//! sets a render target
		void setRenderTarget(video::CImage* image);

		//! sets the current Texture
		//bool setTexture(u32 stage, video::ITexture* texture);

		//! returns a device dependent texture from a software surface (IImage)
		//! THIS METHOD HAS TO BE OVERRIDDEN BY DERIVED DRIVERS WITH OWN TEXTURES
		virtual video::ITexture* createDeviceDependentTexture(IImage* surface, const io::path& name, void* mipmapData=0);

		video::CImage* BackBuffer;
		video::IImagePresenter* Presenter;

		void* WindowId;
		core::rect<s32>* SceneSourceRect;

		video::ITexture* RenderTargetTexture;
		video::IImage* RenderTargetSurface;
		core::dimension2d<u32> RenderTargetSize;

		//! selects the right triangle renderer based on the render states.
		void setCurrentShader();

		IBurningShader* CurrentShader;
		IBurningShader* BurningShader[ETR2_COUNT];

		IDepthBuffer* DepthBuffer;
		IStencilBuffer* StencilBuffer;


		/*
			extend Matrix Stack
			-> combined CameraProjection
			-> combined CameraProjectionWorld
			-> ClipScale from NDC to DC Space
		*/
		enum E_TRANSFORMATION_STATE_BURNING_VIDEO
		{
			ETS_VIEW_PROJECTION = ETS_COUNT,
			ETS_CURRENT,
			ETS_CLIPSCALE,
			ETS_VIEW_INVERSE,
			ETS_WORLD_INVERSE,

			ETS_COUNT_BURNING
		};

		enum E_TRANSFORMATION_FLAG
		{
			ETF_IDENTITY = 1,
			ETF_TEXGEN_CAMERA_NORMAL = 2,
			ETF_TEXGEN_CAMERA_REFLECTION = 4,
		};
		u32 TransformationFlag[ETS_COUNT_BURNING];
		core::matrix4 Transformation[ETS_COUNT_BURNING];

		void getCameraPosWorldSpace ();
		void getLightPosObjectSpace ();


		// Vertex Cache
		static const SVSize vSize[];

		SVertexCache VertexCache;

		void VertexCache_reset (const void* vertices, u32 vertexCount,
					const void* indices, u32 indexCount,
					E_VERTEX_TYPE vType,scene::E_PRIMITIVE_TYPE pType,
					E_INDEX_TYPE iType);
		void VertexCache_get ( const s4DVertex ** face );
		void VertexCache_getbypass ( s4DVertex ** face );

		void VertexCache_fill ( const u32 sourceIndex,const u32 destIndex );
		s4DVertex * VertexCache_getVertex ( const u32 sourceIndex );


		// culling & clipping
		u32 clipToHyperPlane ( s4DVertex * dest, const s4DVertex * source, u32 inCount, const sVec4 &plane );
		u32 clipToFrustumTest ( const s4DVertex * v  ) const;
		u32 clipToFrustum ( s4DVertex *source, s4DVertex * temp, const u32 vIn );


#ifdef SOFTWARE_DRIVER_2_LIGHTING

		void lightVertex ( s4DVertex *dest, u32 vertexargb );
		//! Sets the fog mode.
		virtual void setFog(SColor color, E_FOG_TYPE fogType, f32 start,
			f32 end, f32 density, bool pixelFog, bool rangeFog);
#endif


		// holds transformed, clipped vertices
		SAlignedVertex CurrentOut;
		SAlignedVertex Temp;

		void ndc_2_dc_and_project ( s4DVertex *dest,s4DVertex *source, u32 vIn ) const;
		f32 screenarea ( const s4DVertex *v0 ) const;
		void select_polygon_mipmap ( s4DVertex *source, u32 vIn, u32 tex, const core::dimension2du& texSize ) const;
		f32 texelarea ( const s4DVertex *v0, int tex ) const;


		void ndc_2_dc_and_project2 ( const s4DVertex **v, const u32 size ) const;
		f32 screenarea2 ( const s4DVertex **v ) const;
		f32 texelarea2 ( const s4DVertex **v, int tex ) const;
		void select_polygon_mipmap2 ( s4DVertex **source, u32 tex, const core::dimension2du& texSize ) const;


		SBurningShaderLightSpace LightSpace;
		SBurningShaderMaterial Material;

		static const sVec4 NDCPlane[6];
	};

} // end namespace video
} // end namespace irr


#endif

