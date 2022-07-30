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

struct ClientTexture
{
	/* per-spawner structure used to store the ParticleTexture structs
	 * that spawned particles will refer to through ClientTexRef */
	ParticleTexture tex;
	video::ITexture *ref = nullptr;

	ClientTexture() = default;
	ClientTexture(const ServerParticleTexture& p, ITextureSource *t):
			tex(p),
			ref(t->getTextureForMesh(p.string)) {};
};

struct ClientTexRef
{
	/* per-particle structure used to avoid massively duplicating the
	 * fairly large ParticleTexture struct */
	ParticleTexture* tex = nullptr;
	video::ITexture* ref = nullptr;
	ClientTexRef() = default;

	/* constructor used by particles spawned from a spawner */
	ClientTexRef(ClientTexture& t):
			tex(&t.tex), ref(t.ref) {};

	/* constructor used for node particles */
	ClientTexRef(decltype(ref) tp): ref(tp) {};
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
		const ClientTexRef &texture,
		v2f texpos,
		v2f texsize,
		video::SColor color
	);
	~Particle();

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

	bool get_expired ()
	{ return m_expiration < m_time; }

	ParticleSpawner *m_parent;

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
	ClientTexRef m_texture;
	video::SMaterial m_material;
	v2f m_texpos;
	v2f m_texsize;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;
	v3f m_drag;
	ParticleParamTypes::v3fRange m_jitter;
	ParticleParamTypes::f32Range m_bounce;
	LocalPlayer *m_player;
	float m_size;

	//! Color without lighting
	video::SColor m_base_color;
	//! Final rendered color
	video::SColor m_color;
	bool m_collisiondetection;
	bool m_collision_removal;
	bool m_object_collision;
	bool m_vertical;
	v3s16 m_camera_offset;
	struct TileAnimationParams m_animation;
	float m_animation_time = 0.0f;
	int m_animation_frame = 0;
	u8 m_glow;
	float m_alpha = 0.0f;
};

class ParticleSpawner
{
public:
	ParticleSpawner(IGameDef *gamedef,
		LocalPlayer *player,
		const ParticleSpawnerParameters &p,
		u16 attached_id,
		std::unique_ptr<ClientTexture[]> &texpool,
		size_t texcount,
		ParticleManager* p_manager);

	void step(float dtime, ClientEnvironment *env);

	size_t m_active;

	bool getExpired() const
	{ return m_dying || (p.amount <= 0 && p.time != 0); }
	void setDying() { m_dying = true; }

private:
	void spawnParticle(ClientEnvironment *env, float radius,
		const core::matrix4 *attached_absolute_pos_rot_matrix);

	ParticleManager *m_particlemanager;
	float m_time;
	bool m_dying;
	IGameDef *m_gamedef;
	LocalPlayer *m_player;
	ParticleSpawnerParameters p;
	std::unique_ptr<ClientTexture[]> m_texpool;
	size_t m_texcount;
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

	void addParticle(Particle* toadd);

private:
	void addParticleSpawner(u64 id, ParticleSpawner *toadd);
	void deleteParticleSpawner(u64 id);

	void stepParticles(float dtime);
	void stepSpawners(float dtime);

	void clearAll();

	std::vector<Particle*> m_particles;
	std::unordered_map<u64, ParticleSpawner*> m_particle_spawners;
	// Start the particle spawner ids generated from here after u32_max. lower values are
	// for server sent spawners.
	u64 m_next_particle_spawner_id = U32_MAX + 1;

	ClientEnvironment* m_env;
	std::mutex m_particle_list_lock;
	std::mutex m_spawner_list_lock;
};
