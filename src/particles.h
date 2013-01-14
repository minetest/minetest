/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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
		s32 id,
		v3f pos,
		v3f velocity,
		v3f acceleration,
		float expirationtime,
		float size,
		AtlasPointer texture
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

	void step(float dtime, ClientEnvironment &env);

	bool get_expired ()
	{ return m_expiration < m_time; }

private:
	float m_time;
	float m_expiration;

	IGameDef *m_gamedef;
	core::aabbox3d<f32> m_box;
	core::aabbox3d<f32> m_collisionbox;
	video::SMaterial m_material;
	v3f m_pos;
	v3f m_velocity;
	v3f m_acceleration;
	float tex_x0;
	float tex_x1;
	float tex_y0;
	float tex_y1;
	LocalPlayer *m_player;
	float m_size;
	AtlasPointer m_ap;
	u8 m_light;
};

void allparticles_step (float dtime, ClientEnvironment &env);

void addDiggingParticles(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos, const TileSpec tiles[]);
void addPunchingParticles(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos, const TileSpec tiles[]);
void addNodeParticle(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos, const TileSpec tiles[]);

#endif
