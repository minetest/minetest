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

#include "lua_api/l_modchannels.h"
#include "l_internal.h"

int ModApiChannels::l_mod_channel_join(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	getGameDef(L)->joinModChannel(luaL_checkstring(L, 1));
	return 0;
}

int ModApiChannels::l_mod_channel_leave(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	getGameDef(L)->leaveModChannel(luaL_checkstring(L, 1));
	return 0;
}

int ModApiChannels::l_mod_channel_send_msg(lua_State *L)
{
	if (!lua_isstring(L, 1))
		return 0;

	// @TODO serialize message

	getGameDef(L)->sendModChannelMessage(luaL_checkstring(L, 1), "blob");
	return 0;
}

void ModApiChannels::Initialize(lua_State *L, int top)
{
	API_FCT(mod_channel_join);
	API_FCT(mod_channel_leave);
	API_FCT(mod_channel_send_msg);
}
