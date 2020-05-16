// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_EMPTY_SCENE_NODE_H_INCLUDED__
#define __C_EMPTY_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"

namespace irr
{
namespace scene
{

	class CEmptySceneNode : public ISceneNode
	{
	public:

		//! constructor
		CEmptySceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);

		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! This method is called just before the rendering process of the whole scene.
		virtual void OnRegisterSceneNode();

		//! does nothing.
		virtual void render();

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_EMPTY; }

		//! Creates a clone of this scene node and its children.
		virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);

	private:

		core::aabbox3d<f32> Box;
	};

} // end namespace scene
} // end namespace irr

#endif

