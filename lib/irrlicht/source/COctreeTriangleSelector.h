// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_OCTREE_TRIANGLE_SELECTOR_H_INCLUDED__
#define __C_OCTREE_TRIANGLE_SELECTOR_H_INCLUDED__

#include "CTriangleSelector.h"

namespace irr
{
namespace scene
{

class ISceneNode;

//! Stupid triangle selector without optimization
class COctreeTriangleSelector : public CTriangleSelector
{
public:

	//! Constructs a selector based on a mesh
	COctreeTriangleSelector(const IMesh* mesh, ISceneNode* node, s32 minimalPolysPerNode);

	virtual ~COctreeTriangleSelector();

	//! Gets all triangles which lie within a specific bounding box.
	virtual void getTriangles(core::triangle3df* triangles, s32 arraySize, s32& outTriangleCount,
		const core::aabbox3d<f32>& box, const core::matrix4* transform=0) const;

	//! Gets all triangles which have or may have contact with a 3d line.
	virtual void getTriangles(core::triangle3df* triangles, s32 arraySize,
		s32& outTriangleCount, const core::line3d<f32>& line,
		const core::matrix4* transform=0) const;

private:

	struct SOctreeNode
	{
		SOctreeNode()
		{
			for (u32 i=0; i!=8; ++i)
				Child[i] = 0;
		}

		~SOctreeNode()
		{
			for (u32 i=0; i!=8; ++i)
				delete Child[i];
		}

		core::array<core::triangle3df> Triangles;
		SOctreeNode* Child[8];
		core::aabbox3d<f32> Box;
	};


	void constructOctree(SOctreeNode* node);
	void deleteEmptyNodes(SOctreeNode* node);
	void getTrianglesFromOctree(SOctreeNode* node, s32& trianglesWritten,
			s32 maximumSize, const core::aabbox3d<f32>& box,
			const core::matrix4* transform,
			core::triangle3df* triangles) const;

	void getTrianglesFromOctree(SOctreeNode* node, s32& trianglesWritten,
			s32 maximumSize, const core::line3d<f32>& line,
			const core::matrix4* transform,
			core::triangle3df* triangles) const;

	SOctreeNode* Root;
	s32 NodeCount;
	s32 MinimalPolysPerNode;
};

} // end namespace scene
} // end namespace irr


#endif

