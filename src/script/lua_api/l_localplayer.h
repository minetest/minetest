/*
Minetest
Copyright (C) 2017 Dumbeldor, Vincent Glize <vincent.glize@live.fr>

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

#include "l_base.h"
#include "l_internal.h"

class LocalPlayer;

class LuaLocalPlayer : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	// garbage collector
	ENTRY_POINT_DECL(gc_object);

	// get_velocity(self)
	ENTRY_POINT_DECL(l_get_velocity);

	// get_hp(self)
	ENTRY_POINT_DECL(l_get_hp);

	// get_name(self)
	ENTRY_POINT_DECL(l_get_name);

	// get_wield_index(self)
	ENTRY_POINT_DECL(l_get_wield_index);

	// get_wielded_item(self)
	ENTRY_POINT_DECL(l_get_wielded_item);

	ENTRY_POINT_DECL(l_is_attached);
	ENTRY_POINT_DECL(l_is_touching_ground);
	ENTRY_POINT_DECL(l_is_in_liquid);
	ENTRY_POINT_DECL(l_is_in_liquid_stable);
	ENTRY_POINT_DECL(l_is_climbing);
	ENTRY_POINT_DECL(l_swimming_vertical);

	ENTRY_POINT_DECL(l_get_physics_override);

	ENTRY_POINT_DECL(l_get_override_pos);

	ENTRY_POINT_DECL(l_get_last_pos);
	ENTRY_POINT_DECL(l_get_last_velocity);
	ENTRY_POINT_DECL(l_get_last_look_vertical);
	ENTRY_POINT_DECL(l_get_last_look_horizontal);

	// get_control(self)
	ENTRY_POINT_DECL(l_get_control);

	// get_breath(self)
	ENTRY_POINT_DECL(l_get_breath);

	// get_pos(self)
	ENTRY_POINT_DECL(l_get_pos);

	// get_movement_acceleration(self)
	ENTRY_POINT_DECL(l_get_movement_acceleration);

	// get_movement_speed(self)
	ENTRY_POINT_DECL(l_get_movement_speed);

	// get_movement(self)
	ENTRY_POINT_DECL(l_get_movement);

	// get_armor_groups(self)
	ENTRY_POINT_DECL(l_get_armor_groups);

	// hud_add(self, id, form)
	ENTRY_POINT_DECL(l_hud_add);

	// hud_rm(self, id)
	ENTRY_POINT_DECL(l_hud_remove);

	// hud_change(self, id, stat, data)
	ENTRY_POINT_DECL(l_hud_change);
	// hud_get(self, id)
	ENTRY_POINT_DECL(l_hud_get);

	ENTRY_POINT_DECL(l_get_move_resistance);

	LocalPlayer *m_localplayer = nullptr;

public:
	LuaLocalPlayer(LocalPlayer *m);
	~LuaLocalPlayer() = default;

	static void create(lua_State *L, LocalPlayer *m);

	static LuaLocalPlayer *checkobject(lua_State *L, int narg);
	static LocalPlayer *getobject(LuaLocalPlayer *ref);
	static LocalPlayer *getobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};
