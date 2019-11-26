// Copyright (C) 2002-2012 Nikolaus Gebhardt, Modified by Mustapha Tachouct
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef GUIEDITBOXWITHSCROLLBAR_HEADER
#define GUIEDITBOXWITHSCROLLBAR_HEADER

#include "IGUIEditBox.h"
#include "IOSOperator.h"
#include "guiScrollBar.h"
#include <vector>

using namespace irr;
using namespace irr::gui;

class GUIEditBoxWithScrollBar : public IGUIEditBox
{
public:

	//! constructor
	GUIEditBoxWithScrollBar(const wchar_t* text, bool border, IGUIEnvironment* environment,
		IGUIElement* parent, s32 id, const core::rect<s32>& rectangle,
		bool writable = true, bool has_vscrollbar = true);

	//! destructor
	virtual ~GUIEditBoxWithScrollBar();

	//! Sets another skin independent font.
	virtual void setOverrideFont(IGUIFont* font = 0);

	//! Gets the override font (if any)
	/** \return The override font (may be 0) */
	virtual IGUIFont* getOverrideFont() const;

	//! Get the font which is used right now for drawing
	/** Currently this is the override font when one is set and the
	font of the active skin otherwise */
	virtual IGUIFont* getActiveFont() const;

	//! Sets another color for the text.
	virtual void setOverrideColor(video::SColor color);

	//! Gets the override color
	virtual video::SColor getOverrideColor() const;

	//! Sets if the text should use the overide color or the
	//! color in the gui skin.
	virtual void enableOverrideColor(bool enable);

	//! Checks if an override color is enabled
	/** \return true if the override color is enabled, false otherwise */
	virtual bool isOverrideColorEnabled(void) const;

	//! Sets whether to draw the background
	virtual void setDrawBackground(bool draw);

	//! Turns the border on or off
	virtual void setDrawBorder(bool border);

	//! Enables or disables word wrap for using the edit box as multiline text editor.
	virtual void setWordWrap(bool enable);

	//! Checks if word wrap is enabled
	//! \return true if word wrap is enabled, false otherwise
	virtual bool isWordWrapEnabled() const;

	//! Enables or disables newlines.
	/** \param enable: If set to true, the EGET_EDITBOX_ENTER event will not be fired,
	instead a newline character will be inserted. */
	virtual void setMultiLine(bool enable);

	//! Checks if multi line editing is enabled
	//! \return true if mult-line is enabled, false otherwise
	virtual bool isMultiLineEnabled() const;

	//! Enables or disables automatic scrolling with cursor position
	//! \param enable: If set to true, the text will move around with the cursor position
	virtual void setAutoScroll(bool enable);

	//! Checks to see if automatic scrolling is enabled
	//! \return true if automatic scrolling is enabled, false if not
	virtual bool isAutoScrollEnabled() const;

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

	virtual void setWritable(bool writable);

	//! Change the background color
	virtual void setBackgroundColor(const video::SColor &bg_color);

	//! Writes attributes of the element.
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

	//! Reads attributes of the element
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

	virtual bool isDrawBackgroundEnabled() const;
	virtual bool isDrawBorderEnabled() const;
	virtual void setCursorChar(const wchar_t cursorChar);
	virtual wchar_t getCursorChar() const;
	virtual void setCursorBlinkTime(irr::u32 timeMs);
	virtual irr::u32 getCursorBlinkTime() const;

protected:
	//! Breaks the single text line.
	void breakText();
	//! sets the area of the given line
	void setTextRect(s32 line);
	//! returns the line number that the cursor is on
	s32 getLineFromPos(s32 pos);
	//! adds a letter to the edit box
	void inputChar(wchar_t c);
	//! calculates the current scroll position
	void calculateScrollPos();
	//! calculated the FrameRect
	void calculateFrameRect();
	//! send some gui event to parent
	void sendGuiEvent(EGUI_EVENT_TYPE type);
	//! set text markers
	void setTextMarkers(s32 begin, s32 end);
	//! create a Vertical ScrollBar
	void createVScrollBar();
	//! update the vertical scrollBar (visibilty & position)
	void updateVScrollBar();

	bool processKey(const SEvent& event);
	bool processMouse(const SEvent& event);
	s32 getCursorPos(s32 x, s32 y);

	bool m_mouse_marking;
	bool m_border;
	bool m_background;
	bool m_override_color_enabled;
	s32 m_mark_begin;
	s32 m_mark_end;

	video::SColor m_override_color;
	gui::IGUIFont *m_override_font, *m_last_break_font;
	IOSOperator* m_operator;

	u32 m_blink_start_time;
	s32 m_cursor_pos;
	s32 m_hscroll_pos, m_vscroll_pos; // scroll position in characters
	u32 m_max;

	bool m_word_wrap, m_multiline, m_autoscroll, m_passwordbox;
	wchar_t m_passwordchar;
	EGUI_ALIGNMENT m_halign, m_valign;

	std::vector<core::stringw> m_broken_text;
	std::vector<s32> m_broken_text_positions;

	core::rect<s32> m_current_text_rect, m_frame_rect; // temporary values

	u32 m_scrollbar_width;
	GUIScrollBar *m_vscrollbar;
	bool m_writable;

	bool m_bg_color_used;
	video::SColor m_bg_color;
};


#endif // GUIEDITBOXWITHSCROLLBAR_HEADER

