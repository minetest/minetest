/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include <cassert>
#include <log.h>
#include "lua_api/l_modchannels.h"
#include "l_internal.h"
#include "modchannels.h"

int ModApiChannels::l_mod_channel_join(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	std::string channel = luaL_checkstring(L, 1);
	if (channel.empty())
		return 0;

	getGameDef(L)->joinModChannel(channel);
	ModChannel *channelObj = getGameDef(L)->getModChannel(channel);
	assert(channelObj);
	LuaModChannel::create(L, channelObj);

	int object = lua_gettop(L);
	lua_pushvalue(L, object);
	return 1;
}

void ModApiChannels::Initialize(lua_State *L, int top)
{
	API_FCT(mod_channel_join);
}

/*
 * LuaModChannel
 */

LuaModChannel::LuaModChannel(ModChannel *modchannel) : m_object(modchannel)
{
}

LuaModChannel::~LuaModChannel()
{
}

int LuaModChannel::l_leave(lua_State *L)
{
	LuaModChannel *ref = checkobject(L, 1);
	ModChannel *channel = getobject(ref);
	if (!channel)
		return 0;

	getGameDef(L)->leaveModChannel(channel->getName());
	// Channel left, invalidate the channel object ptr
	// This permits to invalidate every object action from Lua because core removed
	// channel consuming link
	ref->m_object = nullptr;
	return 0;
}

int LuaModChannel::l_send_all(lua_State *L)
{
	LuaModChannel *ref = checkobject(L, 1);
	ModChannel *channel = getobject(ref);
	if (!channel || !channel->canWrite())
		return 0;

	// @TODO serialize message
	std::string message = luaL_checkstring(L, 2);

	getGameDef(L)->sendModChannelMessage(channel->getName(), message);
	return 0;
}

int LuaModChannel::l_is_writeable(lua_State *L)
{
	LuaModChannel *ref = checkobject(L, 1);
	ModChannel *channel = getobject(ref);
	if (!channel)
		return 0;

	lua_pushboolean(L, channel->canWrite());
	return 1;
}
void LuaModChannel::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable); // hide metatable from lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1); // Drop metatable

	luaL_openlib(L, 0, methods, 0); // fill methodtable
	lua_pop(L, 1);			// Drop methodtable
}

void LuaModChannel::create(lua_State *L, ModChannel *channel)
{
	LuaModChannel *o = new LuaModChannel(channel);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

int LuaModChannel::gc_object(lua_State *L)
{
	LuaModChannel *o = *(LuaModChannel **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

LuaModChannel *LuaModChannel::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);

	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);

	return *(LuaModChannel **)ud; // unbox pointer
}

ModChannel *LuaModChannel::getobject(LuaModChannel *ref)
{
	return ref->m_object;
}

// clang-format off
const char LuaModChannel::className[] = "LuaModChannel";
const luaL_Reg LuaModChannel::methods[] = {
	luamethod(LuaModChannel, leave),
	luamethod(LuaModChannel, is_writeable),
	luamethod(LuaModChannel, send_all),
	{0, 0},
};
// clang-format on
