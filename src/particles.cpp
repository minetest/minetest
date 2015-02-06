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
#include "client.h"

#include "nodedef.h"
class Map;
class IGameDef;
class Environment;

// this class is from http://www.bulletbyte.de/index.php?section=8
//#define EPET_EXPLOSION (irr::scene::E_PARTICLE_EMITTER_TYPE)(EPET_COUNT+23)
class CParticleExplosionEmitter : public irr::scene::IParticleEmitter {
        //protected:
private:
        core::array<irr::scene::SParticle> Particles;
        core::vector3df Center;
        f32 Radius;
        core::vector3df Direction;
        core::dimension2df MinStartSize, MaxStartSize;
        u32 MinParticlesPerSecond, MaxParticlesPerSecond;
        video::SColor MinStartColor, MaxStartColor;
        u32 MinLifeTime, MaxLifeTime;
        u32 Time;
        u32 Emitted;
public:
        CParticleExplosionEmitter(
                        const core::vector3df& center, f32 radius,
                        core::vector3df vDirection, u32 minParticlesPerSecond,
                        u32 maxParticlesPerSecond, const video::SColor& minStartColor,
                        const video::SColor& maxStartColor, u32 lifeTimeMin, u32 lifeTimeMax,
                        const core::dimension2df& minStartSize,
                        const core::dimension2df& maxStartSize )
                : Center(center), Radius(radius),
                  MinStartSize(minStartSize), MaxStartSize(maxStartSize),
                  MinParticlesPerSecond(minParticlesPerSecond),
                  MaxParticlesPerSecond(maxParticlesPerSecond),
                  MinStartColor(minStartColor), MaxStartColor(maxStartColor),
                  MinLifeTime(lifeTimeMin), MaxLifeTime(lifeTimeMax),
                  Time(0), Emitted(0)
        {
                Direction=vDirection;
#ifdef _DEBUG
                setDebugName("CParticleExplosionEmitter");
#endif
        }
        s32 emitt(u32 now, u32 timeSinceLastCall, irr::scene::SParticle*& outArray)
        {
                Time += timeSinceLastCall;
                if (Emitted>0) return 0;
                const u32 pps = (MaxParticlesPerSecond - MinParticlesPerSecond);
                const f32 particleCount = pps ? (f32)MinParticlesPerSecond + (rand() % pps) : MinParticlesPerSecond;
                Particles.set_used(0);
                Time = 0;
                irr::scene::SParticle p;
                for(u32 i=0; i<particleCount; ++i)
                {
                        // Random distance from center
                        f32 distance =fmodf( (f32)rand(), Radius * 1000.0f ) * 0.001f;
                        // Random direction from center
                        p.pos.set(Center + distance);
                        /**/p.pos.rotateXYBy( rand() % 360, Center );
                        p.pos.rotateYZBy( rand() % 360, Center );
                        p.pos.rotateXZBy( rand() % 360, Center );/**/
                        p.startTime = now;
                        p.vector = Direction;
                        core::vector3df tgt = Direction;
                        /**/tgt.rotateXYBy((rand()%(360*2)) - 360);
                        tgt.rotateYZBy((rand()%(360*2)) - 360);
                        tgt.rotateXZBy((rand()%(360*2)) - 360);/**/
                        p.vector = tgt;
                        if(MaxLifeTime - MinLifeTime == 0)
                                p.endTime = now + MinLifeTime;
                        else
                                p.endTime = now + MinLifeTime + (rand() % (MaxLifeTime - MinLifeTime));
                        p.color = MinStartColor.getInterpolated(
                                                MaxStartColor, (rand() % 100) / 100.0f);
                        p.startColor = p.color;
                        p.startVector = p.vector;
                        if (MinStartSize==MaxStartSize)
                                p.startSize = MinStartSize;
                        else
                                p.startSize = MinStartSize.getInterpolated(
                                                        MaxStartSize, (rand() % 100) / 100.0f);
                        p.size = p.startSize;
                        Particles.push_back(p);
                }
                outArray = Particles.pointer();
                Emitted+=Particles.size();
                // //hmm
                // if(Particles.size()==0)
                // {
                // delete this;
                // }
                return Particles.size();
        }
        //argh
        virtual void setDirection( const core::vector3df& newDirection ) { Direction = newDirection; }
        virtual void setMinParticlesPerSecond( u32 minPPS ) { MinParticlesPerSecond = minPPS; }
        virtual void setMaxParticlesPerSecond( u32 maxPPS ) { MaxParticlesPerSecond = maxPPS; }
        virtual void setMinStartColor( const video::SColor& color ) { MinStartColor = color; }
        virtual void setMaxStartColor( const video::SColor& color ) { MaxStartColor = color; }
        virtual void setMaxLifeTime( const u32 t ) { MaxLifeTime = t; }
        virtual void setMinLifeTime( const u32 t ) { MinLifeTime = t; }
        virtual u32 getMaxLifeTime() const { return MaxLifeTime; }
        virtual u32 getMinLifeTime() const { return MinLifeTime; }
        virtual void setMaxAngleDegrees(const s32 t ) { MaxLifeTime = MaxLifeTime; }
        virtual s32 getMaxAngleDegrees() const { return 0; }
        virtual void setMaxStartSize( const core::dimension2df& size ) { MaxStartSize = size; }
        virtual void setMinStartSize( const core::dimension2df& size ) { MinStartSize = size; }
        virtual void setCenter( const core::vector3df& center ) { Center = center; }
        virtual void setRadius( f32 radius ) { Radius = radius; }
        virtual const core::vector3df& getDirection() const { return Direction; }
        virtual u32 getMinParticlesPerSecond() const { return MinParticlesPerSecond; }
        virtual u32 getMaxParticlesPerSecond() const { return MaxParticlesPerSecond; }
        virtual const video::SColor& getMinStartColor() const { return MinStartColor; }
        virtual const video::SColor& getMaxStartColor() const { return MaxStartColor; }
        virtual const core::dimension2df& getMaxStartSize() const { return MaxStartSize; }
        virtual const core::dimension2df& getMinStartSize() const { return MinStartSize; }
        virtual const core::vector3df& getCenter() const { return Center; }
        virtual f32 getRadius() const { return Radius; }
        virtual irr::scene::E_PARTICLE_EMITTER_TYPE getType() const {
                return (irr::scene::E_PARTICLE_EMITTER_TYPE) 666;
        }
        void restart() { Emitted=0; }
};

// also does gravity and light
class CollisionAffector : public irr::scene::IParticleAffector
{
public:
        CollisionAffector(IGameDef *gamedef, ClientEnvironment &env)
        {
                m_gamedef = gamedef;
                m_env = &env;
        }
        void affect(u32 now, irr::scene::SParticle* particlearray, u32 count)
        {
                if( LastTime == 0 )
                {
                        LastTime = now;
                        return;
                }
                f32 timeDelta = ( now - LastTime ) / 1000.0f;
                LastTime = now;
                if( !Enabled )
                        return;
                for(u32 i=0; i<count; ++i)
                {
                        //gravity done here b/c it doesnt work with a gravity affector atm.
                        particlearray[i].vector += v3f(0.0, -0.1, 0.0) * timeDelta;
                        irr::scene::SParticle p = particlearray[i];
                        float size = p.size.Width;
                        core::aabbox3d<f32> box = core::aabbox3d<f32>
                                        (-size/2,-size/2,-size/2,size/2,size/2,size/2);
                        //to ensure particles collide with correct position
                        v3f pos = p.pos + intToFloat(m_env->getCameraOffset(), BS);
                        v3f speed = p.vector;
                        v3f acc = v3f(0.0, 0.0, 0.0);

                        collisionMoveSimple(m_env, m_gamedef,
                                BS*0.5, box,
                                0, timeDelta,
                                pos, speed, acc);

                        particlearray[i].pos = pos;
                        particlearray[i].vector = speed + (acc * timeDelta);

                        // //useful to see whats going on
                        // if(res == 0)
                        // particlearray[i].color = video::SColor(255.0, 255.0, 0.0, 0.0);
                        // if(res == 1)
                        // particlearray[i].color = video::SColor(255.0, 0.0, 255.0, 0.0);
                        // if(res==2)
                        // particlearray[i].color = video::SColor(255.0, 0.0, 0.0, 255.0);
                        // u8 light = 0;
                        // try{
                        // v3s16 p = v3s16(
                        // floor(pos.X+0.5),
                        // floor(pos.Y+0.5),
                        // floor(pos.Z+0.5)
                        // );
                        // MapNode n = m_env->getClientMap().getNode(p);
                        // light = n.getLightBlend(m_env->getDayNightRatio(), m_gamedef->ndef());
                        // }
                        // catch(InvalidPositionException &e){
                        // light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);
                        // }
                        // u8 plight = decode_light(light);
                        // particlearray[i].color = video::SColor(255, plight, plight, plight);
                }
        }
        irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const {
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) 666;
        }
        irr::u32 LastTime;
        IGameDef *m_gamedef;
        ClientEnvironment *m_env;
};
class DeleteDoneAffector : public irr::scene::IParticleAffector
{
public:
        DeleteDoneAffector(irr::scene::IParticleSystemSceneNode* ps, irr::scene::ISceneManager* smgr)
        {
                ParticleSystem = ps;
                SceneManager = smgr;
        }
        void affect(u32 now, irr::scene::SParticle* particlearray, u32 count)
        {
                // if the particle count every goes to zero, delete the particle system
                if (count<=0)
                        SceneManager->addToDeletionQueue(ParticleSystem);
        }
        irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const {
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) 667;
        }
private:
        irr::scene::IParticleSystemSceneNode* ParticleSystem;
        irr::scene::ISceneManager* SceneManager;
};

/*
	Utility
*/

v3f random_v3f(v3f min, v3f max)
{
	return v3f( rand()/(float)RAND_MAX*(max.X-min.X)+min.X,
			rand()/(float)RAND_MAX*(max.Y-min.Y)+min.Y,
			rand()/(float)RAND_MAX*(max.Z-min.Z)+min.Z);
}

Particle::Particle(
	IGameDef *gamedef,
	scene::ISceneManager* smgr,
	LocalPlayer *player,
	ClientEnvironment *env,
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
):
	scene::ISceneNode(smgr->getRootSceneNode(), smgr)
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


	// Particle related
	m_pos = pos;
	m_velocity = velocity;
	m_acceleration = acceleration;
	m_expiration = expirationtime;
	m_time = 0;
	m_player = player;
	m_size = size;
	m_collisiondetection = collisiondetection;
	m_vertical = vertical;

	// Irrlicht stuff
	m_collisionbox = core::aabbox3d<f32>
			(-size/2,-size/2,-size/2,size/2,size/2,size/2);
	this->setAutomaticCulling(scene::EAC_OFF);

	// Init lighting
	updateLight();

	// Init model
	updateVertices();
}

Particle::~Particle()
{
}

void Particle::OnRegisterSceneNode()
{
	if (IsVisible)
	{
		SceneManager->registerNodeForRendering
				(this, scene::ESNRP_TRANSPARENT);
		SceneManager->registerNodeForRendering
				(this, scene::ESNRP_SOLID);
	}

	ISceneNode::OnRegisterSceneNode();
}

void Particle::render()
{
	// TODO: Render particles in front of water and the selectionbox

	video::IVideoDriver* driver = SceneManager->getVideoDriver();
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
	if (m_collisiondetection)
	{
		core::aabbox3d<f32> box = m_collisionbox;
		v3f p_pos = m_pos*BS;
		v3f p_velocity = m_velocity*BS;
		v3f p_acceleration = m_acceleration*BS;
		collisionMoveSimple(m_env, m_gamedef,
			BS*0.5, box,
			0, dtime,
			p_pos, p_velocity, p_acceleration);
		m_pos = p_pos/BS;
		m_velocity = p_velocity/BS;
		m_acceleration = p_acceleration/BS;
	}
	else
	{
		m_velocity += m_acceleration * dtime;
		m_pos += m_velocity * dtime;
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
	MapNode n = m_env->getClientMap().getNodeNoEx(p, &pos_ok);
	if (pos_ok)
		light = n.getLightBlend(m_env->getDayNightRatio(), m_gamedef->ndef());
	else
		light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);

	m_light = decode_light(light);
}

void Particle::updateVertices()
{
	video::SColor c(255, m_light, m_light, m_light);
	f32 tx0 = m_texpos.X;
	f32 tx1 = m_texpos.X + m_texsize.X;
	f32 ty0 = m_texpos.Y;
	f32 ty1 = m_texpos.Y + m_texsize.Y;

	m_vertices[0] = video::S3DVertex(-m_size/2,-m_size/2,0, 0,0,0,
			c, tx0, ty1);
	m_vertices[1] = video::S3DVertex(m_size/2,-m_size/2,0, 0,0,0,
			c, tx1, ty1);
	m_vertices[2] = video::S3DVertex(m_size/2,m_size/2,0, 0,0,0,
			c, tx1, ty0);
	m_vertices[3] = video::S3DVertex(-m_size/2,m_size/2,0, 0,0,0,
			c, tx0, ty0);

	v3s16 camera_offset = m_env->getCameraOffset();
	for(u16 i=0; i<4; i++)
	{
		if (m_vertical) {
			v3f ppos = m_player->getPosition()/BS;
			m_vertices[i].Pos.rotateXZBy(atan2(ppos.Z-m_pos.Z, ppos.X-m_pos.X)/core::DEGTORAD+90);
		} else {
			m_vertices[i].Pos.rotateYZBy(m_player->getPitch());
			m_vertices[i].Pos.rotateXZBy(m_player->getYaw());
		}
		m_box.addInternalPoint(m_vertices[i].Pos);
		m_vertices[i].Pos += m_pos*BS - intToFloat(camera_offset, BS);
	}
}

/*
	ParticleSpawner
*/

ParticleSpawner::ParticleSpawner(IGameDef* gamedef, scene::ISceneManager *smgr, LocalPlayer *player,
	u16 amount, float time,
	v3f minpos, v3f maxpos, v3f minvel, v3f maxvel, v3f minacc, v3f maxacc,
	float minexptime, float maxexptime, float minsize, float maxsize,
	bool collisiondetection, bool vertical, video::ITexture *texture, u32 id,
	ParticleManager *p_manager) :
	m_particlemanager(p_manager)
{
	m_gamedef = gamedef;
	m_smgr = smgr;
	m_player = player;
	m_amount = amount;
	m_spawntime = time;
	m_minpos = minpos;
	m_maxpos = maxpos;
	m_minvel = minvel;
	m_maxvel = maxvel;
	m_minacc = minacc;
	m_maxacc = maxacc;
	m_minexptime = minexptime;
	m_maxexptime = maxexptime;
	m_minsize = minsize;
	m_maxsize = maxsize;
	m_collisiondetection = collisiondetection;
	m_vertical = vertical;
	m_texture = texture;
	m_time = 0;

	for (u16 i = 0; i<=m_amount; i++)
	{
		float spawntime = (float)rand()/(float)RAND_MAX*m_spawntime;
		m_spawntimes.push_back(spawntime);
	}
}

ParticleSpawner::~ParticleSpawner() {}

void ParticleSpawner::step(float dtime, ClientEnvironment* env)
{
	m_time += dtime;

	if (m_spawntime != 0) // Spawner exists for a predefined timespan
	{
		for(std::vector<float>::iterator i = m_spawntimes.begin();
				i != m_spawntimes.end();)
		{
			if ((*i) <= m_time && m_amount > 0)
			{
				m_amount--;

				v3f pos = random_v3f(m_minpos, m_maxpos);
				v3f vel = random_v3f(m_minvel, m_maxvel);
				v3f acc = random_v3f(m_minacc, m_maxacc);
				float exptime = rand()/(float)RAND_MAX
						*(m_maxexptime-m_minexptime)
						+m_minexptime;
				float size = rand()/(float)RAND_MAX
						*(m_maxsize-m_minsize)
						+m_minsize;

				Particle* toadd = new Particle(
					m_gamedef,
					m_smgr,
					m_player,
					env,
					pos,
					vel,
					acc,
					exptime,
					size,
					m_collisiondetection,
					m_vertical,
					m_texture,
					v2f(0.0, 0.0),
					v2f(1.0, 1.0));
				m_particlemanager->addParticle(toadd);
				i = m_spawntimes.erase(i);
			}
			else
			{
				i++;
			}
		}
	}
	else // Spawner exists for an infinity timespan, spawn on a per-second base
	{
		for (int i = 0; i <= m_amount; i++)
		{
			if (rand()/(float)RAND_MAX < dtime)
			{
				v3f pos = random_v3f(m_minpos, m_maxpos);
				v3f vel = random_v3f(m_minvel, m_maxvel);
				v3f acc = random_v3f(m_minacc, m_maxacc);
				float exptime = rand()/(float)RAND_MAX
						*(m_maxexptime-m_minexptime)
						+m_minexptime;
				float size = rand()/(float)RAND_MAX
						*(m_maxsize-m_minsize)
						+m_minsize;

				new Particle(
					m_gamedef,
					m_smgr,
					m_player,
					env,
					pos,
					vel,
					acc,
					exptime,
					size,
					m_collisiondetection,
					m_vertical,
					m_texture,
					v2f(0.0, 0.0),
					v2f(1.0, 1.0));
			}
		}
	}
}


ParticleManager::ParticleManager(ClientEnvironment* env) :
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

void ParticleManager::stepSpawners (float dtime)
{
	JMutexAutoLock lock(m_spawner_list_lock);
	for(std::map<u32, ParticleSpawner*>::iterator i = 
			m_particle_spawners.begin();
			i != m_particle_spawners.end();)
	{
		if (i->second->get_expired())
		{
			delete i->second;
			m_particle_spawners.erase(i++);
		}
		else
		{
			i->second->step(dtime, m_env);
			i++;
		}
	}
}

void ParticleManager::stepParticles (float dtime)
{
	JMutexAutoLock lock(m_particle_list_lock);
	for(std::vector<Particle*>::iterator i = m_particles.begin();
			i != m_particles.end();)
	{
		if ((*i)->get_expired())
		{
			(*i)->remove();
			delete *i;
			i = m_particles.erase(i);
		}
		else
		{
			(*i)->step(dtime);
			i++;
		}
	}
}

void ParticleManager::clearAll ()
{
	JMutexAutoLock lock(m_spawner_list_lock);
	JMutexAutoLock lock2(m_particle_list_lock);
	for(std::map<u32, ParticleSpawner*>::iterator i =
			m_particle_spawners.begin();
			i != m_particle_spawners.end();)
	{
		delete i->second;
		m_particle_spawners.erase(i++);
	}

	for(std::vector<Particle*>::iterator i =
			m_particles.begin();
			i != m_particles.end();)
	{
		(*i)->remove();
		delete *i;
		i = m_particles.erase(i);
	}
}

void ParticleManager::handleParticleEvent(ClientEvent *event, IGameDef *gamedef,
		scene::ISceneManager* smgr, LocalPlayer *player)
{
	if (event->type == CE_DELETE_PARTICLESPAWNER) {
		JMutexAutoLock lock(m_spawner_list_lock);
		if (m_particle_spawners.find(event->delete_particlespawner.id) !=
				m_particle_spawners.end())
		{
			delete m_particle_spawners.find(event->delete_particlespawner.id)->second;
			m_particle_spawners.erase(event->delete_particlespawner.id);
		}
		// no allocated memory in delete event
		return;
	}

	if (event->type == CE_ADD_PARTICLESPAWNER) {

		{
			JMutexAutoLock lock(m_spawner_list_lock);
			if (m_particle_spawners.find(event->add_particlespawner.id) !=
							m_particle_spawners.end())
			{
				delete m_particle_spawners.find(event->add_particlespawner.id)->second;
				m_particle_spawners.erase(event->add_particlespawner.id);
			}
		}
		video::ITexture *texture =
			gamedef->tsrc()->getTexture(*(event->add_particlespawner.texture));

		ParticleSpawner* toadd = new ParticleSpawner(gamedef, smgr, player,
				event->add_particlespawner.amount,
				event->add_particlespawner.spawntime,
				*event->add_particlespawner.minpos,
				*event->add_particlespawner.maxpos,
				*event->add_particlespawner.minvel,
				*event->add_particlespawner.maxvel,
				*event->add_particlespawner.minacc,
				*event->add_particlespawner.maxacc,
				event->add_particlespawner.minexptime,
				event->add_particlespawner.maxexptime,
				event->add_particlespawner.minsize,
				event->add_particlespawner.maxsize,
				event->add_particlespawner.collisiondetection,
				event->add_particlespawner.vertical,
				texture,
				event->add_particlespawner.id,
				this);

		/* delete allocated content of event */
		delete event->add_particlespawner.minpos;
		delete event->add_particlespawner.maxpos;
		delete event->add_particlespawner.minvel;
		delete event->add_particlespawner.maxvel;
		delete event->add_particlespawner.minacc;
		delete event->add_particlespawner.texture;
		delete event->add_particlespawner.maxacc;

		{
			JMutexAutoLock lock(m_spawner_list_lock);
			m_particle_spawners.insert(
					std::pair<u32, ParticleSpawner*>(
							event->add_particlespawner.id,
							toadd));
		}

		return;
	}

	if (event->type == CE_SPAWN_PARTICLE) {
		video::ITexture *texture =
			gamedef->tsrc()->getTexture(*(event->spawn_particle.texture));

		Particle* toadd = new Particle(gamedef, smgr, player, m_env,
				*event->spawn_particle.pos,
				*event->spawn_particle.vel,
				*event->spawn_particle.acc,
				event->spawn_particle.expirationtime,
				event->spawn_particle.size,
				event->spawn_particle.collisiondetection,
				event->spawn_particle.vertical,
				texture,
				v2f(0.0, 0.0),
				v2f(1.0, 1.0));

		addParticle(toadd);

		delete event->spawn_particle.pos;
		delete event->spawn_particle.vel;
		delete event->spawn_particle.acc;

		return;
	}
}

void ParticleManager::addDiggingParticles(IGameDef* gamedef, scene::ISceneManager* smgr,
		LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
//	for (u16 j = 0; j < 32; j++) // set the amount of particles here
//	{
		addNodeParticle(gamedef, smgr, player, pos, tiles);
//	}
}

void ParticleManager::addPunchingParticles(IGameDef* gamedef, scene::ISceneManager* smgr,
		LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
	addNodeParticle(gamedef, smgr, player, pos, tiles);
}

void ParticleManager::addNodeParticle(IGameDef* gamedef, scene::ISceneManager* smgr,
		     LocalPlayer *player, /*ClientEnvironment &env,*/ v3s16 pos,
		     const TileSpec tiles[])
{
	// Texture
	u8 texid = myrand_range(0,5);
	video::ITexture *texture = tiles[texid].texture;
	//to ensure particles are at correct position
	//v3s16 camera_offset = (&env)->getCameraOffset();
	v3s16 camera_offset = m_env->getCameraOffset();
	v3f particlepos = intToFloat(pos, BS) - intToFloat(camera_offset, BS);
	scene::IParticleSystemSceneNode* ps =
			smgr->addParticleSystemSceneNode(false);
	scene::IParticleEmitter* em;
	em = new CParticleExplosionEmitter(particlepos, // position
					   0.5, // radius
					   core::vector3df(0.0f,0.02f,0.0f), // initial direction
					   20, 40, // emit rate
					   video::SColor(0,255,255,255), // darkest color
					   video::SColor(0,255,255,255), // brightest color
					   800, 4000, // min and max age,
					   core::dimension2df(0.3f,0.3f), // min size
					   core::dimension2df(2.f,2.f)); // max size
	//irrlicht standard
	ps->setEmitter(em);
	em->drop();
	// this does not work, results in particles sticking to ground and flickering
	// up and down. this affector sets particles startVector
	// scene::IParticleAffector* paf = ps->createGravityAffector(v3f(0.0, -0.1, 0.0));
	// ps->addAffector(paf);
	// paf->drop();
	// //for deletion after fixed time, just here for keeping the info
	// irr::scene::ISceneNodeAnimator* sna = smgr->createDeleteAnimator(2000);
	// ps->addAnimator(sna);
	// sna->drop();
	irr::scene::IParticleAffector* puf = new CollisionAffector(gamedef, *m_env);
	ps->addAffector(puf);
	puf->drop();
	//need to be added after collision
	scene::IParticleAffector* paf = new DeleteDoneAffector(ps, smgr);
	ps->addAffector(paf);
	paf->drop();
	ps->setMaterialFlag(video::EMF_LIGHTING, false);
	ps->setMaterialFlag(video::EMF_ZWRITE_ENABLE, true );
	ps->setMaterialTexture(0, texture);
	//ps->setMaterialType(video::EMT_TRANSPARENT_ADD_COLOR);
}

/*
void ParticleManager::addNodeParticle(IGameDef* gamedef, scene::ISceneManager* smgr,
		LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
	// Texture
	u8 texid = myrand_range(0, 5);
	video::ITexture *texture = tiles[texid].texture;

	// Only use first frame of animated texture
	f32 ymax = 1;
	if(tiles[texid].material_flags & MATERIAL_FLAG_ANIMATION_VERTICAL_FRAMES)
		ymax /= tiles[texid].animation_frame_count;

	float size = rand() % 64 / 512.;
	float visual_size = BS * size;
	v2f texsize(size * 2, ymax * size * 2);
	v2f texpos;
	texpos.X = ((rand() % 64) / 64. - texsize.X);
	texpos.Y = ymax * ((rand() % 64) / 64. - texsize.Y);

	// Physics
	v3f velocity((rand() % 100 / 50. - 1) / 1.5,
			rand() % 100 / 35.,
			(rand() % 100 / 50. - 1) / 1.5);

	v3f acceleration(0,-9,0);
	v3f particlepos = v3f(
		(f32) pos.X + rand() %100 /200. - 0.25,
		(f32) pos.Y + rand() %100 /200. - 0.25,
		(f32) pos.Z + rand() %100 /200. - 0.25
	);

	Particle* toadd = new Particle(
		gamedef,
		smgr,
		player,
		m_env,
		particlepos,
		velocity,
		acceleration,
		rand() % 100 / 100., // expiration time
		visual_size,
		true,
		false,
		texture,
		texpos,
		texsize);

	addParticle(toadd);
}
*/

void ParticleManager::addParticle(Particle* toadd)
{
	JMutexAutoLock lock(m_particle_list_lock);
	m_particles.push_back(toadd);
}
