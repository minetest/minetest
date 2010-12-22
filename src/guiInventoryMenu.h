/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef GUIINVENTORYMENU_HEADER
#define GUIINVENTORYMENU_HEADER

#include "common_irrlicht.h"
#include "inventory.h"

void drawInventoryItem(gui::IGUIEnvironment* env,
		InventoryItem *item, core::rect<s32> rect,
		const core::rect<s32> *clip=0);

class GUIInventorySlot: public gui::IGUIElement
{
public:
	GUIInventorySlot(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id, core::rect<s32> rect);
	
	void setItem(InventoryItem *item)
	{
		m_item = item;
	}

	void draw();

	bool OnEvent(const SEvent& event);

private:
	InventoryItem *m_item;
};

class GUIInventoryMenu : public gui::IGUIElement
{
public:
	GUIInventoryMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			Inventory *inventory);
	~GUIInventoryMenu();

	/*
		Remove and re-add (or reposition) stuff
	*/
	void resizeGui();

	// Updates stuff from inventory to screen
	void update();

	void draw();

	void launch()
	{
		setVisible(true);
		Environment->setFocus(this);
	}

	bool canTakeFocus(gui::IGUIElement *e)
	{
		return (e && (e == this || isMyChild(e)));
	}

	bool OnEvent(const SEvent& event);
	
private:
	Inventory *m_inventory;
	core::array<GUIInventorySlot*> m_slots;
	v2u32 m_screensize_old;
};

#endif

