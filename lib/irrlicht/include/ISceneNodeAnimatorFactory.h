// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SCENE_NODE_ANIMATOR_FACTORY_H_INCLUDED__
#define __I_SCENE_NODE_ANIMATOR_FACTORY_H_INCLUDED__

#include "IReferenceCounted.h"
#include "ESceneNodeAnimatorTypes.h"

namespace irr
{
namespace scene
{
	class ISceneNode;
	class ISceneNodeAnimator;

	//! Interface for dynamic creation of scene node animators
	/** To be able to add custom scene node animators to Irrlicht and to make it possible for the
	scene manager to save and load those external animators, simply implement this
	interface and register it in you scene manager via ISceneManager::registerSceneNodeAnimatorFactory.
	Note: When implementing your own scene node factory, don't call ISceneNodeManager::grab() to
	increase the reference counter of the scene node manager. This is not necessary because the
	scene node manager will grab() the factory anyway, and otherwise cyclic references will
	be created and the scene manager and all its nodes won't get deallocated.
	*/
	class ISceneNodeAnimatorFactory : public virtual IReferenceCounted
	{
	public:

		//! creates a scene node animator based on its type id
		/** \param type: Type of the scene node animator to add.
		\param target: Target scene node of the new animator.
		\return Returns pointer to the new scene node animator or null if not successful. You need to
		drop this pointer after calling this, see IReferenceCounted::drop() for details. */
		virtual ISceneNodeAnimator* createSceneNodeAnimator(ESCENE_NODE_ANIMATOR_TYPE type, ISceneNode* target) = 0;

		//! creates a scene node animator based on its type name
		/** \param typeName: Type of the scene node animator to add.
		\param target: Target scene node of the new animator.
		\return Returns pointer to the new scene node animator or null if not successful. You need to
		drop this pointer after calling this, see IReferenceCounted::drop() for details. */
		virtual ISceneNodeAnimator* createSceneNodeAnimator(const c8* typeName, ISceneNode* target) = 0;

		//! returns amount of scene node animator types this factory is able to create
		virtual u32 getCreatableSceneNodeAnimatorTypeCount() const = 0;

		//! returns type of a createable scene node animator type
		/** \param idx: Index of scene node animator type in this factory. Must be a value between 0 and
		getCreatableSceneNodeTypeCount() */
		virtual ESCENE_NODE_ANIMATOR_TYPE getCreateableSceneNodeAnimatorType(u32 idx) const = 0;

		//! returns type name of a createable scene node animator type
		/** \param idx: Index of scene node animator type in this factory. Must be a value between 0 and
		getCreatableSceneNodeAnimatorTypeCount() */
		virtual const c8* getCreateableSceneNodeAnimatorTypeName(u32 idx) const = 0;

		//! returns type name of a createable scene node animator type
		/** \param type: Type of scene node animator.
		\return: Returns name of scene node animator type if this factory can create the type, otherwise 0. */
		virtual const c8* getCreateableSceneNodeAnimatorTypeName(ESCENE_NODE_ANIMATOR_TYPE type) const = 0;
	};


} // end namespace scene
} // end namespace irr

#endif

