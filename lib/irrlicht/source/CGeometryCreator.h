// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GEOMETRY_CREATOR_H_INCLUDED__
#define __C_GEOMETRY_CREATOR_H_INCLUDED__

#include "IGeometryCreator.h"
#include "SMeshBuffer.h"

namespace irr
{

namespace scene
{

//! class for creating geometry on the fly
class CGeometryCreator : public IGeometryCreator
{
	void addToBuffer(const video::S3DVertex& v, SMeshBuffer* Buffer) const;
public:
	IMesh* createCubeMesh(const core::vector3df& size) const;

	IMesh* createHillPlaneMesh(
		const core::dimension2d<f32>& tileSize, const core::dimension2d<u32>& tileCount,
		video::SMaterial* material, f32 hillHeight, const core::dimension2d<f32>& countHills,
		const core::dimension2d<f32>& textureRepeatCount) const;

	IMesh* createTerrainMesh(video::IImage* texture,
		video::IImage* heightmap, const core::dimension2d<f32>& stretchSize,
		f32 maxHeight, video::IVideoDriver* driver,
		const core::dimension2d<u32>& defaultVertexBlockSize,
		bool debugBorders=false) const;

	IMesh* createArrowMesh(const u32 tesselationCylinder,
			const u32 tesselationCone, const f32 height,
			const f32 cylinderHeight, const f32 width0,
			const f32 width1, const video::SColor vtxColor0,
			const video::SColor vtxColor1) const;

	IMesh* createSphereMesh(f32 radius, u32 polyCountX, u32 polyCountY) const;

	IMesh* createCylinderMesh(f32 radius, f32 length, u32 tesselation,
				const video::SColor& color=0xffffffff,
				bool closeTop=true, f32 oblique=0.f) const;

	IMesh* createConeMesh(f32 radius, f32 length, u32 tesselation,
				const video::SColor& colorTop=0xffffffff,
				const video::SColor& colorBottom=0xffffffff,
				f32 oblique=0.f) const;

	IMesh* createVolumeLightMesh(
			const u32 subdivideU=32, const u32 subdivideV=32,
			const video::SColor footColor=0xffffffff,
			const video::SColor tailColor=0xffffffff,
			const f32 lpDistance = 8.f,
			const core::vector3df& lightDim = core::vector3df(1.f,1.2f,1.f)) const;
};


} // end namespace scene
} // end namespace irr

#endif

