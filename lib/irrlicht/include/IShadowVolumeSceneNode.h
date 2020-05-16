// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__
#define __I_SHADOW_VOLUME_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"

namespace irr
{
namespace scene
{
	class IMesh;

	//! Scene node for rendering a shadow volume into a stencil buffer.
	class IShadowVolumeSceneNode : public ISceneNode
	{
	public:

		//! constructor
		IShadowVolumeSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id)
			: ISceneNode(parent, mgr, id) {}

		//! Sets the mesh from which the shadow volume should be generated.
		/** To optimize shadow rendering, use a simpler mesh for shadows.
		*/
		virtual void setShadowMesh(const IMesh* mesh) = 0;

		//! Updates the shadow volumes for current light positions.
		virtual void updateShadowVolumes() = 0;
	};

} // end namespace scene
} // end namespace irr

#endif

