// Copyright (C) 2002-2012 Nikolaus Gebhardt, Modified by Mustapha Tachouct
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h



#include "guiComboBox.h"

#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IGUIFont.h"
#include "IGUIButton.h"
#include "guiListBox.h"

using namespace irr;
using namespace irr::gui;
using namespace irr::core;

class IGUIButton;
class IGUIListBox;

//! constructor
GUIComboBox::GUIComboBox(IGUIEnvironment *environment, IGUIElement *parent,
	s32 id, core::rect<s32> rectangle)
	: IGUIComboBox(environment, parent, id, rectangle),
	m_list_button(NULL), m_selected_text(NULL), m_listbox(NULL), m_last_focus(NULL),
	m_selected(-1), m_halign(EGUIA_UPPERLEFT), m_valign(EGUIA_CENTER),
	m_max_selection_rows(5), m_has_focus(false),
	m_bg_color_used(false), m_bg_color(video::SColor(0)),
	m_selected_item_color_used(false), m_selected_item_color(video::SColor(0))

{
#ifdef _DEBUG
	setDebugName("CGUIComboBox");
#endif

	IGUISkin* skin = Environment->getSkin();

	s32 width = 15;
	if (skin)
		width = skin->getSize(EGDS_WINDOW_BUTTON_WIDTH);

	core::rect<s32> r;
	r.UpperLeftCorner.X = rectangle.getWidth() - width - 2;
	r.LowerRightCorner.X = rectangle.getWidth() - 2;

	r.UpperLeftCorner.Y = 2;
	r.LowerRightCorner.Y = rectangle.getHeight() - 2;

	m_list_button = Environment->addButton(r, this, -1, L"");
	if (skin && skin->getSpriteBank()) {
		m_list_button->setSpriteBank(skin->getSpriteBank());
		m_list_button->setSprite(EGBS_BUTTON_UP,
			skin->getIcon(EGDI_CURSOR_DOWN),
			skin->getColor(EGDC_WINDOW_SYMBOL));
		m_list_button->setSprite(EGBS_BUTTON_DOWN,
			skin->getIcon(EGDI_CURSOR_DOWN),
			skin->getColor(EGDC_WINDOW_SYMBOL));
	}
	m_list_button->setAlignment(EGUIA_LOWERRIGHT,
		EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	m_list_button->setSubElement(true);
	m_list_button->setTabStop(false);

	r.UpperLeftCorner.X = 2;
	r.UpperLeftCorner.Y = 2;
	r.LowerRightCorner.X = RelativeRect.getWidth() -
		(m_list_button->getAbsolutePosition().getWidth() + 2);
	r.LowerRightCorner.Y = RelativeRect.getHeight() - 2;

	m_selected_text = Environment->addStaticText(L"", r, false, false,
		this, -1, false);
	m_selected_text->setSubElement(true);
	m_selected_text->setAlignment(EGUIA_UPPERLEFT,
		EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	m_selected_text->setTextAlignment(EGUIA_UPPERLEFT, EGUIA_CENTER);
	if (skin)
		m_selected_text->setOverrideColor(skin->getColor(EGDC_BUTTON_TEXT));
	m_selected_text->enableOverrideColor(true);

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);
}


void GUIComboBox::setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical)
{
	m_halign = horizontal;
	m_valign = vertical;
	m_selected_text->setTextAlignment(horizontal, vertical);
}


//! Set the maximal number of rows for the selection listbox
void GUIComboBox::setMaxSelectionRows(u32 max)
{
	m_max_selection_rows = max;

	// force recalculation of open listbox
	if (m_listbox) {
		openCloseMenu();
		openCloseMenu();
	}
}

//! Get the maximimal number of rows for the selection listbox
u32 GUIComboBox::getMaxSelectionRows() const
{
	return m_max_selection_rows;
}


//! Returns amount of items in box
u32 GUIComboBox::getItemCount() const
{
	return m_items.size();
}


//! returns string of an item. the idx may be a value from 0 to itemCount-1
const wchar_t* GUIComboBox::getItem(u32 idx) const
{
	if (idx >= m_items.size())
		return 0;

	return m_items[idx].name.c_str();
}

//! returns string of an item. the idx may be a value from 0 to itemCount-1
u32 GUIComboBox::getItemData(u32 idx) const
{
	if (idx >= m_items.size())
		return 0;

	return m_items[idx].data;
}

//! Returns index based on item data
s32 GUIComboBox::getIndexForItemData(u32 data) const
{
	for (u32 i = 0; i < m_items.size(); ++i) {
		if (m_items[i].data == data)
			return i;
	}
	return -1;
}


//! Removes an item from the combo box.
void GUIComboBox::removeItem(u32 idx)
{
	if (idx >= m_items.size())
		return;

	if (m_selected == (s32)idx)
		setSelected(-1);

	m_items.erase(m_items.begin() + idx);
}


//! Returns caption of this element.
const wchar_t* GUIComboBox::getText() const
{
	return getItem(m_selected);
}


//! adds an item and returns the index of it
u32 GUIComboBox::addItem(const wchar_t* text, u32 data)
{
	m_items.push_back(SComboData(text, data));

	if (m_selected == -1)
		setSelected(0);

	return m_items.size() - 1;
}


//! deletes all items in the combo box
void GUIComboBox::clear()
{
	m_items.clear();
	setSelected(-1);
}


//! returns id of selected item. returns -1 if no item is selected.
s32 GUIComboBox::getSelected() const
{
	return m_selected;
}


//! sets the selected item. Set this to -1 if no item should be selected
void GUIComboBox::setSelected(s32 idx)
{
	if (idx < -1 || idx >= (s32)m_items.size())
		return;

	m_selected = idx;
	if (m_selected == -1)
		m_selected_text->setText(L"");
	else
		m_selected_text->setText(m_items[m_selected].name.c_str());
}


//! called if an event happened.
bool GUIComboBox::OnEvent(const SEvent& event)
{
	if (isEnabled()) {
		switch (event.EventType) {
		case EET_KEY_INPUT_EVENT:
			if (m_listbox && event.KeyInput.PressedDown && event.KeyInput.Key == KEY_ESCAPE) {
				// hide list box
				openCloseMenu();
				return true;
			}
			else
				if (event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE) {
					if (!event.KeyInput.PressedDown) {
						openCloseMenu();
					}

					m_list_button->setPressed(m_listbox == 0);

					return true;
				}
				else if (event.KeyInput.PressedDown) {
					s32 old_selected = m_selected;
					bool absorb = true;
					switch (event.KeyInput.Key) {
					case KEY_DOWN:
						setSelected(m_selected + 1);
						break;
					case KEY_UP:
						setSelected(m_selected - 1);
						break;
					case KEY_HOME:
					case KEY_PRIOR:
						setSelected(0);
						break;
					case KEY_END:
					case KEY_NEXT:
						setSelected((s32)m_items.size() - 1);
						break;
					default:
						absorb = false;
					}

					if (m_selected < 0)
						setSelected(0);

					if (m_selected >= (s32)m_items.size())
						setSelected((s32)m_items.size() - 1);

					if (m_selected != old_selected) {
						sendSelectionChangedEvent();
						return true;
					}

					if (absorb)
						return true;
				}
				break;

		case EET_GUI_EVENT:

			switch (event.GUIEvent.EventType) {
			case EGET_ELEMENT_FOCUS_LOST:
				if (m_listbox &&
					(Environment->hasFocus(m_listbox)
					|| m_listbox->isMyChild(event.GUIEvent.Caller))	&&
					event.GUIEvent.Element != this &&
					!isMyChild(event.GUIEvent.Element) &&
					!m_listbox->isMyChild(event.GUIEvent.Element)) {
					openCloseMenu();
				}
				break;
			case EGET_BUTTON_CLICKED:
				if (event.GUIEvent.Caller == m_list_button) {
					openCloseMenu();
					return true;
				}
				break;
			case EGET_LISTBOX_SELECTED_AGAIN:
			case EGET_LISTBOX_CHANGED:
				if (event.GUIEvent.Caller == m_listbox) {
					setSelected(m_listbox->getSelected());
					if (m_selected < 0 || m_selected >= (s32)m_items.size())
						setSelected(-1);
					openCloseMenu();

					sendSelectionChangedEvent();
				}
				return true;
			default:
				break;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:

			switch (event.MouseInput.Event) {
			case EMIE_LMOUSE_PRESSED_DOWN:
			{
				core::position2d<s32> p(event.MouseInput.X, event.MouseInput.Y);

				// send to list box
				if (m_listbox && m_listbox->isPointInside(p) && m_listbox->OnEvent(event))
					return true;

				return true;
			}
			case EMIE_LMOUSE_LEFT_UP:
			{
				core::position2d<s32> p(event.MouseInput.X, event.MouseInput.Y);

				// send to list box
				if (!(m_listbox &&
					m_listbox->getAbsolutePosition().isPointInside(p) &&
					m_listbox->OnEvent(event)))
				{
					openCloseMenu();
				}
				return true;
			}
			case EMIE_MOUSE_WHEEL:
			{
				s32 old_selected = m_selected;
				s32 mouse_wheel_inc = event.MouseInput.Wheel < 0 ? 1 : -1;
				setSelected(m_selected + mouse_wheel_inc);

				if (m_selected < 0)
					setSelected(0);

				if (m_selected >= (s32)m_items.size())
					setSelected((s32)m_items.size() - 1);

				if (m_selected != old_selected) {
					sendSelectionChangedEvent();
					return true;
				}
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}


void GUIComboBox::sendSelectionChangedEvent()
{
	if (Parent) {
		SEvent event;

		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.Caller = this;
		event.GUIEvent.Element = 0;
		event.GUIEvent.EventType = EGET_COMBO_BOX_CHANGED;
		Parent->OnEvent(event);
	}
}


//! draws the element and its children
void GUIComboBox::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* skin = Environment->getSkin();
	IGUIElement *currentFocus = Environment->getFocus();
	if (currentFocus != m_last_focus) {
		m_has_focus = currentFocus == this || isMyChild(currentFocus);
		m_last_focus = currentFocus;
	}

	// set colors each time as skin-colors can be changed
	m_selected_text->setBackgroundColor(
		m_selected_item_color_used ?
		m_selected_item_color : skin->getColor(EGDC_HIGH_LIGHT));
	if (isEnabled()) {
		m_selected_text->setDrawBackground(m_has_focus);
		m_selected_text->setOverrideColor(
			skin->getColor(m_has_focus ?
				EGDC_HIGH_LIGHT_TEXT : EGDC_BUTTON_TEXT));
	}
	else {
		m_selected_text->setDrawBackground(false);
		m_selected_text->setOverrideColor(skin->getColor(EGDC_GRAY_TEXT));
	}
	
	video::SColor enabled_color = skin->getColor(EGDC_WINDOW_SYMBOL);
#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8
	video::SColor disabled_color(240, 100, 100, 100);
#else
	video::SColor disabled_color = skin->getColor(EGDC_GRAY_WINDOW_SYMBOL);
#endif


	m_list_button->setSprite(EGBS_BUTTON_UP,
		skin->getIcon(EGDI_CURSOR_DOWN),
		isEnabled() ? enabled_color : disabled_color);
	m_list_button->setSprite(EGBS_BUTTON_DOWN,
		skin->getIcon(EGDI_CURSOR_DOWN),
		isEnabled() ? enabled_color : disabled_color);


	core::rect<s32> frameRect(AbsoluteRect);

	// draw the border

	skin->draw3DSunkenPane(this,
		m_bg_color_used ? m_bg_color : skin->getColor(EGDC_3D_HIGH_LIGHT),
		true, true, frameRect, &AbsoluteClippingRect);

	// draw children
	IGUIElement::draw();
}


void GUIComboBox::openCloseMenu()
{
	if (m_listbox) {
		// close list box
		Environment->setFocus(this);
		m_listbox->remove();
		m_listbox = 0;
	}
	else {
		if (Parent)
			Parent->bringToFront(this);

		IGUISkin* skin = Environment->getSkin();
		u32 h = m_items.size();

		if (h > getMaxSelectionRows())
			h = getMaxSelectionRows();
		if (h == 0)
			h = 1;

		irr::gui::IGUIFont *font = skin->getFont();
		if (font)
			h *= (font->getDimension(L"A").Height + 4);

		// open list box
		core::rect<s32> r(0, AbsoluteRect.getHeight(),
			AbsoluteRect.getWidth(), AbsoluteRect.getHeight() + h);

		m_listbox = new GUIListBox(Environment, this, -1, r, false, true, true);

		if (m_bg_color_used) {
			m_listbox->setBackgroundColor(m_bg_color);
		}

		if (m_bg_color_used) {
			m_listbox->setSelectedItemColor(m_selected_item_color);
		}

		m_listbox->setSubElement(true);
		m_listbox->setNotClipped(true);
		//ListBox->drop();


		// ensure that list box is always completely visible
		irr::s32 y = m_listbox->getAbsolutePosition().LowerRightCorner.Y;
		irr::s32 height = Environment->getRootGUIElement()->
			getAbsolutePosition().getHeight();
		if (y > height) {
			core::rect<s32> rect(0,
				-m_listbox->getAbsolutePosition().getHeight(),
				AbsoluteRect.getWidth(), 0);

			m_listbox->setRelativePosition(rect);
		}

		for (s32 i = 0; i < (s32)m_items.size(); ++i)
			m_listbox->addItem(m_items[i].name.c_str());

		m_listbox->setSelected(m_selected);

		// set focus
		Environment->setFocus(m_listbox);
	}
}


//! Change the selected item color
void GUIComboBox::setSelectedItemColor(const video::SColor &color)
{
	m_selected_item_color_used = true;
	m_selected_item_color = color;
}


//! Change the background color
void GUIComboBox::setBackgroundColor(const video::SColor &color)
{
	m_bg_color_used = true;
	m_bg_color = color;
}


//! Writes attributes of the element.
void GUIComboBox::serializeAttributes(io::IAttributes *out, io::SAttributeReadWriteOptions *options = 0) const
{
	IGUIComboBox::serializeAttributes(out, options);

	out->addEnum("HTextAlign", m_halign, GUIAlignmentNames);
	out->addEnum("VTextAlign", m_valign, GUIAlignmentNames);
	out->addInt("MaxSelectionRows", (s32)m_max_selection_rows);

	out->addInt("Selected", m_selected);

	out->addBool("BackgroundColorUsed", m_bg_color_used);
	out->addColor("BackgroundColor", m_bg_color);

	out->addBool("SelectedItemColorUsed", m_selected_item_color_used);
	out->addColor("SelectedItemColor", m_selected_item_color);

	out->addInt("ItemCount", m_items.size());
	for (u32 i = 0; i < m_items.size(); ++i) {
		core::stringc s = "Item";
		s += i;
		s += "Text";
		out->addString(s.c_str(), m_items[i].name.c_str());
	}
}


//! Reads attributes of the element
void GUIComboBox::deserializeAttributes(io::IAttributes *in, io::SAttributeReadWriteOptions *options = 0)
{
	IGUIComboBox::deserializeAttributes(in, options);

	setTextAlignment(
		(EGUI_ALIGNMENT)in->getAttributeAsEnumeration("HTextAlign", GUIAlignmentNames),
		(EGUI_ALIGNMENT)in->getAttributeAsEnumeration("VTextAlign", GUIAlignmentNames));
	setMaxSelectionRows((u32)(in->getAttributeAsInt("MaxSelectionRows")));

	m_bg_color_used = in->getAttributeAsBool("BackgroundColorUsed");
	m_bg_color = in->getAttributeAsColor("BackgroundColor");

	m_selected_item_color_used = in->getAttributeAsBool("SelectedItemColorUsed");
	m_selected_item_color = in->getAttributeAsColor("SelectedItemColor");

	// clear the list
	clear();

	// get item count
	u32 c = in->getAttributeAsInt("ItemCount");

	// add items
	for (u32 i = 0; i < c; ++i) {
		core::stringc s = "Item";
		s += i;
		s += "Text";
		addItem(in->getAttributeAsStringW(s.c_str()).c_str(), 0);
	}

	setSelected(in->getAttributeAsInt("Selected"));
}
