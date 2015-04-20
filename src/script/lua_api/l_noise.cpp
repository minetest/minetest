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

#include "lua_api/l_noise.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "log.h"

///////////////////////////////////////
/*
  LuaPerlinNoise
*/

LuaPerlinNoise::LuaPerlinNoise(NoiseParams *params) :
	np(*params)
{
}


LuaPerlinNoise::~LuaPerlinNoise()
{
}


int LuaPerlinNoise::l_get2d(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaPerlinNoise *o = checkobject(L, 1);
	v2f p = check_v2f(L, 2);
	lua_Number val = NoisePerlin2D(&o->np, p.X, p.Y, 0);
	lua_pushnumber(L, val);
	return 1;
}


int LuaPerlinNoise::l_get3d(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaPerlinNoise *o = checkobject(L, 1);
	v3f p = check_v3f(L, 2);
	lua_Number val = NoisePerlin3D(&o->np, p.X, p.Y, p.Z, 0);
	lua_pushnumber(L, val);
	return 1;
}


int LuaPerlinNoise::create_object(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	NoiseParams params;

	if (lua_istable(L, 1)) {
		read_noiseparams(L, 1, &params);
	} else {
		params.seed    = luaL_checkint(L, 1);
		params.octaves = luaL_checkint(L, 2);
		params.persist = luaL_checknumber(L, 3);
		params.spread  = v3f(1, 1, 1) * luaL_checknumber(L, 4);
	}

	LuaPerlinNoise *o = new LuaPerlinNoise(&params);

	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}


int LuaPerlinNoise::gc_object(lua_State *L)
{
	LuaPerlinNoise *o = *(LuaPerlinNoise **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}


LuaPerlinNoise *LuaPerlinNoise::checkobject(lua_State *L, int narg)
{
	NO_MAP_LOCK_REQUIRED;
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	return *(LuaPerlinNoise **)ud;
}


void LuaPerlinNoise::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_register(L, className, create_object);
}


const char LuaPerlinNoise::className[] = "PerlinNoise";
const luaL_reg LuaPerlinNoise::methods[] = {
	luamethod(LuaPerlinNoise, get2d),
	luamethod(LuaPerlinNoise, get3d),
	{0,0}
};

///////////////////////////////////////
/*
  LuaPerlinNoiseMap
*/

LuaPerlinNoiseMap::LuaPerlinNoiseMap(NoiseParams *params, int seed, v3s16 size)
{
	m_is3d = size.Z > 1;
	np = *params;
	try {
		noise = new Noise(&np, seed, size.X, size.Y, size.Z);
	} catch (InvalidNoiseParamsException &e) {
		throw LuaError(e.what());
	}
}


LuaPerlinNoiseMap::~LuaPerlinNoiseMap()
{
	delete noise;
}


int LuaPerlinNoiseMap::l_get2dMap(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	size_t i = 0;

	LuaPerlinNoiseMap *o = checkobject(L, 1);
	v2f p = check_v2f(L, 2);

	Noise *n = o->noise;
	n->perlinMap2D(p.X, p.Y);

	lua_newtable(L);
	for (int y = 0; y != n->sy; y++) {
		lua_newtable(L);
		for (int x = 0; x != n->sx; x++) {
			lua_pushnumber(L, n->result[i++]);
			lua_rawseti(L, -2, x + 1);
		}
		lua_rawseti(L, -2, y + 1);
	}
	return 1;
}


int LuaPerlinNoiseMap::l_get2dMap_flat(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPerlinNoiseMap *o = checkobject(L, 1);
	v2f p = check_v2f(L, 2);

	Noise *n = o->noise;
	n->perlinMap2D(p.X, p.Y);

	size_t maplen = n->sx * n->sy;

	lua_newtable(L);
	for (size_t i = 0; i != maplen; i++) {
		lua_pushnumber(L, n->result[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}


int LuaPerlinNoiseMap::l_get3dMap(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	size_t i = 0;

	LuaPerlinNoiseMap *o = checkobject(L, 1);
	v3f p = check_v3f(L, 2);

	if (!o->m_is3d)
		return 0;

	Noise *n = o->noise;
	n->perlinMap3D(p.X, p.Y, p.Z);

	lua_newtable(L);
	for (int z = 0; z != n->sz; z++) {
		lua_newtable(L);
		for (int y = 0; y != n->sy; y++) {
			lua_newtable(L);
			for (int x = 0; x != n->sx; x++) {
				lua_pushnumber(L, n->result[i++]);
				lua_rawseti(L, -2, x + 1);
			}
			lua_rawseti(L, -2, y + 1);
		}
		lua_rawseti(L, -2, z + 1);
	}
	return 1;
}


int LuaPerlinNoiseMap::l_get3dMap_flat(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPerlinNoiseMap *o = checkobject(L, 1);
	v3f p = check_v3f(L, 2);

	if (!o->m_is3d)
		return 0;

	Noise *n = o->noise;
	n->perlinMap3D(p.X, p.Y, p.Z);

	size_t maplen = n->sx * n->sy * n->sz;

	lua_newtable(L);
	for (size_t i = 0; i != maplen; i++) {
		lua_pushnumber(L, n->result[i]);
		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}


int LuaPerlinNoiseMap::create_object(lua_State *L)
{
	NoiseParams np;
	if (!read_noiseparams(L, 1, &np))
		return 0;
	v3s16 size = read_v3s16(L, 2);

	LuaPerlinNoiseMap *o = new LuaPerlinNoiseMap(&np, 0, size);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}


int LuaPerlinNoiseMap::gc_object(lua_State *L)
{
	LuaPerlinNoiseMap *o = *(LuaPerlinNoiseMap **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}


LuaPerlinNoiseMap *LuaPerlinNoiseMap::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);

	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);

	return *(LuaPerlinNoiseMap **)ud;
}


void LuaPerlinNoiseMap::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_register(L, className, create_object);
}


const char LuaPerlinNoiseMap::className[] = "PerlinNoiseMap";
const luaL_reg LuaPerlinNoiseMap::methods[] = {
	luamethod(LuaPerlinNoiseMap, get2dMap),
	luamethod(LuaPerlinNoiseMap, get2dMap_flat),
	luamethod(LuaPerlinNoiseMap, get3dMap),
	luamethod(LuaPerlinNoiseMap, get3dMap_flat),
	{0,0}
};

///////////////////////////////////////
/*
	LuaPseudoRandom
*/

int LuaPseudoRandom::l_next(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	LuaPseudoRandom *o = checkobject(L, 1);
	int min = 0;
	int max = 32767;
	lua_settop(L, 3);
	if (lua_isnumber(L, 2))
		min = luaL_checkinteger(L, 2);
	if (lua_isnumber(L, 3))
		max = luaL_checkinteger(L, 3);
	if (max < min) {
		errorstream<<"PseudoRandom.next(): max="<<max<<" min="<<min<<std::endl;
		throw LuaError("PseudoRandom.next(): max < min");
	}
	if(max - min != 32767 && max - min > 32767/5)
		throw LuaError("PseudoRandom.next() max-min is not 32767"
				" and is > 32768/5. This is disallowed due to"
				" the bad random distribution the"
				" implementation would otherwise make.");
	PseudoRandom &pseudo = o->m_pseudo;
	int val = pseudo.next();
	val = (val % (max-min+1)) + min;
	lua_pushinteger(L, val);
	return 1;
}


int LuaPseudoRandom::create_object(lua_State *L)
{
	int seed = luaL_checknumber(L, 1);
	LuaPseudoRandom *o = new LuaPseudoRandom(seed);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}


int LuaPseudoRandom::gc_object(lua_State *L)
{
	LuaPseudoRandom *o = *(LuaPseudoRandom **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}


LuaPseudoRandom *LuaPseudoRandom::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	return *(LuaPseudoRandom **)ud;
}


void LuaPseudoRandom::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_register(L, className, create_object);
}


const char LuaPseudoRandom::className[] = "PseudoRandom";
const luaL_reg LuaPseudoRandom::methods[] = {
	luamethod(LuaPseudoRandom, next),
	{0,0}
};

///////////////////////////////////////
/*
	LuaPcgRandom
*/

int LuaPcgRandom::l_next(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPcgRandom *o = checkobject(L, 1);
	u32 min = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : o->m_rnd.RANDOM_MIN;
	u32 max = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : o->m_rnd.RANDOM_MAX;

	lua_pushinteger(L, o->m_rnd.range(min, max));
	return 1;
}


int LuaPcgRandom::l_rand_normal_dist(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaPcgRandom *o = checkobject(L, 1);
	u32 min = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : o->m_rnd.RANDOM_MIN;
	u32 max = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : o->m_rnd.RANDOM_MAX;
	int num_trials = lua_isnumber(L, 4) ? lua_tointeger(L, 4) : 6;

	lua_pushinteger(L, o->m_rnd.randNormalDist(min, max, num_trials));
	return 1;
}


int LuaPcgRandom::create_object(lua_State *L)
{
	lua_Integer seed = luaL_checknumber(L, 1);
	LuaPcgRandom *o  = lua_isnumber(L, 2) ?
		new LuaPcgRandom(seed, lua_tointeger(L, 2)) :
		new LuaPcgRandom(seed);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}


int LuaPcgRandom::gc_object(lua_State *L)
{
	LuaPcgRandom *o = *(LuaPcgRandom **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}


LuaPcgRandom *LuaPcgRandom::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	return *(LuaPcgRandom **)ud;
}


void LuaPcgRandom::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_register(L, className, create_object);
}


const char LuaPcgRandom::className[] = "PcgRandom";
const luaL_reg LuaPcgRandom::methods[] = {
	luamethod(LuaPcgRandom, next),
	luamethod(LuaPcgRandom, rand_normal_dist),
	{0,0}
};
