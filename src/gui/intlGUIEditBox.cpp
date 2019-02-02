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
	: IGUIEditBox(environment, parent, id, rectangle),
	Border(border), FrameRect(rectangle),
	m_scrollbar_width(0), m_vscrollbar(NULL), m_writable(writable)
{
	#ifdef _DEBUG
	setDebugName("intlintlGUIEditBox");
	#endif

	Text = text;

	if (Environment)
		Operator = Environment->getOSOperator();

	if (Operator)
		Operator->grab();

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	IGUISkin *skin = 0;
	if (Environment)
		skin = Environment->getSkin();
	if (Border && skin)
	{
		FrameRect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		FrameRect.UpperLeftCorner.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
		FrameRect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		FrameRect.LowerRightCorner.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
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


//! destructor
intlGUIEditBox::~intlGUIEditBox()
{
	if (OverrideFont)
		OverrideFont->drop();

	if (Operator)
		Operator->drop();
}


//! Sets another skin independent font.
void intlGUIEditBox::setOverrideFont(IGUIFont* font)
{
	if (OverrideFont == font)
		return;

	if (OverrideFont)
		OverrideFont->drop();

	OverrideFont = font;

	if (OverrideFont)
		OverrideFont->grab();

	breakText();
}

IGUIFont * intlGUIEditBox::getOverrideFont() const
{
	return OverrideFont;
}

//! Get the font which is used right now for drawing
IGUIFont* intlGUIEditBox::getActiveFont() const
{
	if ( OverrideFont )
		return OverrideFont;
	IGUISkin* skin = Environment->getSkin();
	if (skin)
		return skin->getFont();
	return 0;
}

//! Sets another color for the text.
void intlGUIEditBox::setOverrideColor(video::SColor color)
{
	OverrideColor = color;
	OverrideColorEnabled = true;
}

video::SColor intlGUIEditBox::getOverrideColor() const
{
	return OverrideColor;
}

//! Turns the border on or off
void intlGUIEditBox::setDrawBorder(bool border)
{
	Border = border;
}

//! Sets whether to draw the background
void intlGUIEditBox::setDrawBackground(bool draw)
{
}

//! Sets if the text should use the overide color or the color in the gui skin.
void intlGUIEditBox::enableOverrideColor(bool enable)
{
	OverrideColorEnabled = enable;
}

bool intlGUIEditBox::isOverrideColorEnabled() const
{
	return OverrideColorEnabled;
}

//! Enables or disables word wrap
void intlGUIEditBox::setWordWrap(bool enable)
{
	WordWrap = enable;
	breakText();
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


//! Checks if word wrap is enabled
bool intlGUIEditBox::isWordWrapEnabled() const
{
	return WordWrap;
}


//! Enables or disables newlines.
void intlGUIEditBox::setMultiLine(bool enable)
{
	MultiLine = enable;
}


//! Checks if multi line editing is enabled
bool intlGUIEditBox::isMultiLineEnabled() const
{
	return MultiLine;
}


void intlGUIEditBox::setPasswordBox(bool passwordBox, wchar_t passwordChar)
{
	PasswordBox = passwordBox;
	if (PasswordBox)
	{
		PasswordChar = passwordChar;
		setMultiLine(false);
		setWordWrap(false);
		BrokenText.clear();
	}
}


bool intlGUIEditBox::isPasswordBox() const
{
	return PasswordBox;
}


//! Sets text justification
void intlGUIEditBox::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
	HAlign = horizontal;
	VAlign = vertical;
}


//! called if an event happened.
bool intlGUIEditBox::OnEvent(const SEvent& event)
{
	if (IsEnabled)
	{

		switch(event.EventType)
		{
		case EET_GUI_EVENT:
			if (event.GUIEvent.EventType == EGET_ELEMENT_FOCUS_LOST)
			{
				if (event.GUIEvent.Caller == this)
				{
					MouseMarking = false;
					setTextMarkers(0,0);
				}
			}
			break;
		case EET_KEY_INPUT_EVENT:
        {
#if (defined(__linux__) || defined(__FreeBSD__)) || defined(__DragonFly__)
            // ################################################################
			// ValkaTR:
            // This part is the difference from the original intlGUIEditBox
            // It converts UTF-8 character into a UCS-2 (wchar_t)
            wchar_t wc = L'_';
            mbtowc( &wc, (char *) &event.KeyInput.Char, sizeof(event.KeyInput.Char) );

            //printf( "char: %lc (%u)  \r\n", wc, wc );

            SEvent irrevent(event);
            irrevent.KeyInput.Char = wc;
            // ################################################################

			if (processKey(irrevent))
				return true;
#else
			if (processKey(event))
				return true;
#endif // defined(linux)

			break;
        }
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


bool intlGUIEditBox::processKey(const SEvent& event)
{
	if (!event.KeyInput.PressedDown)
		return false;

	bool textChanged = false;
	s32 newMarkBegin = MarkBegin;
	s32 newMarkEnd = MarkEnd;

	// control shortcut handling

	if (event.KeyInput.Control)
	{
		// german backlash '\' entered with control + '?'
		if ( event.KeyInput.Char == '\\' )
		{
			inputChar(event.KeyInput.Char);
			return true;
		}

		switch(event.KeyInput.Key)
		{
		case KEY_KEY_A:
			// select all
			newMarkBegin = 0;
			newMarkEnd = Text.size();
			break;
		case KEY_KEY_C:
			// copy to clipboard
			if (!PasswordBox && Operator && MarkBegin != MarkEnd)
			{
				const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
				const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

				core::stringc s;
				s = Text.subString(realmbgn, realmend - realmbgn).c_str();
				Operator->copyToClipboard(s.c_str());
			}
			break;
		case KEY_KEY_X:
			// cut to the clipboard
			if (!PasswordBox && Operator && MarkBegin != MarkEnd) {
				const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
				const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

				// copy
				core::stringc sc;
				sc = Text.subString(realmbgn, realmend - realmbgn).c_str();
				Operator->copyToClipboard(sc.c_str());

				if (IsEnabled && m_writable) {
					// delete
					core::stringw s;
					s = Text.subString(0, realmbgn);
					s.append( Text.subString(realmend, Text.size()-realmend) );
					Text = s;

					CursorPos = realmbgn;
					newMarkBegin = 0;
					newMarkEnd = 0;
					textChanged = true;
				}
			}
			break;
		case KEY_KEY_V:
			if (!IsEnabled || !m_writable)
				break;

			// paste from the clipboard
			if (Operator)
			{
				const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
				const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

				// add new character
				const c8* p = Operator->getTextFromClipboard();
				if (p)
				{
					if (MarkBegin == MarkEnd)
					{
						// insert text
						core::stringw s = Text.subString(0, CursorPos);
						s.append(p);
						s.append( Text.subString(CursorPos, Text.size()-CursorPos) );

						if (!Max || s.size()<=Max) // thx to Fish FH for fix
						{
							Text = s;
							s = p;
							CursorPos += s.size();
						}
					}
					else
					{
						// replace text

						core::stringw s = Text.subString(0, realmbgn);
						s.append(p);
						s.append( Text.subString(realmend, Text.size()-realmend) );

						if (!Max || s.size()<=Max)  // thx to Fish FH for fix
						{
							Text = s;
							s = p;
							CursorPos = realmbgn + s.size();
						}
					}
				}

				newMarkBegin = 0;
				newMarkEnd = 0;
				textChanged = true;
			}
			break;
		case KEY_HOME:
			// move/highlight to start of text
			if (event.KeyInput.Shift)
			{
				newMarkEnd = CursorPos;
				newMarkBegin = 0;
				CursorPos = 0;
			}
			else
			{
				CursorPos = 0;
				newMarkBegin = 0;
				newMarkEnd = 0;
			}
			break;
		case KEY_END:
			// move/highlight to end of text
			if (event.KeyInput.Shift)
			{
				newMarkBegin = CursorPos;
				newMarkEnd = Text.size();
				CursorPos = 0;
			}
			else
			{
				CursorPos = Text.size();
				newMarkBegin = 0;
				newMarkEnd = 0;
			}
			break;
		default:
			return false;
		}
	}
	// default keyboard handling
	else
	switch(event.KeyInput.Key)
	{
	case KEY_END:
		{
			s32 p = Text.size();
			if (WordWrap || MultiLine)
			{
				p = getLineFromPos(CursorPos);
				p = BrokenTextPositions[p] + (s32)BrokenText[p].size();
				if (p > 0 && (Text[p-1] == L'\r' || Text[p-1] == L'\n' ))
					p-=1;
			}

			if (event.KeyInput.Shift)
			{
				if (MarkBegin == MarkEnd)
					newMarkBegin = CursorPos;

				newMarkEnd = p;
			}
			else
			{
				newMarkBegin = 0;
				newMarkEnd = 0;
			}
			CursorPos = p;
			BlinkStartTime = porting::getTimeMs();
		}
		break;
	case KEY_HOME:
		{

			s32 p = 0;
			if (WordWrap || MultiLine)
			{
				p = getLineFromPos(CursorPos);
				p = BrokenTextPositions[p];
			}

			if (event.KeyInput.Shift)
			{
				if (MarkBegin == MarkEnd)
					newMarkBegin = CursorPos;
				newMarkEnd = p;
			}
			else
			{
				newMarkBegin = 0;
				newMarkEnd = 0;
			}
			CursorPos = p;
			BlinkStartTime = porting::getTimeMs();
		}
		break;
	case KEY_RETURN:
		if (MultiLine)
		{
			inputChar(L'\n');
			return true;
		}
		else
		{
		    sendGuiEvent( EGET_EDITBOX_ENTER );
		}
		break;
	case KEY_LEFT:

		if (event.KeyInput.Shift)
		{
			if (CursorPos > 0)
			{
				if (MarkBegin == MarkEnd)
					newMarkBegin = CursorPos;

				newMarkEnd = CursorPos-1;
			}
		}
		else
		{
			newMarkBegin = 0;
			newMarkEnd = 0;
		}

		if (CursorPos > 0) CursorPos--;
		BlinkStartTime = porting::getTimeMs();
		break;

	case KEY_RIGHT:
		if (event.KeyInput.Shift)
		{
			if (Text.size() > (u32)CursorPos)
			{
				if (MarkBegin == MarkEnd)
					newMarkBegin = CursorPos;

				newMarkEnd = CursorPos+1;
			}
		}
		else
		{
			newMarkBegin = 0;
			newMarkEnd = 0;
		}

		if (Text.size() > (u32)CursorPos) CursorPos++;
		BlinkStartTime = porting::getTimeMs();
		break;
	case KEY_UP:
		if (MultiLine || (WordWrap && BrokenText.size() > 1) )
		{
			s32 lineNo = getLineFromPos(CursorPos);
			s32 mb = (MarkBegin == MarkEnd) ? CursorPos : (MarkBegin > MarkEnd ? MarkBegin : MarkEnd);
			if (lineNo > 0)
			{
				s32 cp = CursorPos - BrokenTextPositions[lineNo];
				if ((s32)BrokenText[lineNo-1].size() < cp)
					CursorPos = BrokenTextPositions[lineNo-1] + (s32)BrokenText[lineNo-1].size()-1;
				else
					CursorPos = BrokenTextPositions[lineNo-1] + cp;
			}

			if (event.KeyInput.Shift)
			{
				newMarkBegin = mb;
				newMarkEnd = CursorPos;
			}
			else
			{
				newMarkBegin = 0;
				newMarkEnd = 0;
			}

		}
		else
		{
			return false;
		}
		break;
	case KEY_DOWN:
		if (MultiLine || (WordWrap && BrokenText.size() > 1) )
		{
			s32 lineNo = getLineFromPos(CursorPos);
			s32 mb = (MarkBegin == MarkEnd) ? CursorPos : (MarkBegin < MarkEnd ? MarkBegin : MarkEnd);
			if (lineNo < (s32)BrokenText.size()-1)
			{
				s32 cp = CursorPos - BrokenTextPositions[lineNo];
				if ((s32)BrokenText[lineNo+1].size() < cp)
					CursorPos = BrokenTextPositions[lineNo+1] + BrokenText[lineNo+1].size()-1;
				else
					CursorPos = BrokenTextPositions[lineNo+1] + cp;
			}

			if (event.KeyInput.Shift)
			{
				newMarkBegin = mb;
				newMarkEnd = CursorPos;
			}
			else
			{
				newMarkBegin = 0;
				newMarkEnd = 0;
			}

		}
		else
		{
			return false;
		}
		break;

	case KEY_BACK:
		if (!this->IsEnabled || !m_writable)
			break;

		if (!Text.empty()) {
			core::stringw s;

			if (MarkBegin != MarkEnd)
			{
				// delete marked text
				const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
				const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

				s = Text.subString(0, realmbgn);
				s.append( Text.subString(realmend, Text.size()-realmend) );
				Text = s;

				CursorPos = realmbgn;
			}
			else
			{
				// delete text behind cursor
				if (CursorPos>0)
					s = Text.subString(0, CursorPos-1);
				else
					s = L"";
				s.append( Text.subString(CursorPos, Text.size()-CursorPos) );
				Text = s;
				--CursorPos;
			}

			if (CursorPos < 0)
				CursorPos = 0;
			BlinkStartTime = porting::getTimeMs();
			newMarkBegin = 0;
			newMarkEnd = 0;
			textChanged = true;
		}
		break;
	case KEY_DELETE:
		if (!this->IsEnabled || !m_writable)
			break;

		if (!Text.empty()) {
			core::stringw s;

			if (MarkBegin != MarkEnd)
			{
				// delete marked text
				const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
				const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

				s = Text.subString(0, realmbgn);
				s.append( Text.subString(realmend, Text.size()-realmend) );
				Text = s;

				CursorPos = realmbgn;
			}
			else
			{
				// delete text before cursor
				s = Text.subString(0, CursorPos);
				s.append( Text.subString(CursorPos+1, Text.size()-CursorPos-1) );
				Text = s;
			}

			if (CursorPos > (s32)Text.size())
				CursorPos = (s32)Text.size();

			BlinkStartTime = porting::getTimeMs();
			newMarkBegin = 0;
			newMarkEnd = 0;
			textChanged = true;
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
    setTextMarkers( newMarkBegin, newMarkEnd );

	// break the text if it has changed
	if (textChanged)
	{
		breakText();
		sendGuiEvent(EGET_EDITBOX_CHANGED);
	}

	calculateScrollPos();

	return true;
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

	FrameRect = AbsoluteRect;

	// draw the border

	if (Border)
	{
		if (m_writable) {
			skin->draw3DSunkenPane(this, skin->getColor(EGDC_WINDOW),
				false, true, FrameRect, &AbsoluteClippingRect);
		}

		FrameRect.UpperLeftCorner.X += skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		FrameRect.UpperLeftCorner.Y += skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
		FrameRect.LowerRightCorner.X -= skin->getSize(EGDS_TEXT_DISTANCE_X)+1;
		FrameRect.LowerRightCorner.Y -= skin->getSize(EGDS_TEXT_DISTANCE_Y)+1;
	}

	updateVScrollBar();
	core::rect<s32> localClipRect = FrameRect;
	localClipRect.clipAgainst(AbsoluteClippingRect);

	// draw the text

	IGUIFont* font = OverrideFont;
	if (!OverrideFont)
		font = skin->getFont();

	s32 cursorLine = 0;
	s32 charcursorpos = 0;

	if (font)
	{
		if (LastBreakFont != font)
		{
			breakText();
		}

		// calculate cursor pos

		core::stringw *txtLine = &Text;
		s32 startPos = 0;

		core::stringw s, s2;

		// get mark position
		const bool ml = (!PasswordBox && (WordWrap || MultiLine));
		const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
		const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;
		const s32 hlineStart = ml ? getLineFromPos(realmbgn) : 0;
		const s32 hlineCount = ml ? getLineFromPos(realmend) - hlineStart + 1 : 1;
		const s32 lineCount = ml ? BrokenText.size() : 1;

		// Save the override color information.
		// Then, alter it if the edit box is disabled.
		const bool prevOver = OverrideColorEnabled;
		const video::SColor prevColor = OverrideColor;

		if (!Text.empty()) {
			if (!IsEnabled && !OverrideColorEnabled)
			{
				OverrideColorEnabled = true;
				OverrideColor = skin->getColor(EGDC_GRAY_TEXT);
			}

			for (s32 i=0; i < lineCount; ++i)
			{
				setTextRect(i);

				// clipping test - don't draw anything outside the visible area
				core::rect<s32> c = localClipRect;
				c.clipAgainst(CurrentTextRect);
				if (!c.isValid())
					continue;

				// get current line
				if (PasswordBox)
				{
					if (BrokenText.size() != 1)
					{
						BrokenText.clear();
						BrokenText.push_back(core::stringw());
					}
					if (BrokenText[0].size() != Text.size())
					{
						BrokenText[0] = Text;
						for (u32 q = 0; q < Text.size(); ++q)
						{
							BrokenText[0] [q] = PasswordChar;
						}
					}
					txtLine = &BrokenText[0];
					startPos = 0;
				}
				else
				{
					txtLine = ml ? &BrokenText[i] : &Text;
					startPos = ml ? BrokenTextPositions[i] : 0;
				}


				// draw normal text
				font->draw(txtLine->c_str(), CurrentTextRect,
					OverrideColorEnabled ? OverrideColor : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &localClipRect);

				// draw mark and marked text
				if (focus && MarkBegin != MarkEnd && i >= hlineStart && i < hlineStart + hlineCount)
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

					CurrentTextRect.UpperLeftCorner.X += mbegin;
					CurrentTextRect.LowerRightCorner.X = CurrentTextRect.UpperLeftCorner.X + mend - mbegin;

					// draw mark
					skin->draw2DRectangle(this, skin->getColor(EGDC_HIGH_LIGHT), CurrentTextRect, &localClipRect);

					// draw marked text
					s = txtLine->subString(lineStartPos, lineEndPos - lineStartPos);

					if (!s.empty())
						font->draw(s.c_str(), CurrentTextRect,
							OverrideColorEnabled ? OverrideColor : skin->getColor(EGDC_HIGH_LIGHT_TEXT),
							false, true, &localClipRect);

				}
			}

			// Return the override color information to its previous settings.
			OverrideColorEnabled = prevOver;
			OverrideColor = prevColor;
		}

		// draw cursor

		if (WordWrap || MultiLine)
		{
			cursorLine = getLineFromPos(CursorPos);
			txtLine = &BrokenText[cursorLine];
			startPos = BrokenTextPositions[cursorLine];
		}
		s = txtLine->subString(0,CursorPos-startPos);
		charcursorpos = font->getDimension(s.c_str()).Width +
			font->getKerningWidth(L"_", CursorPos-startPos > 0 ? &((*txtLine)[CursorPos-startPos-1]) : 0);

		if (m_writable)	{
			if (focus && (porting::getTimeMs() - BlinkStartTime) % 700 < 350) {
				setTextRect(cursorLine);
				CurrentTextRect.UpperLeftCorner.X += charcursorpos;

				font->draw(L"_", CurrentTextRect,
					OverrideColorEnabled ? OverrideColor : skin->getColor(EGDC_BUTTON_TEXT),
					false, true, &localClipRect);
			}
		}
	}

	// draw children
	IGUIElement::draw();
}


//! Sets the new caption of this element.
void intlGUIEditBox::setText(const wchar_t* text)
{
	Text = text;
	if (u32(CursorPos) > Text.size())
		CursorPos = Text.size();
	HScrollPos = 0;
	breakText();
}


//! Enables or disables automatic scrolling with cursor position
//! \param enable: If set to true, the text will move around with the cursor position
void intlGUIEditBox::setAutoScroll(bool enable)
{
	AutoScroll = enable;
}


//! Checks to see if automatic scrolling is enabled
//! \return true if automatic scrolling is enabled, false if not
bool intlGUIEditBox::isAutoScrollEnabled() const
{
	return AutoScroll;
}


//! Gets the area of the text in the edit box
//! \return Returns the size in pixels of the text
core::dimension2du intlGUIEditBox::getTextDimension()
{
	core::rect<s32> ret;

	setTextRect(0);
	ret = CurrentTextRect;

	for (u32 i=1; i < BrokenText.size(); ++i)
	{
		setTextRect(i);
		ret.addInternalPoint(CurrentTextRect.UpperLeftCorner);
		ret.addInternalPoint(CurrentTextRect.LowerRightCorner);
	}

	return core::dimension2du(ret.getSize());
}


//! Sets the maximum amount of characters which may be entered in the box.
//! \param max: Maximum amount of characters. If 0, the character amount is
//! infinity.
void intlGUIEditBox::setMax(u32 max)
{
	Max = max;

	if (Text.size() > Max && Max != 0)
		Text = Text.subString(0, Max);
}


//! Returns maximum amount of characters, previously set by setMax();
u32 intlGUIEditBox::getMax() const
{
	return Max;
}


bool intlGUIEditBox::processMouse(const SEvent& event)
{
	switch(event.MouseInput.Event)
	{
	case irr::EMIE_LMOUSE_LEFT_UP:
		if (Environment->hasFocus(this))
		{
			CursorPos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
			if (MouseMarking)
			{
			    setTextMarkers( MarkBegin, CursorPos );
			}
			MouseMarking = false;
			calculateScrollPos();
			return true;
		}
		break;
	case irr::EMIE_MOUSE_MOVED:
		{
			if (MouseMarking)
			{
				CursorPos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
				setTextMarkers( MarkBegin, CursorPos );
				calculateScrollPos();
				return true;
			}
		}
		break;
	case EMIE_LMOUSE_PRESSED_DOWN:
		if (!Environment->hasFocus(this))
		{
			BlinkStartTime = porting::getTimeMs();
			MouseMarking = true;
			CursorPos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);
			setTextMarkers(CursorPos, CursorPos );
			calculateScrollPos();
			return true;
		}
		else
		{
			if (!AbsoluteClippingRect.isPointInside(
				core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
				return false;
			}


			// move cursor
			CursorPos = getCursorPos(event.MouseInput.X, event.MouseInput.Y);

			s32 newMarkBegin = MarkBegin;
			if (!MouseMarking)
				newMarkBegin = CursorPos;

			MouseMarking = true;
			setTextMarkers( newMarkBegin, CursorPos);
			calculateScrollPos();
			return true;
		}
		break;
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


s32 intlGUIEditBox::getCursorPos(s32 x, s32 y)
{
	IGUIFont* font = OverrideFont;
	IGUISkin* skin = Environment->getSkin();
	if (!OverrideFont)
		font = skin->getFont();

	const u32 lineCount = (WordWrap || MultiLine) ? BrokenText.size() : 1;

	core::stringw *txtLine = NULL;
	s32 startPos = 0;
	u32 curr_line_idx = 0;
	x += 3;

	for (; curr_line_idx < lineCount; ++curr_line_idx) {
		setTextRect(curr_line_idx);
		if (curr_line_idx == 0 && y < CurrentTextRect.UpperLeftCorner.Y)
			y = CurrentTextRect.UpperLeftCorner.Y;
		if (curr_line_idx == lineCount - 1 && y > CurrentTextRect.LowerRightCorner.Y)
			y = CurrentTextRect.LowerRightCorner.Y;

		// is it inside this region?
		if (y >= CurrentTextRect.UpperLeftCorner.Y && y <= CurrentTextRect.LowerRightCorner.Y) {
			// we've found the clicked line
			txtLine = (WordWrap || MultiLine) ? &BrokenText[curr_line_idx] : &Text;
			startPos = (WordWrap || MultiLine) ? BrokenTextPositions[curr_line_idx] : 0;
			break;
		}
	}

	if (x < CurrentTextRect.UpperLeftCorner.X)
		x = CurrentTextRect.UpperLeftCorner.X;
	else if (x > CurrentTextRect.LowerRightCorner.X)
		x = CurrentTextRect.LowerRightCorner.X;

	s32 idx = font->getCharacterFromPos(txtLine->c_str(), x - CurrentTextRect.UpperLeftCorner.X);
	// Special handling for last line, if we are on limits, add 1 extra shift because idx
	// will be the last char, not null char of the wstring
	if (curr_line_idx == lineCount - 1 && x == CurrentTextRect.LowerRightCorner.X)
		idx++;

	return rangelim(idx + startPos, 0, S32_MAX);
}


//! Breaks the single text line.
void intlGUIEditBox::breakText()
{
	IGUISkin* skin = Environment->getSkin();

	if ((!WordWrap && !MultiLine) || !skin)
		return;

	BrokenText.clear(); // need to reallocate :/
	BrokenTextPositions.set_used(0);

	IGUIFont* font = OverrideFont;
	if (!OverrideFont)
		font = skin->getFont();

	if (!font)
		return;

	LastBreakFont = font;

	core::stringw line;
	core::stringw word;
	core::stringw whitespace;
	s32 lastLineStart = 0;
	s32 size = Text.size();
	s32 length = 0;
	s32 elWidth = RelativeRect.getWidth() - 6;
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
		if (!MultiLine)
			lineBreak = false;

		if (c == L' ' || c == 0 || i == (size-1))
		{
			if (!word.empty()) {
				// here comes the next whitespace, look if
				// we can break the last word to the next line.
				s32 whitelgth = font->getDimension(whitespace.c_str()).Width;
				s32 worldlgth = font->getDimension(word.c_str()).Width;

				if (WordWrap && length + worldlgth + whitelgth > elWidth)
				{
					// break to next line
					length = worldlgth;
					BrokenText.push_back(line);
					BrokenTextPositions.push_back(lastLineStart);
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
				BrokenText.push_back(line);
				BrokenTextPositions.push_back(lastLineStart);
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
	BrokenText.push_back(line);
	BrokenTextPositions.push_back(lastLineStart);
}


void intlGUIEditBox::setTextRect(s32 line)
{
	core::dimension2du d;

	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;

	IGUIFont* font = OverrideFont ? OverrideFont : skin->getFont();

	if (!font)
		return;

	// get text dimension
	const u32 lineCount = (WordWrap || MultiLine) ? BrokenText.size() : 1;
	if (WordWrap || MultiLine)
	{
		d = font->getDimension(BrokenText[line].c_str());
	}
	else
	{
		d = font->getDimension(Text.c_str());
		d.Height = AbsoluteRect.getHeight();
	}
	d.Height += font->getKerningHeight();

	// justification
	switch (HAlign)
	{
	case EGUIA_CENTER:
		// align to h centre
		CurrentTextRect.UpperLeftCorner.X = (FrameRect.getWidth()/2) - (d.Width/2);
		CurrentTextRect.LowerRightCorner.X = (FrameRect.getWidth()/2) + (d.Width/2);
		break;
	case EGUIA_LOWERRIGHT:
		// align to right edge
		CurrentTextRect.UpperLeftCorner.X = FrameRect.getWidth() - d.Width;
		CurrentTextRect.LowerRightCorner.X = FrameRect.getWidth();
		break;
	default:
		// align to left edge
		CurrentTextRect.UpperLeftCorner.X = 0;
		CurrentTextRect.LowerRightCorner.X = d.Width;

	}

	switch (VAlign)
	{
	case EGUIA_CENTER:
		// align to v centre
		CurrentTextRect.UpperLeftCorner.Y =
			(FrameRect.getHeight()/2) - (lineCount*d.Height)/2 + d.Height*line;
		break;
	case EGUIA_LOWERRIGHT:
		// align to bottom edge
		CurrentTextRect.UpperLeftCorner.Y =
			FrameRect.getHeight() - lineCount*d.Height + d.Height*line;
		break;
	default:
		// align to top edge
		CurrentTextRect.UpperLeftCorner.Y = d.Height*line;
		break;
	}

	CurrentTextRect.UpperLeftCorner.X  -= HScrollPos;
	CurrentTextRect.LowerRightCorner.X -= HScrollPos;
	CurrentTextRect.UpperLeftCorner.Y  -= VScrollPos;
	CurrentTextRect.LowerRightCorner.Y = CurrentTextRect.UpperLeftCorner.Y + d.Height;

	CurrentTextRect += FrameRect.UpperLeftCorner;

}


s32 intlGUIEditBox::getLineFromPos(s32 pos)
{
	if (!WordWrap && !MultiLine)
		return 0;

	s32 i=0;
	while (i < (s32)BrokenTextPositions.size())
	{
		if (BrokenTextPositions[i] > pos)
			return i-1;
		++i;
	}
	return (s32)BrokenTextPositions.size() - 1;
}


void intlGUIEditBox::inputChar(wchar_t c)
{
	if (!IsEnabled || !m_writable)
		return;

	if (c != 0)
	{
		if (Text.size() < Max || Max == 0)
		{
			core::stringw s;

			if (MarkBegin != MarkEnd)
			{
				// replace marked text
				const s32 realmbgn = MarkBegin < MarkEnd ? MarkBegin : MarkEnd;
				const s32 realmend = MarkBegin < MarkEnd ? MarkEnd : MarkBegin;

				s = Text.subString(0, realmbgn);
				s.append(c);
				s.append( Text.subString(realmend, Text.size()-realmend) );
				Text = s;
				CursorPos = realmbgn+1;
			}
			else
			{
				// add new character
				s = Text.subString(0, CursorPos);
				s.append(c);
				s.append( Text.subString(CursorPos, Text.size()-CursorPos) );
				Text = s;
				++CursorPos;
			}

			BlinkStartTime = porting::getTimeMs();
			setTextMarkers(0, 0);
		}
	}
	breakText();
	sendGuiEvent(EGET_EDITBOX_CHANGED);
	calculateScrollPos();
}


void intlGUIEditBox::calculateScrollPos()
{
	if (!AutoScroll)
		return;

	// calculate horizontal scroll position
	s32 cursLine = getLineFromPos(CursorPos);
	setTextRect(cursLine);

	// don't do horizontal scrolling when wordwrap is enabled.
	if (!WordWrap)
	{
		// get cursor position
		IGUISkin* skin = Environment->getSkin();
		if (!skin)
			return;
		IGUIFont* font = OverrideFont ? OverrideFont : skin->getFont();
		if (!font)
			return;

		core::stringw *txtLine = MultiLine ? &BrokenText[cursLine] : &Text;
		s32 cPos = MultiLine ? CursorPos - BrokenTextPositions[cursLine] : CursorPos;

		s32 cStart = CurrentTextRect.UpperLeftCorner.X + HScrollPos +
			font->getDimension(txtLine->subString(0, cPos).c_str()).Width;

		s32 cEnd = cStart + font->getDimension(L"_ ").Width;

		if (FrameRect.LowerRightCorner.X < cEnd)
			HScrollPos = cEnd - FrameRect.LowerRightCorner.X;
		else if (FrameRect.UpperLeftCorner.X > cStart)
			HScrollPos = cStart - FrameRect.UpperLeftCorner.X;
		else
			HScrollPos = 0;

		// todo: adjust scrollbar
	}

	if (!WordWrap && !MultiLine)
		return;

	// vertical scroll position
	if (FrameRect.LowerRightCorner.Y < CurrentTextRect.LowerRightCorner.Y)
		VScrollPos += CurrentTextRect.LowerRightCorner.Y - FrameRect.LowerRightCorner.Y; // scrolling downwards
	else if (FrameRect.UpperLeftCorner.Y > CurrentTextRect.UpperLeftCorner.Y)
		VScrollPos += CurrentTextRect.UpperLeftCorner.Y - FrameRect.UpperLeftCorner.Y; // scrolling upwards

	// todo: adjust scrollbar
	if (m_vscrollbar)
		m_vscrollbar->setPos(VScrollPos);
}

//! set text markers
void intlGUIEditBox::setTextMarkers(s32 begin, s32 end)
{
    if ( begin != MarkBegin || end != MarkEnd )
    {
        MarkBegin = begin;
        MarkEnd = end;
        sendGuiEvent(EGET_EDITBOX_MARKING_CHANGED);
    }
}

//! send some gui event to parent
void intlGUIEditBox::sendGuiEvent(EGUI_EVENT_TYPE type)
{
	if ( Parent )
	{
        SEvent e;
        e.EventType = EET_GUI_EVENT;
        e.GUIEvent.Caller = this;
        e.GUIEvent.Element = 0;
        e.GUIEvent.EventType = type;

        Parent->OnEvent(e);
	}
}

//! Create a vertical scrollbar
void intlGUIEditBox::createVScrollBar()
{
	s32 fontHeight = 1;

	if (OverrideFont) {
		fontHeight = OverrideFont->getDimension(L"").Height;
	} else {
		if (IGUISkin* skin = Environment->getSkin()) {
			if (IGUIFont* font = skin->getFont()) {
				fontHeight = font->getDimension(L"").Height;
			}
		}
	}

	RelativeRect.LowerRightCorner.X -= m_scrollbar_width + 4;

	irr::core::rect<s32> scrollbarrect = FrameRect;
	scrollbarrect.UpperLeftCorner.X += FrameRect.getWidth() - m_scrollbar_width;
	m_vscrollbar = Environment->addScrollBar(false, scrollbarrect, getParent(), getID());
	m_vscrollbar->setVisible(false);
	m_vscrollbar->setSmallStep(3 * fontHeight);
	m_vscrollbar->setLargeStep(10 * fontHeight);
}

//! Update the vertical scrollbar (visibilty & scroll position)
void intlGUIEditBox::updateVScrollBar()
{
	if (!m_vscrollbar)
		return;

	// OnScrollBarChanged(...)
	if (m_vscrollbar->getPos() != VScrollPos) {
		s32 deltaScrollY = m_vscrollbar->getPos() - VScrollPos;
		CurrentTextRect.UpperLeftCorner.Y -= deltaScrollY;
		CurrentTextRect.LowerRightCorner.Y -= deltaScrollY;

		s32 scrollymax = getTextDimension().Height - FrameRect.getHeight();
		if (scrollymax != m_vscrollbar->getMax()) {
			// manage a newline or a deleted line
			m_vscrollbar->setMax(scrollymax);
			calculateScrollPos();
		} else {
			// manage a newline or a deleted line
			VScrollPos = m_vscrollbar->getPos();
		}
	}

	// check if a vertical scrollbar is needed ?
	if (getTextDimension().Height > (u32) FrameRect.getHeight()) {
		s32 scrollymax = getTextDimension().Height - FrameRect.getHeight();
		if (scrollymax != m_vscrollbar->getMax()) {
			m_vscrollbar->setMax(scrollymax);
		}

		if (!m_vscrollbar->isVisible() && MultiLine) {
			AbsoluteRect.LowerRightCorner.X -= m_scrollbar_width;

			m_vscrollbar->setVisible(true);
		}
	} else {
		if (m_vscrollbar->isVisible()) {
			AbsoluteRect.LowerRightCorner.X += m_scrollbar_width;

			VScrollPos = 0;
			m_vscrollbar->setPos(0);
			m_vscrollbar->setMax(1);
			m_vscrollbar->setVisible(false);
		}
	}
}

void intlGUIEditBox::setWritable(bool can_write_text)
{
	m_writable = can_write_text;
}

//! Writes attributes of the element.
void intlGUIEditBox::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	// IGUIEditBox::serializeAttributes(out,options);

	out->addBool  ("OverrideColorEnabled",OverrideColorEnabled );
	out->addColor ("OverrideColor",       OverrideColor);
	// out->addFont("OverrideFont",OverrideFont);
	out->addInt   ("MaxChars",            Max);
	out->addBool  ("WordWrap",            WordWrap);
	out->addBool  ("MultiLine",           MultiLine);
	out->addBool  ("AutoScroll",          AutoScroll);
	out->addBool  ("PasswordBox",         PasswordBox);
	core::stringw ch = L" ";
	ch[0] = PasswordChar;
	out->addString("PasswordChar",        ch.c_str());
	out->addEnum  ("HTextAlign",          HAlign, GUIAlignmentNames);
	out->addEnum  ("VTextAlign",          VAlign, GUIAlignmentNames);
	out->addBool  ("Writable",            m_writable);

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
