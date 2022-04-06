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

#pragma once

#include "lua_api/l_base.h"
#include "lua_api/l_internal.h"
#include "itemdef.h"
#include "tool.h"

class ModApiClient : public ModApiBase
{
private:
	// get_current_modname()
	ENTRY_POINT_DECL(l_get_current_modname);

	// get_modpath(modname)
	ENTRY_POINT_DECL(l_get_modpath);

	// print(text)
	ENTRY_POINT_DECL(l_print);

	// display_chat_message(message)
	ENTRY_POINT_DECL(l_display_chat_message);

	// send_chat_message(message)
	ENTRY_POINT_DECL(l_send_chat_message);

	// clear_out_chat_queue()
	ENTRY_POINT_DECL(l_clear_out_chat_queue);

	// get_player_names()
	ENTRY_POINT_DECL(l_get_player_names);

	// show_formspec(name, formspec)
	ENTRY_POINT_DECL(l_show_formspec);

	// send_respawn()
	ENTRY_POINT_DECL(l_send_respawn);

	// disconnect()
	ENTRY_POINT_DECL(l_disconnect);

	// gettext(text)
	ENTRY_POINT_DECL(l_gettext);

	// get_last_run_mod(n)
	ENTRY_POINT_DECL(l_get_last_run_mod);

	// set_last_run_mod(modname)
	ENTRY_POINT_DECL(l_set_last_run_mod);

	// get_node(pos)
	ENTRY_POINT_DECL(l_get_node_or_nil);

	// get_language()
	ENTRY_POINT_DECL(l_get_language);

	// get_wielded_item()
	ENTRY_POINT_DECL(l_get_wielded_item);

	// get_meta(pos)
	ENTRY_POINT_DECL(l_get_meta);

	// sound_play(spec, parameters)
	ENTRY_POINT_DECL(l_sound_play);

	// sound_stop(handle)
	ENTRY_POINT_DECL(l_sound_stop);

	// sound_fade(handle, step, gain)
	ENTRY_POINT_DECL(l_sound_fade);

	// get_server_info()
	ENTRY_POINT_DECL(l_get_server_info);

	// get_item_def(itemstring)
	ENTRY_POINT_DECL(l_get_item_def);

	// get_node_def(nodename)
	ENTRY_POINT_DECL(l_get_node_def);

	// get_privilege_list()
	ENTRY_POINT_DECL(l_get_privilege_list);

	// get_builtin_path()
	ENTRY_POINT_DECL(l_get_builtin_path);

	// get_csm_restrictions()
	ENTRY_POINT_DECL(l_get_csm_restrictions);

public:
	static void Initialize(lua_State *L, int top);
};
