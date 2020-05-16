// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CTriangleBBSelector.h"
#include "ISceneNode.h"

namespace irr
{
namespace scene
{

//! constructor
CTriangleBBSelector::CTriangleBBSelector(ISceneNode* node)
: CTriangleSelector(node)
{
	#ifdef _DEBUG
	setDebugName("CTriangleBBSelector");
	#endif

	Triangles.set_used(12); // a box has 12 triangles.
}



//! Gets all triangles.
void CTriangleBBSelector::getTriangles(core::triangle3df* triangles,
					s32 arraySize, s32& outTriangleCount,
					const core::matrix4* transform) const
{
	if (!SceneNode)
		return;

	// construct triangles
	const core::aabbox3d<f32>& box = SceneNode->getBoundingBox();
	core::vector3df edges[8];
	box.getEdges(edges);

	Triangles[0].set( edges[3], edges[0], edges[2]);
	Triangles[1].set( edges[3], edges[1], edges[0]);

	Triangles[2].set( edges[3], edges[2], edges[7]);
	Triangles[3].set( edges[7], edges[2], edges[6]);

	Triangles[4].set( edges[7], edges[6], edges[4]);
	Triangles[5].set( edges[5], edges[7], edges[4]);

	Triangles[6].set( edges[5], edges[4], edges[0]);
	Triangles[7].set( edges[5], edges[0], edges[1]);

	Triangles[8].set( edges[1], edges[3], edges[7]);
	Triangles[9].set( edges[1], edges[7], edges[5]);

	Triangles[10].set(edges[0], edges[6], edges[2]);
	Triangles[11].set(edges[0], edges[4], edges[6]);

	// call parent
	CTriangleSelector::getTriangles(triangles, arraySize, outTriangleCount,	transform);
}

void CTriangleBBSelector::getTriangles(core::triangle3df* triangles,
					s32 arraySize, s32& outTriangleCount,
					const core::aabbox3d<f32>& box,
					const core::matrix4* transform) const
{
	return getTriangles(triangles, arraySize, outTriangleCount, transform);
}

void CTriangleBBSelector::getTriangles(core::triangle3df* triangles,
					s32 arraySize, s32& outTriangleCount,
					const core::line3d<f32>& line,
					const core::matrix4* transform) const
{
	return getTriangles(triangles, arraySize, outTriangleCount, transform);
}


} // end namespace scene
} // end namespace irr

