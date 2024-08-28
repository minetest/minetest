// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IMeshBuffer.h"
#include "CVertexBuffer.h"
#include "CIndexBuffer.h"
#include "S3DVertex.h"
#include "irrArray.h"

namespace irr
{
namespace scene
{

//! A mesh buffer able to choose between S3DVertex2TCoords, S3DVertex and S3DVertexTangents at runtime
struct SSkinMeshBuffer : public IMeshBuffer
{
	//! Default constructor
	SSkinMeshBuffer(video::E_VERTEX_TYPE vt = video::EVT_STANDARD) :
			VertexType(vt), PrimitiveType(EPT_TRIANGLES),
			HWBuffer(nullptr), BoundingBoxNeedsRecalculated(true)
	{
#ifdef _DEBUG
		setDebugName("SSkinMeshBuffer");
#endif
		Vertices_Tangents = new SVertexBufferTangents();
		Vertices_2TCoords = new SVertexBufferLightMap();
		Vertices_Standard = new SVertexBuffer();
		Indices = new SIndexBuffer();
	}

	//! Constructor for standard vertices
	SSkinMeshBuffer(std::vector<video::S3DVertex> &&vertices, std::vector<u16> &&indices) :
			SSkinMeshBuffer()
	{
		Vertices_Standard->Data = std::move(vertices);
		Indices->Data = std::move(indices);
	}

	~SSkinMeshBuffer()
	{
		Vertices_Tangents->drop();
		Vertices_2TCoords->drop();
		Vertices_Standard->drop();
		Indices->drop();
	}

	//! Get Material of this buffer.
	const video::SMaterial &getMaterial() const override
	{
		return Material;
	}

	//! Get Material of this buffer.
	video::SMaterial &getMaterial() override
	{
		return Material;
	}

protected:
	const scene::IVertexBuffer *getVertexBuffer() const
	{
		switch (VertexType) {
		case video::EVT_2TCOORDS:
			return Vertices_2TCoords;
		case video::EVT_TANGENTS:
			return Vertices_Tangents;
		default:
			return Vertices_Standard;
		}
	}

	scene::IVertexBuffer *getVertexBuffer()
	{
		switch (VertexType) {
		case video::EVT_2TCOORDS:
			return Vertices_2TCoords;
		case video::EVT_TANGENTS:
			return Vertices_Tangents;
		default:
			return Vertices_Standard;
		}
	}
public:

	//! Get standard vertex at given index
	virtual video::S3DVertex *getVertex(u32 index)
	{
		switch (VertexType) {
		case video::EVT_2TCOORDS:
			return &Vertices_2TCoords->Data[index];
		case video::EVT_TANGENTS:
			return &Vertices_Tangents->Data[index];
		default:
			return &Vertices_Standard->Data[index];
		}
	}

	//! Get pointer to vertex array
	const void *getVertices() const override
	{
		return getVertexBuffer()->getData();
	}

	//! Get pointer to vertex array
	void *getVertices() override
	{
		return getVertexBuffer()->getData();
	}

	//! Get vertex count
	u32 getVertexCount() const override
	{
		return getVertexBuffer()->getCount();
	}

	//! Get type of index data which is stored in this meshbuffer.
	/** \return Index type of this buffer. */
	video::E_INDEX_TYPE getIndexType() const override
	{
		return Indices->getType();
	}

	//! Get pointer to index array
	const u16 *getIndices() const override
	{
		return static_cast<const u16*>(Indices->getData());
	}

	//! Get pointer to index array
	u16 *getIndices() override
	{
		return static_cast<u16*>(Indices->getData());
	}

	//! Get index count
	u32 getIndexCount() const override
	{
		return Indices->getCount();
	}

	//! Get bounding box
	const core::aabbox3d<f32> &getBoundingBox() const override
	{
		return BoundingBox;
	}

	//! Set bounding box
	void setBoundingBox(const core::aabbox3df &box) override
	{
		BoundingBox = box;
	}

	//! Recalculate bounding box
	void recalculateBoundingBox() override
	{
		if (!BoundingBoxNeedsRecalculated)
			return;

		BoundingBoxNeedsRecalculated = false;

		switch (VertexType) {
		case video::EVT_STANDARD: {
			if (!Vertices_Standard->getCount())
				BoundingBox.reset(0, 0, 0);
			else {
				auto &vertices = Vertices_Standard->Data;
				BoundingBox.reset(vertices[0].Pos);
				for (size_t i = 1; i < vertices.size(); ++i)
					BoundingBox.addInternalPoint(vertices[i].Pos);
			}
			break;
		}
		case video::EVT_2TCOORDS: {
			if (!Vertices_2TCoords->getCount())
				BoundingBox.reset(0, 0, 0);
			else {
				auto &vertices = Vertices_2TCoords->Data;
				BoundingBox.reset(vertices[0].Pos);
				for (size_t i = 1; i < vertices.size(); ++i)
					BoundingBox.addInternalPoint(vertices[i].Pos);
			}
			break;
		}
		case video::EVT_TANGENTS: {
			if (!Vertices_Tangents->getCount())
				BoundingBox.reset(0, 0, 0);
			else {
				auto &vertices = Vertices_Tangents->Data;
				BoundingBox.reset(vertices[0].Pos);
				for (size_t i = 1; i < vertices.size(); ++i)
					BoundingBox.addInternalPoint(vertices[i].Pos);
			}
			break;
		}
		}
	}

	//! Get vertex type
	video::E_VERTEX_TYPE getVertexType() const override
	{
		return VertexType;
	}

	//! Convert to 2tcoords vertex type
	void convertTo2TCoords()
	{
		if (VertexType == video::EVT_STANDARD) {
			video::S3DVertex2TCoords Vertex;
			for (const auto &Vertex_Standard : Vertices_Standard->Data) {
				Vertex.Color = Vertex_Standard.Color;
				Vertex.Pos = Vertex_Standard.Pos;
				Vertex.Normal = Vertex_Standard.Normal;
				Vertex.TCoords = Vertex_Standard.TCoords;
				Vertices_2TCoords->Data.push_back(Vertex);
			}
			Vertices_Standard->Data.clear();
			VertexType = video::EVT_2TCOORDS;
		}
	}

	//! Convert to tangents vertex type
	void convertToTangents()
	{
		if (VertexType == video::EVT_STANDARD) {
				video::S3DVertexTangents Vertex;
			for (const auto &Vertex_Standard : Vertices_Standard->Data) {
				Vertex.Color = Vertex_Standard.Color;
				Vertex.Pos = Vertex_Standard.Pos;
				Vertex.Normal = Vertex_Standard.Normal;
				Vertex.TCoords = Vertex_Standard.TCoords;
				Vertices_Tangents->Data.push_back(Vertex);
			}
			Vertices_Standard->Data.clear();
			VertexType = video::EVT_TANGENTS;
		} else if (VertexType == video::EVT_2TCOORDS) {
			video::S3DVertexTangents Vertex;
			for (const auto &Vertex_2TCoords : Vertices_2TCoords->Data) {
				Vertex.Color = Vertex_2TCoords.Color;
				Vertex.Pos = Vertex_2TCoords.Pos;
				Vertex.Normal = Vertex_2TCoords.Normal;
				Vertex.TCoords = Vertex_2TCoords.TCoords;
				Vertices_Tangents->Data.push_back(Vertex);
			}
			Vertices_2TCoords->Data.clear();
			VertexType = video::EVT_TANGENTS;
		}
	}

	//! returns position of vertex i
	const core::vector3df &getPosition(u32 i) const override
	{
		return getVertexBuffer()->getPosition(i);
	}

	//! returns position of vertex i
	core::vector3df &getPosition(u32 i) override
	{
		return getVertexBuffer()->getPosition(i);
	}

	//! returns normal of vertex i
	const core::vector3df &getNormal(u32 i) const override
	{
		return getVertexBuffer()->getNormal(i);
	}

	//! returns normal of vertex i
	core::vector3df &getNormal(u32 i) override
	{
		return getVertexBuffer()->getNormal(i);
	}

	//! returns texture coords of vertex i
	const core::vector2df &getTCoords(u32 i) const override
	{
		return getVertexBuffer()->getTCoords(i);
	}

	//! returns texture coords of vertex i
	core::vector2df &getTCoords(u32 i) override
	{
		return getVertexBuffer()->getTCoords(i);
	}

	//! append the vertices and indices to the current buffer
	void append(const void *const vertices, u32 numVertices, const u16 *const indices, u32 numIndices) override
	{
		_IRR_DEBUG_BREAK_IF(true);
	}

	//! get the current hardware mapping hint for vertex buffers
	E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const override
	{
		return getVertexBuffer()->getHardwareMappingHint();
	}

	//! get the current hardware mapping hint for index buffers
	E_HARDWARE_MAPPING getHardwareMappingHint_Index() const override
	{
		return Indices->getHardwareMappingHint();
	}

	//! set the hardware mapping hint, for driver
	void setHardwareMappingHint(E_HARDWARE_MAPPING NewMappingHint, E_BUFFER_TYPE Buffer = EBT_VERTEX_AND_INDEX) override
	{
		if (Buffer == EBT_VERTEX || Buffer == EBT_VERTEX_AND_INDEX)
			getVertexBuffer()->setHardwareMappingHint(NewMappingHint);
		if (Buffer == EBT_INDEX || Buffer == EBT_VERTEX_AND_INDEX)
			Indices->setHardwareMappingHint(NewMappingHint);
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
			getVertexBuffer()->setDirty();
		if (Buffer == EBT_VERTEX_AND_INDEX || Buffer == EBT_INDEX)
			Indices->setDirty();
	}

	u32 getChangedID_Vertex() const override
	{
		return getVertexBuffer()->getChangedID();
	}

	u32 getChangedID_Index() const override
	{
		return Indices->getChangedID();
	}

	void setHWBuffer(void *ptr) const override
	{
		HWBuffer = ptr;
	}

	void *getHWBuffer() const override
	{
		return HWBuffer;
	}

	//! Call this after changing the positions of any vertex.
	void boundingBoxNeedsRecalculated(void) { BoundingBoxNeedsRecalculated = true; }

	SVertexBufferTangents *Vertices_Tangents;
	SVertexBufferLightMap *Vertices_2TCoords;
	SVertexBuffer *Vertices_Standard;
	SIndexBuffer *Indices;

	core::matrix4 Transformation;

	video::SMaterial Material;
	video::E_VERTEX_TYPE VertexType;

	core::aabbox3d<f32> BoundingBox;

	//! Primitive type used for rendering (triangles, lines, ...)
	E_PRIMITIVE_TYPE PrimitiveType;

	mutable void *HWBuffer;

	bool BoundingBoxNeedsRecalculated;
};

} // end namespace scene
} // end namespace irr
