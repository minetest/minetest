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
#include "filesys.h"
#include "log.h"
#include "mods.h"
#include "porting.h"
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
		lua_setfield(L, LUA_REGISTRYINDEX, "current_modname");
	}
	~ModNameStorer()
	{
		// Clear current modname in registry
		lua_pushnil(L);
		lua_setfield(L, LUA_REGISTRYINDEX, "current_modname");
	}
};


/*
	ScriptApiBase
*/

ScriptApiBase::ScriptApiBase() :
#ifdef SCRIPTAPI_LOCK_DEBUG
	m_locked(false),
#endif
	m_server(NULL),
	m_environment(NULL),
	m_guiengine(NULL)
{
	m_luastack = luaL_newstate();
	FATAL_ERROR_IF(!m_luastack, "luaL_newstate() failed!  Are you running "
			"LuaJIT in valgrind without LUAJIT_USE_VALGRIND?");
	lua_State *L = m_luastack;

	luaL_openlibs(L);

	// Add and save an error handler
	lua_pushcfunction(L, script_error_handler);
	m_errorhandler = lua_gettop(L);

	// Make the ScriptApiBase* accessible to ModApiBase
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, "scriptapi");

	// If we are using LuaJIT add a C++ wrapper function to catch
	// exceptions thrown in Lua -> C++ calls
#if USE_LUAJIT
	lua_pushlightuserdata(L, (void *) script_exception_wrapper);
	luaJIT_setmode(L, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
	lua_pop(L, 1);
#endif

	// Add basic globals

	// Core table
	lua_newtable(L);

	// Mods sub-table
	lua_newtable(L);
	lua_setfield(L, -2, "mods");

	lua_setglobal(L, "core");


	lua_pushliteral(L, DIR_DELIM);
	lua_setglobal(L, "DIR_DELIM");

	lua_pushstring(L, porting::getPlatformName());
	lua_setglobal(L, "PLATFORM");
}

ScriptApiBase::~ScriptApiBase()
{
	lua_close(m_luastack);
}

bool ScriptApiBase::loadMod(const std::string &script_path,
		const std::string &mod_name)
{
	if (!string_allowed(mod_name, MODNAME_ALLOWED_CHARS)) {
		errorstream<<"Error loading mod \""<<mod_name
				<<"\": mod name does not follow naming conventions: "
				<<"Only chararacters [a-z0-9_] are allowed."<<std::endl;
		return false;
	}

	lua_State *L = getStack();

	// Store mod name in registry for get_current_modname
	ModNameStorer mod_name_storer(L, mod_name);

	if (!loadScript(script_path, 1))
		return false;

	// Set the script return value as core.mods[mod_name]
	// Return value is on the top of the stack
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "mods");
	lua_remove(L, -2); // Remove core
	lua_insert(L, -2); // Insert mods before return value

	// If nothing was returned default to true
	if (lua_isnone(L, -1) || lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_pushboolean(L, true);
	}

	lua_setfield(L, -2, mod_name.c_str());

	lua_pop(L, 1); // Pop mods

	return true;
}

bool ScriptApiBase::loadScript(const std::string &script_path, int nret)
{
	verbosestream << "Loading and running script from " << script_path << std::endl;

	lua_State *L = getStack();

	const char *err_type = NULL;

	if (luaL_loadfile(L, script_path.c_str())) {
		err_type = "load";
	} else if (lua_pcall(L, 0, nret, m_errorhandler)) {
		err_type = "run";
	}

	if (err_type) {
		errorstream << "========== ERROR FROM LUA ===========" << std::endl;
		errorstream << "Failed to " << err_type << " script from " << std::endl;
		errorstream << script_path << ":" << std::endl;
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
	if (lua_gettop(m_luastack) >= 30) {
		dstream << "Stack is over 30:" << std::endl;
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
	int top = lua_gettop(m_luastack);
	for (int i = 1; i <= top; i++) {  // Repeat for each level
		int type = lua_type(m_luastack, i);
		switch (type) {
		case LUA_TSTRING:
			o << '"' << lua_tostring(m_luastack, i) << '"';
			break;
		case LUA_TBOOLEAN:
			o << (lua_toboolean(m_luastack, i) ? "true" : "false");
			break;
		case LUA_TNUMBER: {
			o << lua_tonumber(m_luastack, i);
			break;
		}
		default:
			o << lua_typename(m_luastack, type);
			break;
		}
		o << ' ';
	}
	o << std::endl;
}

void ScriptApiBase::addObjectReference(ServerActiveObject *cobj)
{
	SCRIPTAPI_PRECHECKHEADER
	//infostream<<"scriptapi_add_object_reference: id="<<cobj->getId()<<std::endl;

	// Create object on stack
	ObjectRef::create(L, cobj); // Puts ObjectRef (as userdata) on stack
	int object = lua_gettop(L);

	// Get core.object_refs table
	lua_getglobal(L, "core");
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

	// Get core.object_refs table
	lua_getglobal(L, "core");
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
void ScriptApiBase::objectrefGetOrCreate(lua_State *L,
		ServerActiveObject *cobj)
{
	if(cobj == NULL || cobj->getId() == 0){
		ObjectRef::create(L, cobj);
	} else {
		objectrefGet(L, cobj->getId());
	}
}

void ScriptApiBase::objectrefGet(lua_State *L, u16 id)
{
	// Get core.object_refs[i]
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "object_refs");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // object_refs
	lua_remove(L, -2); // core
}

