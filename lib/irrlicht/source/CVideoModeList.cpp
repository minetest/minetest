// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CVideoModeList.h"
#include "irrMath.h"

namespace irr
{
namespace video
{

//! constructor
CVideoModeList::CVideoModeList()
{
	#ifdef _DEBUG
	setDebugName("CVideoModeList");
	#endif

	Desktop.depth = 0;
	Desktop.size = core::dimension2d<u32>(0,0);
}


void CVideoModeList::setDesktop(s32 desktopDepth, const core::dimension2d<u32>& desktopSize)
{
	Desktop.depth = desktopDepth;
	Desktop.size = desktopSize;
}


//! Gets amount of video modes in the list.
s32 CVideoModeList::getVideoModeCount() const
{
	return (s32)VideoModes.size();
}


//! Returns the screen size of a video mode in pixels.
core::dimension2d<u32> CVideoModeList::getVideoModeResolution(s32 modeNumber) const
{
	if (modeNumber < 0 || modeNumber > (s32)VideoModes.size())
		return core::dimension2d<u32>(0,0);

	return VideoModes[modeNumber].size;
}


core::dimension2d<u32> CVideoModeList::getVideoModeResolution(
		const core::dimension2d<u32>& minSize,
		const core::dimension2d<u32>& maxSize) const
{
	u32 best=VideoModes.size();
	// if only one or no mode
	if (best<2)
		return getVideoModeResolution(0);

	u32 i;
	for (i=0; i<VideoModes.size(); ++i)
	{
		if (VideoModes[i].size.Width>=minSize.Width &&
			VideoModes[i].size.Height>=minSize.Height &&
			VideoModes[i].size.Width<=maxSize.Width &&
			VideoModes[i].size.Height<=maxSize.Height)
			best=i;
	}
	// we take the last one found, the largest one fitting
	if (best<VideoModes.size())
		return VideoModes[best].size;
	const u32 minArea = minSize.getArea();
	const u32 maxArea = maxSize.getArea();
	u32 minDist = 0xffffffff;
	best=0;
	for (i=0; i<VideoModes.size(); ++i)
	{
		const u32 area = VideoModes[i].size.getArea();
		const u32 dist = core::min_(abs(int(minArea-area)), abs(int(maxArea-area)));
		if (dist<minDist)
		{
			minDist=dist;
			best=i;
		}
	}
	return VideoModes[best].size;
}


//! Returns the pixel depth of a video mode in bits.
s32 CVideoModeList::getVideoModeDepth(s32 modeNumber) const
{
	if (modeNumber < 0 || modeNumber > (s32)VideoModes.size())
		return 0;

	return VideoModes[modeNumber].depth;
}


//! Returns current desktop screen resolution.
const core::dimension2d<u32>& CVideoModeList::getDesktopResolution() const
{
	return Desktop.size;
}


//! Returns the pixel depth of a video mode in bits.
s32 CVideoModeList::getDesktopDepth() const
{
	return Desktop.depth;
}


//! adds a new mode to the list
void CVideoModeList::addMode(const core::dimension2d<u32>& size, s32 depth)
{
	SVideoMode m;
	m.depth = depth;
	m.size = size;

	for (u32 i=0; i<VideoModes.size(); ++i)
	{
		if (VideoModes[i] == m)
			return;
	}

	VideoModes.push_back(m);
	VideoModes.sort(); // TODO: could be replaced by inserting into right place
}


} // end namespace video
} // end namespace irr

