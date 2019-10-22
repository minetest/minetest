// Copyright (C) 2002-2012 Nikolaus Gebhardt
// Modified by Mustapha T.
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiEditBoxWithScrollbar.h"

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUIFont.h"
#include "IVideoDriver.h"
#include "rect.h"
#include "porting.h"
#include "Keycodes.h"

/*
todo:
optional scrollbars [done]
ctrl+left/right to select word
double click/ctrl click: word select + drag to select whole words, triple click to select line
optional? dragging selected text
numerical
*/


//! constructor
GUIEditBoxWithScrollBar::GUIEditBoxWithScrollBar(const wchar_t* text, bool border,
	IGUIEnvironment* environment, IGUIElement* parent, s32 id,
	const core::rect<s32>& rectangle, bool writable, bool has_vscrollbar)
	: IGUIEditBox(environment, parent, id, rectangle), m_mouse_marking(false),
	m_border(border), m_background(true), m_override_color_enabled(false), m_mark_begin(0), m_mark_end(0),
	m_override_color(video::SColor(101, 255, 255, 255)), m_override_font(0), m_last_break_font(0),
	m_operator(0), m_blink_start_time(0), m_cursor_pos(0), m_hscroll_pos(0), m_vscroll_pos(0), m_max(0),
	m_word_wrap(false), m_multiline(false), m_autoscroll(true), m_passwordbox(false),
	m_passwordchar(L'*'), m_halign(EGUIA_UPPERLEFT), m_valign(EGUIA_CENTER),
	m_current_text_rect(0, 0, 1, 1), m_frame_rect(rectangle),
	m_scrollbar_width(0), m_vscrollbar(NULL), m_writable(writable),
	m_bg_color_used(false)
{
#ifdef _DEBUG
	setDebugName("GUIEditBoxWithScrollBar");
#endif


	Text = text;

	if (Environment)
		m_operator = Environment->getOSOperator();

	if (m_operator)
		m_operator->grab();

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	if (has_vscrollbar) {
		createVScrollBar();
	}

	calculateFrameRect();
	breakText();

	calculateScrollPos();
	setWritable(writable);
}


//! destructor
GUIEditBoxWithScrollBar::~GUIEditBoxWithScrollBar()
{
	if (m_override_font)
		m_override_font->drop();

	if (m_operator)
		m_operator->drop();

	if (m_vscrollbar)
		m_vscrollbar->drop();
}


//! Sets another skin independent font.
void GUIEditBoxWithScrollBar::setOverrideFont(IGUIFont* font)
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

//! Gets the override font (if any)
IGUIFont * GUIEditBoxWithScrollBar::getOverrideFont() const
{
	return m_override_font;
}

//! Get the font which is used right now for drawing
IGUIFont* GUIEditBoxWithScrollBar::getActiveFont() const
{
	if (m_override_font)
		return m_override_font;
	IGUISkin* skin = Environment->getSkin();
	if (skin)
		return skin->getFont();
	return 0;
}

//! Sets another color for the text.
void GUIEditBoxWithScrollBar::setOverrideColor(video::SColor color)
{
	m_override_color = color;
	m_override_color_enabled = true;
}


video::SColor GUIEditBoxWithScrollBar::getOverrideColor() const
{
	return m_override_color;
}


//! Turns the border on or off
void GUIEditBoxWithScrollBar::setDrawBorder(bool border)
{
	m_border = border;
}

//! Sets whether to draw the background
void GUIEditBoxWithScrollBar::setDrawBackground(bool draw)
{
	m_background = draw;
}

//! Sets if the text should use the overide color or the color in the gui skin.
void GUIEditBoxWithScrollBar::enableOverrideColor(bool enable)
{
	m_override_color_enabled = enable;
}

bool GUIEditBoxWithScrollBar::isOverrideColorEnabled() const
{
	return m_override_color_enabled;
}

//! Enables or disables word wrap
void GUIEditBoxWithScrollBar::setWordWrap(bool enable)
{
	m_word_wrap = enable;
	breakText();
}


void GUIEditBoxWithScrollBar::updateAbsolutePosition()
{
	core::rect<s32> old_absolute_rect(AbsoluteRect);
	IGUIElement::updateAbsolutePosition();
	if (old_absolute_rect != AbsoluteRect) {
		calculateFrameRect();
		breakText();
		calculateScrollPos();
	}
}

//! Checks if word wrap is enabled
bool GUIEditBoxWithScrollBar::isWordWrapEnabled() const
{
	return m_word_wrap;
}


//! Enables or disables newlines.
void GUIEditBoxWithScrollBar::setMultiLine(bool enable)
{
	m_multiline = enable;
}


//! Checks if multi line editing is enabled
bool GUIEditBoxWithScrollBar::isMultiLineEnabled() const
{
	return m_multiline;
}


void GUIEditBoxWithScrollBar::setPasswordBox(bool password_box, wchar_t password_char)
{
	m_passwordbox = password_box;
	if (m_passwordbox) {
		m_passwordchar = password_char;
		setMultiLine(false);
		setWordWrap(false);
		m_broken_text.clear();
	}
}


bool GUIEditBoxWithScrollBar::isPasswordBox() const
{
	return m_passwordbox;
}


//! Sets text justification
void GUIEditBoxWithScrollBar::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
	m_halign = horizontal;
	m_valign = vertical;
}


//! called if an event happened.
bool GUIEditBoxWithScrollBar::OnEvent(const SEvent& event)
{
	if (isEnabled()) {
		switch (event.EventType)
		{
		case EET_GUI_EVENT:
			if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST) {
				if (event.GUIEvent.Caller == this) {
					m_mouse_marking = false;
					setTextMarkers(0, 0);
				}
			}
			break;
		case EET_KEY_INPUT_EVENT:
			if (processKey(event))
				return true;
			break;
		case EET_MOUSE_INPUT_EVENT:
			if (processMouse(event))
				return true;
			break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}


bool GUIEditBoxWithScrollBar::processKey(const SEvent& event)
{
	if (!m_writable) {
		return false;
	}

	if (!event.KeyInput.PressedDown)
		return false;

	bool text_changed = false;
	s32 new_mark_begin = m_mark_begin;
	s32 new_mark_end = m_mark_end;

	// control shortcut handling

	if (event.KeyInput.Control) {

		// german backlash '\' entered with control + '?'
		if (event.KeyInput.Char == '\\') {
			inputChar(event.KeyInput.Char);
			return true;
		}

		switch (event.KeyInput.Key) {
		case KEY_KEY_A:
			// select all
			new_mark_begin = 0;
			new_mark_end = Text.size();
			break;
		case KEY_KEY_C:
			// copy to clipboard
			if (!m_passwordbox && m_operator && m_mark_begin != m_mark_end)
			{
				const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
				const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

				core::stringc s;
				s = Text.subString(realmbgn, realmend - realmbgn).c_str();
				m_operator->copyToClipboard(s.c_str());
			}
			break;
		case KEY_KEY_X:
			// cut to the clipboard
			if (!m_passwordbox && m_operator && m_mark_begin != m_mark_end) {
				const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
				const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

				// copy
				core::stringc sc;
				sc = Text.subString(realmbgn, realmend - realmbgn).c_str();
				m_operator->copyToClipboard(sc.c_str());

				if (isEnabled())
				{
					// delete
					core::stringw s;
					s = Text.subString(0, realmbgn);
					s.append(Text.subString(realmend, Text.size() - realmend));
					Text = s;

					m_cursor_pos = realmbgn;
					new_mark_begin = 0;
					new_mark_end = 0;
					text_changed = true;
				}
			}
			break;
		case KEY_KEY_V:
			if (!isEnabled())
				break;

			// paste from the clipboard
			if (m_operator) {
				const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
				const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

				// add new character
				const c8* p = m_operator->getTextFromClipboard();
				if (p) {
					if (m_mark_begin == m_mark_end) {
						// insert text
						core::stringw s = Text.subString(0, m_cursor_pos);
						s.append(p);
						s.append(Text.subString(m_cursor_pos, Text.size() - m_cursor_pos));

						if (!m_max || s.size() <= m_max) // thx to Fish FH for fix
						{
							Text = s;
							s = p;
							m_cursor_pos += s.size();
						}
					} else {
						// replace text

						core::stringw s = Text.subString(0, realmbgn);
						s.append(p);
						s.append(Text.subString(realmend, Text.size() - realmend));

						if (!m_max || s.size() <= m_max)  // thx to Fish FH for fix
						{
							Text = s;
							s = p;
							m_cursor_pos = realmbgn + s.size();
						}
					}
				}

				new_mark_begin = 0;
				new_mark_end = 0;
				text_changed = true;
			}
			break;
		case KEY_HOME:
			// move/highlight to start of text
			if (event.KeyInput.Shift) {
				new_mark_end = m_cursor_pos;
				new_mark_begin = 0;
				m_cursor_pos = 0;
			} else {
				m_cursor_pos = 0;
				new_mark_begin = 0;
				new_mark_end = 0;
			}
			break;
		case KEY_END:
			// move/highlight to end of text
			if (event.KeyInput.Shift) {
				new_mark_begin = m_cursor_pos;
				new_mark_end = Text.size();
				m_cursor_pos = 0;
			} else {
				m_cursor_pos = Text.size();
				new_mark_begin = 0;
				new_mark_end = 0;
			}
			break;
		default:
			return false;
		}
	}
	// default keyboard handling
	else
		switch (event.KeyInput.Key)	{
		case KEY_END:
		{
			s32 p = Text.size();
			if (m_word_wrap || m_multiline) {
				p = getLineFromPos(m_cursor_pos);
				p = m_broken_text_positions[p] + (s32)m_broken_text[p].size();
				if (p > 0 && (Text[p - 1] == L'\r' || Text[p - 1] == L'\n'))
					p -= 1;
			}

			if (event.KeyInput.Shift) {
				if (m_mark_begin == m_mark_end)
					new_mark_begin = m_cursor_pos;

				new_mark_end = p;
			} else {
				new_mark_begin = 0;
				new_mark_end = 0;
			}
			m_cursor_pos = p;
			m_blink_start_time = porting::getTimeMs();
		}
		break;
		case KEY_HOME:
		{

			s32 p = 0;
			if (m_word_wrap || m_multiline) {
				p = getLineFromPos(m_cursor_pos);
				p = m_broken_text_positions[p];
			}

			if (event.KeyInput.Shift) {
				if (m_mark_begin == m_mark_end)
					new_mark_begin = m_cursor_pos;
				new_mark_end = p;
			} else {
				new_mark_begin = 0;
				new_mark_end = 0;
			}
			m_cursor_pos = p;
			m_blink_start_time = porting::getTimeMs();
		}
		break;
		case KEY_RETURN:
			if (m_multiline) {
				inputChar(L'\n');
			} else {
				calculateScrollPos();
				sendGuiEvent(EGET_EDITBOX_ENTER);
			}
			return true;
		case KEY_LEFT:

			if (event.KeyInput.Shift) {
				if (m_cursor_pos > 0) {
					if (m_mark_begin == m_mark_end)
						new_mark_begin = m_cursor_pos;

					new_mark_end = m_cursor_pos - 1;
				}
			} else {
				new_mark_begin = 0;
				new_mark_end = 0;
			}

			if (m_cursor_pos > 0)
				m_cursor_pos--;
			m_blink_start_time = porting::getTimeMs();
			break;

		case KEY_RIGHT:
			if (event.KeyInput.Shift) {
				if (Text.size() > (u32)m_cursor_pos) {
					if (m_mark_begin == m_mark_end)
						new_mark_begin = m_cursor_pos;

					new_mark_end = m_cursor_pos + 1;
				}
			} else {
				new_mark_begin = 0;
				new_mark_end = 0;
			}

			if (Text.size() > (u32)m_cursor_pos)
				m_cursor_pos++;
			m_blink_start_time = porting::getTimeMs();
			break;
		case KEY_UP:
			if (m_multiline || (m_word_wrap && m_broken_text.size() > 1)) {
				s32 lineNo = getLineFromPos(m_cursor_pos);
				s32 mb = (m_mark_begin == m_mark_end) ? m_cursor_pos : (m_mark_begin > m_mark_end ? m_mark_begin : m_mark_end);
				if (lineNo > 0)	{
					s32 cp = m_cursor_pos - m_broken_text_positions[lineNo];
					if ((s32)m_broken_text[lineNo - 1].size() < cp)
						m_cursor_pos = m_broken_text_positions[lineNo - 1] + core::max_((u32)1, m_broken_text[lineNo - 1].size()) - 1;
					else
						m_cursor_pos = m_broken_text_positions[lineNo - 1] + cp;
				}

				if (event.KeyInput.Shift) {
					new_mark_begin = mb;
					new_mark_end = m_cursor_pos;
				} else {
					new_mark_begin = 0;
					new_mark_end = 0;
				}
			} else {
				return false;
			}
			break;
		case KEY_DOWN:
			if (m_multiline || (m_word_wrap && m_broken_text.size() > 1)) {
				s32 lineNo = getLineFromPos(m_cursor_pos);
				s32 mb = (m_mark_begin == m_mark_end) ? m_cursor_pos : (m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end);
				if (lineNo < (s32)m_broken_text.size() - 1)
				{
					s32 cp = m_cursor_pos - m_broken_text_positions[lineNo];
					if ((s32)m_broken_text[lineNo + 1].size() < cp)
						m_cursor_pos = m_broken_text_positions[lineNo + 1] + core::max_((u32)1, m_broken_text[lineNo + 1].size()) - 1;
					else
						m_cursor_pos = m_broken_text_positions[lineNo + 1] + cp;
				}

				if (event.KeyInput.Shift) {
					new_mark_begin = mb;
					new_mark_end = m_cursor_pos;
				} else {
					new_mark_begin = 0;
					new_mark_end = 0;
				}

			} else {
				return false;
			}
			break;

		case KEY_BACK:
			if (!isEnabled())
				break;

			if (Text.size()) {
				core::stringw s;

				if (m_mark_begin != m_mark_end) {
					// delete marked text
					const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
					const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

					s = Text.subString(0, realmbgn);
					s.append(Text.subString(realmend, Text.size() - realmend));
					Text = s;

					m_cursor_pos = realmbgn;
				} else {
					// delete text behind cursor
					if (m_cursor_pos > 0)
						s = Text.subString(0, m_cursor_pos - 1);
					else
						s = L"";
					s.append(Text.subString(m_cursor_pos, Text.size() - m_cursor_pos));
					Text = s;
					--m_cursor_pos;
				}

				if (m_cursor_pos < 0)
					m_cursor_pos = 0;
				m_blink_start_time = porting::getTimeMs(); // os::Timer::getTime();
				new_mark_begin = 0;
				new_mark_end = 0;
				text_changed = true;
			}
			break;
		case KEY_DELETE:
			if (!isEnabled())
				break;

			if (Text.size() != 0) {
				core::stringw s;

				if (m_mark_begin != m_mark_end) {
					// delete marked text
					const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
					const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

					s = Text.subString(0, realmbgn);
					s.append(Text.subString(realmend, Text.size() - realmend));
					Text = s;

					m_cursor_pos = realmbgn;
				} else {
					// delete text before cursor
					s = Text.subString(0, m_cursor_pos);
					s.append(Text.subString(m_cursor_pos + 1, Text.size() - m_cursor_pos - 1));
					Text = s;
				}

				if (m_cursor_pos > (s32)Text.size())
					m_cursor_pos = (s32)Text.size();

				m_blink_start_time = porting::getTimeMs(); // os::Timer::getTime();
				new_mark_begin = 0;
				new_mark_end = 0;
				text_changed = true;
			}
			break;

		case KEY_ESCAPE:
		case KEY_TAB:
		case KEY_SHIFT:
		case KEY_F1:
		case KEY_F2:
		case KEY_F3:
		case KEY_F4:
		case KEY_F5:
		case KEY_F6:
		case KEY_F7:
		case KEY_F8:
		case KEY_F9:
		case KEY_F10:
		case KEY_F11:
		case KEY_F12:
		case KEY_F13:
		case KEY_F14:
		case KEY_F15:
		case KEY_F16:
		case KEY_F17:
		case KEY_F18:
		case KEY_F19:
		case KEY_F20:
		case KEY_F21:
		case KEY_F22:
		case KEY_F23:
		case KEY_F24:
			// ignore these keys
			return false;

		default:
			inputChar(event.KeyInput.Char);
			return true;
		}

	// Set new text markers
	setTextMarkers(new_mark_begin, new_mark_end);

	// break the text if it has changed
	if (text_changed) {
		breakText();
		calculateScrollPos();
		sendGuiEvent(EGET_EDITBOX_CHANGED);
	}
	else
	{
		calculateScrollPos();
	}

	return true;
}


//! draws the element and its children
void GUIEditBoxWithScrollBar::draw()
{
	if (!IsVisible)
		return;

	const bool focus = Environment->hasFocus(this);

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	video::SColor default_bg_color;
	video::SColor bg_color;

	default_bg_color = m_writable ? skin->getColor(EGDC_WINDOW) : video::SColor(0);
	bg_color = m_bg_color_used ? m_bg_color : default_bg_color;

	if (!m_border && m_background) {
		skin->draw2DRectangle(this, bg_color, AbsoluteRect, &AbsoluteClippingRect);
	}

	// draw the border

	if (m_border) {

		if (m_writable) {
			skin->draw3DSunkenPane(this, bg_color, false, m_background,
				AbsoluteRect, &AbsoluteClippingRect);
		}

		calculateFrameRect();
	}

	core::rect<s32> local_clip_rect = m_frame_rect;
	local_clip_rect.clipAgainst(AbsoluteClippingRect);

	// draw the text

	IGUIFont* font = getActiveFont();

	s32 cursor_line = 0;
	s32 charcursorpos = 0;

	if (font) {
		if (m_last_break_font != font) {
			breakText();
		}

		// calculate cursor pos

		core::stringw *txt_line = &Text;
		s32 start_pos = 0;

		core::stringw s, s2;

		// get mark position
		const bool ml = (!m_passwordbox && (m_word_wrap || m_multiline));
		const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
		const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;
		const s32 hline_start = ml ? getLineFromPos(realmbgn) : 0;
		const s32 hline_count = ml ? getLineFromPos(realmend) - hline_start + 1 : 1;
		const s32 line_count = ml ? m_broken_text.size() : 1;

		// Save the override color information.
		// Then, alter it if the edit box is disabled.
		const bool prevOver = m_override_color_enabled;
		const video::SColor prevColor = m_override_color;

		if (Text.size()) {
			if (!isEnabled() && !m_override_color_enabled) {
				m_override_color_enabled = true;
				m_override_color = skin->getColor(EGDC_GRAY_TEXT);
			}

			for (s32 i = 0; i < line_count; ++i) {
				setTextRect(i);

				// clipping test - don't draw anything outside the visible area
				core::rect<s32> c = local_clip_rect;
				c.clipAgainst(m_current_text_rect);
				if (!c.isValid())
					continue;

				// get current line
				if (m_passwordbox) {
					if (m_broken_text.size() != 1) {
						m_broken_text.clear();
						m_broken_text.emplace_back();
					}
					if (m_broken_text[0].size() != Text.size()){
						m_broken_text[0] = Text;
						for (u32 q = 0; q < Text.size(); ++q)
						{
							m_broken_text[0][q] = m_passwordchar;
						}
					}
					txt_line = &m_broken_text[0];
					start_pos = 0;
				} else {
					txt_line = ml ? &m_broken_text[i] : &Text;
					start_pos = ml ? m_broken_text_positions[i] : 0;
				}


				// draw normal text
				font->draw(txt_line->c_str(), m_current_text_rect,
					m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &local_clip_rect);

				// draw mark and marked text
				if (focus && m_mark_begin != m_mark_end && i >= hline_start && i < hline_start + hline_count) {

					s32 mbegin = 0, mend = 0;
					s32 lineStartPos = 0, lineEndPos = txt_line->size();

					if (i == hline_start) {
						// highlight start is on this line
						s = txt_line->subString(0, realmbgn - start_pos);
						mbegin = font->getDimension(s.c_str()).Width;

						// deal with kerning
						mbegin += font->getKerningWidth(
							&((*txt_line)[realmbgn - start_pos]),
							realmbgn - start_pos > 0 ? &((*txt_line)[realmbgn - start_pos - 1]) : 0);

						lineStartPos = realmbgn - start_pos;
					}
					if (i == hline_start + hline_count - 1) {
						// highlight end is on this line
						s2 = txt_line->subString(0, realmend - start_pos);
						mend = font->getDimension(s2.c_str()).Width;
						lineEndPos = (s32)s2.size();
					} else {
						mend = font->getDimension(txt_line->c_str()).Width;
					}


					m_current_text_rect.UpperLeftCorner.X += mbegin;
					m_current_text_rect.LowerRightCorner.X = m_current_text_rect.UpperLeftCorner.X + mend - mbegin;


					// draw mark
					skin->draw2DRectangle(this, skin->getColor(EGDC_HIGH_LIGHT), m_current_text_rect, &local_clip_rect);

					// draw marked text
					s = txt_line->subString(lineStartPos, lineEndPos - lineStartPos);

					if (s.size())
						font->draw(s.c_str(), m_current_text_rect,
							m_override_color_enabled ? m_override_color : skin->getColor(EGDC_HIGH_LIGHT_TEXT),
							false, true, &local_clip_rect);

				}
			}

			// Return the override color information to its previous settings.
			m_override_color_enabled = prevOver;
			m_override_color = prevColor;
		}

		// draw cursor
		if (IsEnabled && m_writable) {
			if (m_word_wrap || m_multiline) {
				cursor_line = getLineFromPos(m_cursor_pos);
				txt_line = &m_broken_text[cursor_line];
				start_pos = m_broken_text_positions[cursor_line];
			}
			s = txt_line->subString(0, m_cursor_pos - start_pos);
			charcursorpos = font->getDimension(s.c_str()).Width +
				font->getKerningWidth(L"_", m_cursor_pos - start_pos > 0 ? &((*txt_line)[m_cursor_pos - start_pos - 1]) : 0);

			if (focus && (porting::getTimeMs() - m_blink_start_time) % 700 < 350) {
				setTextRect(cursor_line);
				m_current_text_rect.UpperLeftCorner.X += charcursorpos;

				font->draw(L"_", m_current_text_rect,
					m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &local_clip_rect);
			}
		}
	}

	// draw children
	IGUIElement::draw();
}


//! Sets the new caption of this element.
void GUIEditBoxWithScrollBar::setText(const wchar_t* text)
{
	Text = text;
	if (u32(m_cursor_pos) > Text.size())
		m_cursor_pos = Text.size();
	m_hscroll_pos = 0;
	breakText();
}


//! Enables or disables automatic scrolling with cursor position
//! \param enable: If set to true, the text will move around with the cursor position
void GUIEditBoxWithScrollBar::setAutoScroll(bool enable)
{
	m_autoscroll = enable;
}


//! Checks to see if automatic scrolling is enabled
//! \return true if automatic scrolling is enabled, false if not
bool GUIEditBoxWithScrollBar::isAutoScrollEnabled() const
{
	return m_autoscroll;
}


//! Gets the area of the text in the edit box
//! \return Returns the size in pixels of the text
core::dimension2du GUIEditBoxWithScrollBar::getTextDimension()
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


//! Sets the maximum amount of characters which may be entered in the box.
//! \param max: Maximum amount of characters. If 0, the character amount is
//! infinity.
void GUIEditBoxWithScrollBar::setMax(u32 max)
{
	m_max = max;

	if (Text.size() > m_max && m_max != 0)
		Text = Text.subString(0, m_max);
}


//! Returns maximum amount of characters, previously set by setMax();
u32 GUIEditBoxWithScrollBar::getMax() const
{
	return m_max;
}


bool GUIEditBoxWithScrollBar::processMouse(const SEvent& event)
{
	switch (event.MouseInput.Event)
	{
	case irr::EMIE_LMOUSE_LEFT_UP:
		if (Environment->hasFocus(this)) {
			m_cursor_pos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
			if (m_mouse_marking) {
				setTextMarkers(m_mark_begin, m_cursor_pos);
			}
			m_mouse_marking = false;
			calculateScrollPos();
			return true;
		}
		break;
	case irr::EMIE_MOUSE_MOVED:
	{
		if (m_mouse_marking) {
			m_cursor_pos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
			setTextMarkers(m_mark_begin, m_cursor_pos);
			calculateScrollPos();
			return true;
		}
	}
	break;
	case EMIE_LMOUSE_PRESSED_DOWN:

		if (!Environment->hasFocus(this)) {
			m_blink_start_time = porting::getTimeMs();
			m_mouse_marking = true;
			m_cursor_pos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
			setTextMarkers(m_cursor_pos, m_cursor_pos);
			calculateScrollPos();
			return true;
		} else {
			if (!AbsoluteClippingRect.isPointInside(
				core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y)))	{
				return false;
			} else {
				// move cursor
				m_cursor_pos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);

				s32 newMarkBegin = m_mark_begin;
				if (!m_mouse_marking)
					newMarkBegin = m_cursor_pos;

				m_mouse_marking = true;
				setTextMarkers(newMarkBegin, m_cursor_pos);
				calculateScrollPos();
				return true;
			}
		}
	default:
		break;
	}

	return false;
}


s32 GUIEditBoxWithScrollBar::getCursorPos(s32 x, s32 y)
{
	IGUIFont* font = getActiveFont();

	const u32 line_count = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;

	core::stringw *txt_line = 0;
	s32 start_pos = 0;
	x += 3;

	for (u32 i = 0; i < line_count; ++i) {
		setTextRect(i);
		if (i == 0 && y < m_current_text_rect.UpperLeftCorner.Y)
			y = m_current_text_rect.UpperLeftCorner.Y;
		if (i == line_count - 1 && y > m_current_text_rect.LowerRightCorner.Y)
			y = m_current_text_rect.LowerRightCorner.Y;

		// is it inside this region?
		if (y >= m_current_text_rect.UpperLeftCorner.Y && y <= m_current_text_rect.LowerRightCorner.Y) {
			// we've found the clicked line
			txt_line = (m_word_wrap || m_multiline) ? &m_broken_text[i] : &Text;
			start_pos = (m_word_wrap || m_multiline) ? m_broken_text_positions[i] : 0;
			break;
		}
	}

	if (x < m_current_text_rect.UpperLeftCorner.X)
		x = m_current_text_rect.UpperLeftCorner.X;

	if (!txt_line)
		return 0;

	s32 idx = font->getCharacterFromPos(txt_line->c_str(), x - m_current_text_rect.UpperLeftCorner.X);

	// click was on or left of the line
	if (idx != -1)
		return idx + start_pos;

	// click was off the right edge of the line, go to end.
	return txt_line->size() + start_pos;
}


//! Breaks the single text line.
void GUIEditBoxWithScrollBar::breakText()
{
	if ((!m_word_wrap && !m_multiline))
		return;

	m_broken_text.clear(); // need to reallocate :/
	m_broken_text_positions.clear();

	IGUIFont* font = getActiveFont();
	if (!font)
		return;

	m_last_break_font = font;

	core::stringw line;
	core::stringw word;
	core::stringw whitespace;
	s32 last_line_start = 0;
	s32 size = Text.size();
	s32 length = 0;
	s32 el_width = RelativeRect.getWidth() - 6;
	wchar_t c;

	for (s32 i = 0; i < size; ++i) {
		c = Text[i];
		bool line_break = false;

		if (c == L'\r') { // Mac or Windows breaks

			line_break = true;
			c = 0;
			if (Text[i + 1] == L'\n') { // Windows breaks
				// TODO: I (Michael) think that we shouldn't change the text given by the user for whatever reason.
				// Instead rework the cursor positioning to be able to handle this (but not in stable release
				// branch as users might already expect this behavior).
				Text.erase(i + 1);
				--size;
				if (m_cursor_pos > i)
					--m_cursor_pos;
			}
		} else if (c == L'\n') { // Unix breaks
			line_break = true;
			c = 0;
		}

		// don't break if we're not a multi-line edit box
		if (!m_multiline)
			line_break = false;

		if (c == L' ' || c == 0 || i == (size - 1)) {
			// here comes the next whitespace, look if
			// we can break the last word to the next line
			// We also break whitespace, otherwise cursor would vanish beside the right border.
			s32 whitelgth = font->getDimension(whitespace.c_str()).Width;
			s32 worldlgth = font->getDimension(word.c_str()).Width;

			if (m_word_wrap && length + worldlgth + whitelgth > el_width && line.size() > 0) {
				// break to next line
				length = worldlgth;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(last_line_start);
				last_line_start = i - (s32)word.size();
				line = word;
			} else {
				// add word to line
				line += whitespace;
				line += word;
				length += whitelgth + worldlgth;
			}

			word = L"";
			whitespace = L"";


			if (c)
				whitespace += c;

			// compute line break
			if (line_break) {
				line += whitespace;
				line += word;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(last_line_start);
				last_line_start = i + 1;
				line = L"";
				word = L"";
				whitespace = L"";
				length = 0;
			}
		} else {
			// yippee this is a word..
			word += c;
		}
	}

	line += whitespace;
	line += word;
	m_broken_text.push_back(line);
	m_broken_text_positions.push_back(last_line_start);
}

// TODO: that function does interpret VAlign according to line-index (indexed
// line is placed on top-center-bottom) but HAlign according to line-width
// (pixels) and not by row.
// Intuitively I suppose HAlign handling is better as VScrollPos should handle
// the line-scrolling.
// But please no one change this without also rewriting (and this time
// testing!!!) autoscrolling (I noticed this when fixing the old autoscrolling).
void GUIEditBoxWithScrollBar::setTextRect(s32 line)
{
	if (line < 0)
		return;

	IGUIFont* font = getActiveFont();
	if (!font)
		return;

	core::dimension2du d;

	// get text dimension
	const u32 line_count = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;
	if (m_word_wrap || m_multiline) {
		d = font->getDimension(m_broken_text[line].c_str());
	} else {
		d = font->getDimension(Text.c_str());
		d.Height = AbsoluteRect.getHeight();
	}
	d.Height += font->getKerningHeight();

	// justification
	switch (m_halign) {
	case EGUIA_CENTER:
		// align to h centre
		m_current_text_rect.UpperLeftCorner.X = (m_frame_rect.getWidth() / 2) - (d.Width / 2);
		m_current_text_rect.LowerRightCorner.X = (m_frame_rect.getWidth() / 2) + (d.Width / 2);
		break;
	case EGUIA_LOWERRIGHT:
		// align to right edge
		m_current_text_rect.UpperLeftCorner.X = m_frame_rect.getWidth() - d.Width;
		m_current_text_rect.LowerRightCorner.X = m_frame_rect.getWidth();
		break;
	default:
		// align to left edge
		m_current_text_rect.UpperLeftCorner.X = 0;
		m_current_text_rect.LowerRightCorner.X = d.Width;

	}

	switch (m_valign) {
	case EGUIA_CENTER:
		// align to v centre
		m_current_text_rect.UpperLeftCorner.Y =
			(m_frame_rect.getHeight() / 2) - (line_count*d.Height) / 2 + d.Height*line;
		break;
	case EGUIA_LOWERRIGHT:
		// align to bottom edge
		m_current_text_rect.UpperLeftCorner.Y =
			m_frame_rect.getHeight() - line_count*d.Height + d.Height*line;
		break;
	default:
		// align to top edge
		m_current_text_rect.UpperLeftCorner.Y = d.Height*line;
		break;
	}

	m_current_text_rect.UpperLeftCorner.X -= m_hscroll_pos;
	m_current_text_rect.LowerRightCorner.X -= m_hscroll_pos;
	m_current_text_rect.UpperLeftCorner.Y -= m_vscroll_pos;
	m_current_text_rect.LowerRightCorner.Y = m_current_text_rect.UpperLeftCorner.Y + d.Height;

	m_current_text_rect += m_frame_rect.UpperLeftCorner;
}


s32 GUIEditBoxWithScrollBar::getLineFromPos(s32 pos)
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


void GUIEditBoxWithScrollBar::inputChar(wchar_t c)
{
	if (!isEnabled())
		return;

	if (c != 0)	{
		if (Text.size() < m_max || m_max == 0) {
			core::stringw s;

			if (m_mark_begin != m_mark_end) {
				// replace marked text
				const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
				const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

				s = Text.subString(0, realmbgn);
				s.append(c);
				s.append(Text.subString(realmend, Text.size() - realmend));
				Text = s;
				m_cursor_pos = realmbgn + 1;
			} else {
				// add new character
				s = Text.subString(0, m_cursor_pos);
				s.append(c);
				s.append(Text.subString(m_cursor_pos, Text.size() - m_cursor_pos));
				Text = s;
				++m_cursor_pos;
			}

			m_blink_start_time = porting::getTimeMs();
			setTextMarkers(0, 0);
		}
	}
	breakText();
	calculateScrollPos();
	sendGuiEvent(EGET_EDITBOX_CHANGED);
}

// calculate autoscroll
void GUIEditBoxWithScrollBar::calculateScrollPos()
{
	if (!m_autoscroll)
		return;

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	IGUIFont* font = m_override_font ? m_override_font : skin->getFont();
	if (!font)
		return;

	s32 curs_line = getLineFromPos(m_cursor_pos);
	if (curs_line < 0)
		return;
	setTextRect(curs_line);
	const bool has_broken_text = m_multiline || m_word_wrap;

	// Check horizonal scrolling
	// NOTE: Calculations different to vertical scrolling because setTextRect interprets VAlign relative to line but HAlign not relative to row
	{
		// get cursor position
		IGUIFont* font = getActiveFont();
		if (!font)
			return;

		// get cursor area
		irr::u32 cursor_width = font->getDimension(L"_").Width;
		core::stringw *txt_line = has_broken_text ? &m_broken_text[curs_line] : &Text;
		s32 cpos = has_broken_text ? m_cursor_pos - m_broken_text_positions[curs_line] : m_cursor_pos;	// column
		s32 cstart = font->getDimension(txt_line->subString(0, cpos).c_str()).Width;		// pixels from text-start
		s32 cend = cstart + cursor_width;
		s32 txt_width = font->getDimension(txt_line->c_str()).Width;

		if (txt_width < m_frame_rect.getWidth()) {
			// TODO: Needs a clean left and right gap removal depending on HAlign, similar to vertical scrolling tests for top/bottom.
			// This check just fixes the case where it was most noticable (text smaller than clipping area).

			m_hscroll_pos = 0;
			setTextRect(curs_line);
		}

		if (m_current_text_rect.UpperLeftCorner.X + cstart < m_frame_rect.UpperLeftCorner.X) {
			// cursor to the left of the clipping area
			m_hscroll_pos -= m_frame_rect.UpperLeftCorner.X - (m_current_text_rect.UpperLeftCorner.X + cstart);
			setTextRect(curs_line);

			// TODO: should show more characters to the left when we're scrolling left
			//	and the cursor reaches the border.
		} else if (m_current_text_rect.UpperLeftCorner.X + cend > m_frame_rect.LowerRightCorner.X)	{
			// cursor to the right of the clipping area
			m_hscroll_pos += (m_current_text_rect.UpperLeftCorner.X + cend) - m_frame_rect.LowerRightCorner.X;
			setTextRect(curs_line);
		}
	}

	// calculate vertical scrolling
	if (has_broken_text) {
		irr::u32 line_height = font->getDimension(L"A").Height + font->getKerningHeight();
		// only up to 1 line fits?
		if (line_height >= (irr::u32)m_frame_rect.getHeight()) {
			m_vscroll_pos = 0;
			setTextRect(curs_line);
			s32 unscrolledPos = m_current_text_rect.UpperLeftCorner.Y;
			s32 pivot = m_frame_rect.UpperLeftCorner.Y;
			switch (m_valign) {
			case EGUIA_CENTER:
				pivot += m_frame_rect.getHeight() / 2;
				unscrolledPos += line_height / 2;
				break;
			case EGUIA_LOWERRIGHT:
				pivot += m_frame_rect.getHeight();
				unscrolledPos += line_height;
				break;
			default:
				break;
			}
			m_vscroll_pos = unscrolledPos - pivot;
			setTextRect(curs_line);
		} else {
			// First 2 checks are necessary when people delete lines
			setTextRect(0);
			if (m_current_text_rect.UpperLeftCorner.Y > m_frame_rect.UpperLeftCorner.Y && m_valign != EGUIA_LOWERRIGHT) {
				// first line is leaving a gap on top
				m_vscroll_pos = 0;
			} else if (m_valign != EGUIA_UPPERLEFT) {
				u32 lastLine = m_broken_text_positions.empty() ? 0 : m_broken_text_positions.size() - 1;
				setTextRect(lastLine);
				if (m_current_text_rect.LowerRightCorner.Y < m_frame_rect.LowerRightCorner.Y)
				{
					// last line is leaving a gap on bottom
					m_vscroll_pos -= m_frame_rect.LowerRightCorner.Y - m_current_text_rect.LowerRightCorner.Y;
				}
			}

			setTextRect(curs_line);
			if (m_current_text_rect.UpperLeftCorner.Y < m_frame_rect.UpperLeftCorner.Y) {
				// text above valid area
				m_vscroll_pos -= m_frame_rect.UpperLeftCorner.Y - m_current_text_rect.UpperLeftCorner.Y;
				setTextRect(curs_line);
			} else if (m_current_text_rect.LowerRightCorner.Y > m_frame_rect.LowerRightCorner.Y){
				// text below valid area
				m_vscroll_pos += m_current_text_rect.LowerRightCorner.Y - m_frame_rect.LowerRightCorner.Y;
				setTextRect(curs_line);
			}
		}
	}

	if (m_vscrollbar) {
		m_vscrollbar->setPos(m_vscroll_pos);
	}
}

void GUIEditBoxWithScrollBar::calculateFrameRect()
{
	m_frame_rect = AbsoluteRect;


	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();
	if (m_border && skin) {
		m_frame_rect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X) + 1;
		m_frame_rect.UpperLeftCorner.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y) + 1;
		m_frame_rect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X) + 1;
		m_frame_rect.LowerRightCorner.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y) + 1;
	}

	updateVScrollBar();
}

//! set text markers
void GUIEditBoxWithScrollBar::setTextMarkers(s32 begin, s32 end)
{
	if (begin != m_mark_begin || end != m_mark_end) {
		m_mark_begin = begin;
		m_mark_end = end;
		sendGuiEvent(EGET_EDITBOX_MARKING_CHANGED);
	}
}

//! send some gui event to parent
void GUIEditBoxWithScrollBar::sendGuiEvent(EGUI_EVENT_TYPE type)
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

//! create a vertical scroll bar
void GUIEditBoxWithScrollBar::createVScrollBar()
{
	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();

	m_scrollbar_width = skin ? skin->getSize(gui::EGDS_SCROLLBAR_SIZE) : 16;

	RelativeRect.LowerRightCorner.X -= m_scrollbar_width + 4;

	irr::core::rect<s32> scrollbarrect = m_frame_rect;
	scrollbarrect.UpperLeftCorner.X += m_frame_rect.getWidth() - m_scrollbar_width;
	m_vscrollbar = new GUIScrollBar(Environment, getParent(), -1,
			scrollbarrect, false, true);

	m_vscrollbar->setVisible(false);
	m_vscrollbar->setSmallStep(1);
	m_vscrollbar->setLargeStep(1);
}

void GUIEditBoxWithScrollBar::updateVScrollBar()
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
	if (getTextDimension().Height > (u32) m_frame_rect.getHeight()) {
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
		if (m_vscrollbar->isVisible())
		{
			m_vscrollbar->setVisible(false);
			m_vscroll_pos = 0;
			m_vscrollbar->setPos(0);
			m_vscrollbar->setMax(1);
			m_vscrollbar->setPageSize(s32(getTextDimension().Height));
		}
	}

}

//! set true if this editbox is writable
void GUIEditBoxWithScrollBar::setWritable(bool writable)
{
	m_writable = writable;
}

//! Change the background color
void GUIEditBoxWithScrollBar::setBackgroundColor(const video::SColor &bg_color)
{
	m_bg_color = bg_color;
	m_bg_color_used = true;
}

//! Writes attributes of the element.
void GUIEditBoxWithScrollBar::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options = 0) const
{
	// IGUIEditBox::serializeAttributes(out,options);

	out->addBool("Border", m_border);
	out->addBool("Background", m_background);
	out->addBool("OverrideColorEnabled", m_override_color_enabled);
	out->addColor("OverrideColor", m_override_color);
	// out->addFont("OverrideFont", OverrideFont);
	out->addInt("MaxChars", m_max);
	out->addBool("WordWrap", m_word_wrap);
	out->addBool("MultiLine", m_multiline);
	out->addBool("AutoScroll", m_autoscroll);
	out->addBool("PasswordBox", m_passwordbox);
	core::stringw ch = L" ";
	ch[0] = m_passwordchar;
	out->addString("PasswordChar", ch.c_str());
	out->addEnum("HTextAlign", m_halign, GUIAlignmentNames);
	out->addEnum("VTextAlign", m_valign, GUIAlignmentNames);
	out->addBool("Writable", m_writable);

	IGUIEditBox::serializeAttributes(out, options);
}


//! Reads attributes of the element
void GUIEditBoxWithScrollBar::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options = 0)
{
	IGUIEditBox::deserializeAttributes(in, options);

	setDrawBorder(in->getAttributeAsBool("Border"));
	setDrawBackground(in->getAttributeAsBool("Background"));
	setOverrideColor(in->getAttributeAsColor("OverrideColor"));
	enableOverrideColor(in->getAttributeAsBool("OverrideColorEnabled"));
	setMax(in->getAttributeAsInt("MaxChars"));
	setWordWrap(in->getAttributeAsBool("WordWrap"));
	setMultiLine(in->getAttributeAsBool("MultiLine"));
	setAutoScroll(in->getAttributeAsBool("AutoScroll"));
	core::stringw ch = in->getAttributeAsStringW("PasswordChar");

	if (!ch.size())
		setPasswordBox(in->getAttributeAsBool("PasswordBox"));
	else
		setPasswordBox(in->getAttributeAsBool("PasswordBox"), ch[0]);

	setTextAlignment((EGUI_ALIGNMENT)in->getAttributeAsEnumeration("HTextAlign", GUIAlignmentNames),
		(EGUI_ALIGNMENT)in->getAttributeAsEnumeration("VTextAlign", GUIAlignmentNames));

	// setOverrideFont(in->getAttributeAsFont("OverrideFont"));
	setWritable(in->getAttributeAsBool("Writable"));
}

bool GUIEditBoxWithScrollBar::isDrawBackgroundEnabled() const { return false; }
bool GUIEditBoxWithScrollBar::isDrawBorderEnabled() const { return false; }
void GUIEditBoxWithScrollBar::setCursorChar(const wchar_t cursorChar) { }
wchar_t GUIEditBoxWithScrollBar::getCursorChar() const { return '|'; }
void GUIEditBoxWithScrollBar::setCursorBlinkTime(irr::u32 timeMs) { }
irr::u32 GUIEditBoxWithScrollBar::getCursorBlinkTime() const { return 500; }
