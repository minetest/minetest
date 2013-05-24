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

#include <stdio.h>
#include <cstdarg>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

#include "cpp_api/s_base.h"
#include "lua_api/l_object.h"
#include "serverobject.h"

ScriptApiBase::ScriptApiBase() :
		m_luastackmutex(),
#ifdef LOCK_DEBUG
		m_locked(false),
#endif
		m_luastack(0),
		m_server(0),
		m_environment(0)
{

}


void ScriptApiBase::realityCheck()
{
	int top = lua_gettop(m_luastack);
	if(top >= 30){
		dstream<<"Stack is over 30:"<<std::endl;
		stackDump(dstream);
		scriptError("Stack is over 30 (reality check)");
	}
}

void  ScriptApiBase::scriptError(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);
	char buf[10000];
	vsnprintf(buf, 10000, fmt, argp);
	va_end(argp);
	//errorstream<<"SCRIPT ERROR: "<<buf;
	throw LuaError(m_luastack, buf);
}

void ScriptApiBase::stackDump(std::ostream &o)
{
  int i;
  int top = lua_gettop(m_luastack);
  for (i = 1; i <= top; i++) {  /* repeat for each level */
	int t = lua_type(m_luastack, i);
	switch (t) {

	  case LUA_TSTRING:  /* strings */
	  	o<<"\""<<lua_tostring(m_luastack, i)<<"\"";
		break;

	  case LUA_TBOOLEAN:  /* booleans */
		o<<(lua_toboolean(m_luastack, i) ? "true" : "false");
		break;

	  case LUA_TNUMBER:  /* numbers */ {
	  	char buf[10];
		snprintf(buf, 10, "%g", lua_tonumber(m_luastack, i));
		o<<buf;
		break; }

	  default:  /* other values */
		o<<lua_typename(m_luastack, t);
		break;

	}
	o<<" ";
  }
  o<<std::endl;
}

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - removes the table and arguments from the lua stack
// - pushes the return value, computed depending on mode
void ScriptApiBase::runCallbacks(int nargs,RunCallbacksMode mode)
{
	lua_State *L = getStack();

	// Insert the return value into the lua stack, below the table
	assert(lua_gettop(L) >= nargs + 1);
	lua_pushnil(L);
	lua_insert(L, -(nargs + 1) - 1);
	// Stack now looks like this:
	// ... <return value = nil> <table> <arg#1> <arg#2> ... <arg#n>

	int rv = lua_gettop(L) - nargs - 1;
	int table = rv + 1;
	int arg = table + 1;

	luaL_checktype(L, table, LUA_TTABLE);

	// Foreach
	lua_pushnil(L);
	bool first_loop = true;
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		for(int i = 0; i < nargs; i++)
			lua_pushvalue(L, arg+i);
		if(lua_pcall(L, nargs, 1, 0))
			scriptError("error: %s", lua_tostring(L, -1));

		// Move return value to designated space in stack
		// Or pop it
		if(first_loop){
			// Result of first callback is always moved
			lua_replace(L, rv);
			first_loop = false;
		} else {
			// Otherwise, what happens depends on the mode
			if(mode == RUN_CALLBACKS_MODE_FIRST)
				lua_pop(L, 1);
			else if(mode == RUN_CALLBACKS_MODE_LAST)
				lua_replace(L, rv);
			else if(mode == RUN_CALLBACKS_MODE_AND ||
					mode == RUN_CALLBACKS_MODE_AND_SC){
				if((bool)lua_toboolean(L, rv) == true &&
						(bool)lua_toboolean(L, -1) == false)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else if(mode == RUN_CALLBACKS_MODE_OR ||
					mode == RUN_CALLBACKS_MODE_OR_SC){
				if((bool)lua_toboolean(L, rv) == false &&
						(bool)lua_toboolean(L, -1) == true)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else
				assert(0);
		}

		// Handle short circuit modes
		if(mode == RUN_CALLBACKS_MODE_AND_SC &&
				(bool)lua_toboolean(L, rv) == false)
			break;
		else if(mode == RUN_CALLBACKS_MODE_OR_SC &&
				(bool)lua_toboolean(L, rv) == true)
			break;

		// value removed, keep key for next iteration
	}

	// Remove stuff from stack, leaving only the return value
	lua_settop(L, rv);

	// Fix return value in case no callbacks were called
	if(first_loop){
		if(mode == RUN_CALLBACKS_MODE_AND ||
				mode == RUN_CALLBACKS_MODE_AND_SC){
			lua_pop(L, 1);
			lua_pushboolean(L, true);
		}
		else if(mode == RUN_CALLBACKS_MODE_OR ||
				mode == RUN_CALLBACKS_MODE_OR_SC){
			lua_pop(L, 1);
			lua_pushboolean(L, false);
		}
	}
}

void ScriptApiBase::addObjectReference(ServerActiveObject *cobj)
{
	SCRIPTAPI_PRECHECKHEADER
	//infostream<<"scriptapi_add_object_reference: id="<<cobj->getId()<<std::endl;

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
}

void ScriptApiBase::removeObjectReference(ServerActiveObject *cobj)
{
	SCRIPTAPI_PRECHECKHEADER
	//infostream<<"scriptapi_rm_object_reference: id="<<cobj->getId()<<std::endl;

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
}

// Creates a new anonymous reference if cobj=NULL or id=0
void ScriptApiBase::objectrefGetOrCreate(
		ServerActiveObject *cobj)
{
	lua_State *L = getStack();

	if(cobj == NULL || cobj->getId() == 0){
		ObjectRef::create(L, cobj);
	} else {
		objectrefGet(cobj->getId());
	}
}

void ScriptApiBase::objectrefGet(u16 id)
{
	lua_State *L = getStack();

	// Get minetest.object_refs[i]
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "object_refs");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // object_refs
	lua_remove(L, -2); // minetest
}
