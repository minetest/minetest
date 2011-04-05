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

#include "guiFurnaceMenu.h"
#include "client.h"

GUIFurnaceMenu::GUIFurnaceMenu(
		gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		v3s16 nodepos,
		Client *client
		):
	GUIInventoryMenu(env, parent, id, menumgr, v2s16(8,9),
			client->getInventoryContext(), client),
	m_nodepos(nodepos),
	m_client(client)
{
	std::string furnace_inv_id;
	furnace_inv_id += "nodemeta:";
	furnace_inv_id += itos(nodepos.X);
	furnace_inv_id += ",";
	furnace_inv_id += itos(nodepos.Y);
	furnace_inv_id += ",";
	furnace_inv_id += itos(nodepos.Z);

	core::array<GUIInventoryMenu::DrawSpec> draw_spec;
	draw_spec.push_back(GUIInventoryMenu::DrawSpec(
			"list", furnace_inv_id, "fuel",
			v2s32(2, 3), v2s32(1, 1)));
	draw_spec.push_back(GUIInventoryMenu::DrawSpec(
			"list", furnace_inv_id, "src",
			v2s32(2, 1), v2s32(1, 1)));
	draw_spec.push_back(GUIInventoryMenu::DrawSpec(
			"list", furnace_inv_id, "dst",
			v2s32(5, 1), v2s32(2, 2)));
	draw_spec.push_back(GUIInventoryMenu::DrawSpec(
			"list", "current_player", "main",
			v2s32(0, 5), v2s32(8, 4)));
	setDrawSpec(draw_spec);
}

