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
	Particle
*/

Particle::Particle(
	IGameDef *gamedef,
	LocalPlayer *player,
	ClientEnvironment *env,
	const ParticleParameters &p,
	const ClientTexRef& texture,
	v2f texpos,
	v2f texsize,
	video::SColor color
):
	scene::ISceneNode(((Client *)gamedef)->getSceneManager()->getRootSceneNode(),
		((Client *)gamedef)->getSceneManager()),
	m_texture(texture)
{
	// Misc
	m_gamedef = gamedef;
	m_env = env;

	// translate blend modes to GL blend functions
	video::E_BLEND_FACTOR bfsrc, bfdst;
	video::E_BLEND_OPERATION blendop;
	const auto blendmode = texture.tex != nullptr
			? texture.tex -> blendmode
			: ParticleParamTypes::BlendMode::alpha;

	switch (blendmode) {
		case ParticleParamTypes::BlendMode::add:
			bfsrc = video::EBF_SRC_ALPHA;
			bfdst = video::EBF_DST_ALPHA;
			blendop = video::EBO_ADD;
		break;

		case ParticleParamTypes::BlendMode::sub:
			bfsrc = video::EBF_SRC_ALPHA;
			bfdst = video::EBF_DST_ALPHA;
			blendop = video::EBO_REVSUBTRACT;
		break;

		case ParticleParamTypes::BlendMode::screen:
			bfsrc = video::EBF_ONE;
			bfdst = video::EBF_ONE_MINUS_SRC_COLOR;
			blendop = video::EBO_ADD;
		break;

		default: // includes ParticleParamTypes::BlendMode::alpha
			bfsrc = video::EBF_SRC_ALPHA;
			bfdst = video::EBF_ONE_MINUS_SRC_ALPHA;
			blendop = video::EBO_ADD;
		break;
	}

	// Texture
	m_material.setFlag(video::EMF_LIGHTING, false);
	m_material.setFlag(video::EMF_BACK_FACE_CULLING, false);
	m_material.setFlag(video::EMF_BILINEAR_FILTER, false);
	m_material.setFlag(video::EMF_FOG_ENABLE, true);

	// correctly render layered transparent particles -- see #10398
	m_material.setFlag(video::EMF_ZWRITE_ENABLE, true);

	// enable alpha blending and set blend mode
	m_material.MaterialType = video::EMT_ONETEXTURE_BLEND;
	m_material.MaterialTypeParam = video::pack_textureBlendFunc(
			bfsrc, bfdst,
			video::EMFN_MODULATE_1X,
			video::EAS_TEXTURE | video::EAS_VERTEX_COLOR);
	m_material.BlendOperation = blendop;
	m_material.setTexture(0, m_texture.ref);
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
	m_drag = p.drag;
	m_jitter = p.jitter;
	m_bounce = p.bounce;
	m_expiration = p.expirationtime;
	m_player = player;
	m_size = p.size;
	m_collisiondetection = p.collisiondetection;
	m_collision_removal = p.collision_removal;
	m_object_collision = p.object_collision;
	m_vertical = p.vertical;
	m_glow = p.glow;
	m_alpha = 0;
	m_parent = nullptr;

	// Irrlicht stuff
	const float c = p.size / 2;
	m_collisionbox = aabb3f(-c, -c, -c, c, c, c);
	this->setAutomaticCulling(scene::EAC_OFF);

	// Init lighting
	updateLight();

	// Init model
	updateVertices();
}

Particle::~Particle()
{
	/* if our textures aren't owned by a particlespawner, we need to clean
	 * them up ourselves when the particle dies */
	if (m_parent == nullptr)
		delete m_texture.tex;
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

	// apply drag (not handled by collisionMoveSimple) and brownian motion
	v3f av = vecAbsolute(m_velocity);
	av -= av * (m_drag * dtime);
	m_velocity = av*vecSign(m_velocity) + v3f(m_jitter.pickWithin())*dtime;

	if (m_collisiondetection) {
		aabb3f box = m_collisionbox;
		v3f p_pos = m_pos * BS;
		v3f p_velocity = m_velocity * BS;
		collisionMoveResult r = collisionMoveSimple(m_env, m_gamedef, BS * 0.5f,
			box, 0.0f, dtime, &p_pos, &p_velocity, m_acceleration * BS, nullptr,
			m_object_collision);

		f32 bounciness = m_bounce.pickWithin();
		if (r.collides && (m_collision_removal || bounciness > 0)) {
			if (m_collision_removal) {
				// force expiration of the particle
				m_expiration = -1.0f;
			} else if (bounciness > 0) {
				/* cheap way to get a decent bounce effect is to only invert the
				 * largest component of the velocity vector, so e.g. you don't
				 * have a rock immediately bounce back in your face when you try
				 * to skip it across the water (as would happen if we simply
				 * downscaled and negated the velocity vector). this means
				 * bounciness will work properly for cubic objects, but meshes
				 * with diagonal angles and entities will not yield the correct
				 * visual. this is probably unavoidable */
				if (av.Y > av.X && av.Y > av.Z) {
					m_velocity.Y = -(m_velocity.Y * bounciness);
				} else if (av.X > av.Y && av.X > av.Z) {
					m_velocity.X = -(m_velocity.X * bounciness);
				} else if (av.Z > av.Y && av.Z > av.X) {
					m_velocity.Z = -(m_velocity.Z * bounciness);
				} else { // well now we're in a bit of a pickle
					m_velocity = -(m_velocity * bounciness);
				}
			}
		} else {
			m_velocity = p_velocity / BS;
		}
		m_pos = p_pos / BS;
	} else {
		// apply velocity and acceleration to position
		m_pos += (m_velocity + m_acceleration * 0.5f * dtime) * dtime;
		// apply acceleration to velocity
		m_velocity += m_acceleration * dtime;
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

	// animate particle alpha in accordance with settings
	if (m_texture.tex != nullptr)
		m_alpha = m_texture.tex -> alpha.blend(m_time / (m_expiration+0.1f));
	else
		m_alpha = 1.f;

	// Update lighting
	updateLight();

	// Update model
	updateVertices();

	// Update position -- see #10398
	v3s16 camera_offset = m_env->getCameraOffset();
	setPosition(m_pos*BS - intToFloat(camera_offset, BS));
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
		light = n.getLightBlend(m_env->getDayNightRatio(),
				m_gamedef->ndef()->getLightingFlags(n));
	else
		light = blend_light(m_env->getDayNightRatio(), LIGHT_SUN, 0);

	u8 m_light = decode_light(light + m_glow);
	m_color.set(m_alpha*255,
		m_light * m_base_color.getRed() / 255,
		m_light * m_base_color.getGreen() / 255,
		m_light * m_base_color.getBlue() / 255);
}

void Particle::updateVertices()
{
	f32 tx0, tx1, ty0, ty1;
	v2f scale;

	if (m_texture.tex != nullptr)
		scale = m_texture.tex -> scale.blend(m_time / (m_expiration+0.1));
	else
		scale = v2f(1.f, 1.f);

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

	auto half = m_size * .5f,
	     hx   = half * scale.X,
	     hy   = half * scale.Y;
	m_vertices[0] = video::S3DVertex(-hx, -hy,
		0, 0, 0, 0, m_color, tx0, ty1);
	m_vertices[1] = video::S3DVertex(hx, -hy,
		0, 0, 0, 0, m_color, tx1, ty1);
	m_vertices[2] = video::S3DVertex(hx, hy,
		0, 0, 0, 0, m_color, tx1, ty0);
	m_vertices[3] = video::S3DVertex(-hx, hy,
		0, 0, 0, 0, m_color, tx0, ty0);


	// see #10398
	// v3s16 camera_offset = m_env->getCameraOffset();
	// particle position is now handled by step()
	m_box.reset(v3f());

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
	std::unique_ptr<ClientTexture[]>& texpool,
	size_t texcount,
	ParticleManager *p_manager
):
	m_particlemanager(p_manager), p(p)
{
	m_gamedef = gamedef;
	m_player = player;
	m_attached_id = attached_id;
	m_texpool = std::move(texpool);
	m_texcount = texcount;
	m_time = 0;
	m_active = 0;
	m_dying = false;

	m_spawntimes.reserve(p.amount + 1);
	for (u16 i = 0; i <= p.amount; i++) {
		float spawntime = myrand_float() * p.time;
		m_spawntimes.push_back(spawntime);
	}

	size_t max_particles = 0; // maximum number of particles likely to be visible at any given time
	if (p.time != 0) {
		auto maxGenerations = p.time / std::min(p.exptime.start.min, p.exptime.end.min);
		max_particles = p.amount / maxGenerations;
	} else {
		auto longestLife = std::max(p.exptime.start.max, p.exptime.end.max);
		max_particles = p.amount * longestLife;
	}

	p_manager->reserveParticleSpace(max_particles * 1.2);
}

namespace {
	GenericCAO *findObjectByID(ClientEnvironment *env, u16 id) {
		if (id == 0)
			return nullptr;
		return env->getGenericCAO(id);
	}
}

void ParticleSpawner::spawnParticle(ClientEnvironment *env, float radius,
	const core::matrix4 *attached_absolute_pos_rot_matrix)
{
	float fac = 0;
	if (p.time != 0) { // ensure safety from divide-by-zeroes
		fac = m_time / (p.time+0.1f);
	}

	auto r_pos = p.pos.blend(fac);
	auto r_vel = p.vel.blend(fac);
	auto r_acc = p.acc.blend(fac);
	auto r_drag = p.drag.blend(fac);
	auto r_radius = p.radius.blend(fac);
	auto r_jitter = p.jitter.blend(fac);
	auto r_bounce = p.bounce.blend(fac);
	v3f  attractor_origin = p.attractor_origin.blend(fac);
	v3f  attractor_direction = p.attractor_direction.blend(fac);
	auto attractor_obj       = findObjectByID(env, p.attractor_attachment);
	auto attractor_direction_obj = findObjectByID(env, p.attractor_direction_attachment);

	auto r_exp = p.exptime.blend(fac);
	auto r_size = p.size.blend(fac);
	auto r_attract = p.attract.blend(fac);
	auto attract = r_attract.pickWithin();

	v3f ppos = m_player->getPosition() / BS;
	v3f pos = r_pos.pickWithin();
	v3f sphere_radius = r_radius.pickWithin();

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

	if (pos.getDistanceFromSQ(ppos) > radius*radius)
		return;

	// Parameters for the single particle we're about to spawn
	ParticleParameters pp;
	pp.pos = pos;

	pp.vel = r_vel.pickWithin();
	pp.acc = r_acc.pickWithin();
	pp.drag = r_drag.pickWithin();
	pp.jitter = r_jitter;
	pp.bounce = r_bounce;

	if (attached_absolute_pos_rot_matrix) {
		// Apply attachment rotation
		attached_absolute_pos_rot_matrix->rotateVect(pp.vel);
		attached_absolute_pos_rot_matrix->rotateVect(pp.acc);
	}

	if (attractor_obj)
		attractor_origin += attractor_obj->getPosition() / BS;
	if (attractor_direction_obj) {
		auto *attractor_absolute_pos_rot_matrix = attractor_direction_obj->getAbsolutePosRotMatrix();
		if (attractor_absolute_pos_rot_matrix)
			attractor_absolute_pos_rot_matrix->rotateVect(attractor_direction);
	}

	pp.expirationtime = r_exp.pickWithin();

	if (sphere_radius != v3f()) {
		f32 l = sphere_radius.getLength();
		v3f mag = sphere_radius;
		mag.normalize();

		v3f ofs = v3f(l,0,0);
		ofs.rotateXZBy(myrand_range(0.f,360.f));
		ofs.rotateYZBy(myrand_range(0.f,360.f));
		ofs.rotateXYBy(myrand_range(0.f,360.f));

		pp.pos += ofs * mag;
	}

	if (p.attractor_kind != ParticleParamTypes::AttractorKind::none && attract != 0) {
		v3f dir;
		f32 dist = 0; /* =0 necessary to silence warning */
		switch (p.attractor_kind) {
			case ParticleParamTypes::AttractorKind::none:
				break;

			case ParticleParamTypes::AttractorKind::point: {
				dist = pp.pos.getDistanceFrom(attractor_origin);
				dir = pp.pos - attractor_origin;
				dir.normalize();
				break;
			}

			case ParticleParamTypes::AttractorKind::line: {
				// https://github.com/minetest/minetest/issues/11505#issuecomment-915612700
				const auto& lorigin = attractor_origin;
				v3f ldir = attractor_direction;
				ldir.normalize();
				auto origin_to_point = pp.pos - lorigin;
				auto scalar_projection = origin_to_point.dotProduct(ldir);
				auto point_on_line = lorigin + (ldir * scalar_projection);

				dist = pp.pos.getDistanceFrom(point_on_line);
				dir = (point_on_line - pp.pos);
				dir.normalize();
				dir *= -1; // flip it around so strength=1 attracts, not repulses
				break;
			}

			case ParticleParamTypes::AttractorKind::plane: {
				// https://github.com/minetest/minetest/issues/11505#issuecomment-915612700
				const v3f& porigin = attractor_origin;
				v3f normal = attractor_direction;
				normal.normalize();
				v3f point_to_origin = porigin - pp.pos;
				f32 factor = normal.dotProduct(point_to_origin);
				if (numericAbsolute(factor) == 0.0f) {
					dir = normal;
				} else {
					factor = numericSign(factor);
					dir = normal * factor;
				}
				dist = numericAbsolute(normal.dotProduct(pp.pos - porigin));
				dir *= -1; // flip it around so strength=1 attracts, not repulses
				break;
			}
		}

		f32 speedTowards = numericAbsolute(attract) * dist;
		v3f avel = dir * speedTowards;
		if (attract > 0 && speedTowards > 0) {
			avel *= -1;
			if (p.attractor_kill) {
				// make sure the particle dies after crossing the attractor threshold
				f32 timeToCenter = dist / speedTowards;
				if (timeToCenter < pp.expirationtime)
					pp.expirationtime = timeToCenter;
			}
		}
		pp.vel += avel;
	}

	p.copyCommon(pp);

	ClientTexRef texture;
	v2f texpos, texsize;
	video::SColor color(0xFFFFFFFF);

	if (p.node.getContent() != CONTENT_IGNORE) {
		const ContentFeatures &f =
			m_particlemanager->m_env->getGameDef()->ndef()->get(p.node);
		if (!ParticleManager::getNodeParticleParams(p.node, f, pp, &texture.ref,
				texpos, texsize, &color, p.node_tile))
			return;
	} else {
		if (m_texcount == 0)
			return;
		texture = decltype(texture)(m_texpool[m_texcount == 1 ? 0 : myrand_range(0,m_texcount-1)]);
		texpos = v2f(0.0f, 0.0f);
		texsize = v2f(1.0f, 1.0f);
		if (texture.tex->animated)
			pp.animation = texture.tex->animation;
	}

	// synchronize animation length with particle life if desired
	if (pp.animation.type != TAT_NONE) {
		// FIXME: this should be moved into a TileAnimationParams class method
		if (pp.animation.type == TAT_VERTICAL_FRAMES &&
			pp.animation.vertical_frames.length < 0) {
			auto& a = pp.animation.vertical_frames;
			// we add a tiny extra value to prevent the first frame
			// from flickering back on just before the particle dies
			a.length = (pp.expirationtime / -a.length) + 0.1;
		} else if (pp.animation.type == TAT_SHEET_2D &&
				   pp.animation.sheet_2d.frame_length < 0) {
			auto& a = pp.animation.sheet_2d;
			auto frames = a.frames_w * a.frames_h;
			auto runtime = (pp.expirationtime / -a.frame_length) + 0.1;
			pp.animation.sheet_2d.frame_length = frames / runtime;
		}
	}

	// Allow keeping default random size
	if (p.size.start.max > 0.0f || p.size.end.max > 0.0f)
		pp.size = r_size.pickWithin();

	++m_active;
	auto pa = new Particle(
		m_gamedef,
		m_player,
		env,
		pp,
		texture,
		texpos,
		texsize,
		color
	);
	pa->m_parent = this;
	m_particlemanager->addParticle(pa);
}

void ParticleSpawner::step(float dtime, ClientEnvironment *env)
{
	m_time += dtime;

	static thread_local const float radius =
			g_settings->getS16("max_block_send_distance") * MAP_BLOCKSIZE;

	bool unloaded = false;
	const core::matrix4 *attached_absolute_pos_rot_matrix = nullptr;
	if (m_attached_id) {
		if (GenericCAO *attached = env->getGenericCAO(m_attached_id)) {
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
			if (myrand_float() < dtime)
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
		if (i->second->getExpired()) {
			// the particlespawner owns the textures, so we need to make
			// sure there are no active particles before we free it
			if (i->second->m_active == 0) {
				delete i->second;
				m_particle_spawners.erase(i++);
			} else {
				++i;
			}
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
			if ((*i)->m_parent) {
				assert((*i)->m_parent->m_active != 0);
				--(*i)->m_parent->m_active;
			}
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

			// texture pool
			std::unique_ptr<ClientTexture[]> texpool = nullptr;
			size_t txpsz = 0;
			if (!p.texpool.empty()) {
				txpsz = p.texpool.size();
				texpool = decltype(texpool)(new ClientTexture [txpsz]);

				for (size_t i = 0; i < txpsz; ++i) {
					texpool[i] = ClientTexture(p.texpool[i], client->tsrc());
				}
			} else {
				// no texpool in use, use fallback texture
				txpsz = 1;
				texpool = decltype(texpool)(new ClientTexture[1] {
					ClientTexture(p.texture, client->tsrc())
				});
			}

			auto toadd = new ParticleSpawner(client, player,
					p,
					event->add_particlespawner.attached_id,
					texpool,
					txpsz,
					this);

			addParticleSpawner(event->add_particlespawner.id, toadd);

			delete event->add_particlespawner.p;
			break;
		}
		case CE_SPAWN_PARTICLE: {
			ParticleParameters &p = *event->spawn_particle;

			ClientTexRef texture;
			v2f texpos, texsize;
			video::SColor color(0xFFFFFFFF);

			f32 oldsize = p.size;

			if (p.node.getContent() != CONTENT_IGNORE) {
				const ContentFeatures &f = m_env->getGameDef()->ndef()->get(p.node);
				getNodeParticleParams(p.node, f, p, &texture.ref, texpos,
						texsize, &color, p.node_tile);
			} else {
				/* with no particlespawner to own the texture, we need
				 * to save it on the heap. it will be freed when the
				 * particle is destroyed */
				auto texstore = new ClientTexture(p.texture, client->tsrc());

				texture = ClientTexRef(*texstore);
				texpos = v2f(0.0f, 0.0f);
				texsize = v2f(1.0f, 1.0f);
			}

			// Allow keeping default random size
			if (oldsize > 0.0f)
				p.size = oldsize;

			if (texture.ref) {
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
		texid = myrand_range(0,5);
	const TileLayer &tile = f.tiles[texid].layers[0];
	p.animation.type = TAT_NONE;

	// Only use first frame of animated texture
	if (tile.material_flags & MATERIAL_FLAG_ANIMATION)
		*texture = (*tile.frames)[0].texture;
	else
		*texture = tile.texture;

	float size = (myrand_range(0,8)) / 64.0f;
	p.size = BS * size;
	if (tile.scale)
		size /= tile.scale;
	texsize = v2f(size * 2.0f, size * 2.0f);
	texpos.X = (myrand_range(0,64)) / 64.0f - texsize.X;
	texpos.Y = (myrand_range(0,64)) / 64.0f - texsize.Y;

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
	video::ITexture *ref = nullptr;
	v2f texpos, texsize;
	video::SColor color;

	if (!getNodeParticleParams(n, f, p, &ref, texpos, texsize, &color))
		return;

	p.expirationtime = myrand_range(0, 100) / 100.0f;

	// Physics
	p.vel = v3f(
		myrand_range(-1.5f,1.5f),
		myrand_range(0.f,3.f),
		myrand_range(-1.5f,1.5f)
	);
	p.acc = v3f(
		0.0f,
		-player->movement_gravity * player->physics_override.gravity / BS,
		0.0f
	);
	p.pos = v3f(
		(f32)pos.X + myrand_range(0.f, .5f) - .25f,
		(f32)pos.Y + myrand_range(0.f, .5f) - .25f,
		(f32)pos.Z + myrand_range(0.f, .5f) - .25f
	);

	Particle *toadd = new Particle(
		gamedef,
		player,
		m_env,
		p,
		ClientTexRef(ref),
		texpos,
		texsize,
		color);

	addParticle(toadd);
}

void ParticleManager::reserveParticleSpace(size_t max_estimate)
{
	MutexAutoLock lock(m_particle_list_lock);
	m_particles.reserve(m_particles.size() + max_estimate);
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
		it->second->setDying();
	}
}
