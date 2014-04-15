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

#include "cpp_api/s_base.h"
#include "cpp_api/s_internal.h"
#include "lua_api/l_object.h"
#include "serverobject.h"
#include "debug.h"
#include "log.h"
#include "mods.h"
#include "util/string.h"


extern "C" {
#include "lualib.h"
#if USE_LUAJIT
	#include "luajit.h"
#endif
}

#include <stdio.h>
#include <cstdarg>


class ModNameStorer
{
private:
	lua_State *L;
public:
	ModNameStorer(lua_State *L_, const std::string &modname):
		L(L_)
	{
		// Store current modname in registry
		lua_pushstring(L, modname.c_str());
		lua_setfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	}
	~ModNameStorer()
	{
		// Clear current modname in registry
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	}
};


/*
	ScriptApiBase
*/

ScriptApiBase::ScriptApiBase()
{
	#ifdef SCRIPTAPI_LOCK_DEBUG
	m_locked = false;
	#endif

	m_luastack = luaL_newstate();
	assert(m_luastack);

	// Add and save an error handler
	lua_pushcfunction(m_luastack, script_error_handler);
	m_errorhandler = lua_gettop(m_luastack);

	// Make the ScriptApiBase* accessible to ModApiBase
	lua_pushlightuserdata(m_luastack, this);
	lua_setfield(m_luastack, LUA_REGISTRYINDEX, "scriptapi");

	// If we are using LuaJIT add a C++ wrapper function to catch
	// exceptions thrown in Lua -> C++ calls
#if USE_LUAJIT
	lua_pushlightuserdata(m_luastack, (void*) script_exception_wrapper);
	luaJIT_setmode(m_luastack, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
	lua_pop(m_luastack, 1);
#endif

	m_server = 0;
	m_environment = 0;
	m_guiengine = 0;
}

ScriptApiBase::~ScriptApiBase()
{
	lua_close(m_luastack);
}

bool ScriptApiBase::loadMod(const std::string &scriptpath,
		const std::string &modname)
{
	ModNameStorer modnamestorer(getStack(), modname);

	if(!string_allowed(modname, MODNAME_ALLOWED_CHARS)){
		errorstream<<"Error loading mod \""<<modname
				<<"\": modname does not follow naming conventions: "
				<<"Only chararacters [a-z0-9_] are allowed."<<std::endl;
		return false;
	}

	bool success = false;

	try{
		success = loadScript(scriptpath);
	}
	catch(LuaError &e){
		errorstream<<"Error loading mod \""<<modname
				<<"\": "<<e.what()<<std::endl;
	}

	return success;
}

bool ScriptApiBase::loadScript(const std::string &scriptpath)
{
	verbosestream<<"Loading and running script from "<<scriptpath<<std::endl;

	lua_State *L = getStack();

	int ret = luaL_loadfile(L, scriptpath.c_str()) || lua_pcall(L, 0, 0, m_errorhandler);
	if (ret) {
		errorstream << "========== ERROR FROM LUA ===========" << std::endl;
		errorstream << "Failed to load and run script from " << std::endl;
		errorstream << scriptpath << ":" << std::endl;
		errorstream << std::endl;
		errorstream << lua_tostring(L, -1) << std::endl;
		errorstream << std::endl;
		errorstream << "======= END OF ERROR FROM LUA ========" << std::endl;
		lua_pop(L, 1); // Pop error message from stack
		return false;
	}
	return true;
}

void ScriptApiBase::realityCheck()
{
	int top = lua_gettop(m_luastack);
	if(top >= 30){
		dstream<<"Stack is over 30:"<<std::endl;
		stackDump(dstream);
		std::string traceback = script_get_backtrace(m_luastack);
		throw LuaError("Stack is over 30 (reality check)\n" + traceback);
	}
}

void ScriptApiBase::scriptError()
{
	throw LuaError(lua_tostring(m_luastack, -1));
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
