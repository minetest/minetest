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
	LUAREF_OBJECT(LocalPlayer);
private:
	static int l_get_velocity(lua_State *L);

	static int l_get_hp(lua_State *L);

	static int l_get_name(lua_State *L);

	static int l_is_attached(lua_State *L);
	static int l_is_touching_ground(lua_State *L);
	static int l_is_in_liquid(lua_State *L);
	static int l_is_in_liquid_stable(lua_State *L);
	static int l_get_liquid_viscosity(lua_State *L);
	static int l_is_climbing(lua_State *L);
	static int l_swimming_vertical(lua_State *L);

	static int l_get_physics_override(lua_State *L);

	static int l_get_override_pos(lua_State *L);

	static int l_get_last_pos(lua_State *L);
	static int l_get_last_velocity(lua_State *L);
	static int l_get_last_look_vertical(lua_State *L);
	static int l_get_last_look_horizontal(lua_State *L);
	static int l_get_key_pressed(lua_State *L);

	static int l_get_breath(lua_State *L);

	static int l_get_pos(lua_State *L);

	static int l_get_movement_acceleration(lua_State *L);

	static int l_get_movement_speed(lua_State *L);

	static int l_get_movement(lua_State *L);

	// hud_add(self, id, form)
	static int l_hud_add(lua_State *L);

	// hud_rm(self, id)
	static int l_hud_remove(lua_State *L);

	// hud_change(self, id, stat, data)
	static int l_hud_change(lua_State *L);
	// hud_get(self, id)
	static int l_hud_get(lua_State *L);
public:
	static LocalPlayer *getobject(lua_State *L, int narg);
};
