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

#include "guiEditBox.h"

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUIFont.h"

GUIEditBox::~GUIEditBox()
{
	if (m_override_font)
		m_override_font->drop();
}

void GUIEditBox::setOverrideFont(IGUIFont *font)
{
	if (m_override_font == font)
		return;

	if (m_override_font)
		m_override_font->drop();

	m_override_font = font;

	if (m_override_font)
		m_override_font->grab();

	breakText();
}

//! Get the font which is used right now for drawing
IGUIFont *GUIEditBox::getActiveFont() const
{
	if (m_override_font)
		return m_override_font;
	IGUISkin *skin = Environment->getSkin();
	if (skin)
		return skin->getFont();
	return 0;
}

//! Sets another color for the text.
void GUIEditBox::setOverrideColor(video::SColor color)
{
	m_override_color = color;
	m_override_color_enabled = true;
}

video::SColor GUIEditBox::getOverrideColor() const
{
	return m_override_color;
}

//! Sets if the text should use the overide color or the color in the gui skin.
void GUIEditBox::enableOverrideColor(bool enable)
{
	m_override_color_enabled = enable;
}

//! Enables or disables word wrap
void GUIEditBox::setWordWrap(bool enable)
{
	m_word_wrap = enable;
	breakText();
}

//! Enables or disables newlines.
void GUIEditBox::setMultiLine(bool enable)
{
	m_multiline = enable;
}

//! Enables or disables automatic scrolling with cursor position
//! \param enable: If set to true, the text will move around with the cursor position
void GUIEditBox::setAutoScroll(bool enable)
{
	m_autoscroll = enable;
}

void GUIEditBox::setPasswordBox(bool password_box, wchar_t password_char)
{
	m_passwordbox = password_box;
	if (m_passwordbox) {
		m_passwordchar = password_char;
		setMultiLine(false);
		setWordWrap(false);
		m_broken_text.clear();
	}
}

//! Sets text justification
void GUIEditBox::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
	m_halign = horizontal;
	m_valign = vertical;
}

//! Sets the new caption of this element.
void GUIEditBox::setText(const wchar_t* text)
{
	Text = text;
	if (u32(m_cursor_pos) > Text.size())
		m_cursor_pos = Text.size();
	m_hscroll_pos = 0;
	breakText();
}

//! Sets the maximum amount of characters which may be entered in the box.
//! \param max: Maximum amount of characters. If 0, the character amount is
//! infinity.
void GUIEditBox::setMax(u32 max)
{
	m_max = max;

	if (Text.size() > m_max && m_max != 0)
		Text = Text.subString(0, m_max);
}
