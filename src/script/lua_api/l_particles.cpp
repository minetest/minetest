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

#include "cpp_api/scriptapi.h"
#include "common/c_converter.h"
#include "lua_api/l_base.h"
#include "lua_api/l_particles.h"
#include "server.h"
#include "common/c_internal.h"

bool ModApiParticles::Initialize(lua_State *L, int top) {
	bool retval = true;

	retval &= API_FCT(add_particle);
	retval &= API_FCT(add_particlespawner);
	retval &= API_FCT(delete_particlespawner);

	return retval;
}

// add_particle(pos, velocity, acceleration, expirationtime,
// 		size, collisiondetection, texture, player)
// pos/velocity/acceleration = {x=num, y=num, z=num}
// expirationtime = num (seconds)
// size = num
// texture = e.g."default_wood.png"
int ModApiParticles::l_add_particle(lua_State *L)
{
	// Get parameters
	v3f pos = check_v3f(L, 1);
	v3f vel = check_v3f(L, 2);
	v3f acc = check_v3f(L, 3);
	float expirationtime = luaL_checknumber(L, 4);
	float size = luaL_checknumber(L, 5);
	bool collisiondetection = lua_toboolean(L, 6);
	std::string texture = luaL_checkstring(L, 7);

	if (lua_gettop(L) == 8) // only spawn for a single player
	{
		const char *playername = luaL_checkstring(L, 8);
		getServer(L)->spawnParticle(playername,
			pos, vel, acc, expirationtime,
			size, collisiondetection, texture);
	}
	else // spawn for all players
	{
		getServer(L)->spawnParticleAll(pos, vel, acc,
			expirationtime, size, collisiondetection, texture);
	}
	return 1;
}

// add_particlespawner(amount, time,
//				minpos, maxpos,
//				minvel, maxvel,
//				minacc, maxacc,
//				minexptime, maxexptime,
//				minsize, maxsize,
//				collisiondetection,
//				texture,
//				player)
// minpos/maxpos/minvel/maxvel/minacc/maxacc = {x=num, y=num, z=num}
// minexptime/maxexptime = num (seconds)
// minsize/maxsize = num
// collisiondetection = bool
// texture = e.g."default_wood.png"
int ModApiParticles::l_add_particlespawner(lua_State *L)
{
	// Get parameters
	u16 amount = luaL_checknumber(L, 1);
	float time = luaL_checknumber(L, 2);
	v3f minpos = check_v3f(L, 3);
	v3f maxpos = check_v3f(L, 4);
	v3f minvel = check_v3f(L, 5);
	v3f maxvel = check_v3f(L, 6);
	v3f minacc = check_v3f(L, 7);
	v3f maxacc = check_v3f(L, 8);
	float minexptime = luaL_checknumber(L, 9);
	float maxexptime = luaL_checknumber(L, 10);
	float minsize = luaL_checknumber(L, 11);
	float maxsize = luaL_checknumber(L, 12);
	bool collisiondetection = lua_toboolean(L, 13);
	std::string texture = luaL_checkstring(L, 14);

	if (lua_gettop(L) == 15) // only spawn for a single player
	{
		const char *playername = luaL_checkstring(L, 15);
		u32 id = getServer(L)->addParticleSpawner(playername,
							amount, time,
							minpos, maxpos,
							minvel, maxvel,
							minacc, maxacc,
							minexptime, maxexptime,
							minsize, maxsize,
							collisiondetection,
							texture);
		lua_pushnumber(L, id);
	}
	else // spawn for all players
	{
		u32 id = getServer(L)->addParticleSpawnerAll(	amount, time,
							minpos, maxpos,
							minvel, maxvel,
							minacc, maxacc,
							minexptime, maxexptime,
							minsize, maxsize,
							collisiondetection,
							texture);
		lua_pushnumber(L, id);
	}
	return 1;
}

// delete_particlespawner(id, player)
// player (string) is optional
int ModApiParticles::l_delete_particlespawner(lua_State *L)
{
	// Get parameters
	u32 id = luaL_checknumber(L, 1);

	if (lua_gettop(L) == 2) // only delete for one player
	{
		const char *playername = luaL_checkstring(L, 2);
		getServer(L)->deleteParticleSpawner(playername, id);
	}
	else // delete for all players
	{
		getServer(L)->deleteParticleSpawnerAll(id);
	}
	return 1;
}

ModApiParticles modapiparticles_prototyp;
