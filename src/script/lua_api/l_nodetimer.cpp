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

#include "lua_api/l_nodetimer.h"
#include "lua_api/l_internal.h"
#include "environment.h"
#include "map.h"


int NodeTimerRef::gc_object(lua_State *L) {
	NodeTimerRef *o = *(NodeTimerRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

NodeTimerRef* NodeTimerRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(NodeTimerRef**)ud;  // unbox pointer
}

int NodeTimerRef::l_set(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;
	f32 t = luaL_checknumber(L,2);
	f32 e = luaL_checknumber(L,3);
	env->getMap().setNodeTimer(o->m_p,NodeTimer(t,e));
	return 0;
}

int NodeTimerRef::l_start(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;
	f32 t = luaL_checknumber(L,2);
	env->getMap().setNodeTimer(o->m_p,NodeTimer(t,0));
	return 0;
}

int NodeTimerRef::l_stop(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;
	env->getMap().removeNodeTimer(o->m_p);
	return 0;
}

int NodeTimerRef::l_is_started(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;

	NodeTimer t = env->getMap().getNodeTimer(o->m_p);
	lua_pushboolean(L,(t.timeout != 0));
	return 1;
}

int NodeTimerRef::l_get_timeout(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;

	NodeTimer t = env->getMap().getNodeTimer(o->m_p);
	lua_pushnumber(L,t.timeout);
	return 1;
}

int NodeTimerRef::l_get_elapsed(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;

	NodeTimer t = env->getMap().getNodeTimer(o->m_p);
	lua_pushnumber(L,t.elapsed);
	return 1;
}


NodeTimerRef::NodeTimerRef(v3s16 p, ServerEnvironment *env):
	m_p(p),
	m_env(env)
{
}

NodeTimerRef::~NodeTimerRef()
{
}

// Creates an NodeTimerRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void NodeTimerRef::create(lua_State *L, v3s16 p, ServerEnvironment *env)
{
	NodeTimerRef *o = new NodeTimerRef(p, env);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void NodeTimerRef::set_null(lua_State *L)
{
	NodeTimerRef *o = checkobject(L, -1);
	o->m_env = NULL;
}

void NodeTimerRef::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);  // hide metatable from Lua getmetatable()

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pop(L, 1);  // drop metatable

	luaL_openlib(L, 0, methods, 0);  // fill methodtable
	lua_pop(L, 1);  // drop methodtable

	// Cannot be created from Lua
	//lua_register(L, className, create_object);
}

const char NodeTimerRef::className[] = "NodeTimerRef";
const luaL_reg NodeTimerRef::methods[] = {
	luamethod(NodeTimerRef, start),
	luamethod(NodeTimerRef, set),
	luamethod(NodeTimerRef, stop),
	luamethod(NodeTimerRef, is_started),
	luamethod(NodeTimerRef, get_timeout),
	luamethod(NodeTimerRef, get_elapsed),
	{0,0}
};
