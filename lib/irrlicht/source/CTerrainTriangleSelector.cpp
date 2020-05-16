// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CTerrainTriangleSelector.h"
#include "CTerrainSceneNode.h"
#include "os.h"

namespace irr
{
namespace scene
{


//! constructor
CTerrainTriangleSelector::CTerrainTriangleSelector ( ITerrainSceneNode* node, s32 LOD )
	: SceneNode(node)
{
	#ifdef _DEBUG
	setDebugName ("CTerrainTriangleSelector");
	#endif

	setTriangleData(node, LOD);
}


//! destructor
CTerrainTriangleSelector::~CTerrainTriangleSelector()
{
	TrianglePatches.TrianglePatchArray.clear();
}


//! Clears and sets triangle data
void CTerrainTriangleSelector::setTriangleData(ITerrainSceneNode* node, s32 LOD)
{
	// Get pointer to the GeoMipMaps vertices
	const video::S3DVertex2TCoords* vertices = static_cast<const video::S3DVertex2TCoords*>(node->getRenderBuffer()->getVertices());

	// Clear current data
	const s32 count = (static_cast<CTerrainSceneNode*>(node))->TerrainData.PatchCount;
	TrianglePatches.TotalTriangles = 0;
	TrianglePatches.NumPatches = count*count;

	TrianglePatches.TrianglePatchArray.reallocate(TrianglePatches.NumPatches);
	for (s32 o=0; o<TrianglePatches.NumPatches; ++o)
		TrianglePatches.TrianglePatchArray.push_back(SGeoMipMapTrianglePatch());

	core::triangle3df tri;
	core::array<u32> indices;
	s32 tIndex = 0;
	for(s32 x = 0; x < count; ++x )
	{
		for(s32 z = 0; z < count; ++z )
		{
			TrianglePatches.TrianglePatchArray[tIndex].NumTriangles = 0;
			TrianglePatches.TrianglePatchArray[tIndex].Box = node->getBoundingBox( x, z );
			u32 indexCount = node->getIndicesForPatch( indices, x, z, LOD );

			TrianglePatches.TrianglePatchArray[tIndex].Triangles.reallocate(indexCount/3);
			for(u32 i = 0; i < indexCount; i += 3 )
			{
				tri.pointA = vertices[indices[i+0]].Pos;
				tri.pointB = vertices[indices[i+1]].Pos;
				tri.pointC = vertices[indices[i+2]].Pos;
				TrianglePatches.TrianglePatchArray[tIndex].Triangles.push_back(tri);
				++TrianglePatches.TrianglePatchArray[tIndex].NumTriangles;
			}

			TrianglePatches.TotalTriangles += TrianglePatches.TrianglePatchArray[tIndex].NumTriangles;
			++tIndex;
		}
	}
}


//! Gets all triangles.
void CTerrainTriangleSelector::getTriangles(core::triangle3df* triangles,
			s32 arraySize, s32& outTriangleCount,
			const core::matrix4* transform) const
{
	s32 count = TrianglePatches.TotalTriangles;

	if (count > arraySize)
		count = arraySize;

	core::matrix4 mat;

	if (transform)
		mat = (*transform);

	s32 tIndex = 0;

	for (s32 i=0; i<TrianglePatches.NumPatches; ++i)
	{
		if (tIndex + TrianglePatches.TrianglePatchArray[i].NumTriangles <= count)
			for (s32 j=0; j<TrianglePatches.TrianglePatchArray[i].NumTriangles; ++j)
			{
				triangles[tIndex] = TrianglePatches.TrianglePatchArray[i].Triangles[j];

				mat.transformVect(triangles[tIndex].pointA);
				mat.transformVect(triangles[tIndex].pointB);
				mat.transformVect(triangles[tIndex].pointC);

				++tIndex;
			}
	}

	outTriangleCount = tIndex;
}


//! Gets all triangles which lie within a specific bounding box.
void CTerrainTriangleSelector::getTriangles(core::triangle3df* triangles,
		s32 arraySize, s32& outTriangleCount,
		const core::aabbox3d<f32>& box, const core::matrix4* transform) const
{
	s32 count = TrianglePatches.TotalTriangles;

	if (count > arraySize)
		count = arraySize;

	core::matrix4 mat;

	if (transform)
		mat = (*transform);

	s32 tIndex = 0;

	for (s32 i=0; i<TrianglePatches.NumPatches; ++i)
	{
		if (tIndex + TrianglePatches.TrianglePatchArray[i].NumTriangles <= count &&
			TrianglePatches.TrianglePatchArray[i].Box.intersectsWithBox(box))
			for (s32 j=0; j<TrianglePatches.TrianglePatchArray[i].NumTriangles; ++j)
			{
				triangles[tIndex] = TrianglePatches.TrianglePatchArray[i].Triangles[j];

				mat.transformVect(triangles[tIndex].pointA);
				mat.transformVect(triangles[tIndex].pointB);
				mat.transformVect(triangles[tIndex].pointC);

				++tIndex;
			}
	}

	outTriangleCount = tIndex;
}


//! Gets all triangles which have or may have contact with a 3d line.
void CTerrainTriangleSelector::getTriangles(core::triangle3df* triangles,
		s32 arraySize, s32& outTriangleCount, const core::line3d<f32>& line,
		const core::matrix4* transform) const
{
	const s32 count = core::min_((s32)TrianglePatches.TotalTriangles, arraySize);

	core::matrix4 mat;

	if (transform)
		mat = (*transform);

	s32 tIndex = 0;

	for (s32 i=0; i<TrianglePatches.NumPatches; ++i)
	{
		if (tIndex + TrianglePatches.TrianglePatchArray[i].NumTriangles <= count
            && TrianglePatches.TrianglePatchArray[i].Box.intersectsWithLine(line))
		{
			for (s32 j=0; j<TrianglePatches.TrianglePatchArray[i].NumTriangles; ++j)
			{
				triangles[tIndex] = TrianglePatches.TrianglePatchArray[i].Triangles[j];

				mat.transformVect(triangles[tIndex].pointA);
				mat.transformVect(triangles[tIndex].pointB);
				mat.transformVect(triangles[tIndex].pointC);

				++tIndex;
			}
		}
	}

	outTriangleCount = tIndex;
}


//! Returns amount of all available triangles in this selector
s32 CTerrainTriangleSelector::getTriangleCount() const
{
	return TrianglePatches.TotalTriangles;
}


ISceneNode* CTerrainTriangleSelector::getSceneNodeForTriangle(
		u32 triangleIndex) const
{
	return SceneNode;
}


/* Get the number of TriangleSelectors that are part of this one.
Only useful for MetaTriangleSelector others return 1
*/
u32 CTerrainTriangleSelector::getSelectorCount() const
{
	return 1;
}


/* Get the TriangleSelector based on index based on getSelectorCount.
Only useful for MetaTriangleSelector others return 'this' or 0
*/
ITriangleSelector* CTerrainTriangleSelector::getSelector(u32 index)
{
	if (index)
		return 0;
	else
		return this;
}


/* Get the TriangleSelector based on index based on getSelectorCount.
Only useful for MetaTriangleSelector others return 'this' or 0
*/
const ITriangleSelector* CTerrainTriangleSelector::getSelector(u32 index) const
{
	if (index)
		return 0;
	else
		return this;
}


} // end namespace scene
} // end namespace irr
