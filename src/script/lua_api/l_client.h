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

#ifndef L_CLIENT_H_
#define L_CLIENT_H_

#include "lua_api/l_base.h"

class ModApiClient: public ModApiBase
{
private:
	// get_current_modname()
	static int l_get_current_modname(lua_State *L);

	// display_chat_message(message)
	static int l_display_chat_message(lua_State *L);
	
	// get_player_names()
	static int l_get_player_names(lua_State *L);

	// show_formspec(name, fornspec)
	static int l_show_formspec(lua_State *L);

	// send_respawn()
	static int l_send_respawn(lua_State *L);

	// gettext(text)
	static int l_gettext(lua_State *L);

	// get_last_run_mod(n)
	static int l_get_last_run_mod(lua_State *L);

	// set_last_run_mod(modname)
	static int l_set_last_run_mod(lua_State *L);

	// get_node(pos)
	static int l_get_node(lua_State *L);

	// get_node_or_nil(pos)
	static int l_get_node_or_nil(lua_State *L);

	// get_wielded_item()
	static int l_get_wielded_item(lua_State *L);
	
	// send_chat_message(message)
	static int l_send_chat_message(lua_State *L);

public:
	static void Initialize(lua_State *L, int top);
};

#endif
