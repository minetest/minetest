// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIMeshViewer.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IAnimatedMesh.h"
#include "IMesh.h"
#include "os.h"
#include "IGUISkin.h"

namespace irr
{

namespace gui
{


//! constructor
CGUIMeshViewer::CGUIMeshViewer(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
: IGUIMeshViewer(environment, parent, id, rectangle), Mesh(0)
{
	#ifdef _DEBUG
	setDebugName("CGUIMeshViewer");
	#endif
}


//! destructor
CGUIMeshViewer::~CGUIMeshViewer()
{
	if (Mesh)
		Mesh->drop();
}


//! sets the mesh to be shown
void CGUIMeshViewer::setMesh(scene::IAnimatedMesh* mesh)
{
	if (mesh)
		mesh->grab();
	if (Mesh)
		Mesh->drop();

	Mesh = mesh;

	/* This might be used for proper transformation etc.
	core::vector3df center(0.0f,0.0f,0.0f);
	core::aabbox3d<f32> box;

	box = Mesh->getMesh(0)->getBoundingBox();
	center = (box.MaxEdge + box.MinEdge) / 2;
	*/
}


//! Gets the displayed mesh
scene::IAnimatedMesh* CGUIMeshViewer::getMesh() const
{
	return Mesh;
}


//! sets the material
void CGUIMeshViewer::setMaterial(const video::SMaterial& material)
{
	Material = material;
}


//! gets the material
const video::SMaterial& CGUIMeshViewer::getMaterial() const
{
	return Material;
}


//! called if an event happened.
bool CGUIMeshViewer::OnEvent(const SEvent& event)
{
	return IGUIElement::OnEvent(event);
}


//! draws the element and its children
void CGUIMeshViewer::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* skin = Environment->getSkin();
	video::IVideoDriver* driver = Environment->getVideoDriver();
	core::rect<s32> viewPort = AbsoluteRect;
	viewPort.LowerRightCorner.X -= 1;
	viewPort.LowerRightCorner.Y -= 1;
	viewPort.UpperLeftCorner.X += 1;
	viewPort.UpperLeftCorner.Y += 1;

	viewPort.clipAgainst(AbsoluteClippingRect);

	// draw the frame

	core::rect<s32> frameRect(AbsoluteRect);
	frameRect.LowerRightCorner.Y = frameRect.UpperLeftCorner.Y + 1;
	skin->draw2DRectangle(this, skin->getColor(EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

	frameRect.LowerRightCorner.Y = AbsoluteRect.LowerRightCorner.Y;
	frameRect.LowerRightCorner.X = frameRect.UpperLeftCorner.X + 1;
	skin->draw2DRectangle(this, skin->getColor(EGDC_3D_SHADOW), frameRect, &AbsoluteClippingRect);

	frameRect = AbsoluteRect;
	frameRect.UpperLeftCorner.X = frameRect.LowerRightCorner.X - 1;
	skin->draw2DRectangle(this, skin->getColor(EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

	frameRect = AbsoluteRect;
	frameRect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - 1;
	skin->draw2DRectangle(this, skin->getColor(EGDC_3D_HIGH_LIGHT), frameRect, &AbsoluteClippingRect);

	// draw the mesh

	if (Mesh)
	{
		//TODO: if outside of screen, dont draw.
		// - why is the absolute clipping rect not already the screen?

		core::rect<s32> oldViewPort = driver->getViewPort();

		driver->setViewPort(viewPort);

		core::matrix4 mat;

		//CameraControl->calculateProjectionMatrix(mat);
		//driver->setTransform(video::TS_PROJECTION, mat);

		mat.makeIdentity();
		mat.setTranslation(core::vector3df(0,0,0));
		driver->setTransform(video::ETS_WORLD, mat);

		//CameraControl->calculateViewMatrix(mat);
		//driver->setTransform(video::TS_VIEW, mat);

		driver->setMaterial(Material);

		u32 frame = 0;
		if(Mesh->getFrameCount())
			frame = (os::Timer::getTime()/20)%Mesh->getFrameCount();
		const scene::IMesh* const m = Mesh->getMesh(frame);
		for (u32 i=0; i<m->getMeshBufferCount(); ++i)
		{
			scene::IMeshBuffer* mb = m->getMeshBuffer(i);
			driver->drawVertexPrimitiveList(mb->getVertices(),
					mb->getVertexCount(), mb->getIndices(),
					mb->getIndexCount()/ 3, mb->getVertexType(),
					scene::EPT_TRIANGLES, mb->getIndexType());
		}

		driver->setViewPort(oldViewPort);
	}

	IGUIElement::draw();
}


} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

