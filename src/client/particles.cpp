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

#include "client.h"
#include "collision.h"
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
#include <array>
#include <atomic>

static std::atomic<u64> single_particles_count(0);
static std::atomic<u64> particle_spawners_count(0);

static v3f random_v3f(v3f min, v3f max)
{
	return v3f(rand() / (float) RAND_MAX * (max.X - min.X) + min.X,
		rand() / (float) RAND_MAX * (max.Y - min.Y) + min.Y,
		rand() / (float) RAND_MAX * (max.Z - min.Z) + min.Z);
}

static irr::core::matrix4 bottomUpTextureMatrix(float scale_x, float scale_y,
	float coord_x, float coord_y)
{
	// Transposed 3x3 2D-transformation matrix padded to 4x4
	irr::core::matrix4 texture_matrix(irr::core::matrix4::EM4CONST_IDENTITY);
	// Rotation and scaling
	texture_matrix[0] = -scale_x;
	texture_matrix[5] = -scale_y;
	// Translation
	texture_matrix[8] = scale_x + coord_x;
	texture_matrix[9] = scale_y + coord_y;
	return texture_matrix;
}

class CameraOffsetAffector : public irr::scene::IParticleAffector
{
public:
	CameraOffsetAffector(const ClientEnvironment *env,
		irr::scene::IParticleSystemSceneNode *ps, const v3f &position) :
		m_env(env), ps(ps), position(position)
	{
	}

	void affect(u32 now, irr::scene::SParticle *particlearray, u32 count) override
	{
		v3s16 camera_offset = m_env->getCameraOffset();
		ps->setPosition(position - intToFloat(camera_offset, BS));
	}

	irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const override
	{
		return irr::scene::EPAT_NONE;
	}
private:
	const ClientEnvironment *m_env;
	irr::scene::IParticleSystemSceneNode *ps;
	v3f position;
};

class LightingAffector : public irr::scene::IParticleAffector
{
public:
	LightingAffector(ClientEnvironment *env, IGameDef *gamedef,
		u8 glow, const video::SColor &base_color = video::SColor(0xFFFFFFFF))
		: m_env(env), m_gamedef(gamedef), glow(glow),
		base_color(base_color)
	{
	}

	void affect(u32 now, irr::scene::SParticle *particlearray, u32 count) override
	{
		for (u32 i = 0; i < count; ++i) {
			v3f pos = particlearray[i].pos +
				intToFloat(m_env->getCameraOffset(), BS);
			color = getParticleLightColor(pos / BS);
			particlearray[i].color = color;
		}
	}

	irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const override
	{
		return irr::scene::EPAT_NONE;
	}
private:
	ClientEnvironment *m_env;
	IGameDef *m_gamedef;
	u8 glow;
	video::SColor base_color;
	video::SColor color;

	video::SColor getParticleLightColor(const v3f &position)
	{
		u8 light = 0;
		bool pos_ok = true;

		v3s16 p = v3s16(
			floor(position.X + 0.5f),
			floor(position.Y + 0.5f),
			floor(position.Z + 0.5f));

		MapNode map_node = m_env->getClientMap().getNodeNoEx(p, &pos_ok);

		if (pos_ok)
			light = map_node.getLightBlend(m_env->getDayNightRatio(), m_gamedef->ndef());
		else
			light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);

		u8 m_light = decode_light(light + glow);
		return video::SColor (255,
			m_light * base_color.getRed() / 255,
			m_light * base_color.getGreen() / 255,
			m_light * base_color.getBlue() / 255);
	}
};

class AnimationAffector : public irr::scene::IParticleAffector
{
public:
	AnimationAffector (irr::scene::IParticleSystemSceneNode *ps,
		const struct TileAnimationParams &anim)
		: anim(anim), texture_matrix(&ps->getMaterial(0).getTextureMatrix(0)),
		texsize(ps->getMaterial(0).getTexture(0)->getSize())
	{
		v2u32 framesize;
		anim.determineParams(ps->getMaterial(0).getTexture(0)->getSize(),
			&frame_count, &frame_length_i, &framesize);
		framesize_ratio = v2f(framesize.X / (float) texsize.X,
			framesize.Y / (float) texsize.Y);
	}

	void affect(u32 now, irr::scene::SParticle *particlearray, u32 count) override
	{
		if (last_time == 0) {
			last_time = now;
			return;
		}

		animation_time += (now - last_time) * 1e-3f;
		last_time = now;
		float frame_length = frame_length_i * 1e-3f;

		animation_frame += (int) (animation_time / frame_length);
		animation_frame %= frame_count;
		animation_time = fmodf(animation_time, frame_length);

		v2f texcoord = anim.getTextureCoords(texsize, animation_frame);
		*texture_matrix = bottomUpTextureMatrix(framesize_ratio.X, framesize_ratio.Y, texcoord.X, texcoord.Y);
	}

	irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const override
	{
		return irr::scene::EPAT_NONE;
	}
private:
	const struct TileAnimationParams anim;
	float animation_time = 0.f;
	int animation_frame = 0;
	int frame_length_i, frame_count;
	core::matrix4 *texture_matrix;
	const v2u32 texsize;
	v2f framesize_ratio;
	u32 last_time = 0;
};

// Should be added before AccelerationAffector to make behaviour in case of
// collision consistent with the one for no collision
class CollisionAffector : public irr::scene::IParticleAffector
{
public:
	CollisionAffector(ClientEnvironment *env, bool collision_removal, bool object_collision,
			f32 bounce_fraction, f32 bounce_threshold)
		: m_env(env), last_time(0), collision_removal(collision_removal),
		object_collision(object_collision), bounce_fraction(bounce_fraction),
		bounce_threshold(bounce_threshold)
	{
	}

	void affect(u32 now, irr::scene::SParticle *particlearray, u32 count) override
	{
		if (last_time == 0) {
			last_time = now;
			return;
		}
		f32 dtime = (now - last_time) * 1e-3f;
		last_time = now;

		for (u32 i = 0; i < count; ++i) {
			irr::scene::SParticle &p = particlearray[i];
			float half_width = p.size.Width / 2;
			v3f camera_offset = intToFloat(m_env->getCameraOffset(), BS);

			// Undo Irrlicht position step
			v3f position = p.pos + camera_offset - dtime * p.vector * 1e3;;
			v3f velocity = p.vector * 1e3f;

			collisionMoveResult r = collisionMovePoint(m_env, m_env->getGameDef(),
					half_width, dtime, &position, &velocity, v3f {0.f, 0.f, 0.f},
					nullptr, bounce_fraction, bounce_threshold, object_collision);

			if (r.collides) {
				if (collision_removal) {
					p.endTime = 0;
				} else {
					p.pos = position - camera_offset;
					p.vector = velocity * 1e-3f;
				}
			}
		}
	}

	irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const override
	{
		return irr::scene::EPAT_NONE;
	}
private:
	ClientEnvironment *m_env;
	u32 last_time;
	const bool collision_removal;
	const bool object_collision;
	const f32 bounce_fraction;
	const f32 bounce_threshold;
};

class AccelerationAffector : public irr::scene::IParticleAffector
{
public:
	AccelerationAffector(const v3f &minacc, const v3f &maxacc)
		: last_time(0)
	{
		disabled = minacc.getLengthSQ() <= 1e-6 && maxacc.getLengthSQ() <= 1e-6;
		acc = random_v3f(minacc,maxacc);
	}
	AccelerationAffector(const v3f &acc)
		: acc(acc), disabled(acc.getLengthSQ() == 0.f), last_time(0)
	{
	}

	void affect(u32 now, irr::scene::SParticle *particlearray, u32 count) override
	{
		if (last_time == 0) {
			last_time = now;
			return;
		}
		float dtime = (now - last_time) * 1e-3f;
		last_time = now;
		for (u32 i = 0; i < count; ++i) {
			particlearray[i].vector += dtime * acc * BS * 1e-3f;
		}
	}

	irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const override
	{
		return irr::scene::EPAT_NONE;
	}
private:
	v3f acc;
	bool disabled;
	u32 last_time;
};


class CustomEmitter : public irr::scene::IParticleEmitter
{
public:
	// unused pure virtual methods
	void setDirection(const core::vector3df& /* newDirection */) override {}
	void setMinParticlesPerSecond(u32 /* minPPS */) override {}
	void setMaxParticlesPerSecond(u32 /* maxPPS */) override {}
	void setMinStartColor(const video::SColor& /* color */) override {}
	void setMaxStartColor(const video::SColor& /* color */) override {}
	void setMaxLifeTime(const u32 /* t */) override {}
	void setMinLifeTime(const u32 /* t */) override {}
	u32 getMaxLifeTime() const override {return 1;}
	u32 getMinLifeTime() const override {return 1;}
	void setMaxAngleDegrees(const s32 /* t */) override {}
	s32 getMaxAngleDegrees() const override {return 0;}
	void setMaxStartSize(const core::dimension2df& /* size */) override {}
	void setMinStartSize(const core::dimension2df& /* size */) override {}
	//void setCenter(const core::vector3df& /* center */) override {}
	//void setRadius(f32 /* radius */) override {}
	const core::vector3df& getDirection() const override {return direction;}
	u32 getMinParticlesPerSecond() const override {return 1;}
	u32 getMaxParticlesPerSecond() const override {return 1;}
	const video::SColor& getMinStartColor() const override {return minStartColor;}
	const video::SColor& getMaxStartColor() const override {return maxStartColor;}
	const core::dimension2df& getMaxStartSize() const override {return max_size;}
	const core::dimension2df& getMinStartSize() const override {return min_size;}
	irr::scene::E_PARTICLE_EMITTER_TYPE getType() const override
	{
		return irr::scene::EPET_COUNT;
	}
protected:
	core::dimension2df min_size, max_size;
private:
	// unused
	const v3f direction;
	const video::SColor minStartColor, maxStartColor;
	const v3f center;
};

class SingleEmitter : public CustomEmitter
{
public:
	SingleEmitter(irr::scene::ISceneManager *smgr,
		irr::scene::IParticleSystemSceneNode *ps, const video::SColor &color, u32 number)
		: m_smgr(smgr), ps(ps), number(number), deletion_time(0), color(color),
		random_properties(true)
	{
		particles.reserve(number);
		++single_particles_count;
	}

	SingleEmitter(irr::scene::ISceneManager *smgr,
		irr::scene::IParticleSystemSceneNode *ps, const video::SColor &color,
		f32 expirationtime, f32 size, const v3f &vel):
		m_smgr(smgr), ps(ps), number(1), deletion_time(0), color(color),
		expirationtime((u32) (expirationtime * 1e3f)), size(size), pos(0), vel(vel),
		random_properties(false)
	{
		++single_particles_count;
	}

	~SingleEmitter()
	{
		--single_particles_count;
	}

	s32 emitt(u32 now, u32 timeSinceLastCall, irr::scene::SParticle *&outArray) override
	{
		if (number == 0) {
			if (now > deletion_time)
				m_smgr->addToDeletionQueue(ps);
			return 0;
		}
		if (random_properties)
			deletion_time = now + 1000;
		else
			deletion_time = now + expirationtime;

		irr::scene::SParticle p;
		p.color = color;
		p.startColor = p.color;
		for (u32 i = 0; i < number; ++i) {
			if (random_properties) {
				pos = {
					rand() % 100 / 200.f - 0.25f,
					rand() % 100 / 200.f - 0.25f,
					rand() % 100 / 200.f - 0.25f
				};
				vel = {
					rand() % 150 / 50.f - 1.5f,
					rand() % 150 / 50.f,
					rand() % 150 / 50.f - 1.5f
				};
				size = rand() % 8 / 64.f * BS;
			}
			p.pos = pos * BS;
			p.startVector = p.vector = vel * BS * 1e-3f;

			p.startTime = now;
			if (random_properties)
				p.endTime = now + (u32) (rand() % 100 * 10.f);
			else
				p.endTime = now + (u32) (expirationtime * 1e3f);

			p.startSize = core::dimension2df(size,size);
			p.size = p.startSize;
			particles.push_back(p);
		}
		outArray = particles.data();
		number = 0;
		return particles.size();
	}
private:
	irr::scene::ISceneManager *m_smgr;
	irr::scene::IParticleSystemSceneNode *ps;
	u32 number;
	std::vector<irr::scene::SParticle> particles;
	u32 deletion_time;
	video::SColor color;
	u32 expirationtime;
	f32 size;
	v3f pos, vel;
	const bool random_properties;
};

class ContinuousEmitter : public CustomEmitter
{
public:
	ContinuousEmitter(irr::scene::ISceneManager *smgr, ParticleManager *pmgr, u64 id,
		ClientEnvironment *env, irr::scene::IParticleSystemSceneNode *ps,
		u16 amount, f32 spawntime, const v3f &minrelpos, const v3f &maxrelpos,
		const v3f &minvel, const v3f &maxvel, f32 minexptime,
		f32 maxexptime, f32 minsize, f32 maxsize, u16 attached_id)
		: m_smgr(smgr), m_pmgr(pmgr), id(id), m_env(env), ps(ps), remaining_amount(amount),
		spawntime(spawntime), minrelpos(minrelpos), maxrelpos(maxrelpos),
		minvel(minvel), maxvel(maxvel), minexptime(minexptime),
		maxexptime(maxexptime), minsize(minsize), maxsize(maxsize),
		attached_id(attached_id), expired_time(0), stopped(false), emitting_time(0.f)
	{
		time_for_particle = spawntime == 0 ? 1.f / (float) amount :
			spawntime / (float) amount;
		++particle_spawners_count;
	}

	~ContinuousEmitter()
	{
		--particle_spawners_count;
	}

	s32 emitt(u32 now, u32 timeSinceLastCall, irr::scene::SParticle *&outArray) override
	{
		if (spawntime != 0) {
			if (stopped && expired_time * 1e-3f < spawntime)
				expired_time = spawntime;
			expired_time += timeSinceLastCall;
			if (expired_time * 1e-3f >= maxexptime + spawntime) {
				if (!stopped)
					m_pmgr->removeParticleSpawner(id, false);
				m_smgr->addToDeletionQueue(ps);
			}

			// Don't delete yet because of the existing particles
			if (stopped || remaining_amount == 0)
				return 0;
		} else {
			if (stopped) {
				expired_time += timeSinceLastCall;
				if (expired_time * 1e-3f >= maxexptime)
					m_smgr->addToDeletionQueue(ps);
				return 0;
			}
		}

		emitting_time += timeSinceLastCall * 1e-3f;
		if (emitting_time >= time_for_particle) {
			emitting_time = 0.f;
			p.color = video::SColor(0xFFFFFFFF);
			p.startColor = p.color;

			p.pos = random_v3f(minrelpos, maxrelpos);
			v3f vel = random_v3f(minvel, maxvel);
			if (attached_id != 0) {
				ClientActiveObject *attached =
					m_env->getActiveObject(attached_id);
				if (attached) {
					vel.rotateXZBy(attached->getYaw());
					p.pos.rotateXZBy(attached->getYaw());
					p.pos += attached->getPosition();
				} else {
					return 0;
				}
			}

			// Velocity per millisecond
			p.startVector = p.vector = random_v3f(minvel, maxvel) * BS * 1e-3f;
			float size = minsize + (float) rand() / (float) RAND_MAX *
				(maxsize - minsize);
			p.size = core::dimension2df(size,size);

			p.startTime = now;
			p.endTime = now + (u32) ((minexptime +
				(float) rand() / (float) RAND_MAX *
				(maxexptime - minexptime)) * 1e3f);

			if (remaining_amount > 0)
				--remaining_amount;
			outArray = &p;
			return 1;
		}
		return 0;
	}

	void stop()
	{
		stopped = true;
	}
private:
	irr::scene::ISceneManager *m_smgr;
	ParticleManager *m_pmgr;
	u64 id;
	ClientEnvironment *m_env;
	irr::scene::IParticleSystemSceneNode *ps;
	u16 remaining_amount;
	f32 spawntime;
	const v3f minrelpos, maxrelpos, minvel, maxvel;
	const f32 minexptime, maxexptime;
	const f32 minsize, maxsize;
	const u16 attached_id;
	u32 expired_time;
	bool stopped;
	float emitting_time;
	float time_for_particle;
	irr::scene::SParticle p;
};


ParticleManager::ParticleManager(ClientEnvironment *env)
	: m_env(env)
{
	m_smgr = RenderingEngine::get_scene_manager();
}

ParticleManager::~ParticleManager()
{
}

void ParticleManager::handleParticleEvent(ClientEvent *event, Client *client, LocalPlayer *player)
{
	switch (event->type) {
	case CE_DELETE_PARTICLESPAWNER: {
		removeParticleSpawner(event->delete_particlespawner.id, true);
		// No allocated memory in delete event
		break;
	}
	case CE_ADD_PARTICLESPAWNER: {
		removeParticleSpawner(event->add_particlespawner.id, true);

		scene::IParticleSystemSceneNode *ps =
			m_smgr->addParticleSystemSceneNode(false,
				RenderingEngine::get_scene_manager()->getRootSceneNode());

		{
			MutexAutoLock lock(m_spawner_list_lock);
			m_particle_spawners.insert({event->add_particlespawner.id, ps});
		}

		ps->getMaterial(0).getTextureMatrix(0) = bottomUpTextureMatrix(1.f,1.f,0.f,0.f);

		v3f minpos = *event->add_particlespawner.minpos * BS;
		v3f maxpos = *event->add_particlespawner.maxpos * BS;
		v3f meanpos = (minpos + maxpos) / 2.f;
		ps->setPosition(meanpos);

		scene::IParticlePointEmitter *continous_emitter =
			new ContinuousEmitter(m_smgr, this,
			event->add_particlespawner.id,
			m_env, ps,
			event->add_particlespawner.amount,
			event->add_particlespawner.spawntime,
			minpos - meanpos,
			maxpos - meanpos,
			*event->add_particlespawner.minvel,
			*event->add_particlespawner.maxvel,
			event->add_particlespawner.minexptime,
			event->add_particlespawner.maxexptime,
			event->add_particlespawner.minsize,
			event->add_particlespawner.maxsize,
			event->add_particlespawner.attached_id);
		ps->setEmitter(continous_emitter);
		continous_emitter->drop();
		ps->setAutomaticCulling(scene::EAC_FRUSTUM_BOX);

		scene::IParticleAffector *camera_offset_affector =
			new CameraOffsetAffector(m_env, ps,meanpos);
		ps->addAffector(camera_offset_affector);
		camera_offset_affector->drop();

		video::ITexture *texture = client->tsrc()->getTextureForMesh(
			*(event->add_particlespawner.texture));
		ps->setMaterialTexture(0, texture);

		ps->setMaterialFlag(video::EMF_LIGHTING, false);
		ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
		ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);

		const struct TileAnimationParams &anim = event->add_particlespawner.animation;
		if (anim.type != TAT_NONE) {
			scene::IParticleAffector *animation_affector =
				new AnimationAffector(ps, anim);
			ps->addAffector(animation_affector);
			animation_affector->drop();
		}

		if (event->add_particlespawner.collisiondetection) {
			scene::IParticleAffector *collision_affector =
				new CollisionAffector(m_env,
					event->add_particlespawner.collision_removal,
					event->add_particlespawner.object_collision,
					event->add_particlespawner.bounce_fraction,
					event->add_particlespawner.bounce_threshold);
			ps->addAffector(collision_affector);
			collision_affector->drop();
		}

		scene::IParticleAffector *acceleration_affector =
			new AccelerationAffector(*event->add_particlespawner.minacc,
				*event->add_particlespawner.maxacc);
		ps->addAffector(acceleration_affector);
		acceleration_affector->drop();

		scene::IParticleAffector *lighting_affector =
			new LightingAffector(m_env, client, 0);
		ps->addAffector(lighting_affector);
		lighting_affector->drop();

		// Delete allocated content of event
		delete event->add_particlespawner.minpos;
		delete event->add_particlespawner.maxpos;
		delete event->add_particlespawner.minvel;
		delete event->add_particlespawner.maxvel;
		delete event->add_particlespawner.minacc;
		delete event->add_particlespawner.maxacc;
		delete event->add_particlespawner.texture;
		break;
	}
	case CE_SPAWN_PARTICLE: {
		scene::IParticleSystemSceneNode *ps =
			m_smgr->addParticleSystemSceneNode(false,
				RenderingEngine::get_scene_manager()->getRootSceneNode());

		ps->getMaterial(0).getTextureMatrix(0) = bottomUpTextureMatrix(1.f,1.f,0.f,0.f);

		video::ITexture *texture =
			client->tsrc()->getTextureForMesh(
				*(event->spawn_particle.texture));
		ps->setMaterialTexture(0, texture);

		v3s16 camera_offset = m_env->getCameraOffset();
		v3f particle_pos = *event->spawn_particle.pos * BS - intToFloat(camera_offset, BS);
		ps->setPosition(particle_pos);

		scene::IParticleAffector *camera_offset_affector =
			new CameraOffsetAffector(m_env, ps, particle_pos);
		ps->addAffector(camera_offset_affector);
		camera_offset_affector->drop();

		// Deletes ps after the particles have vanished
		scene::IParticleEmitter *em = new SingleEmitter(m_smgr, ps,
			event->spawn_particle.glow,
			event->spawn_particle.expirationtime,
			event->spawn_particle.size,
			*event->spawn_particle.vel);
		ps->setEmitter(em);
		em->drop();

		const struct TileAnimationParams &anim = event->spawn_particle.animation;
		if (anim.type != TAT_NONE) {
			scene::IParticleAffector *animation_affector =
				new AnimationAffector(ps, anim);
			ps->addAffector(animation_affector);
			animation_affector->drop();
		}

		if (event->spawn_particle.collisiondetection){
			scene::IParticleAffector *collision_affector =
				new CollisionAffector(m_env,
				event->spawn_particle.collision_removal,
				event->spawn_particle.object_collision,
				event->spawn_particle.bounce_fraction,
				event->spawn_particle.bounce_threshold);
			ps->addAffector(collision_affector);
			collision_affector->drop();
		}

		scene::IParticleAffector *acceleration_affector =
			new AccelerationAffector(*event->spawn_particle.acc);
		ps->addAffector(acceleration_affector);
		acceleration_affector->drop();

		scene::IParticleAffector *lighting_affector =
			new LightingAffector(m_env, client, 0);
		ps->addAffector(lighting_affector);
		lighting_affector->drop();

		ps->setMaterialFlag(video::EMF_LIGHTING, false);
		ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
		ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);
		ps->setAutomaticCulling(scene::EAC_OFF);

		// Delete allocated content of event
		delete event->spawn_particle.pos;
		delete event->spawn_particle.vel;
		delete event->spawn_particle.acc;
		delete event->spawn_particle.texture;
		break;
	}
	default:
		break;
	}
}

// The final burst of particles when a node is finally dug, *not* particles
// spawned during the digging of a node.
void ParticleManager::addDiggingParticles(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
	const MapNode &n, const ContentFeatures &f)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return;

	addNodeParticle(gamedef, player, pos, n, f, 16);
}

// During the digging of a node particles are spawned individually by this
// function, called from Game::handleDigging() in game.cpp.
void ParticleManager::addNodeParticle(IGameDef *gamedef, LocalPlayer *player, v3s16 pos,
	const MapNode &n, const ContentFeatures &f, u32 number)
{
	// No particles for "airlike" nodes
	if (f.drawtype == NDT_AIRLIKE)
		return;

	scene::IParticleSystemSceneNode* ps =
		m_smgr->addParticleSystemSceneNode(false);

	v3s16 camera_offset = m_env->getCameraOffset();
	v3f particle_pos = intToFloat(pos - camera_offset, BS);
	ps->setPosition(particle_pos);

	// Texture
	u8 texid = myrand_range(0, 5);
	const TileLayer &tile = f.tiles[texid].layers[0];
	video::ITexture *texture;

	// Only use first frame of animated texture
	if (tile.material_flags & MATERIAL_FLAG_ANIMATION)
		texture = (*tile.frames)[0].texture;
	else
		texture = tile.texture;

	video::SColor color;
	if (tile.has_color)
		color = tile.color;
	else
		n.getColor(f, &color);

	// Deletes ps after the particles have vanished
	scene::IParticleEmitter *em = new SingleEmitter(m_smgr,ps,color,number);

	ps->setEmitter(em);
	em->drop();

	v3f acceleration(
		0.f,
		-player->movement_gravity * player->physics_override_gravity / BS,
		0.f
	);

	scene::IParticleAffector *collision_affector =
		new CollisionAffector(m_env, false, true, 0.3f, 0.2f);
	ps->addAffector(collision_affector);
	collision_affector->drop();

	scene::IParticleAffector *acceleration_affector =
		new AccelerationAffector(acceleration);
	ps->addAffector(acceleration_affector);
	acceleration_affector->drop();

	scene::IParticleAffector *lighting_affector =
		new LightingAffector(m_env, gamedef, 0);
	ps->addAffector(lighting_affector);
	lighting_affector->drop();

	ps->setMaterialTexture(0, texture);

	ps->setMaterialFlag(video::EMF_LIGHTING, false);
	ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
	ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
	ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);

	// Take a square (quarter the size of the texture) at a random position
	float tile_scale = tile.scale ? (float) tile.scale : 1.f;
	float scale_factor = 0.25 / tile_scale;
	v2f texpos;
	texpos.X = (rand() % 48) / 64.f;
	texpos.Y = (rand() % 48) / 64.f;

	ps->getMaterial(0).getTextureMatrix(0) = bottomUpTextureMatrix(scale_factor,
		scale_factor, texpos.X, texpos.Y);
}

u64 ParticleManager::getSingleParticleNumber()
{
	return single_particles_count.load();
}

u64 ParticleManager::getParticleSpawnerNumber()
{
	return particle_spawners_count.load();
}

void ParticleManager::removeParticleSpawner(u64 id, bool stop)
{
	MutexAutoLock lock(m_spawner_list_lock);
	auto node_it = m_particle_spawners.find(id);
	if (node_it != m_particle_spawners.end()) {
		if (stop) {
			auto *em = static_cast<ContinuousEmitter*>(node_it->second->getEmitter());
			em->stop();
		}
		m_particle_spawners.erase(node_it);
	}
}
