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

#ifndef S_PLAYER_H_
#define S_PLAYER_H_

#include "cpp_api/s_base.h"
#include "irr_v3d.h"
#include "util/string.h"
#include "inventory.h" // For inventry and itemstack declarations
#include "inventorymanager.h"

struct ToolCapabilities;

class ScriptApiPlayer
		: virtual public ScriptApiBase
{
public:
	virtual ~ScriptApiPlayer();

	void on_newplayer(ServerActiveObject *player);
	void on_dieplayer(ServerActiveObject *player);
	bool on_respawnplayer(ServerActiveObject *player);
	void on_inventory_move_item(
		InventoryLocation from_loc,
		const std::string &from_list, int from_index,
		InventoryLocation to_loc,
		const std::string &to_list, int to_index,
		ItemStack &stack,
		int count, ServerActiveObject *player);
	void on_inventory_add_item(
		InventoryLocation to_loc,
		const std::string &to_list,
		ItemStack &added_stack,
		ItemStack &leftover_stack,
		ServerActiveObject *player);
	void on_inventory_drop_item(
		InventoryLocation from_loc,
		const std::string &from_list, int from_index,
		ItemStack &stack,
		ServerActiveObject *player, v3f pos);
	bool on_prejoinplayer(const std::string &name, const std::string &ip,
		std::string *reason);
	void on_joinplayer(ServerActiveObject *player);
	void on_leaveplayer(ServerActiveObject *player);
	void on_cheat(ServerActiveObject *player, const std::string &cheat_type);
	bool on_punchplayer(ServerActiveObject *player,
		ServerActiveObject *hitter, float time_from_last_punch,
		const ToolCapabilities *toolcap, v3f dir, s16 damage);
	s16 on_player_hpchange(ServerActiveObject *player, s16 hp_change);
	void on_playerReceiveFields(ServerActiveObject *player,
		const std::string &formname, const StringMap &fields);
};


#endif /* S_PLAYER_H_ */
