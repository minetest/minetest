// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_TRIANGLE_SELECTOR_H_INCLUDED__
#define __C_TRIANGLE_SELECTOR_H_INCLUDED__

#include "ITriangleSelector.h"
#include "IMesh.h"
#include "irrArray.h"
#include "aabbox3d.h"

namespace irr
{
namespace scene
{

class ISceneNode;
class IAnimatedMeshSceneNode;

//! Stupid triangle selector without optimization
class CTriangleSelector : public ITriangleSelector
{
public:

	//! Constructs a selector based on a mesh
	CTriangleSelector(ISceneNode* node);

	//! Constructs a selector based on a mesh
	CTriangleSelector(const IMesh* mesh, ISceneNode* node);

	//! Constructs a selector based on an animated mesh scene node
	//!\param node An animated mesh scene node, which must have a valid mesh
	CTriangleSelector(IAnimatedMeshSceneNode* node);

	//! Constructs a selector based on a bounding box
	CTriangleSelector(const core::aabbox3d<f32>& box, ISceneNode* node);

	//! Gets all triangles.
	void getTriangles(core::triangle3df* triangles, s32 arraySize, s32& outTriangleCount,
		const core::matrix4* transform=0) const;

	//! Gets all triangles which lie within a specific bounding box.
	void getTriangles(core::triangle3df* triangles, s32 arraySize, s32& outTriangleCount,
		const core::aabbox3d<f32>& box, const core::matrix4* transform=0) const;

	//! Gets all triangles which have or may have contact with a 3d line.
	virtual void getTriangles(core::triangle3df* triangles, s32 arraySize,
		s32& outTriangleCount, const core::line3d<f32>& line,
		const core::matrix4* transform=0) const;

	//! Returns amount of all available triangles in this selector
	virtual s32 getTriangleCount() const;

	//! Return the scene node associated with a given triangle.
	virtual ISceneNode* getSceneNodeForTriangle(u32 triangleIndex) const { return SceneNode; }

	// Get the number of TriangleSelectors that are part of this one
	virtual u32 getSelectorCount() const;

	// Get the TriangleSelector based on index based on getSelectorCount
	virtual ITriangleSelector* getSelector(u32 index);

	// Get the TriangleSelector based on index based on getSelectorCount
	virtual const ITriangleSelector* getSelector(u32 index) const;

protected:
	//! Create from a mesh
	virtual void createFromMesh(const IMesh* mesh);

	//! Update when the mesh has changed
	virtual void updateFromMesh(const IMesh* mesh) const;

	//! Update the triangle selector, which will only have an effect if it
	//! was built from an animated mesh and that mesh's frame has changed
	//! since the last time it was updated.
	virtual void update(void) const;

	ISceneNode* SceneNode;
	mutable core::array<core::triangle3df> Triangles; // (mutable for CTriangleBBSelector)
	mutable core::aabbox3df BoundingBox; // Allows for trivial rejection

	IAnimatedMeshSceneNode* AnimatedNode;
	mutable u32 LastMeshFrame;
};

} // end namespace scene
} // end namespace irr


#endif

