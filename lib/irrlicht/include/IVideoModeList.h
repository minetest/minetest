// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_I_VIDEO_MODE_LIST_H_INCLUDED__
#define __IRR_I_VIDEO_MODE_LIST_H_INCLUDED__

#include "IReferenceCounted.h"
#include "dimension2d.h"

namespace irr
{
namespace video
{

	//! A list of all available video modes.
	/** You can get a list via IrrlichtDevice::getVideoModeList(). If you are confused
	now, because you think you have to create an Irrlicht Device with a video
	mode before being able to get the video mode list, let me tell you that
	there is no need to start up an Irrlicht Device with EDT_DIRECT3D8, EDT_OPENGL or
	EDT_SOFTWARE: For this (and for lots of other reasons) the null device,
	EDT_NULL exists.*/
	class IVideoModeList : public virtual IReferenceCounted
	{
	public:

		//! Gets amount of video modes in the list.
		/** \return Returns amount of video modes. */
		virtual s32 getVideoModeCount() const = 0;

		//! Get the screen size of a video mode in pixels.
		/** \param modeNumber: zero based index of the video mode.
		\return Size of screen in pixels of the specified video mode. */
		virtual core::dimension2d<u32> getVideoModeResolution(s32 modeNumber) const = 0;

		//! Get a supported screen size with certain constraints.
		/** \param minSize: Minimum dimensions required.
		\param maxSize: Maximum dimensions allowed.
		\return Size of screen in pixels which matches the requirements.
		as good as possible. */
		virtual core::dimension2d<u32> getVideoModeResolution(const core::dimension2d<u32>& minSize, const core::dimension2d<u32>& maxSize) const = 0;

		//! Get the pixel depth of a video mode in bits.
		/** \param modeNumber: zero based index of the video mode.
		\return Size of each pixel of the specified video mode in bits. */
		virtual s32 getVideoModeDepth(s32 modeNumber) const = 0;

		//! Get current desktop screen resolution.
		/** \return Size of screen in pixels of the current desktop video mode. */
		virtual const core::dimension2d<u32>& getDesktopResolution() const = 0;

		//! Get the pixel depth of a video mode in bits.
		/** \return Size of each pixel of the current desktop video mode in bits. */
		virtual s32 getDesktopDepth() const = 0;
	};

} // end namespace video
} // end namespace irr


#endif

