// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __T_MESH_BUFFER_H_INCLUDED__
#define __T_MESH_BUFFER_H_INCLUDED__

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
		CMeshBuffer():ChangedID_Vertex(1),ChangedID_Index(1),MappingHint_Vertex(EHM_NEVER), MappingHint_Index(EHM_NEVER)
		{
			#ifdef _DEBUG
			setDebugName("SMeshBuffer");
			#endif
		}


		//! Get material of this meshbuffer
		/** \return Material of this buffer */
		virtual const video::SMaterial& getMaterial() const
		{
			return Material;
		}


		//! Get material of this meshbuffer
		/** \return Material of this buffer */
		virtual video::SMaterial& getMaterial()
		{
			return Material;
		}


		//! Get pointer to vertices
		/** \return Pointer to vertices. */
		virtual const void* getVertices() const
		{
			return Vertices.const_pointer();
		}


		//! Get pointer to vertices
		/** \return Pointer to vertices. */
		virtual void* getVertices()
		{
			return Vertices.pointer();
		}


		//! Get number of vertices
		/** \return Number of vertices. */
		virtual u32 getVertexCount() const
		{
			return Vertices.size();
		}

		//! Get type of index data which is stored in this meshbuffer.
		/** \return Index type of this buffer. */
		virtual video::E_INDEX_TYPE getIndexType() const
		{
			return video::EIT_16BIT;
		}

		//! Get pointer to indices
		/** \return Pointer to indices. */
		virtual const u16* getIndices() const
		{
			return Indices.const_pointer();
		}


		//! Get pointer to indices
		/** \return Pointer to indices. */
		virtual u16* getIndices()
		{
			return Indices.pointer();
		}


		//! Get number of indices
		/** \return Number of indices. */
		virtual u32 getIndexCount() const
		{
			return Indices.size();
		}


		//! Get the axis aligned bounding box
		/** \return Axis aligned bounding box of this buffer. */
		virtual const core::aabbox3d<f32>& getBoundingBox() const
		{
			return BoundingBox;
		}


		//! Set the axis aligned bounding box
		/** \param box New axis aligned bounding box for this buffer. */
		//! set user axis aligned bounding box
		virtual void setBoundingBox(const core::aabbox3df& box)
		{
			BoundingBox = box;
		}


		//! Recalculate the bounding box.
		/** should be called if the mesh changed. */
		virtual void recalculateBoundingBox()
		{
			if (Vertices.empty())
				BoundingBox.reset(0,0,0);
			else
			{
				BoundingBox.reset(Vertices[0].Pos);
				for (u32 i=1; i<Vertices.size(); ++i)
					BoundingBox.addInternalPoint(Vertices[i].Pos);
			}
		}


		//! Get type of vertex data stored in this buffer.
		/** \return Type of vertex data. */
		virtual video::E_VERTEX_TYPE getVertexType() const
		{
			return T().getType();
		}

		//! returns position of vertex i
		virtual const core::vector3df& getPosition(u32 i) const
		{
			return Vertices[i].Pos;
		}

		//! returns position of vertex i
		virtual core::vector3df& getPosition(u32 i)
		{
			return Vertices[i].Pos;
		}

		//! returns normal of vertex i
		virtual const core::vector3df& getNormal(u32 i) const
		{
			return Vertices[i].Normal;
		}

		//! returns normal of vertex i
		virtual core::vector3df& getNormal(u32 i)
		{
			return Vertices[i].Normal;
		}

		//! returns texture coord of vertex i
		virtual const core::vector2df& getTCoords(u32 i) const
		{
			return Vertices[i].TCoords;
		}

		//! returns texture coord of vertex i
		virtual core::vector2df& getTCoords(u32 i)
		{
			return Vertices[i].TCoords;
		}


		//! Append the vertices and indices to the current buffer
		/** Only works for compatible types, i.e. either the same type
		or the main buffer is of standard type. Otherwise, behavior is
		undefined.
		*/
		virtual void append(const void* const vertices, u32 numVertices, const u16* const indices, u32 numIndices)
		{
			if (vertices == getVertices())
				return;

			const u32 vertexCount = getVertexCount();
			u32 i;

			Vertices.reallocate(vertexCount+numVertices);
			for (i=0; i<numVertices; ++i)
			{
				Vertices.push_back(reinterpret_cast<const T*>(vertices)[i]);
				BoundingBox.addInternalPoint(reinterpret_cast<const T*>(vertices)[i].Pos);
			}

			Indices.reallocate(getIndexCount()+numIndices);
			for (i=0; i<numIndices; ++i)
			{
				Indices.push_back(indices[i]+vertexCount);
			}
		}


		//! Append the meshbuffer to the current buffer
		/** Only works for compatible types, i.e. either the same type
		or the main buffer is of standard type. Otherwise, behavior is
		undefined.
		\param other Meshbuffer to be appended to this one.
		*/
		virtual void append(const IMeshBuffer* const other)
		{
			/*
			if (this==other)
				return;

			const u32 vertexCount = getVertexCount();
			u32 i;

			Vertices.reallocate(vertexCount+other->getVertexCount());
			for (i=0; i<other->getVertexCount(); ++i)
			{
				Vertices.push_back(reinterpret_cast<const T*>(other->getVertices())[i]);
			}

			Indices.reallocate(getIndexCount()+other->getIndexCount());
			for (i=0; i<other->getIndexCount(); ++i)
			{
				Indices.push_back(other->getIndices()[i]+vertexCount);
			}
			BoundingBox.addInternalBox(other->getBoundingBox());
			*/
		}


		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const
		{
			return MappingHint_Vertex;
		}

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint_Index() const
		{
			return MappingHint_Index;
		}

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint( E_HARDWARE_MAPPING NewMappingHint, E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX )
		{
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_VERTEX)
				MappingHint_Vertex=NewMappingHint;
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_INDEX)
				MappingHint_Index=NewMappingHint;
		}


		//! flags the mesh as changed, reloads hardware buffers
		virtual void setDirty(E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX)
		{
			if (Buffer==EBT_VERTEX_AND_INDEX ||Buffer==EBT_VERTEX)
				++ChangedID_Vertex;
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_INDEX)
				++ChangedID_Index;
		}

		//! Get the currently used ID for identification of changes.
		/** This shouldn't be used for anything outside the VideoDriver. */
		virtual u32 getChangedID_Vertex() const {return ChangedID_Vertex;}

		//! Get the currently used ID for identification of changes.
		/** This shouldn't be used for anything outside the VideoDriver. */
		virtual u32 getChangedID_Index() const {return ChangedID_Index;}

		u32 ChangedID_Vertex;
		u32 ChangedID_Index;

		//! hardware mapping hint
		E_HARDWARE_MAPPING MappingHint_Vertex;
		E_HARDWARE_MAPPING MappingHint_Index;

		//! Material for this meshbuffer.
		video::SMaterial Material;
		//! Vertices of this buffer
		core::array<T> Vertices;
		//! Indices into the vertices of this buffer.
		core::array<u16> Indices;
		//! Bounding box of this meshbuffer.
		core::aabbox3d<f32> BoundingBox;
	};

	//! Standard meshbuffer
	typedef CMeshBuffer<video::S3DVertex> SMeshBuffer;
	//! Meshbuffer with two texture coords per vertex, e.g. for lightmaps
	typedef CMeshBuffer<video::S3DVertex2TCoords> SMeshBufferLightMap;
	//! Meshbuffer with vertices having tangents stored, e.g. for normal mapping
	typedef CMeshBuffer<video::S3DVertexTangents> SMeshBufferTangents;
} // end namespace scene
} // end namespace irr

#endif


