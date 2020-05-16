// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SCENE_NODE_ANIMATOR_FINISHING_H_INCLUDED__
#define __I_SCENE_NODE_ANIMATOR_FINISHING_H_INCLUDED__

#include "ISceneNode.h"

namespace irr
{
namespace scene
{
	//! This is an abstract base class for animators that have a discrete end time.
	class ISceneNodeAnimatorFinishing : public ISceneNodeAnimator
	{
	public:

		//! constructor
		ISceneNodeAnimatorFinishing(u32 finishTime)
			: FinishTime(finishTime), HasFinished(false) { }

		virtual bool hasFinished(void) const { return HasFinished; }

	protected:

		u32 FinishTime;
		bool HasFinished;
	};


} // end namespace scene
} // end namespace irr

#endif

