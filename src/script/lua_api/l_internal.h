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

/******************************************************************************/
/******************************************************************************/
/* WARNING!!!! do NOT add this header in any include file or any code file    */
/*             not being a modapi file!!!!!!!!                                */
/******************************************************************************/
/******************************************************************************/

#pragma once

#include "common/c_internal.h"

#define luamethod(class, name) {#name, class::l_##name}
#define luamethod_aliased(class, name, alias) {#name, class::l_##name}, {#alias, class::l_##name}
#define API_FCT(name) registerFunction(L, #name, l_##name, top)

#define MAP_LOCK_REQUIRED
#define NO_MAP_LOCK_REQUIRED

#define GET_ENV_PTR_NO_MAP_LOCK                              \
	ServerEnvironment *env = (ServerEnvironment *)getEnv(L); \
	if (env == NULL)                                         \
		return 0

#define GET_ENV_PTR         \
	MAP_LOCK_REQUIRED;      \
	GET_ENV_PTR_NO_MAP_LOCK


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
