// Copyright (C) 2002-2012 Nikolaus Gebhardt. Modified by Mustapha Tachouct
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "guiListBox.h"

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIFont.h"
#include "IGUISpriteBank.h"
#include "IGUIScrollBar.h"
#include "porting.h"

using namespace irr;
using namespace irr::gui;


//! constructor
GUIListBox::GUIListBox(IGUIEnvironment *environment, IGUIElement *parent,
	s32 id, core::rect<s32> rectangle, bool clip,
	bool draw_back, bool move_over_select)
	: IGUIListBox(environment, parent, id, rectangle), m_selected(-1),
	m_item_height(0), m_item_height_override(0),
	m_total_item_height(0), m_items_icon_width(0), m_font(0), m_icon_bank(0),
	m_scrollbar(0), m_select_time(0), m_last_key_time(0), m_selecting(false), m_draw_back(draw_back),
	m_move_over_select(move_over_select), m_autoscroll(true), m_highlight_when_not_focused(true),
	m_bg_color_used(false), m_bg_color(video::SColor(0, 0, 0, 0)),
	m_selected_item_color_used(false), m_selected_item_color(video::SColor(0, 0, 0, 0))
{
#ifdef _DEBUG
	setDebugName("GUIListBox");
#endif

	IGUISkin* skin = Environment->getSkin();
	const s32 s = skin->getSize(EGDS_SCROLLBAR_SIZE);

	core::rect<s32> scrollbar_rect(RelativeRect.getWidth() - s, 0,
		RelativeRect.getWidth(), RelativeRect.getHeight());

	m_scrollbar = Environment->addScrollBar(false, scrollbar_rect, this, -1);
	m_scrollbar->setSubElement(true);
	m_scrollbar->setTabStop(false);
	m_scrollbar->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT,
		EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	m_scrollbar->setVisible(false);
	m_scrollbar->setPos(0);

	setNotClipped(!clip);

	// this element can be tabbed to
	setTabStop(true);
	setTabOrder(-1);

	updateAbsolutePosition();
}


//! destructor
GUIListBox::~GUIListBox()
{
	if (m_scrollbar)
		m_scrollbar->drop();

	if (m_font)
		m_font->drop();

	if (m_icon_bank)
		m_icon_bank->drop();
}


//! returns amount of list items
u32 GUIListBox::getItemCount() const
{
	return m_items.size();
}


//! returns string of a list item. the may be a value from 0 to itemCount-1
const wchar_t* GUIListBox::getListItem(u32 id) const
{
	if (id >= m_items.size())
		return 0;

	return m_items[id].text.c_str();
}


//! Returns the icon of an item
s32 GUIListBox::getIcon(u32 id) const
{
	if (id >= m_items.size())
		return -1;

	return m_items[id].icon;
}


//! adds a list item, returns id of item
u32 GUIListBox::addItem(const wchar_t* text)
{
	return addItem(text, -1);
}


//! adds a list item, returns id of item
void GUIListBox::removeItem(u32 id)
{
	if (id >= m_items.size())
		return;

	if ((u32)m_selected == id)
	{
		m_selected = -1;
	}
	else if ((u32)m_selected > id)
	{
		m_selected -= 1;
		m_select_time = porting::getTimeMs();
	}

	m_items.erase(m_items.begin() + id);

	recalculateItemHeight();
}


s32 GUIListBox::getItemAt(s32 xpos, s32 ypos) const
{
	if (xpos < AbsoluteRect.UpperLeftCorner.X
		|| xpos >= AbsoluteRect.LowerRightCorner.X
		|| ypos < AbsoluteRect.UpperLeftCorner.Y
		|| ypos >= AbsoluteRect.LowerRightCorner.Y
		)
		return -1;

	if (m_item_height == 0)
		return -1;

	s32 item = ((ypos - AbsoluteRect.UpperLeftCorner.Y - 1) + m_scrollbar->getPos())
		/ m_item_height;
	if (item < 0 || item >= (s32)m_items.size())
		return -1;

	return item;
}

//! clears the list
void GUIListBox::clear()
{
	m_items.clear();
	m_items_icon_width = 0;
	m_selected = -1;

	if (m_scrollbar)
		m_scrollbar->setPos(0);

	recalculateItemHeight();
}


void GUIListBox::recalculateItemHeight()
{
	IGUISkin* skin = Environment->getSkin();

	if (m_font != skin->getFont())
	{
		if (m_font)
			m_font->drop();

		m_font = skin->getFont();
		if (0 == m_item_height_override)
			m_item_height = 0;

		if (m_font)
		{
			if (0 == m_item_height_override)
				m_item_height = m_font->getDimension(L"A").Height + 4;

			m_font->grab();
		}
	}

	m_total_item_height = m_item_height * m_items.size();
	m_scrollbar->setMax(core::max_(0, m_total_item_height - AbsoluteRect.getHeight()));
	s32 minItemHeight = m_item_height > 0 ? m_item_height : 1;
	m_scrollbar->setSmallStep(minItemHeight);
	m_scrollbar->setLargeStep(2 * minItemHeight);

	if (m_total_item_height <= AbsoluteRect.getHeight())
		m_scrollbar->setVisible(false);
	else
		m_scrollbar->setVisible(true);
}


//! returns id of selected item. returns -1 if no item is selected.
s32 GUIListBox::getSelected() const
{
	return m_selected;
}


//! sets the selected item. Set this to -1 if no item should be selected
void GUIListBox::setSelected(s32 id)
{
	if ((u32)id >= m_items.size())
		m_selected = -1;
	else
		m_selected = id;

	m_select_time = porting::getTimeMs();

	recalculateScrollPos();
}

//! sets the selected item. Set this to -1 if no item should be selected
void GUIListBox::setSelected(const wchar_t *item)
{
	s32 index = -1;

	if (item)
	{
		for (index = 0; index < (s32)m_items.size(); ++index)
		{
			if (m_items[index].text == item)
				break;
		}
	}
	setSelected(index);
}

//! called if an event happened.
bool GUIListBox::OnEvent(const SEvent& event)
{
	if (isEnabled())
	{
		switch (event.EventType)
		{
		case EET_KEY_INPUT_EVENT:
			if (event.KeyInput.PressedDown &&
				(event.KeyInput.Key == KEY_DOWN ||
					event.KeyInput.Key == KEY_UP ||
					event.KeyInput.Key == KEY_HOME ||
					event.KeyInput.Key == KEY_END ||
					event.KeyInput.Key == KEY_NEXT ||
					event.KeyInput.Key == KEY_PRIOR)) {
				s32 oldSelected = m_selected;
				switch (event.KeyInput.Key)
				{
				case KEY_DOWN:
					m_selected += 1;
					break;
				case KEY_UP:
					m_selected -= 1;
					break;
				case KEY_HOME:
					m_selected = 0;
					break;
				case KEY_END:
					m_selected = (s32)m_items.size() - 1;
					break;
				case KEY_NEXT:
					m_selected += AbsoluteRect.getHeight() / m_item_height;
					break;
				case KEY_PRIOR:
					m_selected -= AbsoluteRect.getHeight() / m_item_height;
					break;
				default:
					break;
				}
				if (m_selected >= (s32)m_items.size())
					m_selected = m_items.size() - 1;
				else
					if (m_selected < 0)
						m_selected = 0;

				recalculateScrollPos();

				// post the news

				if (oldSelected != m_selected && Parent && !m_selecting && !m_move_over_select) {
					SEvent e;
					e.EventType = EET_GUI_EVENT;
					e.GUIEvent.Caller = this;
					e.GUIEvent.Element = 0;
					e.GUIEvent.EventType = EGET_LISTBOX_CHANGED;
					Parent->OnEvent(e);
				}

				return true;
			}
			else
				if (!event.KeyInput.PressedDown &&
					(event.KeyInput.Key == KEY_RETURN
						|| event.KeyInput.Key == KEY_SPACE)) {
					if (Parent) {
						SEvent e;
						e.EventType = EET_GUI_EVENT;
						e.GUIEvent.Caller = this;
						e.GUIEvent.Element = 0;
						e.GUIEvent.EventType = EGET_LISTBOX_SELECTED_AGAIN;
						Parent->OnEvent(e);
					}
					return true;
				}
				else if (event.KeyInput.PressedDown && event.KeyInput.Char) {
					// change selection based on text as it is typed.
					u32 now = porting::getTimeMs();

					if (now - m_last_key_time < 500) {
						// add to key buffer if it isn't a key repeat
						if (!(m_key_buffer.size() == 1
							&& m_key_buffer[0] == event.KeyInput.Char)) {
							m_key_buffer += L" ";
							m_key_buffer[m_key_buffer.size() - 1] = event.KeyInput.Char;
						}
					}
					else {
						m_key_buffer = L" ";
						m_key_buffer[0] = event.KeyInput.Char;
					}
					m_last_key_time = now;

					// find the selected item, starting at the current selection
					s32 start = m_selected;
					// dont change selection if the key buffer matches the current item
					if (m_selected > -1 && m_key_buffer.size() > 1) {
						if (m_items[m_selected].text.size() >= m_key_buffer.size() &&
							m_key_buffer.equals_ignore_case(
								m_items[m_selected].text.subString(
									0, m_key_buffer.size())))
							return true;
					}

					s32 current;
					for (current = start + 1; current < (s32)m_items.size(); ++current) {
						if (m_items[current].text.size() >= m_key_buffer.size()) {
							if (m_key_buffer.equals_ignore_case(m_items[current].text.subString(0, m_key_buffer.size()))) {
								if (Parent && m_selected != current && !m_selecting && !m_move_over_select) {
									SEvent e;
									e.EventType = EET_GUI_EVENT;
									e.GUIEvent.Caller = this;
									e.GUIEvent.Element = 0;
									e.GUIEvent.EventType = EGET_LISTBOX_CHANGED;
									Parent->OnEvent(e);
								}
								setSelected(current);
								return true;
							}
						}
					}
					for (current = 0; current <= start; ++current) {
						if (m_items[current].text.size() >= m_key_buffer.size()) {
							if (m_key_buffer.equals_ignore_case(
								m_items[current].text.subString(0, m_key_buffer.size()))) {
								if (Parent && m_selected != current &&
									!m_selecting && !m_move_over_select) {
									m_selected = current;
									SEvent e;
									e.EventType = EET_GUI_EVENT;
									e.GUIEvent.Caller = this;
									e.GUIEvent.Element = 0;
									e.GUIEvent.EventType = EGET_LISTBOX_CHANGED;
									Parent->OnEvent(e);
								}
								setSelected(current);
								return true;
							}
						}
					}

					return true;
				}
				break;

		case EET_GUI_EVENT:
			switch (event.GUIEvent.EventType) {
			case gui::EGET_SCROLL_BAR_CHANGED:
				if (event.GUIEvent.Caller == m_scrollbar)
					return true;
				break;
			case gui::EGET_ELEMENT_FOCUS_LOST:
			{
				if (event.GUIEvent.Caller == this)
					m_selecting = false;
			}
			default:
				break;
			}
			break;

		case EET_MOUSE_INPUT_EVENT:
		{
			core::position2d<s32> p(event.MouseInput.X, event.MouseInput.Y);

			switch (event.MouseInput.Event) {
			case EMIE_MOUSE_WHEEL:
			{
				s32 mouse_wheel_inc = event.MouseInput.Wheel < 0 ? -1 : 1;
				m_scrollbar->setPos(m_scrollbar->getPos()
					+ mouse_wheel_inc * -m_item_height / 2);
				return true;
			}

			case EMIE_LMOUSE_PRESSED_DOWN:
			{
				m_selecting = true;
				return true;
			}

			case EMIE_LMOUSE_LEFT_UP:
			{
				m_selecting = false;

				if (isPointInside(p))
					selectNew(event.MouseInput.Y);

				return true;
			}

			case EMIE_MOUSE_MOVED:
				if (m_selecting || m_move_over_select) {
					if (isPointInside(p)) {
						selectNew(event.MouseInput.Y, true);
						return true;
					}
				}
			default:
				break;
			}
		}
		break;
		case EET_LOG_TEXT_EVENT:
		case EET_USER_EVENT:
		case EET_JOYSTICK_INPUT_EVENT:
		case irr::gui::EGUIET_FORCE_32_BIT:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}


void GUIListBox::selectNew(s32 ypos, bool onlyHover)
{
	u32 now = porting::getTimeMs(); //os::Timer::getTime();
	s32 oldSelected = m_selected;

	m_selected = getItemAt(AbsoluteRect.UpperLeftCorner.X, ypos);
	if (m_selected < 0 && !m_items.empty())
		m_selected = 0;

	recalculateScrollPos();

	gui::EGUI_EVENT_TYPE eventType = (m_selected == oldSelected &&
		now < m_select_time + 500) ?
			EGET_LISTBOX_SELECTED_AGAIN : EGET_LISTBOX_CHANGED;
	m_select_time = now;
	// post the news
	if (Parent && !onlyHover) {
		SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.Caller = this;
		event.GUIEvent.Element = 0;
		event.GUIEvent.EventType = eventType;
		Parent->OnEvent(event);
	}
}


//! Update the position and size of the listbox, and update the scrollbar
void GUIListBox::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();

	recalculateItemHeight();
}


//! draws the element and its children
void GUIListBox::draw()
{
	if (!IsVisible)
		return;

	recalculateItemHeight(); // if the font changed

	IGUISkin* skin = Environment->getSkin();

	core::rect<s32>* clipRect = 0;

	// draw background
	core::rect<s32> frameRect(AbsoluteRect);

	// draw items

	core::rect<s32> clientClip(AbsoluteRect);
	clientClip.UpperLeftCorner.Y += 1;
	clientClip.UpperLeftCorner.X += 1;
	if (m_scrollbar->isVisible())
		clientClip.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X
		- skin->getSize(EGDS_SCROLLBAR_SIZE);
	clientClip.LowerRightCorner.Y -= 1;
	clientClip.clipAgainst(AbsoluteClippingRect);

	skin->draw3DSunkenPane(this,
		m_bg_color_used ? m_bg_color : skin->getColor(EGDC_3D_HIGH_LIGHT),
		true, m_draw_back, frameRect, &AbsoluteClippingRect);

	if (clipRect)
		clientClip.clipAgainst(*clipRect);

	frameRect = AbsoluteRect;
	frameRect.UpperLeftCorner.X += 1;
	if (m_scrollbar->isVisible())
		frameRect.LowerRightCorner.X = AbsoluteRect.LowerRightCorner.X
		- skin->getSize(EGDS_SCROLLBAR_SIZE);

	frameRect.LowerRightCorner.Y = AbsoluteRect.UpperLeftCorner.Y + m_item_height;

	frameRect.UpperLeftCorner.Y -= m_scrollbar->getPos();
	frameRect.LowerRightCorner.Y -= m_scrollbar->getPos();

	bool hl = m_highlight_when_not_focused ||
		Environment->hasFocus(this) ||
		Environment->hasFocus(m_scrollbar);

	for (s32 i = 0; i < (s32)m_items.size(); ++i) {
		if (frameRect.LowerRightCorner.Y >= AbsoluteRect.UpperLeftCorner.Y &&
			frameRect.UpperLeftCorner.Y <= AbsoluteRect.LowerRightCorner.Y) {
			if (i == m_selected && hl)
				skin->draw2DRectangle(this,
					m_selected_item_color_used ?
					m_selected_item_color :
					skin->getColor(EGDC_HIGH_LIGHT),
					frameRect, &clientClip);

			core::rect<s32> text_rect = frameRect;
			text_rect.UpperLeftCorner.X += 3;

			if (m_font)	{
				if (m_icon_bank && (m_items[i].icon > -1)) {
					core::position2di iconPos = text_rect.UpperLeftCorner;
					iconPos.Y += text_rect.getHeight() / 2;
					iconPos.X += m_items_icon_width / 2;

					if (i == m_selected && hl) {
						m_icon_bank->draw2DSprite((u32)m_items[i].icon, iconPos,
							&clientClip,
							hasItemOverrideColor(i, EGUI_LBC_ICON_HIGHLIGHT) ?
							getItemOverrideColor(i, EGUI_LBC_ICON_HIGHLIGHT) :
							getItemDefaultColor(EGUI_LBC_ICON_HIGHLIGHT),
							m_select_time, porting::getTimeMs(), false, true);
					} else {
						m_icon_bank->draw2DSprite((u32)m_items[i].icon, iconPos,
							&clientClip,
							hasItemOverrideColor(i, EGUI_LBC_ICON) ?
							getItemOverrideColor(i, EGUI_LBC_ICON) :
							getItemDefaultColor(EGUI_LBC_ICON),
							0, (i == m_selected) ? porting::getTimeMs() : 0, false, true);
					}
				}

				text_rect.UpperLeftCorner.X += m_items_icon_width + 3;

				if (i == m_selected && hl) {
					m_font->draw(m_items[i].text.c_str(), text_rect,
						hasItemOverrideColor(i, EGUI_LBC_TEXT_HIGHLIGHT) ?
						getItemOverrideColor(i, EGUI_LBC_TEXT_HIGHLIGHT) :
						getItemDefaultColor(EGUI_LBC_TEXT_HIGHLIGHT),
						false, true, &clientClip);
				} else {
					m_font->draw(m_items[i].text.c_str(), text_rect,
						hasItemOverrideColor(i, EGUI_LBC_TEXT) ?
						getItemOverrideColor(i, EGUI_LBC_TEXT) :
						getItemDefaultColor(EGUI_LBC_TEXT),
						false, true, &clientClip);
				}

				text_rect.UpperLeftCorner.X -= m_items_icon_width + 3;
			}
		}

		frameRect.UpperLeftCorner.Y += m_item_height;
		frameRect.LowerRightCorner.Y += m_item_height;
	}

	IGUIElement::draw();
}


//! adds an list item with an icon
u32 GUIListBox::addItem(const wchar_t* text, s32 icon)
{
	ListItem i;
	i.text = text;
	i.icon = icon;

	m_items.push_back(i);
	recalculateItemHeight();
	recalculateItemWidth(icon);

	return m_items.size() - 1;
}


void GUIListBox::setSpriteBank(IGUISpriteBank* bank)
{
	if (bank == m_icon_bank)
		return;
	if (m_icon_bank)
		m_icon_bank->drop();

	m_icon_bank = bank;
	if (m_icon_bank)
		m_icon_bank->grab();
}


void GUIListBox::recalculateScrollPos()
{
	if (!m_autoscroll)
		return;

	s32 height = m_selected == -1 ? m_total_item_height : m_selected * m_item_height;
	const s32 selPos = height - m_scrollbar->getPos();

	if (selPos < 0) {
		m_scrollbar->setPos(m_scrollbar->getPos() + selPos);
	} else if (selPos > AbsoluteRect.getHeight() - m_item_height){
		m_scrollbar->setPos(m_scrollbar->getPos() +
			selPos - AbsoluteRect.getHeight() + m_item_height);
	}
}


void GUIListBox::setAutoScrollEnabled(bool scroll)
{
	m_autoscroll = scroll;
}


bool GUIListBox::isAutoScrollEnabled() const
{
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return m_autoscroll;
}


bool GUIListBox::getSerializationLabels(EGUI_LISTBOX_COLOR colorType, core::stringc & useColorLabel, core::stringc & colorLabel) const
{
	switch (colorType)
	{
	case EGUI_LBC_TEXT:
		useColorLabel = "UseColText";
		colorLabel = "ColText";
		break;
	case EGUI_LBC_TEXT_HIGHLIGHT:
		useColorLabel = "UseColTextHl";
		colorLabel = "ColTextHl";
		break;
	case EGUI_LBC_ICON:
		useColorLabel = "UseColIcon";
		colorLabel = "ColIcon";
		break;
	case EGUI_LBC_ICON_HIGHLIGHT:
		useColorLabel = "UseColIconHl";
		colorLabel = "ColIconHl";
		break;
	default:
		return false;
	}
	return true;
}


//! Writes attributes of the element.
void GUIListBox::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options = 0) const
{
	IGUIListBox::serializeAttributes(out, options);

	// todo: out->addString	("IconBank",		IconBank->getName?);
	out->addBool("DrawBack", m_draw_back);
	out->addBool("MoveOverSelect", m_move_over_select);
	out->addBool("AutoScroll", m_autoscroll);

	out->addInt("ItemCount", m_items.size());
	for (u32 i = 0; i < m_items.size(); ++i)
	{
		core::stringc label("text");
		label += i;
		out->addString(label.c_str(), m_items[i].text.c_str());

		for (s32 c = 0; c < (s32)EGUI_LBC_COUNT; ++c)
		{
			core::stringc useColorLabel, colorLabel;
			if (!getSerializationLabels((EGUI_LISTBOX_COLOR)c, useColorLabel, colorLabel))
				return;
			label = useColorLabel; label += i;
			if (m_items[i].override_colors[c].use)
			{
				out->addBool(label.c_str(), true);
				label = colorLabel; label += i;
				out->addColor(label.c_str(), m_items[i].override_colors[c].color);
			}
			else
			{
				out->addBool(label.c_str(), false);
			}
		}
	}
}


//! Reads attributes of the element
void GUIListBox::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options = 0)
{
	clear();

	m_draw_back = in->getAttributeAsBool("DrawBack");
	m_move_over_select = in->getAttributeAsBool("MoveOverSelect");
	m_autoscroll = in->getAttributeAsBool("AutoScroll");

	IGUIListBox::deserializeAttributes(in, options);

	const s32 count = in->getAttributeAsInt("ItemCount");
	for (s32 i = 0; i < count; ++i)
	{
		core::stringc label("text");
		ListItem item;

		label += i;
		item.text = in->getAttributeAsStringW(label.c_str());

		addItem(item.text.c_str(), item.icon);

		for (u32 c = 0; c < EGUI_LBC_COUNT; ++c)
		{
			core::stringc useColorLabel, colorLabel;
			if (!getSerializationLabels((EGUI_LISTBOX_COLOR)c, useColorLabel, colorLabel))
				return;
			label = useColorLabel; label += i;
			m_items[i].override_colors[c].use = in->getAttributeAsBool(label.c_str());
			if (m_items[i].override_colors[c].use)
			{
				label = colorLabel; label += i;
				m_items[i].override_colors[c].color = in->getAttributeAsColor(label.c_str());
			}
		}
	}
}


void GUIListBox::recalculateItemWidth(s32 icon)
{
	if (m_icon_bank && icon > -1 &&
		m_icon_bank->getSprites().size() > (u32)icon &&
		m_icon_bank->getSprites()[(u32)icon].Frames.size())
	{
		u32 rno = m_icon_bank->getSprites()[(u32)icon].Frames[0].rectNumber;
		if (m_icon_bank->getPositions().size() > rno)
		{
			const s32 w = m_icon_bank->getPositions()[rno].getWidth();
			if (w > m_items_icon_width)
				m_items_icon_width = w;
		}
	}
}


void GUIListBox::setItem(u32 index, const wchar_t* text, s32 icon)
{
	if (index >= m_items.size())
		return;

	m_items[index].text = text;
	m_items[index].icon = icon;

	recalculateItemHeight();
	recalculateItemWidth(icon);
}


//! Insert the item at the given index
//! Return the index on success or -1 on failure.
s32 GUIListBox::insertItem(u32 index, const wchar_t* text, s32 icon)
{
	ListItem i;
	i.text = text;
	i.icon = icon;

	m_items.insert(m_items.begin() + index, i);
	recalculateItemHeight();
	recalculateItemWidth(icon);

	return index;
}


void GUIListBox::swapItems(u32 index1, u32 index2)
{
	if (index1 >= m_items.size() || index2 >= m_items.size())
		return;

	ListItem dummmy = m_items[index1];
	m_items[index1] = m_items[index2];
	m_items[index2] = dummmy;
}


#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8
void GUIListBox::setItemOverrideColor(u32 index, const video::SColor &color)
#else
void GUIListBox::setItemOverrideColor(u32 index, video::SColor color)
#endif
{
	for (u32 c = 0; c < EGUI_LBC_COUNT; ++c)
	{
		m_items[index].override_colors[c].use = true;
		m_items[index].override_colors[c].color = color;
	}
}


#if IRRLICHT_VERSION_MAJOR == 1 && IRRLICHT_VERSION_MINOR < 8
void GUIListBox::setItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType, const video::SColor &color)
#else
void GUIListBox::setItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType, video::SColor color)
#endif
{
	if (index >= m_items.size() || colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return;

	m_items[index].override_colors[colorType].use = true;
	m_items[index].override_colors[colorType].color = color;
}


void GUIListBox::clearItemOverrideColor(u32 index)
{
	for (u32 c = 0; c < (u32)EGUI_LBC_COUNT; ++c)
	{
		m_items[index].override_colors[c].use = false;
	}
}


void GUIListBox::clearItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType)
{
	if (index >= m_items.size() || colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return;

	m_items[index].override_colors[colorType].use = false;
}


bool GUIListBox::hasItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType) const
{
	if (index >= m_items.size() || colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return false;

	return m_items[index].override_colors[colorType].use;
}


video::SColor GUIListBox::getItemOverrideColor(u32 index, EGUI_LISTBOX_COLOR colorType) const
{
	if ((u32)index >= m_items.size() || colorType < 0 || colorType >= EGUI_LBC_COUNT)
		return video::SColor();

	return m_items[index].override_colors[colorType].color;
}


video::SColor GUIListBox::getItemDefaultColor(EGUI_LISTBOX_COLOR colorType) const
{
	IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return video::SColor();

	switch (colorType)
	{
	case EGUI_LBC_TEXT:
		return skin->getColor(EGDC_BUTTON_TEXT);
	case EGUI_LBC_TEXT_HIGHLIGHT:
		return skin->getColor(EGDC_HIGH_LIGHT_TEXT);
	case EGUI_LBC_ICON:
		return skin->getColor(EGDC_ICON);
	case EGUI_LBC_ICON_HIGHLIGHT:
		return skin->getColor(EGDC_ICON_HIGH_LIGHT);
	default:
		return video::SColor();
	}
}

//! set global itemHeight
void GUIListBox::setItemHeight(s32 height)
{
	m_item_height = height;
	m_item_height_override = 1;
}


//! Sets whether to draw the background
void GUIListBox::setDrawBackground(bool draw)
{
	m_draw_back = draw;
}


//! Change the selected item color
void GUIListBox::setSelectedItemColor(const video::SColor &color)
{
	m_selected_item_color_used = true;
	m_selected_item_color = color;
}


//! Change the background color
void GUIListBox::setBackgroundColor(const video::SColor &color)
{
	m_bg_color_used = true;
	m_bg_color = color;
}
