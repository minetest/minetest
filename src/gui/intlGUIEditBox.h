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
		virtual ~intlGUIEditBox() {}

		//! Sets whether to draw the background
		virtual void setDrawBackground(bool draw);

		virtual bool isDrawBackgroundEnabled() const { return true; }

		//! draws the element and its children
		virtual void draw();

		//! Updates the absolute position, splits text if required
		virtual void updateAbsolutePosition();

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
		virtual void setTextRect(s32 line);
		//! adds a letter to the edit box
		virtual void inputChar(wchar_t c);
		//! calculates the current scroll position
		void calculateScrollPos();

		s32 getCursorPos(s32 x, s32 y);

		//! Create a vertical scrollbar
		void createVScrollBar();
	};


} // end namespace gui
} // end namespace irr

//#endif // _IRR_COMPILE_WITH_GUI_
