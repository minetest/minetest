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

#include "l_server_sound.h"
#include "l_internal.h"
#include "common/c_content.h"
#include "server.h"

/* ModApiServerSound */

// sound_play(spec, parameters, [ephemeral])
int ModApiServerSound::l_sound_play(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ServerPlayingSound params;
	read_simplesoundspec(L, 1, params.spec);
	read_server_sound_params(L, 2, params);
	bool ephemeral = lua_gettop(L) > 2 && readParam<bool>(L, 3);
	if (ephemeral) {
		getServer(L)->playSound(std::move(params), true);
		lua_pushnil(L);
	} else {
		params.grabbed = true;
		s32 handle = getServer(L)->playSound(std::move(params));
		ServerSoundHandle::create(L, handle);
	}
	return 1;
}

void ModApiServerSound::Initialize(lua_State *L, int top)
{
	API_FCT(sound_play);
}

/* ServerSoundHandle */

ServerSoundHandle *ServerSoundHandle::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	return *(ServerSoundHandle **)ud; // unbox pointer
}

int ServerSoundHandle::gc_object(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	std::unique_ptr<ServerSoundHandle> o(*(ServerSoundHandle **)(lua_touserdata(L, 1)));
	if (getServer(L))
		getServer(L)->dropSound(o->m_handle);
	return 0;
}

// :stop()
int ServerSoundHandle::l_stop(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ServerSoundHandle *o = checkobject(L, 1);
	getServer(L)->stopSound(o->m_handle);
	return 0;
}

// :fade(step, gain)
int ServerSoundHandle::l_fade(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	ServerSoundHandle *o = checkobject(L, 1);
	float step = readParam<float>(L, 2);
	float gain = readParam<float>(L, 3);
	getServer(L)->fadeSound(o->m_handle, step, gain);
	return 0;
}

void ServerSoundHandle::create(lua_State *L, s32 handle)
{
	ServerSoundHandle *o = new ServerSoundHandle(handle);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void ServerSoundHandle::Register(lua_State *L)
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

	luaL_register(L, nullptr, methods);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable
}

const char ServerSoundHandle::className[] = "ServerSoundHandle";
const luaL_Reg ServerSoundHandle::methods[] = {
	luamethod(ServerSoundHandle, stop),
	luamethod(ServerSoundHandle, fade),
	{0,0}
};
