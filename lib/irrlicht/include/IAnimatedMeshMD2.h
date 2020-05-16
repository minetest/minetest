// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_ANIMATED_MESH_MD2_H_INCLUDED__
#define __I_ANIMATED_MESH_MD2_H_INCLUDED__

#include "IAnimatedMesh.h"

namespace irr
{
namespace scene
{

	//! Types of standard md2 animations
	enum EMD2_ANIMATION_TYPE
	{
		EMAT_STAND = 0,
		EMAT_RUN,
		EMAT_ATTACK,
		EMAT_PAIN_A,
		EMAT_PAIN_B,
		EMAT_PAIN_C,
		EMAT_JUMP,
		EMAT_FLIP,
		EMAT_SALUTE,
		EMAT_FALLBACK,
		EMAT_WAVE,
		EMAT_POINT,
		EMAT_CROUCH_STAND,
		EMAT_CROUCH_WALK,
		EMAT_CROUCH_ATTACK,
		EMAT_CROUCH_PAIN,
		EMAT_CROUCH_DEATH,
		EMAT_DEATH_FALLBACK,
		EMAT_DEATH_FALLFORWARD,
		EMAT_DEATH_FALLBACKSLOW,
		EMAT_BOOM,

		//! Not an animation, but amount of animation types.
		EMAT_COUNT
	};

	//! Interface for using some special functions of MD2 meshes
	class IAnimatedMeshMD2 : public IAnimatedMesh
	{
	public:

		//! Get frame loop data for a default MD2 animation type.
		/** \param l The EMD2_ANIMATION_TYPE to get the frames for.
		\param outBegin The returned beginning frame for animation type specified.
		\param outEnd The returned ending frame for the animation type specified.
		\param outFPS The number of frames per second, this animation should be played at.
		\return beginframe, endframe and frames per second for a default MD2 animation type. */
		virtual void getFrameLoop(EMD2_ANIMATION_TYPE l, s32& outBegin,
			s32& outEnd, s32& outFPS) const = 0;

		//! Get frame loop data for a special MD2 animation type, identified by name.
		/** \param name Name of the animation.
		\param outBegin The returned beginning frame for animation type specified.
		\param outEnd The returned ending frame for the animation type specified.
		\param outFPS The number of frames per second, this animation should be played at.
		\return beginframe, endframe and frames per second for a special MD2 animation type. */
		virtual bool getFrameLoop(const c8* name,
			s32& outBegin, s32& outEnd, s32& outFPS) const = 0;

		//! Get amount of md2 animations in this file.
		virtual s32 getAnimationCount() const = 0;

		//! Get name of md2 animation.
		/** \param nr: Zero based index of animation. */
		virtual const c8* getAnimationName(s32 nr) const = 0;
	};

} // end namespace scene
} // end namespace irr

#endif

