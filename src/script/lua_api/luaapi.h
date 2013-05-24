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

#ifndef LUAAPI_H_
#define LUAAPI_H_


class ModApiBasic : public ModApiBase {

public:
	ModApiBasic();

	bool Initialize(lua_State* L,int top);

private:
	// debug(text)
	// Writes a line to dstream
	static int l_debug(lua_State *L);

	// log([level,] text)
	// Writes a line to the logger.
	// The one-argument version logs to infostream.
	// The two-argument version accept a log level: error, action, info, or verbose.
	static int l_log(lua_State *L);

	// request_shutdown()
	static int l_request_shutdown(lua_State *L);

	// get_server_status()
	static int l_get_server_status(lua_State *L);

	// register_biome_groups({frequencies})
	static int l_register_biome_groups(lua_State *L);

	// register_biome({lots of stuff})
	static int l_register_biome(lua_State *L);

	// setting_set(name, value)
	static int l_setting_set(lua_State *L);

	// setting_get(name)
	static int l_setting_get(lua_State *L);

	// setting_getbool(name)
	static int l_setting_getbool(lua_State *L);

	// setting_save()
	static int l_setting_save(lua_State *L);

	// chat_send_all(text)
	static int l_chat_send_all(lua_State *L);

	// chat_send_player(name, text)
	static int l_chat_send_player(lua_State *L);

	// get_player_privs(name, text)
	static int l_get_player_privs(lua_State *L);

	// get_player_ip()
	static int l_get_player_ip(lua_State *L);

	// get_ban_list()
	static int l_get_ban_list(lua_State *L);

	// get_ban_description()
	static int l_get_ban_description(lua_State *L);

	// ban_player()
	static int l_ban_player(lua_State *L);

	// unban_player_or_ip()
	static int l_unban_player_of_ip(lua_State *L);

	// show_formspec(playername,formname,formspec)
	static int l_show_formspec(lua_State *L);

	// get_dig_params(groups, tool_capabilities[, time_from_last_punch])
	static int l_get_dig_params(lua_State *L);

	// get_hit_params(groups, tool_capabilities[, time_from_last_punch])
	static int l_get_hit_params(lua_State *L);

	// get_current_modname()
	static int l_get_current_modname(lua_State *L);

	// get_modpath(modname)
	static int l_get_modpath(lua_State *L);

	// get_modnames()
	// the returned list is sorted alphabetically for you
	static int l_get_modnames(lua_State *L);

	// get_worldpath()
	static int l_get_worldpath(lua_State *L);

	// sound_play(spec, parameters)
	static int l_sound_play(lua_State *L);

	// sound_stop(handle)
	static int l_sound_stop(lua_State *L);

	// is_singleplayer()
	static int l_is_singleplayer(lua_State *L);

	// get_password_hash(name, raw_password)
	static int l_get_password_hash(lua_State *L);

	// notify_authentication_modified(name)
	static int l_notify_authentication_modified(lua_State *L);

	// rollback_get_last_node_actor(p, range, seconds) -> actor, p, seconds
	static int l_rollback_get_last_node_actor(lua_State *L);

	// rollback_revert_actions_by(actor, seconds) -> bool, log messages
	static int l_rollback_revert_actions_by(lua_State *L);

	static int l_register_ore(lua_State *L);

	static struct EnumString es_OreType[];
};



#endif /* LUAAPI_H_ */
