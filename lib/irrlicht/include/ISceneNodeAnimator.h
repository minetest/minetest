// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SCENE_NODE_ANIMATOR_H_INCLUDED__
#define __I_SCENE_NODE_ANIMATOR_H_INCLUDED__

#include "IReferenceCounted.h"
#include "vector3d.h"
#include "ESceneNodeAnimatorTypes.h"
#include "IAttributeExchangingObject.h"
#include "IEventReceiver.h"

namespace irr
{
namespace io
{
	class IAttributes;
} // end namespace io
namespace scene
{
	class ISceneNode;
	class ISceneManager;

	//! Animates a scene node. Can animate position, rotation, material, and so on.
	/** A scene node animator is able to animate a scene node in a very simple way. It may
	change its position, rotation, scale and/or material. There are lots of animators
	to choose from. You can create scene node animators with the ISceneManager interface.
	*/
	class ISceneNodeAnimator : public io::IAttributeExchangingObject, public IEventReceiver
	{
	public:
		//! Animates a scene node.
		/** \param node Node to animate.
		\param timeMs Current time in milli seconds. */
		virtual void animateNode(ISceneNode* node, u32 timeMs) =0;

		//! Creates a clone of this animator.
		/** Please note that you will have to drop
		(IReferenceCounted::drop()) the returned pointer after calling this. */
		virtual ISceneNodeAnimator* createClone(ISceneNode* node,
				ISceneManager* newManager=0) =0;

		//! Returns true if this animator receives events.
		/** When attached to an active camera, this animator will be
		able to respond to events such as mouse and keyboard events. */
		virtual bool isEventReceiverEnabled() const
		{
			return false;
		}

		//! Event receiver, override this function for camera controlling animators
		virtual bool OnEvent(const SEvent& event)
		{
			return false;
		}

		//! Returns type of the scene node animator
		virtual ESCENE_NODE_ANIMATOR_TYPE getType() const
		{
			return ESNAT_UNKNOWN;
		}

		//! Returns if the animator has finished.
		/** This is only valid for non-looping animators with a discrete end state.
		\return true if the animator has finished, false if it is still running. */
		virtual bool hasFinished(void) const
		{
			return false;
		}
	};


} // end namespace scene
} // end namespace irr

#endif

