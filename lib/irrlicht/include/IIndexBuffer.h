// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_INDEX_BUFFER_H_INCLUDED__
#define __I_INDEX_BUFFER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrArray.h"

#include "SVertexIndex.h"

namespace irr
{

namespace video
{

}

namespace scene
{

	class IIndexBuffer : public virtual IReferenceCounted
	{
	public:

		virtual void* getData() =0;

		virtual video::E_INDEX_TYPE getType() const =0;
		virtual void setType(video::E_INDEX_TYPE IndexType) =0;

		virtual u32 stride() const =0;

		virtual u32 size() const =0;
		virtual void push_back (const u32 &element) =0;
		virtual u32 operator [](u32 index) const =0;
		virtual u32 getLast() =0;
		virtual void setValue(u32 index, u32 value) =0;
		virtual void set_used(u32 usedNow) =0;
		virtual void reallocate(u32 new_size) =0;
		virtual u32 allocated_size() const=0;

		virtual void* pointer() =0;

		//! get the current hardware mapping hint
		virtual E_HARDWARE_MAPPING getHardwareMappingHint() const =0;

		//! set the hardware mapping hint, for driver
		virtual void setHardwareMappingHint( E_HARDWARE_MAPPING NewMappingHint ) =0;

		//! flags the meshbuffer as changed, reloads hardware buffers
		virtual void setDirty() = 0;

		//! Get the currently used ID for identification of changes.
		/** This shouldn't be used for anything outside the VideoDriver. */
		virtual u32 getChangedID() const = 0;
	};


} // end namespace scene
} // end namespace irr

#endif

