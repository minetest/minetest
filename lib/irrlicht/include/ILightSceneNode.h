// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_LIGHT_SCENE_NODE_H_INCLUDED__
#define __I_LIGHT_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"
#include "SLight.h"

namespace irr
{
namespace scene
{

//! Scene node which is a dynamic light.
/** You can switch the light on and off by making it visible or not. It can be
animated by ordinary scene node animators. If the light type is directional or
spot, the direction of the light source is defined by the rotation of the scene
node (assuming (0,0,1) as the local direction of the light).
*/
class ILightSceneNode : public ISceneNode
{
public:

	//! constructor
	ILightSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
		const core::vector3df& position = core::vector3df(0,0,0))
		: ISceneNode(parent, mgr, id, position) {}

	//! Sets the light data associated with this ILightSceneNode
	/** \param light The new light data. */
	virtual void setLightData(const video::SLight& light) = 0;

	//! Gets the light data associated with this ILightSceneNode
	/** \return The light data. */
	virtual const video::SLight& getLightData() const = 0;

	//! Gets the light data associated with this ILightSceneNode
	/** \return The light data. */
	virtual video::SLight& getLightData() = 0;

	//! Sets if the node should be visible or not.
	/** All children of this node won't be visible either, when set
	to true.
	\param isVisible If the node shall be visible. */
	virtual void setVisible(bool isVisible) = 0;

	//! Sets the light's radius of influence.
	/** Outside this radius the light won't lighten geometry and cast no
	shadows. Setting the radius will also influence the attenuation, setting
	it to (0,1/radius,0). If you want to override this behavior, set the
	attenuation after the radius.
	\param radius The new radius. */
	virtual void setRadius(f32 radius) = 0;

	//! Gets the light's radius of influence.
	/** \return The current radius. */
	virtual f32 getRadius() const = 0;

	//! Sets the light type.
	/** \param type The new type. */
	virtual void setLightType(video::E_LIGHT_TYPE type) = 0;

	//! Gets the light type.
	/** \return The current light type. */
	virtual video::E_LIGHT_TYPE getLightType() const = 0;

	//! Sets whether this light casts shadows.
	/** Enabling this flag won't automatically cast shadows, the meshes
	will still need shadow scene nodes attached. But one can enable or
	disable distinct lights for shadow casting for performance reasons.
	\param shadow True if this light shall cast shadows. */
	virtual void enableCastShadow(bool shadow=true) = 0;

	//! Check whether this light casts shadows.
	/** \return True if light would cast shadows, else false. */
	virtual bool getCastShadow() const = 0;
};

} // end namespace scene
} // end namespace irr


#endif

