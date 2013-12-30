/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include <vector>
#include <algorithm>

#include "util/serialize.h"
#include "server.h"
#include "player.h"
#include "clientserver.h"


// Spawns a particle on peer with peer_id
void Server::SendSpawnParticle(u16 peer_id, v3f pos, v3f velocity, v3f acceleration,
				float expirationtime, float size, bool collisiondetection,
				std::string texture)
{
	DSTACK(__FUNCTION_NAME);

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_SPAWN_PARTICLE);
	writeV3F1000(os, pos);
	writeV3F1000(os, velocity);
	writeV3F1000(os, acceleration);
	writeF1000(os, expirationtime);
	writeF1000(os, size);
	writeU8(os,  collisiondetection);
	os<<serializeLongString(texture);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

// Spawns a particle on all peers
void Server::SendSpawnParticleAll(v3f pos, v3f velocity, v3f acceleration,
				float expirationtime, float size, bool collisiondetection,
				std::string texture)
{
	DSTACK(__FUNCTION_NAME);
	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); i++)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		SendSpawnParticle(client->peer_id, pos, velocity, acceleration,
			expirationtime, size, collisiondetection, texture);
	}
}

// Adds a ParticleSpawner on peer with peer_id
void Server::SendAddParticleSpawner(u16 peer_id, u16 amount, float spawntime, v3f minpos, v3f maxpos,
	v3f minvel, v3f maxvel, v3f minacc, v3f maxacc, float minexptime, float maxexptime,
	float minsize, float maxsize, bool collisiondetection, std::string texture, u32 id)
{
	DSTACK(__FUNCTION_NAME);

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_ADD_PARTICLESPAWNER);

	writeU16(os, amount);
	writeF1000(os, spawntime);
	writeV3F1000(os, minpos);
	writeV3F1000(os, maxpos);
	writeV3F1000(os, minvel);
	writeV3F1000(os, maxvel);
	writeV3F1000(os, minacc);
	writeV3F1000(os, maxacc);
	writeF1000(os, minexptime);
	writeF1000(os, maxexptime);
	writeF1000(os, minsize);
	writeF1000(os, maxsize);
	writeU8(os,  collisiondetection);
	os<<serializeLongString(texture);
	writeU32(os, id);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

// Adds a ParticleSpawner on all peers
void Server::SendAddParticleSpawnerAll(u16 amount, float spawntime, v3f minpos, v3f maxpos,
	v3f minvel, v3f maxvel, v3f minacc, v3f maxacc, float minexptime, float maxexptime,
	float minsize, float maxsize, bool collisiondetection, std::string texture, u32 id)
{
	DSTACK(__FUNCTION_NAME);
	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); i++)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		SendAddParticleSpawner(client->peer_id, amount, spawntime,
			minpos, maxpos, minvel, maxvel, minacc, maxacc,
			minexptime, maxexptime, minsize, maxsize, collisiondetection, texture, id);
	}
}

void Server::SendDeleteParticleSpawner(u16 peer_id, u32 id)
{
	DSTACK(__FUNCTION_NAME);

	std::ostringstream os(std::ios_base::binary);
	writeU16(os, TOCLIENT_DELETE_PARTICLESPAWNER);

	writeU16(os, id);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendDeleteParticleSpawnerAll(u32 id)
{
	DSTACK(__FUNCTION_NAME);
	for(std::map<u16, RemoteClient*>::iterator
		i = m_clients.begin();
		i != m_clients.end(); i++)
	{
		// Get client and check that it is valid
		RemoteClient *client = i->second;
		assert(client->peer_id == i->first);
		if(client->serialization_version == SER_FMT_VER_INVALID)
			continue;

		SendDeleteParticleSpawner(client->peer_id, id);
	}
}

void Server::spawnParticle(const char *playername, v3f pos,
		v3f velocity, v3f acceleration,
		float expirationtime, float size, bool
		collisiondetection, std::string texture)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(playername);
	if(!player)
		return;
	SendSpawnParticle(player->peer_id, pos, velocity, acceleration,
			expirationtime, size, collisiondetection, texture);
}

void Server::spawnParticleAll(v3f pos, v3f velocity, v3f acceleration,
		float expirationtime, float size,
		bool collisiondetection, std::string texture)
{
	DSTACK(__FUNCTION_NAME);
	SendSpawnParticleAll(pos, velocity, acceleration,
			expirationtime, size, collisiondetection, texture);
}

u32 Server::addParticleSpawner(const char *playername,
		u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, std::string texture)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(playername);
	if(!player)
		return -1;

	u32 id = 0;
	for(;;) // look for unused particlespawner id
	{
		id++;
		if (std::find(m_particlespawner_ids.begin(),
				m_particlespawner_ids.end(), id)
				== m_particlespawner_ids.end())
		{
			m_particlespawner_ids.push_back(id);
			break;
		}
	}

	SendAddParticleSpawner(player->peer_id, amount, spawntime,
		minpos, maxpos, minvel, maxvel, minacc, maxacc,
		minexptime, maxexptime, minsize, maxsize,
		collisiondetection, texture, id);

	return id;
}

u32 Server::addParticleSpawnerAll(u16 amount, float spawntime,
		v3f minpos, v3f maxpos,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection, std::string texture)
{
	DSTACK(__FUNCTION_NAME);
	u32 id = 0;
	for(;;) // look for unused particlespawner id
	{
		id++;
		if (std::find(m_particlespawner_ids.begin(),
				m_particlespawner_ids.end(), id)
				== m_particlespawner_ids.end())
		{
			m_particlespawner_ids.push_back(id);
			break;
		}
	}

	SendAddParticleSpawnerAll(amount, spawntime,
		minpos, maxpos, minvel, maxvel, minacc, maxacc,
		minexptime, maxexptime, minsize, maxsize,
		collisiondetection, texture, id);

	return id;
}

void Server::deleteParticleSpawner(const char *playername, u32 id)
{
	DSTACK(__FUNCTION_NAME);
	Player *player = m_env->getPlayer(playername);
	if(!player)
		return;

	m_particlespawner_ids.erase(
			std::remove(m_particlespawner_ids.begin(),
			m_particlespawner_ids.end(), id),
			m_particlespawner_ids.end());
	SendDeleteParticleSpawner(player->peer_id, id);
}

void Server::deleteParticleSpawnerAll(u32 id)
{
	DSTACK(__FUNCTION_NAME);
	m_particlespawner_ids.erase(
			std::remove(m_particlespawner_ids.begin(),
			m_particlespawner_ids.end(), id),
			m_particlespawner_ids.end());
	SendDeleteParticleSpawnerAll(id);
}
