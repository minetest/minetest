/*
Minetest
Copyright (C) 2021 velartrill, Lexi Hale <lexi@hale.su>

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
#include "lua_api/l_particles.h"
#include "lua_api/l_object.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"
#include "particles.h"

namespace LuaParticleParams {
	using namespace ParticleParamTypes;

	inline void readLuaValue(lua_State* L, f32Parameter& ret)
		{ ret = (f32Parameter)((f32)lua_tonumber(L, -1)); }

	inline void readLuaValue(lua_State* L, v3fParameter& ret) {
		if (lua_isnumber(L, -1)) { // shortcut for uniform vectors
			auto n = lua_tonumber(L, -1);
			ret = (v3fParameter)v3f(n,n,n);
		} else {
			ret = (v3fParameter)check_v3f(L, -1);
		}
	}

	template <typename T> void
	readLuaValue(lua_State* L, RangedParameter<T>& field) {
		if (lua_isnil(L,-1))
			return;
		if (!lua_istable(L,-1)) // is this is just a literal value?
			goto set_uniform;

		lua_getfield(L, -1, "min");
		// handle convenience syntax for non-range values
		if (lua_isnil(L,-1)) {
			lua_pop(L, 1);
			goto set_uniform;
		}
		readLuaValue(L,field.min);
		lua_pop(L, 1);

		lua_getfield(L, -1, "max");
		readLuaValue(L,field.max);
		lua_pop(L, 1);
		return;

		set_uniform:
			readLuaValue(L, field.min);
			readLuaValue(L, field.max);
	}

	template <typename T> void
	readLegacyValue(lua_State* L, const char* name, T& field) {}

	template <typename T> void
	readLegacyValue(lua_State* L, const char* name, RangedParameter<T>& field) {
		int tbl = lua_gettop(L);
		lua_pushliteral(L, "min");
		lua_pushstring(L, name);
		lua_concat(L, 2);
		lua_gettable(L, tbl);
		if (!lua_isnil(L, -1)) {
			readLuaValue(L, field.min);
		}
		lua_settop(L, tbl);

		lua_pushliteral(L, "max");
		lua_pushstring(L, name);
		lua_concat(L, 2);
		lua_gettable(L, tbl);
		if (!lua_isnil(L, -1)) {
			readLuaValue(L, field.max);
		}
		lua_settop(L, tbl);
	}


	template <typename T> void
	readTweenTable(lua_State* L, const char* name, TweenedParameter<T>& field)
	{
		int tbl = lua_gettop(L);
		lua_pushstring(L, name);
		lua_pushliteral(L, "_tween");
		lua_concat(L, 2);
		lua_gettable(L, tbl);
		if(lua_istable(L, -1)) {
			// get the starting value
			lua_pushinteger(L, 1), lua_gettable(L, -2);
			readLuaValue(L, field.start);
			lua_pop(L, 1);

			// get the final value -- use len instead of 2 so that this
			// gracefully degrades if keyframe support is later added
			lua_pushinteger(L, (lua_Integer)lua_objlen(L, -1)), lua_gettable(L, -2);
			readLuaValue(L, field.end);
			lua_pop(L, 1);
			goto done;
		} else lua_pop(L,1);
		// the table is not present; check for nonanimated values

		lua_getfield(L, 1, name);
		if(!lua_isnil(L, -1)) {
			readLuaValue(L, field.start);
			lua_settop(L, tbl);
			goto set_uniform;
		} else lua_pop(L,1);

		// this table is not present either; check for legacy values
		readLegacyValue(L, name, field.start);

		set_uniform: field.end = field.start;
		done: lua_settop(L, tbl); // clean up after ourselves
	}

	ServerParticleTexture readTexValue(lua_State* L);
}
