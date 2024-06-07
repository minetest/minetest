// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IReferenceCounted.h"
#include "SMaterial.h"
#include "aabbox3d.h"
#include "S3DVertex.h"
#include "SVertexIndex.h"
#include "EHardwareBufferFlags.h"
#include "EPrimitiveTypes.h"

namespace irr
{
namespace scene
{
//! Struct for holding a mesh with a single material.
/** A part of an IMesh which has the same material on each face of that
group. Logical groups of an IMesh need not be put into separate mesh
buffers, but can be. Separately animated parts of the mesh must be put
into separate mesh buffers.
Some mesh buffer implementations have limitations on the number of
vertices the buffer can hold. In that case, logical grouping can help.
Moreover, the number of vertices should be optimized for the GPU upload,
which often depends on the type of gfx card. Typical figures are
1000-10000 vertices per buffer.
SMeshBuffer is a simple implementation of a MeshBuffer, which supports
up to 65535 vertices.

Since meshbuffers are used for drawing, and hence will be exposed
to the driver, chances are high that they are grab()'ed from somewhere.
It's therefore required to dynamically allocate meshbuffers which are
passed to a video driver and only drop the buffer once it's not used in
the current code block anymore.
*/
class IMeshBuffer : public virtual IReferenceCounted
{
public:
	//! Get the material of this meshbuffer
	/** \return Material of this buffer. */
	virtual video::SMaterial &getMaterial() = 0;

	//! Get the material of this meshbuffer
	/** \return Material of this buffer. */
	virtual const video::SMaterial &getMaterial() const = 0;

	//! Get type of vertex data which is stored in this meshbuffer.
	/** \return Vertex type of this buffer. */
	virtual video::E_VERTEX_TYPE getVertexType() const = 0;

	//! Get access to vertex data. The data is an array of vertices.
	/** Which vertex type is used can be determined by getVertexType().
	\return Pointer to array of vertices. */
	virtual const void *getVertices() const = 0;

	//! Get access to vertex data. The data is an array of vertices.
	/** Which vertex type is used can be determined by getVertexType().
	\return Pointer to array of vertices. */
	virtual void *getVertices() = 0;

	//! Get amount of vertices in meshbuffer.
	/** \return Number of vertices in this buffer. */
	virtual u32 getVertexCount() const = 0;

	//! Get type of index data which is stored in this meshbuffer.
	/** \return Index type of this buffer. */
	virtual video::E_INDEX_TYPE getIndexType() const = 0;

	//! Get access to indices.
	/** \return Pointer to indices array. */
	virtual const u16 *getIndices() const = 0;

	//! Get access to indices.
	/** \return Pointer to indices array. */
	virtual u16 *getIndices() = 0;

	//! Get amount of indices in this meshbuffer.
	/** \return Number of indices in this buffer. */
	virtual u32 getIndexCount() const = 0;

	//! Get the axis aligned bounding box of this meshbuffer.
	/** \return Axis aligned bounding box of this buffer. */
	virtual const core::aabbox3df &getBoundingBox() const = 0;

	//! Set axis aligned bounding box
	/** \param box User defined axis aligned bounding box to use
	for this buffer. */
	virtual void setBoundingBox(const core::aabbox3df &box) = 0;

	//! Recalculates the bounding box. Should be called if the mesh changed.
	virtual void recalculateBoundingBox() = 0;

	//! returns position of vertex i
	virtual const core::vector3df &getPosition(u32 i) const = 0;

	//! returns position of vertex i
	virtual core::vector3df &getPosition(u32 i) = 0;

	//! returns normal of vertex i
	virtual const core::vector3df &getNormal(u32 i) const = 0;

	//! returns normal of vertex i
	virtual core::vector3df &getNormal(u32 i) = 0;

	//! returns texture coord of vertex i
	virtual const core::vector2df &getTCoords(u32 i) const = 0;

	//! returns texture coord of vertex i
	virtual core::vector2df &getTCoords(u32 i) = 0;

	//! Append the vertices and indices to the current buffer
	/** Only works for compatible vertex types.
	\param vertices Pointer to a vertex array.
	\param numVertices Number of vertices in the array.
	\param indices Pointer to index array.
	\param numIndices Number of indices in array. */
	virtual void append(const void *const vertices, u32 numVertices, const u16 *const indices, u32 numIndices) = 0;

	//! get the current hardware mapping hint
	virtual E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const = 0;

	//! get the current hardware mapping hint
	virtual E_HARDWARE_MAPPING getHardwareMappingHint_Index() const = 0;

	//! set the hardware mapping hint, for driver
	virtual void setHardwareMappingHint(E_HARDWARE_MAPPING newMappingHint, E_BUFFER_TYPE buffer = EBT_VERTEX_AND_INDEX) = 0;

	//! flags the meshbuffer as changed, reloads hardware buffers
	virtual void setDirty(E_BUFFER_TYPE buffer = EBT_VERTEX_AND_INDEX) = 0;

	//! Get the currently used ID for identification of changes.
	/** This shouldn't be used for anything outside the VideoDriver. */
	virtual u32 getChangedID_Vertex() const = 0;

	//! Get the currently used ID for identification of changes.
	/** This shouldn't be used for anything outside the VideoDriver. */
	virtual u32 getChangedID_Index() const = 0;

	//! Used by the VideoDriver to remember the buffer link.
	virtual void setHWBuffer(void *ptr) const = 0;
	virtual void *getHWBuffer() const = 0;

	//! Describe what kind of primitive geometry is used by the meshbuffer
	/** Note: Default is EPT_TRIANGLES. Using other types is fine for rendering.
	But meshbuffer manipulation functions might expect type EPT_TRIANGLES
	to work correctly. Also mesh writers will generally fail (badly!) with other
	types than EPT_TRIANGLES. */
	virtual void setPrimitiveType(E_PRIMITIVE_TYPE type) = 0;

	//! Get the kind of primitive geometry which is used by the meshbuffer
	virtual E_PRIMITIVE_TYPE getPrimitiveType() const = 0;

	//! Calculate how many geometric primitives are used by this meshbuffer
	virtual u32 getPrimitiveCount() const
	{
		const u32 indexCount = getIndexCount();
		switch (getPrimitiveType()) {
		case scene::EPT_POINTS:
			return indexCount;
		case scene::EPT_LINE_STRIP:
			return indexCount - 1;
		case scene::EPT_LINE_LOOP:
			return indexCount;
		case scene::EPT_LINES:
			return indexCount / 2;
		case scene::EPT_TRIANGLE_STRIP:
			return (indexCount - 2);
		case scene::EPT_TRIANGLE_FAN:
			return (indexCount - 2);
		case scene::EPT_TRIANGLES:
			return indexCount / 3;
		case scene::EPT_POINT_SPRITES:
			return indexCount;
		}
		return 0;
	}

	//! Calculate size of vertices and indices in memory
	virtual size_t getSize() const
	{
		size_t ret = 0;
		switch (getVertexType()) {
			case video::EVT_STANDARD:
				ret += sizeof(video::S3DVertex) * getVertexCount();
				break;
			case video::EVT_2TCOORDS:
				ret += sizeof(video::S3DVertex2TCoords) * getVertexCount();
				break;
			case video::EVT_TANGENTS:
				ret += sizeof(video::S3DVertexTangents) * getVertexCount();
				break;
		}
		switch (getIndexType()) {
			case video::EIT_16BIT:
				ret += sizeof(u16) * getIndexCount();
				break;
			case video::EIT_32BIT:
				ret += sizeof(u32) * getIndexCount();
				break;
		}
		return ret;
	}
};

} // end namespace scene
} // end namespace irr
