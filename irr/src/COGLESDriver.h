// Copyright (C) 2002-20014 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#pragma once

#include "SIrrCreationParameters.h"

#ifdef _IRR_COMPILE_WITH_OGLES1_

#include "CNullDriver.h"
#include "IMaterialRendererServices.h"
#include "EDriverFeatures.h"
#include "fast_atof.h"
#include "COGLESExtensionHandler.h"
#include "IContextManager.h"

#define TEST_GL_ERROR(cls) (cls)->testGLError(__LINE__)

namespace irr
{
namespace video
{

class COGLES1Driver : public CNullDriver, public IMaterialRendererServices, public COGLES1ExtensionHandler
{
	friend class COpenGLCoreTexture<COGLES1Driver>;

public:
	//! constructor
	COGLES1Driver(const SIrrlichtCreationParameters &params, io::IFileSystem *io, IContextManager *contextManager);

	//! destructor
	virtual ~COGLES1Driver();

	virtual bool beginScene(u16 clearFlag, SColor clearColor = SColor(255, 0, 0, 0), f32 clearDepth = 1.f, u8 clearStencil = 0,
			const SExposedVideoData &videoData = SExposedVideoData(), core::rect<s32> *sourceRect = 0) override;

	bool endScene() override;

	//! sets transformation
	void setTransform(E_TRANSFORMATION_STATE state, const core::matrix4 &mat) override;

	struct SHWBufferLink_opengl : public SHWBufferLink
	{
		SHWBufferLink_opengl(const scene::IMeshBuffer *_MeshBuffer) :
				SHWBufferLink(_MeshBuffer), vbo_verticesID(0), vbo_indicesID(0) {}

		GLuint vbo_verticesID; // tmp
		GLuint vbo_indicesID;  // tmp

		GLuint vbo_verticesSize; // tmp
		GLuint vbo_indicesSize;  // tmp
	};

	bool updateVertexHardwareBuffer(SHWBufferLink_opengl *HWBuffer);
	bool updateIndexHardwareBuffer(SHWBufferLink_opengl *HWBuffer);

	//! updates hardware buffer if needed
	bool updateHardwareBuffer(SHWBufferLink *HWBuffer) override;

	//! Create hardware buffer from mesh
	SHWBufferLink *createHardwareBuffer(const scene::IMeshBuffer *mb) override;

	//! Delete hardware buffer (only some drivers can)
	void deleteHardwareBuffer(SHWBufferLink *HWBuffer) override;

	//! Draw hardware buffer
	void drawHardwareBuffer(SHWBufferLink *HWBuffer) override;

	IRenderTarget *addRenderTarget() override;

	//! draws a vertex primitive list
	virtual void drawVertexPrimitiveList(const void *vertices, u32 vertexCount,
			const void *indexList, u32 primitiveCount,
			E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType) override;

	void drawVertexPrimitiveList2d3d(const void *vertices, u32 vertexCount, const void *indexList, u32 primitiveCount, E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType = EIT_16BIT, bool threed = true);

	//! queries the features of the driver, returns true if feature is available
	bool queryFeature(E_VIDEO_DRIVER_FEATURE feature) const override
	{
		//			return FeatureEnabled[feature] && COGLES1ExtensionHandler::queryFeature(feature);
		return COGLES1ExtensionHandler::queryFeature(feature);
	}

	//! Sets a material.
	void setMaterial(const SMaterial &material) override;

	virtual void draw2DImage(const video::ITexture *texture, const core::position2d<s32> &destPos,
			const core::rect<s32> &sourceRect, const core::rect<s32> *clipRect = 0,
			SColor color = SColor(255, 255, 255, 255), bool useAlphaChannelOfTexture = false) override;

	virtual void draw2DImage(const video::ITexture *texture, const core::rect<s32> &destRect,
			const core::rect<s32> &sourceRect, const core::rect<s32> *clipRect = 0,
			const video::SColor *const colors = 0, bool useAlphaChannelOfTexture = false) override;

	virtual void draw2DImage(const video::ITexture *texture, u32 layer, bool flip);

	//! draws a set of 2d images, using a color and the alpha channel of the texture if desired.
	virtual void draw2DImageBatch(const video::ITexture *texture,
			const core::array<core::position2d<s32>> &positions,
			const core::array<core::rect<s32>> &sourceRects,
			const core::rect<s32> *clipRect = 0,
			SColor color = SColor(255, 255, 255, 255),
			bool useAlphaChannelOfTexture = false) override;

	//! draw an 2d rectangle
	virtual void draw2DRectangle(SColor color, const core::rect<s32> &pos,
			const core::rect<s32> *clip = 0) override;

	//! Draws an 2d rectangle with a gradient.
	virtual void draw2DRectangle(const core::rect<s32> &pos,
			SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
			const core::rect<s32> *clip = 0) override;

	//! Draws a 2d line.
	virtual void draw2DLine(const core::position2d<s32> &start,
			const core::position2d<s32> &end,
			SColor color = SColor(255, 255, 255, 255)) override;

	//! Draws a 3d line.
	virtual void draw3DLine(const core::vector3df &start,
			const core::vector3df &end,
			SColor color = SColor(255, 255, 255, 255)) override;

	//! Returns the name of the video driver.
	const char *getName() const override;

	//! Sets the dynamic ambient light color.
	void setAmbientLight(const SColorf &color) override;

	//! sets a viewport
	void setViewPort(const core::rect<s32> &area) override;

	//! Sets the fog mode.
	virtual void setFog(SColor color, E_FOG_TYPE fogType, f32 start,
			f32 end, f32 density, bool pixelFog, bool rangeFog) override;

	//! Only used internally by the engine
	void OnResize(const core::dimension2d<u32> &size) override;

	//! Returns type of video driver
	E_DRIVER_TYPE getDriverType() const override;

	//! get color format of the current color buffer
	ECOLOR_FORMAT getColorFormat() const override;

	//! Returns the transformation set by setTransform
	const core::matrix4 &getTransform(E_TRANSFORMATION_STATE state) const override;

	//! Can be called by an IMaterialRenderer to make its work easier.
	virtual void setBasicRenderStates(const SMaterial &material, const SMaterial &lastmaterial,
			bool resetAllRenderstates) override;

	//! Compare in SMaterial doesn't check texture parameters, so we should call this on each OnRender call.
	virtual void setTextureRenderStates(const SMaterial &material, bool resetAllRenderstates);

	//! Get a vertex shader constant index.
	s32 getVertexShaderConstantID(const c8 *name) override;

	//! Get a pixel shader constant index.
	s32 getPixelShaderConstantID(const c8 *name) override;

	//! Sets a constant for the vertex shader based on an index.
	bool setVertexShaderConstant(s32 index, const f32 *floats, int count) override;

	//! Int interface for the above.
	bool setVertexShaderConstant(s32 index, const s32 *ints, int count) override;

	//! Uint interface for the above.
	bool setVertexShaderConstant(s32 index, const u32 *ints, int count) override;

	//! Sets a constant for the pixel shader based on an index.
	bool setPixelShaderConstant(s32 index, const f32 *floats, int count) override;

	//! Int interface for the above.
	bool setPixelShaderConstant(s32 index, const s32 *ints, int count) override;

	//! Uint interface for the above.
	bool setPixelShaderConstant(s32 index, const u32 *ints, int count) override;

	//! Adds a new material renderer to the VideoDriver
	virtual s32 addHighLevelShaderMaterial(const c8 *vertexShaderProgram, const c8 *vertexShaderEntryPointName,
			E_VERTEX_SHADER_TYPE vsCompileTarget, const c8 *pixelShaderProgram, const c8 *pixelShaderEntryPointName,
			E_PIXEL_SHADER_TYPE psCompileTarget, const c8 *geometryShaderProgram, const c8 *geometryShaderEntryPointName,
			E_GEOMETRY_SHADER_TYPE gsCompileTarget, scene::E_PRIMITIVE_TYPE inType, scene::E_PRIMITIVE_TYPE outType,
			u32 verticesOut, IShaderConstantSetCallBack *callback, E_MATERIAL_TYPE baseMaterial,
			s32 userData) override;

	//! Returns pointer to the IGPUProgrammingServices interface.
	IGPUProgrammingServices *getGPUProgrammingServices() override;

	//! Returns a pointer to the IVideoDriver interface.
	IVideoDriver *getVideoDriver() override;

	//! Returns the maximum amount of primitives
	u32 getMaximalPrimitiveCount() const override;

	virtual ITexture *addRenderTargetTexture(const core::dimension2d<u32> &size,
			const io::path &name, const ECOLOR_FORMAT format = ECF_UNKNOWN) override;

	//! Creates a render target texture for a cubemap
	ITexture *addRenderTargetTextureCubemap(const irr::u32 sideLen,
			const io::path &name, const ECOLOR_FORMAT format) override;

	virtual bool setRenderTargetEx(IRenderTarget *target, u16 clearFlag, SColor clearColor = SColor(255, 0, 0, 0),
			f32 clearDepth = 1.f, u8 clearStencil = 0) override;

	void clearBuffers(u16 flag, SColor color = SColor(255, 0, 0, 0), f32 depth = 1.f, u8 stencil = 0) override;

	//! Returns an image created from the last rendered frame.
	IImage *createScreenShot(video::ECOLOR_FORMAT format = video::ECF_UNKNOWN, video::E_RENDER_TARGET target = video::ERT_FRAME_BUFFER) override;

	//! checks if an OpenGL error has happened and prints it (+ some internal code which is usually the line number)
	bool testGLError(int code = 0);

	//! Set/unset a clipping plane.
	bool setClipPlane(u32 index, const core::plane3df &plane, bool enable = false) override;

	//! Enable/disable a clipping plane.
	void enableClipPlane(u32 index, bool enable) override;

	//! Returns the graphics card vendor name.
	core::stringc getVendorInfo() override
	{
		return VendorName;
	}

	//! Get the maximal texture size for this driver
	core::dimension2du getMaxTextureSize() const override;

	void removeTexture(ITexture *texture) override;

	//! Check if the driver supports creating textures with the given color format
	bool queryTextureFormat(ECOLOR_FORMAT format) const override;

	//! Used by some SceneNodes to check if a material should be rendered in the transparent render pass
	bool needsTransparentRenderPass(const irr::video::SMaterial &material) const override;

	//! Convert E_BLEND_FACTOR to OpenGL equivalent
	GLenum getGLBlend(E_BLEND_FACTOR factor) const;

	//! Get ZBuffer bits.
	GLenum getZBufferBits() const;

	bool getColorFormatParameters(ECOLOR_FORMAT format, GLint &internalFormat, GLenum &pixelFormat,
			GLenum &pixelType, void (**converter)(const void *, s32, void *)) const;

	COGLES1CacheHandler *getCacheHandler() const;

private:
	void uploadClipPlane(u32 index);

	//! inits the opengl-es driver
	bool genericDriverInit(const core::dimension2d<u32> &screenSize, bool stencilBuffer);

	ITexture *createDeviceDependentTexture(const io::path &name, IImage *image) override;

	ITexture *createDeviceDependentTextureCubemap(const io::path &name, const core::array<IImage *> &image) override;

	//! creates a transposed matrix in supplied GLfloat array to pass to OGLES1
	inline void getGLMatrix(GLfloat gl_matrix[16], const core::matrix4 &m);
	inline void getGLTextureMatrix(GLfloat gl_matrix[16], const core::matrix4 &m);

	//! Set GL pipeline to desired texture wrap modes of the material
	void setWrapMode(const SMaterial &material);

	//! Get OpenGL wrap enum from Irrlicht enum
	GLint getTextureWrapMode(u8 clamp) const;

	//! sets the needed renderstates
	void setRenderStates3DMode();

	//! sets the needed renderstates
	void setRenderStates2DMode(bool alpha, bool texture, bool alphaChannel);

	void createMaterialRenderers();

	//! Assign a hardware light to the specified requested light, if any
	//! free hardware lights exist.
	//! \param[in] lightIndex: the index of the requesting light
	void assignHardwareLight(u32 lightIndex);

	//! Same as `CacheHandler->setViewport`, but also sets `ViewPort`
	virtual void setViewPortRaw(u32 width, u32 height);

	COGLES1CacheHandler *CacheHandler;

	core::stringc Name;
	core::matrix4 Matrices[ETS_COUNT];
	core::array<u8> ColorBuffer;

	//! enumeration for rendering modes such as 2d and 3d for minimizing the switching of renderStates.
	enum E_RENDER_MODE
	{
		ERM_NONE = 0, // no render state has been set yet.
		ERM_2D,       // 2d drawing rendermode
		ERM_3D        // 3d rendering mode
	};

	E_RENDER_MODE CurrentRenderMode;
	//! bool to make all renderstates reset if set to true.
	bool ResetRenderStates;
	bool Transformation3DChanged;
	u8 AntiAlias;

	SMaterial Material, LastMaterial;
	core::array<core::plane3df> UserClipPlane;
	std::vector<bool> UserClipPlaneEnabled;

	core::stringc VendorName;

	core::matrix4 TextureFlipMatrix;

	//! Color buffer format
	ECOLOR_FORMAT ColorFormat;

	SIrrlichtCreationParameters Params;

	IContextManager *ContextManager;
};

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_OGLES1_
