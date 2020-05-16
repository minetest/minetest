// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_

#include "CBoneSceneNode.h"

namespace irr
{
namespace scene
{

//! constructor
CBoneSceneNode::CBoneSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
	u32 boneIndex, const c8* boneName)
: IBoneSceneNode(parent, mgr, id), BoneIndex(boneIndex),
	AnimationMode(EBAM_AUTOMATIC), SkinningSpace(EBSS_LOCAL)
{
	#ifdef _DEBUG
	setDebugName("CBoneSceneNode");
	#endif
	setName(boneName);
}


//! Returns the index of the bone
u32 CBoneSceneNode::getBoneIndex() const
{
	return BoneIndex;
}


//! Sets the animation mode of the bone. Returns true if successful.
bool CBoneSceneNode::setAnimationMode(E_BONE_ANIMATION_MODE mode)
{
	AnimationMode = mode;
	return true;
}


//! Gets the current animation mode of the bone
E_BONE_ANIMATION_MODE CBoneSceneNode::getAnimationMode() const
{
	return AnimationMode;
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CBoneSceneNode::getBoundingBox() const
{
	return Box;
}


/*
//! Returns the relative transformation of the scene node.
core::matrix4 CBoneSceneNode::getRelativeTransformation() const
{
	return core::matrix4(); // RelativeTransformation;
}
*/


void CBoneSceneNode::OnAnimate(u32 timeMs)
{
	if (IsVisible)
	{
		// animate this node with all animators

		ISceneNodeAnimatorList::Iterator ait = Animators.begin();
		for (; ait != Animators.end(); ++ait)
			(*ait)->animateNode(this, timeMs);

		// update absolute position
		//updateAbsolutePosition();

		// perform the post render process on all children
		ISceneNodeList::Iterator it = Children.begin();
		for (; it != Children.end(); ++it)
			(*it)->OnAnimate(timeMs);
	}
}


void CBoneSceneNode::helper_updateAbsolutePositionOfAllChildren(ISceneNode *Node)
{
	Node->updateAbsolutePosition();

	ISceneNodeList::ConstIterator it = Node->getChildren().begin();
	for (; it != Node->getChildren().end(); ++it)
	{
		helper_updateAbsolutePositionOfAllChildren( (*it) );
	}
}


void CBoneSceneNode::updateAbsolutePositionOfAllChildren()
{
	helper_updateAbsolutePositionOfAllChildren( this );
}


void CBoneSceneNode::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	IBoneSceneNode::serializeAttributes(out, options);
	out->addInt("BoneIndex", BoneIndex);
	out->addEnum("AnimationMode", AnimationMode, BoneAnimationModeNames);
}


void CBoneSceneNode::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	BoneIndex = in->getAttributeAsInt("BoneIndex");
	AnimationMode = (E_BONE_ANIMATION_MODE)in->getAttributeAsEnumeration("AnimationMode", BoneAnimationModeNames);
	// for legacy files (before 1.5)
	const core::stringc boneName = in->getAttributeAsString("BoneName");
	setName(boneName);
	IBoneSceneNode::deserializeAttributes(in, options);
	// TODO: add/replace bone in parent with bone from mesh
}


} // namespace scene
} // namespace irr

#endif

