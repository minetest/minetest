// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIContextMenu.h"

#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUISkin.h"
#include "IGUIEnvironment.h"
#include "IVideoDriver.h"
#include "IGUIFont.h"
#include "IGUISpriteBank.h"
#include "os.h"

namespace irr
{
namespace gui
{


//! constructor
CGUIContextMenu::CGUIContextMenu(IGUIEnvironment* environment,
				IGUIElement* parent, s32 id,
				core::rect<s32> rectangle, bool getFocus, bool allowFocus)
	: IGUIContextMenu(environment, parent, id, rectangle), EventParent(0), LastFont(0),
		CloseHandling(ECMC_REMOVE), HighLighted(-1), ChangeTime(0), AllowFocus(allowFocus)
{
	#ifdef _DEBUG
	setDebugName("CGUIContextMenu");
	#endif

	Pos = rectangle.UpperLeftCorner;
	recalculateSize();

	if (getFocus)
		Environment->setFocus(this);

	setNotClipped(true);
}


//! destructor
CGUIContextMenu::~CGUIContextMenu()
{
	for (u32 i=0; i<Items.size(); ++i)
		if (Items[i].SubMenu)
			Items[i].SubMenu->drop();

	if (LastFont)
		LastFont->drop();
}

//! set behavior when menus are closed
void CGUIContextMenu::setCloseHandling(ECONTEXT_MENU_CLOSE onClose)
{
	CloseHandling = onClose;
}

//! get current behavior when the menue will be closed
ECONTEXT_MENU_CLOSE CGUIContextMenu::getCloseHandling() const
{
	return CloseHandling;
}

//! Returns amount of menu items
u32 CGUIContextMenu::getItemCount() const
{
	return Items.size();
}


//! Adds a menu item.
u32 CGUIContextMenu::addItem(const wchar_t* text, s32 commandId, bool enabled, bool hasSubMenu, bool checked, bool autoChecking)
{
    return insertItem(Items.size(), text, commandId, enabled, hasSubMenu, checked, autoChecking);
}

//! Insert a menu item at specified position.
u32 CGUIContextMenu::insertItem(u32 idx, const wchar_t* text, s32 commandId, bool enabled,
    bool hasSubMenu, bool checked, bool autoChecking)
{
	SItem s;
	s.Enabled = enabled;
	s.Checked = checked;
	s.AutoChecking = autoChecking;
	s.Text = text;
	s.IsSeparator = (text == 0);
	s.SubMenu = 0;
	s.CommandId = commandId;

	if (hasSubMenu)
	{
		s.SubMenu = new CGUIContextMenu(Environment, this, commandId,
			core::rect<s32>(0,0,100,100), false, false);
		s.SubMenu->setVisible(false);
	}

    u32 result = idx;
    if ( idx < Items.size() )
    {
        Items.insert(s, idx);
    }
    else
    {
        Items.push_back(s);
        result = Items.size() - 1;
    }

	recalculateSize();
	return result;
}

s32 CGUIContextMenu::findItemWithCommandId(s32 commandId, u32 idxStartSearch) const
{
	for ( u32 i=idxStartSearch; i<Items.size(); ++i )
	{
		if ( Items[i].CommandId == commandId )
		{
			return (s32)i;
		}
	}
	return -1;
}

//! Adds a sub menu from an element that already exists.
void CGUIContextMenu::setSubMenu(u32 index, CGUIContextMenu* menu)
{
	if (index >= Items.size())
		return;

	if (menu)
		menu->grab();
	if (Items[index].SubMenu)
		Items[index].SubMenu->drop();

	Items[index].SubMenu = menu;
	menu->setVisible(false);

	if (Items[index].SubMenu)
	{
		menu->AllowFocus = false;
		if ( Environment->getFocus() == menu )
		{
			Environment->setFocus( this );
		}
	}

	recalculateSize();
}


//! Adds a separator item to the menu
void CGUIContextMenu::addSeparator()
{
	addItem(0, -1, true, false, false, false);
}


//! Returns text of the menu item.
const wchar_t* CGUIContextMenu::getItemText(u32 idx) const
{
	if (idx >= Items.size())
		return 0;

	return Items[idx].Text.c_str();
}


//! Sets text of the menu item.
void CGUIContextMenu::setItemText(u32 idx, const wchar_t* text)
{
	if (idx >= Items.size())
		return;

	Items[idx].Text = text;
	recalculateSize();
}

//! should the element change the checked status on clicking
void CGUIContextMenu::setItemAutoChecking(u32 idx, bool autoChecking)
{
	if ( idx >= Items.size())
		return;

	Items[idx].AutoChecking = autoChecking;
}

//! does the element change the checked status on clicking
bool CGUIContextMenu::getItemAutoChecking(u32 idx) const
{
	if (idx >= Items.size())
		return false;

	return Items[idx].AutoChecking;
}


//! Returns if a menu item is enabled
bool CGUIContextMenu::isItemEnabled(u32 idx) const
{
	if (idx >= Items.size())
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return Items[idx].Enabled;
}


//! Returns if a menu item is checked
bool CGUIContextMenu::isItemChecked(u32 idx) const
{
	if (idx >= Items.size())
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return Items[idx].Checked;
}


//! Sets if the menu item should be enabled.
void CGUIContextMenu::setItemEnabled(u32 idx, bool enabled)
{
	if (idx >= Items.size())
		return;

	Items[idx].Enabled = enabled;
}


//! Sets if the menu item should be checked.
void CGUIContextMenu::setItemChecked(u32 idx, bool checked )
{
	if (idx >= Items.size())
		return;

	Items[idx].Checked = checked;
}


//! Removes a menu item
void CGUIContextMenu::removeItem(u32 idx)
{
	if (idx >= Items.size())
		return;

	if (Items[idx].SubMenu)
	{
		Items[idx].SubMenu->drop();
		Items[idx].SubMenu = 0;
	}

	Items.erase(idx);
	recalculateSize();
}


//! Removes all menu items
void CGUIContextMenu::removeAllItems()
{
	for (u32 i=0; i<Items.size(); ++i)
		if (Items[i].SubMenu)
			Items[i].SubMenu->drop();

	Items.clear();
	recalculateSize();
}


//! called if an event happened.
bool CGUIContextMenu::OnEvent(const SEvent& event)
{
	if (isEnabled())
	{

		switch(event.EventType)
		{
		case EET_GUI_EVENT:
			switch(event.GUIEvent.EventType)
			{
			case EGET_ELEMENT_FOCUS_LOST:
				if (event.GUIEvent.Caller == this && !isMyChild(event.GUIEvent.Element) && AllowFocus)
				{
					// set event parent of submenus
					IGUIElement * p =  EventParent ? EventParent : Parent;
					setEventParent(p);

					SEvent event;
					event.EventType = EET_GUI_EVENT;
					event.GUIEvent.Caller = this;
					event.GUIEvent.Element = 0;
					event.GUIEvent.EventType = EGET_ELEMENT_CLOSED;
					if ( !p->OnEvent(event) )
					{
						if ( CloseHandling & ECMC_HIDE )
						{
							setVisible(false);
						}
						if ( CloseHandling & ECMC_REMOVE )
						{
							remove();
						}
					}

					return false;
				}
				break;
			case EGET_ELEMENT_FOCUSED:
				if (event.GUIEvent.Caller == this && !AllowFocus)
				{
					return true;
				}
				break;
			default:
				break;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			switch(event.MouseInput.Event)
			{
			case EMIE_LMOUSE_LEFT_UP:
				{
					// menu might be removed if it loses focus in sendClick, so grab a reference
					grab();
					const u32 t = sendClick(core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y));
					if ((t==0 || t==1) && Environment->hasFocus(this))
						Environment->removeFocus(this);
					drop();
				}
				return true;
			case EMIE_LMOUSE_PRESSED_DOWN:
				return true;
			case EMIE_MOUSE_MOVED:
				if (Environment->hasFocus(this))
					highlight(core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y), true);
				return true;
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


//! Sets the visible state of this element.
void CGUIContextMenu::setVisible(bool visible)
{
	HighLighted = -1;
	ChangeTime = os::Timer::getTime();
	for (u32 j=0; j<Items.size(); ++j)
		if (Items[j].SubMenu)
			Items[j].SubMenu->setVisible(false);

	IGUIElement::setVisible(visible);
}


//! sends a click Returns:
//! 0 if click went outside of the element,
//! 1 if a valid button was clicked,
//! 2 if a nonclickable element was clicked
u32 CGUIContextMenu::sendClick(const core::position2d<s32>& p)
{
	u32 t = 0;

	// get number of open submenu
	s32 openmenu = -1;
	s32 j;
	for (j=0; j<(s32)Items.size(); ++j)
		if (Items[j].SubMenu && Items[j].SubMenu->isVisible())
		{
			openmenu = j;
			break;
		}

	// delegate click operation to submenu
	if (openmenu != -1)
	{
		t = Items[j].SubMenu->sendClick(p);
		if (t != 0)
			return t; // clicked something
	}

	// check click on myself
	if (isPointInside(p) &&
		(u32)HighLighted < Items.size())
	{
		if (!Items[HighLighted].Enabled ||
			Items[HighLighted].IsSeparator ||
			Items[HighLighted].SubMenu)
			return 2;

		if ( Items[HighLighted].AutoChecking )
		{
			Items[HighLighted].Checked = Items[HighLighted].Checked ? false : true;
		}

		SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.Caller = this;
		event.GUIEvent.Element = 0;
		event.GUIEvent.EventType = EGET_MENU_ITEM_SELECTED;
		if (EventParent)
 			EventParent->OnEvent(event);
		else if (Parent)
			Parent->OnEvent(event);

		return 1;
	}

	return 0;
}


//! returns true, if an element was highligted
bool CGUIContextMenu::highlight(const core::position2d<s32>& p, bool canOpenSubMenu)
{
	if (!isEnabled())
	{
		return false;
	}

	// get number of open submenu
	s32 openmenu = -1;
	s32 i;
	for (i=0; i<(s32)Items.size(); ++i)
		if (Items[i].Enabled && Items[i].SubMenu && Items[i].SubMenu->isVisible())
		{
			openmenu = i;
			break;
		}

	// delegate highlight operation to submenu
	if (openmenu != -1)
	{
		if (Items[openmenu].Enabled && Items[openmenu].SubMenu->highlight(p, canOpenSubMenu))
		{
			HighLighted = openmenu;
			ChangeTime = os::Timer::getTime();
			return true;
		}
	}

	// highlight myself
	for (i=0; i<(s32)Items.size(); ++i)
	{
		if (Items[i].Enabled && getHRect(Items[i], AbsoluteRect).isPointInside(p))
		{
			HighLighted = i;
			ChangeTime = os::Timer::getTime();

			// make submenus visible/invisible
				for (s32 j=0; j<(s32)Items.size(); ++j)
					if (Items[j].SubMenu)
					{
						if ( j == i && canOpenSubMenu && Items[j].Enabled )
							Items[j].SubMenu->setVisible(true);
						else if ( j != i )
							Items[j].SubMenu->setVisible(false);
					}
			return true;
		}
	}

	HighLighted = openmenu;
	return false;
}


//! returns the item highlight-area
core::rect<s32> CGUIContextMenu::getHRect(const SItem& i, const core::rect<s32>& absolute) const
{
	core::rect<s32> r = absolute;
	r.UpperLeftCorner.Y += i.PosY;
	r.LowerRightCorner.Y = r.UpperLeftCorner.Y + i.Dim.Height;
	return r;
}


//! Gets drawing rect of Item
core::rect<s32> CGUIContextMenu::getRect(const SItem& i, const core::rect<s32>& absolute) const
{
	core::rect<s32> r = absolute;
	r.UpperLeftCorner.Y += i.PosY;
	r.LowerRightCorner.Y = r.UpperLeftCorner.Y + i.Dim.Height;
	r.UpperLeftCorner.X += 20;
	return r;
}


//! draws the element and its children
void CGUIContextMenu::draw()
{
	if (!IsVisible)
		return;

	IGUISkin* skin = Environment->getSkin();

	if (!skin)
		return;

	IGUIFont* font = skin->getFont(EGDF_MENU);
	if (font != LastFont)
	{
		if (LastFont)
			LastFont->drop();
		LastFont = font;
		if (LastFont)
			LastFont->grab();

		recalculateSize();
	}

	IGUISpriteBank* sprites = skin->getSpriteBank();

	core::rect<s32> rect = AbsoluteRect;
	core::rect<s32>* clip = 0;

	// draw frame
	skin->draw3DMenuPane(this, AbsoluteRect, clip);

	// loop through all menu items

	rect = AbsoluteRect;
	s32 y = AbsoluteRect.UpperLeftCorner.Y;

	for (s32 i=0; i<(s32)Items.size(); ++i)
	{
		if (Items[i].IsSeparator)
		{
			// draw separator
			rect = AbsoluteRect;
			rect.UpperLeftCorner.Y += Items[i].PosY + 3;
			rect.LowerRightCorner.Y = rect.UpperLeftCorner.Y + 1;
			rect.UpperLeftCorner.X += 5;
			rect.LowerRightCorner.X -= 5;
			skin->draw2DRectangle(this, skin->getColor(EGDC_3D_SHADOW), rect, clip);

			rect.LowerRightCorner.Y += 1;
			rect.UpperLeftCorner.Y += 1;
			skin->draw2DRectangle(this, skin->getColor(EGDC_3D_HIGH_LIGHT), rect, clip);

			y += 10;
		}
		else
		{
			rect = getRect(Items[i], AbsoluteRect);

			// draw highlighted

			if (i == HighLighted && Items[i].Enabled)
			{
				core::rect<s32> r = AbsoluteRect;
				r.LowerRightCorner.Y = rect.LowerRightCorner.Y;
				r.UpperLeftCorner.Y = rect.UpperLeftCorner.Y;
				r.LowerRightCorner.X -= 5;
				r.UpperLeftCorner.X += 5;
				skin->draw2DRectangle(this, skin->getColor(EGDC_HIGH_LIGHT), r, clip);
			}

			// draw text

			EGUI_DEFAULT_COLOR c = EGDC_BUTTON_TEXT;

			if (i == HighLighted)
				c = EGDC_HIGH_LIGHT_TEXT;

			if (!Items[i].Enabled)
				c = EGDC_GRAY_TEXT;

			if (font)
				font->draw(Items[i].Text.c_str(), rect,
					skin->getColor(c), false, true, clip);

			// draw submenu symbol
			if (Items[i].SubMenu && sprites)
			{
				core::rect<s32> r = rect;
				r.UpperLeftCorner.X = r.LowerRightCorner.X - 15;

				sprites->draw2DSprite(skin->getIcon(EGDI_CURSOR_RIGHT),
					r.getCenter(), clip, skin->getColor(c),
					(i == HighLighted) ? ChangeTime : 0,
					(i == HighLighted) ? os::Timer::getTime() : 0,
					(i == HighLighted), true);
			}

			// draw checked symbol
			if (Items[i].Checked && sprites)
			{
				core::rect<s32> r = rect;
				r.LowerRightCorner.X = r.UpperLeftCorner.X - 15;
				r.UpperLeftCorner.X = r.LowerRightCorner.X + 15;
				sprites->draw2DSprite(skin->getIcon(EGDI_CHECK_BOX_CHECKED),
					r.getCenter(), clip, skin->getColor(c),
					(i == HighLighted) ? ChangeTime : 0,
					(i == HighLighted) ? os::Timer::getTime() : 0,
					(i == HighLighted), true);
			}
		}
	}

	IGUIElement::draw();
}


void CGUIContextMenu::recalculateSize()
{
	IGUIFont* font = Environment->getSkin()->getFont(EGDF_MENU);

	if (!font)
		return;

	core::rect<s32> rect;
	rect.UpperLeftCorner = RelativeRect.UpperLeftCorner;
	u32 width = 100;
	u32 height = 3;

	u32 i;
	for (i=0; i<Items.size(); ++i)
	{
		if (Items[i].IsSeparator)
		{
			Items[i].Dim.Width = 100;
			Items[i].Dim.Height = 10;
		}
		else
		{
			Items[i].Dim = font->getDimension(Items[i].Text.c_str());
			Items[i].Dim.Width += 40;

			if (Items[i].Dim.Width > width)
				width = Items[i].Dim.Width;
		}

		Items[i].PosY = height;
		height += Items[i].Dim.Height;
	}

	height += 5;

	if (height < 10)
		height = 10;

	rect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + width;
	rect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + height;

	setRelativePosition(rect);

	// recalculate submenus
	for (i=0; i<Items.size(); ++i)
	{
		if (Items[i].SubMenu)
		{
			// move submenu
			const s32 w = Items[i].SubMenu->getAbsolutePosition().getWidth();
			const s32 h = Items[i].SubMenu->getAbsolutePosition().getHeight();

            core::rect<s32> subRect(width-5, Items[i].PosY, width+w-5, Items[i].PosY+h);

            // if it would be drawn beyond the right border, then add it to the left side
            gui::IGUIElement * root = Environment->getRootGUIElement();
            if ( root )
            {
                core::rect<s32> rectRoot( root->getAbsolutePosition() );
                if ( getAbsolutePosition().UpperLeftCorner.X+subRect.LowerRightCorner.X > rectRoot.LowerRightCorner.X )
                {
                    subRect.UpperLeftCorner.X = -w;
                    subRect.LowerRightCorner.X = 0;
                }
            }

			Items[i].SubMenu->setRelativePosition(subRect);
		}
	}
}


//! Returns the selected item in the menu
s32 CGUIContextMenu::getSelectedItem() const
{
	return HighLighted;
}


//! \return Returns a pointer to the submenu of an item.
IGUIContextMenu* CGUIContextMenu::getSubMenu(u32 idx) const
{
	if (idx >= Items.size())
		return 0;

	return Items[idx].SubMenu;
}


//! Returns command id of a menu item
s32 CGUIContextMenu::getItemCommandId(u32 idx) const
{
	if (idx >= Items.size())
		return -1;

	return Items[idx].CommandId;
}


//! Sets the command id of a menu item
void CGUIContextMenu::setItemCommandId(u32 idx, s32 id)
{
	if (idx >= Items.size())
		return;

	Items[idx].CommandId = id;
}


//! Writes attributes of the element.
void CGUIContextMenu::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUIElement::serializeAttributes(out,options);
	out->addPosition2d("Position", Pos);

	if (Parent->getType() == EGUIET_CONTEXT_MENU || Parent->getType() == EGUIET_MENU )
	{
		const IGUIContextMenu* const ptr = (const IGUIContextMenu*)Parent;
		// find the position of this item in its parent's list
		u32 i;
		// VC6 needs the cast for this
		for (i=0; (i<ptr->getItemCount()) && (ptr->getSubMenu(i) != (const IGUIContextMenu*)this); ++i)
			; // do nothing

		out->addInt("ParentItem", i);
	}

	out->addInt("CloseHandling", (s32)CloseHandling);

	// write out the item list
	out->addInt("ItemCount", Items.size());

	core::stringc tmp;

	for (u32 i=0; i < Items.size(); ++i)
	{
		tmp = "IsSeparator"; tmp += i;
		out->addBool(tmp.c_str(), Items[i].IsSeparator);

		if (!Items[i].IsSeparator)
		{
			tmp = "Text"; tmp += i;
			out->addString(tmp.c_str(), Items[i].Text.c_str());
			tmp = "CommandID"; tmp += i;
			out->addInt(tmp.c_str(), Items[i].CommandId);
			tmp = "Enabled"; tmp += i;
			out->addBool(tmp.c_str(), Items[i].Enabled);
			tmp = "Checked"; tmp += i;
			out->addBool(tmp.c_str(), Items[i].Checked);
			tmp = "AutoChecking"; tmp += i;
			out->addBool(tmp.c_str(), Items[i].AutoChecking);
		}
	}
}


//! Reads attributes of the element
void CGUIContextMenu::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUIElement::deserializeAttributes(in,options);

	Pos = in->getAttributeAsPosition2d("Position");

	// link to this item's parent
	if (Parent && ( Parent->getType() == EGUIET_CONTEXT_MENU || Parent->getType() == EGUIET_MENU ) )
		((CGUIContextMenu*)Parent)->setSubMenu(in->getAttributeAsInt("ParentItem"),this);

	CloseHandling = (ECONTEXT_MENU_CLOSE)in->getAttributeAsInt("CloseHandling");

	removeAllItems();

	// read the item list
	const s32 count = in->getAttributeAsInt("ItemCount");

	for (s32 i=0; i<count; ++i)
	{
		core::stringc tmp;
		core::stringw txt;
		s32 commandid=-1;
		bool enabled=true;
		bool checked=false;
		bool autochecking=false;

		tmp = "IsSeparator"; tmp += i;
		if ( in->existsAttribute(tmp.c_str()) && in->getAttributeAsBool(tmp.c_str()) )
			addSeparator();
		else
		{
			tmp = "Text"; tmp += i;
			if ( in->existsAttribute(tmp.c_str()) )
				txt = in->getAttributeAsStringW(tmp.c_str());

			tmp = "CommandID"; tmp += i;
			if ( in->existsAttribute(tmp.c_str()) )
				commandid = in->getAttributeAsInt(tmp.c_str());

			tmp = "Enabled"; tmp += i;
			if ( in->existsAttribute(tmp.c_str()) )
				enabled = in->getAttributeAsBool(tmp.c_str());

			tmp = "Checked"; tmp += i;
			if ( in->existsAttribute(tmp.c_str()) )
				checked = in->getAttributeAsBool(tmp.c_str());

 			tmp = "AutoChecking"; tmp += i;
			if ( in->existsAttribute(tmp.c_str()) )
				autochecking = in->getAttributeAsBool(tmp.c_str());

 			addItem(core::stringw(txt.c_str()).c_str(), commandid, enabled, false, checked, autochecking);
		}
	}

	recalculateSize();
}


// because sometimes the element has no parent at click time
void CGUIContextMenu::setEventParent(IGUIElement *parent)
{
	EventParent = parent;

	for (u32 i=0; i<Items.size(); ++i)
		if (Items[i].SubMenu)
			Items[i].SubMenu->setEventParent(parent);
}


bool CGUIContextMenu::hasOpenSubMenu() const
{
	for (u32 i=0; i<Items.size(); ++i)
		if (Items[i].SubMenu && Items[i].SubMenu->isVisible())
			return true;

	return false;
}


void CGUIContextMenu::closeAllSubMenus()
{
	for (u32 i=0; i<Items.size(); ++i)
		if (Items[i].SubMenu)
			Items[i].SubMenu->setVisible(false);

	//HighLighted = -1;
}


} // end namespace
} // end namespace

#endif // _IRR_COMPILE_WITH_GUI_

