// Copyright (C) 2002-2010 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "MyBillboardSceneNode.h"
#include "IVideoDriver.h"
#include "ISceneManager.h"
#include "ICameraSceneNode.h"

namespace irr
{
namespace scene
{

//! constructor
MyBillboardSceneNode::MyBillboardSceneNode(ISceneNode* parent,
		ISceneManager* mgr, s32 id,
		const core::vector3df& position, const core::dimension2d<f32>& size)
	: IBillboardSceneNode(parent, mgr, id, position)
{
	#ifdef _DEBUG
	setDebugName("MyBillboardSceneNode");
	#endif

	setSize(size);

	indices[0] = 0;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 0;
	indices[4] = 3;
	indices[5] = 2;

	video::SColor colorTop = video::SColor(0xFFFFFFFF);
	video::SColor colorBottom = video::SColor(0xFFFFFFFF);

	vertices[0].TCoords.set(1.0f, 1.0f);
	vertices[0].Color = colorBottom;

	vertices[1].TCoords.set(1.0f, 0.0f);
	vertices[1].Color = colorTop;

	vertices[2].TCoords.set(0.0f, 0.0f);
	vertices[2].Color = colorTop;

	vertices[3].TCoords.set(0.0f, 1.0f);
	vertices[3].Color = colorBottom;
}


//! pre render event
void MyBillboardSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this);

	ISceneNode::OnRegisterSceneNode();
}


//! render
void MyBillboardSceneNode::render()
{
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	ICameraSceneNode* camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;

	// make billboard look to camera

	core::vector3df pos = getAbsolutePosition();

	core::vector3df campos = camera->getAbsolutePosition();
	core::vector3df target = camera->getTarget();
	core::vector3df up = camera->getUpVector();
	core::vector3df view = target - campos;
	view.normalize();

	core::vector3df horizontal = up.crossProduct(view);
	if ( horizontal.getLength() == 0 )
	{
		horizontal.set(up.Y,up.X,up.Z);
	}
	horizontal.normalize();
	horizontal *= 0.5f * Size.Width;

	core::vector3df vertical = horizontal.crossProduct(view);
	vertical.normalize();
	vertical *= 0.5f * Size.Height;

	view *= -1.0f;

	for (s32 i=0; i<4; ++i)
		vertices[i].Normal = view;

	vertices[0].Pos = pos + horizontal + vertical;
	vertices[1].Pos = pos + horizontal - vertical;
	vertices[2].Pos = pos - horizontal - vertical;
	vertices[3].Pos = pos - horizontal + vertical;

	// draw

	if ( DebugDataVisible & scene::EDS_BBOX )
	{
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);
		driver->draw3DBox(BBox, video::SColor(0,208,195,152));
	}

	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);

	driver->setMaterial(Material);

	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& MyBillboardSceneNode::getBoundingBox() const
{
	return BBox;
}


//! sets the size of the billboard
void MyBillboardSceneNode::setSize(const core::dimension2d<f32>& size)
{
	Size = size;

	if (Size.Width == 0.0f)
		Size.Width = 1.0f;

	if (Size.Height == 0.0f )
		Size.Height = 1.0f;

	f32 avg = (size.Width + size.Height)/6;
	BBox.MinEdge.set(-avg,-avg,-avg);
	BBox.MaxEdge.set(avg,avg,avg);
}


video::SMaterial& MyBillboardSceneNode::getMaterial(u32 i)
{
	return Material;
}


//! returns amount of materials used by this scene node.
u32 MyBillboardSceneNode::getMaterialCount() const
{
	return 1;
}


//! gets the size of the billboard
const core::dimension2d<f32>& MyBillboardSceneNode::getSize() const
{
	return Size;
}


//! Set the color of all vertices of the billboard
//! \param overallColor: the color to set
void MyBillboardSceneNode::setColor(const video::SColor & overallColor)
{
	for(u32 vertex = 0; vertex < 4; ++vertex)
		vertices[vertex].Color = overallColor;
}


//! Set the color of the top and bottom vertices of the billboard
//! \param topColor: the color to set the top vertices
//! \param bottomColor: the color to set the bottom vertices
void MyBillboardSceneNode::setColor(const video::SColor & topColor, const video::SColor & bottomColor)
{
	vertices[0].Color = bottomColor;
	vertices[1].Color = topColor;
	vertices[2].Color = topColor;
	vertices[3].Color = bottomColor;
}


//! Gets the color of the top and bottom vertices of the billboard
//! \param[out] topColor: stores the color of the top vertices
//! \param[out] bottomColor: stores the color of the bottom vertices
void MyBillboardSceneNode::getColor(video::SColor & topColor, video::SColor & bottomColor) const
{
	bottomColor = vertices[0].Color;
	topColor = vertices[1].Color;
}
	
void MyBillboardSceneNode::setTCoords(u32 i, core::vector2d<f32> c)
{
	vertices[i].TCoords = c;
}

} // end namespace scene
} // end namespace irr

