// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_DYNAMIC_MESHBUFFER_H_INCLUDED__
#define __C_DYNAMIC_MESHBUFFER_H_INCLUDED__

#include "IDynamicMeshBuffer.h"

#include "CVertexBuffer.h"
#include "CIndexBuffer.h"

namespace irr
{
namespace scene
{

	class CDynamicMeshBuffer: public IDynamicMeshBuffer
	{
	public:
		//! constructor
		CDynamicMeshBuffer(video::E_VERTEX_TYPE vertexType, video::E_INDEX_TYPE indexType)
		{
			VertexBuffer=new CVertexBuffer(vertexType);
			IndexBuffer=new CIndexBuffer(indexType);
		}

		//! destructor
		virtual ~CDynamicMeshBuffer()
		{
			if (VertexBuffer)
				VertexBuffer->drop();
			if (IndexBuffer)
				IndexBuffer->drop();
		}

		virtual IVertexBuffer& getVertexBuffer() const
		{
			return *VertexBuffer;
		}

		virtual IIndexBuffer& getIndexBuffer() const
		{
			return *IndexBuffer;
		}

		virtual void setVertexBuffer(IVertexBuffer *newVertexBuffer)
		{
			if (newVertexBuffer)
				newVertexBuffer->grab();
			if (VertexBuffer)
				VertexBuffer->drop();

			VertexBuffer=newVertexBuffer;
		}

		virtual void setIndexBuffer(IIndexBuffer *newIndexBuffer)
		{
			if (newIndexBuffer)
				newIndexBuffer->grab();
			if (IndexBuffer)
				IndexBuffer->drop();

			IndexBuffer=newIndexBuffer;
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
			if (!getVertexBuffer().size())
				BoundingBox.reset(0,0,0);
			else
			{
				BoundingBox.reset(getVertexBuffer()[0].Pos);
				for (u32 i=1; i<getVertexBuffer().size(); ++i)
					BoundingBox.addInternalPoint(getVertexBuffer()[i].Pos);
			}
		}

		video::SMaterial Material;
		core::aabbox3d<f32> BoundingBox;
	private:
		IVertexBuffer *VertexBuffer;
		IIndexBuffer *IndexBuffer;
	};


} // end namespace scene
} // end namespace irr

#endif

