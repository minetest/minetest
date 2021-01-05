// 11.11.2011 11:11 ValkaTR
//
// This is a copy of intlGUIEditBox from the irrlicht, but with a
// fix in the OnEvent function, which doesn't allowed input of
// other keyboard layouts than latin-1
//
// Characters like: ä ö ü õ ы й ю я ъ № € ° ...
//
// This fix is only needed for linux, because of a bug
// in the CIrrDeviceLinux.cpp:1014-1015 of the irrlicht
//
// Also locale in the programm should not be changed to
// a "C", "POSIX" or whatever, it should be set to "",
// or XLookupString will return nothing for the international
// characters.
//
// From the "man setlocale":
//
// On startup of the main program, the portable "C" locale
// is selected as default.  A  program  may  be  made
// portable to all locales by calling:
//
//           setlocale(LC_ALL, "");
//
//       after  program initialization....
//

// Copyright (C) 2002-2013 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include <util/numeric.h>
#include "intlGUIEditBox.h"

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUIFont.h"
#include "IVideoDriver.h"
//#include "irrlicht/os.cpp"
#include "porting.h"
//#include "Keycodes.h"
#include "log.h"

/*
	todo:
	optional scrollbars
	ctrl+left/right to select word
	double click/ctrl click: word select + drag to select whole words, triple click to select line
	optional? dragging selected text
	numerical
*/

namespace irr
{
namespace gui
{

//! constructor
intlGUIEditBox::intlGUIEditBox(const wchar_t* text, bool border,
		IGUIEnvironment* environment, IGUIElement* parent, s32 id,
		const core::rect<s32>& rectangle, bool writable, bool has_vscrollbar)
	: GUIEditBox(environment, parent, id, rectangle, border, writable)
{
	#ifdef _DEBUG
	setDebugName("intlintlGUIEditBox");
	#endif

	Text = text;

	if (Environment)
		m_operator = Environment->getOSOperator();

	if (m_operator)
		m_operator->grab();

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();
	if (m_border && skin)
	{
		m_frame_rect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		m_frame_rect.UpperLeftCorner.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
		m_frame_rect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		m_frame_rect.LowerRightCorner.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
	}

	if (skin && has_vscrollbar) {
		m_scrollbar_width = skin->getSize(gui::EGDS_SCROLLBAR_SIZE);

		if (m_scrollbar_width > 0) {
			createVScrollBar();
		}
	}

	breakText();

	calculateScrollPos();
	setWritable(writable);
}

//! Sets whether to draw the background
void intlGUIEditBox::setDrawBackground(bool draw)
{
}

void intlGUIEditBox::updateAbsolutePosition()
{
    core::rect<s32> oldAbsoluteRect(AbsoluteRect);
	IGUIElement::updateAbsolutePosition();
	if ( oldAbsoluteRect != AbsoluteRect )
	{
        breakText();
	}
}


//! draws the element and its children
void intlGUIEditBox::draw()
{
	if (!IsVisible)
		return;

	const bool focus = Environment->hasFocus(this);

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	m_frame_rect = AbsoluteRect;

	// draw the border

	if (m_border)
	{
		if (m_writable) {
			skin->draw3DSunkenPane(this, skin->getColor(EGDC_WINDOW),
				false, true, m_frame_rect, &AbsoluteClippingRect);
		}

		m_frame_rect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		m_frame_rect.UpperLeftCorner.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
		m_frame_rect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		m_frame_rect.LowerRightCorner.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
	}

	updateVScrollBar();
	core::rect<s32> localClipRect = m_frame_rect;
	localClipRect.clipAgainst(AbsoluteClippingRect);

	// draw the text

	IGUIFont* font = m_override_font;
	if (!m_override_font)
		font = skin->getFont();

	s32 cursorLine = 0;
	s32 charcursorpos = 0;

	if (font)
	{
		if (m_last_break_font != font)
		{
			breakText();
		}

		// calculate cursor pos

		core::stringw *txtLine = &Text;
		s32 startPos = 0;

		core::stringw s, s2;

		// get mark position
		const bool ml = (!m_passwordbox && (m_word_wrap || m_multiline));
		const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
		const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;
		const s32 hlineStart = ml ? getLineFromPos(realmbgn) : 0;
		const s32 hlineCount = ml ? getLineFromPos(realmend) - hlineStart + 1 : 1;
		const s32 lineCount = ml ? m_broken_text.size() : 1;

		// Save the override color information.
		// Then, alter it if the edit box is disabled.
		const bool prevOver = m_override_color_enabled;
		const video::SColor prevColor = m_override_color;

		if (!Text.empty()) {
			if (!IsEnabled && !m_override_color_enabled)
			{
				m_override_color_enabled = true;
				m_override_color = skin->getColor(EGDC_GRAY_TEXT);
			}

			for (s32 i=0; i < lineCount; ++i)
			{
				setTextRect(i);

				// clipping test - don't draw anything outside the visible area
				core::rect<s32> c = localClipRect;
				c.clipAgainst(m_current_text_rect);
				if (!c.isValid())
					continue;

				// get current line
				if (m_passwordbox)
				{
					if (m_broken_text.size() != 1)
					{
						m_broken_text.clear();
						m_broken_text.emplace_back();
					}
					if (m_broken_text[0].size() != Text.size())
					{
						m_broken_text[0] = Text;
						for (u32 q = 0; q < Text.size(); ++q)
						{
							m_broken_text[0] [q] = m_passwordchar;
						}
					}
					txtLine = &m_broken_text[0];
					startPos = 0;
				}
				else
				{
					txtLine = ml ? &m_broken_text[i] : &Text;
					startPos = ml ? m_broken_text_positions[i] : 0;
				}


				// draw normal text
				font->draw(txtLine->c_str(), m_current_text_rect,
					m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &localClipRect);

				// draw mark and marked text
				if (focus && m_mark_begin != m_mark_end && i >= hlineStart && i < hlineStart + hlineCount)
				{

					s32 mbegin = 0, mend = 0;
					s32 lineStartPos = 0, lineEndPos = txtLine->size();

					if (i == hlineStart)
					{
						// highlight start is on this line
						s = txtLine->subString(0, realmbgn - startPos);
						mbegin = font->getDimension(s.c_str()).Width;

						// deal with kerning
						mbegin += font->getKerningWidth(
							&((*txtLine)[realmbgn - startPos]),
							realmbgn - startPos > 0 ? &((*txtLine)[realmbgn - startPos - 1]) : 0);

						lineStartPos = realmbgn - startPos;
					}
					if (i == hlineStart + hlineCount - 1)
					{
						// highlight end is on this line
						s2 = txtLine->subString(0, realmend - startPos);
						mend = font->getDimension(s2.c_str()).Width;
						lineEndPos = (s32)s2.size();
					}
					else
						mend = font->getDimension(txtLine->c_str()).Width;

					m_current_text_rect.UpperLeftCorner.X += mbegin;
					m_current_text_rect.LowerRightCorner.X = m_current_text_rect.UpperLeftCorner.X + mend - mbegin;

					// draw mark
					skin->draw2DRectangle(this, skin->getColor(EGDC_HIGH_LIGHT), m_current_text_rect, &localClipRect);

					// draw marked text
					s = txtLine->subString(lineStartPos, lineEndPos - lineStartPos);

					if (!s.empty())
						font->draw(s.c_str(), m_current_text_rect,
							m_override_color_enabled ? m_override_color : skin->getColor(EGDC_HIGH_LIGHT_TEXT),
							false, true, &localClipRect);

				}
			}

			// Return the override color information to its previous settings.
			m_override_color_enabled = prevOver;
			m_override_color = prevColor;
		}

		// draw cursor

		if (m_word_wrap || m_multiline)
		{
			cursorLine = getLineFromPos(m_cursor_pos);
			txtLine = &m_broken_text[cursorLine];
			startPos = m_broken_text_positions[cursorLine];
		}
		s = txtLine->subString(0,m_cursor_pos-startPos);
		charcursorpos = font->getDimension(s.c_str()).Width +
			font->getKerningWidth(L"_", m_cursor_pos-startPos > 0 ? &((*txtLine)[m_cursor_pos-startPos-1]) : 0);

		if (m_writable)	{
			if (focus && (porting::getTimeMs() - m_blink_start_time) % 700 < 350) {
				setTextRect(cursorLine);
				m_current_text_rect.UpperLeftCorner.X += charcursorpos;

				font->draw(L"_", m_current_text_rect,
					m_override_color_enabled ? m_override_color : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &localClipRect);
			}
		}
	}

	// draw children
	IGUIElement::draw();
}


s32 intlGUIEditBox::getCursorPos(s32 x, s32 y)
{
	IGUIFont* font = m_override_font;
	IGUISkin* skin = Environment->getSkin();
	if (!m_override_font)
		font = skin->getFont();

	const u32 lineCount = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;

	core::stringw *txtLine = NULL;
	s32 startPos = 0;
	u32 curr_line_idx = 0;
	x += 3;

	for (; curr_line_idx < lineCount; ++curr_line_idx) {
		setTextRect(curr_line_idx);
		if (curr_line_idx == 0 && y < m_current_text_rect.UpperLeftCorner.Y)
			y = m_current_text_rect.UpperLeftCorner.Y;
		if (curr_line_idx == lineCount - 1 && y > m_current_text_rect.LowerRightCorner.Y)
			y = m_current_text_rect.LowerRightCorner.Y;

		// is it inside this region?
		if (y >= m_current_text_rect.UpperLeftCorner.Y && y <= m_current_text_rect.LowerRightCorner.Y) {
			// we've found the clicked line
			txtLine = (m_word_wrap || m_multiline) ? &m_broken_text[curr_line_idx] : &Text;
			startPos = (m_word_wrap || m_multiline) ? m_broken_text_positions[curr_line_idx] : 0;
			break;
		}
	}

	if (x < m_current_text_rect.UpperLeftCorner.X)
		x = m_current_text_rect.UpperLeftCorner.X;
	else if (x > m_current_text_rect.LowerRightCorner.X)
		x = m_current_text_rect.LowerRightCorner.X;

	s32 idx = font->getCharacterFromPos(txtLine->c_str(), x - m_current_text_rect.UpperLeftCorner.X);
	// Special handling for last line, if we are on limits, add 1 extra shift because idx
	// will be the last char, not null char of the wstring
	if (curr_line_idx == lineCount - 1 && x == m_current_text_rect.LowerRightCorner.X)
		idx++;

	return rangelim(idx + startPos, 0, S32_MAX);
}


//! Breaks the single text line.
void intlGUIEditBox::breakText()
{
	IGUISkin* skin = Environment->getSkin();

	if ((!m_word_wrap && !m_multiline) || !skin)
		return;

	m_broken_text.clear(); // need to reallocate :/
	m_broken_text_positions.clear();

	IGUIFont* font = m_override_font;
	if (!m_override_font)
		font = skin->getFont();

	if (!font)
		return;

	m_last_break_font = font;

	core::stringw line;
	core::stringw word;
	core::stringw whitespace;
	s32 lastLineStart = 0;
	s32 size = Text.size();
	s32 length = 0;
	s32 elWidth = RelativeRect.getWidth() - m_scrollbar_width - 10;
	wchar_t c;

	for (s32 i=0; i<size; ++i)
	{
		c = Text[i];
		bool lineBreak = false;

		if (c == L'\r') // Mac or Windows breaks
		{
			lineBreak = true;
			c = ' ';
			if (Text[i+1] == L'\n') // Windows breaks
			{
				Text.erase(i+1);
				--size;
			}
		}
		else if (c == L'\n') // Unix breaks
		{
			lineBreak = true;
			c = ' ';
		}

		// don't break if we're not a multi-line edit box
		if (!m_multiline)
			lineBreak = false;

		if (c == L' ' || c == 0 || i == (size-1))
		{
			if (!word.empty()) {
				// here comes the next whitespace, look if
				// we can break the last word to the next line.
				s32 whitelgth = font->getDimension(whitespace.c_str()).Width;
				s32 worldlgth = font->getDimension(word.c_str()).Width;

				if (m_word_wrap && length + worldlgth + whitelgth > elWidth)
				{
					// break to next line
					length = worldlgth;
					m_broken_text.push_back(line);
					m_broken_text_positions.push_back(lastLineStart);
					lastLineStart = i - (s32)word.size();
					line = word;
				}
				else
				{
					// add word to line
					line += whitespace;
					line += word;
					length += whitelgth + worldlgth;
				}

				word = L"";
				whitespace = L"";
			}

			whitespace += c;

			// compute line break
			if (lineBreak)
			{
				line += whitespace;
				line += word;
				m_broken_text.push_back(line);
				m_broken_text_positions.push_back(lastLineStart);
				lastLineStart = i+1;
				line = L"";
				word = L"";
				whitespace = L"";
				length = 0;
			}
		}
		else
		{
			// yippee this is a word..
			word += c;
		}
	}

	line += whitespace;
	line += word;
	m_broken_text.push_back(line);
	m_broken_text_positions.push_back(lastLineStart);
}


void intlGUIEditBox::setTextRect(s32 line)
{
	core::dimension2du d;

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	IGUIFont* font = m_override_font ? m_override_font : skin->getFont();

	if (!font)
		return;

	// get text dimension
	const u32 lineCount = (m_word_wrap || m_multiline) ? m_broken_text.size() : 1;
	if (m_word_wrap || m_multiline)
	{
		d = font->getDimension(m_broken_text[line].c_str());
	}
	else
	{
		d = font->getDimension(Text.c_str());
		d.Height = AbsoluteRect.getHeight();
	}
	d.Height += font->getKerningHeight();

	// justification
	switch (m_halign)
	{
	case EGUIA_CENTER:
		// align to h centre
		m_current_text_rect.UpperLeftCorner.X = (m_frame_rect.getWidth()/2) - (d.Width/2);
		m_current_text_rect.LowerRightCorner.X = (m_frame_rect.getWidth()/2) + (d.Width/2);
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

	switch (m_valign)
	{
	case EGUIA_CENTER:
		// align to v centre
		m_current_text_rect.UpperLeftCorner.Y =
			(m_frame_rect.getHeight()/2) - (lineCount*d.Height)/2 + d.Height*line;
		break;
	case EGUIA_LOWERRIGHT:
		// align to bottom edge
		m_current_text_rect.UpperLeftCorner.Y =
			m_frame_rect.getHeight() - lineCount*d.Height + d.Height*line;
		break;
	default:
		// align to top edge
		m_current_text_rect.UpperLeftCorner.Y = d.Height*line;
		break;
	}

	m_current_text_rect.UpperLeftCorner.X  -= m_hscroll_pos;
	m_current_text_rect.LowerRightCorner.X -= m_hscroll_pos;
	m_current_text_rect.UpperLeftCorner.Y  -= m_vscroll_pos;
	m_current_text_rect.LowerRightCorner.Y = m_current_text_rect.UpperLeftCorner.Y + d.Height;

	m_current_text_rect += m_frame_rect.UpperLeftCorner;

}

void intlGUIEditBox::inputChar(wchar_t c)
{
	if (!isEnabled() || !m_writable)
		return;

	if (c != 0)
	{
		if (Text.size() < m_max || m_max == 0)
		{
			core::stringw s;

			if (m_mark_begin != m_mark_end)
			{
				// replace marked text
				const s32 realmbgn = m_mark_begin < m_mark_end ? m_mark_begin : m_mark_end;
				const s32 realmend = m_mark_begin < m_mark_end ? m_mark_end : m_mark_begin;

				s = Text.subString(0, realmbgn);
				s.append(c);
				s.append( Text.subString(realmend, Text.size()-realmend) );
				Text = s;
				m_cursor_pos = realmbgn+1;
			}
			else
			{
				// add new character
				s = Text.subString(0, m_cursor_pos);
				s.append(c);
				s.append( Text.subString(m_cursor_pos, Text.size()-m_cursor_pos) );
				Text = s;
				++m_cursor_pos;
			}

			m_blink_start_time = porting::getTimeMs();
			setTextMarkers(0, 0);
		}
	}
	breakText();
	sendGuiEvent(EGET_EDITBOX_CHANGED);
	calculateScrollPos();
}


void intlGUIEditBox::calculateScrollPos()
{
	if (!m_autoscroll)
		return;

	// calculate horizontal scroll position
	s32 cursLine = getLineFromPos(m_cursor_pos);
	setTextRect(cursLine);

	// don't do horizontal scrolling when wordwrap is enabled.
	if (!m_word_wrap)
	{
		// get cursor position
		IGUISkin* skin = Environment->getSkin();
		if (!skin)
			return;
		IGUIFont* font = m_override_font ? m_override_font : skin->getFont();
		if (!font)
			return;

		core::stringw *txtLine = m_multiline ? &m_broken_text[cursLine] : &Text;
		s32 cPos = m_multiline ? m_cursor_pos - m_broken_text_positions[cursLine] : m_cursor_pos;

		s32 cStart = m_current_text_rect.UpperLeftCorner.X + m_hscroll_pos +
			font->getDimension(txtLine->subString(0, cPos).c_str()).Width;

		s32 cEnd = cStart + font->getDimension(L"_ ").Width;

		if (m_frame_rect.LowerRightCorner.X < cEnd)
			m_hscroll_pos = cEnd - m_frame_rect.LowerRightCorner.X;
		else if (m_frame_rect.UpperLeftCorner.X > cStart)
			m_hscroll_pos = cStart - m_frame_rect.UpperLeftCorner.X;
		else
			m_hscroll_pos = 0;

		// todo: adjust scrollbar
	}

	if (!m_word_wrap && !m_multiline)
		return;

	// vertical scroll position
	if (m_frame_rect.LowerRightCorner.Y < m_current_text_rect.LowerRightCorner.Y)
		m_vscroll_pos += m_current_text_rect.LowerRightCorner.Y - m_frame_rect.LowerRightCorner.Y; // scrolling downwards
	else if (m_frame_rect.UpperLeftCorner.Y > m_current_text_rect.UpperLeftCorner.Y)
		m_vscroll_pos += m_current_text_rect.UpperLeftCorner.Y - m_frame_rect.UpperLeftCorner.Y; // scrolling upwards

	// todo: adjust scrollbar
	if (m_vscrollbar)
		m_vscrollbar->setPos(m_vscroll_pos);
}


//! Create a vertical scrollbar
void intlGUIEditBox::createVScrollBar()
{
	s32 fontHeight = 1;

	if (m_override_font) {
		fontHeight = m_override_font->getDimension(L"").Height;
	} else {
		if (IGUISkin* skin = Environment->getSkin()) {
			if (IGUIFont* font = skin->getFont()) {
				fontHeight = font->getDimension(L"").Height;
			}
		}
	}

	irr::core::rect<s32> scrollbarrect = m_frame_rect;
	scrollbarrect.UpperLeftCorner.X += m_frame_rect.getWidth() - m_scrollbar_width;
	m_vscrollbar = new GUIScrollBar(Environment, getParent(), -1,
			scrollbarrect, false, true);

	m_vscrollbar->setVisible(false);
	m_vscrollbar->setSmallStep(3 * fontHeight);
	m_vscrollbar->setLargeStep(10 * fontHeight);
}


//! Writes attributes of the element.
void intlGUIEditBox::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	// IGUIEditBox::serializeAttributes(out,options);

	out->addBool  ("OverrideColorEnabled", m_override_color_enabled );
	out->addColor ("OverrideColor",        m_override_color);
	// out->addFont("OverrideFont",m_override_font);
	out->addInt   ("MaxChars",             m_max);
	out->addBool  ("WordWrap",             m_word_wrap);
	out->addBool  ("MultiLine",            m_multiline);
	out->addBool  ("AutoScroll",           m_autoscroll);
	out->addBool  ("PasswordBox",          m_passwordbox);
	core::stringw ch = L" ";
	ch[0] = m_passwordchar;
	out->addString("PasswordChar",         ch.c_str());
	out->addEnum  ("HTextAlign",           m_halign, GUIAlignmentNames);
	out->addEnum  ("VTextAlign",           m_valign, GUIAlignmentNames);
	out->addBool  ("Writable",             m_writable);

	IGUIEditBox::serializeAttributes(out,options);
}


//! Reads attributes of the element
void intlGUIEditBox::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIEditBox::deserializeAttributes(in,options);

	setOverrideColor(in->getAttributeAsColor("OverrideColor"));
	enableOverrideColor(in->getAttributeAsBool("OverrideColorEnabled"));
	setMax(in->getAttributeAsInt("MaxChars"));
	setWordWrap(in->getAttributeAsBool("WordWrap"));
	setMultiLine(in->getAttributeAsBool("MultiLine"));
	setAutoScroll(in->getAttributeAsBool("AutoScroll"));
	core::stringw ch = in->getAttributeAsStringW("PasswordChar");

	if (ch.empty())
		setPasswordBox(in->getAttributeAsBool("PasswordBox"));
	else
		setPasswordBox(in->getAttributeAsBool("PasswordBox"), ch[0]);

	setTextAlignment( (EGUI_ALIGNMENT) in->getAttributeAsEnumeration("HTextAlign", GUIAlignmentNames),
			(EGUI_ALIGNMENT) in->getAttributeAsEnumeration("VTextAlign", GUIAlignmentNames));

	setWritable(in->getAttributeAsBool("Writable"));
	// setOverrideFont(in->getAttributeAsFont("OverrideFont"));
}


} // end namespace gui
} // end namespace irr
