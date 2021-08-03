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

#include "lua_api/l_particles.h"
#include "lua_api/l_object.h"
#include "lua_api/l_internal.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "server.h"
#include "particles.h"

namespace LuaParticleParams {
	using namespace ParticleParamTypes;

	inline void readLuaValue(lua_State* L, srz_f32& ret)
		{ ret = (srz_f32)((f32)lua_tonumber(L, -1)); }

	inline void readLuaValue(lua_State* L, srz_v3f& ret)
		{ ret = (srz_v3f)check_v3f(L, -1); }

	template <typename T> void
	readLuaValue(lua_State* L, RangedParameter<T>& field) {
		lua_getfield(L, -1, "min");
		readLuaValue(L,field.min);
		lua_pop(L, 1);

		lua_getfield(L, -1, "max");
		readLuaValue(L,field.max);
		lua_pop(L, 1);
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


	template <typename T, BlendFunction<T> Blend> void
	readTweenTable(lua_State* L, const char* name, TweenedParameter<T, Blend>& field)
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
		if(lua_istable(L, -1)) {
			readLuaValue(L, field.start);
			lua_settop(L, tbl); 
			goto legacy_done;
		} else lua_pop(L,1);

		// this table is not present either; check for legacy values
		readLegacyValue(L, name, field.start);

		legacy_done: field.end = field.start;
		done: lua_settop(L, tbl); // clean up after ourselves
	}

	ParticleTexture readTexValue(lua_State* L);
}
