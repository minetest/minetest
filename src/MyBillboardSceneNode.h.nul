// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_BILLBOARD_SCENE_NODE_H_INCLUDED__
#define __C_BILLBOARD_SCENE_NODE_H_INCLUDED__

#include "IBillboardSceneNode.h"
#include "S3DVertex.h"

namespace irr
{
namespace scene
{

//! Scene node which is a billboard. A billboard is like a 3d sprite: A 2d element,
//! which always looks to the camera. 
class MyBillboardSceneNode : virtual public IBillboardSceneNode
{
public:

	//! constructor
	MyBillboardSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
		const core::vector3df& position, const core::dimension2d<f32>& size);

	//! pre render event
	virtual void OnRegisterSceneNode();

	//! render
	virtual void render();

	//! returns the axis aligned bounding box of this node
	virtual const core::aabbox3d<f32>& getBoundingBox() const;

	//! sets the size of the billboard
	virtual void setSize(const core::dimension2d<f32>& size);

	//! gets the size of the billboard
	virtual const core::dimension2d<f32>& getSize() const;

	virtual video::SMaterial& getMaterial(u32 i);
	
	//! returns amount of materials used by this scene node.
	virtual u32 getMaterialCount() const;
	
	//! Set the color of all vertices of the billboard
	//! \param overallColor: the color to set
	virtual void setColor(const video::SColor & overallColor);

	//! Set the color of the top and bottom vertices of the billboard
	//! \param topColor: the color to set the top vertices
	//! \param bottomColor: the color to set the bottom vertices
	virtual void setColor(const video::SColor & topColor, const video::SColor & bottomColor);

	//! Gets the color of the top and bottom vertices of the billboard
	//! \param[out] topColor: stores the color of the top vertices
	//! \param[out] bottomColor: stores the color of the bottom vertices
	virtual void getColor(video::SColor& topColor, video::SColor& bottomColor) const;

	virtual void setTCoords(u32 i, core::vector2d<f32> c);

private:

	core::dimension2d<f32> Size;
	core::aabbox3d<f32> BBox;
	video::SMaterial Material;

	video::S3DVertex vertices[4];
	u16 indices[6];
};


} // end namespace scene
} // end namespace irr

#endif

