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
#include "cpp_api/s_security.h"
#include "lua_api/l_object.h"
#include "common/c_converter.h"
#include "server/player_sao.h"
#include "filesys.h"
#include "content/mods.h"
#include "porting.h"
#include "util/string.h"
#include "server.h"
#ifndef SERVER
#include "client/client.h"
#endif


extern "C" {
#include "lualib.h"
#if USE_LUAJIT
	#include "luajit.h"
#else
	#include "bit.h"
#endif
}

#include <cstdio>
#include <cstdarg>
#include "script/common/c_content.h"
#include <sstream>


class ModNameStorer
{
private:
	lua_State *L;
public:
	ModNameStorer(lua_State *L_, const std::string &mod_name):
		L(L_)
	{
		// Store current mod name in registry
		lua_pushstring(L, mod_name.c_str());
		lua_rawseti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	}
	~ModNameStorer()
	{
		// Clear current mod name from registry
		lua_pushnil(L);
		lua_rawseti(L, LUA_REGISTRYINDEX, CUSTOM_RIDX_CURRENT_MOD_NAME);
	}
};


/*
	ScriptApiBase
*/

ScriptApiBase::ScriptApiBase(ScriptingType type):
		m_type(type)
{
#ifdef SCRIPTAPI_LOCK_DEBUG
	m_lock_recursion_count = 0;
#endif

	m_luastack = luaL_newstate();
	FATAL_ERROR_IF(!m_luastack, "luaL_newstate() failed");

	lua_atpanic(m_luastack, &luaPanic);

	if (m_type == ScriptingType::Client)
		clientOpenLibs(m_luastack);
	else
		luaL_openlibs(m_luastack);

	// Load bit library
	lua_pushcfunction(m_luastack, luaopen_bit);
	lua_pushstring(m_luastack, LUA_BITLIBNAME);
	lua_call(m_luastack, 1, 0);

	// Make the ScriptApiBase* accessible to ModApiBase
#if INDIRECT_SCRIPTAPI_RIDX
	*(void **)(lua_newuserdata(m_luastack, sizeof(void *))) = this;
#else
	lua_pushlightuserdata(m_luastack, this);
#endif
	lua_rawseti(m_luastack, LUA_REGISTRYINDEX, CUSTOM_RIDX_SCRIPTAPI);

	// Add and save an error handler
	lua_getglobal(m_luastack, "debug");
	lua_getfield(m_luastack, -1, "traceback");
	lua_rawseti(m_luastack, LUA_REGISTRYINDEX, CUSTOM_RIDX_BACKTRACE);
	lua_pop(m_luastack, 1); // pop debug

	// If we are using LuaJIT add a C++ wrapper function to catch
	// exceptions thrown in Lua -> C++ calls
#if USE_LUAJIT
	lua_pushlightuserdata(m_luastack, (void*) script_exception_wrapper);
	luaJIT_setmode(m_luastack, -1, LUAJIT_MODE_WRAPCFUNC | LUAJIT_MODE_ON);
	lua_pop(m_luastack, 1);
#endif

	// Add basic globals
	lua_newtable(m_luastack);
	lua_setglobal(m_luastack, "core");

	if (m_type == ScriptingType::Client)
		lua_pushstring(m_luastack, "/");
	else
		lua_pushstring(m_luastack, DIR_DELIM);
	lua_setglobal(m_luastack, "DIR_DELIM");

	lua_pushstring(m_luastack, porting::getPlatformName());
	lua_setglobal(m_luastack, "PLATFORM");

	// Make sure Lua uses the right locale
	setlocale(LC_NUMERIC, "C");
}

ScriptApiBase::~ScriptApiBase()
{
	lua_close(m_luastack);
}

int ScriptApiBase::luaPanic(lua_State *L)
{
	std::ostringstream oss;
	oss << "LUA PANIC: unprotected error in call to Lua API ("
		<< readParam<std::string>(L, -1) << ")";
	FATAL_ERROR(oss.str().c_str());
	// NOTREACHED
	return 0;
}

void ScriptApiBase::clientOpenLibs(lua_State *L)
{
	static const std::vector<std::pair<std::string, lua_CFunction>> m_libs = {
		{ "", luaopen_base },
		{ LUA_TABLIBNAME,  luaopen_table   },
		{ LUA_OSLIBNAME,   luaopen_os      },
		{ LUA_STRLIBNAME,  luaopen_string  },
		{ LUA_MATHLIBNAME, luaopen_math    },
		{ LUA_DBLIBNAME,   luaopen_debug   },
#if USE_LUAJIT
		{ LUA_JITLIBNAME,  luaopen_jit     },
#endif
	};

	for (const std::pair<std::string, lua_CFunction> &lib : m_libs) {
	    lua_pushcfunction(L, lib.second);
	    lua_pushstring(L, lib.first.c_str());
	    lua_call(L, 1, 0);
	}
}

void ScriptApiBase::loadMod(const std::string &script_path,
		const std::string &mod_name)
{
	ModNameStorer mod_name_storer(getStack(), mod_name);

	loadScript(script_path);
}

void ScriptApiBase::loadScript(const std::string &script_path)
{
	verbosestream << "Loading and running script from " << script_path << std::endl;

	lua_State *L = getStack();

	int error_handler = PUSH_ERROR_HANDLER(L);

	bool ok;
	if (m_secure) {
		ok = ScriptApiSecurity::safeLoadFile(L, script_path.c_str());
	} else {
		ok = !luaL_loadfile(L, script_path.c_str());
	}
	ok = ok && !lua_pcall(L, 0, 0, error_handler);
	if (!ok) {
		const char *error_msg = lua_tostring(L, -1);
		if (!error_msg)
			error_msg = "(error object is not a string)";
		lua_pop(L, 2); // Pop error message and error handler
		throw ModError("Failed to load and run script from " +
				script_path + ":\n" + error_msg);
	}
	lua_pop(L, 1); // Pop error handler
}

#ifndef SERVER
void ScriptApiBase::loadModFromMemory(const std::string &mod_name)
{
	ModNameStorer mod_name_storer(getStack(), mod_name);

	sanity_check(m_type == ScriptingType::Client);

	const std::string init_filename = mod_name + ":init.lua";
	const std::string chunk_name = "@" + init_filename;

	const std::string *contents = getClient()->getModFile(init_filename);
	if (!contents)
		throw ModError("Mod \"" + mod_name + "\" lacks init.lua");

	verbosestream << "Loading and running script " << chunk_name << std::endl;

	lua_State *L = getStack();

	int error_handler = PUSH_ERROR_HANDLER(L);

	bool ok = ScriptApiSecurity::safeLoadString(L, *contents, chunk_name.c_str());
	if (ok)
		ok = !lua_pcall(L, 0, 0, error_handler);
	if (!ok) {
		const char *error_msg = lua_tostring(L, -1);
		if (!error_msg)
			error_msg = "(error object is not a string)";
		lua_pop(L, 2); // Pop error message and error handler
		throw ModError("Failed to load and run mod \"" +
				mod_name + "\":\n" + error_msg);
	}
	lua_pop(L, 1); // Pop error handler
}
#endif

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - replaces the table and arguments with the return value,
//     computed depending on mode
// This function must only be called with scriptlock held (i.e. inside of a
// code block with SCRIPTAPI_PRECHECKHEADER declared)
void ScriptApiBase::runCallbacksRaw(int nargs,
		RunCallbacksMode mode, const char *fxn)
{
#ifndef SERVER
	// Hard fail for bad guarded callbacks
	// Only run callbacks when the scripting enviroment is loaded
	FATAL_ERROR_IF(m_type == ScriptingType::Client &&
			!getClient()->modsLoaded(), fxn);
#endif

#ifdef SCRIPTAPI_LOCK_DEBUG
	assert(m_lock_recursion_count > 0);
#endif
	lua_State *L = getStack();
	FATAL_ERROR_IF(lua_gettop(L) < nargs + 1, "Not enough arguments");

	// Insert error handler
	PUSH_ERROR_HANDLER(L);
	int error_handler = lua_gettop(L) - nargs - 1;
	lua_insert(L, error_handler);

	// Insert run_callbacks between error handler and table
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "run_callbacks");
	lua_remove(L, -2);
	lua_insert(L, error_handler + 1);

	// Insert mode after table
	lua_pushnumber(L, (int)mode);
	lua_insert(L, error_handler + 3);

	// Stack now looks like this:
	// ... <error handler> <run_callbacks> <table> <mode> <arg#1> <arg#2> ... <arg#n>

	int result = lua_pcall(L, nargs + 2, 1, error_handler);
	if (result != 0)
		scriptError(result, fxn);

	lua_remove(L, error_handler);
}

void ScriptApiBase::realityCheck()
{
	int top = lua_gettop(m_luastack);
	if (top >= 30) {
		dstream << "Stack is over 30:" << std::endl;
		stackDump(dstream);
		std::string traceback = script_get_backtrace(m_luastack);
		throw LuaError("Stack is over 30 (reality check)\n" + traceback);
	}
}

void ScriptApiBase::scriptError(int result, const char *fxn)
{
	script_error(getStack(), result, m_last_run_mod.c_str(), fxn);
}

void ScriptApiBase::stackDump(std::ostream &o)
{
	int top = lua_gettop(m_luastack);
	for (int i = 1; i <= top; i++) {  /* repeat for each level */
		int t = lua_type(m_luastack, i);
		switch (t) {
			case LUA_TSTRING:  /* strings */
				o << "\"" << readParam<std::string>(m_luastack, i) << "\"";
				break;
			case LUA_TBOOLEAN:  /* booleans */
				o << (readParam<bool>(m_luastack, i) ? "true" : "false");
				break;
			case LUA_TNUMBER:  /* numbers */ {
				char buf[10];
				porting::mt_snprintf(buf, sizeof(buf), "%lf", lua_tonumber(m_luastack, i));
				o << buf;
				break;
			}
			default:  /* other values */
				o << lua_typename(m_luastack, t);
				break;
		}
		o << " ";
	}
	o << std::endl;
}

void ScriptApiBase::setOriginDirect(const char *origin)
{
	m_last_run_mod = origin ? origin : "??";
}

void ScriptApiBase::setOriginFromTableRaw(int index, const char *fxn)
{
	lua_State *L = getStack();
	m_last_run_mod = lua_istable(L, index) ?
		getstringfield_default(L, index, "mod_origin", "") : "";
}

/*
 * How ObjectRefs are handled in Lua:
 * When an active object is created, an ObjectRef is created on the Lua side
 * and stored in core.object_refs[id].
 * Methods that require an ObjectRef to a certain object retrieve it from that
 * table instead of creating their own.(*)
 * When an active object is removed, the existing ObjectRef is invalidated
 * using ::set_null() and removed from the core.object_refs table.
 * (*) An exception to this are NULL ObjectRefs and anonymous ObjectRefs
 *     for objects without ID.
 *     It's unclear what the latter are needed for and their use is problematic
 *     since we lose control over the ref and the contained pointer.
 */

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
	if (cobj == NULL || cobj->getId() == 0) {
		ObjectRef::create(L, cobj);
	} else {
		push_objectRef(L, cobj->getId());
		if (cobj->isGone())
			warningstream << "ScriptApiBase::objectrefGetOrCreate(): "
					<< "Pushing ObjectRef to removed/deactivated object"
					<< ", this is probably a bug." << std::endl;
	}
}

void ScriptApiBase::pushPlayerHPChangeReason(lua_State *L, const PlayerHPChangeReason &reason)
{
	if (reason.hasLuaReference())
		lua_rawgeti(L, LUA_REGISTRYINDEX, reason.lua_reference);
	else
		lua_newtable(L);

	lua_getfield(L, -1, "type");
	bool has_type = (bool)lua_isstring(L, -1);
	lua_pop(L, 1);
	if (!has_type) {
		lua_pushstring(L, reason.getTypeAsString().c_str());
		lua_setfield(L, -2, "type");
	}

	lua_pushstring(L, reason.from_mod ? "mod" : "engine");
	lua_setfield(L, -2, "from");

	if (reason.object) {
		objectrefGetOrCreate(L, reason.object);
		lua_setfield(L, -2, "object");
	}
	if (!reason.node.empty()) {
		lua_pushstring(L, reason.node.c_str());
		lua_setfield(L, -2, "node");
	}
}

Server* ScriptApiBase::getServer()
{
	return dynamic_cast<Server *>(m_gamedef);
}
#ifndef SERVER
Client* ScriptApiBase::getClient()
{
	return dynamic_cast<Client *>(m_gamedef);
}
#endif
