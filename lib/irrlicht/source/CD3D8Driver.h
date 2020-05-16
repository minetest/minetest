// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_VIDEO_DIRECTX_8_H_INCLUDED__
#define __C_VIDEO_DIRECTX_8_H_INCLUDED__

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_DIRECT3D_8_

#ifdef _IRR_WINDOWS_
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#include "SIrrCreationParameters.h"
// always included for static createDriver function
#include "CNullDriver.h"
#include "IMaterialRendererServices.h"

#include <d3d8.h>

namespace irr
{
namespace video
{
	class CD3D8Driver : public CNullDriver, IMaterialRendererServices
	{
		friend class CD3D8Texture;

	public:

		//! constructor
		CD3D8Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io);

		//! destructor
		virtual ~CD3D8Driver();

		//! applications must call this method before performing any rendering. returns false if failed.
		virtual bool beginScene(bool backBuffer=true, bool zBuffer=true,
				SColor color=SColor(255,0,0,0),
				const SExposedVideoData& videoData=SExposedVideoData(),
				core::rect<s32>* sourceRect=0);

		//! applications must call this method after performing any rendering. returns false if failed.
		virtual bool endScene();

		//! queries the features of the driver, returns true if feature is available
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const;

		//! sets transformation
		virtual void setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat);

		//! sets a material
		virtual void setMaterial(const SMaterial& material);

		//! sets a render target
		virtual bool setRenderTarget(video::ITexture* texture,
			bool clearBackBuffer=true, bool clearZBuffer=true,
			SColor color=video::SColor(0,0,0,0));

		//! sets a viewport
		virtual void setViewPort(const core::rect<s32>& area);

		//! gets the area of the current viewport
		virtual const core::rect<s32>& getViewPort() const;

		//! draws a vertex primitive list
		virtual void drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType,
				E_INDEX_TYPE iType);

		//! draws a vertex primitive list in 2d
		virtual void draw2DVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType,
				E_INDEX_TYPE iType);

		//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
		virtual void draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
			SColor color=SColor(255,255,255,255), bool useAlphaChannelOfTexture=false);

		//! Draws a part of the texture into the rectangle.
		virtual void draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
			const video::SColor* const colors=0, bool useAlphaChannelOfTexture=false);

		//!Draws an 2d rectangle with a gradient.
		virtual void draw2DRectangle(const core::rect<s32>& pos,
			SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
			const core::rect<s32>* clip = 0);

		//! Draws a 2d line.
		virtual void draw2DLine(const core::position2d<s32>& start,
					const core::position2d<s32>& end,
					SColor color=SColor(255,255,255,255));

		//! Draws a pixel.
		virtual void drawPixel(u32 x, u32 y, const SColor & color);

		//! Draws a 3d line.
		virtual void draw3DLine(const core::vector3df& start,
			const core::vector3df& end, SColor color = SColor(255,255,255,255));

		//! initialises the Direct3D API
		bool initDriver(HWND hwnd, bool pureSoftware);

		//! \return Returns the name of the video driver. Example: In case of the DIRECT3D8
		//! driver, it would return "Direct3D8.1".
		virtual const wchar_t* getName() const;

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

		//! Draws a shadow volume into the stencil buffer. To draw a stencil shadow, do
		//! this: Frist, draw all geometry. Then use this method, to draw the shadow
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

		//! Returns the maximum amount of primitives (mostly vertices) which
		//! the device is able to render with one drawIndexedTriangleList
		//! call.
		virtual u32 getMaximalPrimitiveCount() const;

		//! Enables or disables a texture creation flag.
		virtual void setTextureCreationFlag(E_TEXTURE_CREATION_FLAG flag, bool enabled);

		//! Sets the fog mode.
		virtual void setFog(SColor color, E_FOG_TYPE fogType, f32 start,
			f32 end, f32 density, bool pixelFog, bool rangeFog);

		//! Only used by the internal engine. Used to notify the driver that
		//! the window was resized.
		virtual void OnResize(const core::dimension2d<u32>& size);

		//! Returns type of video driver
		virtual E_DRIVER_TYPE getDriverType() const;

		//! Returns the transformation set by setTransform
		virtual const core::matrix4& getTransform(E_TRANSFORMATION_STATE state) const;

		//! Can be called by an IMaterialRenderer to make its work easier.
		virtual void setBasicRenderStates(const SMaterial& material, const SMaterial& lastMaterial,
			bool resetAllRenderstates);

		//! Sets a vertex shader constant.
		virtual void setVertexShaderConstant(const f32* data, s32 startRegister, s32 constantAmount=1);

		//! Sets a pixel shader constant.
		virtual void setPixelShaderConstant(const f32* data, s32 startRegister, s32 constantAmount=1);

		//! Sets a constant for the vertex shader based on a name.
		virtual bool setVertexShaderConstant(const c8* name, const f32* floats, int count);

		//! Bool interface for the above.
		virtual bool setVertexShaderConstant(const c8* name, const bool* bools, int count);

		//! Int interface for the above.
		virtual bool setVertexShaderConstant(const c8* name, const s32* ints, int count);

		//! Sets a constant for the pixel shader based on a name.
		virtual bool setPixelShaderConstant(const c8* name, const f32* floats, int count);

		//! Bool interface for the above.
		virtual bool setPixelShaderConstant(const c8* name, const bool* bools, int count);

		//! Int interface for the above.
		virtual bool setPixelShaderConstant(const c8* name, const s32* ints, int count);

		//! Returns a pointer to the IVideoDriver interface. (Implementation for
		//! IMaterialRendererServices)
		virtual IVideoDriver* getVideoDriver();

		//! Creates a render target texture.
		virtual ITexture* addRenderTargetTexture(const core::dimension2d<u32>& size,
				const io::path& name, const ECOLOR_FORMAT format = ECF_UNKNOWN);

		//! Clears the ZBuffer.
		virtual void clearZBuffer();

		//! Returns an image created from the last rendered frame.
		virtual IImage* createScreenShot(video::ECOLOR_FORMAT format=video::ECF_UNKNOWN, video::E_RENDER_TARGET target=video::ERT_FRAME_BUFFER);

		//! Set/unset a clipping plane.
		//! There are at least 6 clipping planes available for the user to set at will.
		//! \param index: The plane index. Must be between 0 and MaxUserClipPlanes.
		//! \param plane: The plane itself.
		//! \param enable: If true, enable the clipping plane else disable it.
		virtual bool setClipPlane(u32 index, const core::plane3df& plane, bool enable=false);

		//! Enable/disable a clipping plane.
		//! There are at least 6 clipping planes available for the user to set at will.
		//! \param index: The plane index. Must be between 0 and MaxUserClipPlanes.
		//! \param enable: If true, enable the clipping plane else disable it.
		virtual void enableClipPlane(u32 index, bool enable);

		//! Returns the maximum texture size supported.
		virtual core::dimension2du getMaxTextureSize() const;

		virtual bool checkDriverReset() {return DriverWasReset;}
	private:

		// enumeration for rendering modes such as 2d and 3d for minizing the switching of renderStates.
		enum E_RENDER_MODE
		{
			ERM_NONE = 0,	// no render state has been set yet.
			ERM_2D,			// 2d drawing rendermode
			ERM_3D,			// 3d rendering mode
			ERM_STENCIL_FILL, // stencil fill mode
			ERM_SHADOW_VOLUME_ZFAIL, // stencil volume draw mode
			ERM_SHADOW_VOLUME_ZPASS // stencil volume draw mode
		};

		//! sets right vertex shader
		void setVertexShader(video::E_VERTEX_TYPE newType);

		//! sets the needed renderstates
		bool setRenderStates3DMode();

		//! sets the needed renderstates
		void setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel);

		//! sets the needed renderstates
		void setRenderStatesStencilFillMode(bool alpha);

		//! sets the needed renderstates
		void setRenderStatesStencilShadowMode(bool zfail, u32 debugDataVisible);

		//! sets the current Texture
		bool setActiveTexture(u32 stage, const video::ITexture* texture);

		//! resets the device
		bool reset();

		//! returns a device dependent texture from a software surface (IImage)
		//! THIS METHOD HAS TO BE OVERRIDDEN BY DERIVED DRIVERS WITH OWN TEXTURES
		virtual video::ITexture* createDeviceDependentTexture(IImage* surface, const io::path& name, void* mipmapData=0);

		// returns the current size of the screen or rendertarget
		virtual const core::dimension2d<u32>& getCurrentRenderTargetSize() const;

		//! Adds a new material renderer to the VideoDriver, using pixel and/or
		//! vertex shaders to render geometry.
		s32 addShaderMaterial(const c8* vertexShaderProgram, const c8* pixelShaderProgram,
			IShaderConstantSetCallBack* callback,
			E_MATERIAL_TYPE baseMaterial, s32 userData);

		void createMaterialRenderers();

		void draw2D3DVertexPrimitiveList(const void* vertices,
				u32 vertexCount, const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType,
				E_INDEX_TYPE iType, bool is3D);

		D3DTEXTUREADDRESS getTextureWrapMode(const u8 clamp);

		inline D3DCOLORVALUE colorToD3D(const SColor& col)
		{
			const f32 f = 1.0f / 255.0f;
			D3DCOLORVALUE v;
			v.r = col.getRed() * f;
			v.g = col.getGreen() * f;
			v.b = col.getBlue() * f;
			v.a = col.getAlpha() * f;
			return v;
		}

		E_RENDER_MODE CurrentRenderMode;
		D3DPRESENT_PARAMETERS present;

		SMaterial Material, LastMaterial;
		bool ResetRenderStates; // bool to make all renderstates be reseted if set.
		bool Transformation3DChanged;
		const ITexture* CurrentTexture[MATERIAL_MAX_TEXTURES];
		core::matrix4 Matrices[ETS_COUNT]; // matrices of the 3d mode we need to restore when we switch back from the 2d mode.

		HINSTANCE D3DLibrary;
		IDirect3D8* pID3D;
		IDirect3DDevice8* pID3DDevice;

		IDirect3DSurface8* PrevRenderTarget;
		core::dimension2d<u32> CurrentRendertargetSize;

		HWND WindowId;
		core::rect<s32>* SceneSourceRect;

		D3DCAPS8 Caps;

		E_VERTEX_TYPE LastVertexType;

		D3DMATRIX UnitMatrix;

		u32 MaxTextureUnits;
		u32 MaxUserClipPlanes;
		f32 MaxLightDistance;
		s32 LastSetLight;
		bool DeviceLost;
		bool DriverWasReset;

		SColorf AmbientLight;

		SIrrlichtCreationParameters Params;
	};

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_DIRECT3D_8_

#endif // __C_VIDEO_DIRECTX_8_H_INCLUDED__

