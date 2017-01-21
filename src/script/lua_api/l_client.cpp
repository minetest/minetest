/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#include "l_client.h"
#include "l_internal.h"
#include "util/string.h"

int ModApiClient::l_get_current_modname(lua_State *L)
{
	lua_rawgeti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	return 1;
}

// display_chat_message(message)
int ModApiClient::l_display_chat_message(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	std::string message = luaL_checkstring(L, 1);
	getClient(L)->pushToChatQueue(utf8_to_wide(message));
	return 1;
}

void ModApiClient::Initialize(lua_State *L, int top)
{
	API_FCT(get_current_modname);
	API_FCT(display_chat_message);
}
