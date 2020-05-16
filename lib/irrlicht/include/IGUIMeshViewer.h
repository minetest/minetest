// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_MESH_VIEWER_H_INCLUDED__
#define __I_GUI_MESH_VIEWER_H_INCLUDED__

#include "IGUIElement.h"

namespace irr
{

namespace video
{
	class SMaterial;
} // end namespace video

namespace scene
{
	class IAnimatedMesh;
} // end namespace scene

namespace gui
{

	//! 3d mesh viewing GUI element.
	class IGUIMeshViewer : public IGUIElement
	{
	public:

		//! constructor
		IGUIMeshViewer(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_MESH_VIEWER, environment, parent, id, rectangle) {}

		//! Sets the mesh to be shown
		virtual void setMesh(scene::IAnimatedMesh* mesh) = 0;

		//! Gets the displayed mesh
		virtual scene::IAnimatedMesh* getMesh() const = 0;

		//! Sets the material
		virtual void setMaterial(const video::SMaterial& material) = 0;

		//! Gets the material
		virtual const video::SMaterial& getMaterial() const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

