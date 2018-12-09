/*
Minetest
Copyright (C) 2017 Loic Blot <loic.blot@unix-experience.fr>

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


#include "lua_api/l_minimap.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "client/client.h"
#include "client/minimap.h"
#include "settings.h"

LuaMinimap::LuaMinimap(Minimap *m) : m_minimap(m)
{
}

void LuaMinimap::create(lua_State *L, Minimap *m)
{
	LuaMinimap *o = new LuaMinimap(m);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);

	// Keep minimap object stack id
	int minimap_object = lua_gettop(L);

	lua_getglobal(L, "core");
	lua_getfield(L, -1, "ui");
	luaL_checktype(L, -1, LUA_TTABLE);
	int uitable = lua_gettop(L);

	lua_pushvalue(L, minimap_object); // Copy object to top of stack
	lua_setfield(L, uitable, "minimap");
}

int LuaMinimap::l_get_pos(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	push_v3s16(L, m->getPos());
	return 1;
}

int LuaMinimap::l_set_pos(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	m->setPos(read_v3s16(L, 2));
	return 1;
}

int LuaMinimap::l_get_angle(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	lua_pushinteger(L, m->getAngle());
	return 1;
}

int LuaMinimap::l_set_angle(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	m->setAngle(lua_tointeger(L, 2));
	return 1;
}

int LuaMinimap::l_get_mode(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	lua_pushinteger(L, m->getMinimapMode());
	return 1;
}

int LuaMinimap::l_set_mode(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	s32 mode = lua_tointeger(L, 2);
	if (mode < MINIMAP_MODE_OFF ||
		mode >= MINIMAP_MODE_COUNT) {
		return 0;
	}

	m->setMinimapMode((MinimapMode) mode);
	return 1;
}

int LuaMinimap::l_set_shape(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);
	if (!lua_isnumber(L, 2))
		return 0;

	m->setMinimapShape((MinimapShape)((int)lua_tonumber(L, 2)));
	return 0;
}

int LuaMinimap::l_get_shape(lua_State *L)
{
	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	lua_pushnumber(L, (int)m->getMinimapShape());
	return 1;
}

int LuaMinimap::l_show(lua_State *L)
{
	// If minimap is disabled by config, don't show it.
	if (!g_settings->getBool("enable_minimap"))
		return 1;

	Client *client = getClient(L);
	assert(client);

	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	if (m->getMinimapMode() == MINIMAP_MODE_OFF)
		m->setMinimapMode(MINIMAP_MODE_SURFACEx1);

	client->showMinimap(true);
	return 1;
}

int LuaMinimap::l_hide(lua_State *L)
{
	Client *client = getClient(L);
	assert(client);

	LuaMinimap *ref = checkobject(L, 1);
	Minimap *m = getobject(ref);

	if (m->getMinimapMode() != MINIMAP_MODE_OFF)
		m->setMinimapMode(MINIMAP_MODE_OFF);

	client->showMinimap(false);
	return 1;
}

LuaMinimap *LuaMinimap::checkobject(lua_State *L, int narg)
{
	NO_MAP_LOCK_REQUIRED;

	luaL_checktype(L, narg, LUA_TUSERDATA);

	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);

	return *(LuaMinimap **)ud;  // unbox pointer
}

Minimap* LuaMinimap::getobject(LuaMinimap *ref)
{
	return ref->m_minimap;
}

int LuaMinimap::gc_object(lua_State *L) {
	LuaMinimap *o = *(LuaMinimap **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

void LuaMinimap::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_openlib(L, 0, methods, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable
}

const char LuaMinimap::className[] = "Minimap";
const luaL_Reg LuaMinimap::methods[] = {
	luamethod(LuaMinimap, show),
	luamethod(LuaMinimap, hide),
	luamethod(LuaMinimap, get_pos),
	luamethod(LuaMinimap, set_pos),
	luamethod(LuaMinimap, get_angle),
	luamethod(LuaMinimap, set_angle),
	luamethod(LuaMinimap, get_mode),
	luamethod(LuaMinimap, set_mode),
	luamethod(LuaMinimap, set_shape),
	luamethod(LuaMinimap, get_shape),
	{0,0}
};
