/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2017 red-001 <red-001@outlook.ie>

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

#include "lua_api/l_particles_local.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_object.h"
#include "client/particles.h"
#include "client/client.h"
#include "client/clientevent.h"

int ModApiParticlesLocal::l_add_particle(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	// Get parameters
	v3f pos, vel, acc;
	float expirationtime, size;
	bool collisiondetection, vertical, collision_removal;

	struct TileAnimationParams animation;
	animation.type = TAT_NONE;

	std::string texture;

	u8 glow;

	lua_getfield(L, 1, "pos");
	pos = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "velocity");
	vel = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "acceleration");
	acc = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	expirationtime = getfloatfield_default(L, 1, "expirationtime", 1);
	size = getfloatfield_default(L, 1, "size", 1);
	collisiondetection = getboolfield_default(L, 1, "collisiondetection", false);
	collision_removal = getboolfield_default(L, 1, "collision_removal", false);
	vertical = getboolfield_default(L, 1, "vertical", false);

	lua_getfield(L, 1, "animation");
	animation = read_animation_definition(L, -1);
	lua_pop(L, 1);

	texture = getstringfield_default(L, 1, "texture", "");

	glow = getintfield_default(L, 1, "glow", 0);

	ClientEvent *event = new ClientEvent();
	event->type                              = CE_SPAWN_PARTICLE;
	event->spawn_particle.pos                = new v3f (pos);
	event->spawn_particle.vel                = new v3f (vel);
	event->spawn_particle.acc                = new v3f (acc);
	event->spawn_particle.expirationtime     = expirationtime;
	event->spawn_particle.size               = size;
	event->spawn_particle.collisiondetection = collisiondetection;
	event->spawn_particle.collision_removal  = collision_removal;
	event->spawn_particle.vertical           = vertical;
	event->spawn_particle.texture            = new std::string(texture);
	event->spawn_particle.animation          = animation;
	event->spawn_particle.glow               = glow;
	getClient(L)->pushToEventQueue(event);

	return 0;
}

int ModApiParticlesLocal::l_add_particlespawner(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	// Get parameters
	u16 amount;
	v3f minpos, maxpos, minvel, maxvel, minacc, maxacc;
	float time, minexptime, maxexptime, minsize, maxsize;
	bool collisiondetection, vertical, collision_removal;

	struct TileAnimationParams animation;
	animation.type = TAT_NONE;
	// TODO: Implement this when there is a way to get an objectref.
	// ServerActiveObject *attached = NULL;
	std::string texture;
	u8 glow;

	amount = getintfield_default(L, 1, "amount", 1);
	time = getfloatfield_default(L, 1, "time", 1);

	lua_getfield(L, 1, "minpos");
	minpos = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "maxpos");
	maxpos = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "minvel");
	minvel = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "maxvel");
	maxvel = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "minacc");
	minacc = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	lua_getfield(L, 1, "maxacc");
	maxacc = lua_istable(L, -1) ? check_v3f(L, -1) : v3f(0, 0, 0);
	lua_pop(L, 1);

	minexptime = getfloatfield_default(L, 1, "minexptime", 1);
	maxexptime = getfloatfield_default(L, 1, "maxexptime", 1);
	minsize = getfloatfield_default(L, 1, "minsize", 1);
	maxsize = getfloatfield_default(L, 1, "maxsize", 1);

	collisiondetection = getboolfield_default(L, 1, "collisiondetection", false);
	collision_removal = getboolfield_default(L, 1, "collision_removal", false);
	vertical = getboolfield_default(L, 1, "vertical", false);

	lua_getfield(L, 1, "animation");
	animation = read_animation_definition(L, -1);
	lua_pop(L, 1);

	// TODO: Implement this when a way to get an objectref on the client is added
//	lua_getfield(L, 1, "attached");
//	if (!lua_isnil(L, -1)) {
//		ObjectRef *ref = ObjectRef::checkobject(L, -1);
//		lua_pop(L, 1);
//		attached = ObjectRef::getobject(ref);
//	}

	texture = getstringfield_default(L, 1, "texture", "");
	glow = getintfield_default(L, 1, "glow", 0);

	u64 id = getClient(L)->getParticleManager()->generateSpawnerId();

	auto event = new ClientEvent();
	event->type                                   = CE_ADD_PARTICLESPAWNER;
	event->add_particlespawner.amount             = amount;
	event->add_particlespawner.spawntime          = time;
	event->add_particlespawner.minpos             = new v3f (minpos);
	event->add_particlespawner.maxpos             = new v3f (maxpos);
	event->add_particlespawner.minvel             = new v3f (minvel);
	event->add_particlespawner.maxvel             = new v3f (maxvel);
	event->add_particlespawner.minacc             = new v3f (minacc);
	event->add_particlespawner.maxacc             = new v3f (maxacc);
	event->add_particlespawner.minexptime         = minexptime;
	event->add_particlespawner.maxexptime         = maxexptime;
	event->add_particlespawner.minsize            = minsize;
	event->add_particlespawner.maxsize            = maxsize;
	event->add_particlespawner.collisiondetection = collisiondetection;
	event->add_particlespawner.collision_removal  = collision_removal;
	event->add_particlespawner.attached_id        = 0;
	event->add_particlespawner.vertical           = vertical;
	event->add_particlespawner.texture            = new std::string(texture);
	event->add_particlespawner.id                 = id;
	event->add_particlespawner.animation          = animation;
	event->add_particlespawner.glow               = glow;

	getClient(L)->pushToEventQueue(event);
	lua_pushnumber(L, id);

	return 1;
}

int ModApiParticlesLocal::l_delete_particlespawner(lua_State *L)
{
	// Get parameters
	u32 id = luaL_checknumber(L, 1);

	ClientEvent *event = new ClientEvent();
	event->type                      = CE_DELETE_PARTICLESPAWNER;
	event->delete_particlespawner.id = id;

	getClient(L)->pushToEventQueue(event);
	return 0;
}

void ModApiParticlesLocal::Initialize(lua_State *L, int top)
{
	API_FCT(add_particle);
	API_FCT(add_particlespawner);
	API_FCT(delete_particlespawner);
}
