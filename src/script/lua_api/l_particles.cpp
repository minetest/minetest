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

#include "lua_api/l_particles.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "server.h"

// add_particle({pos=, velocity=, acceleration=, expirationtime=,
// 		size=, collisiondetection=, vertical=, texture=, player=})
// pos/velocity/acceleration = {x=num, y=num, z=num}
// expirationtime = num (seconds)
// size = num
// collisiondetection = bool
// vertical = bool
// texture = e.g."default_wood.png"
int ModApiParticles::l_add_particle(lua_State *L)
{
	// Get parameters
	v3f pos, vel, acc;
	    pos= vel= acc= v3f(0, 0, 0);
	float expirationtime, size;
	      expirationtime= size= 1;
	bool collisiondetection, vertical;
	     collisiondetection= vertical= false;
	std::string texture = "";
	const char *playername = "";

	if (lua_gettop(L) > 1) // deprecated
	{
		log_deprecated(L,"Deprecated add_particle call with individual parameters instead of definition");
		pos = check_v3f(L, 1);
		vel = check_v3f(L, 2);
		acc = check_v3f(L, 3);
		expirationtime = luaL_checknumber(L, 4);
		size = luaL_checknumber(L, 5);
		collisiondetection = lua_toboolean(L, 6);
		texture = luaL_checkstring(L, 7);
		if (lua_gettop(L) == 8) // only spawn for a single player
			playername = luaL_checkstring(L, 8);
	}
	else if (lua_istable(L, 1))
	{
		int table = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, table) != 0)
		{
			const char *key = lua_tostring(L, -2);
				  if(strcmp(key,"pos")==0){
					pos=check_v3f(L, -1);
			}else if(strcmp(key,"vel")==0){
					vel=check_v3f(L, -1);
			}else if(strcmp(key,"acc")==0){
					acc=check_v3f(L, -1);
			}else if(strcmp(key,"expirationtime")==0){
					expirationtime=luaL_checknumber(L, -1);
			}else if(strcmp(key,"size")==0){
					size=luaL_checknumber(L, -1);
			}else if(strcmp(key,"collisiondetection")==0){
					collisiondetection=lua_toboolean(L, -1);
			}else if(strcmp(key,"vertical")==0){
					vertical=lua_toboolean(L, -1);
			}else if(strcmp(key,"texture")==0){
					texture=luaL_checkstring(L, -1);
			}else if(strcmp(key,"playername")==0){
					playername=luaL_checkstring(L, -1);
			}
			lua_pop(L, 1);
		}
	}
	if (strcmp(playername, "")==0) // spawn for all players
	{
		getServer(L)->spawnParticleAll(pos, vel, acc,
			expirationtime, size, collisiondetection, vertical, texture);
	}
	else
	{
		getServer(L)->spawnParticle(playername,
			pos, vel, acc, expirationtime,
			size, collisiondetection, vertical, texture);
	}
	return 1;
}

// add_particlespawner({amount=, time=,
//				minpos=, maxpos=,
//				minvel=, maxvel=,
//				minacc=, maxacc=,
//				minexptime=, maxexptime=,
//				minsize=, maxsize=,
//				collisiondetection=,
//				vertical=,
//				texture=,
//				player=})
// minpos/maxpos/minvel/maxvel/minacc/maxacc = {x=num, y=num, z=num}
// minexptime/maxexptime = num (seconds)
// minsize/maxsize = num
// collisiondetection = bool
// vertical = bool
// texture = e.g."default_wood.png"
int ModApiParticles::l_add_particlespawner(lua_State *L)
{
	// Get parameters
	u16 amount = 1;
	v3f minpos, maxpos, minvel, maxvel, minacc, maxacc;
	    minpos= maxpos= minvel= maxvel= minacc= maxacc= v3f(0, 0, 0);
	float time, minexptime, maxexptime, minsize, maxsize;
	      time= minexptime= maxexptime= minsize= maxsize= 1;
	bool collisiondetection, vertical;
	     collisiondetection= vertical= false;
	std::string texture = "";
	const char *playername = "";

	if (lua_gettop(L) > 1) //deprecated
	{
		log_deprecated(L,"Deprecated add_particlespawner call with individual parameters instead of definition");
		amount = luaL_checknumber(L, 1);
		time = luaL_checknumber(L, 2);
		minpos = check_v3f(L, 3);
		maxpos = check_v3f(L, 4);
		minvel = check_v3f(L, 5);
		maxvel = check_v3f(L, 6);
		minacc = check_v3f(L, 7);
		maxacc = check_v3f(L, 8);
		minexptime = luaL_checknumber(L, 9);
		maxexptime = luaL_checknumber(L, 10);
		minsize = luaL_checknumber(L, 11);
		maxsize = luaL_checknumber(L, 12);
		collisiondetection = lua_toboolean(L, 13);
		texture = luaL_checkstring(L, 14);
		if (lua_gettop(L) == 15) // only spawn for a single player
			playername = luaL_checkstring(L, 15);
	}
	else if (lua_istable(L, 1))
	{
		int table = lua_gettop(L);
		lua_pushnil(L);
		while (lua_next(L, table) != 0)
		{
			const char *key = lua_tostring(L, -2);
			      if(strcmp(key,"amount")==0){
					amount=luaL_checknumber(L, -1);
			}else if(strcmp(key,"time")==0){
					time=luaL_checknumber(L, -1);
			}else if(strcmp(key,"minpos")==0){
					minpos=check_v3f(L, -1);
			}else if(strcmp(key,"maxpos")==0){
					maxpos=check_v3f(L, -1);
			}else if(strcmp(key,"minvel")==0){
					minvel=check_v3f(L, -1);
			}else if(strcmp(key,"maxvel")==0){
					maxvel=check_v3f(L, -1);
			}else if(strcmp(key,"minacc")==0){
					minacc=check_v3f(L, -1);
			}else if(strcmp(key,"maxacc")==0){
					maxacc=check_v3f(L, -1);
			}else if(strcmp(key,"minexptime")==0){
					minexptime=luaL_checknumber(L, -1);
			}else if(strcmp(key,"maxexptime")==0){
					maxexptime=luaL_checknumber(L, -1);
			}else if(strcmp(key,"minsize")==0){
					minsize=luaL_checknumber(L, -1);
			}else if(strcmp(key,"maxsize")==0){
					maxsize=luaL_checknumber(L, -1);
			}else if(strcmp(key,"collisiondetection")==0){
					collisiondetection=lua_toboolean(L, -1);
			}else if(strcmp(key,"vertical")==0){
					vertical=lua_toboolean(L, -1);
			}else if(strcmp(key,"texture")==0){
					texture=luaL_checkstring(L, -1);
			}else if(strcmp(key,"playername")==0){
					playername=luaL_checkstring(L, -1);
			}
			lua_pop(L, 1);
		}
	}
	if (strcmp(playername, "")==0) //spawn for all players
	{
		u32 id = getServer(L)->addParticleSpawnerAll(	amount, time,
							minpos, maxpos,
							minvel, maxvel,
							minacc, maxacc,
							minexptime, maxexptime,
							minsize, maxsize,
							collisiondetection,
							vertical,
							texture);
		lua_pushnumber(L, id);
	}
	else
	{
		u32 id = getServer(L)->addParticleSpawner(playername,
							amount, time,
							minpos, maxpos,
							minvel, maxvel,
							minacc, maxacc,
							minexptime, maxexptime,
							minsize, maxsize,
							collisiondetection,
							vertical,
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

void ModApiParticles::Initialize(lua_State *L, int top)
{
	API_FCT(add_particle);
	API_FCT(add_particlespawner);
	API_FCT(delete_particlespawner);
}

