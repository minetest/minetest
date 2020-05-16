// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h
//
// created by Dean Wadsworth aka Varmint Dec 31 2007

#ifndef __I_VOLUME_LIGHT_SCENE_NODE_H_INCLUDED__
#define __I_VOLUME_LIGHT_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"

namespace irr
{
namespace scene
{
	class IMeshBuffer;

	class IVolumeLightSceneNode : public ISceneNode
	{
	public:

		//! constructor
		IVolumeLightSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			const core::vector3df& position,
			const core::vector3df& rotation,
			const core::vector3df& scale)
			: ISceneNode(parent, mgr, id, position, rotation, scale) {};

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_VOLUME_LIGHT; }

		//! Sets the number of segments across the U axis
		virtual void setSubDivideU(const u32 inU) =0;

		//! Sets the number of segments across the V axis
		virtual void setSubDivideV(const u32 inV) =0;

		//! Returns the number of segments across the U axis
		virtual u32 getSubDivideU() const =0;

		//! Returns the number of segments across the V axis
		virtual u32 getSubDivideV() const =0;

		//! Sets the color of the base of the light
		virtual void setFootColor(const video::SColor inColor) =0;

		//! Sets the color of the tip of the light
		virtual void setTailColor(const video::SColor inColor) =0;

		//! Returns the color of the base of the light
		virtual video::SColor getFootColor() const =0;

		//! Returns the color of the tip of the light
		virtual video::SColor getTailColor() const =0;
	};

} // end namespace scene
} // end namespace irr

#endif
