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
#include "client/tile.h"
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

/*
	Utility
*/

v3f random_v3f(v3f min, v3f max)
{
	return v3f( rand()/(float)RAND_MAX*(max.X-min.X)+min.X,
			rand()/(float)RAND_MAX*(max.Y-min.Y)+min.Y,
			rand()/(float)RAND_MAX*(max.Z-min.Z)+min.Z);
}

class fixNumEmitter :  public irr::scene::IParticleEmitter {
private:
        core::array<irr::scene::SParticle> particles;

        u32 number;
        v3f pos;
        u32 emitted;
public:

        fixNumEmitter(v3f pos, int number) : number(number), pos(pos), emitted(0) {}

        s32 emitt(u32 now, u32 timeSinceLastCall, irr::scene::SParticle*& outArray)
        {
                if (emitted > 0) return 0;

                particles.set_used(0);

                irr::scene::SParticle p;
                for(u32 i=0; i<number; ++i)
                {
                        v3f particlepos = v3f(
                                                pos.X + rand() %100 /200. - 0.25,
                                                pos.Y + rand() %100 /200. - 0.25,
                                                pos.Z + rand() %100 /200. - 0.25);

                        p.pos.set(particlepos);

                        v3f velocity((rand() % 100 / 50. - 1) / 1.5,
                                     rand() % 100 / 35.,
                                     (rand() % 100 / 50. - 1) / 1.5);
                        p.vector = velocity/100;
                        p.startVector = p.vector;

                        p.startTime = now;
                        p.endTime = now + 500 + (rand() % (2000 - 500));;

                        p.color = video::SColor(255.0, 255.0, 255.0, 255.0);
                        p.startColor = p.color;
                        float size = rand() % 64 / 51.2;
                        p.startSize = core::dimension2d<f32>(size, size);
                        p.size = p.startSize;
                        particles.push_back(p);
                }
                outArray = particles.pointer();
                emitted += particles.size();

                return particles.size();
        }

        virtual void setDirection( const core::vector3df& newDirection ) {  }
        virtual void setMinParticlesPerSecond( u32 minPPS ) {  }
        virtual void setMaxParticlesPerSecond( u32 maxPPS ) {  }
        virtual void setMinStartColor( const video::SColor& color ) {  }
        virtual void setMaxStartColor( const video::SColor& color ) {  }
        virtual void setMaxLifeTime( const u32 t ) {  }
        virtual void setMinLifeTime( const u32 t ) { }
        virtual u32 getMaxLifeTime() const { return 1; }
        virtual u32 getMinLifeTime() const { return 1; }
        virtual void setMaxAngleDegrees(const s32 t ) {  }
        virtual s32 getMaxAngleDegrees() const { return 0; }
        virtual void setMaxStartSize( const core::dimension2df& size ) { }
        virtual void setMinStartSize( const core::dimension2df& size ) {  }
        virtual void setCenter( const core::vector3df& center ) {  }
        virtual void setRadius( f32 radius ) {  }
        virtual const core::vector3df& getDirection() const { return v3f(0.0, 0.0, 0.0); }
        virtual u32 getMinParticlesPerSecond() const { return 1; }
        virtual u32 getMaxParticlesPerSecond() const { return 1; }
        virtual const video::SColor& getMinStartColor() const { return video::SColor(1); }
        virtual const video::SColor& getMaxStartColor() const { return video::SColor(1);; }
        virtual const core::dimension2df& getMaxStartSize() const { return core::dimension2d<f32>(0.2, 0.2); }
        virtual const core::dimension2df& getMinStartSize() const { return core::dimension2d<f32>(0.2, 0.2); }
        virtual const core::vector3df& getCenter() const { return v3f(0.0, 0.0, 0.0); }
        virtual f32 getRadius() const { return 1.; }
        virtual irr::scene::E_PARTICLE_EMITTER_TYPE getType() const {
                return (irr::scene::E_PARTICLE_EMITTER_TYPE) (irr::scene::EPET_COUNT+1);
        }

        void restart() { emitted=0; }

};

class CollisionAffector : public irr::scene::IParticleAffector
{
public:
        CollisionAffector(ClientEnvironment &env)
        {
                m_env = &env;
                m_gamedef = m_env->getGameDef();
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

                v3f acc = v3f(0.0, 0.0, 0.0);
                for(u32 i=0; i<count; ++i)
                {
                        irr::scene::SParticle p = particlearray[i];
                        float size = p.size.Width;
                        core::aabbox3d<f32> box = core::aabbox3d<f32>
                                        (-size/2,-size/2,-size/2,size/2,size/2,size/2);

                        //to ensure particles collide with correct position
                        v3f off = intToFloat(m_env->getCameraOffset(), BS);

                        particlearray[i].pos += off;

                        collisionMoveSimple(m_env, m_gamedef,
                                            BS * 0.5, box,
                                            0, timeDelta,
                                            particlearray[i].pos,
                                            particlearray[i].vector,
                                            acc);

                        particlearray[i].pos -= off;

                        // TODO: create light affector?
//                        try{
//                                v3s16 p = v3s16(
//                                                        floor(pos.X+0.5),
//                                                        floor(pos.Y+0.5),
//                                                        floor(pos.Z+0.5)
//                                                        );
//                                MapNode n = m_env->getClientMap().getNode(p);
//                                light = n.getLightBlend(m_env->getDayNightRatio(), m_gamedef->ndef());
//                        }
//                        catch(InvalidPositionException &e){
//                                light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);
//                        }
//                        u8 plight = decode_light(light);
//                        particlearray[i].color = video::SColor(255, plight, plight, plight);
                }
        }
        virtual irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const {
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) (irr::scene::EPAT_COUNT+1);
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
                if (count<=0) {
                        SceneManager->addToDeletionQueue(ParticleSystem);
                }
        }
        virtual irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const {
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) (irr::scene::EPAT_COUNT+2);
        }
private:
        irr::scene::IParticleSystemSceneNode* ParticleSystem;
        irr::scene::ISceneManager* SceneManager;
};

class CameraOffsetAffector : public irr::scene::IParticleAffector
{
public:
        CameraOffsetAffector(ClientEnvironment &env,
                             irr::scene::IParticleSystemSceneNode* ps, v3f pos)
        {
                m_env = &env;

                // invalid offset intentional, updated after creating
                m_offset = v3s16(666, 666, 666);
                m_ps = ps;
                m_pos = pos;
        }

        void affect(u32 now, irr::scene::SParticle* particlearray, u32 count)
        {
                v3s16 offset = m_env->getCameraOffset();
                if (offset == m_offset)
                        return;
                m_offset = offset;
                v3f off = intToFloat(offset, BS);
                // this is done to avoid burst of emitted particles in wrong location
                // when moving around. maybe keep the loop through particles and adjust
                // those already around before moving the spawner instead of deleting them.
                //m_ps->clearParticles();
                for(u32 i=0; i<count; ++i) {
                        particlearray[i].pos -= off;
                }
                m_ps->setPosition(m_pos - off);
        }
        virtual irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const {
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) (irr::scene::EPAT_COUNT+3);
        }
private:
        ClientEnvironment *m_env;
        v3s16 m_offset;
        irr::scene::IParticleSystemSceneNode* m_ps;
        v3f m_pos;
};

ParticleManager::ParticleManager(ClientEnvironment* env, irr::scene::ISceneManager* smgr) :
	m_env(env),
	m_smgr(smgr)
{}

ParticleManager::~ParticleManager()
{
	clearAll();
}

void ParticleManager::clearAll ()
{
	return;
}

void ParticleManager::handleParticleEvent(ClientEvent *event, IGameDef *gamedef, LocalPlayer *player)
{
	if (event->type == CE_DELETE_PARTICLESPAWNER) {
		scene::ISceneNode *node = m_smgr->getSceneNodeFromId(
					event->delete_particlespawner.id);
		if (node)
			m_smgr->addToDeletionQueue(node);
		return;
	}

	if (event->type == CE_ADD_PARTICLESPAWNER) {

		scene::ISceneNode *node = m_smgr->getSceneNodeFromId(
					event->delete_particlespawner.id);
		if (node)
			m_smgr->addToDeletionQueue(node);

		video::ITexture *texture =
				gamedef->tsrc()->getTextureForMesh(*(event->add_particlespawner.texture));

		scene::IParticleSystemSceneNode * ps = m_smgr->addParticleSystemSceneNode();

		ps->setID((s32) event->add_particlespawner.id);

		float pps = event->add_particlespawner.amount;
		float time = event->add_particlespawner.spawntime;

		if (time != 0)
			pps = pps / time;

		float minsize = event->add_particlespawner.minsize;
		float maxsize = event->add_particlespawner.maxsize;

		scene::IParticlePointEmitter * em = ps->createPointEmitter(
					random_v3f(v3f(-0.25,-0.25,-0.25), v3f(0.25, 0.25, 0.25))/10,
					pps, // 5, //minpps
					pps, // 20,//maxpps
					video::SColor(255.0, 255.0, 255.0, 255.0), //mincol,
					video::SColor(255.0, 255.0, 255.0, 255.0), //maxcol,
					event->add_particlespawner.minexptime*1000,
					event->add_particlespawner.maxexptime*1000,
					360, //maxdeg,
					core::dimension2d<f32>(minsize, minsize),
					core::dimension2d<f32>(maxsize, maxsize));

		ps->setEmitter(em);
		em->drop();

		ps->setMaterialTexture(0, texture);
		ps->setAutomaticCulling(scene::EAC_FRUSTUM_BOX);
		//ps->setDebugDataVisible(irr::scene::EDS_BBOX);

		v3f pos = *event->add_particlespawner.minpos;
//		pos = pos * BS - intToFloat(m_env->getCameraOffset(), BS);
		pos = pos * BS;

		// position is updated with affector
		//ps->setPosition(pos);

		ps->setMaterialFlag(video::EMF_LIGHTING, false);
		ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
		ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);

		scene::IParticleAffector * caf =
				new CameraOffsetAffector(*m_env, ps, pos);
		ps->addAffector(caf);
		caf->drop();

		// TODO: create acceleration affector similar to GravityAffector
		// with min/max acceleration
		scene::IParticleAffector* paf1 = ps->createGravityAffector(v3f(0.0, 0.01, 0.0), 2000);
		ps->addAffector(paf1);
		paf1->drop();

		if (event->add_particlespawner.collisiondetection) {
			scene::IParticleAffector* paf2 = new CollisionAffector(*m_env);
			ps->addAffector(paf2);
			paf2->drop();
		}

		if (time != 0) {
			scene::ISceneNodeAnimator* pan =  m_smgr->createDeleteAnimator(time * 1000);
			ps->addAnimator(pan);
			pan->drop();
		}
		return;
	}

	if (event->type == CE_SPAWN_PARTICLE) {
		//TODO: use fixNumEmitter here

		//		video::ITexture *texture =
		//			gamedef->tsrc()->getTextureForMesh(*(event->spawn_particle.texture));

		//		Particle* toadd = new Particle(gamedef, smgr, player, m_env,
		//				*event->spawn_particle.pos,
		//				*event->spawn_particle.vel,
		//				*event->spawn_particle.acc,
		//				event->spawn_particle.expirationtime,
		//				event->spawn_particle.size,
		//				event->spawn_particle.collisiondetection,
		//				event->spawn_particle.vertical,
		//				texture,
		//				v2f(0.0, 0.0),
		//				v2f(1.0, 1.0));

		//		addParticle(toadd);
		return;
	}
}

void ParticleManager::addDiggingParticles(IGameDef* gamedef,
		LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
		addNodeParticle(gamedef, player, pos, tiles, 32);
}

void ParticleManager::addPunchingParticles(IGameDef* gamedef,
		LocalPlayer *player, v3s16 pos, const TileSpec tiles[])
{
	addNodeParticle(gamedef, player, pos, tiles, 1);
}

void ParticleManager::addNodeParticle(IGameDef* gamedef, LocalPlayer *player,
				      v3s16 pos, const TileSpec tiles[], int number)
{
	// Texture
	u8 texid = myrand_range(0, 5);
	video::ITexture *texture = tiles[texid].texture;

	// set camera offset manually, single burst spawner is not updated
	// test if update via affector is instantaneous enough
	v3s16 camera_offset = m_env->getCameraOffset();
	v3f particlepos = intToFloat(pos, BS) - intToFloat(camera_offset, BS);

	scene::IParticleSystemSceneNode* ps =
			m_smgr->addParticleSystemSceneNode(false);
	scene::IParticleEmitter* em;

	em = new fixNumEmitter(particlepos, number);
	ps->setEmitter(em);
	em->drop();

	scene::IParticleAffector* paf1 = ps->createGravityAffector(v3f(0.0, -0.1, 0.0), 2000);
	ps->addAffector(paf1);
	paf1->drop();

	scene::IParticleAffector* paf2 = new CollisionAffector(*m_env);
	ps->addAffector(paf2);
	paf2->drop();

	scene::IParticleAffector* paf3 = new DeleteDoneAffector(ps, m_smgr);
	ps->addAffector(paf3);
	paf3->drop();

	ps->setMaterialTexture(0, texture);

	ps->setMaterialFlag(video::EMF_LIGHTING, false);
	ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
	ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
	ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);
}

