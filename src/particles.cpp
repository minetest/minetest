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

#include "particles.h"
#include "constants.h"
#include "debug.h"
#include "main.h" // For g_profiler and g_settings
#include "settings.h"
#include "tile.h"
#include "gamedef.h"
#include "collision.h"
#include <stdlib.h>
#include "util/numeric.h"
#include "light.h"
#include "environment.h"
#include "clientmap.h"
#include "mapnode.h"

Particle::Particle(
	IGameDef *gamedef,
	scene::ISceneManager* smgr,
	LocalPlayer *player,
	s32 id,
	v3f pos,
	v3f velocity,
	v3f acceleration,
	float expirationtime,
	float size,
	AtlasPointer ap
):
	scene::ISceneNode(smgr->getRootSceneNode(), smgr, id)
{
	// Misc
	m_gamedef = gamedef;

	// Texture
	m_material.setFlag(video::EMF_LIGHTING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, true);
	m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	m_material.setTexture(0, ap.atlas);
	m_ap = ap;
	m_light = 0;


	// Particle related
	m_pos = pos;
	m_velocity = velocity;
	m_acceleration = acceleration;
	m_expiration = expirationtime;
	m_time = 0;
	m_player = player;
	m_size = size;

	// Irrlicht stuff (TODO)
	m_collisionbox = core::aabbox3d<f32>(-size/2,-size/2,-size/2,size/2,size/2,size/2);
	this->setAutomaticCulling(scene::EAC_OFF);
}

Particle::~Particle()
{
}

void Particle::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT);
		SceneManager->registerNodeForRendering(this, scene::ESNRP_SOLID);
	}

	ISceneNode::OnRegisterSceneNode();
}

void Particle::render()
{
	// TODO: Render particles in front of water and the selectionbox

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	driver->setMaterial(m_material);
	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
	video::SColor c(255, m_light, m_light, m_light);

	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(-m_size/2,-m_size/2,0, 0,0,0, c, m_ap.x0(), m_ap.y1()),
		video::S3DVertex(m_size/2,-m_size/2,0, 0,0,0, c, m_ap.x1(), m_ap.y1()),
		video::S3DVertex(m_size/2,m_size/2,0, 0,0,0, c, m_ap.x1(), m_ap.y0()),
		video::S3DVertex(-m_size/2,m_size/2,0, 0,0,0, c ,m_ap.x0(), m_ap.y0()),
	};

	for(u16 i=0; i<4; i++)
	{
		vertices[i].Pos.rotateYZBy(m_player->getPitch());
		vertices[i].Pos.rotateXZBy(m_player->getYaw());
		m_box.addInternalPoint(vertices[i].Pos);
		vertices[i].Pos += m_pos*BS;
	}

	u16 indices[] = {0,1,2, 2,3,0};
	driver->drawVertexPrimitiveList(vertices, 4, indices, 2,
			video::EVT_STANDARD, scene::EPT_TRIANGLES, video::EIT_16BIT);
}

void Particle::step(float dtime, ClientEnvironment &env)
{
	core::aabbox3d<f32> box = m_collisionbox;
	v3f p_pos = m_pos*BS;
	v3f p_velocity = m_velocity*BS;
	v3f p_acceleration = m_acceleration*BS;
	collisionMoveSimple(&env.getClientMap(), m_gamedef,
		BS*0.5, box,
		0, dtime,
		p_pos, p_velocity, p_acceleration);
	m_pos = p_pos/BS;
	m_velocity = p_velocity/BS;
	m_acceleration = p_acceleration/BS;
	m_time += dtime;

	// Update lighting
	u8 light = 0;
	try{
		v3s16 p = v3s16(
			floor(m_pos.X+0.5),
			floor(m_pos.Y+0.5),
			floor(m_pos.Z+0.5)
		);
		MapNode n = env.getClientMap().getNode(p);
		light = n.getLightBlend(env.getDayNightRatio(), m_gamedef->ndef());
	}
	catch(InvalidPositionException &e){
		light = blend_light(env.getDayNightRatio(), LIGHT_SUN, 0);
	}
	m_light = decode_light(light);
}

std::vector<Particle*> all_particles;

void allparticles_step (float dtime, ClientEnvironment &env)
{
	for(std::vector<Particle*>::iterator i = all_particles.begin(); i != all_particles.end();)
	{
		if ((*i)->get_expired())
		{
			(*i)->remove();
			delete *i;
			all_particles.erase(i);
		}
		else
		{
			(*i)->step(dtime, env);
			i++;
		}
	}
}

void addDiggingParticles(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
	for (u16 j = 0; j < 32; j++) // set the amount of particles here
	{
		addNodeParticle(gamedef, smgr, player, pos, tiles);
	}
}

void addPunchingParticles(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
	addNodeParticle(gamedef, smgr, player, pos, tiles);
}

// add a particle of a node
// used by digging and punching particles
void addNodeParticle(IGameDef* gamedef, scene::ISceneManager* smgr, LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
	// Texture
	u8 texid = myrand_range(0,5);
	AtlasPointer ap = tiles[texid].texture;
	float size = rand()%64/512.;
	float visual_size = BS*size;
	float texsize = size*2;

	float x1 = ap.x1();
	float y1 = ap.y1();

	ap.size.X = (ap.x1() - ap.x0()) * texsize;
	ap.size.Y = (ap.x1() - ap.x0()) * texsize;

	ap.pos.X = ap.x0() + (x1 - ap.x0()) * ((rand()%64)/64.-texsize);
	ap.pos.Y = ap.y0() + (y1 - ap.y0()) * ((rand()%64)/64.-texsize);

	// Physics
	v3f velocity((rand()%100/50.-1)/1.5, rand()%100/35., (rand()%100/50.-1)/1.5);
	v3f acceleration(0,-9,0);
	v3f particlepos = v3f(
		(f32)pos.X+rand()%100/200.-0.25,
		(f32)pos.Y+rand()%100/200.-0.25,
		(f32)pos.Z+rand()%100/200.-0.25
	);

	Particle *particle = new Particle(
		gamedef,
		smgr,
		player,
		0,
		particlepos,
		velocity,
		acceleration,
		rand()%100/100., // expiration time
		visual_size,
		ap);

	all_particles.push_back(particle);
}
