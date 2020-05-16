// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_WINDOW_H_INCLUDED__
#define __I_GUI_WINDOW_H_INCLUDED__

#include "IGUIElement.h"
#include "EMessageBoxFlags.h"

namespace irr
{
namespace gui
{
	class IGUIButton;

	//! Default moveable window GUI element with border, caption and close icons.
	/** \par This element can create the following events of type EGUI_EVENT_TYPE:
	\li EGET_ELEMENT_CLOSED
	*/
	class IGUIWindow : public IGUIElement
	{
	public:

		//! constructor
		IGUIWindow(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_WINDOW, environment, parent, id, rectangle) {}

		//! Returns pointer to the close button
		/** You can hide the button by calling setVisible(false) on the result. */
		virtual IGUIButton* getCloseButton() const = 0;

		//! Returns pointer to the minimize button
		/** You can hide the button by calling setVisible(false) on the result. */
		virtual IGUIButton* getMinimizeButton() const = 0;

		//! Returns pointer to the maximize button
		/** You can hide the button by calling setVisible(false) on the result. */
		virtual IGUIButton* getMaximizeButton() const = 0;

		//! Returns true if the window can be dragged with the mouse, false if not
		virtual bool isDraggable() const = 0;

		//! Sets whether the window can be dragged by the mouse
		virtual void setDraggable(bool draggable) = 0;

		//! Set if the window background will be drawn
		virtual void setDrawBackground(bool draw) = 0;

		//! Get if the window background will be drawn
		virtual bool getDrawBackground() const = 0;

		//! Set if the window titlebar will be drawn
		//! Note: If the background is not drawn, then the titlebar is automatically also not drawn
		virtual void setDrawTitlebar(bool draw) = 0;

		//! Get if the window titlebar will be drawn
		virtual bool getDrawTitlebar() const = 0;

		//! Returns the rectangle of the drawable area (without border and without titlebar)
		/** The coordinates are given relative to the top-left position of the gui element.<br>
		So to get absolute positions you have to add the resulting rectangle to getAbsolutePosition().UpperLeftCorner.<br>
		To get it relative to the parent element you have to add the resulting rectangle to getRelativePosition().UpperLeftCorner.
		Beware that adding a menu will not change the clientRect as menus are own gui elements, so in that case you might want to subtract
		the menu area additionally.	*/
		virtual core::rect<s32> getClientRect() const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

