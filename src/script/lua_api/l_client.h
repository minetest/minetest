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

#ifndef L_CLIENT_H_
#define L_CLIENT_H_

#include "lua_api/l_base.h"

class ModApiClient : public ModApiBase {
private:
	// get_client_status()
	static int l_get_client_status(lua_State *L);

	// is_singleplayer()
	static int l_is_singleplayer(lua_State *L);

	// get_current_modname()
	static int l_get_current_modname(lua_State *L);

	// get_modpath(modname)
	static int l_get_modpath(lua_State *L);

	// get_modnames()
	// the returned list is sorted alphabetically for you
	static int l_get_modnames(lua_State *L);

	// print(text)
	static int l_print(lua_State *L);

	// show_formspec(playername,formname,formspec)
	static int l_show_formspec(lua_State *L);

	// sound_play(spec, parameters)
	static int l_sound_play(lua_State *L);

	// sound_stop(handle)
	static int l_sound_stop(lua_State *L);

	// get_player_privs(name, text)
	static int l_get_player_privs(lua_State *L);

	// get_player_information()
	static int l_get_player_information(lua_State *L);

	// notify_authentication_modified(name)
	static int l_notify_authentication_modified(lua_State *L);

#ifndef NDEBUG
	//  cause_error(type_of_error)
	static int l_cause_error(lua_State *L);
#endif

public:
	static void Initialize(lua_State *L, int top);

};

#endif /* L_CLIENT_H_ */
