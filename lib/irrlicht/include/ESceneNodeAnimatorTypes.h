// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_SCENE_NODE_ANIMATOR_TYPES_H_INCLUDED__
#define __E_SCENE_NODE_ANIMATOR_TYPES_H_INCLUDED__

namespace irr
{
namespace scene
{

	//! An enumeration for all types of built-in scene node animators
	enum ESCENE_NODE_ANIMATOR_TYPE
	{
		//! Fly circle scene node animator
		ESNAT_FLY_CIRCLE = 0,

		//! Fly straight scene node animator
		ESNAT_FLY_STRAIGHT,

		//! Follow spline scene node animator
		ESNAT_FOLLOW_SPLINE,

		//! Rotation scene node animator
		ESNAT_ROTATION,

		//! Texture scene node animator
		ESNAT_TEXTURE,

		//! Deletion scene node animator
		ESNAT_DELETION,

		//! Collision respose scene node animator
		ESNAT_COLLISION_RESPONSE,

		//! FPS camera animator
		ESNAT_CAMERA_FPS,

		//! Maya camera animator
		ESNAT_CAMERA_MAYA,

		//! Amount of built-in scene node animators
		ESNAT_COUNT,

		//! Unknown scene node animator
		ESNAT_UNKNOWN,

		//! This enum is never used, it only forces the compiler to compile this enumeration to 32 bit.
		ESNAT_FORCE_32_BIT = 0x7fffffff
	};

} // end namespace scene
} // end namespace irr


#endif

