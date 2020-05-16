// Copyright (C) 2002-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_Z2_BUFFER_H_INCLUDED__
#define __I_Z2_BUFFER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "dimension2d.h"
#include "S4DVertex.h"

namespace irr
{
namespace video
{
	class IDepthBuffer : public virtual IReferenceCounted
	{
	public:

		//! destructor
		virtual ~IDepthBuffer() {};

		//! clears the zbuffer
		virtual void clear() = 0;

		//! sets the new size of the zbuffer
		virtual void setSize(const core::dimension2d<u32>& size) = 0;

		//! returns the size of the zbuffer
		virtual const core::dimension2d<u32>& getSize() const = 0;

		//! locks the zbuffer
		virtual void* lock() = 0;

		//! unlocks the zbuffer
		virtual void unlock() = 0;

		//! returns pitch of depthbuffer (in bytes)
		virtual u32 getPitch() const = 0;

	};


	//! creates a ZBuffer
	IDepthBuffer* createDepthBuffer(const core::dimension2d<u32>& size);

	class IStencilBuffer : public virtual IReferenceCounted
	{
	public:

		//! destructor
		virtual ~IStencilBuffer() {};

		//! clears the zbuffer
		virtual void clear() = 0;

		//! sets the new size of the zbuffer
		virtual void setSize(const core::dimension2d<u32>& size) = 0;

		//! returns the size of the zbuffer
		virtual const core::dimension2d<u32>& getSize() const = 0;

		//! locks the zbuffer
		virtual void* lock() = 0;

		//! unlocks the zbuffer
		virtual void unlock() = 0;

		//! returns pitch of depthbuffer (in bytes)
		virtual u32 getPitch() const = 0;

	};


	//! creates a Stencil Buffer
	IStencilBuffer* createStencilBuffer(const core::dimension2d<u32>& size);

} // end namespace video
} // end namespace irr

#endif

