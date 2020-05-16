// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_TOOL_BAR_H_INCLUDED__
#define __I_GUI_TOOL_BAR_H_INCLUDED__

#include "IGUIElement.h"

namespace irr
{
namespace video
{
	class ITexture;
} // end namespace video
namespace gui
{
	class IGUIButton;

	//! Stays at the top of its parent like the menu bar and contains tool buttons
	class IGUIToolBar : public IGUIElement
	{
	public:

		//! constructor
		IGUIToolBar(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_TOOL_BAR, environment, parent, id, rectangle) {}

		//! Adds a button to the tool bar
		virtual IGUIButton* addButton(s32 id=-1, const wchar_t* text=0,const wchar_t* tooltiptext=0,
			video::ITexture* img=0, video::ITexture* pressedimg=0,
			bool isPushButton=false, bool useAlphaChannel=false) = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

