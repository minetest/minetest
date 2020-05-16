// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_BILLBOARD_TEXT_SCENE_NODE_H_INCLUDED__
#define __I_BILLBOARD_TEXT_SCENE_NODE_H_INCLUDED__

#include "IBillboardSceneNode.h"

namespace irr
{
namespace scene
{

//! A billboard text scene node.
/** Acts like a billboard which displays the currently set text.
  Due to the exclusion of RTTI in Irrlicht we have to avoid multiple
  inheritance. Hence, changes to the ITextSceneNode interface have
  to be copied here manually.
*/
class IBillboardTextSceneNode : public IBillboardSceneNode
{
public:

	//! Constructor
	IBillboardTextSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
		const core::vector3df& position = core::vector3df(0,0,0))
		: IBillboardSceneNode(parent, mgr, id, position) {}

	//! Sets the size of the billboard.
	virtual void setSize(const core::dimension2d<f32>& size) = 0;

	//! Returns the size of the billboard.
	virtual const core::dimension2d<f32>& getSize() const = 0;

	//! Set the color of all vertices of the billboard
	/** \param overallColor: the color to set */
	virtual void setColor(const video::SColor & overallColor) = 0;

	//! Set the color of the top and bottom vertices of the billboard
	/** \param topColor: the color to set the top vertices
	\param bottomColor: the color to set the bottom vertices */
	virtual void setColor(const video::SColor & topColor, const video::SColor & bottomColor) = 0;

	//! Gets the color of the top and bottom vertices of the billboard
	/** \param topColor: stores the color of the top vertices
	\param bottomColor: stores the color of the bottom vertices */
	virtual void getColor(video::SColor & topColor, video::SColor & bottomColor) const = 0;

	//! sets the text string
	virtual void setText(const wchar_t* text) = 0;

	//! sets the color of the text
	virtual void setTextColor(video::SColor color) = 0;
};

} // end namespace scene
} // end namespace irr


#endif

