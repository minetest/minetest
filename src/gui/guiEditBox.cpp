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

#include "porting.h"

GUIEditBox::~GUIEditBox()
{
	if (m_override_font)
		m_override_font->drop();

	if (m_operator)
		m_operator->drop();

	if (m_vscrollbar)
		m_vscrollbar->drop();
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
void GUIEditBox::setText(const wchar_t *text)
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

//! Gets the area of the text in the edit box
//! \return Returns the size in pixels of the text
core::dimension2du GUIEditBox::getTextDimension()
{
	core::rect<s32> ret;

	setTextRect(0);
	ret = m_current_text_rect;

	for (u32 i = 1; i < m_broken_text.size(); ++i) {
		setTextRect(i);
		ret.addInternalPoint(m_current_text_rect.UpperLeftCorner);
		ret.addInternalPoint(m_current_text_rect.LowerRightCorner);
	}

	return core::dimension2du(ret.getSize());
}

//! Turns the border on or off
void GUIEditBox::setDrawBorder(bool border)
{
	m_border = border;
}

void GUIEditBox::setWritable(bool can_write_text)
{
	m_writable = can_write_text;
}

//! set text markers
void GUIEditBox::setTextMarkers(s32 begin, s32 end)
{
	if (begin != m_mark_begin || end != m_mark_end) {
		m_mark_begin = begin;
		m_mark_end = end;
		sendGuiEvent(EGET_EDITBOX_MARKING_CHANGED);
	}
}

//! send some gui event to parent
void GUIEditBox::sendGuiEvent(EGUI_EVENT_TYPE type)
{
	if (Parent) {
		SEvent e;
		e.EventType = EET_GUI_EVENT;
		e.GUIEvent.Caller = this;
		e.GUIEvent.Element = 0;
		e.GUIEvent.EventType = type;

		Parent->OnEvent(e);
	}
}

bool GUIEditBox::processMouse(const SEvent &event)
{
	switch (event.MouseInput.Event) {
	case irr::EMIE_LMOUSE_LEFT_UP:
		if (Environment->hasFocus(this)) {
			m_cursor_pos = getCursorPos(
					event.MouseInput.X, event.MouseInput.Y);
			if (m_mouse_marking) {
				setTextMarkers(m_mark_begin, m_cursor_pos);
			}
			m_mouse_marking = false;
			calculateScrollPos();
			return true;
		}
		break;
	case irr::EMIE_MOUSE_MOVED: {
		if (m_mouse_marking) {
			m_cursor_pos = getCursorPos(
					event.MouseInput.X, event.MouseInput.Y);
			setTextMarkers(m_mark_begin, m_cursor_pos);
			calculateScrollPos();
			return true;
		}
	} break;
	case EMIE_LMOUSE_PRESSED_DOWN:

		if (!Environment->hasFocus(this)) {
			m_blink_start_time = porting::getTimeMs();
			m_mouse_marking = true;
			m_cursor_pos = getCursorPos(
					event.MouseInput.X, event.MouseInput.Y);
			setTextMarkers(m_cursor_pos, m_cursor_pos);
			calculateScrollPos();
			return true;
		} else {
			if (!AbsoluteClippingRect.isPointInside(core::position2d<s32>(
					    event.MouseInput.X, event.MouseInput.Y))) {
				return false;
			} else {
				// move cursor
				m_cursor_pos = getCursorPos(
						event.MouseInput.X, event.MouseInput.Y);

				s32 newMarkBegin = m_mark_begin;
				if (!m_mouse_marking)
					newMarkBegin = m_cursor_pos;

				m_mouse_marking = true;
				setTextMarkers(newMarkBegin, m_cursor_pos);
				calculateScrollPos();
				return true;
			}
		}
	case EMIE_MOUSE_WHEEL:
		if (m_vscrollbar && m_vscrollbar->isVisible()) {
			s32 pos = m_vscrollbar->getPos();
			s32 step = m_vscrollbar->getSmallStep();
			m_vscrollbar->setPos(pos - event.MouseInput.Wheel * step);
		}
		break;
	default:
		break;
	}

	return false;
}

s32 GUIEditBox::getLineFromPos(s32 pos)
{
	if (!m_word_wrap && !m_multiline)
		return 0;

	s32 i = 0;
	while (i < (s32)m_broken_text_positions.size()) {
		if (m_broken_text_positions[i] > pos)
			return i - 1;
		++i;
	}
	return (s32)m_broken_text_positions.size() - 1;
}

void GUIEditBox::updateVScrollBar()
{
	if (!m_vscrollbar) {
		return;
	}

	// OnScrollBarChanged(...)
	if (m_vscrollbar->getPos() != m_vscroll_pos) {
		s32 deltaScrollY = m_vscrollbar->getPos() - m_vscroll_pos;
		m_current_text_rect.UpperLeftCorner.Y -= deltaScrollY;
		m_current_text_rect.LowerRightCorner.Y -= deltaScrollY;

		s32 scrollymax = getTextDimension().Height - m_frame_rect.getHeight();
		if (scrollymax != m_vscrollbar->getMax()) {
			// manage a newline or a deleted line
			m_vscrollbar->setMax(scrollymax);
			m_vscrollbar->setPageSize(s32(getTextDimension().Height));
			calculateScrollPos();
		} else {
			// manage a newline or a deleted line
			m_vscroll_pos = m_vscrollbar->getPos();
		}
	}

	// check if a vertical scrollbar is needed ?
	if (getTextDimension().Height > (u32)m_frame_rect.getHeight()) {
		m_frame_rect.LowerRightCorner.X -= m_scrollbar_width;

		s32 scrollymax = getTextDimension().Height - m_frame_rect.getHeight();
		if (scrollymax != m_vscrollbar->getMax()) {
			m_vscrollbar->setMax(scrollymax);
			m_vscrollbar->setPageSize(s32(getTextDimension().Height));
		}

		if (!m_vscrollbar->isVisible()) {
			m_vscrollbar->setVisible(true);
		}
	} else {
		if (m_vscrollbar->isVisible()) {
			m_vscrollbar->setVisible(false);
			m_vscroll_pos = 0;
			m_vscrollbar->setPos(0);
			m_vscrollbar->setMax(1);
			m_vscrollbar->setPageSize(s32(getTextDimension().Height));
		}
	}
}