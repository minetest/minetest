/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "scriptapi.h"

#include <iostream>
extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include "log.h"
#include "server.h"
#include "porting.h"
#include "filesys.h"
#include "serverobject.h"
#include "script.h"
//#include "luna.h"

static void stackDump(lua_State *L, std::ostream &o)
{
  int i;
  int top = lua_gettop(L);
  for (i = 1; i <= top; i++) {  /* repeat for each level */
	int t = lua_type(L, i);
	switch (t) {

	  case LUA_TSTRING:  /* strings */
	  	o<<"\""<<lua_tostring(L, i)<<"\"";
		break;

	  case LUA_TBOOLEAN:  /* booleans */
		o<<(lua_toboolean(L, i) ? "true" : "false");
		break;

	  case LUA_TNUMBER:  /* numbers */ {
	  	char buf[10];
		snprintf(buf, 10, "%g", lua_tonumber(L, i));
		o<<buf;
		break; }

	  default:  /* other values */
		o<<lua_typename(L, t);
		break;

	}
	o<<" ";
  }
  o<<std::endl;
}

static void realitycheck(lua_State *L)
{
	int top = lua_gettop(L);
	if(top >= 30){
		dstream<<"Stack is over 30:"<<std::endl;
		stackDump(L, dstream);
		script_error(L, "Stack is over 30 (reality check)");
	}
}

// Register new object prototype (must be based on entity)
static int l_register_object(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	luaL_checkany(L, 2);
	infostream<<"register_object: "<<name<<std::endl;
	// Get the minetest table
	lua_getglobal(L, "minetest");
	// Get field "registered_objects"
	lua_getfield(L, -1, "registered_objects");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	// Object is in param 2
	lua_pushvalue(L, 2); // Copy object to top of stack
	lua_setfield(L, objectstable, name); // registered_objects[name] = object

	return 0; /* number of results */
}

static int l_new_entity(lua_State *L)
{
	/* o = o or {}
	   setmetatable(o, self)
	   self.__index = self
	   return o */
	if(lua_isnil(L, -1))
		lua_newtable(L);
	luaL_checktype(L, -1, LUA_TTABLE);
	luaL_getmetatable(L, "minetest.entity");
	lua_pushvalue(L, -1); // duplicate metatable
	lua_setfield(L, -2, "__index");
	lua_setmetatable(L, -2);
	// return table
	return 1;
}

static const struct luaL_Reg minetest_f [] = {
	{"register_object", l_register_object},
	{"new_entity", l_new_entity},
	{NULL, NULL}
};

static int l_entity_set_deleted(lua_State *L)
{
	return 0;
}

static const struct luaL_Reg minetest_entity_m [] = {
	{"set_deleted", l_entity_set_deleted},
	{NULL, NULL}
};

class ObjectRef
{
private:
	ServerActiveObject *m_object;

	static const char className[];
	static const luaL_reg methods[];

	static ObjectRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(ObjectRef**)ud;  // unbox pointer
	}
	
	// Exported functions

	static int l_remove(lua_State *L)
	{
		ObjectRef *o = checkobject(L, 1);
		ServerActiveObject *co = o->m_object;
		if(co == NULL) return 0;
		infostream<<"ObjectRef::l_remove(): id="<<co->getId()<<std::endl;
		co->m_removed = true;
		return 0;
	}

	static int gc_object(lua_State *L) {
		//ObjectRef *o = checkobject(L, 1);
		ObjectRef *o = *(ObjectRef **)(lua_touserdata(L, 1));
		//infostream<<"ObjectRef::gc_object: o="<<o<<std::endl;
		delete o;
		return 0;
	}

public:
	ObjectRef(ServerActiveObject *object):
		m_object(object)
	{
		infostream<<"ObjectRef created for id="<<m_object->getId()<<std::endl;
	}

	~ObjectRef()
	{
		if(m_object)
			infostream<<"ObjectRef destructing for id="<<m_object->getId()<<std::endl;
		else
			infostream<<"ObjectRef destructing for id=unknown"<<std::endl;
	}

	// Creates an ObjectRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, ServerActiveObject *object)
	{
		ObjectRef *o = new ObjectRef(object);
		//infostream<<"ObjectRef::create: o="<<o<<std::endl;
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
	}

	static void set_null(lua_State *L)
	{
		ObjectRef *o = checkobject(L, -1);
		ServerActiveObject *co = o->m_object;
		if(co == NULL)
			return;
		o->m_object = NULL;
	}
	
	static void Register(lua_State *L)
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
};

const char ObjectRef::className[] = "ObjectRef";

#define method(class, name) {#name, class::l_##name}

const luaL_reg ObjectRef::methods[] = {
	method(ObjectRef, remove),
	{0,0}
};

void scriptapi_export(lua_State *L, Server *server)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_export"<<std::endl;
	
	// Register global functions in table minetest
	lua_newtable(L);
	luaL_register(L, NULL, minetest_f);
	lua_setglobal(L, "minetest");
	
	// Get the main minetest table
	lua_getglobal(L, "minetest");

	// Add registered_objects table in minetest
	lua_newtable(L);
	lua_setfield(L, -2, "registered_objects");

	// Add object_refs table in minetest
	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");

	// Add luaentities table in minetest
	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");

	// Load and run some base Lua stuff
	/*script_load(L, (porting::path_data + DIR_DELIM + "scripts"
			+ DIR_DELIM + "base.lua").c_str());*/
	
	// Create entity reference metatable
	luaL_newmetatable(L, "minetest.entity_reference");
	lua_pop(L, 1);
	
	// Create entity prototype
	luaL_newmetatable(L, "minetest.entity");
	// metatable.__index = metatable
	lua_pushvalue(L, -1); // Duplicate metatable
	lua_setfield(L, -2, "__index");
	// Put functions in metatable
	luaL_register(L, NULL, minetest_entity_m);
	// Put other stuff in metatable

	// Entity C reference
	ObjectRef::Register(L);
}

void scriptapi_luaentity_register(lua_State *L, u16 id, const char *name,
		const char *init_state)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_register: id="<<id<<std::endl;
	
	// Create object as a dummy string (TODO: Create properly)
	lua_pushstring(L, "dummy object string");
	int object = lua_gettop(L);

	// Get minetest.luaentities table
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	
	// luaentities[id] = object
	lua_pushnumber(L, id); // Push id
	lua_pushvalue(L, object); // Copy object to top of stack
	lua_settable(L, objectstable);
	
	lua_pop(L, 3); // pop luaentities, minetest and the object
}

void scriptapi_luaentity_deregister(lua_State *L, u16 id)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_deregister: id="<<id<<std::endl;

	// Get minetest.luaentities table
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	
	/*// Get luaentities[id]
	lua_pushnumber(L, cobj->getId()); // Push id
	lua_gettable(L, objectstable);
	// Object is at stack top
	lua_pop(L, 1); // pop object*/

	// Set luaentities[id] = nil
	lua_pushnumber(L, cobj->getId()); // Push id
	lua_pushnil(L);
	lua_settable(L, objectstable);
	
	lua_pop(L, 2); // pop luaentities, minetest
}

void scriptapi_luaentity_step(lua_State *L, u16 id,
		float dtime, bool send_recommended)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;

	// Get minetest.luaentities table
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	
	// Get luaentities[id]
	lua_pushnumber(L, cobj->getId()); // Push id
	lua_gettable(L, objectstable);

	// TODO: Call step function

	lua_pop(L, 1); // pop object
	lua_pop(L, 2); // pop luaentities, minetest
}

std::string scriptapi_luaentity_get_state(lua_State *L, u16 id)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_get_state: id="<<id<<std::endl;
	
	return "";
}

void scriptapi_add_object_reference(lua_State *L, ServerActiveObject *cobj)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_add_object_reference: id="<<cobj->getId()<<std::endl;

	// Create object on stack
	ObjectRef::create(L, cobj); // Puts ObjectRef (as userdata) on stack
	int object = lua_gettop(L);

	// Get minetest.object_refs table
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "object_refs");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	
	// object_refs[id] = object
	lua_pushnumber(L, cobj->getId()); // Push id
	lua_pushvalue(L, object); // Copy object to top of stack
	lua_settable(L, objectstable);
	
	// pop object_refs, minetest and the object
	lua_pop(L, 3);
}

void scriptapi_rm_object_reference(lua_State *L, ServerActiveObject *cobj)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_rm_object_reference: id="<<cobj->getId()<<std::endl;

	// Get minetest.object_refs table
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "object_refs");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	
	// Get object_refs[id]
	lua_pushnumber(L, cobj->getId()); // Push id
	lua_gettable(L, objectstable);
	// Set object reference to NULL
	ObjectRef::set_null(L);
	lua_pop(L, 1); // pop object

	// Set object_refs[id] = nil
	lua_pushnumber(L, cobj->getId()); // Push id
	lua_pushnil(L);
	lua_settable(L, objectstable);
	
	// pop object_refs, minetest
	lua_pop(L, 2);
}

