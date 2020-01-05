/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

#include "inventorymanager.h"
#include "irrlichttypes_extrabloated.h"
#include "util/string.h"

class GUIFormSpecMenu;

class GUIInventoryList : public gui::IGUIElement
{
public:
	struct ItemSpec
	{
		ItemSpec() = default;

		ItemSpec(const InventoryLocation &a_inventoryloc,
				const std::string &a_listname,
				s32 a_i) :
			inventoryloc(a_inventoryloc),
			listname(a_listname),
			i(a_i)
		{
		}

		bool isValid() const { return i != -1; }

		InventoryLocation inventoryloc;
		std::string listname;
		s32 i = -1;
	};

	GUIInventoryList(gui::IGUIEnvironment *env,
		gui::IGUIElement *parent,
		s32 id,
		const core::rect<s32> &rectangle,
		InventoryManager *invmgr,
		const InventoryLocation &inventoryloc,
		const std::string &listname,
		const v2s32 &geom,
		const s32 start_item_i,
		const v2s32 &slot_size,
		const v2f32 &slot_spacing,
		GUIFormSpecMenu *fs_menu,
		const video::SColor &slotbg_n,
		const video::SColor &slotbg_h,
		bool slotborder,
		const video::SColor &slotbordercolor,
		gui::IGUIFont *font);

	virtual void draw() override;

	const InventoryLocation &getInventoryloc() const
	{
		return m_inventoryloc;
	}

	const std::string &getListname() const
	{
		return m_listname;
	}

	void setSlotBGColors(const video::SColor &slotbg_n, const video::SColor &slotbg_h)
	{
		m_slotbg_n = slotbg_n;
		m_slotbg_h = slotbg_h;
	}

	// returns -1 if not item is at pos p
	s32 getItemIndexAtPos(v2s32 p) const;

private:
	InventoryManager *m_invmgr;
	const InventoryLocation m_inventoryloc;
	const std::string m_listname;

	// specifies the width and height of the inventorylist in itemslots
	const v2s32 m_geom;
	// the first item's index in inventory
	const s32 m_start_item_i;

	// specifies how large the slot rects are
	const v2s32 m_slot_size;
	// specifies how large the space between slots is (space between is spacing-size)
	const v2f32 m_slot_spacing;

	// the GUIFormSpecMenu can have an item selected and co.
	GUIFormSpecMenu *m_fs_menu;

	// normal and highlighted background color
	video::SColor m_slotbg_n;
	video::SColor m_slotbg_h;

	// slotboarder
	bool m_slotborder;
	video::SColor m_slotbordercolor;

	// the font
	gui::IGUIFont *m_font;
};
