/*
Minetest
Copyright (C) 2022 sfan5 <sfan5@live.de>

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

#include "cpp_api/s_mapgen.h"
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"
#include "lua_api/l_vmanip.h"
#include "emerge.h"

void ScriptApiMapgen::on_mods_loaded()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_mods_loaded");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiMapgen::on_shutdown()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiMapgen::on_generated(BlockMakeData *bmdata, u32 seed)
{
	SCRIPTAPI_PRECHECKHEADER

	v3s16 minp = bmdata->blockpos_min * MAP_BLOCKSIZE;
	v3s16 maxp = bmdata->blockpos_max * MAP_BLOCKSIZE +
				 v3s16(1,1,1) * (MAP_BLOCKSIZE - 1);

	LuaVoxelManip::create(L, bmdata->vmanip, true);
	const int vmanip = lua_gettop(L);

	// Store vmanip globally (used by helpers)
	lua_getglobal(L, "core");
	lua_pushvalue(L, vmanip);
	lua_setfield(L, -2, "vmanip");

	// Call callbacks
	lua_getfield(L, -1, "registered_on_generateds");
	lua_pushvalue(L, vmanip);
	push_v3s16(L, minp);
	push_v3s16(L, maxp);
	lua_pushnumber(L, seed);
	runCallbacks(4, RUN_CALLBACKS_MODE_FIRST);
	lua_pop(L, 1); // return val

	// Unset core.vmanip again
	lua_pushnil(L);
	lua_setfield(L, -2, "vmanip");
}
