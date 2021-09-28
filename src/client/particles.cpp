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
#include <cmath>
#include "client.h"
#include "collision.h"
#include "client/content_cao.h"
#include "client/clientevent.h"
#include "client/renderingengine.h"
#include "util/numeric.h"
#include "light.h"
#include "environment.h"
#include "clientmap.h"
#include "mapnode.h"
#include "nodedef.h"
#include "client.h"
#include "settings.h"

/*
	Utility
*/

static f32 random_f32(f32 min, f32 max)
{
	return rand() / (float)RAND_MAX * (max - min) + min;
}

static v3f random_v3f(v3f min, v3f max)
{
	return v3f(
		random_f32(min.X, max.X),
		random_f32(min.Y, max.Y),
		random_f32(min.Z, max.Z));
}

/*
	Particle
*/

Particle::Particle(
	IGameDef *gamedef,
	LocalPlayer *player,
	ClientEnvironment *env,
	const ParticleParameters &p,
	video::ITexture *texture,
	v2f texpos,
	v2f texsize,
	video::SColor color
):
	scene::ISceneNode(((Client *)gamedef)->getSceneManager()->getRootSceneNode(),
		((Client *)gamedef)->getSceneManager())
{
	// Misc
	m_gamedef = gamedef;
	m_env = env;

	// Texture
	m_material.setFlag(video::EMF_LIGHTING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, true);
	m_material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	m_material.setTexture(0, texture);
	m_texpos = texpos;
	m_texsize = texsize;
	m_animation = p.animation;

	// Color
	m_base_color = color;
	m_color = color;

	// Particle related
	m_pos = p.pos;
	m_velocity = p.vel;
	m_acceleration = p.acc;
	m_expiration = p.expirationtime;
	m_player = player;
	m_size = p.size;
	m_collisiondetection = p.collisiondetection;
	m_collision_removal = p.collision_removal;
	m_object_collision = p.object_collision;
	m_vertical = p.vertical;
	m_glow = p.glow;

	// Irrlicht stuff
	const float c = p.size / 2;
	m_collisionbox = aabb3f(-c, -c, -c, c, c, c);
	this->setAutomaticCulling(scene::EAC_OFF);

	// Init lighting
	updateLight();

	// Init model
	updateVertices();
}

void Particle::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, scene::ESNRP_TRANSPARENT_EFFECT);

	ISceneNode::OnRegisterSceneNode();
}

void Particle::render()
{
	video::IVideoDriver *driver = SceneManager->getVideoDriver();
	driver->setMaterial(m_material);
	driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);

	u16 indices[] = {0,1,2, 2,3,0};
	driver->drawVertexPrimitiveList(m_vertices, 4,
			indices, 2, video::EVT_STANDARD,
			scene::EPT_TRIANGLES, video::EIT_16BIT);
}

void Particle::step(float dtime)
{
	m_time += dtime;
	if (m_collisiondetection) {
		aabb3f box = m_collisionbox;
		v3f p_pos = m_pos * BS;
		v3f p_velocity = m_velocity * BS;
		collisionMoveResult r = collisionMoveSimple(m_env, m_gamedef, BS * 0.5f,
			box, 0.0f, dtime, &p_pos, &p_velocity, m_acceleration * BS, nullptr,
			m_object_collision);
		if (m_collision_removal && r.collides) {
			// force expiration of the particle
			m_expiration = -1.0;
		} else {
			m_pos = p_pos / BS;
			m_velocity = p_velocity / BS;
		}
	} else {
		m_velocity += m_acceleration * dtime;
		m_pos += m_velocity * dtime;
	}
	if (m_animation.type != TAT_NONE) {
		m_animation_time += dtime;
		int frame_length_i, frame_count;
		m_animation.determineParams(
				m_material.getTexture(0)->getSize(),
				&frame_count, &frame_length_i, NULL);
		float frame_length = frame_length_i / 1000.0;
		while (m_animation_time > frame_length) {
			m_animation_frame++;
			m_animation_time -= frame_length;
		}
	}

	// Update lighting
	updateLight();

	// Update model
	updateVertices();
}

void Particle::updateLight()
{
	u8 light = 0;
	bool pos_ok;

	v3s16 p = v3s16(
		floor(m_pos.X+0.5),
		floor(m_pos.Y+0.5),
		floor(m_pos.Z+0.5)
	);
	MapNode n = m_env->getClientMap().getNode(p, &pos_ok);
	if (pos_ok)
		light = n.getLightBlend(m_env->getDayNightRatio(), m_gamedef->ndef());
	else
		light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);

	u8 m_light = decode_light(light + m_glow);
	m_color.set(255,
		m_light * m_base_color.getRed() / 255,
		m_light * m_base_color.getGreen() / 255,
		m_light * m_base_color.getBlue() / 255);
}

void Particle::updateVertices()
{
	f32 tx0, tx1, ty0, ty1;

	if (m_animation.type != TAT_NONE) {
		const v2u32 texsize = m_material.getTexture(0)->getSize();
		v2f texcoord, framesize_f;
		v2u32 framesize;
		texcoord = m_animation.getTextureCoords(texsize, m_animation_frame);
		m_animation.determineParams(texsize, NULL, NULL, &framesize);
		framesize_f = v2f(framesize.X / (float) texsize.X, framesize.Y / (float) texsize.Y);

		tx0 = m_texpos.X + texcoord.X;
		tx1 = m_texpos.X + texcoord.X + framesize_f.X * m_texsize.X;
		ty0 = m_texpos.Y + texcoord.Y;
		ty1 = m_texpos.Y + texcoord.Y + framesize_f.Y * m_texsize.Y;
	} else {
		tx0 = m_texpos.X;
		tx1 = m_texpos.X + m_texsize.X;
		ty0 = m_texpos.Y;
		ty1 = m_texpos.Y + m_texsize.Y;
	}

	m_vertices[0] = video::S3DVertex(-m_size / 2, -m_size / 2,
		0, 0, 0, 0, m_color, tx0, ty1);
	m_vertices[1] = video::S3DVertex(m_size / 2, -m_size / 2,
		0, 0, 0, 0, m_color, tx1, ty1);
	m_vertices[2] = video::S3DVertex(m_size / 2, m_size / 2,
		0, 0, 0, 0, m_color, tx1, ty0);
	m_vertices[3] = video::S3DVertex(-m_size / 2, m_size / 2,
		0, 0, 0, 0, m_color, tx0, ty0);

	v3s16 camera_offset = m_env->getCameraOffset();
	for (video::S3DVertex &vertex : m_vertices) {
		if (m_vertical) {
			v3f ppos = m_player->getPosition()/BS;
			vertex.Pos.rotateXZBy(std::atan2(ppos.Z - m_pos.Z, ppos.X - m_pos.X) /
				core::DEGTORAD + 90);
		} else {
			vertex.Pos.rotateYZBy(m_player->getPitch());
			vertex.Pos.rotateXZBy(m_player->getYaw());
		}
		m_box.addInternalPoint(vertex.Pos);
		vertex.Pos += m_pos*BS - intToFloat(camera_offset, BS);
	}
}

/*
	ParticleSpawner
*/

ParticleSpawner::ParticleSpawner(
	IGameDef *gamedef,
	LocalPlayer *player,
	const ParticleSpawnerParameters &p,
	u16 attached_id,
	video::ITexture *texture,
	ParticleManager *p_manager
):
	m_particlemanager(p_manager), p(p)
{
	m_gamedef = gamedef;
	m_player = player;
	m_attached_id = attached_id;
	m_texture = texture;
	m_time = 0;

	m_spawntimes.reserve(p.amount + 1);
	for (u16 i = 0; i <= p.amount; i++) {
		float spawntime = rand() / (float)RAND_MAX * p.time;
		m_spawntimes.push_back(spawntime);
	}
}

void ParticleSpawner::spawnParticle(ClientEnvironment *env, float radius,
	const core::matrix4 *attached_absolute_pos_rot_matrix)
{
	v3f ppos = m_player->getPosition() / BS;
	v3f pos = random_v3f(p.minpos, p.maxpos);

	// Need to apply this first or the following check
	// will be wrong for attached spawners
	if (attached_absolute_pos_rot_matrix) {
		pos *= BS;
		attached_absolute_pos_rot_matrix->transformVect(pos);
		pos /= BS;
		v3s16 camera_offset = m_particlemanager->m_env->getCameraOffset();
		pos.X += camera_offset.X;
		pos.Y += camera_offset.Y;
		pos.Z += camera_offset.Z;
	}

	if (pos.getDistanceFrom(ppos) > radius)
		return;

	// Parameters for the single particle we're about to spawn
	ParticleParameters pp;
	pp.pos = pos;

	pp.vel = random_v3f(p.minvel, p.maxvel);
	pp.acc = random_v3f(p.minacc, p.maxacc);

	if (attached_absolute_pos_rot_matrix) {
		// Apply attachment rotation
		attached_absolute_pos_rot_matrix->rotateVect(pp.vel);
		attached_absolute_pos_rot_matrix->rotateVect(pp.acc);
	}

	pp.expirationtime = random_f32(p.minexptime, p.maxexptime);
	p.copyCommon(pp);

	video::ITexture *texture;
	v2f texpos, texsize;
	video::SColor color(0xFFFFFFFF);

	if (p.node.getContent() != CONTENT_IGNORE) {
		const ContentFeatures &f =
			m_particlemanager->m_env->getGameDef()->ndef()->get(p.node);
		if (!ParticleManager::getNodeParticleParams(p.node, f, pp, &texture,
				texpos, texsize, &color, p.node_tile))
			return;
	} else {
		texture = m_texture;
		texpos = v2f(0.0f, 0.0f);
		texsize = v2f(1.0f, 1.0f);
	}

	// Allow keeping default random size
	if (p.maxsize > 0.0f)
		pp.size = random_f32(p.minsize, p.maxsize);

	m_particlemanager->addParticle(new Particle(
		m_gamedef,
		m_player,
		env,
		pp,
		texture,
		texpos,
		texsize,
		color
	));
}

void ParticleSpawner::step(float dtime, ClientEnvironment *env)
{
	m_time += dtime;

	static thread_local const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE;

	bool unloaded = false;
	const core::matrix4 *attached_absolute_pos_rot_matrix = nullptr;
	if (m_attached_id) {
		if (GenericCAO *attached = dynamic_cast<GenericCAO *>(env->getActiveObject(m_attached_id))) {
			attached_absolute_pos_rot_matrix = attached->getAbsolutePosRotMatrix();
		} else {
			unloaded = true;
		}
	}

	if (p.time != 0) {
		// Spawner exists for a predefined timespan
		for (auto i = m_spawntimes.begin(); i != m_spawntimes.end(); ) {
			if ((*i) <= m_time && p.amount > 0) {
				--p.amount;

				// Pretend to, but don't actually spawn a particle if it is
				// attached to an unloaded object or distant from player.
				if (!unloaded)
					spawnParticle(env, radius, attached_absolute_pos_rot_matrix);

				i = m_spawntimes.erase(i);
			} else {
				++i;
			}
		}
	} else {
		// Spawner exists for an infinity timespan, spawn on a per-second base

		// Skip this step if attached to an unloaded object
		if (unloaded)
			return;

		for (int i = 0; i <= p.amount; i++) {
			if (rand() / (float)RAND_MAX < dtime)
				spawnParticle(env, radius, attached_absolute_pos_rot_matrix);
		}
	}
}

/*
	ParticleManager
*/

ParticleManager::ParticleManager(ClientEnvironment *env) :
	m_env(env)
{}

ParticleManager::~ParticleManager()
{
	clearAll();
}

void ParticleManager::step(float dtime)
{
	stepParticles (dtime);
	stepSpawners (dtime);
}

void ParticleManager::stepSpawners(float dtime)
{
	MutexAutoLock lock(m_spawner_list_lock);
	for (auto i = m_particle_spawners.begin(); i != m_particle_spawners.end();) {
		if (i->second->get_expired()) {
			delete i->second;
			m_particle_spawners.erase(i++);
		} else {
			i->second->step(dtime, m_env);
			++i;
		}
	}
}

void ParticleManager::stepParticles(float dtime)
{
	MutexAutoLock lock(m_particle_list_lock);
	for (auto i = m_particles.begin(); i != m_particles.end();) {
		if ((*i)->get_expired()) {
			(*i)->remove();
			delete *i;
			i = m_particles.erase(i);
		} else {
			(*i)->step(dtime);
			++i;
		}
	}
}

void ParticleManager::clearAll()
{
	MutexAutoLock lock(m_spawner_list_lock);
	MutexAutoLock lock2(m_particle_list_lock);
	for (auto i = m_particle_spawners.begin(); i != m_particle_spawners.end();) {
		delete i->second;
		m_particle_spawners.erase(i++);
	}

	for(auto i = m_particles.begin(); i != m_particles.end();)
	{
		(*i)->remove();
		delete *i;
		i = m_particles.erase(i);
	}
}

void ParticleManager::handleParticleEvent(ClientEvent *event, Client *client,
	LocalPlayer *player)
{
	switch (event->type) {
		case CE_DELETE_PARTICLESPAWNER: {
			deleteParticleSpawner(event->delete_particlespawner.id);
			// no allocated memory in delete event
			break;
		}
		case CE_ADD_PARTICLESPAWNER: {
			deleteParticleSpawner(event->add_particlespawner.id);

			const ParticleSpawnerParameters &p = *event->add_particlespawner.p;

			video::ITexture *texture =
				client->tsrc()->getTextureForMesh(p.texture);

			auto toadd = new ParticleSpawner(client, player,
					p,
					event->add_particlespawner.attached_id,
					texture,
					this);

			addParticleSpawner(event->add_particlespawner.id, toadd);

			delete event->add_particlespawner.p;
			break;
		}
		case CE_SPAWN_PARTICLE: {
			ParticleParameters &p = *event->spawn_particle;

			video::ITexture *texture;
			v2f texpos, texsize;
			video::SColor color(0xFFFFFFFF);

			f32 oldsize = p.size;

			if (p.node.getContent() != CONTENT_IGNORE) {
				const ContentFeatures &f = m_env->getGameDef()->ndef()->get(p.node);
				if (!getNodeParticleParams(p.node, f, p, &texture, texpos,
						texsize, &color, p.node_tile))
					texture = nullptr;
			} else {
				texture = client->tsrc()->getTextureForMesh(p.texture);
				texpos = v2f(0.0f, 0.0f);
				texsize = v2f(1.0f, 1.0f);
			}

			// Allow keeping default random size
			if (oldsize > 0.0f)
				p.size = oldsize;

			if (texture) {
				Particle *toadd = new Particle(client, player, m_env,
						p, texture, texpos, texsize, color);

				addParticle(toadd);
			}

			delete event->spawn_particle;
			break;
		}
		default: break;
	}
}

bool ParticleManager::getNodeParticleParams(const MapNode &n,
	const ContentFeatures &f, ParticleParameters &p, video::ITexture **texture,
	v2f &texpos, v2f &texsize, video::SColor *color, u8 tilenum)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return false;

	// Texture
	u8 texid;
	if (tilenum > 0 && tilenum <= 6)
		texid = tilenum - 1;
	else
		texid = rand() % 6;
	const TileLayer &tile = f.tiles[texid].layers[0];
	p.animation.type = TAT_NONE;

	// Only use first frame of animated texture
	if (tile.material_flags & MATERIAL_FLAG_ANIMATION)
		*texture = (*tile.frames)[0].texture;
	else
		*texture = tile.texture;

	float size = (rand() % 8) / 64.0f;
	p.size = BS * size;
	if (tile.scale)
		size /= tile.scale;
	texsize = v2f(size * 2.0f, size * 2.0f);
	texpos.X = (rand() % 64) / 64.0f - texsize.X;
	texpos.Y = (rand() % 64) / 64.0f - texsize.Y;

	if (tile.has_color)
		*color = tile.color;
	else
		n.getColor(f, color);

	return true;
}

// The final burst of particles when a node is finally dug, *not* particles
// spawned during the digging of a node.

void ParticleManager::addDiggingParticles(IGameDef *gamedef,
	LocalPlayer *player, v3s16 pos, const MapNode &n, const ContentFeatures &f)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return;

	for (u16 j = 0; j < 16; j++) {
		addNodeParticle(gamedef, player, pos, n, f);
	}
}

// During the digging of a node particles are spawned individually by this
// function, called from Game::handleDigging() in game.cpp.

void ParticleManager::addNodeParticle(IGameDef *gamedef,
	LocalPlayer *player, v3s16 pos, const MapNode &n, const ContentFeatures &f)
{
	ParticleParameters p;
	video::ITexture *texture;
	v2f texpos, texsize;
	video::SColor color;

	if (!getNodeParticleParams(n, f, p, &texture, texpos, texsize, &color))
		return;

	p.expirationtime = (rand() % 100) / 100.0f;

	// Physics
	p.vel = v3f(
		(rand() % 150) / 50.0f - 1.5f,
		(rand() % 150) / 50.0f,
		(rand() % 150) / 50.0f - 1.5f
	);
	p.acc = v3f(
		0.0f,
		-player->movement_gravity * player->physics_override_gravity / BS,
		0.0f
	);
	p.pos = v3f(
		(f32)pos.X + (rand() % 100) / 200.0f - 0.25f,
		(f32)pos.Y + (rand() % 100) / 200.0f - 0.25f,
		(f32)pos.Z + (rand() % 100) / 200.0f - 0.25f
	);

	Particle *toadd = new Particle(
		gamedef,
		player,
		m_env,
		p,
		texture,
		texpos,
		texsize,
		color);

	addParticle(toadd);
}

void ParticleManager::addParticle(Particle *toadd)
{
	MutexAutoLock lock(m_particle_list_lock);
	m_particles.push_back(toadd);
}


void ParticleManager::addParticleSpawner(u64 id, ParticleSpawner *toadd)
{
	MutexAutoLock lock(m_spawner_list_lock);
	m_particle_spawners[id] = toadd;
}

void ParticleManager::deleteParticleSpawner(u64 id)
{
	MutexAutoLock lock(m_spawner_list_lock);
	auto it = m_particle_spawners.find(id);
	if (it != m_particle_spawners.end()) {
		delete it->second;
		m_particle_spawners.erase(it);
	}
}
