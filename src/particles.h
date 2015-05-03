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

#ifndef PARTICLES_HEADER
#define PARTICLES_HEADER

#define DIGGING_PARTICLES_AMOUNT 10

#include <iostream>
#include "irrlichttypes_extrabloated.h"
#include "client/tile.h"
#include "localplayer.h"
#include "environment.h"

struct ClientEvent;
class ParticleManager;

/**
 * Class doing particle as well as their spawners handling
 */
class ParticleManager
{
public:
	ParticleManager(ClientEnvironment* env, irr::scene::ISceneManager* smgr);

	void handleParticleEvent(ClientEvent *event, IGameDef *gamedef, LocalPlayer *player);

	void addDiggingParticles(IGameDef* gamedef, LocalPlayer *player,
				 v3s16 pos, const TileSpec tiles[]);

	void addPunchingParticles(IGameDef* gamedef, LocalPlayer *player,
				  v3s16 pos, const TileSpec tiles[]);

	void addNodeParticle(IGameDef* gamedef, LocalPlayer *player,
			     v3s16 pos, const TileSpec tiles[], u32 number);

protected:

private:
	ClientEnvironment* m_env;
	irr::scene::ISceneManager* m_smgr;
};

#endif
