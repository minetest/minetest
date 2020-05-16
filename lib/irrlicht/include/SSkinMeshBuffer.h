// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SKIN_MESH_BUFFER_H_INCLUDED__
#define __I_SKIN_MESH_BUFFER_H_INCLUDED__

#include "IMeshBuffer.h"
#include "S3DVertex.h"


namespace irr
{
namespace scene
{


//! A mesh buffer able to choose between S3DVertex2TCoords, S3DVertex and S3DVertexTangents at runtime
struct SSkinMeshBuffer : public IMeshBuffer
{
	//! Default constructor
	SSkinMeshBuffer(video::E_VERTEX_TYPE vt=video::EVT_STANDARD) :
		ChangedID_Vertex(1), ChangedID_Index(1), VertexType(vt),
		MappingHint_Vertex(EHM_NEVER), MappingHint_Index(EHM_NEVER),
		BoundingBoxNeedsRecalculated(true)
	{
		#ifdef _DEBUG
		setDebugName("SSkinMeshBuffer");
		#endif
	}

	//! Get Material of this buffer.
	virtual const video::SMaterial& getMaterial() const
	{
		return Material;
	}

	//! Get Material of this buffer.
	virtual video::SMaterial& getMaterial()
	{
		return Material;
	}

	//! Get standard vertex at given index
	virtual video::S3DVertex *getVertex(u32 index)
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return (video::S3DVertex*)&Vertices_2TCoords[index];
			case video::EVT_TANGENTS:
				return (video::S3DVertex*)&Vertices_Tangents[index];
			default:
				return &Vertices_Standard[index];
		}
	}

	//! Get pointer to vertex array
	virtual const void* getVertices() const
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords.const_pointer();
			case video::EVT_TANGENTS:
				return Vertices_Tangents.const_pointer();
			default:
				return Vertices_Standard.const_pointer();
		}
	}

	//! Get pointer to vertex array
	virtual void* getVertices()
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords.pointer();
			case video::EVT_TANGENTS:
				return Vertices_Tangents.pointer();
			default:
				return Vertices_Standard.pointer();
		}
	}

	//! Get vertex count
	virtual u32 getVertexCount() const
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords.size();
			case video::EVT_TANGENTS:
				return Vertices_Tangents.size();
			default:
				return Vertices_Standard.size();
		}
	}

	//! Get type of index data which is stored in this meshbuffer.
	/** \return Index type of this buffer. */
	virtual video::E_INDEX_TYPE getIndexType() const
	{
		return video::EIT_16BIT;
	}

	//! Get pointer to index array
	virtual const u16* getIndices() const
	{
		return Indices.const_pointer();
	}

	//! Get pointer to index array
	virtual u16* getIndices()
	{
		return Indices.pointer();
	}

	//! Get index count
	virtual u32 getIndexCount() const
	{
		return Indices.size();
	}

	//! Get bounding box
	virtual const core::aabbox3d<f32>& getBoundingBox() const
	{
		return BoundingBox;
	}

	//! Set bounding box
	virtual void setBoundingBox( const core::aabbox3df& box)
	{
		BoundingBox = box;
	}

	//! Recalculate bounding box
	virtual void recalculateBoundingBox()
	{
		if(!BoundingBoxNeedsRecalculated)
			return;

		BoundingBoxNeedsRecalculated = false;

		switch (VertexType)
		{
			case video::EVT_STANDARD:
			{
				if (Vertices_Standard.empty())
					BoundingBox.reset(0,0,0);
				else
				{
					BoundingBox.reset(Vertices_Standard[0].Pos);
					for (u32 i=1; i<Vertices_Standard.size(); ++i)
						BoundingBox.addInternalPoint(Vertices_Standard[i].Pos);
				}
				break;
			}
			case video::EVT_2TCOORDS:
			{
				if (Vertices_2TCoords.empty())
					BoundingBox.reset(0,0,0);
				else
				{
					BoundingBox.reset(Vertices_2TCoords[0].Pos);
					for (u32 i=1; i<Vertices_2TCoords.size(); ++i)
						BoundingBox.addInternalPoint(Vertices_2TCoords[i].Pos);
				}
				break;
			}
			case video::EVT_TANGENTS:
			{
				if (Vertices_Tangents.empty())
					BoundingBox.reset(0,0,0);
				else
				{
					BoundingBox.reset(Vertices_Tangents[0].Pos);
					for (u32 i=1; i<Vertices_Tangents.size(); ++i)
						BoundingBox.addInternalPoint(Vertices_Tangents[i].Pos);
				}
				break;
			}
		}
	}

	//! Get vertex type
	virtual video::E_VERTEX_TYPE getVertexType() const
	{
		return VertexType;
	}

	//! Convert to 2tcoords vertex type
	virtual void convertTo2TCoords()
	{
		if (VertexType==video::EVT_STANDARD)
		{
			for(u32 n=0;n<Vertices_Standard.size();++n)
			{
				video::S3DVertex2TCoords Vertex;
				Vertex.Color=Vertices_Standard[n].Color;
				Vertex.Pos=Vertices_Standard[n].Pos;
				Vertex.Normal=Vertices_Standard[n].Normal;
				Vertex.TCoords=Vertices_Standard[n].TCoords;
				Vertices_2TCoords.push_back(Vertex);
			}
			Vertices_Standard.clear();
			VertexType=video::EVT_2TCOORDS;
		}
	}

	//! Convert to tangents vertex type
	virtual void convertToTangents()
	{
		if (VertexType==video::EVT_STANDARD)
		{
			for(u32 n=0;n<Vertices_Standard.size();++n)
			{
				video::S3DVertexTangents Vertex;
				Vertex.Color=Vertices_Standard[n].Color;
				Vertex.Pos=Vertices_Standard[n].Pos;
				Vertex.Normal=Vertices_Standard[n].Normal;
				Vertex.TCoords=Vertices_Standard[n].TCoords;
				Vertices_Tangents.push_back(Vertex);
			}
			Vertices_Standard.clear();
			VertexType=video::EVT_TANGENTS;
		}
		else if (VertexType==video::EVT_2TCOORDS)
		{
			for(u32 n=0;n<Vertices_2TCoords.size();++n)
			{
				video::S3DVertexTangents Vertex;
				Vertex.Color=Vertices_2TCoords[n].Color;
				Vertex.Pos=Vertices_2TCoords[n].Pos;
				Vertex.Normal=Vertices_2TCoords[n].Normal;
				Vertex.TCoords=Vertices_2TCoords[n].TCoords;
				Vertices_Tangents.push_back(Vertex);
			}
			Vertices_2TCoords.clear();
			VertexType=video::EVT_TANGENTS;
		}
	}

	//! returns position of vertex i
	virtual const core::vector3df& getPosition(u32 i) const
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords[i].Pos;
			case video::EVT_TANGENTS:
				return Vertices_Tangents[i].Pos;
			default:
				return Vertices_Standard[i].Pos;
		}
	}

	//! returns position of vertex i
	virtual core::vector3df& getPosition(u32 i)
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords[i].Pos;
			case video::EVT_TANGENTS:
				return Vertices_Tangents[i].Pos;
			default:
				return Vertices_Standard[i].Pos;
		}
	}

	//! returns normal of vertex i
	virtual const core::vector3df& getNormal(u32 i) const
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords[i].Normal;
			case video::EVT_TANGENTS:
				return Vertices_Tangents[i].Normal;
			default:
				return Vertices_Standard[i].Normal;
		}
	}

	//! returns normal of vertex i
	virtual core::vector3df& getNormal(u32 i)
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords[i].Normal;
			case video::EVT_TANGENTS:
				return Vertices_Tangents[i].Normal;
			default:
				return Vertices_Standard[i].Normal;
		}
	}

	//! returns texture coords of vertex i
	virtual const core::vector2df& getTCoords(u32 i) const
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords[i].TCoords;
			case video::EVT_TANGENTS:
				return Vertices_Tangents[i].TCoords;
			default:
				return Vertices_Standard[i].TCoords;
		}
	}

	//! returns texture coords of vertex i
	virtual core::vector2df& getTCoords(u32 i)
	{
		switch (VertexType)
		{
			case video::EVT_2TCOORDS:
				return Vertices_2TCoords[i].TCoords;
			case video::EVT_TANGENTS:
				return Vertices_Tangents[i].TCoords;
			default:
				return Vertices_Standard[i].TCoords;
		}
	}

	//! append the vertices and indices to the current buffer
	virtual void append(const void* const vertices, u32 numVertices, const u16* const indices, u32 numIndices) {}

	//! append the meshbuffer to the current buffer
	virtual void append(const IMeshBuffer* const other) {}

	//! get the current hardware mapping hint for vertex buffers
	virtual E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const
	{
		return MappingHint_Vertex;
	}

	//! get the current hardware mapping hint for index buffers
	virtual E_HARDWARE_MAPPING getHardwareMappingHint_Index() const
	{
		return MappingHint_Index;
	}

	//! set the hardware mapping hint, for driver
	virtual void setHardwareMappingHint( E_HARDWARE_MAPPING NewMappingHint, E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX )
	{
		if (Buffer==EBT_VERTEX)
			MappingHint_Vertex=NewMappingHint;
		else if (Buffer==EBT_INDEX)
			MappingHint_Index=NewMappingHint;
		else if (Buffer==EBT_VERTEX_AND_INDEX)
		{
			MappingHint_Vertex=NewMappingHint;
			MappingHint_Index=NewMappingHint;
		}
	}

	//! flags the mesh as changed, reloads hardware buffers
	virtual void setDirty(E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX)
	{
		if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_VERTEX)
			++ChangedID_Vertex;
		if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_INDEX)
			++ChangedID_Index;
	}

	virtual u32 getChangedID_Vertex() const {return ChangedID_Vertex;}

	virtual u32 getChangedID_Index() const {return ChangedID_Index;}

	//! Call this after changing the positions of any vertex.
	void boundingBoxNeedsRecalculated(void) { BoundingBoxNeedsRecalculated = true; }

	core::array<video::S3DVertexTangents> Vertices_Tangents;
	core::array<video::S3DVertex2TCoords> Vertices_2TCoords;
	core::array<video::S3DVertex> Vertices_Standard;
	core::array<u16> Indices;

	u32 ChangedID_Vertex;
	u32 ChangedID_Index;

	//ISkinnedMesh::SJoint *AttachedJoint;
	core::matrix4 Transformation;

	video::SMaterial Material;
	video::E_VERTEX_TYPE VertexType;

	core::aabbox3d<f32> BoundingBox;

	// hardware mapping hint
	E_HARDWARE_MAPPING MappingHint_Vertex:3;
	E_HARDWARE_MAPPING MappingHint_Index:3;

	bool BoundingBoxNeedsRecalculated:1;
};


} // end namespace scene
} // end namespace irr

#endif

