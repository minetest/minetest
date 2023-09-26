// Copyright (C) 2017 Michael Zeilfelder
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#pragma once

#include "SIrrCreationParameters.h"

#ifdef _IRR_COMPILE_WITH_WEBGL1_

#include "COGLES2Driver.h"
#include "CWebGLExtensionHandler.h"
#include "CMeshBuffer.h"
#include "EHardwareBufferFlags.h"

namespace irr
{
namespace video
{
	//! WebGL friendly subset of OGL ES 2.0.
	//! Written for use with emscripten
	class CWebGL1Driver : public COGLES2Driver
	{
		friend class COpenGLCoreTexture<CWebGL1Driver>;
		friend IVideoDriver* createWebGL1Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);

	protected:
		//! constructor
		CWebGL1Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager);

	public:

		//! destructor
		virtual ~CWebGL1Driver();

		//! Returns type of video driver
		E_DRIVER_TYPE getDriverType() const override;

		//! Is VBO recommended on this mesh?
		bool isHardwareBufferRecommend(const scene::IMeshBuffer* mb) override
		{
			// All buffers must be bound, WebGL doesn't allow sending unbound buffers at all.
			return true;
		}

		//! draws a vertex primitive list
		virtual void drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
				const void* indexList, u32 primitiveCount,
				E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType) override;

		//! Draws a mesh buffer
		void drawMeshBuffer(const scene::IMeshBuffer* mb) override;

		virtual void draw2DImage(const video::ITexture* texture,
				const core::position2d<s32>& destPos,
				const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
				SColor color = SColor(255, 255, 255, 255), bool useAlphaChannelOfTexture = false) override;

		virtual void draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
			const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect = 0,
			const video::SColor* const colors = 0, bool useAlphaChannelOfTexture = false) override;

		// internally used
		void draw2DImage(const video::ITexture* texture, u32 layer, bool flip)  override;

		//! draws a set of 2d images
		virtual void draw2DImageBatch(const video::ITexture* texture,
				const core::position2d<s32>& pos,
				const core::array<core::rect<s32> >& sourceRects,
				const core::array<s32>& indices, s32 kerningWidth = 0,
				const core::rect<s32>* clipRect = 0,
				SColor color = SColor(255, 255, 255, 255),
				bool useAlphaChannelOfTexture = false) override;

		void draw2DImageBatch(const video::ITexture* texture,
				const core::array<core::position2d<s32> >& positions,
				const core::array<core::rect<s32> >& sourceRects,
				const core::rect<s32>* clipRect,
				SColor color,
				bool useAlphaChannelOfTexture) override;

		//! draw an 2d rectangle
		virtual void draw2DRectangle(SColor color, const core::rect<s32>& pos,
				const core::rect<s32>* clip = 0) override;

		//!Draws an 2d rectangle with a gradient.
		virtual void draw2DRectangle(const core::rect<s32>& pos,
				SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
				const core::rect<s32>* clip = 0) override;

		//! Draws a 2d line.
		virtual void draw2DLine(const core::position2d<s32>& start,
				const core::position2d<s32>& end,
				SColor color = SColor(255, 255, 255, 255)) override;

		//! Draws a single pixel
		void drawPixel(u32 x, u32 y, const SColor & color) override;

		//! Draws a 3d line.
		virtual void draw3DLine(const core::vector3df& start,
				const core::vector3df& end,
				SColor color = SColor(255, 255, 255, 255)) override;

		//! Draws a shadow volume into the stencil buffer.
		void drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible=0) override;

		//! Fills the stencil shadow with color.
		virtual void drawStencilShadow(bool clearStencilBuffer=false,
			video::SColor leftUpEdge = video::SColor(0,0,0,0),
			video::SColor rightUpEdge = video::SColor(0,0,0,0),
			video::SColor leftDownEdge = video::SColor(0,0,0,0),
			video::SColor rightDownEdge = video::SColor(0,0,0,0)) override;

		//! Get ZBuffer bits.
		GLenum getZBufferBits() const override;

		virtual bool getColorFormatParameters(ECOLOR_FORMAT format, GLint& internalFormat, GLenum& pixelFormat,
			GLenum& pixelType, void(**converter)(const void*, s32, void*)) const override;

	protected:
		// create a meshbuffer which has as many vertices as indices
		scene::SMeshBuffer* createSimpleMeshBuffer(irr::u32 numVertices, scene::E_PRIMITIVE_TYPE primitiveType, scene::E_HARDWARE_MAPPING vertexMappingHint=scene::EHM_STREAM, scene::E_HARDWARE_MAPPING indexMappingHint=scene::EHM_STATIC) const;

		bool genericDriverInit(const core::dimension2d<u32>& screenSize, bool stencilBuffer) override;
		void initWebGLExtensions();

	private:
		// CWebGL1Driver is derived from COGLES2Driver so it already got an extension handler from that.
		// But we shouldn't use other extensions most of the time as there are minor differences.
		CWebGLExtensionHandler WebGLExtensions;

		// Because we can't have unbound buffers in webgl we give drawing functions bound buffers to use
		scene::SMeshBuffer* MBTriangleFanSize4;
		scene::SMeshBuffer* MBLinesSize2;
		scene::SMeshBuffer* MBPointsSize1;
	};

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_WEBGL1_
