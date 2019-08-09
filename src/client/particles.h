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
class ParticleManager;
class ClientEnvironment;
struct MapNode;
struct ContentFeatures;

class Particle : public scene::ISceneNode
{
	public:
	Particle(
		IGameDef* gamedef,
		LocalPlayer *player,
		ClientEnvironment *env,
		v3f pos,
		v3f velocity,
		v3f acceleration,
		float expirationtime,
		float size,
		bool collisiondetection,
		bool collision_removal,
		bool object_collision,
		bool vertical,
		video::ITexture *texture,
		v2f texpos,
		v2f texsize,
		const struct TileAnimationParams &anim,
		u8 glow,
		video::SColor color = video::SColor(0xFFFFFFFF)
	);
	~Particle() = default;

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

private:
	void updateLight();
	void updateVertices();

	video::S3DVertex m_vertices[4];
	float m_time = 0.0f;
	float m_expiration;

	ClientEnvironment *m_env;
	IGameDef *m_gamedef;
	aabb3f m_box;
	aabb3f m_collisionbox;
	video::SMaterial m_material;
	v2f m_texpos;
	v2f m_texsize;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;
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
};

class ParticleSpawner
{
public:
	ParticleSpawner(IGameDef* gamedef,
		LocalPlayer *player,
		u16 amount,
		float time,
		v3f minp, v3f maxp,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection,
		bool collision_removal,
		bool object_collision,
		u16 attached_id,
		bool vertical,
		video::ITexture *texture,
		const struct TileAnimationParams &anim, u8 glow,
		ParticleManager* p_manager);

	~ParticleSpawner() = default;

	void step(float dtime, ClientEnvironment *env);

	bool get_expired ()
	{ return (m_amount <= 0) && m_spawntime != 0; }

private:
	void spawnParticle(ClientEnvironment *env, float radius,
		const core::matrix4 *attached_absolute_pos_rot_matrix);

	ParticleManager *m_particlemanager;
	float m_time;
	IGameDef *m_gamedef;
	LocalPlayer *m_player;
	u16 m_amount;
	float m_spawntime;
	v3f m_minpos;
	v3f m_maxpos;
	v3f m_minvel;
	v3f m_maxvel;
	v3f m_minacc;
	v3f m_maxacc;
	float m_minexptime;
	float m_maxexptime;
	float m_minsize;
	float m_maxsize;
	video::ITexture *m_texture;
	std::vector<float> m_spawntimes;
	bool m_collisiondetection;
	bool m_collision_removal;
	bool m_object_collision;
	bool m_vertical;
	u16 m_attached_id;
	struct TileAnimationParams m_animation;
	u8 m_glow;
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

protected:
	void addParticle(Particle* toadd);

private:

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
