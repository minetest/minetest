/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef ENVIRONMENT_HEADER
#define ENVIRONMENT_HEADER

/*
	This class is the game's environment.
	It contains:
	- The map
	- Players
	- Other objects
	- The current time in the game, etc.
*/

#include <list>
#include "common_irrlicht.h"
#include "player.h"
#include "map.h"
#include <ostream>

class Environment
{
public:
	// Environment will delete the map passed to the constructor
	Environment(Map *map, std::ostream &dout);
	~Environment();
	/*
		This can do anything to the environment, such as removing
		timed-out players.
		Also updates Map's timers.
	*/
	void step(f32 dtime);

	Map & getMap();

	/*
		Environment deallocates players after use.
	*/
	void addPlayer(Player *player);
	void removePlayer(u16 peer_id);
#ifndef SERVER
	LocalPlayer * getLocalPlayer();
#endif
	Player * getPlayer(u16 peer_id);
	Player * getPlayer(const char *name);
	core::list<Player*> getPlayers();
	core::list<Player*> getPlayers(bool ignore_disconnected);
	void printPlayers(std::ostream &o);

#ifndef SERVER
	void updateMeshes(v3s16 blockpos);
	void expireMeshes(bool only_daynight_diffed);
#endif
	void setDayNightRatio(u32 r);
	u32 getDayNightRatio();

private:
	Map *m_map;
	core::list<Player*> m_players;
	// Debug output goes here
	std::ostream &m_dout;
	u32 m_daynight_ratio;
};

#endif

