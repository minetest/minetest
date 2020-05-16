// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_DYNAMIC_MESH_BUFFER_H_INCLUDED__
#define __I_DYNAMIC_MESH_BUFFER_H_INCLUDED__

#include "IMeshBuffer.h"
#include "IVertexBuffer.h"
#include "IIndexBuffer.h"

namespace irr
{
namespace scene
{

	/** a dynamic meshBuffer */
	class IDynamicMeshBuffer : public IMeshBuffer
	{
	public:
		virtual IVertexBuffer &getVertexBuffer() const =0;
		virtual IIndexBuffer &getIndexBuffer() const =0;

		virtual void setVertexBuffer(IVertexBuffer *vertexBuffer) =0;
		virtual void setIndexBuffer(IIndexBuffer *indexBuffer) =0;

		//! Get the material of this meshbuffer
		/** \return Material of this buffer. */
		virtual video::SMaterial& getMaterial() =0;

		//! Get the material of this meshbuffer
		/** \return Material of this buffer. */
		virtual const video::SMaterial& getMaterial() const =0;

		//! Get the axis aligned bounding box of this meshbuffer.
		/** \return Axis aligned bounding box of this buffer. */
		virtual const core::aabbox3df& getBoundingBox() const =0;

		//! Set axis aligned bounding box
		/** \param box User defined axis aligned bounding box to use
		for this buffer. */
		virtual void setBoundingBox(const core::aabbox3df& box) =0;

		//! Recalculates the bounding box. Should be called if the mesh changed.
		virtual void recalculateBoundingBox() =0;

		//! Append the vertices and indices to the current buffer
		/** Only works for compatible vertex types.
		\param vertices Pointer to a vertex array.
		\param numVertices Number of vertices in the array.
		\param indices Pointer to index array.
		\param numIndices Number of indices in array. */
		virtual void append(const void* const vertices, u32 numVertices, const u16* const indices, u32 numIndices)
		{

		}

		//! Append the meshbuffer to the current buffer
		/** Only works for compatible vertex types
		\param other Buffer to append to this one. */
		virtual void append(const IMeshBuffer* const other)
		{

		}

		// ------------------- To be removed? -------------------  //

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint_Vertex() const
		{
			return getVertexBuffer().getHardwareMappingHint();
		}

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint_Index() const
		{
			return getIndexBuffer().getHardwareMappingHint();
		}

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint( E_HARDWARE_MAPPING NewMappingHint, E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX )
		{
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_VERTEX)
				getVertexBuffer().setHardwareMappingHint(NewMappingHint);
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_INDEX)
				getIndexBuffer().setHardwareMappingHint(NewMappingHint);
		}

		//! flags the mesh as changed, reloads hardware buffers
		virtual void setDirty(E_BUFFER_TYPE Buffer=EBT_VERTEX_AND_INDEX)
		{
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_VERTEX)
				getVertexBuffer().setDirty();
			if (Buffer==EBT_VERTEX_AND_INDEX || Buffer==EBT_INDEX)
				getIndexBuffer().setDirty();
		}

		virtual u32 getChangedID_Vertex() const
		{
			return getVertexBuffer().getChangedID();
		}

		virtual u32 getChangedID_Index() const
		{
			return getIndexBuffer().getChangedID();
		}

		// ------------------- Old interface -------------------  //

		//! Get type of vertex data which is stored in this meshbuffer.
		/** \return Vertex type of this buffer. */
		virtual video::E_VERTEX_TYPE getVertexType() const
		{
			return getVertexBuffer().getType();
		}

		//! Get access to vertex data. The data is an array of vertices.
		/** Which vertex type is used can be determined by getVertexType().
		\return Pointer to array of vertices. */
		virtual const void* getVertices() const
		{
			return getVertexBuffer().getData();
		}

		//! Get access to vertex data. The data is an array of vertices.
		/** Which vertex type is used can be determined by getVertexType().
		\return Pointer to array of vertices. */
		virtual void* getVertices()
		{
			return getVertexBuffer().getData();
		}

		//! Get amount of vertices in meshbuffer.
		/** \return Number of vertices in this buffer. */
		virtual u32 getVertexCount() const
		{
			return getVertexBuffer().size();
		}

		//! Get type of index data which is stored in this meshbuffer.
		/** \return Index type of this buffer. */
		virtual video::E_INDEX_TYPE getIndexType() const
		{
			return getIndexBuffer().getType();
		}

		//! Get access to Indices.
		/** \return Pointer to indices array. */
		virtual const u16* getIndices() const
		{
			return (u16*)getIndexBuffer().getData();
		}

		//! Get access to Indices.
		/** \return Pointer to indices array. */
		virtual u16* getIndices()
		{
			return (u16*)getIndexBuffer().getData();
		}

		//! Get amount of indices in this meshbuffer.
		/** \return Number of indices in this buffer. */
		virtual u32 getIndexCount() const
		{
			return getIndexBuffer().size();
		}

		//! returns position of vertex i
		virtual const core::vector3df& getPosition(u32 i) const
		{
			return getVertexBuffer()[i].Pos;
		}

		//! returns position of vertex i
		virtual core::vector3df& getPosition(u32 i)
		{
			return getVertexBuffer()[i].Pos;
		}

		//! returns texture coords of vertex i
		virtual const core::vector2df& getTCoords(u32 i) const
		{
			return getVertexBuffer()[i].TCoords;
		}

		//! returns texture coords of vertex i
		virtual core::vector2df& getTCoords(u32 i)
		{
			return getVertexBuffer()[i].TCoords;
		}

		//! returns normal of vertex i
		virtual const core::vector3df& getNormal(u32 i) const
		{
			return getVertexBuffer()[i].Normal;
		}

		//! returns normal of vertex i
		virtual core::vector3df& getNormal(u32 i)
		{
			return getVertexBuffer()[i].Normal;
		}
	};


} // end namespace scene
} // end namespace irr

#endif


