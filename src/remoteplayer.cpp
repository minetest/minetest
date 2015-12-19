/*
Minetest
Copyright (C) 2010-2015 celeron55, Perttu Ahola <celeron55@gmail.com>
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

#include <fstream>
#include <sstream>


#include "remoteplayer.h"
#include "filesys.h"
#include "log.h"
#include "porting.h"
#include "content_sao.h"
#include "util/string.h"
#include "settings.h"


RemotePlayer::RemotePlayer(IGameDef *gamedef, const char *name):
	Player(gamedef, name),
	m_sao(NULL)
{
	movement_acceleration_default   = g_settings->getFloat("movement_acceleration_default")   * BS;
	movement_acceleration_air       = g_settings->getFloat("movement_acceleration_air")       * BS;
	movement_acceleration_fast      = g_settings->getFloat("movement_acceleration_fast")      * BS;
	movement_speed_walk             = g_settings->getFloat("movement_speed_walk")             * BS;
	movement_speed_crouch           = g_settings->getFloat("movement_speed_crouch")           * BS;
	movement_speed_fast             = g_settings->getFloat("movement_speed_fast")             * BS;
	movement_speed_climb            = g_settings->getFloat("movement_speed_climb")            * BS;
	movement_speed_jump             = g_settings->getFloat("movement_speed_jump")             * BS;
	movement_liquid_fluidity        = g_settings->getFloat("movement_liquid_fluidity")        * BS;
	movement_liquid_fluidity_smooth = g_settings->getFloat("movement_liquid_fluidity_smooth") * BS;
	movement_liquid_sink            = g_settings->getFloat("movement_liquid_sink")            * BS;
	movement_gravity                = g_settings->getFloat("movement_gravity")                * BS;
}

void RemotePlayer::save(std::string savedir)
{
	/*
	 * We have to open all possible player files in the players directory
	 * and check their player names because some file systems are not
	 * case-sensitive and player names are case-sensitive.
	 */

	// A player to deserialize files into to check their names
	RemotePlayer testplayer(m_gamedef, "");

	savedir += DIR_DELIM;
	std::string path = savedir + m_name;
	for (u32 i = 0; i < PLAYER_FILE_ALTERNATE_TRIES; i++) {
		if (!fs::PathExists(path)) {
			// Open file and serialize
			std::ostringstream ss(std::ios_base::binary);
			serialize(ss);
			if (!fs::safeWriteToFile(path, ss.str())) {
				infostream << "Failed to write " << path << std::endl;
			}
			setModified(false);
			return;
		}
		// Open file and deserialize
		std::ifstream is(path.c_str(), std::ios_base::binary);
		if (!is.good()) {
			infostream << "Failed to open " << path << std::endl;
			return;
		}
		testplayer.deSerialize(is, path);
		is.close();
		if (strcmp(testplayer.getName(), m_name) == 0) {
			// Open file and serialize
			std::ostringstream ss(std::ios_base::binary);
			serialize(ss);
			if (!fs::safeWriteToFile(path, ss.str())) {
				infostream << "Failed to write " << path << std::endl;
			}
			setModified(false);
			return;
		}
		path = savedir + m_name + itos(i);
	}

	infostream << "Didn't find free file for player " << m_name << std::endl;
	return;
}

/*
	RemotePlayer
*/
void RemotePlayer::setPosition(const v3f &position)
{
	Player::setPosition(position);
	if(m_sao)
		m_sao->setBasePosition(position);
}

