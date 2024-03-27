// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irrArray.h"
#include "IMeshBuffer.h"

namespace irr
{
namespace scene
{
//! Template implementation of the IMeshBuffer interface
template <class T>
class CMeshBuffer : public IMeshBuffer
{
public:
	//! Default constructor for empty meshbuffer
	CMeshBuffer() :
			ChangedID_Vertex(1), ChangedID_Index(1), MappingHint_Vertex(EHM_NEVER), MappingHint_Index(EHM_NEVER), HWBuffer(NULL), PrimitiveType(EPT_TRIANGLES)
	{
#ifdef _DEBUG
		setDebugName("CMeshBuffer");
#endif
	}

	//! Get material of this meshbuffer
	/** \return Material of this buffer */
	const video::SMaterial &getMaterial() const override
	{
		return Material;
	}

	//! Get material of this meshbuffer
	/** \return Material of this buffer */
	video::SMaterial &getMaterial() override
	{
		return Material;
	}

	//! Get pointer to vertices
	/** \return Pointer to vertices. */
	const void *getVertices() const override
	{
		return Vertices.const_pointer();
	}

	//! Get pointer to vertices
	/** \return Pointer to vertices. */
	void *getVertices() override
	{
		return Vertices.pointer();
	}

	//! Get number of vertices
	/** \return Number of vertices. */
	u32 getVertexCount() const override
	{
		return Vertices.size();
	}

	//! Get type of index data which is stored in this meshbuffer.
	/** \return Index type of this buffer. */
	video::E_INDEX_TYPE getIndexType() const override
	{
		return video::EIT_16BIT;
	}

	//! Get pointer to indices
	/** \return Pointer to indices. */
	const u16 *getIndices() const override
	{
		return Indices.const_pointer();
	}

	//! Get pointer to indices
	/** \return Pointer to indices. */
	u16 *getIndices() override
	{
		return Indices.pointer();
	}

	//! Get number of indices
	/** \return Number of indices. */
	u32 getIndexCount() const override
	{
		return Indices.size();
	}

	//! Get the axis aligned bounding box
	/** \return Axis aligned bounding box of this buffer. */
	const core::aabbox3d<f32> &getBoundingBox() const override
	{
		return BoundingBox;
	}

	//! Set the axis aligned bounding box
	/** \param box New axis aligned bounding box for this buffer. */
	//! set user axis aligned bounding box
	void setBoundingBox(const core::aabbox3df &box) override
	{
		BoundingBox = box;
	}

	//! Recalculate the bounding box.
	/** should be called if the mesh changed. */
	void recalculateBoundingBox() override
	{
		if (!Vertices.empty()) {
			BoundingBox.reset(Vertices[0].Pos);
			const irr::u32 vsize = Vertices.size();
			for (u32 i = 1; i < vsize; ++i)
				BoundingBox.addInternalPoint(Vertices[i].Pos);
		} else
			BoundingBox.reset(0, 0, 0);
	}

	//! Get type of vertex data stored in this buffer.
	/** \return Type of vertex data. */
	video::E_VERTEX_TYPE getVertexType() const override
	{
		return T::getType();
	}

	//! returns position of vertex i
	const core::vector3df &getPosition(u32 i) const override
	{
		return Vertices[i].Pos;
	}

	//! returns position of vertex i
	core::vector3df &getPosition(u32 i) override
	{
		return Vertices[i].Pos;
	}

	//! returns normal of vertex i
	const core::vector3df &getNormal(u32 i) const override
	{
		return Vertices[i].Normal;
	}

	//! returns normal of vertex i
	core::vector3df &getNormal(u32 i) override
	{
		return Vertices[i].Normal;
	}

	//! returns texture coord of vertex i
	const core::vector2df &getTCoords(u32 i) const override
	{
		return Vertices[i].TCoords;
	}

	//! returns texture coord of vertex i
	core::vector2df &getTCoords(u32 i) override
	{
		return Vertices[i].TCoords;
	}

	//! Append the vertices and indices to the current buffer
	/** Only works for compatible types, i.e. either the same type
	or the main buffer is of standard type. Otherwise, behavior is
	undefined.
	*/
	void append(const void *const vertices, u32 numVertices, const u16 *const indices, u32 numIndices) override
	{
		if (vertices == getVertices())
			return;

		const u32 vertexCount = getVertexCount();
		u32 i;

		Vertices.reallocate(vertexCount + numVertices);
		for (i = 0; i < numVertices; ++i) {
			Vertices.push_back(static_cast<const T *>(vertices)[i]);
			BoundingBox.addInternalPoint(static_cast<const T *>(vertices)[i].Pos);
		}

		Indices.reallocate(getIndexCount() + numIndices);
		for (i = 0; i < numIndices; ++i) {
			Indices.push_back(indices[i] + vertexCount);
		}
	}

	//! get the current hardware mapping hint
	E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const override
	{
		return MappingHint_Vertex;
	}

	//! get the current hardware mapping hint
	E_HARDWARE_MAPPING getHardwareMappingHint_Index() const override
	{
		return MappingHint_Index;
	}

	//! set the hardware mapping hint, for driver
	void setHardwareMappingHint(E_HARDWARE_MAPPING NewMappingHint, E_BUFFER_TYPE Buffer = EBT_VERTEX_AND_INDEX) override
	{
		if (Buffer == EBT_VERTEX_AND_INDEX || Buffer == EBT_VERTEX)
			MappingHint_Vertex = NewMappingHint;
		if (Buffer == EBT_VERTEX_AND_INDEX || Buffer == EBT_INDEX)
			MappingHint_Index = NewMappingHint;
	}

	//! Describe what kind of primitive geometry is used by the meshbuffer
	void setPrimitiveType(E_PRIMITIVE_TYPE type) override
	{
		PrimitiveType = type;
	}

	//! Get the kind of primitive geometry which is used by the meshbuffer
	E_PRIMITIVE_TYPE getPrimitiveType() const override
	{
		return PrimitiveType;
	}

	//! flags the mesh as changed, reloads hardware buffers
	void setDirty(E_BUFFER_TYPE Buffer = EBT_VERTEX_AND_INDEX) override
	{
		if (Buffer == EBT_VERTEX_AND_INDEX || Buffer == EBT_VERTEX)
			++ChangedID_Vertex;
		if (Buffer == EBT_VERTEX_AND_INDEX || Buffer == EBT_INDEX)
			++ChangedID_Index;
	}

	//! Get the currently used ID for identification of changes.
	/** This shouldn't be used for anything outside the VideoDriver. */
	u32 getChangedID_Vertex() const override { return ChangedID_Vertex; }

	//! Get the currently used ID for identification of changes.
	/** This shouldn't be used for anything outside the VideoDriver. */
	u32 getChangedID_Index() const override { return ChangedID_Index; }

	void setHWBuffer(void *ptr) const override
	{
		HWBuffer = ptr;
	}

	void *getHWBuffer() const override
	{
		return HWBuffer;
	}

	u32 ChangedID_Vertex;
	u32 ChangedID_Index;

	//! hardware mapping hint
	E_HARDWARE_MAPPING MappingHint_Vertex;
	E_HARDWARE_MAPPING MappingHint_Index;
	mutable void *HWBuffer;

	//! Material for this meshbuffer.
	video::SMaterial Material;
	//! Vertices of this buffer
	core::array<T> Vertices;
	//! Indices into the vertices of this buffer.
	core::array<u16> Indices;
	//! Bounding box of this meshbuffer.
	core::aabbox3d<f32> BoundingBox;
	//! Primitive type used for rendering (triangles, lines, ...)
	E_PRIMITIVE_TYPE PrimitiveType;
};

//! Standard meshbuffer
typedef CMeshBuffer<video::S3DVertex> SMeshBuffer;
//! Meshbuffer with two texture coords per vertex, e.g. for lightmaps
typedef CMeshBuffer<video::S3DVertex2TCoords> SMeshBufferLightMap;
//! Meshbuffer with vertices having tangents stored, e.g. for normal mapping
typedef CMeshBuffer<video::S3DVertexTangents> SMeshBufferTangents;
} // end namespace scene
} // end namespace irr
