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
#include "modalMenu.h"

void drawInventoryItem(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		InventoryItem *item, core::rect<s32> rect,
		const core::rect<s32> *clip);

class GUIInventoryMenu : public GUIModalMenu
{
	struct ItemSpec
	{
		ItemSpec()
		{
			i = -1;
		}
		ItemSpec(const std::string &a_inventoryname,
				const std::string &a_listname,
				s32 a_i)
		{
			inventoryname = a_inventoryname;
			listname = a_listname;
			i = a_i;
		}
		bool isValid() const
		{
			return i != -1;
		}

		std::string inventoryname;
		std::string listname;
		s32 i;
	};

	struct ListDrawSpec
	{
		ListDrawSpec()
		{
		}
		ListDrawSpec(const std::string &a_inventoryname,
				const std::string &a_listname,
				v2s32 a_pos, v2s32 a_geom)
		{
			inventoryname = a_inventoryname;
			listname = a_listname;
			pos = a_pos;
			geom = a_geom;
		}

		std::string inventoryname;
		std::string listname;
		v2s32 pos;
		v2s32 geom;
	};
public:
	struct DrawSpec
	{
		DrawSpec()
		{
		}
		DrawSpec(const std::string &a_type,
				const std::string &a_name,
				const std::string &a_subname,
				v2s32 a_pos,
				v2s32 a_geom)
		{
			type = a_type;
			name = a_name;
			subname = a_subname;
			pos = a_pos;
			geom = a_geom;
		}

		std::string type;
		std::string name;
		std::string subname;
		v2s32 pos;
		v2s32 geom;
	};

	GUIInventoryMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr,
			v2s16 menu_size,
			core::array<DrawSpec> &init_draw_spec,
			InventoryContext *c,
			InventoryManager *invmgr
			);
	~GUIInventoryMenu();

	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);
	
	ItemSpec getItemAtPos(v2s32 p) const;
	void drawList(const ListDrawSpec &s);
	void drawMenu();

	bool OnEvent(const SEvent& event);
	
private:
	v2s32 getBasePos() const
	{
		return padding + AbsoluteRect.UpperLeftCorner;
	}

	v2s16 m_menu_size;

	v2s32 padding;
	v2s32 spacing;
	v2s32 imgsize;
	
	InventoryContext *m_c;
	InventoryManager *m_invmgr;

	core::array<DrawSpec> m_init_draw_spec;
	core::array<ListDrawSpec> m_draw_spec;

	ItemSpec *m_selected_item;
};

#endif

