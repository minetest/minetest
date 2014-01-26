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
#include "tile.h"
#include "localplayer.h"
#include "environment.h"

class Particle : public scene::ISceneNode
{
	public:
	Particle(
		IGameDef* gamedef,
		scene::ISceneManager* mgr,
		LocalPlayer *player,
		ClientEnvironment &env,
		v3f pos,
		v3f velocity,
		v3f acceleration,
		float expirationtime,
		float size,
		bool collisiondetection,
		bool vertical,
		video::ITexture *texture,
		v2f texpos,
		v2f texsize
	);
	~Particle();

	virtual const core::aabbox3d<f32>& getBoundingBox() const
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
	float m_time;
	float m_expiration;

	ClientEnvironment *m_env;
	IGameDef *m_gamedef;
	core::aabbox3d<f32> m_box;
	core::aabbox3d<f32> m_collisionbox;
	video::SMaterial m_material;
	v2f m_texpos;
	v2f m_texsize;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;
	LocalPlayer *m_player;
	float m_size;
	u8 m_light;
	bool m_collisiondetection;
	bool m_vertical;
	v3s16 m_camera_offset;
};

class ParticleSpawner
{
	public:
	ParticleSpawner(IGameDef* gamedef,
		scene::ISceneManager *smgr,
		LocalPlayer *player,
		u16 amount,
		float time,
		v3f minp, v3f maxp,
		v3f minvel, v3f maxvel,
		v3f minacc, v3f maxacc,
		float minexptime, float maxexptime,
		float minsize, float maxsize,
		bool collisiondetection,
		bool vertical,
		video::ITexture *texture,
		u32 id);

	~ParticleSpawner();

	void step(float dtime, ClientEnvironment &env);

	bool get_expired ()
	{ return (m_amount <= 0) && m_spawntime != 0; }

	private:
	float m_time;
	IGameDef *m_gamedef;
	scene::ISceneManager *m_smgr;
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
	bool m_vertical;
};

void allparticles_step (float dtime);
void allparticlespawners_step (float dtime, ClientEnvironment &env);

void delete_particlespawner (u32 id);
void clear_particles ();

void addDiggingParticles(IGameDef* gamedef, scene::ISceneManager* smgr,
	LocalPlayer *player, ClientEnvironment &env, v3s16 pos,
	const TileSpec tiles[]);

void addPunchingParticles(IGameDef* gamedef, scene::ISceneManager* smgr,
	LocalPlayer *player, ClientEnvironment &env, v3s16 pos,
	const TileSpec tiles[]);

void addNodeParticle(IGameDef* gamedef, scene::ISceneManager* smgr,
	LocalPlayer *player, ClientEnvironment &env, v3s16 pos,
	const TileSpec tiles[]);

#endif
