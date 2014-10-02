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

#include "cpp_api/s_entity.h"
#include "cpp_api/s_internal.h"
#include "log.h"
#include "object_properties.h"
#include "common/c_converter.h"
#include "common/c_content.h"

bool ScriptApiEntity::luaentity_Add(u16 id, const char *name)
{
	SCRIPTAPI_PRECHECKHEADER

	verbosestream<<"scriptapi_luaentity_add: id="<<id<<" name=\""
			<<name<<"\""<<std::endl;

	// Get core.registered_entities[name]
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_entities");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	// Should be a table, which we will use as a prototype
	//luaL_checktype(L, -1, LUA_TTABLE);
	if (lua_type(L, -1) != LUA_TTABLE){
		errorstream<<"LuaEntity name \""<<name<<"\" not defined"<<std::endl;
		return false;
	}
	int prototype_table = lua_gettop(L);
	//dump2(L, "prototype_table");

	// Create entity object
	lua_newtable(L);
	int object = lua_gettop(L);

	// Set object metatable
	lua_pushvalue(L, prototype_table);
	lua_setmetatable(L, -2);

	// Add object reference
	// This should be userdata with metatable ObjectRef
	objectrefGet(L, id);
	luaL_checktype(L, -1, LUA_TUSERDATA);
	if (!luaL_checkudata(L, -1, "ObjectRef"))
		luaL_typerror(L, -1, "ObjectRef");
	lua_setfield(L, -2, "object");

	// core.luaentities[id] = object
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id); // Push id
	lua_pushvalue(L, object); // Copy object to top of stack
	lua_settable(L, -3);

	return true;
}

void ScriptApiEntity::luaentity_Activate(u16 id,
		const std::string &staticdata, u32 dtime_s)
{
	SCRIPTAPI_PRECHECKHEADER

	verbosestream << "scriptapi_luaentity_activate: id=" << id << std::endl;

	// Get core.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);

	// Get on_activate function
	lua_getfield(L, -1, "on_activate");
	if (!lua_isnil(L, -1)) {
		luaL_checktype(L, -1, LUA_TFUNCTION);
		lua_pushvalue(L, object); // self
		lua_pushlstring(L, staticdata.c_str(), staticdata.size());
		lua_pushinteger(L, dtime_s);
		// Call with 3 arguments, 0 results
		if (lua_pcall(L, 3, 0, m_errorhandler))
			scriptError();
	} else {
		lua_pop(L, 1);
	}
	lua_pop(L, 1); // Pop object
}

void ScriptApiEntity::luaentity_Remove(u16 id)
{
	SCRIPTAPI_PRECHECKHEADER

	verbosestream << "scriptapi_luaentity_rm: id=" << id << std::endl;

	// Get core.luaentities table
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);

	// Set luaentities[id] = nil
	lua_pushnumber(L, id); // Push id
	lua_pushnil(L);
	lua_settable(L, objectstable);

	lua_pop(L, 2); // pop luaentities, core
}

std::string ScriptApiEntity::luaentity_GetStaticdata(u16 id)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_get_staticdata: id="<<id<<std::endl;

	// Get core.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);

	// Get get_staticdata function
	lua_getfield(L, -1, "get_staticdata");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2); // Pop entity and  get_staticdata
		return "";
	}

	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	// Call with 1 arguments, 1 results
	if (lua_pcall(L, 1, 1, m_errorhandler))
		scriptError();
	lua_remove(L, object); // Remove object

	size_t len = 0;
	const char *s = lua_tolstring(L, -1, &len);
	lua_pop(L, 1); // Pop static data
	return std::string(s, len);
}

void ScriptApiEntity::luaentity_GetProperties(u16 id,
		ObjectProperties *prop)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_get_properties: id="<<id<<std::endl;

	// Get core.luaentities[id]
	luaentity_get(L, id);

	// Set default values that differ from ObjectProperties defaults
	prop->hp_max = 10;

	/* Read stuff */

	prop->hp_max = getintfield_default(L, -1, "hp_max", 10);

	getboolfield(L, -1, "physical", prop->physical);
	getboolfield(L, -1, "collide_with_objects", prop->collideWithObjects);

	getfloatfield(L, -1, "weight", prop->weight);

	lua_getfield(L, -1, "collisionbox");
	if (lua_istable(L, -1))
		prop->collisionbox = read_aabb3f(L, -1, 1.0);
	lua_pop(L, 1);

	getstringfield(L, -1, "visual", prop->visual);

	getstringfield(L, -1, "mesh", prop->mesh);

	// Deprecated: read object properties directly
	read_object_properties(L, -1, prop);

	// Read initial_properties
	lua_getfield(L, -1, "initial_properties");
	read_object_properties(L, -1, prop);
	lua_pop(L, 1);
}

void ScriptApiEntity::luaentity_Step(u16 id, float dtime)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;

	// Get core.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get step function
	lua_getfield(L, -1, "on_step");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2); // Pop on_step and entity
		return;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	lua_pushnumber(L, dtime); // dtime
	// Call with 2 arguments, 0 results
	if (lua_pcall(L, 2, 0, m_errorhandler))
		scriptError();
	lua_pop(L, 1); // Pop object
}

// Calls entity:on_punch(ObjectRef puncher, time_from_last_punch,
//                       tool_capabilities, direction)
void ScriptApiEntity::luaentity_Punch(u16 id,
		ServerActiveObject *puncher, float time_from_last_punch,
		const ToolCapabilities *toolcap, v3f dir)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;

	// Get core.luaentities[id]
	luaentity_get(L,id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get function
	lua_getfield(L, -1, "on_punch");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2); // Pop on_punch and entitu
		return;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object);  // self
	objectrefGetOrCreate(L, puncher);  // Clicker reference
	lua_pushnumber(L, time_from_last_punch);
	push_tool_capabilities(L, *toolcap);
	push_v3f(L, dir);
	// Call with 5 arguments, 0 results
	if (lua_pcall(L, 5, 0, m_errorhandler))
		scriptError();
	lua_pop(L, 1); // Pop object
}

// Calls entity:on_rightclick(ObjectRef clicker)
void ScriptApiEntity::luaentity_Rightclick(u16 id,
		ServerActiveObject *clicker)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;

	// Get core.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get function
	lua_getfield(L, -1, "on_rightclick");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2); // Pop on_rightclick and entity
		return;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	objectrefGetOrCreate(L, clicker); // Clicker reference
	// Call with 2 arguments, 0 results
	if (lua_pcall(L, 2, 0, m_errorhandler))
		scriptError();
	lua_pop(L, 1); // Pop object
}

