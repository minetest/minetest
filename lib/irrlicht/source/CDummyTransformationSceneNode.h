// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_DUMMY_TRANSFORMATION_SCENE_NODE_H_INCLUDED__
#define __C_DUMMY_TRANSFORMATION_SCENE_NODE_H_INCLUDED__

#include "IDummyTransformationSceneNode.h"

namespace irr
{
namespace scene
{

	class CDummyTransformationSceneNode : public IDummyTransformationSceneNode
	{
	public:

		//! constructor
		CDummyTransformationSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id);

		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! Returns a reference to the current relative transformation matrix.
		//! This is the matrix, this scene node uses instead of scale, translation
		//! and rotation.
		virtual core::matrix4& getRelativeTransformationMatrix();

		//! Returns the relative transformation of the scene node.
		virtual core::matrix4 getRelativeTransformation() const;

		//! does nothing.
		virtual void render() {}

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_DUMMY_TRANSFORMATION; }

		//! Creates a clone of this scene node and its children.
		virtual ISceneNode* clone(ISceneNode* newParent=0, ISceneManager* newManager=0);


	private:

		// TODO: We can add least add some warnings to find troubles faster until we have 
		// fixed bug id 2318691.
		virtual const core::vector3df& getScale() const;
		virtual void setScale(const core::vector3df& scale);
		virtual const core::vector3df& getRotation() const;
		virtual void setRotation(const core::vector3df& rotation);
		virtual const core::vector3df& getPosition() const;
		virtual void setPosition(const core::vector3df& newpos);

		core::matrix4 RelativeTransformationMatrix;
		core::aabbox3d<f32> Box;
	};

} // end namespace scene
} // end namespace irr

#endif

