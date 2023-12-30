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
#include "../particles.h"

struct ClientEvent;
class ParticleManager;
class ClientEnvironment;
struct MapNode;
struct ContentFeatures;

struct ClientParticleTexture
{
	/* per-spawner structure used to store the ParticleTexture structs
	 * that spawned particles will refer to through ClientParticleTexRef */
	ParticleTexture tex;
	video::ITexture *ref = nullptr;

	ClientParticleTexture() = default;
	ClientParticleTexture(const ServerParticleTexture& p, ITextureSource *t):
			tex(p),
			ref(t->getTextureForMesh(p.string)) {};
};

struct ClientParticleTexRef
{
	/* per-particle structure used to avoid massively duplicating the
	 * fairly large ParticleTexture struct */
	ParticleTexture *tex = nullptr;
	video::ITexture *ref = nullptr;

	ClientParticleTexRef() = default;

	/* constructor used by particles spawned from a spawner */
	explicit ClientParticleTexRef(ClientParticleTexture &t):
			tex(&t.tex), ref(t.ref) {};

	/* constructor used for node particles */
	explicit ClientParticleTexRef(video::ITexture *tp): ref(tp) {};
};

class ParticleSpawner;

class Particle : public scene::ISceneNode
{
public:
	Particle(
		IGameDef *gamedef,
		LocalPlayer *player,
		ClientEnvironment *env,
		const ParticleParameters &p,
		const ClientParticleTexRef &texture,
		v2f texpos,
		v2f texsize,
		video::SColor color,
		ParticleSpawner *parent = nullptr,
		std::unique_ptr<ClientParticleTexture> owned_texture = nullptr
	);

	virtual const aabb3f &getBoundingBox() const
	{
		return m_box;
	}

	virtual u32 getMaterialCount() const
	{
		return 1;
	}

	virtual video::SMaterial& getMaterial(u32 i)
	{
		return m_material;
	}

	virtual void OnRegisterSceneNode();
	virtual void render();

	void step(float dtime);

	bool isExpired ()
	{ return m_expiration < m_time; }

	ParticleSpawner *getParent() { return m_parent; }

private:
	void updateLight();
	void updateVertices();
	void setVertexAlpha(float a);

	video::S3DVertex m_vertices[4];
	float m_time = 0.0f;
	float m_expiration;

	ClientEnvironment *m_env;
	IGameDef *m_gamedef;
	aabb3f m_box;
	aabb3f m_collisionbox;
	ClientParticleTexRef m_texture;
	video::SMaterial m_material;
	v2f m_texpos;
	v2f m_texsize;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;
	const ParticleParameters m_p;
	LocalPlayer *m_player;

	//! Color without lighting
	video::SColor m_base_color;
	//! Final rendered color
	video::SColor m_color;
	float m_animation_time = 0.0f;
	int m_animation_frame = 0;
	float m_alpha = 0.0f;

	ParticleSpawner *m_parent = nullptr;
	// Used if not spawned from a particlespawner
	std::unique_ptr<ClientParticleTexture> m_owned_texture;
};

class ParticleSpawner
{
public:
	ParticleSpawner(IGameDef *gamedef,
		LocalPlayer *player,
		const ParticleSpawnerParameters &params,
		u16 attached_id,
		std::vector<ClientParticleTexture> &&texpool,
		ParticleManager *p_manager);

	void step(float dtime, ClientEnvironment *env);

	bool getExpired() const
	{ return p.amount <= 0 && p.time != 0; }

	bool hasActive() const { return m_active != 0; }
	void decrActive() { m_active -= 1; }

private:
	void spawnParticle(ClientEnvironment *env, float radius,
		const core::matrix4 *attached_absolute_pos_rot_matrix);

	size_t m_active;
	ParticleManager *m_particlemanager;
	float m_time;
	IGameDef *m_gamedef;
	LocalPlayer *m_player;
	ParticleSpawnerParameters p;
	std::vector<ClientParticleTexture> m_texpool;
	std::vector<float> m_spawntimes;
	u16 m_attached_id;
};

/**
 * Class doing particle as well as their spawners handling
 */
class ParticleManager
{
friend class ParticleSpawner;
public:
	ParticleManager(ClientEnvironment* env);
	DISABLE_CLASS_COPY(ParticleManager)
	~ParticleManager();

	void step (float dtime);

	void handleParticleEvent(ClientEvent *event, Client *client,
			LocalPlayer *player);

	void addDiggingParticles(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
		const MapNode &n, const ContentFeatures &f);

	void addNodeParticle(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
		const MapNode &n, const ContentFeatures &f);

	void reserveParticleSpace(size_t max_estimate);

	/**
	 * This function is only used by client particle spawners
	 *
	 * We don't need to check the particle spawner list because client ID will
	 * never overlap (u64)
	 * @return new id
	 */
	u64 generateSpawnerId()
	{
		return m_next_particle_spawner_id++;
	}

protected:
	static bool getNodeParticleParams(const MapNode &n, const ContentFeatures &f,
		ParticleParameters &p, video::ITexture **texture, v2f &texpos,
		v2f &texsize, video::SColor *color, u8 tilenum = 0);

	void addParticle(std::unique_ptr<Particle> toadd);

private:
	void addParticleSpawner(u64 id, std::unique_ptr<ParticleSpawner> toadd);
	void deleteParticleSpawner(u64 id);

	void stepParticles(float dtime);
	void stepSpawners(float dtime);

	void clearAll();

	std::vector<std::unique_ptr<Particle>> m_particles;
	std::unordered_map<u64, std::unique_ptr<ParticleSpawner>> m_particle_spawners;
	std::vector<std::unique_ptr<ParticleSpawner>> m_dying_particle_spawners;
	// Start the particle spawner ids generated from here after u32_max. lower values are
	// for server sent spawners.
	u64 m_next_particle_spawner_id = U32_MAX + 1;

	ClientEnvironment *m_env;
	std::mutex m_particle_list_lock;
	std::mutex m_spawner_list_lock;
};
