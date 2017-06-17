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

#include <cstring>

#include "cpp_api/s_player.h"
#include "cpp_api/s_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "util/string.h"
#include "server.h"
#include "lua_api/l_base.h"

void ScriptApiPlayer::on_newplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_newplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_newplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiPlayer::on_dieplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_dieplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_dieplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
}

bool ScriptApiPlayer::on_punchplayer(ServerActiveObject *player,
		ServerActiveObject *hitter,
		float time_from_last_punch,
		const ToolCapabilities *toolcap,
		v3f dir,
		s16 damage)
{
	SCRIPTAPI_PRECHECKHEADER
	// Get core.registered_on_punchplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_punchplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	objectrefGetOrCreate(L, hitter);
	lua_pushnumber(L, time_from_last_punch);
	push_tool_capabilities(L, *toolcap);
	push_v3f(L, dir);
	lua_pushnumber(L, damage);
	runCallbacks(6, RUN_CALLBACKS_MODE_OR);
	return lua_toboolean(L, -1);
}

s16 ScriptApiPlayer::on_player_hpchange(ServerActiveObject *player,
	s16 hp_change)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Get core.registered_on_player_hpchange
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_player_hpchange");
	lua_remove(L, -2);

	objectrefGetOrCreate(L, player);
	lua_pushnumber(L, hp_change);
	PCALL_RES(lua_pcall(L, 2, 1, error_handler));
	hp_change = lua_tointeger(L, -1);
	lua_pop(L, 2); // Pop result and error handler
	return hp_change;
}

bool ScriptApiPlayer::on_respawnplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_respawnplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_respawnplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	runCallbacks(1, RUN_CALLBACKS_MODE_OR);
	bool positioning_handled_by_some = lua_toboolean(L, -1);
	return positioning_handled_by_some;
}

bool ScriptApiPlayer::on_prejoinplayer(
	const std::string &name,
	const std::string &ip,
	std::string *reason)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_prejoinplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_prejoinplayers");
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, ip.c_str());
	runCallbacks(2, RUN_CALLBACKS_MODE_OR);
	if (lua_isstring(L, -1)) {
		reason->assign(lua_tostring(L, -1));
		return true;
	}
	return false;
}

void ScriptApiPlayer::on_joinplayer(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_joinplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_joinplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	runCallbacks(1, RUN_CALLBACKS_MODE_FIRST);
}

void ScriptApiPlayer::on_leaveplayer(ServerActiveObject *player,
		bool timeout)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_leaveplayers
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_leaveplayers");
	// Call callbacks
	objectrefGetOrCreate(L, player);
	lua_pushboolean(L, timeout);
	runCallbacks(2, RUN_CALLBACKS_MODE_FIRST);
}

inline void ScriptApiPlayer::prepare_anticheat_check(ServerActiveObject *player,
		const std::string &cheat_type,
		int &error_handler)
{
	lua_State *L = getStack();

	// Insert error handler
	error_handler = PUSH_ERROR_HANDLER(L);

	// Get core.get_anticheat_object
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "get_anticheat_object");
	lua_remove(L, -2);

	// Push parameters
	objectrefGetOrCreate(L, player);
	lua_pushlstring(L, cheat_type.c_str(), cheat_type.size());

	PCALL_RES(lua_pcall(L, 2, 1, error_handler)); // Call it.

	// Stack now looks like this: ... <error handler> <CheatObject>
	// We don't pop the error handler, the calling function will use it

	lua_getfield(L, -1, "check"); // Push function
	lua_pushvalue(L, -2); // Push "self" parameter
	lua_remove(L, -3); // Remove CheatObject
}

bool ScriptApiPlayer::anticheat_check_interacted_too_far(ServerActiveObject *player,
		v3s16 node_pos)
{
	SCRIPTAPI_PRECHECKHEADER

	ANTICHEAT_CHECK_HEADER("interacted_too_far")

	push_v3s16(L, node_pos);

	ANTICHEAT_CHECK_FOOTER(1)
}

bool ScriptApiPlayer::anticheat_check_finished_unknown_dig(ServerActiveObject *player,
		v3s16 started_pos, v3s16 completed_pos)
{
	SCRIPTAPI_PRECHECKHEADER

	ANTICHEAT_CHECK_HEADER("finished_unknown_dig")

	push_v3s16(L, started_pos);
	push_v3s16(L, completed_pos);

	ANTICHEAT_CHECK_FOOTER(2)
}

bool ScriptApiPlayer::anticheat_check_dug_unbreakable(ServerActiveObject *player,
		v3s16 node_pos)
{
	SCRIPTAPI_PRECHECKHEADER

	ANTICHEAT_CHECK_HEADER("dug_unbreakable")

	push_v3s16(L, node_pos);

	ANTICHEAT_CHECK_FOOTER(1)
}

bool ScriptApiPlayer::anticheat_check_interacted_while_dead(ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	ANTICHEAT_CHECK_HEADER("interacted_while_dead")

	ANTICHEAT_CHECK_FOOTER(0)
}

// This interface is needed by the dug_too_fast and moved_too_fast anticheat check function
static int grab_lag_pool(lua_State *L)
{
	lua_Number dtime = luaL_checknumber(L, 1);
	const char *name = luaL_checkstring(L, 2);
	const char *pool_type = luaL_checkstring(L, 3);
	RemotePlayer *remote_player = dynamic_cast<ServerEnvironment *>(ModApiBase::getEnv(L))->getPlayer(name);
	// We assume that the player exists, and make no check
	if (strcmp(pool_type, "dig") == 0)
		lua_pushboolean(L, remote_player->getPlayerSAO()->getDigPool().grab(dtime));
	else // pool_type = "move"
		lua_pushboolean(L, remote_player->getPlayerSAO()->getMovePool().grab(dtime));
	return 1;
}

bool ScriptApiPlayer::anticheat_check_dug_too_fast(ServerActiveObject *player,
		v3s16 node_pos, float nocheat_time)
{
	SCRIPTAPI_PRECHECKHEADER

	ANTICHEAT_CHECK_HEADER("dug_too_fast")

	push_v3s16(L, node_pos);
	lua_pushnumber(L, nocheat_time);
	lua_pushcfunction(L, grab_lag_pool);

	ANTICHEAT_CHECK_FOOTER(3)
}

bool ScriptApiPlayer::anticheat_check_moved_too_fast(RemotePlayer *remote_player, ServerActiveObject *player)
{
	SCRIPTAPI_PRECHECKHEADER

	ANTICHEAT_CHECK_HEADER("moved_too_fast")

	push_v3s16(L, floatToInt(player->getBasePosition(), BS));
	lua_pushnumber(L, dynamic_cast<ServerEnvironment *>(getEnv())->getMaxLagEstimate());
	lua_pushnumber(L, remote_player->getPlayerSAO()->getTimeFromLastTeleport());
	push_v3s16(L, floatToInt(remote_player->getPlayerSAO()->getLastGoodPosition(), BS));
	lua_pushcfunction(L, grab_lag_pool);

	ANTICHEAT_CHECK_FOOTER(5)
}

void ScriptApiPlayer::on_playerReceiveFields(ServerActiveObject *player,
		const std::string &formname,
		const StringMap &fields)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_chat_messages
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_player_receive_fields");
	// Call callbacks
	// param 1
	objectrefGetOrCreate(L, player);
	// param 2
	lua_pushstring(L, formname.c_str());
	// param 3
	lua_newtable(L);
	StringMap::const_iterator it;
	for (it = fields.begin(); it != fields.end(); ++it) {
		const std::string &name = it->first;
		const std::string &value = it->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}
	runCallbacks(3, RUN_CALLBACKS_MODE_OR_SC);
}

ScriptApiPlayer::~ScriptApiPlayer()
{
}
