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

#include "common/c_types.h"
#include "common/c_internal.h"
#include "gamedef.h"

extern "C" {
#include <lua.h>
#include <lauxlib.h>
}

#ifndef SERVER
class Client;
#endif

class ScriptApiBase;
class Server;
class Environment;
class GUIEngine;

/**
 * This macro permits to bootstrap easily a Lua Referenced object
 */
#define LUAREF_OBJECT(name)                                                                                        \
private:                                                                                                           \
    name *m_object = nullptr;                                                                                                \
    static const char className[];                                                                                 \
    static const luaL_Reg methods[];                                                                               \
    static int gc_object(lua_State *L);                                                                            \
                                                                                                                   \
public:                                                                                                            \
    explicit Lua##name(name *object);                                                                           \
    virtual ~Lua##name();                                                                                       \
    static void Register(lua_State *L);                                                                            \
    static void create(lua_State *L, name *object);                                                                \
    static Lua##name *checkobject(lua_State *L, int narg);                                                      \
    static name *getobject(Lua##name *ref);


class ModApiBase {

public:
	static ScriptApiBase*   getScriptApiBase(lua_State *L);
	static Server*          getServer(lua_State *L);
	#ifndef SERVER
	static Client*          getClient(lua_State *L);
	#endif // !SERVER

	static IGameDef*        getGameDef(lua_State *L);

	static Environment*     getEnv(lua_State *L);
	static GUIEngine*       getGuiEngine(lua_State *L);
	// When we are not loading the mod, this function returns "."
	static std::string      getCurrentModPath(lua_State *L);

	// Get an arbitrary subclass of ScriptApiBase
	// by using dynamic_cast<> on getScriptApiBase()
	template<typename T>
	static T* getScriptApi(lua_State *L) {
		ScriptApiBase *scriptIface = getScriptApiBase(L);
		T *scriptIfaceDowncast = dynamic_cast<T*>(scriptIface);
		if (!scriptIfaceDowncast) {
			throw LuaError("Requested unavailable ScriptApi - core engine bug!");
		}
		return scriptIfaceDowncast;
	}

	static bool registerFunction(lua_State *L,
			const char* name,
			lua_CFunction func,
			int top);
};
