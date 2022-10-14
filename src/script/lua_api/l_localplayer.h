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

class LocalPlayer;

class LuaLocalPlayer : public ModApiBase
{
private:
	static const luaL_Reg methods[];

	// garbage collector
	static int gc_object(lua_State *L);

	// get_velocity(self)
	static int l_get_velocity(lua_State *L);

	// get_hp(self)
	static int l_get_hp(lua_State *L);

	// get_name(self)
	static int l_get_name(lua_State *L);

	// get_wield_index(self)
	static int l_get_wield_index(lua_State *L);

	// get_wielded_item(self)
	static int l_get_wielded_item(lua_State *L);

	static int l_is_attached(lua_State *L);
	static int l_is_touching_ground(lua_State *L);
	static int l_is_in_liquid(lua_State *L);
	static int l_is_in_liquid_stable(lua_State *L);
	static int l_is_climbing(lua_State *L);
	static int l_swimming_vertical(lua_State *L);

	static int l_get_physics_override(lua_State *L);

	static int l_get_override_pos(lua_State *L);

	static int l_get_last_pos(lua_State *L);
	static int l_get_last_velocity(lua_State *L);
	static int l_get_last_look_vertical(lua_State *L);
	static int l_get_last_look_horizontal(lua_State *L);

	// get_control(self)
	static int l_get_control(lua_State *L);

	// get_breath(self)
	static int l_get_breath(lua_State *L);

	// get_pos(self)
	static int l_get_pos(lua_State *L);

	// get_movement_acceleration(self)
	static int l_get_movement_acceleration(lua_State *L);

	// get_movement_speed(self)
	static int l_get_movement_speed(lua_State *L);

	// get_movement(self)
	static int l_get_movement(lua_State *L);

	// get_armor_groups(self)
	static int l_get_armor_groups(lua_State *L);

	// hud_add(self, id, form)
	static int l_hud_add(lua_State *L);

	// hud_rm(self, id)
	static int l_hud_remove(lua_State *L);

	// hud_change(self, id, stat, data)
	static int l_hud_change(lua_State *L);
	// hud_get(self, id)
	static int l_hud_get(lua_State *L);

	static int l_get_move_resistance(lua_State *L);

	static LocalPlayer *getobject(LuaLocalPlayer *ref);
	static LocalPlayer *getobject(lua_State *L, int narg);

	LocalPlayer *m_localplayer = nullptr;

public:
	LuaLocalPlayer(LocalPlayer *m);
	~LuaLocalPlayer() = default;

	static void create(lua_State *L, LocalPlayer *m);

	static void Register(lua_State *L);

	static const char className[];
};
