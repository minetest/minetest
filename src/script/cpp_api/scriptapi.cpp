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

extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}


#include "scriptapi.h"
#include "common/c_converter.h"
#include "lua_api/l_base.h"
#include "log.h"
#include "mods.h"

int script_ErrorHandler(lua_State *L) {
	lua_getfield(L, LUA_GLOBALSINDEX, "debug");
	if (!lua_istable(L, -1)) {
		lua_pop(L, 1);
		return 1;
	}
	lua_getfield(L, -1, "traceback");
	if (!lua_isfunction(L, -1)) {
		lua_pop(L, 2);
		return 1;
	}
	lua_pushvalue(L, 1);
	lua_pushinteger(L, 2);
	lua_call(L, 2, 1);
	return 1;
}


bool ScriptApi::getAuth(const std::string &playername,
		std::string *dst_password, std::set<std::string> *dst_privs)
{
	SCRIPTAPI_PRECHECKHEADER

	getAuthHandler();
	lua_getfield(L, -1, "get_auth");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing get_auth");
	lua_pushstring(L, playername.c_str());
	if(lua_pcall(L, 1, 1, 0))
		scriptError("error: %s", lua_tostring(L, -1));

	// nil = login not allowed
	if(lua_isnil(L, -1))
		return false;
	luaL_checktype(L, -1, LUA_TTABLE);

	std::string password;
	bool found = getstringfield(L, -1, "password", password);
	if(!found)
		throw LuaError(L, "Authentication handler didn't return password");
	if(dst_password)
		*dst_password = password;

	lua_getfield(L, -1, "privileges");
	if(!lua_istable(L, -1))
		throw LuaError(L,
				"Authentication handler didn't return privilege table");
	if(dst_privs)
		readPrivileges(-1, *dst_privs);
	lua_pop(L, 1);

	return true;
}

void ScriptApi::getAuthHandler()
{
	lua_State *L = getStack();

	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_auth_handler");
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		lua_getfield(L, -1, "builtin_auth_handler");
	}
	if(lua_type(L, -1) != LUA_TTABLE)
		throw LuaError(L, "Authentication handler table not valid");
}

void ScriptApi::readPrivileges(int index,std::set<std::string> &result)
{
	lua_State *L = getStack();

	result.clear();
	lua_pushnil(L);
	if(index < 0)
		index -= 1;
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		std::string key = luaL_checkstring(L, -2);
		bool value = lua_toboolean(L, -1);
		if(value)
			result.insert(key);
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

void ScriptApi::createAuth(const std::string &playername,
		const std::string &password)
{
	SCRIPTAPI_PRECHECKHEADER

	getAuthHandler();
	lua_getfield(L, -1, "create_auth");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing create_auth");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	if(lua_pcall(L, 2, 0, 0))
		scriptError("error: %s", lua_tostring(L, -1));
}

bool ScriptApi::setPassword(const std::string &playername,
		const std::string &password)
{
	SCRIPTAPI_PRECHECKHEADER

	getAuthHandler();
	lua_getfield(L, -1, "set_password");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing set_password");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	if(lua_pcall(L, 2, 1, 0))
		scriptError("error: %s", lua_tostring(L, -1));
	return lua_toboolean(L, -1);
}

bool ScriptApi::on_chat_message(const std::string &name,
		const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get minetest.registered_on_chat_messages
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_chat_messages");
	// Call callbacks
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, message.c_str());
	runCallbacks(2, RUN_CALLBACKS_MODE_OR_SC);
	bool ate = lua_toboolean(L, -1);
	return ate;
}

void ScriptApi::on_shutdown()
{
	SCRIPTAPI_PRECHECKHEADER

	// Get registered shutdown hooks
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_shutdown");
	// Call callbacks
	runCallbacks(0, RUN_CALLBACKS_MODE_FIRST);
}

bool ScriptApi::loadMod(const std::string &scriptpath,const std::string &modname)
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
		success = scriptLoad(scriptpath.c_str());
	}
	catch(LuaError &e){
		errorstream<<"Error loading mod \""<<modname
				<<"\": "<<e.what()<<std::endl;
	}

	return success;
}

ScriptApi::ScriptApi() {
	assert("Invalid call to default constructor of scriptapi!" == 0);
}

ScriptApi::ScriptApi(Server* server)
{

	setServer(server);
	setStack(luaL_newstate());
	assert(getStack());

	//TODO add security

	luaL_openlibs(getStack());

	SCRIPTAPI_PRECHECKHEADER

	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, "scriptapi");

	lua_newtable(L);
	lua_setglobal(L, "minetest");


	for (std::vector<ModApiBase*>::iterator i = m_mod_api_modules->begin();
			i != m_mod_api_modules->end(); i++) {
		//initializers are called within minetest global table!
		lua_getglobal(L, "minetest");
		int top = lua_gettop(L);
		bool ModInitializedSuccessfull = (*i)->Initialize(L,top);
		assert(ModInitializedSuccessfull);
	}

	infostream << "SCRIPTAPI: initialized " << m_mod_api_modules->size()
													<< " modules" << std::endl;

	// Get the main minetest table
	lua_getglobal(L, "minetest");

	// Add tables to minetest
	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");

	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");
}

ScriptApi::~ScriptApi() {
	lua_close(getStack());
}

bool ScriptApi::scriptLoad(const char *path)
{
	lua_State* L = getStack();
	setStack(0);

	verbosestream<<"Loading and running script from "<<path<<std::endl;

	lua_pushcfunction(L, script_ErrorHandler);
	int errorhandler = lua_gettop(L);

	int ret = luaL_loadfile(L, path) || lua_pcall(L, 0, 0, errorhandler);
	if(ret){
		errorstream<<"========== ERROR FROM LUA ==========="<<std::endl;
		errorstream<<"Failed to load and run script from "<<std::endl;
		errorstream<<path<<":"<<std::endl;
		errorstream<<std::endl;
		errorstream<<lua_tostring(L, -1)<<std::endl;
		errorstream<<std::endl;
		errorstream<<"=======END OF ERROR FROM LUA ========"<<std::endl;
		lua_pop(L, 1); // Pop error message from stack
		lua_pop(L, 1); // Pop the error handler from stack
		return false;
	}
	lua_pop(L, 1); // Pop the error handler from stack
	return true;
}

bool ScriptApi::registerModApiModule(ModApiBase* ptr) {
	if (ScriptApi::m_mod_api_modules == 0)
		ScriptApi::m_mod_api_modules = new std::vector<ModApiBase*>();

	assert(ScriptApi::m_mod_api_modules != 0);

	ScriptApi::m_mod_api_modules->push_back(ptr);

	return true;
}

std::vector<ModApiBase*>* ScriptApi::m_mod_api_modules = 0;
