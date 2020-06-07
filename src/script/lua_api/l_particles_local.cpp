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
	ParticleParameters p;

	lua_getfield(L, 1, "pos");
	if (lua_istable(L, -1))
		p.pos = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "velocity");
	if (lua_istable(L, -1))
		p.vel = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "acceleration");
	if (lua_istable(L, -1))
		p.acc = check_v3f(L, -1);
	lua_pop(L, 1);

	p.expirationtime = getfloatfield_default(L, 1, "expirationtime",
		p.expirationtime);
	p.size = getfloatfield_default(L, 1, "size", p.size);
	p.collisiondetection = getboolfield_default(L, 1,
		"collisiondetection", p.collisiondetection);
	p.collision_removal = getboolfield_default(L, 1,
		"collision_removal", p.collision_removal);
	p.object_collision = getboolfield_default(L, 1,
		"object_collision", p.object_collision);
	p.vertical = getboolfield_default(L, 1, "vertical", p.vertical);

	lua_getfield(L, 1, "animation");
	p.animation = read_animation_definition(L, -1);
	lua_pop(L, 1);

	p.texture = getstringfield_default(L, 1, "texture", p.texture);
	p.glow = getintfield_default(L, 1, "glow", p.glow);

	lua_getfield(L, 1, "node");
	if (lua_istable(L, -1))
		p.node = readnode(L, -1, getGameDef(L)->ndef());
	lua_pop(L, 1);

	p.node_tile = getintfield_default(L, 1, "node_tile", p.node_tile);

	ClientEvent *event = new ClientEvent();
	event->type           = CE_SPAWN_PARTICLE;
	event->spawn_particle = new ParticleParameters(p);
	getClient(L)->pushToEventQueue(event);

	return 0;
}

int ModApiParticlesLocal::l_add_particlespawner(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	// Get parameters
	ParticleSpawnerParameters p;

	p.amount = getintfield_default(L, 1, "amount", p.amount);
	p.time = getfloatfield_default(L, 1, "time", p.time);

	lua_getfield(L, 1, "minpos");
	if (lua_istable(L, -1))
		p.minpos = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "maxpos");
	if (lua_istable(L, -1))
		p.maxpos = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "minvel");
	if (lua_istable(L, -1))
		p.minvel = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "maxvel");
	if (lua_istable(L, -1))
		p.maxvel = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "minacc");
	if (lua_istable(L, -1))
		p.minacc = check_v3f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, 1, "maxacc");
	if (lua_istable(L, -1))
		p.maxacc = check_v3f(L, -1);
	lua_pop(L, 1);

	p.minexptime = getfloatfield_default(L, 1, "minexptime", p.minexptime);
	p.maxexptime = getfloatfield_default(L, 1, "maxexptime", p.maxexptime);
	p.minsize = getfloatfield_default(L, 1, "minsize", p.minsize);
	p.maxsize = getfloatfield_default(L, 1, "maxsize", p.maxsize);
	p.collisiondetection = getboolfield_default(L, 1,
		"collisiondetection", p.collisiondetection);
	p.collision_removal = getboolfield_default(L, 1,
		"collision_removal", p.collision_removal);
	p.object_collision = getboolfield_default(L, 1,
		"object_collision", p.object_collision);

	lua_getfield(L, 1, "animation");
	p.animation = read_animation_definition(L, -1);
	lua_pop(L, 1);

	p.vertical = getboolfield_default(L, 1, "vertical", p.vertical);
	p.texture = getstringfield_default(L, 1, "texture", p.texture);
	p.glow = getintfield_default(L, 1, "glow", p.glow);

	lua_getfield(L, 1, "node");
	if (lua_istable(L, -1))
		p.node = readnode(L, -1, getGameDef(L)->ndef());
	lua_pop(L, 1);

	p.node_tile = getintfield_default(L, 1, "node_tile", p.node_tile);

	u64 id = getClient(L)->getParticleManager()->generateSpawnerId();

	auto event = new ClientEvent();
	event->type                            = CE_ADD_PARTICLESPAWNER;
	event->add_particlespawner.p           = new ParticleSpawnerParameters(p);
	event->add_particlespawner.attached_id = 0;
	event->add_particlespawner.id          = id;

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
