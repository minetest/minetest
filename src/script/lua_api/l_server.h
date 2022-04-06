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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"

class ModApiServer : public ModApiBase
{
private:
	// request_shutdown([message], [reconnect])
	ENTRY_POINT_DECL(l_request_shutdown);

	// get_server_status()
	ENTRY_POINT_DECL(l_get_server_status);

	// get_server_uptime()
	ENTRY_POINT_DECL(l_get_server_uptime);

	// get_server_max_lag()
	ENTRY_POINT_DECL(l_get_server_max_lag);

	// get_worldpath()
	ENTRY_POINT_DECL(l_get_worldpath);

	// is_singleplayer()
	ENTRY_POINT_DECL(l_is_singleplayer);

	// get_current_modname()
	ENTRY_POINT_DECL(l_get_current_modname);

	// get_modpath(modname)
	ENTRY_POINT_DECL(l_get_modpath);

	// get_modnames()
	// the returned list is sorted alphabetically for you
	ENTRY_POINT_DECL(l_get_modnames);

	// print(text)
	ENTRY_POINT_DECL(l_print);

	// chat_send_all(text)
	ENTRY_POINT_DECL(l_chat_send_all);

	// chat_send_player(name, text)
	ENTRY_POINT_DECL(l_chat_send_player);

	// show_formspec(playername,formname,formspec)
	ENTRY_POINT_DECL(l_show_formspec);

	// sound_play(spec, parameters)
	ENTRY_POINT_DECL(l_sound_play);

	// sound_stop(handle)
	ENTRY_POINT_DECL(l_sound_stop);

	// sound_fade(handle, step, gain)
	ENTRY_POINT_DECL(l_sound_fade);

	// dynamic_add_media(filepath)
	ENTRY_POINT_DECL(l_dynamic_add_media);

	// get_player_privs(name, text)
	ENTRY_POINT_DECL(l_get_player_privs);

	// get_player_ip()
	ENTRY_POINT_DECL(l_get_player_ip);

	// get_player_information(name)
	ENTRY_POINT_DECL(l_get_player_information);

	// get_ban_list()
	ENTRY_POINT_DECL(l_get_ban_list);

	// get_ban_description()
	ENTRY_POINT_DECL(l_get_ban_description);

	// ban_player()
	ENTRY_POINT_DECL(l_ban_player);

	// unban_player_or_ip()
	ENTRY_POINT_DECL(l_unban_player_or_ip);

	// disconnect_player(name, [reason]) -> success
	ENTRY_POINT_DECL(l_disconnect_player);

	// remove_player(name)
	ENTRY_POINT_DECL(l_remove_player);

	// notify_authentication_modified(name)
	ENTRY_POINT_DECL(l_notify_authentication_modified);

public:
	static void Initialize(lua_State *L, int top);
};
