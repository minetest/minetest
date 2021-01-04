/*
Minetest
Copyright (C) 2021 Minetest

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

#include "IGUIEditBox.h"
#include "IOSOperator.h"
#include "guiScrollBar.h"

using namespace irr;
using namespace irr::gui;

class GUIEditBox : public IGUIEditBox
{
public:
	GUIEditBox(IGUIEnvironment *environment, IGUIElement *parent, s32 id,
			core::rect<s32> rectangle) :
			IGUIEditBox(environment, parent, id, rectangle)
	{
	}

	virtual ~GUIEditBox();

	//! Sets another skin independent font.
	virtual void setOverrideFont(IGUIFont *font = 0);

	virtual IGUIFont *getOverrideFont() const { return m_override_font; }

	//! Get the font which is used right now for drawing
	/** Currently this is the override font when one is set and the
	font of the active skin otherwise */
	virtual IGUIFont *getActiveFont() const;

	//! Sets another color for the text.
	virtual void setOverrideColor(video::SColor color);

	//! Gets the override color
	virtual video::SColor getOverrideColor() const;

	//! Sets if the text should use the overide color or the
	//! color in the gui skin.
	virtual void enableOverrideColor(bool enable);

	//! Checks if an override color is enabled
	/** \return true if the override color is enabled, false otherwise */
	virtual bool isOverrideColorEnabled(void) const
	{
		return m_override_color_enabled;
	}

	//! Enables or disables word wrap for using the edit box as multiline text editor.
	virtual void setWordWrap(bool enable);

	//! Checks if word wrap is enabled
	//! \return true if word wrap is enabled, false otherwise
	virtual bool isWordWrapEnabled() const { return m_word_wrap; }

	//! Enables or disables newlines.
	/** \param enable: If set to true, the EGET_EDITBOX_ENTER event will not be fired,
	instead a newline character will be inserted. */
	virtual void setMultiLine(bool enable);

	//! Checks if multi line editing is enabled
	//! \return true if mult-line is enabled, false otherwise
	virtual bool isMultiLineEnabled() const { return m_multiline; }

	//! Enables or disables automatic scrolling with cursor position
	//! \param enable: If set to true, the text will move around with the cursor
	//! position
	virtual void setAutoScroll(bool enable);

	//! Checks to see if automatic scrolling is enabled
	//! \return true if automatic scrolling is enabled, false if not
	virtual bool isAutoScrollEnabled() const { return m_autoscroll; }

protected:
	virtual void breakText() = 0;

	gui::IGUIFont *m_override_font = nullptr;

	bool m_override_color_enabled = false;
	bool m_word_wrap = false;
	bool m_multiline = false;
	bool m_autoscroll = true;

	video::SColor m_override_color = video::SColor(101, 255, 255, 255);
};