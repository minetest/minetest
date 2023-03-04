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

#pragma once

#include "irr_v3d.h"
#include "lua_api/l_base.h"
#include "noise.h"

/*
	LuaPerlinNoise
*/
class LuaPerlinNoise : public ModApiBase
{
private:
	NoiseParams np;

	static luaL_Reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	static int l_get_2d(lua_State *L);
	static int l_get_3d(lua_State *L);

public:
	LuaPerlinNoise(const NoiseParams *params);
	~LuaPerlinNoise() = default;

	// LuaPerlinNoise(seed, octaves, persistence, scale)
	// Creates an LuaPerlinNoise and leaves it on top of stack
	static int create_object(lua_State *L);

	static void *packIn(lua_State *L, int idx);
	static void packOut(lua_State *L, void *ptr);

	static void Register(lua_State *L);

	static const char className[];
};

/*
	LuaPerlinNoiseMap
*/
class LuaPerlinNoiseMap : public ModApiBase
{
	Noise *noise;

	static luaL_Reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	static int l_get_2d_map(lua_State *L);
	static int l_get_2d_map_flat(lua_State *L);
	static int l_get_3d_map(lua_State *L);
	static int l_get_3d_map_flat(lua_State *L);

	static int l_calc_2d_map(lua_State *L);
	static int l_calc_3d_map(lua_State *L);
	static int l_get_map_slice(lua_State *L);

public:
	LuaPerlinNoiseMap(const NoiseParams *np, s32 seed, v3s16 size);
	~LuaPerlinNoiseMap();

	inline bool is3D() const { return noise->sz > 1; }

	// LuaPerlinNoiseMap(np, size)
	// Creates an LuaPerlinNoiseMap and leaves it on top of stack
	static int create_object(lua_State *L);

	static void *packIn(lua_State *L, int idx);
	static void packOut(lua_State *L, void *ptr);

	static void Register(lua_State *L);

	static const char className[];
};

/*
	LuaPseudoRandom
*/
class LuaPseudoRandom : public ModApiBase
{
private:
	PseudoRandom m_pseudo;

	static const luaL_Reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// next(self, min=0, max=32767) -> get next value
	static int l_next(lua_State *L);

public:
	LuaPseudoRandom(s32 seed) : m_pseudo(seed) {}

	// LuaPseudoRandom(seed)
	// Creates an LuaPseudoRandom and leaves it on top of stack
	static int create_object(lua_State *L);

	static void Register(lua_State *L);

	static const char className[];
};

/*
	LuaPcgRandom
*/
class LuaPcgRandom : public ModApiBase
{
private:
	PcgRandom m_rnd;

	static const luaL_Reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// next(self, min=-2147483648, max=2147483647) -> get next value
	static int l_next(lua_State *L);

	// rand_normal_dist(self, min=-2147483648, max=2147483647, num_trials=6) ->
	// get next normally distributed random value
	static int l_rand_normal_dist(lua_State *L);

public:
	LuaPcgRandom(u64 seed) : m_rnd(seed) {}
	LuaPcgRandom(u64 seed, u64 seq) : m_rnd(seed, seq) {}

	// LuaPcgRandom(seed)
	// Creates an LuaPcgRandom and leaves it on top of stack
	static int create_object(lua_State *L);

	static void Register(lua_State *L);

	static const char className[];
};

/*
	LuaSecureRandom
*/
class LuaSecureRandom : public ModApiBase
{
private:
	static const size_t RAND_BUF_SIZE = 2048;
	static const luaL_Reg methods[];

	u32 m_rand_idx;
	char m_rand_buf[RAND_BUF_SIZE];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L);

	// next_bytes(self, count) -> get count many bytes
	static int l_next_bytes(lua_State *L);

public:
	bool fillRandBuf();

	// LuaSecureRandom()
	// Creates an LuaSecureRandom and leaves it on top of stack
	static int create_object(lua_State *L);

	static void Register(lua_State *L);

	static const char className[];
};
