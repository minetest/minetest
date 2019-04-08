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

#include <iostream>
#include "irrlichttypes_extrabloated.h"
#include "client/tile.h"
#include "localplayer.h"
#include "tileanimation.h"

struct ClientEvent;
class ClientEnvironment;
struct MapNode;
struct ContentFeatures;
/**
 * Class doing handling of single particles and particle spawners
 */
class ParticleManager
{
public:
	ParticleManager(ClientEnvironment* env);
	~ParticleManager();

	void handleParticleEvent(ClientEvent *event, Client *client,
			LocalPlayer *player);

	void addDiggingParticles(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
		const MapNode &n, const ContentFeatures &f);

	void addNodeParticle(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
		const MapNode &n, const ContentFeatures &f, u32 number = 1);

	/**
	* This function is only used by client particle spawners
	*
	* We don't need to check the particle spawner list because client ID will n
	* ever overlap (u64)
	* @return new id
	 */
	u64 generateSpawnerId()
	{
			return m_next_particle_spawner_id++;
	}

	u64 getSingleParticleNumber();
	u64 getParticleSpawnerNumber();
	void removeParticleSpawner(u64 id);

private:
	ClientEnvironment* m_env;
	irr::scene::ISceneManager *m_smgr;

	u64 m_next_particle_spawner_id = U32_MAX + 1;
	std::unordered_map<u64, scene::IParticleSystemSceneNode*> m_particle_spawners;
	std::mutex m_spawner_list_lock;
};
