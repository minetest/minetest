// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_VERTEX_BUFFER_H_INCLUDED__
#define __C_VERTEX_BUFFER_H_INCLUDED__

#include "IVertexBuffer.h"


namespace irr
{
namespace scene
{

	class CVertexBuffer : public IVertexBuffer
	{
		class IVertexList
		{
		public:
			virtual ~IVertexList(){};

			virtual u32 stride() const =0;

			virtual u32 size() const =0;

			virtual void push_back (const video::S3DVertex &element) =0;
			virtual video::S3DVertex& operator [](const u32 index) const =0;
			virtual video::S3DVertex& getLast() =0;
			virtual void set_used(u32 usedNow) =0;
			virtual void reallocate(u32 new_size) =0;
			virtual u32 allocated_size() const =0;
			virtual video::S3DVertex* pointer() =0;
			virtual video::E_VERTEX_TYPE getType() const =0;
		};

		template <class T>
		class CSpecificVertexList : public IVertexList
		{
		public:
			core::array<T> Vertices;

			virtual u32 stride() const {return sizeof(T);}

			virtual u32 size() const {return Vertices.size();}

			virtual void push_back (const video::S3DVertex &element)
			{Vertices.push_back((T&)element);}

			virtual video::S3DVertex& operator [](const u32 index) const
			{return (video::S3DVertex&)Vertices[index];}

			virtual video::S3DVertex& getLast()
			{return (video::S3DVertex&)Vertices.getLast();}

			virtual void set_used(u32 usedNow)
			{Vertices.set_used(usedNow);}

			virtual void reallocate(u32 new_size)
			{Vertices.reallocate(new_size);}

			virtual u32 allocated_size() const
			{
				return Vertices.allocated_size();
			}

			virtual video::S3DVertex* pointer() {return Vertices.pointer();}

			virtual video::E_VERTEX_TYPE getType() const {return T().getType();}
		};

	public:
		IVertexList *Vertices;

		CVertexBuffer(video::E_VERTEX_TYPE vertexType) : Vertices(0),
				MappingHint(EHM_NEVER), ChangedID(1)
		{
			setType(vertexType);
		}

		CVertexBuffer(const IVertexBuffer &VertexBufferCopy) :
				Vertices(0), MappingHint(EHM_NEVER),
				ChangedID(1)
		{
			setType(VertexBufferCopy.getType());
			reallocate(VertexBufferCopy.size());

			for (u32 n=0;n<VertexBufferCopy.size();++n)
				push_back(VertexBufferCopy[n]);
		}

		virtual ~CVertexBuffer()
		{
			delete Vertices;
		}


		virtual void setType(video::E_VERTEX_TYPE vertexType)
		{
			IVertexList *NewVertices=0;

			switch (vertexType)
			{
				case video::EVT_STANDARD:
				{
					NewVertices=new CSpecificVertexList<video::S3DVertex>;
					break;
				}
				case video::EVT_2TCOORDS:
				{
					NewVertices=new CSpecificVertexList<video::S3DVertex2TCoords>;
					break;
				}
				case video::EVT_TANGENTS:
				{
					NewVertices=new CSpecificVertexList<video::S3DVertexTangents>;
					break;
				}
			}
			if (Vertices)
			{
				NewVertices->reallocate( Vertices->size() );

				for(u32 n=0;n<Vertices->size();++n)
					NewVertices->push_back((*Vertices)[n]);

				delete Vertices;
			}

			Vertices=NewVertices;
		}

		virtual void* getData() {return Vertices->pointer();}

		virtual video::E_VERTEX_TYPE getType() const {return Vertices->getType();}

		virtual u32 stride() const {return Vertices->stride();}

		virtual u32 size() const
		{
			return Vertices->size();
		}

		virtual void push_back (const video::S3DVertex &element)
		{
			Vertices->push_back(element);
		}

		virtual video::S3DVertex& operator [](const u32 index) const
		{
			return (*Vertices)[index];
		}

		virtual video::S3DVertex& getLast()
		{
			return Vertices->getLast();
		}

		virtual void set_used(u32 usedNow)
		{
			Vertices->set_used(usedNow);
		}

		virtual void reallocate(u32 new_size)
		{
			Vertices->reallocate(new_size);
		}

		virtual u32 allocated_size() const
		{
			return Vertices->allocated_size();
		}

		virtual video::S3DVertex* pointer()
		{
			return Vertices->pointer();
		}

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint() const
		{
			return MappingHint;
		}

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint( E_HARDWARE_MAPPING NewMappingHint )
		{
			MappingHint=NewMappingHint;
		}

		//! flags the mesh as changed, reloads hardware buffers
		virtual void setDirty()
		{
			++ChangedID;
		}

		//! Get the currently used ID for identification of changes.
		/** This shouldn't be used for anything outside the VideoDriver. */
		virtual u32 getChangedID() const {return ChangedID;}

		E_HARDWARE_MAPPING MappingHint;
		u32 ChangedID;
	};


} // end namespace scene
} // end namespace irr

#endif

