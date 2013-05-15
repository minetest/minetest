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

#ifndef L_BASE_H_
#define L_BASE_H_

#include "biome.h"
#include "common/c_types.h"

extern "C" {
#include "lua.h"
}

extern struct EnumString es_BiomeTerrainType[];

class ScriptApi;
class Server;
class Environment;

typedef class ModApiBase {

public:
	ModApiBase();

	virtual bool Initialize(lua_State* L, int top) = 0;
	virtual ~ModApiBase() {};

protected:
	static Server* getServer(      lua_State* L);
	static Environment* getEnv(    lua_State* L);
	static bool registerFunction(	lua_State* L,
									const char* name,
									lua_CFunction fct,
									int top
									);
} ModApiBase;

#if (defined(WIN32) || defined(_WIN32_WCE))
#define NO_MAP_LOCK_REQUIRED
#else
#include "main.h"
#include "profiler.h"
#define NO_MAP_LOCK_REQUIRED ScopeProfiler nolocktime(g_profiler,"Scriptapi: unlockable time",SPT_ADD)
//#define NO_ENVLOCK_REQUIRED assert(getServer(L).m_env_mutex.IsLocked() == false)
#endif

#endif /* L_BASE_H_ */
