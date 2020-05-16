// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_BILLBOARD_SCENE_NODE_H_INCLUDED__
#define __I_BILLBOARD_SCENE_NODE_H_INCLUDED__

#include "ISceneNode.h"

namespace irr
{
namespace scene
{

//! A billboard scene node.
/** A billboard is like a 3d sprite: A 2d element,
which always looks to the camera. It is usually used for explosions, fire,
lensflares, particles and things like that.
*/
class IBillboardSceneNode : public ISceneNode
{
public:

	//! Constructor
	IBillboardSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
		const core::vector3df& position = core::vector3df(0,0,0))
		: ISceneNode(parent, mgr, id, position) {}

	//! Sets the size of the billboard, making it rectangular.
	virtual void setSize(const core::dimension2d<f32>& size) = 0;

	//! Sets the size of the billboard with independent widths of the bottom and top edges.
	/** \param[in] height The height of the billboard.
	\param[in] bottomEdgeWidth The width of the bottom edge of the billboard.
	\param[in] topEdgeWidth The width of the top edge of the billboard.
	*/
	virtual void setSize(f32 height, f32 bottomEdgeWidth, f32 topEdgeWidth) = 0;

	//! Returns the size of the billboard.
	/** This will return the width of the bottom edge of the billboard.
	Use getWidths() to retrieve the bottom and top edges independently.
	\return Size of the billboard.
	*/
	virtual const core::dimension2d<f32>& getSize() const = 0;

	//! Gets the size of the the billboard and handles independent top and bottom edge widths correctly.
	/** \param[out] height The height of the billboard.
	\param[out] bottomEdgeWidth The width of the bottom edge of the billboard.
	\param[out] topEdgeWidth The width of the top edge of the billboard.
	*/
	virtual void getSize(f32& height, f32& bottomEdgeWidth, f32& topEdgeWidth) const =0;

	//! Set the color of all vertices of the billboard
	/** \param[in] overallColor Color to set */
	virtual void setColor(const video::SColor& overallColor) = 0;

	//! Set the color of the top and bottom vertices of the billboard
	/** \param[in] topColor Color to set the top vertices
	\param[in] bottomColor Color to set the bottom vertices */
	virtual void setColor(const video::SColor& topColor,
			const video::SColor& bottomColor) = 0;

	//! Gets the color of the top and bottom vertices of the billboard
	/** \param[out] topColor Stores the color of the top vertices
	\param[out] bottomColor Stores the color of the bottom vertices */
	virtual void getColor(video::SColor& topColor,
			video::SColor& bottomColor) const = 0;
};

} // end namespace scene
} // end namespace irr


#endif

