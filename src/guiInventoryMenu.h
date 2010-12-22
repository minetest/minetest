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
#include "utility.h"

void drawInventoryItem(gui::IGUIEnvironment* env,
		InventoryItem *item, core::rect<s32> rect,
		const core::rect<s32> *clip=0);

class GUIInventoryMenu : public gui::IGUIElement
{
	struct ItemSpec
	{
		ItemSpec()
		{
			i = -1;
		}
		ItemSpec(const std::string &a_name, s32 a_i)
		{
			listname = a_name;
			i = a_i;
		}
		bool isValid() const
		{
			return i != -1;
		}

		std::string listname;
		s32 i;
	};

	struct ListDrawSpec
	{
		ListDrawSpec()
		{
		}
		ListDrawSpec(const std::string &a_name, v2s32 a_pos, v2s32 a_geom)
		{
			listname = a_name;
			pos = a_pos;
			geom = a_geom;
		}

		std::string listname;
		v2s32 pos;
		v2s32 geom;
	};

public:
	GUIInventoryMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			Inventory *inventory);
	~GUIInventoryMenu();

	/*
		Remove and re-add (or reposition) stuff
	*/
	void resizeGui();
	
	ItemSpec getItemAtPos(v2s32 p) const;
	void drawList(const ListDrawSpec &s);
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
	
	// Actions returned by this are sent to the server.
	// Server replies by updating the inventory.
	InventoryAction* getNextAction();
	
private:
	v2s32 getBasePos() const
	{
		return padding + AbsoluteRect.UpperLeftCorner;
	}

	v2s32 padding;
	v2s32 spacing;
	v2s32 imgsize;

	core::array<ListDrawSpec> m_draw_positions;

	Inventory *m_inventory;
	v2u32 m_screensize_old;

	ItemSpec *m_selected_item;
	
	Queue<InventoryAction*> m_actions;
};

#endif

