/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "guiButton.h"
#include "IGUIButton.h"
#include "guiAnimatedImage.h"

using namespace irr;

class GUIButtonImage : public GUIButton
{
public:
	//! constructor
	GUIButtonImage(gui::IGUIEnvironment *environment, gui::IGUIElement *parent,
			s32 id, core::rect<s32> rectangle, ISimpleTextureSource *tsrc,
			bool noclip = false);

	void setForegroundImage(video::ITexture *image = nullptr,
			const core::rect<s32> &middle = core::rect<s32>());

	//! Set element properties from a StyleSpec
	virtual void setFromStyle(const StyleSpec &style) override;

	//! Do not drop returned handle
	static GUIButtonImage *addButton(gui::IGUIEnvironment *environment,
			const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
			IGUIElement *parent, s32 id, const wchar_t *text,
			const wchar_t *tooltiptext = L"");

private:
	video::ITexture *m_foreground_image = nullptr;
	GUIAnimatedImage *m_image;
};
