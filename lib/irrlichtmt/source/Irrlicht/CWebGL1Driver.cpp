// Copyright (C) 2017 Michael Zeilfelder
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in Irrlicht.h

#include "CWebGL1Driver.h"

#ifdef _IRR_COMPILE_WITH_WEBGL1_

#include "COpenGLCoreTexture.h"
#include "COpenGLCoreRenderTarget.h"
#include "COpenGLCoreCacheHandler.h"
#include "EVertexAttributes.h"

namespace irr
{
namespace video
{

CWebGL1Driver::CWebGL1Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager) :
	COGLES2Driver(params, io, contextManager)
	, MBTriangleFanSize4(0), MBLinesSize2(0), MBPointsSize1(0)
{
#ifdef _DEBUG
	setDebugName("CWebGL1Driver");
#endif

	// NPOT are not allowed for WebGL in most cases.
	// One can use them when:
	// - The TEXTURE_MIN_FILTER is linear or nearest
	// - no mipmapping is used
	// - no texture wrapping is used (so all texture_wraps have to be CLAMP_TO_EDGE)
	// So users could still enable them for specific cases (usually GUI), but in general better to have it off.
	disableFeature(EVDF_TEXTURE_NPOT);

	MBLinesSize2 = createSimpleMeshBuffer(2, scene::EPT_LINES);
	MBTriangleFanSize4 = createSimpleMeshBuffer(4, scene::EPT_TRIANGLE_FAN);
	MBPointsSize1 = createSimpleMeshBuffer(1, scene::EPT_POINTS);
}

CWebGL1Driver::~CWebGL1Driver()
{
	if ( MBTriangleFanSize4 )
		MBTriangleFanSize4->drop();
	if ( MBLinesSize2 )
		MBLinesSize2->drop();
	if ( MBPointsSize1 )
		MBPointsSize1->drop();
}

//! Returns type of video driver
E_DRIVER_TYPE CWebGL1Driver::getDriverType() const
{
	return EDT_WEBGL1;
}

//! draws a vertex primitive list
void CWebGL1Driver::drawVertexPrimitiveList(const void* vertices, u32 vertexCount,
                             const void* indexList, u32 primitiveCount,
                             E_VERTEX_TYPE vType, scene::E_PRIMITIVE_TYPE pType, E_INDEX_TYPE iType)
{
	if ( !vertices )
	{
		COGLES2Driver::drawVertexPrimitiveList(vertices, vertexCount, indexList, primitiveCount, vType, pType, iType);
	}
	else
	{
		static bool first = true;
		if ( first )
		{
			first = false;
			os::Printer::log("WebGL driver does not support drawVertexPrimitiveList calls without a VBO", ELL_WARNING);
			os::Printer::log(__FILE__, irr::core::stringc(__LINE__).c_str(), ELL_WARNING);
		}
	}
}

//! Draws a mesh buffer
void CWebGL1Driver::drawMeshBuffer(const scene::IMeshBuffer* mb)
{
	if ( mb )
	{
		// OK - this is bad and I hope I can find a better solution.
		// Basically casting away a const which shouldn't be cast away.
		// Not a nice surprise for users to see their mesh changes I guess :-(
		scene::IMeshBuffer* mbUglyHack = const_cast<scene::IMeshBuffer*>(mb);

		// We can't allow any buffers which are not bound to some VBO.
		if ( mb->getHardwareMappingHint_Vertex() == scene::EHM_NEVER)
			mbUglyHack->setHardwareMappingHint(scene::EHM_STREAM, scene::EBT_VERTEX);
		if ( mb->getHardwareMappingHint_Index() == scene::EHM_NEVER)
			mbUglyHack->setHardwareMappingHint(scene::EHM_STREAM, scene::EBT_INDEX);

		COGLES2Driver::drawMeshBuffer(mb);
	}
}

void CWebGL1Driver::draw2DImage(const video::ITexture* texture,
		const core::position2d<s32>& destPos,const core::rect<s32>& sourceRect,
		const core::rect<s32>* clipRect, SColor color, bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	if (!sourceRect.isValid())
		return;

	core::position2d<s32> targetPos(destPos);
	core::position2d<s32> sourcePos(sourceRect.UpperLeftCorner);
	core::dimension2d<s32> sourceSize(sourceRect.getSize());
	if (clipRect)
	{
		if (targetPos.X < clipRect->UpperLeftCorner.X)
		{
			sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
			if (sourceSize.Width <= 0)
				return;

			sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
			targetPos.X = clipRect->UpperLeftCorner.X;
		}

		if (targetPos.X + sourceSize.Width > clipRect->LowerRightCorner.X)
		{
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
			if (sourceSize.Width <= 0)
				return;
		}

		if (targetPos.Y < clipRect->UpperLeftCorner.Y)
		{
			sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
			if (sourceSize.Height <= 0)
				return;

			sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
			targetPos.Y = clipRect->UpperLeftCorner.Y;
		}

		if (targetPos.Y + sourceSize.Height > clipRect->LowerRightCorner.Y)
		{
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
			if (sourceSize.Height <= 0)
				return;
		}
	}

	// clip these coordinates

	if (targetPos.X < 0)
	{
		sourceSize.Width += targetPos.X;
		if (sourceSize.Width <= 0)
			return;

		sourcePos.X -= targetPos.X;
		targetPos.X = 0;
	}

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	if (targetPos.X + sourceSize.Width > (s32)renderTargetSize.Width)
	{
		sourceSize.Width -= (targetPos.X + sourceSize.Width) - renderTargetSize.Width;
		if (sourceSize.Width <= 0)
			return;
	}

	if (targetPos.Y < 0)
	{
		sourceSize.Height += targetPos.Y;
		if (sourceSize.Height <= 0)
			return;

		sourcePos.Y -= targetPos.Y;
		targetPos.Y = 0;
	}

	if (targetPos.Y + sourceSize.Height > (s32)renderTargetSize.Height)
	{
		sourceSize.Height -= (targetPos.Y + sourceSize.Height) - renderTargetSize.Height;
		if (sourceSize.Height <= 0)
			return;
	}

	// ok, we've clipped everything.
	// now draw it.

	// texcoords need to be flipped horizontally for RTTs
	const bool isRTT = texture->isRenderTarget();
	const core::dimension2d<u32>& ss = texture->getOriginalSize();
	const f32 invW = 1.f / static_cast<f32>(ss.Width);
	const f32 invH = 1.f / static_cast<f32>(ss.Height);
	const core::rect<f32> tcoords(
		sourcePos.X * invW,
		(isRTT ? (sourcePos.Y + sourceSize.Height) : sourcePos.Y) * invH,
		(sourcePos.X + sourceSize.Width) * invW,
		(isRTT ? sourcePos.Y : (sourcePos.Y + sourceSize.Height)) * invH);

	const core::rect<s32> poss(targetPos, sourceSize);

	chooseMaterial2D();
	if ( !setMaterialTexture(0, texture) )
		return;

	setRenderStates2DMode(color.getAlpha() < 255, true, useAlphaChannelOfTexture);
	lockRenderStateMode();

	f32 left = (f32)poss.UpperLeftCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 right = (f32)poss.LowerRightCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 down = 2.f - (f32)poss.LowerRightCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
	f32 top = 2.f - (f32)poss.UpperLeftCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

	MBTriangleFanSize4->Vertices[0] = S3DVertex(left, top, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	MBTriangleFanSize4->Vertices[1] = S3DVertex(right, top, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	MBTriangleFanSize4->Vertices[2] = S3DVertex(right, down, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	MBTriangleFanSize4->Vertices[3] = S3DVertex(left, down, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
	MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBTriangleFanSize4);

	unlockRenderStateMode();
}

void CWebGL1Driver::draw2DImage(const video::ITexture* texture, const core::rect<s32>& destRect,
	const core::rect<s32>& sourceRect, const core::rect<s32>* clipRect,
	const video::SColor* const colors, bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	// texcoords need to be flipped horizontally for RTTs
	const bool isRTT = texture->isRenderTarget();
	const core::dimension2du& ss = texture->getOriginalSize();
	const f32 invW = 1.f / static_cast<f32>(ss.Width);
	const f32 invH = 1.f / static_cast<f32>(ss.Height);
	const core::rect<f32> tcoords(
		sourceRect.UpperLeftCorner.X * invW,
		(isRTT ? sourceRect.LowerRightCorner.Y : sourceRect.UpperLeftCorner.Y) * invH,
		sourceRect.LowerRightCorner.X * invW,
		(isRTT ? sourceRect.UpperLeftCorner.Y : sourceRect.LowerRightCorner.Y) *invH);

	const video::SColor temp[4] =
	{
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF,
		0xFFFFFFFF
	};

	const video::SColor* const useColor = colors ? colors : temp;

	chooseMaterial2D();
	if ( !setMaterialTexture(0, texture) )
		return;

	setRenderStates2DMode(useColor[0].getAlpha() < 255 || useColor[1].getAlpha() < 255 ||
		useColor[2].getAlpha() < 255 || useColor[3].getAlpha() < 255,
		true, useAlphaChannelOfTexture);
	lockRenderStateMode();

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	bool useScissorTest = false;
	if (clipRect && clipRect->isValid())
	{
		useScissorTest = true;
		glEnable(GL_SCISSOR_TEST);
		glScissor(clipRect->UpperLeftCorner.X, renderTargetSize.Height - clipRect->LowerRightCorner.Y,
			clipRect->getWidth(), clipRect->getHeight());
	}

	f32 left = (f32)destRect.UpperLeftCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 right = (f32)destRect.LowerRightCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 down = 2.f - (f32)destRect.LowerRightCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
	f32 top = 2.f - (f32)destRect.UpperLeftCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

	MBTriangleFanSize4->Vertices[0] = S3DVertex(left, top, 0, 0, 0, 1, useColor[0], tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
	MBTriangleFanSize4->Vertices[1] = S3DVertex(right, top, 0, 0, 0, 1, useColor[3], tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
	MBTriangleFanSize4->Vertices[2] = S3DVertex(right, down, 0, 0, 0, 1, useColor[2], tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
	MBTriangleFanSize4->Vertices[3] = S3DVertex(left, down, 0, 0, 0, 1, useColor[1], tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
	MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBTriangleFanSize4);

	if (useScissorTest)
		glDisable(GL_SCISSOR_TEST);

	unlockRenderStateMode();

	testGLError();
}

void CWebGL1Driver::draw2DImage(const video::ITexture* texture, u32 layer, bool flip)
{
	if (!texture )
		return;

	chooseMaterial2D();
	if ( !setMaterialTexture(0, texture) )
		return;

	setRenderStates2DMode(false, true, true);
	lockRenderStateMode();

	MBTriangleFanSize4->Vertices[0].Pos = core::vector3df(-1.f, 1.f, 0.f);
	MBTriangleFanSize4->Vertices[1].Pos = core::vector3df(1.f, 1.f, 0.f);
	MBTriangleFanSize4->Vertices[2].Pos = core::vector3df(1.f, -1.f, 0.f);
	MBTriangleFanSize4->Vertices[3].Pos = core::vector3df(-1.f, -1.f, 0.f);

	f32 modificator = (flip) ? 1.f : 0.f;

	MBTriangleFanSize4->Vertices[0].TCoords = core::vector2df(0.f, 0.f + modificator);
	MBTriangleFanSize4->Vertices[1].TCoords = core::vector2df(1.f, 0.f + modificator);
	MBTriangleFanSize4->Vertices[2].TCoords = core::vector2df(1.f, 1.f - modificator);
	MBTriangleFanSize4->Vertices[3].TCoords = core::vector2df(0.f, 1.f - modificator);

	MBTriangleFanSize4->Vertices[0].Color = SColor(0xFFFFFFFF);
	MBTriangleFanSize4->Vertices[1].Color = SColor(0xFFFFFFFF);
	MBTriangleFanSize4->Vertices[2].Color = SColor(0xFFFFFFFF);
	MBTriangleFanSize4->Vertices[3].Color = SColor(0xFFFFFFFF);

	MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBTriangleFanSize4);

	unlockRenderStateMode();
}

void CWebGL1Driver::draw2DImageBatch(const video::ITexture* texture,
				const core::position2d<s32>& pos,
				const core::array<core::rect<s32> >& sourceRects,
				const core::array<s32>& indices, s32 kerningWidth,
				const core::rect<s32>* clipRect,
				SColor color, bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	chooseMaterial2D();
	if ( !setMaterialTexture(0, texture) )
		return;

	setRenderStates2DMode(color.getAlpha() < 255, true, useAlphaChannelOfTexture);
	lockRenderStateMode();

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	bool useScissorTest = false;
	if (clipRect && clipRect->isValid())
	{
		useScissorTest = true;
		glEnable(GL_SCISSOR_TEST);
		glScissor(clipRect->UpperLeftCorner.X, renderTargetSize.Height - clipRect->LowerRightCorner.Y,
				clipRect->getWidth(), clipRect->getHeight());
	}

	const core::dimension2du& ss = texture->getOriginalSize();
	core::position2d<s32> targetPos(pos);
	// texcoords need to be flipped horizontally for RTTs
	const bool isRTT = texture->isRenderTarget();
	const f32 invW = 1.f / static_cast<f32>(ss.Width);
	const f32 invH = 1.f / static_cast<f32>(ss.Height);

	for (u32 i = 0; i < indices.size(); ++i)
	{
		const s32 currentIndex = indices[i];
		if (!sourceRects[currentIndex].isValid())
			break;

		const core::rect<f32> tcoords(
			sourceRects[currentIndex].UpperLeftCorner.X * invW,
			(isRTT ? sourceRects[currentIndex].LowerRightCorner.Y : sourceRects[currentIndex].UpperLeftCorner.Y) * invH,
			sourceRects[currentIndex].LowerRightCorner.X * invW,
			(isRTT ? sourceRects[currentIndex].UpperLeftCorner.Y : sourceRects[currentIndex].LowerRightCorner.Y) * invH);

		const core::rect<s32> poss(targetPos, sourceRects[currentIndex].getSize());

		f32 left = (f32)poss.UpperLeftCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
		f32 right = (f32)poss.LowerRightCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
		f32 down = 2.f - (f32)poss.LowerRightCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
		f32 top = 2.f - (f32)poss.UpperLeftCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

		MBTriangleFanSize4->Vertices[0] = S3DVertex(left, top, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
		MBTriangleFanSize4->Vertices[1] = S3DVertex(right, top, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
		MBTriangleFanSize4->Vertices[2] = S3DVertex(right, down, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
		MBTriangleFanSize4->Vertices[3] = S3DVertex(left, down, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
		MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

		drawMeshBuffer(MBTriangleFanSize4);

		targetPos.X += sourceRects[currentIndex].getWidth();
	}

	if (useScissorTest)
		glDisable(GL_SCISSOR_TEST);

	unlockRenderStateMode();

	testGLError();
}

void CWebGL1Driver::draw2DImageBatch(const video::ITexture* texture,
		const core::array<core::position2d<s32> >& positions,
		const core::array<core::rect<s32> >& sourceRects,
		const core::rect<s32>* clipRect,
		SColor color, bool useAlphaChannelOfTexture)
{
	if (!texture)
		return;

	const irr::u32 drawCount = core::min_<u32>(positions.size(), sourceRects.size());
	if ( !drawCount )
		return;

	chooseMaterial2D();
	if ( !setMaterialTexture(0, texture) )
		return;

	setRenderStates2DMode(color.getAlpha() < 255, true, useAlphaChannelOfTexture);
	lockRenderStateMode();

	for (u32 i = 0; i < drawCount; i++)
	{
		core::position2d<s32> targetPos = positions[i];
		core::position2d<s32> sourcePos = sourceRects[i].UpperLeftCorner;
		// This needs to be signed as it may go negative.
		core::dimension2d<s32> sourceSize(sourceRects[i].getSize());

		if (clipRect)
		{
			if (targetPos.X < clipRect->UpperLeftCorner.X)
			{
				sourceSize.Width += targetPos.X - clipRect->UpperLeftCorner.X;
				if (sourceSize.Width <= 0)
					continue;

				sourcePos.X -= targetPos.X - clipRect->UpperLeftCorner.X;
				targetPos.X = clipRect->UpperLeftCorner.X;
			}

			if (targetPos.X + (s32)sourceSize.Width > clipRect->LowerRightCorner.X)
			{
				sourceSize.Width -= (targetPos.X + sourceSize.Width) - clipRect->LowerRightCorner.X;
				if (sourceSize.Width <= 0)
					continue;
			}

			if (targetPos.Y < clipRect->UpperLeftCorner.Y)
			{
				sourceSize.Height += targetPos.Y - clipRect->UpperLeftCorner.Y;
				if (sourceSize.Height <= 0)
					continue;

				sourcePos.Y -= targetPos.Y - clipRect->UpperLeftCorner.Y;
				targetPos.Y = clipRect->UpperLeftCorner.Y;
			}

			if (targetPos.Y + (s32)sourceSize.Height > clipRect->LowerRightCorner.Y)
			{
				sourceSize.Height -= (targetPos.Y + sourceSize.Height) - clipRect->LowerRightCorner.Y;
				if (sourceSize.Height <= 0)
					continue;
			}
		}

		// clip these coordinates

		if (targetPos.X < 0)
		{
			sourceSize.Width += targetPos.X;
			if (sourceSize.Width <= 0)
				continue;

			sourcePos.X -= targetPos.X;
			targetPos.X = 0;
		}

		const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

		if (targetPos.X + sourceSize.Width > (s32)renderTargetSize.Width)
		{
			sourceSize.Width -= (targetPos.X + sourceSize.Width) - renderTargetSize.Width;
			if (sourceSize.Width <= 0)
				continue;
		}

		if (targetPos.Y < 0)
		{
			sourceSize.Height += targetPos.Y;
			if (sourceSize.Height <= 0)
				continue;

			sourcePos.Y -= targetPos.Y;
			targetPos.Y = 0;
		}

		if (targetPos.Y + sourceSize.Height > (s32)renderTargetSize.Height)
		{
			sourceSize.Height -= (targetPos.Y + sourceSize.Height) - renderTargetSize.Height;
			if (sourceSize.Height <= 0)
				continue;
		}

		// ok, we've clipped everything.
		// now draw it.

		core::rect<f32> tcoords;
		tcoords.UpperLeftCorner.X = (((f32)sourcePos.X)) / texture->getOriginalSize().Width ;
		tcoords.UpperLeftCorner.Y = (((f32)sourcePos.Y)) / texture->getOriginalSize().Height;
		tcoords.LowerRightCorner.X = tcoords.UpperLeftCorner.X + ((f32)(sourceSize.Width) / texture->getOriginalSize().Width);
		tcoords.LowerRightCorner.Y = tcoords.UpperLeftCorner.Y + ((f32)(sourceSize.Height) / texture->getOriginalSize().Height);

		const core::rect<s32> poss(targetPos, sourceSize);

		f32 left = (f32)poss.UpperLeftCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
		f32 right = (f32)poss.LowerRightCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
		f32 down = 2.f - (f32)poss.LowerRightCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
		f32 top = 2.f - (f32)poss.UpperLeftCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

		MBTriangleFanSize4->Vertices[0] = S3DVertex(left, top, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.UpperLeftCorner.Y);
		MBTriangleFanSize4->Vertices[1] = S3DVertex(right, top, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.UpperLeftCorner.Y);
		MBTriangleFanSize4->Vertices[2] = S3DVertex(right, down, 0, 0, 0, 1, color, tcoords.LowerRightCorner.X, tcoords.LowerRightCorner.Y);
		MBTriangleFanSize4->Vertices[3] = S3DVertex(left, down, 0, 0, 0, 1, color, tcoords.UpperLeftCorner.X, tcoords.LowerRightCorner.Y);
		MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

		drawMeshBuffer(MBTriangleFanSize4);
	}

	unlockRenderStateMode();
}

//! draw a 2d rectangle
void CWebGL1Driver::draw2DRectangle(SColor color,
		const core::rect<s32>& position,
		const core::rect<s32>* clip)
{
	chooseMaterial2D();
	setMaterialTexture(0, 0);

	setRenderStates2DMode(color.getAlpha() < 255, false, false);
	lockRenderStateMode();

	core::rect<s32> pos = position;

	if (clip)
		pos.clipAgainst(*clip);

	if (!pos.isValid())
		return;

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	f32 left = (f32)pos.UpperLeftCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 right = (f32)pos.LowerRightCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 down = 2.f - (f32)pos.LowerRightCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
	f32 top = 2.f - (f32)pos.UpperLeftCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

	MBTriangleFanSize4->Vertices[0] = S3DVertex(left, top, 0, 0, 0, 1, color, 0, 0);
	MBTriangleFanSize4->Vertices[1] = S3DVertex(right, top, 0, 0, 0, 1, color, 0, 0);
	MBTriangleFanSize4->Vertices[2] = S3DVertex(right, down, 0, 0, 0, 1, color, 0, 0);
	MBTriangleFanSize4->Vertices[3] = S3DVertex(left, down, 0, 0, 0, 1, color, 0, 0);
	MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBTriangleFanSize4);

	unlockRenderStateMode();
}

void CWebGL1Driver::draw2DRectangle(const core::rect<s32>& position,
				SColor colorLeftUp, SColor colorRightUp, SColor colorLeftDown, SColor colorRightDown,
				const core::rect<s32>* clip)
{
	core::rect<s32> pos = position;

	if (clip)
		pos.clipAgainst(*clip);

	if (!pos.isValid())
		return;

	chooseMaterial2D();
	setMaterialTexture(0, 0);

	setRenderStates2DMode(colorLeftUp.getAlpha() < 255 ||
			colorRightUp.getAlpha() < 255 ||
			colorLeftDown.getAlpha() < 255 ||
			colorRightDown.getAlpha() < 255, false, false);
	lockRenderStateMode();

	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

	f32 left = (f32)pos.UpperLeftCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 right = (f32)pos.LowerRightCorner.X / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 down = 2.f - (f32)pos.LowerRightCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
	f32 top = 2.f - (f32)pos.UpperLeftCorner.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

	MBTriangleFanSize4->Vertices[0] = S3DVertex(left, top, 0, 0, 0, 1, colorLeftUp, 0, 0);
	MBTriangleFanSize4->Vertices[1] = S3DVertex(right, top, 0, 0, 0, 1, colorRightUp, 0, 0);
	MBTriangleFanSize4->Vertices[2] = S3DVertex(right, down, 0, 0, 0, 1, colorRightDown, 0, 0);
	MBTriangleFanSize4->Vertices[3] = S3DVertex(left, down, 0, 0, 0, 1, colorLeftDown, 0, 0);
	MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBTriangleFanSize4);

	unlockRenderStateMode();
}

		//! Draws a 2d line.
void CWebGL1Driver::draw2DLine(const core::position2d<s32>& start, const core::position2d<s32>& end, SColor color)
{
	if (start==end)
		drawPixel(start.X, start.Y, color);
	else
	{
		chooseMaterial2D();
		setMaterialTexture(0, 0);

		setRenderStates2DMode(color.getAlpha() < 255, false, false);
		lockRenderStateMode();

		const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();

		f32 startX = (f32)start.X / (f32)renderTargetSize.Width * 2.f - 1.f;
		f32 endX = (f32)end.X / (f32)renderTargetSize.Width * 2.f - 1.f;
		f32 startY = 2.f - (f32)start.Y / (f32)renderTargetSize.Height * 2.f - 1.f;
		f32 endY = 2.f - (f32)end.Y / (f32)renderTargetSize.Height * 2.f - 1.f;

		MBLinesSize2->Vertices[0] = S3DVertex(startX, startY, 0, 0, 0, 1, color, 0, 0);
		MBLinesSize2->Vertices[1] = S3DVertex(endX, endY, 0, 0, 0, 1, color, 1, 1);
		MBLinesSize2->setDirty(scene::EBT_VERTEX);

		drawMeshBuffer(MBLinesSize2);

		unlockRenderStateMode();
	}
}

void CWebGL1Driver::drawPixel(u32 x, u32 y, const SColor & color)
{
	const core::dimension2d<u32>& renderTargetSize = getCurrentRenderTargetSize();
	if (x > (u32)renderTargetSize.Width || y > (u32)renderTargetSize.Height)
		return;

	chooseMaterial2D();
	setMaterialTexture(0, 0);

	setRenderStates2DMode(color.getAlpha() < 255, false, false);
	lockRenderStateMode();

	f32 X = (f32)x / (f32)renderTargetSize.Width * 2.f - 1.f;
	f32 Y = 2.f - (f32)y / (f32)renderTargetSize.Height * 2.f - 1.f;

	MBPointsSize1->Vertices[0] = S3DVertex(X, Y, 0, 0, 0, 1, color, 0, 0);
	MBPointsSize1->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBPointsSize1);

	unlockRenderStateMode();
}

void CWebGL1Driver::draw3DLine(const core::vector3df& start, const core::vector3df& end, SColor color)
{
	MBLinesSize2->Vertices[0] = S3DVertex(start.X, start.Y, start.Z, 0, 0, 1, color, 0, 0);
	MBLinesSize2->Vertices[1] = S3DVertex(end.X, end.Y, end.Z, 0, 0, 1, color, 0, 0);
	MBLinesSize2->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBLinesSize2);
}

void CWebGL1Driver::drawStencilShadowVolume(const core::array<core::vector3df>& triangles, bool zfail, u32 debugDataVisible)
{
	static bool first = true;
	if ( first )
	{
		first = false;
		os::Printer::log("WebGL1 driver does not yet support drawStencilShadowVolume", ELL_WARNING);
		os::Printer::log(__FILE__, irr::core::stringc(__LINE__).c_str(), ELL_WARNING);
	}
}

void CWebGL1Driver::drawStencilShadow(bool clearStencilBuffer,
	video::SColor leftUpEdge,
	video::SColor rightUpEdge,
	video::SColor leftDownEdge,
	video::SColor rightDownEdge)
{
	// NOTE: Might work, but untested as drawStencilShadowVolume is not yet supported.

	if (!StencilBuffer)
		return;

	chooseMaterial2D();
	setMaterialTexture(0, 0);

	setRenderStates2DMode(true, false, false);
	lockRenderStateMode();

	CacheHandler->setDepthMask(false);
	CacheHandler->setColorMask(ECP_ALL);

	CacheHandler->setBlend(true);
	CacheHandler->setBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, ~0);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

	MBTriangleFanSize4->Vertices[0] = S3DVertex(-1.f, 1.f, 0.9f, 0, 0, 1, leftDownEdge, 0, 0);
	MBTriangleFanSize4->Vertices[1] = S3DVertex(1.f, 1.f, 0.9f, 0, 0, 1, leftUpEdge, 0, 0);
	MBTriangleFanSize4->Vertices[2] = S3DVertex(1.f, -1.f, 0.9f, 0, 0, 1, rightUpEdge, 0, 0);
	MBTriangleFanSize4->Vertices[3] = S3DVertex(-1.f, -1.f, 0.9f, 0, 0, 1, rightDownEdge, 0, 0);
	MBTriangleFanSize4->setDirty(scene::EBT_VERTEX);

	drawMeshBuffer(MBTriangleFanSize4);

	unlockRenderStateMode();

	if (clearStencilBuffer)
		glClear(GL_STENCIL_BUFFER_BIT);

	glDisable(GL_STENCIL_TEST);
}

GLenum CWebGL1Driver::getZBufferBits() const
{
	// TODO: Never used, so not sure what this was really about (zbuffer used by device? Or for RTT's?)
	//       If it's about device it might need a check like: GLint depthBits; glGetIntegerv(GL_DEPTH_BITS, &depthBits);
	//       If it's about textures it might need a check for IRR_WEBGL_depth_texture

	GLenum bits = 0;

	switch (Params.ZBufferBits)
	{
#if defined(GL_OES_depth24)
	case 24:
		bits = GL_DEPTH_COMPONENT24_OES;
		break;
#endif
#if defined(GL_OES_depth32)
	case 32:
		bits = GL_DEPTH_COMPONENT32_OES;
		break;
#endif
	default:
		bits = GL_DEPTH_COMPONENT16_OES;
		break;
	}

	return bits;
}

bool CWebGL1Driver::getColorFormatParameters(ECOLOR_FORMAT format, GLint& internalFormat, GLenum& pixelFormat,
	GLenum& pixelType, void(**converter)(const void*, s32, void*)) const
{
		bool supported = false;
		pixelFormat = GL_RGBA;
		pixelType = GL_UNSIGNED_BYTE;
		*converter = 0;

		switch (format)
		{
		case ECF_A1R5G5B5:
			supported = true;
			pixelFormat = GL_RGBA;
			pixelType = GL_UNSIGNED_SHORT_5_5_5_1;
			*converter = CColorConverter::convert_A1R5G5B5toR5G5B5A1;
			break;
		case ECF_R5G6B5:
			supported = true;
			pixelFormat = GL_RGB;
			pixelType = GL_UNSIGNED_SHORT_5_6_5;
			break;
		case ECF_R8G8B8:
			supported = true;
			pixelFormat = GL_RGB;
			pixelType = GL_UNSIGNED_BYTE;
			break;
		case ECF_A8R8G8B8:
			// WebGL doesn't seem to support GL_BGRA so we always convert
			supported = true;
			pixelFormat = GL_RGBA;
			*converter = CColorConverter::convert_A8R8G8B8toA8B8G8R8;
			pixelType = GL_UNSIGNED_BYTE;
			break;
#ifdef GL_EXT_texture_compression_dxt1
		case ECF_DXT1:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_s3tc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
			}
			break;
#endif
#ifdef GL_EXT_texture_compression_s3tc
		case ECF_DXT2:
		case ECF_DXT3:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_s3tc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
			}
			break;
#endif
#ifdef GL_EXT_texture_compression_s3tc
		case ECF_DXT4:
		case ECF_DXT5:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_s3tc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
			}
			break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc
		case ECF_PVRTC_RGB2:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_pvrtc) )
			{
				supported = true;
				pixelFormat = GL_RGB;
				pixelType = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
			}
			break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc
		case ECF_PVRTC_ARGB2:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_pvrtc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
			}
			break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc
		case ECF_PVRTC_RGB4:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_pvrtc) )
			{
				supported = true;
				pixelFormat = GL_RGB;
				pixelType = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
			}
			break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc
		case ECF_PVRTC_ARGB4:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_pvrtc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
			}
			break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc2
		case ECF_PVRTC2_ARGB2:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_pvrtc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_PVRTC_2BPPV2_IMG;
			}
			break;
#endif
#ifdef GL_IMG_texture_compression_pvrtc2
		case ECF_PVRTC2_ARGB4:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_pvrtc) )
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_COMPRESSED_RGBA_PVRTC_4BPPV2_IMG;
			}
			break;
#endif
#ifdef GL_OES_compressed_ETC1_RGB8_texture
		case ECF_ETC1:
			if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_compressed_texture_etc1) )
			{
				supported = true;
				pixelFormat = GL_RGB;
				pixelType = GL_ETC1_RGB8_OES;
			}
			break;
#endif
#ifdef GL_ES_VERSION_3_0 // TO-DO - fix when extension name will be available
		case ECF_ETC2_RGB:
			supported = true;
			pixelFormat = GL_RGB;
			pixelType = GL_COMPRESSED_RGB8_ETC2;
			break;
#endif
#ifdef GL_ES_VERSION_3_0 // TO-DO - fix when extension name will be available
		case ECF_ETC2_ARGB:
			supported = true;
			pixelFormat = GL_RGBA;
			pixelType = GL_COMPRESSED_RGBA8_ETC2_EAC;
			break;
#endif
		case ECF_D16:
			if (WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_depth_texture))
			{
				supported = true;
				pixelFormat = GL_DEPTH_COMPONENT;
				pixelType = GL_UNSIGNED_SHORT;
			}
			break;
		case ECF_D32:
			if (WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_depth_texture))
			{
				// NOTE: There is still no guarantee it will return a 32 bit depth buffer. It might convert stuff internally to 16 bit :-(
				supported = true;
				pixelFormat = GL_DEPTH_COMPONENT;
				pixelType = GL_UNSIGNED_INT;
			}
			break;
		case ECF_D24S8:
			if (WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_WEBGL_depth_texture))
			{
				supported = true;
				pixelFormat = 0x84F9; // GL_DEPTH_STENCIL
				pixelType = 0x84FA;	  // UNSIGNED_INT_24_8_WEBGL
			}
			break;
		case ECF_R8:
			// Does not seem to be supported in WebGL so far (missing GL_EXT_texture_rg)
			break;
		case ECF_R8G8:
			// Does not seem to be supported in WebGL so far (missing GL_EXT_texture_rg)
			break;
		case ECF_R16:
			// Does not seem to be supported in WebGL so far
			break;
		case ECF_R16G16:
			// Does not seem to be supported in WebGL so far
			break;
		case ECF_R16F:
			// Does not seem to be supported in WebGL so far (missing GL_EXT_texture_rg)
			break;
		case ECF_G16R16F:
			// Does not seem to be supported in WebGL so far (missing GL_EXT_texture_rg)
			break;
		case ECF_A16B16G16R16F:
#if defined(GL_OES_texture_half_float)
			if (WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_OES_texture_half_float))
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_HALF_FLOAT_OES ;
			}
#endif
			break;
		case ECF_R32F:
			// Does not seem to be supported in WebGL so far (missing GL_EXT_texture_rg)
			break;
		case ECF_G32R32F:
			// Does not seem to be supported in WebGL so far (missing GL_EXT_texture_rg)
			break;
		case ECF_A32B32G32R32F:
#if defined(GL_OES_texture_float)
			if (WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_OES_texture_half_float))
			{
				supported = true;
				pixelFormat = GL_RGBA;
				pixelType = GL_FLOAT ;
			}
#endif
			break;
		default:
			break;
		}

		// ES 2.0 says internalFormat must match pixelFormat (chapter 3.7.1 in Spec).
		// Doesn't mention if "match" means "equal" or some other way of matching, but
		// some bug on Emscripten and browsing discussions by others lead me to believe
		// it means they have to be equal. Note that this was different in OpenGL.
		internalFormat = pixelFormat;

		return supported;
}


scene::SMeshBuffer* CWebGL1Driver::createSimpleMeshBuffer(irr::u32 numVertices, scene::E_PRIMITIVE_TYPE primitiveType, scene::E_HARDWARE_MAPPING vertexMappingHint, scene::E_HARDWARE_MAPPING indexMappingHint) const
{
	scene::SMeshBuffer* mbResult = new scene::SMeshBuffer();
	mbResult->Vertices.set_used(numVertices);
	mbResult->Indices.set_used(numVertices);
	for ( irr::u32 i=0; i < numVertices; ++i )
		mbResult->Indices[i] = i;

	mbResult->setPrimitiveType(primitiveType);
	mbResult->setHardwareMappingHint(vertexMappingHint, scene::EBT_VERTEX);
	mbResult->setHardwareMappingHint(indexMappingHint, scene::EBT_INDEX);
	mbResult->setDirty();

	return mbResult;
}

bool CWebGL1Driver::genericDriverInit(const core::dimension2d<u32>& screenSize, bool stencilBuffer)
{
	Name = glGetString(GL_VERSION);
	printVersion();

	// print renderer information
	VendorName = glGetString(GL_VENDOR);
	os::Printer::log(VendorName.c_str(), ELL_INFORMATION);

	// load extensions
	initWebGLExtensions();

	// reset cache handler
	delete CacheHandler;
	CacheHandler = new COGLES2CacheHandler(this);

	StencilBuffer = stencilBuffer;

	DriverAttributes->setAttribute("MaxTextures", (s32)Feature.MaxTextureUnits);
	DriverAttributes->setAttribute("MaxSupportedTextures", (s32)Feature.MaxTextureUnits);
	DriverAttributes->setAttribute("MaxAnisotropy", MaxAnisotropy);
	DriverAttributes->setAttribute("MaxIndices", (s32)MaxIndices);
	DriverAttributes->setAttribute("MaxTextureSize", (s32)MaxTextureSize);
	DriverAttributes->setAttribute("MaxTextureLODBias", MaxTextureLODBias);
	DriverAttributes->setAttribute("Version", Version);
	DriverAttributes->setAttribute("AntiAlias", AntiAlias);

	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	UserClipPlane.reallocate(0);

	for (s32 i = 0; i < ETS_COUNT; ++i)
		setTransform(static_cast<E_TRANSFORMATION_STATE>(i), core::IdentityMatrix);

	setAmbientLight(SColorf(0.0f, 0.0f, 0.0f, 0.0f));
	glClearDepthf(1.0f);

	glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST);
	glFrontFace(GL_CW);

	// create material renderers
	createMaterialRenderers();

	// set the renderstates
	setRenderStates3DMode();

	// set fog mode
	setFog(FogColor, FogType, FogStart, FogEnd, FogDensity, PixelFog, RangeFog);

	// create matrix for flipping textures
	TextureFlipMatrix.buildTextureTransform(0.0f, core::vector2df(0, 0), core::vector2df(0, 1.0f), core::vector2df(1.0f, -1.0f));

	// We need to reset once more at the beginning of the first rendering.
	// This fixes problems with intermediate changes to the material during texture load.
	ResetRenderStates = true;

	testGLError(__LINE__);

	return true;
}

void CWebGL1Driver::initWebGLExtensions()
{
	// Stuff still a little bit hacky as we derive from ES2Driver with it's own extensions.
	// We only get the feature-strings from WebGLExtensions.

	getGLVersion();

	WebGLExtensions.getGLExtensions();

	// TODO: basically copied ES2 implementation, so not certain if 100% correct for WebGL
	GLint val=0;
	glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &val);
	Feature.MaxTextureUnits = static_cast<u8>(val);

#ifdef GL_EXT_texture_filter_anisotropic
	if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_EXT_texture_filter_anisotropic) )
	{
		glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &val);
		MaxAnisotropy = static_cast<u8>(val);
	}
#endif

	if ( WebGLExtensions.queryWebGLFeature(CWebGLExtensionHandler::IRR_OES_element_index_uint) )	// note: WebGL2 won't need extension as that got default there
	{
		MaxIndices=0xffffffff;
	}

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &val);
	MaxTextureSize=static_cast<u32>(val);

#ifdef GL_MAX_TEXTURE_LOD_BIAS_EXT
	// TODO: Found no info about this anywhere. It's no extension in WebGL
	//       and GL_MAX_TEXTURE_LOD_BIAS_EXT doesn't seem to be part of gl2ext.h in emscripten
	glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &MaxTextureLODBias);
#endif

	glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, DimAliasedLine);
	glGetFloatv(GL_ALIASED_POINT_SIZE_RANGE, DimAliasedPoint);
	Feature.MaxTextureUnits = core::min_(Feature.MaxTextureUnits, static_cast<u8>(MATERIAL_MAX_TEXTURES));

	Feature.ColorAttachment = 1;
}

} // end namespace video
} // end namespace irr

#endif // _IRR_COMPILE_WITH_WEBGL1_

namespace irr
{
#ifndef _IRR_COMPILE_WITH_WEBGL1_
namespace io
{
	class IFileSystem;
}
#endif
namespace video
{

#ifndef _IRR_COMPILE_WITH_WEBGL1_
class IVideoDriver;
class IContextManager;
#endif

IVideoDriver* createWebGL1Driver(const SIrrlichtCreationParameters& params, io::IFileSystem* io, IContextManager* contextManager)
{
#ifdef _IRR_COMPILE_WITH_WEBGL1_
	CWebGL1Driver* driver = new CWebGL1Driver(params, io, contextManager);
	driver->genericDriverInit(params.WindowSize, params.Stencilbuffer);	// don't call in constructor, it uses virtual function calls of driver
	return driver;
#else
	return 0;
#endif //  _IRR_COMPILE_WITH_WEBGL1_
}

} // end namespace
} // end namespace
