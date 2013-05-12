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

#include "cpp_api/scriptapi.h"
#include "lua_api/l_base.h"
#include "common/c_internal.h"
#include "log.h"

extern "C" {
#include "lua.h"
}

ModApiBase::ModApiBase() {
	ScriptApi::registerModApiModule(this);
}

Server* ModApiBase::getServer(lua_State* L) {
	return get_scriptapi(L)->getServer();
}

Environment* ModApiBase::getEnv(lua_State* L) {
	return get_scriptapi(L)->getEnv();
}

bool ModApiBase::registerFunction(	lua_State* L,
								const char* name,
								lua_CFunction fct,
								int top
								) {
	//TODO check presence first!

	lua_pushstring(L,name);
	lua_pushcfunction(L,fct);
	lua_settable(L, top);

	return true;
}

struct EnumString es_BiomeTerrainType[] =
{
	{BIOME_TERRAIN_NORMAL, "normal"},
	{BIOME_TERRAIN_LIQUID, "liquid"},
	{BIOME_TERRAIN_NETHER, "nether"},
	{BIOME_TERRAIN_AETHER, "aether"},
	{BIOME_TERRAIN_FLAT,   "flat"},
	{0, NULL},
};
