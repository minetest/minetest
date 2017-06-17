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

#ifndef S_PLAYER_H_
#define S_PLAYER_H_

#include "cpp_api/s_base.h"
#include "irr_v3d.h"
#include "util/string.h"
#include "content_sao.h"

#define ANTICHEAT_CHECK_HEADER(type)                                                     \
	int error_handler;                                                               \
	prepare_anticheat_check(player, (type), error_handler);

#define ANTICHEAT_CHECK_FOOTER(nb_params)                                                \
	/* Add 1 to nb_params for the self parameter */                                  \
	PCALL_RES(lua_pcall(L, (nb_params) + 1, 1, error_handler));                      \
	bool ret = (lua_toboolean(L, -1) != 0);                                          \
	lua_pop(L, 2); /* Pop error handler and return value */                          \
	return ret;

struct ToolCapabilities;

class ScriptApiPlayer : virtual public ScriptApiBase
{
public:
	virtual ~ScriptApiPlayer();

	void on_newplayer(ServerActiveObject *player);
	void on_dieplayer(ServerActiveObject *player);
	bool on_respawnplayer(ServerActiveObject *player);
	bool on_prejoinplayer(const std::string &name, const std::string &ip,
			std::string *reason);
	void on_joinplayer(ServerActiveObject *player);
	void on_leaveplayer(ServerActiveObject *player, bool timeout);
	bool anticheat_check_interacted_too_far(
			ServerActiveObject *player, v3s16 node_pos);
	bool anticheat_check_finished_unknown_dig(ServerActiveObject *player,
			v3s16 started_pos, v3s16 completed_pos);
	bool anticheat_check_dug_unbreakable(ServerActiveObject *player, v3s16 node_pos);
	bool anticheat_check_interacted_while_dead(ServerActiveObject *player);
	bool anticheat_check_dug_too_fast(
			ServerActiveObject *player, v3s16 node_pos, float nocheat_time);
	bool anticheat_check_moved_too_fast(
			RemotePlayer *remote_player, ServerActiveObject *player);
	bool on_punchplayer(ServerActiveObject *player, ServerActiveObject *hitter,
			float time_from_last_punch, const ToolCapabilities *toolcap,
			v3f dir, s16 damage);
	s16 on_player_hpchange(ServerActiveObject *player, s16 hp_change);
	void on_playerReceiveFields(ServerActiveObject *player,
			const std::string &formname, const StringMap &fields);

private:
	inline void prepare_anticheat_check(ServerActiveObject *player,
			const std::string &cheat_type, int &error_handler);
};

#endif /* S_PLAYER_H_ */
