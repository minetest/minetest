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

#define PARTICLES_BBOX

/*
	Utility
*/

v3f random_v3f(v3f min, v3f max) {
	return v3f( rand()/(float)RAND_MAX*(max.X-min.X)+min.X,
		    rand()/(float)RAND_MAX*(max.Y-min.Y)+min.Y,
		    rand()/(float)RAND_MAX*(max.Z-min.Z)+min.Z);
}

class FixNumEmitter :  public irr::scene::IParticleEmitter
{
public:
        FixNumEmitter(u32 number):
                number(number),
                emitted(0)
        {}

        s32 emitt(u32 now, u32 timeSinceLastCall, irr::scene::SParticle*& outArray)
        {
                if (emitted > 0)
                        return 0;

                particles.set_used(0);
                irr::scene::SParticle p;
                for(u32 i=0; i<number; ++i) {
                        v3f particlepos = random_v3f( v3f(-0.75, -0.75, -0.75),
                                                      v3f( 0.75,  0.75,  0.75));
                        p.pos.set(particlepos);
                        v3f velocity = random_v3f(v3f(-0.9, 0.6, -0.9), v3f( 0.9, 0.6, 0.9));

                        p.vector = velocity/100;
                        p.startVector = p.vector;

                        p.startTime = now;
                        p.endTime = now + 500 + (rand() % (2000 - 500));;

                        p.color = video::SColor(255.0, 255.0, 255.0, 255.0);
                        p.startColor = p.color;
                        float size = rand() % 16 / BS;
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

private:
        core::array<irr::scene::SParticle> particles;
        u32 number;
        u32 emitted;
};

class CollisionAffector : public irr::scene::IParticleAffector
{
public:
        CollisionAffector(ClientEnvironment &env):
                env(&env),
                gamedef(env.getGameDef())
        {}

        void affect(u32 now, irr::scene::SParticle* particlearray, u32 count)
        {
                if ( last_time == 0 ) {
                        last_time = now;
                        return;
                }
                f32 dtime = ( now - last_time ) / 1000.0f;
                last_time = now;

                if ( !Enabled )
                        return;

                v3f acc = v3f(0.0, 0.0, 0.0);
                for (u32 i=0; i<count; ++i) {
                        irr::scene::SParticle p = particlearray[i];
                        float size = p.size.Width;
                        core::aabbox3d<f32> box = core::aabbox3d<f32>
                                        (-size/2, -size/2, -size/2, size/2, size/2, size/2);

                        // Collision detection works in MT coordinates,
                        // adjust for camera offset.
                        v3f off = intToFloat(env->getCameraOffset(), BS);
                        particlearray[i].pos += off;

                        collisionMoveSimple(env, gamedef,
                                            BS * 0.5, box,
                                            0, dtime,
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
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) (irr::scene::EPAT_COUNT + 1);
        }
        u32 last_time;
        ClientEnvironment *env;
        IGameDef *gamedef;
};

class CameraOffsetAffector : public irr::scene::IParticleAffector
{
public:
        CameraOffsetAffector(ClientEnvironment &env, irr::scene::IParticleSystemSceneNode* ps):
                        env(&env),
                        old_camera_offset(v3s16(0,0,0)),
                        ps(ps)
        {}

        void affect(u32 now, irr::scene::SParticle* particlearray, u32 count)
        {
                v3s16 camera_offset = env->getCameraOffset();
                if (camera_offset != old_camera_offset) {
                        v3f ps_offset = intToFloat(old_camera_offset - camera_offset, BS);
                        ps->setPosition(ps->getPosition() + ps_offset);
                        for (u32 i=0; i<count; ++i) {
                                particlearray[i].pos += ps_offset;
                        }
                        old_camera_offset = camera_offset;
                }
        }
        virtual irr::scene::E_PARTICLE_AFFECTOR_TYPE getType() const {
                return (irr::scene::E_PARTICLE_AFFECTOR_TYPE) (irr::scene::EPAT_COUNT+2);
        }
private:
        ClientEnvironment *env;
        v3s16 old_camera_offset;
        irr::scene::IParticleSystemSceneNode* ps;
};

ParticleManager::ParticleManager(ClientEnvironment* env, irr::scene::ISceneManager* smgr):
	m_env(env),
	m_smgr(smgr)
{}

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

		scene::IParticleSystemSceneNode * ps = m_smgr->addParticleSystemSceneNode();

		ps->setID((s32) event->add_particlespawner.id);

		float pps = event->add_particlespawner.amount;
		float time = event->add_particlespawner.spawntime;

		if (time > 0)
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

		ps->setAutomaticCulling(scene::EAC_FRUSTUM_BOX);

		v3f pos = *event->add_particlespawner.minpos;
		pos = pos * BS;
		ps->setPosition(pos);
#ifdef PARTICLES_BBOX
		ps->setDebugDataVisible(irr::scene::EDS_BBOX);
#endif
		scene::IParticleAffector * caf = new CameraOffsetAffector(*m_env, ps);
		ps->addAffector(caf);
		caf->drop();

		video::ITexture *texture =
				gamedef->tsrc()->getTextureForMesh(*(event->add_particlespawner.texture));
		ps->setMaterialTexture(0, texture);

		ps->setMaterialFlag(video::EMF_LIGHTING, false);
		ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
		ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
		ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
		ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);

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

		if (time > 0) {
			scene::ISceneNodeAnimator* pan =  m_smgr->createDeleteAnimator(time * 1000);
			ps->addAnimator(pan);
			pan->drop();
		}
		return;
	}

	if (event->type == CE_SPAWN_PARTICLE) {
		//TODO: use FixNumEmitter here
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
				      v3s16 pos, const TileSpec tiles[], u32 number)
{
	scene::IParticleSystemSceneNode* ps =
			m_smgr->addParticleSystemSceneNode(false);

	// set camera offset manually, single burst spawner is not updated
	v3s16 camera_offset = m_env->getCameraOffset();
	v3f particlepos = intToFloat(pos, BS) - intToFloat(camera_offset, BS);
	ps->setPosition(particlepos);
#ifdef PARTICLES_BBOX
	ps->setDebugDataVisible(irr::scene::EDS_BBOX);
#endif
	scene::IParticleEmitter* em;
	em = new FixNumEmitter(number);
	ps->setEmitter(em);
	em->drop();

	scene::IParticleAffector* paf1 = ps->createGravityAffector(v3f(0.0, -0.1, 0.0), 2000);
	ps->addAffector(paf1);
	paf1->drop();

	scene::IParticleAffector* paf2 = new CollisionAffector(*m_env);
	ps->addAffector(paf2);
	paf2->drop();

	scene::ISceneNodeAnimator* pan =  m_smgr->createDeleteAnimator(2000);
	ps->addAnimator(pan);
	pan->drop();

	u8 texid = myrand_range(0, 5);
	video::ITexture *texture = tiles[texid].texture;
	ps->setMaterialTexture(0, texture);

	ps->setMaterialFlag(video::EMF_LIGHTING, false);
	ps->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
	ps->setMaterialFlag(video::EMF_BILINEAR_FILTER, false);
	ps->setMaterialFlag(video::EMF_FOG_ENABLE, true);
	ps->setMaterialType(video::EMT_TRANSPARENT_ALPHA_CHANNEL);
}

