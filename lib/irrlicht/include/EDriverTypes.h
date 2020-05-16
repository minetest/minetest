// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_DRIVER_TYPES_H_INCLUDED__
#define __E_DRIVER_TYPES_H_INCLUDED__

namespace irr
{
namespace video
{

	//! An enum for all types of drivers the Irrlicht Engine supports.
	enum E_DRIVER_TYPE
	{
		//! Null driver, useful for applications to run the engine without visualisation.
		/** The null device is able to load textures, but does not
		render and display any graphics. */
		EDT_NULL,

		//! The Irrlicht Engine Software renderer.
		/** Runs on all platforms, with every hardware. It should only
		be used for 2d graphics, but it can also perform some primitive
		3d functions. These 3d drawing functions are quite fast, but
		very inaccurate, and don't even support clipping in 3D mode. */
		EDT_SOFTWARE,

		//! The Burning's Software Renderer, an alternative software renderer
		/** Basically it can be described as the Irrlicht Software
		renderer on steroids. It rasterizes 3D geometry perfectly: It
		is able to perform correct 3d clipping, perspective correct
		texture mapping, perspective correct color mapping, and renders
		sub pixel correct, sub texel correct primitives. In addition,
		it does bilinear texel filtering and supports more materials
		than the EDT_SOFTWARE driver. This renderer has been written
		entirely by Thomas Alten, thanks a lot for this huge
		contribution. */
		EDT_BURNINGSVIDEO,

		//! Direct3D8 device, only available on Win32 platforms.
		/** Performs hardware accelerated rendering of 3D and 2D
		primitives. */
		EDT_DIRECT3D8,

		//! Direct3D 9 device, only available on Win32 platforms.
		/** Performs hardware accelerated rendering of 3D and 2D
		primitives. */
		EDT_DIRECT3D9,

		//! OpenGL device, available on most platforms.
		/** Performs hardware accelerated rendering of 3D and 2D
		primitives. */
		EDT_OPENGL,

		//! No driver, just for counting the elements
		EDT_COUNT
	};

} // end namespace video
} // end namespace irr


#endif

