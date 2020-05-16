// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#ifndef __C_VIDEO_OPEN_GL_H_INCLUDED__
#define __C_VIDEO_OPEN_GL_H_INCLUDED__

#include "IrrCompileConfig.h"

#include "SIrrCreationParameters.h"

namespace irr
{
	class CIrrDeviceWin32;
	class CIrrDeviceLinux;
	class CIrrDeviceSDL;
	class CIrrDeviceMacOSX;
}

#ifdef _IRR_COMPILE_WITH_OPENGL_

#include "CNullDriver.h"
#include "IMaterialRendererServices.h"
// also includes the OpenGL stuff
#include "COpenGLExtensionHandler.h"

#ifdef _IRR_COMPILE_WITH_CG_
#include "Cg/cg.h"
#endif

namespace irr
{

namespace video
{
	class COpenGLTexture;

	class COpenGLDriver : public CNullDriver, public IMaterialRendererServices, public COpenGLExtensionHandler
	{
		friend class COpenGLTexture;
	public:

		#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceWin32* device);
		//! inits the windows specific parts of the open gl driver
		bool initDriver(CIrrDeviceWin32* device);
		bool changeRenderContext(const SExposedVideoData& videoData, CIrrDeviceWin32* device);
		#endif

		#ifdef _IRR_COMPILE_WITH_X11_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceLinux* device);
		//! inits the GLX specific parts of the open gl driver
		bool initDriver(CIrrDeviceLinux* device);
		bool changeRenderContext(const SExposedVideoData& videoData, CIrrDeviceLinux* device);
		#endif

		#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceSDL* device);
		#endif

		#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_
		COpenGLDriver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, CIrrDeviceMacOSX *device);
		#endif

		//! generic version which overloads the unimplemented versions
		bool changeRenderContext(const SExposedVideoData& videoData, void* device) {return false;}

		//! destructor
		virtual ~COpenGLDriver();

		//! clears the zbuffer
		virtual bool beginScene(bool backBuffer=true, bool zBuffer=true,
				SColor color=SColor(255,0,0,0),
				const SExposedVideoData& videoData=SExposedVideoData(),
				core::rect<s32>* sourceRect=0);

		//! presents the rendered scene on the screen, returns false if failed
		virtual bool endScene();

		//! sets transformation
		virtual void setTransform(E_TRANSFORMATION_STATE state, const core::matrix4& mat);


		struct SHWBufferLink_opengl : public SHWBufferLink
		{
			SHWBufferLink_opengl(const scene::IMeshBuffer *_MeshBuffer): SHWBufferLink(_MeshBuffer), vbo_verticesID(0),vbo_indicesID(0){}

			GLuint vbo_verticesID; //tmp
			GLuint vbo_indicesID; //tmp

			GLuint vbo_verticesSize; //tmp
			GLuint vbo_indicesSize; //tmp
		};

		//! updates hardware buffer if needed
		virtual bool updateHardwareBuffer(SHWBufferLink *HWBuffer);

		//! Create hardware buffer from mesh
		virtual SHWBufferLink *createHardwareBuffer(const scene::IMeshBuffer* mb);

		//! Delete hardware buffer (only some drivers can)
		virtual void deleteHardwareBuffer(SHWBufferLink *HWBuffer);

		//! Draw hardware buffer
		virtual void drawHardwareBuffer(SHWBufferLink *HWBuffer);

		//! Create occlusion query.
		/** Use node for identification and mesh for occlusion test. */
		virtual void addOcclusionQuery(scene::ISceneNode* node,
				const scene::IMesh* mesh=0);

		//! Remove occlusion query.
		virtual void removeOcclusionQuery(scene::ISceneNode* node);

		//! Run occlusion query. Draws mesh stored in query.
		/** If the mesh shall not be rendered visible, use
		overrideMaterial to disable the color and depth buffer. */
		virtual void runOcclusionQuery(scene::ISceneNode* node, bool visible=false);

		//! Update occlusion query. Retrieves results from GPU.
		/** If the query shall not block, set the flag to false.
		Update might not occur in this case, though */
		virtual void updateOcclusionQuery(scene::ISceneNode* node, bool block=true);

		//! Return query result.
		/** Return value is the number of visible pixels/fragments.
		The value is a safe approximation, i.e. can be larger then the
		actual value of pixels. */
		virtual u32 getOcclusionQueryResult(scene::ISceneNode* node) const;

		//! draws a vertex primitive list
		virtual void drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType);

		//! draws a vertex primitive list in 2d
		virtual void draw2DVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType);

		//! queries the features of the driver, returns true if feature is available
		virtual bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const
		{
			return FeatureEnabled[feature] && COpenGLExtensionHandler::queryFeature(feature);
		}

		//! Sets a material. All 3d drawing functions draw geometry now
		//! using this material.
		//! \param material: Material to be used from now on.
		virtual void setMaterial(const SMaterial& material);

		//! draws a set of 2d images, using a color and the alpha channel of the
		//! texture if desired.
		void draw2DImageBatch(const video::ITexture* texture,
				const core::array<core::position2d<s32> >& positions,
				const core::array<core::rect<s32> >& sourceRects,
				const core::rect<s32>* clipRect,
				SColor color,
				bool useAlphaChannelOfTexture);

		//! draws an 2d image, using a color (if color is other then Color(255,255,255,255)) and the alpha channel of the texture if wanted.
		virtual void draw2DImage(const video::ITexture* texture, const core::position2d<s32>& destPos,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
			SColor color=SColor(255,255,255,255), bool useAlphaChannelOfTexture=false);

		//! draws a set of 2d images, using a color and the alpha
		/** channel of the texture if desired. The images are drawn
		beginning at pos and concatenated in one line. All drawings
		are clipped against clipRect (if != 0).
		The subtextures are defined by the array of sourceRects
		and are chosen by the indices given.
		\param texture: Texture to be drawn.
		\param pos: Upper left 2d destination position where the image will be drawn.
		\param sourceRects: Source rectangles of the image.
		\param indices: List of indices which choose the actual rectangle used each time.
		\param clipRect: Pointer to rectangle on the screen where the image is clipped to.
		This pointer can be 0. Then the image is not clipped.
		\param color: Color with which the image is colored.
		Note that the alpha component is used: If alpha is other than 255, the image will be transparent.
		\param useAlphaChannelOfTexture: If true, the alpha channel of the texture is
		used to draw the image. */
		virtual void draw2DImage(const video::ITexture* texture,
				const core::position2d<s32>& pos,
				const core::array<core::rect<s32> >& sourceRects,
				const core::array<s32>& indices,
				const core::rect<s32>* clipRect=0,
				SColor color=SColor(255,255,255,255),
				bool useAlphaChannelOfTexture=false);

		//! Draws a part of the texture into the rectangle.
		virtual void draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
			const video::SColor* const colors=0, bool useAlphaChannelOfTexture=false);

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

		//! Draws a 3d line.
		virtual void draw3DLine(const core::vector3df& start,
					const core::vector3df& end,
					SColor color = SColor(255,255,255,255));

		//! \return Returns the name of the video driver. Example: In case of the Direct3D8
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
		//! this: First, draw all geometry. Then use this method, to draw the shadow
		//! volume. Then, use IVideoDriver::drawStencilShadow() to visualize the shadow.
		virtual void drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible=0);

		//! Fills the stencil shadow with color. After the shadow volume has been drawn
		//! into the stencil buffer using IVideoDriver::drawStencilShadowVolume(), use this
		//! to draw the color of the shadow.
		virtual void drawStencilShadow(bool clearStencilBuffer=false,
			video::SColor leftUpEdge = video::SColor(0,0,0,0),
			video::SColor rightUpEdge = video::SColor(0,0,0,0),
			video::SColor leftDownEdge = video::SColor(0,0,0,0),
			video::SColor rightDownEdge = video::SColor(0,0,0,0));

		//! sets a viewport
		virtual void setViewPort(const core::rect<s32>& area);

		//! Sets the fog mode.
		virtual void setFog(SColor color, E_FOG_TYPE fogType, f32 start,
			f32 end, f32 density, bool pixelFog, bool rangeFog);

		//! Only used by the internal engine. Used to notify the driver that
		//! the window was resized.
		virtual void OnResize(const core::dimension2d<u32>& size);

		//! Returns type of video driver
		virtual E_DRIVER_TYPE getDriverType() const;

		//! get color format of the current color buffer
		virtual ECOLOR_FORMAT getColorFormat() const;

		//! Returns the transformation set by setTransform
		virtual const core::matrix4& getTransform(E_TRANSFORMATION_STATE state) const;

		//! Can be called by an IMaterialRenderer to make its work easier.
		virtual void setBasicRenderStates(const SMaterial& material, const SMaterial& lastmaterial,
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

		//! sets the current Texture
		//! Returns whether setting was a success or not.
		bool setActiveTexture(u32 stage, const video::ITexture* texture);

		//! disables all textures beginning with the optional fromStage parameter. Otherwise all texture stages are disabled.
		//! Returns whether disabling was successful or not.
		bool disableTextures(u32 fromStage=0);

		//! Adds a new material renderer to the VideoDriver, using
		//! extGLGetObjectParameteriv(shaderHandle, GL_OBJECT_COMPILE_STATUS_ARB, &status)
		//! pixel and/or vertex shaders to render geometry.
		virtual s32 addShaderMaterial(const c8* vertexShaderProgram, const c8* pixelShaderProgram,
			IShaderConstantSetCallBack* callback, E_MATERIAL_TYPE baseMaterial, s32 userData);

		//! Adds a new material renderer to the VideoDriver, using GLSL to render geometry.
		virtual s32 addHighLevelShaderMaterial(
				const c8* vertexShaderProgram,
				const c8* vertexShaderEntryPointName,
				E_VERTEX_SHADER_TYPE vsCompileTarget,
				const c8* pixelShaderProgram,
				const c8* pixelShaderEntryPointName,
				E_PIXEL_SHADER_TYPE psCompileTarget,
				const c8* geometryShaderProgram,
				const c8* geometryShaderEntryPointName = "main",
				E_GEOMETRY_SHADER_TYPE gsCompileTarget = EGST_GS_4_0,
				scene::E_PRIMITIVE_TYPE inType = scene::EPT_TRIANGLES,
				scene::E_PRIMITIVE_TYPE outType = scene::EPT_TRIANGLE_STRIP,
				u32 verticesOut = 0,
				IShaderConstantSetCallBack* callback = 0,
				E_MATERIAL_TYPE baseMaterial = video::EMT_SOLID,
				s32 userData = 0,
				E_GPU_SHADING_LANGUAGE shadingLang = EGSL_DEFAULT);

		//! Returns a pointer to the IVideoDriver interface. (Implementation for
		//! IMaterialRendererServices)
		virtual IVideoDriver* getVideoDriver();

		//! Returns the maximum amount of primitives (mostly vertices) which
		//! the device is able to render with one drawIndexedTriangleList
		//! call.
		virtual u32 getMaximalPrimitiveCount() const;

		virtual ITexture* addRenderTargetTexture(const core::dimension2d<u32>& size,
				const io::path& name, const ECOLOR_FORMAT format = ECF_UNKNOWN);

		//! set or reset render target
		virtual bool setRenderTarget(video::E_RENDER_TARGET target, bool clearTarget,
					bool clearZBuffer, SColor color);

		//! set or reset render target texture
		virtual bool setRenderTarget(video::ITexture* texture, bool clearBackBuffer,
					bool clearZBuffer, SColor color);

		//! Sets multiple render targets
		virtual bool setRenderTarget(const core::array<video::IRenderTarget>& texture,
			bool clearBackBuffer=true, bool clearZBuffer=true, SColor color=SColor(0,0,0,0));

		//! Clears the ZBuffer.
		virtual void clearZBuffer();

		//! Returns an image created from the last rendered frame.
		virtual IImage* createScreenShot(video::ECOLOR_FORMAT format=video::ECF_UNKNOWN, video::E_RENDER_TARGET target=video::ERT_FRAME_BUFFER);

		//! checks if an OpenGL error has happend and prints it
		//! for performance reasons only available in debug mode
		bool testGLError();

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

		//! Enable the 2d override material
		virtual void enableMaterial2D(bool enable=true);

		//! Returns the graphics card vendor name.
		virtual core::stringc getVendorInfo() {return VendorName;}

		//! Returns the maximum texture size supported.
		virtual core::dimension2du getMaxTextureSize() const;

		ITexture* createDepthTexture(ITexture* texture, bool shared=true);
		void removeDepthTexture(ITexture* texture);

		//! Removes a texture from the texture cache and deletes it, freeing lot of memory.
		void removeTexture(ITexture* texture);

		//! Convert E_PRIMITIVE_TYPE to OpenGL equivalent
		GLenum primitiveTypeToGL(scene::E_PRIMITIVE_TYPE type) const;

		//! Convert E_BLEND_FACTOR to OpenGL equivalent
		GLenum getGLBlend(E_BLEND_FACTOR factor) const;

		//! Get ZBuffer bits.
		GLenum getZBufferBits() const;

		//! Get Cg context
		#ifdef _IRR_COMPILE_WITH_CG_
		const CGcontext& getCgContext();
		#endif

	private:

		//! clears the zbuffer and color buffer
		void clearBuffers(bool backBuffer, bool zBuffer, bool stencilBuffer, SColor color);

		bool updateVertexHardwareBuffer(SHWBufferLink_opengl *HWBuffer);
		bool updateIndexHardwareBuffer(SHWBufferLink_opengl *HWBuffer);

		void uploadClipPlane(u32 index);

		//! inits the parts of the open gl driver used on all platforms
		bool genericDriverInit();
		//! returns a device dependent texture from a software surface (IImage)
		virtual video::ITexture* createDeviceDependentTexture(IImage* surface, const io::path& name, void* mipmapData);

		//! creates a transposed matrix in supplied GLfloat array to pass to OpenGL
		inline void getGLMatrix(GLfloat gl_matrix[16], const core::matrix4& m);
		inline void getGLTextureMatrix(GLfloat gl_matrix[16], const core::matrix4& m);

		//! Set GL pipeline to desired texture wrap modes of the material
		void setWrapMode(const SMaterial& material);

		//! get native wrap mode value
		GLint getTextureWrapMode(const u8 clamp);

		//! sets the needed renderstates
		void setRenderStates3DMode();

		//! sets the needed renderstates
		void setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel);

		// returns the current size of the screen or rendertarget
		virtual const core::dimension2d<u32>& getCurrentRenderTargetSize() const;

		void createMaterialRenderers();

		//! Assign a hardware light to the specified requested light, if any
		//! free hardware lights exist.
		//! \param[in] lightIndex: the index of the requesting light
		void assignHardwareLight(u32 lightIndex);

		//! helper function for render setup.
		void getColorBuffer(const void* vertices, u32 vertexCount, E_VERTEX_TYPE vType);

		//! helper function doing the actual rendering.
		void renderArray(const void* indexList, u32 primitiveCount,
				scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType);

		core::stringw Name;
		core::matrix4 Matrices[ETS_COUNT];
		core::array<u8> ColorBuffer;

		//! enumeration for rendering modes such as 2d and 3d for minizing the switching of renderStates.
		enum E_RENDER_MODE
		{
			ERM_NONE = 0,	// no render state has been set yet.
			ERM_2D,		// 2d drawing rendermode
			ERM_3D		// 3d rendering mode
		};

		E_RENDER_MODE CurrentRenderMode;
		//! bool to make all renderstates reset if set to true.
		bool ResetRenderStates;
		bool Transformation3DChanged;
		u8 AntiAlias;

		SMaterial Material, LastMaterial;
		COpenGLTexture* RenderTargetTexture;
		core::array<video::IRenderTarget> MRTargets;
		class STextureStageCache
		{
			const ITexture* CurrentTexture[MATERIAL_MAX_TEXTURES];
		public:
			STextureStageCache()
			{
				for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
				{
					CurrentTexture[i] = 0;
				}
			}

			~STextureStageCache()
			{
				clear();
			}

			void set(u32 stage, const ITexture* tex)
			{
				if (stage<MATERIAL_MAX_TEXTURES)
				{
					const ITexture* oldTexture=CurrentTexture[stage];
					if (tex)
						tex->grab();
					CurrentTexture[stage]=tex;
					if (oldTexture)
						oldTexture->drop();
				}
			}

			const ITexture* operator[](int stage) const
			{
				if ((u32)stage<MATERIAL_MAX_TEXTURES)
					return CurrentTexture[stage];
				else
					return 0;
			}

			void remove(const ITexture* tex)
			{
				for (s32 i = MATERIAL_MAX_TEXTURES-1; i>= 0; --i)
				{
					if (CurrentTexture[i] == tex)
					{
						tex->drop();
						CurrentTexture[i] = 0;
					}
				}
			}

			void clear()
			{
				// Drop all the CurrentTexture handles
				for (u32 i=0; i<MATERIAL_MAX_TEXTURES; ++i)
				{
					if (CurrentTexture[i])
					{
						CurrentTexture[i]->drop();
						CurrentTexture[i] = 0;
					}
				}
			}
		};
		STextureStageCache CurrentTexture;
		core::array<ITexture*> DepthTextures;
		struct SUserClipPlane
		{
			SUserClipPlane() : Enabled(false) {}
			core::plane3df Plane;
			bool Enabled;
		};
		core::array<SUserClipPlane> UserClipPlanes;

		core::dimension2d<u32> CurrentRendertargetSize;

		core::stringc VendorName;

		core::matrix4 TextureFlipMatrix;

		//! Color buffer format
		ECOLOR_FORMAT ColorFormat;

		//! Render target type for render operations
		E_RENDER_TARGET CurrentTarget;

		SIrrlichtCreationParameters Params;

		//! All the lights that have been requested; a hardware limited
		//! number of them will be used at once.
		struct RequestedLight
		{
			RequestedLight(SLight const & lightData)
				: LightData(lightData), HardwareLightIndex(-1), DesireToBeOn(true) { }

			SLight	LightData;
			s32	HardwareLightIndex; // GL_LIGHT0 - GL_LIGHT7
			bool	DesireToBeOn;
		};
		core::array<RequestedLight> RequestedLights;

		#ifdef _IRR_WINDOWS_API_
			HDC HDc; // Private GDI Device Context
			HWND Window;
		#ifdef _IRR_COMPILE_WITH_WINDOWS_DEVICE_
			CIrrDeviceWin32 *Win32Device;
		#endif
		#endif
		#ifdef _IRR_COMPILE_WITH_X11_DEVICE_
			GLXDrawable Drawable;
			Display* X11Display;
			CIrrDeviceLinux *X11Device;
		#endif
		#ifdef _IRR_COMPILE_WITH_OSX_DEVICE_
			CIrrDeviceMacOSX *OSXDevice;
		#endif
		#ifdef _IRR_COMPILE_WITH_SDL_DEVICE_
			CIrrDeviceSDL *SDLDevice;
		#endif
		#ifdef _IRR_COMPILE_WITH_CG_
		CGcontext CgContext;
		#endif

		E_DEVICE_TYPE DeviceType;
	};

} // end namespace video
} // end namespace irr


#endif // _IRR_COMPILE_WITH_OPENGL_
#endif

