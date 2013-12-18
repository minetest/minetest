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

#include <map>

#include "cpp_api/s_base.h"


class ScriptApiPlayer
		: virtual public ScriptApiBase
{
public:
	virtual ~ScriptApiPlayer();

	void on_newplayer(ServerActiveObject *player);
	void on_dieplayer(ServerActiveObject *player);
	bool on_respawnplayer(ServerActiveObject *player);
	bool on_prejoinplayer(std::string name, std::string ip, std::string &reason);
	void on_joinplayer(ServerActiveObject *player);
	void on_leaveplayer(ServerActiveObject *player);
	void on_cheat(ServerActiveObject *player, const std::string &cheat_type);

	void on_playerReceiveFields(ServerActiveObject *player,
			const std::string &formname,
			const std::map<std::string, std::string> &fields);
};


#endif /* S_PLAYER_H_ */
