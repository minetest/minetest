// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_TAB_CONTROL_H_INCLUDED__
#define __I_GUI_TAB_CONTROL_H_INCLUDED__

#include "IGUIElement.h"
#include "SColor.h"
#include "IGUISkin.h"

namespace irr
{
namespace gui
{
	//! A tab-page, onto which other gui elements could be added.
	/** IGUITab refers to the page itself, not to the tab in the tabbar of an IGUITabControl. */
	class IGUITab : public IGUIElement
	{
	public:

		//! constructor
		IGUITab(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_TAB, environment, parent, id, rectangle) {}

		//! Returns zero based index of tab if in tabcontrol.
		/** Can be accessed later IGUITabControl::getTab() by this number.
			Note that this number can change when other tabs are inserted or removed .
		*/
		virtual s32 getNumber() const = 0;

		//! sets if the tab should draw its background
		virtual void setDrawBackground(bool draw=true) = 0;

		//! sets the color of the background, if it should be drawn.
		virtual void setBackgroundColor(video::SColor c) = 0;

		//! returns true if the tab is drawing its background, false if not
		virtual bool isDrawingBackground() const = 0;

		//! returns the color of the background
		virtual video::SColor getBackgroundColor() const = 0;

		//! sets the color of the text
		virtual void setTextColor(video::SColor c) = 0;

		//! gets the color of the text
		virtual video::SColor getTextColor() const = 0;
	};

	//! A standard tab control
	/** \par This element can create the following events of type EGUI_EVENT_TYPE:
	\li EGET_TAB_CHANGED
	*/
	class IGUITabControl : public IGUIElement
	{
	public:

		//! constructor
		IGUITabControl(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_TAB_CONTROL, environment, parent, id, rectangle) {}

		//! Adds a tab
		virtual IGUITab* addTab(const wchar_t* caption, s32 id=-1) = 0;

		//! Insert the tab at the given index
		/** \return The tab on success or NULL on failure. */
		virtual IGUITab* insertTab(s32 idx, const wchar_t* caption, s32 id=-1) = 0;

		//! Removes a tab from the tabcontrol
		virtual void removeTab(s32 idx) = 0;

		//! Clears the tabcontrol removing all tabs
		virtual void clear() = 0;

		//! Returns amount of tabs in the tabcontrol
		virtual s32 getTabCount() const = 0;

		//! Returns a tab based on zero based index
		/** \param idx: zero based index of tab. Is a value betwenn 0 and getTabcount()-1;
		\return Returns pointer to the Tab. Returns 0 if no tab
		is corresponding to this tab. */
		virtual IGUITab* getTab(s32 idx) const = 0;

		//! Brings a tab to front.
		/** \param idx: number of the tab.
		\return Returns true if successful. */
		virtual bool setActiveTab(s32 idx) = 0;

		//! Brings a tab to front.
		/** \param tab: pointer to the tab.
		\return Returns true if successful. */
		virtual bool setActiveTab(IGUITab *tab) = 0;

		//! Returns which tab is currently active
		virtual s32 getActiveTab() const = 0;

		//! get the the id of the tab at the given absolute coordinates
		/** \return The id of the tab or -1 when no tab is at those coordinates*/
		virtual s32 getTabAt(s32 xpos, s32 ypos) const = 0;

		//! Set the height of the tabs
		virtual void setTabHeight( s32 height ) = 0;

		//! Get the height of the tabs
		/** return Returns the height of the tabs */
		virtual s32 getTabHeight() const = 0;

		//! set the maximal width of a tab. Per default width is 0 which means "no width restriction".
		virtual void setTabMaxWidth(s32 width ) = 0;

		//! get the maximal width of a tab
		virtual s32 getTabMaxWidth() const = 0;

		//! Set the alignment of the tabs
		/** Use EGUIA_UPPERLEFT or EGUIA_LOWERRIGHT */
		virtual void setTabVerticalAlignment( gui::EGUI_ALIGNMENT alignment ) = 0;

		//! Get the alignment of the tabs
		/** return Returns the alignment of the tabs */
		virtual gui::EGUI_ALIGNMENT getTabVerticalAlignment() const = 0;

		//! Set the extra width added to tabs on each side of the text
		virtual void setTabExtraWidth( s32 extraWidth ) = 0;

		//! Get the extra width added to tabs on each side of the text
		/** return Returns the extra width of the tabs */
		virtual s32 getTabExtraWidth() const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

