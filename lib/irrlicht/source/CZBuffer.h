// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_Z_BUFFER_H_INCLUDED__
#define __C_Z_BUFFER_H_INCLUDED__

#include "IZBuffer.h"

namespace irr
{
namespace video
{

	class CZBuffer : public IZBuffer
	{
	public:

		//! constructor
		CZBuffer(const core::dimension2d<u32>& size);

		//! destructor
		virtual ~CZBuffer();

		//! clears the zbuffer
		virtual void clear();

		//! sets the new size of the zbuffer
		virtual void setSize(const core::dimension2d<u32>& size);

		//! returns the size of the zbuffer
		virtual const core::dimension2d<u32>& getSize() const;

		//! locks the zbuffer
		virtual TZBufferType* lock();

		//! unlocks the zbuffer
		virtual void unlock();

	private:

		TZBufferType* Buffer;
		TZBufferType* BufferEnd;
		core::dimension2d<u32> Size;
		s32 TotalSize;
	};

} // end namespace video
} // end namespace irr

#endif

