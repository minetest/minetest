// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_MESH_MANIPULATOR_H_INCLUDED__
#define __C_MESH_MANIPULATOR_H_INCLUDED__

#include "IMeshManipulator.h"

namespace irr
{
namespace scene
{

//! An interface for easy manipulation of meshes.
/** Scale, set alpha value, flip surfaces, and so on. This exists for fixing
problems with wrong imported or exported meshes quickly after loading. It is
not intended for doing mesh modifications and/or animations during runtime.
*/
class CMeshManipulator : public IMeshManipulator
{
public:
	//! Flips the direction of surfaces.
	/** Changes backfacing triangles to frontfacing triangles and vice versa.
	\param mesh: Mesh on which the operation is performed. */
	virtual void flipSurfaces(scene::IMesh* mesh) const;

	//! Recalculates all normals of the mesh.
	/** \param mesh: Mesh on which the operation is performed.
	    \param smooth: Whether to use smoothed normals. */
	virtual void recalculateNormals(scene::IMesh* mesh, bool smooth = false, bool angleWeighted = false) const;

	//! Recalculates all normals of the mesh buffer.
	/** \param buffer: Mesh buffer on which the operation is performed.
	    \param smooth: Whether to use smoothed normals. */
	virtual void recalculateNormals(IMeshBuffer* buffer, bool smooth = false, bool angleWeighted = false) const;

	//! Clones a static IMesh into a modifiable SMesh.
	virtual SMesh* createMeshCopy(scene::IMesh* mesh) const;

	//! Creates a planar texture mapping on the mesh
	/** \param mesh: Mesh on which the operation is performed.
	\param resolution: resolution of the planar mapping. This is the value
	specifying which is the relation between world space and 
	texture coordinate space. */
	virtual void makePlanarTextureMapping(scene::IMesh* mesh, f32 resolution=0.001f) const;

	//! Creates a planar texture mapping on the meshbuffer
	virtual void makePlanarTextureMapping(scene::IMeshBuffer* meshbuffer, f32 resolution=0.001f) const;

	//! Creates a planar texture mapping on the meshbuffer
	void makePlanarTextureMapping(scene::IMeshBuffer* buffer, f32 resolutionS, f32 resolutionT, u8 axis, const core::vector3df& offset) const;

	//! Creates a planar texture mapping on the mesh
	void makePlanarTextureMapping(scene::IMesh* mesh, f32 resolutionS, f32 resolutionT, u8 axis, const core::vector3df& offset) const;

	//! Recalculates tangents, requires a tangent mesh buffer
	virtual void recalculateTangents(IMeshBuffer* buffer, bool recalculateNormals=false, bool smooth=false, bool angleWeighted=false) const;

	//! Recalculates tangents, requires a tangent mesh
	virtual void recalculateTangents(IMesh* mesh, bool recalculateNormals=false, bool smooth=false, bool angleWeighted=false) const;

	//! Creates a copy of the mesh, which will only consist of S3DVertexTangents vertices.
	virtual IMesh* createMeshWithTangents(IMesh* mesh, bool recalculateNormals=false, bool smooth=false, bool angleWeighted=false, bool recalculateTangents=true) const;

	//! Creates a copy of the mesh, which will only consist of S3D2TCoords vertices.
	virtual IMesh* createMeshWith2TCoords(IMesh* mesh) const;

	//! Creates a copy of the mesh, which will only consist of S3DVertex vertices.
	virtual IMesh* createMeshWith1TCoords(IMesh* mesh) const;

	//! Creates a copy of the mesh, which will only consist of unique triangles, i.e. no vertices are shared.
	virtual IMesh* createMeshUniquePrimitives(IMesh* mesh) const;

	//! Creates a copy of the mesh, which will have all duplicated vertices removed, i.e. maximal amount of vertices are shared via indexing.
	virtual IMesh* createMeshWelded(IMesh *mesh, f32 tolerance=core::ROUNDING_ERROR_f32) const;

	//! Returns amount of polygons in mesh.
	virtual s32 getPolyCount(scene::IMesh* mesh) const;

	//! Returns amount of polygons in mesh.
	virtual s32 getPolyCount(scene::IAnimatedMesh* mesh) const;

	//! create a new AnimatedMesh and adds the mesh to it
	virtual IAnimatedMesh * createAnimatedMesh(scene::IMesh* mesh,scene::E_ANIMATED_MESH_TYPE type) const;

	//! create a mesh optimized for the vertex cache
	virtual IMesh* createForsythOptimizedMesh(const scene::IMesh *mesh) const;
};

} // end namespace scene
} // end namespace irr


#endif
