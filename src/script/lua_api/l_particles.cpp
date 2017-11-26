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
#include "lua_api/l_object.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "util/serialize.h"
#include "server.h"
#include "particles.h"

// Takes as input a list of bytecode commands formatted as tables.
std::string read_particle_bytecode(lua_State *L, int index)
{
	typedef ParticleShaders ps;

	if (lua_isnoneornil(L, index))
		return "";

	std::stringstream os;
	
	luaL_checktype(L, index, LUA_TTABLE);
	const size_t opCount = lua_objlen(L, index);

	for (size_t i = 1; i <= opCount; ++i) {
		lua_rawgeti(L, index, i);
		luaL_checktype(L, -1, LUA_TTABLE);

		// Write the opcode
		lua_getfield(L, -1, "operation");
		const u8 opcode = (u8) luaL_checkint(L, -1);
		lua_pop(L, 1);

		writeU8(os, opcode);

		// Write arguments
		switch(opcode) {
		case ps::op_push_constant:
			lua_rawgeti(L, -1, 1);
			writeF1000(os, luaL_checknumber(L, -1));
			lua_pop(L, 1);
			break;
		case ps::op_push_variable:
		case ps::op_write_variable:
			lua_rawgeti(L, -1, 1);
			writeU32(os, luaL_checkint(L, -1));
			lua_pop(L, 1);
			break;
		default:
			break;
		}
		lua_pop(L, 1);
	}

	return os.str();
}

// Returns true if and only if the field is non-nil
bool getfield_or_zero(lua_State *L, int index, const char* field)
{
	lua_getfield(L, index, field);
	if (lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		lua_pushinteger(L, 0);
		return false;
	}

	return true;
}

void read_particle_shaders(lua_State *L, int index,
	ParticleShaders& shaders)
{
	// We will call the lua-defined compile function
	lua_getglobal(L, "expression");
	lua_getfield(L, -1, "compile");
	
	// Make argument to compile
	lua_newtable(L);

	bool posGood = getfield_or_zero(L, index, "pos_expression");
	lua_rawseti(L, -2, 1);

	bool velGood = getfield_or_zero(L, index, "vel_expression");
	lua_rawseti(L, -2, 2);

	bool accGood = getfield_or_zero(L, index, "acc_expression");
	lua_rawseti(L, -2, 3);

	bool expGood = getfield_or_zero(L, index, "exptime_expression");
	lua_rawseti(L, -2, 4);

	bool sizeGood = getfield_or_zero(L, index, "size_expression");
	lua_rawseti(L, -2, 5);

	// Put a list of compiled exprs and the variable count on the stack
	lua_call(L, 1, 2);

	shaders.varCount = luaL_checkint(L, -1);

	if (posGood) {
		lua_rawgeti(L, -2, 1);
		shaders.pos = read_particle_bytecode(L, -1);
		lua_pop(L, 1);
	}

	if (velGood) {
		lua_rawgeti(L, -2, 2);
		shaders.vel = read_particle_bytecode(L, -1);
		lua_pop(L, 1);
	}

	if (accGood) {
		lua_rawgeti(L, -2, 3);
		shaders.acc = read_particle_bytecode(L, -1);
		lua_pop(L, 1);
	}

	if (expGood) {
		lua_rawgeti(L, -2, 4);
		shaders.exptime = read_particle_bytecode(L, -1);
		lua_pop(L, 1);
	}

	if (sizeGood) {
		lua_rawgeti(L, -2, 5);
		shaders.size = read_particle_bytecode(L, -1);
		lua_pop(L, 1);
	}
}

// add_particle({pos=, velocity=, acceleration=, expirationtime=,
// 		size=, collisiondetection=, collision_removal=, vertical=,
//		texture=, player=})
// pos/velocity/acceleration = {x=num, y=num, z=num}
// expirationtime = num (seconds)
// size = num
// collisiondetection = bool
// collision_removal = bool
// vertical = bool
// texture = e.g."default_wood.png"
// animation = TileAnimation definition
// glow = num
int ModApiParticles::l_add_particle(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	// Get parameters
	v3f pos, vel, acc;
	pos = vel = acc = v3f(0, 0, 0);

	float expirationtime, size;
	expirationtime = size = 1;

	bool collisiondetection, vertical, collision_removal;
	collisiondetection = vertical = collision_removal = false;
	struct TileAnimationParams animation;
	animation.type = TAT_NONE;

	std::string texture;
	std::string playername;

	u8 glow = 0;

	if (lua_gettop(L) > 1) // deprecated
	{
		log_deprecated(L, "Deprecated add_particle call with individual parameters instead of definition");
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
		lua_getfield(L, 1, "pos");
		pos = lua_istable(L, -1) ? check_v3f(L, -1) : v3f();
		lua_pop(L, 1);

		lua_getfield(L, 1, "vel");
		if (lua_istable(L, -1)) {
			vel = check_v3f(L, -1);
			log_deprecated(L, "The use of vel is deprecated. "
				"Use velocity instead");
		}
		lua_pop(L, 1);

		lua_getfield(L, 1, "velocity");
		vel = lua_istable(L, -1) ? check_v3f(L, -1) : vel;
		lua_pop(L, 1);

		lua_getfield(L, 1, "acc");
		if (lua_istable(L, -1)) {
			acc = check_v3f(L, -1);
			log_deprecated(L, "The use of acc is deprecated. "
				"Use acceleration instead");
		}
		lua_pop(L, 1);

		lua_getfield(L, 1, "acceleration");
		acc = lua_istable(L, -1) ? check_v3f(L, -1) : acc;
		lua_pop(L, 1);

		expirationtime = getfloatfield_default(L, 1, "expirationtime", 1);
		size = getfloatfield_default(L, 1, "size", 1);
		collisiondetection = getboolfield_default(L, 1,
			"collisiondetection", collisiondetection);
		collision_removal = getboolfield_default(L, 1,
			"collision_removal", collision_removal);
		vertical = getboolfield_default(L, 1, "vertical", vertical);

		lua_getfield(L, 1, "animation");
		animation = read_animation_definition(L, -1);
		lua_pop(L, 1);

		texture = getstringfield_default(L, 1, "texture", "");
		playername = getstringfield_default(L, 1, "playername", "");

		glow = getintfield_default(L, 1, "glow", 0);
	}
	getServer(L)->spawnParticle(playername, pos, vel, acc, expirationtime, size,
			collisiondetection, collision_removal, vertical, texture, animation, glow);
	return 1;
}

// add_particlespawner({amount=, time=,
//				minpos=, maxpos=,
//				minvel=, maxvel=,
//				minacc=, maxacc=,
//				minexptime=, maxexptime=,
//				minsize=, maxsize=,
//				collisiondetection=,
//				collision_removal=,
//				vertical=,
//				texture=,
//				player=})
// minpos/maxpos/minvel/maxvel/minacc/maxacc = {x=num, y=num, z=num}
// minexptime/maxexptime = num (seconds)
// minsize/maxsize = num
// collisiondetection = bool
// collision_removal = bool
// vertical = bool
// texture = e.g."default_wood.png"
// animation = TileAnimation definition
// glow = num
int ModApiParticles::l_add_particlespawner(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	// Get parameters
	u16 amount = 1;
	v3f minpos, maxpos, minvel, maxvel, minacc, maxacc;
	    minpos= maxpos= minvel= maxvel= minacc= maxacc= v3f(0, 0, 0);
	float time, minexptime, maxexptime, minsize, maxsize;
	      time= minexptime= maxexptime= minsize= maxsize= 1;
	bool collisiondetection, vertical, collision_removal;
	     collisiondetection = vertical = collision_removal = false;
	struct TileAnimationParams animation;
	animation.type = TAT_NONE;
	ServerActiveObject *attached = NULL;
	std::string texture;
	std::string playername;
	u8 glow = 0;
	ParticleShaders shaders;

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
		amount = getintfield_default(L, 1, "amount", amount);
		time = getfloatfield_default(L, 1, "time", time);

		lua_getfield(L, 1, "minpos");
		minpos = lua_istable(L, -1) ? check_v3f(L, -1) : minpos;
		lua_pop(L, 1);

		lua_getfield(L, 1, "maxpos");
		maxpos = lua_istable(L, -1) ? check_v3f(L, -1) : maxpos;
		lua_pop(L, 1);

		lua_getfield(L, 1, "minvel");
		minvel = lua_istable(L, -1) ? check_v3f(L, -1) : minvel;
		lua_pop(L, 1);

		lua_getfield(L, 1, "maxvel");
		maxvel = lua_istable(L, -1) ? check_v3f(L, -1) : maxvel;
		lua_pop(L, 1);

		lua_getfield(L, 1, "minacc");
		minacc = lua_istable(L, -1) ? check_v3f(L, -1) : minacc;
		lua_pop(L, 1);

		lua_getfield(L, 1, "maxacc");
		maxacc = lua_istable(L, -1) ? check_v3f(L, -1) : maxacc;
		lua_pop(L, 1);

		minexptime = getfloatfield_default(L, 1, "minexptime", minexptime);
		maxexptime = getfloatfield_default(L, 1, "maxexptime", maxexptime);
		minsize = getfloatfield_default(L, 1, "minsize", minsize);
		maxsize = getfloatfield_default(L, 1, "maxsize", maxsize);
		collisiondetection = getboolfield_default(L, 1,
			"collisiondetection", collisiondetection);
		collision_removal = getboolfield_default(L, 1,
			"collision_removal", collision_removal);

		lua_getfield(L, 1, "animation");
		animation = read_animation_definition(L, -1);
		lua_pop(L, 1);

		lua_getfield(L, 1, "attached");
		if (!lua_isnil(L, -1)) {
			ObjectRef *ref = ObjectRef::checkobject(L, -1);
			lua_pop(L, 1);
			attached = ObjectRef::getobject(ref);
		}

		vertical = getboolfield_default(L, 1, "vertical", vertical);
		texture = getstringfield_default(L, 1, "texture", "");
		playername = getstringfield_default(L, 1, "playername", "");
		glow = getintfield_default(L, 1, "glow", 0);
		read_particle_shaders(L, 1, shaders);
	}

	u32 id = getServer(L)->addParticleSpawner(amount, time,
			minpos, maxpos,
			minvel, maxvel,
			minacc, maxacc,
			minexptime, maxexptime,
			minsize, maxsize,
			collisiondetection,
			collision_removal,
			attached,
			vertical,
			texture, playername,
			animation, glow, shaders);
	lua_pushnumber(L, id);

	return 1;
}

// delete_particlespawner(id, player)
// player (string) is optional
int ModApiParticles::l_delete_particlespawner(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	// Get parameters
	u32 id = luaL_checknumber(L, 1);
	std::string playername;
	if (lua_gettop(L) == 2) {
		playername = luaL_checkstring(L, 2);
	}

	getServer(L)->deleteParticleSpawner(playername, id);
	return 1;
}

void ModApiParticles::Initialize(lua_State *L, int top)
{
	API_FCT(add_particle);
	API_FCT(add_particlespawner);
	API_FCT(delete_particlespawner);
}

