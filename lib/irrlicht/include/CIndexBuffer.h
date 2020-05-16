// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_INDEX_BUFFER_H_INCLUDED__
#define __C_INDEX_BUFFER_H_INCLUDED__

#include "IIndexBuffer.h"

namespace irr
{
namespace scene
{

	class CIndexBuffer : public IIndexBuffer
	{

		class IIndexList
		{
		public:
			virtual ~IIndexList(){};

			virtual u32 stride() const =0;
			virtual u32 size() const =0;
			virtual void push_back(const u32 &element) =0;
			virtual u32 operator [](u32 index) const =0;
			virtual u32 getLast() =0;
			virtual void setValue(u32 index, u32 value) =0;
			virtual void set_used(u32 usedNow) =0;
			virtual void reallocate(u32 new_size) =0;
			virtual u32 allocated_size() const =0;
			virtual void* pointer() =0;
			virtual video::E_INDEX_TYPE getType() const =0;
		};

		template <class T>
		class CSpecificIndexList : public IIndexList
		{
		public:
			core::array<T> Indices;

			virtual u32 stride() const {return sizeof(T);}

			virtual u32 size() const {return Indices.size();}

			virtual void push_back(const u32 &element)
			{
				// push const ref due to compiler problem with gcc 4.6, big endian
				Indices.push_back((const T&)element);
			}

			virtual u32 operator [](u32 index) const
			{
				return (u32)(Indices[index]);
			}

			virtual u32 getLast() {return (u32)Indices.getLast();}

			virtual void setValue(u32 index, u32 value)
			{
				Indices[index]=(T)value;
			}

			virtual void set_used(u32 usedNow)
			{
				Indices.set_used(usedNow);
			}

			virtual void reallocate(u32 new_size)
			{
				Indices.reallocate(new_size);
			}

			virtual u32 allocated_size() const
			{
				return Indices.allocated_size();
			}

			virtual void* pointer() {return Indices.pointer();}

			virtual video::E_INDEX_TYPE getType() const
			{
				if (sizeof(T)==sizeof(u16))
					return video::EIT_16BIT;
				else
					return video::EIT_32BIT;
			}
		};

	public:
		IIndexList *Indices;

		CIndexBuffer(video::E_INDEX_TYPE IndexType) :Indices(0), MappingHint(EHM_NEVER), ChangedID(1)
		{
			setType(IndexType);
		}

		CIndexBuffer(const IIndexBuffer &IndexBufferCopy) :Indices(0), MappingHint(EHM_NEVER), ChangedID(1)
		{
			setType(IndexBufferCopy.getType());
			reallocate(IndexBufferCopy.size());

			for (u32 n=0;n<IndexBufferCopy.size();++n)
				push_back(IndexBufferCopy[n]);
		}

		virtual ~CIndexBuffer()
		{
			delete Indices;
		}

		//virtual void setType(video::E_INDEX_TYPE IndexType);
		virtual void setType(video::E_INDEX_TYPE IndexType)
		{
			IIndexList *NewIndices=0;

			switch (IndexType)
			{
				case video::EIT_16BIT:
				{
					NewIndices=new CSpecificIndexList<u16>;
					break;
				}
				case video::EIT_32BIT:
				{
					NewIndices=new CSpecificIndexList<u32>;
					break;
				}
			}

			if (Indices)
			{
				NewIndices->reallocate( Indices->size() );

				for(u32 n=0;n<Indices->size();++n)
					NewIndices->push_back((*Indices)[n]);

				delete Indices;
			}

			Indices=NewIndices;
		}

		virtual void* getData() {return Indices->pointer();}

		virtual video::E_INDEX_TYPE getType() const {return Indices->getType();}

		virtual u32 stride() const {return Indices->stride();}

		virtual u32 size() const
		{
			return Indices->size();
		}

		virtual void push_back(const u32 &element)
		{
			Indices->push_back(element);
		}

		virtual u32 operator [](u32 index) const
		{
			return (*Indices)[index];
		}

		virtual u32 getLast()
		{
			return Indices->getLast();
		}

		virtual void setValue(u32 index, u32 value)
		{
			Indices->setValue(index, value);
		}

		virtual void set_used(u32 usedNow)
		{
			Indices->set_used(usedNow);
		}

		virtual void reallocate(u32 new_size)
		{
			Indices->reallocate(new_size);
		}

		virtual u32 allocated_size() const
		{
			return Indices->allocated_size();
		}

		virtual void* pointer()
		{
			return Indices->pointer();
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

