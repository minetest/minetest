// Copyright (C) 2002-2013 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "IrrCompileConfig.h"
//#ifdef _IRR_COMPILE_WITH_GUI_

#include "guiEditBox.h"
#include "irrArray.h"
#include "IOSOperator.h"

namespace irr
{
namespace gui
{
	class intlGUIEditBox : public GUIEditBox
	{
	public:

		//! constructor
		intlGUIEditBox(const wchar_t* text, bool border, IGUIEnvironment* environment,
			IGUIElement* parent, s32 id, const core::rect<s32>& rectangle,
			bool writable = true, bool has_vscrollbar = false);

		//! destructor
		virtual ~intlGUIEditBox();

		//! Sets whether to draw the background
		virtual void setDrawBackground(bool draw);

		virtual bool isDrawBackgroundEnabled() const { return true; }

		//! Turns the border on or off
		virtual void setDrawBorder(bool border);

		virtual bool isDrawBorderEnabled() const { return Border; }

		//! Gets the size area of the text in the edit box
		//! \return Returns the size in pixels of the text
		virtual core::dimension2du getTextDimension();

		//! Sets text justification
		virtual void setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical);

		//! called if an event happened.
		virtual bool OnEvent(const SEvent& event);

		//! draws the element and its children
		virtual void draw();

		//! Sets the new caption of this element.
		virtual void setText(const wchar_t* text);

		//! Sets the maximum amount of characters which may be entered in the box.
		//! \param max: Maximum amount of characters. If 0, the character amount is
		//! infinity.
		virtual void setMax(u32 max);

		//! Returns maximum amount of characters, previously set by setMax();
		virtual u32 getMax() const;

		//! Sets whether the edit box is a password box. Setting this to true will
		/** disable MultiLine, WordWrap and the ability to copy with ctrl+c or ctrl+x
		\param passwordBox: true to enable password, false to disable
		\param passwordChar: the character that is displayed instead of letters */
		virtual void setPasswordBox(bool passwordBox, wchar_t passwordChar = L'*');

		//! Returns true if the edit box is currently a password box.
		virtual bool isPasswordBox() const;

		//! Updates the absolute position, splits text if required
		virtual void updateAbsolutePosition();

		//! set true if this EditBox is writable
		virtual void setWritable(bool can_write_text);

		//! Writes attributes of the element.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		//! Reads attributes of the element
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

		virtual void setCursorChar(const wchar_t cursorChar) {}

		virtual wchar_t getCursorChar() const { return L'|'; }

		virtual void setCursorBlinkTime(u32 timeMs) {}

		virtual u32 getCursorBlinkTime() const { return 500; }

	protected:
		//! Breaks the single text line.
		virtual void breakText();
		//! sets the area of the given line
		void setTextRect(s32 line);
		//! returns the line number that the cursor is on
		s32 getLineFromPos(s32 pos);
		//! adds a letter to the edit box
		void inputChar(wchar_t c);
		//! calculates the current scroll position
		void calculateScrollPos();
		//! send some gui event to parent
		void sendGuiEvent(EGUI_EVENT_TYPE type);
		//! set text markers
		void setTextMarkers(s32 begin, s32 end);

		bool processKey(const SEvent& event);
		bool processMouse(const SEvent& event);
		s32 getCursorPos(s32 x, s32 y);

		//! Create a vertical scrollbar
		void createVScrollBar();

		//! Update the vertical scrollbar (visibilty & scroll position)
		void updateVScrollBar();

		bool MouseMarking = false;
		bool Border;
		s32 MarkBegin = 0;
		s32 MarkEnd = 0;

		gui::IGUIFont *LastBreakFont = nullptr;
		IOSOperator *Operator = nullptr;

		u64 BlinkStartTime = 0;
		s32 CursorPos = 0;
		s32 HScrollPos = 0;
		s32 VScrollPos = 0; // scroll position in characters
		u32 Max = 0;

		bool PasswordBox = false;
		wchar_t PasswordChar = L'*';
		EGUI_ALIGNMENT HAlign = EGUIA_UPPERLEFT;
		EGUI_ALIGNMENT VAlign = EGUIA_CENTER;

		core::array<core::stringw> BrokenText;
		core::array<s32> BrokenTextPositions;

		core::rect<s32> CurrentTextRect = core::rect<s32>(0,0,1,1);
		core::rect<s32> FrameRect; // temporary values
		u32 m_scrollbar_width;
		GUIScrollBar *m_vscrollbar;
		bool m_writable;

	};


} // end namespace gui
} // end namespace irr

//#endif // _IRR_COMPILE_WITH_GUI_
