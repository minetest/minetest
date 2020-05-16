// Written by Colin MacDonald - all rights assigned to Nikolaus Gebhardt
// Copyright (C) 2008-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_LIGHT_MANAGER_H_INCLUDED__
#define __I_LIGHT_MANAGER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrArray.h"

namespace irr
{
namespace scene
{
	class ILightSceneNode;

	//! ILightManager provides an interface for user applications to manipulate the list of lights in the scene.
	/** The light list can be trimmed or re-ordered before device/ hardware
	lights are created, and/or individual lights can be switched on and off
	before or after each scene node is rendered. It is assumed that the
	ILightManager implementation will store any data that it wishes to
	retain, i.e. the ISceneManager to which it is assigned, the lightList,
	the current render pass, and the current scene node. */
	class ILightManager : public IReferenceCounted
	{
	public:
		//! Called after the scene's light list has been built, but before rendering has begun.
		/** As actual device/hardware lights are not created until the
		ESNRP_LIGHT render pass, this provides an opportunity for the
		light manager to trim or re-order the light list, before any
		device/hardware lights have actually been created.
		\param lightList: the Scene Manager's light list, which
		the light manager may modify. This reference will remain valid
		until OnPostRender().
		*/
		virtual void OnPreRender(core::array<ISceneNode*> & lightList) = 0;

		//! Called after the last scene node is rendered.
		/** After this call returns, the lightList passed to OnPreRender() becomes invalid. */
		virtual void OnPostRender(void) = 0;

		//! Called before a render pass begins
		/** \param renderPass: the render pass that's about to begin */
		virtual void OnRenderPassPreRender(E_SCENE_NODE_RENDER_PASS renderPass) = 0;

		//! Called after the render pass specified in OnRenderPassPreRender() ends
		/** \param[in] renderPass: the render pass that has finished */
		virtual void OnRenderPassPostRender(E_SCENE_NODE_RENDER_PASS renderPass) = 0;

		//! Called before the given scene node is rendered
		/** \param[in] node: the scene node that's about to be rendered */
		virtual void OnNodePreRender(ISceneNode* node) = 0;

		//! Called after the the node specified in OnNodePreRender() has been rendered
		/** \param[in] node: the scene node that has just been rendered */
		virtual void OnNodePostRender(ISceneNode* node) = 0;
	};
} // end namespace scene
} // end namespace irr

#endif
