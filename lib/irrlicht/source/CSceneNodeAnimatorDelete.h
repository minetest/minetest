// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SCENE_NODE_ANIMATOR_DELETE_H_INCLUDED__
#define __C_SCENE_NODE_ANIMATOR_DELETE_H_INCLUDED__

#include "ISceneNodeAnimatorFinishing.h"

namespace irr
{
namespace scene
{
	class CSceneNodeAnimatorDelete : public ISceneNodeAnimatorFinishing
	{
	public:

		//! constructor
		CSceneNodeAnimatorDelete(ISceneManager* manager, u32 when);

		//! animates a scene node
		virtual void animateNode(ISceneNode* node, u32 timeMs);

		//! Returns type of the scene node animator
		virtual ESCENE_NODE_ANIMATOR_TYPE getType() const
		{
			return ESNAT_DELETION;
		}
		
		//! Creates a clone of this animator.
		/** Please note that you will have to drop
		(IReferenceCounted::drop()) the returned pointer after calling
		this. */
		virtual ISceneNodeAnimator* createClone(ISceneNode* node, ISceneManager* newManager=0);

	private:

		ISceneManager* SceneManager;
	};


} // end namespace scene
} // end namespace irr

#endif

