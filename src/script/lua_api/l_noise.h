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

#ifndef L_NOISE_H_
#define L_NOISE_H_

#include "lua_api/l_base.h"
#include "irr_v3d.h"
#include "noise.h"

/*
	LuaPerlinNoise
*/
class LuaPerlinNoise : public ModApiBase {
private:
	int seed;
	int octaves;
	float persistence;
	float scale;
	static const char className[];
	static const luaL_reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	static int l_get2d(lua_State *L);
	static int l_get3d(lua_State *L);

public:
	LuaPerlinNoise(int a_seed, int a_octaves, float a_persistence,
			float a_scale);

	~LuaPerlinNoise();

	// LuaPerlinNoise(seed, octaves, persistence, scale)
	// Creates an LuaPerlinNoise and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaPerlinNoise* checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

/*
	LuaPerlinNoiseMap
*/
class LuaPerlinNoiseMap : public ModApiBase {
private:
	Noise *noise;
	static const char className[];
	static const luaL_reg methods[];

	static int gc_object(lua_State *L);

	static int l_get2dMap(lua_State *L);
	static int l_get2dMap_flat(lua_State *L);
	static int l_get3dMap(lua_State *L);
	static int l_get3dMap_flat(lua_State *L);

public:
	LuaPerlinNoiseMap(NoiseParams *np, int seed, v3s16 size);

	~LuaPerlinNoiseMap();

	// LuaPerlinNoiseMap(np, size)
	// Creates an LuaPerlinNoiseMap and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaPerlinNoiseMap *checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

/*
	LuaPseudoRandom
*/
class LuaPseudoRandom : public ModApiBase {
private:
	PseudoRandom m_pseudo;

	static const char className[];
	static const luaL_reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// next(self, min=0, max=32767) -> get next value
	static int l_next(lua_State *L);

public:
	LuaPseudoRandom(int seed);

	~LuaPseudoRandom();

	const PseudoRandom& getItem() const;
	PseudoRandom& getItem();

	// LuaPseudoRandom(seed)
	// Creates an LuaPseudoRandom and leaves it on top of stack
	static int create_object(lua_State *L);

	static LuaPseudoRandom* checkobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

#endif /* L_NOISE_H_ */
