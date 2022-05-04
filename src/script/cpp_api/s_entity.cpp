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
#include "server.h"

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
	push_objectRef(L, id);
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

	int error_handler = PUSH_ERROR_HANDLER(L);

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

		setOriginFromTable(object);
		PCALL_RES(lua_pcall(L, 3, 0, error_handler));
	} else {
		lua_pop(L, 1);
	}
	lua_pop(L, 2); // Pop object and error handler
}

void ScriptApiEntity::luaentity_Deactivate(u16 id)
{
	SCRIPTAPI_PRECHECKHEADER

	verbosestream << "scriptapi_luaentity_deactivate: id=" << id << std::endl;

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Get the entity
	luaentity_get(L, id);
	int object = lua_gettop(L);

	// Get on_deactivate
	lua_getfield(L, -1, "on_deactivate");
	if (!lua_isnil(L, -1)) {
		luaL_checktype(L, -1, LUA_TFUNCTION);
		lua_pushvalue(L, object);

		setOriginFromTable(object);
		PCALL_RES(lua_pcall(L, 1, 0, error_handler));
	} else {
		lua_pop(L, 1);
	}
	lua_pop(L, 2); // Pop object and error handler
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

	int error_handler = PUSH_ERROR_HANDLER(L);

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

	setOriginFromTable(object);
	PCALL_RES(lua_pcall(L, 1, 1, error_handler));

	lua_remove(L, object);
	lua_remove(L, error_handler);

	size_t len = 0;
	const char *s = lua_tolstring(L, -1, &len);
	lua_pop(L, 1); // Pop static data
	return std::string(s, len);
}

void ScriptApiEntity::luaentity_GetProperties(u16 id,
		ServerActiveObject *self, ObjectProperties *prop)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_get_properties: id="<<id<<std::endl;

	// Get core.luaentities[id]
	luaentity_get(L, id);

	// Set default values that differ from ObjectProperties defaults
	prop->hp_max = 10;

	// Deprecated: read object properties directly
	read_object_properties(L, -1, self, prop, getServer()->idef());

	// Read initial_properties
	lua_getfield(L, -1, "initial_properties");
	read_object_properties(L, -1, self, prop, getServer()->idef());
	lua_pop(L, 1);
}

void ScriptApiEntity::luaentity_Step(u16 id, float dtime,
	const collisionMoveResult *moveresult)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

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
	/* moveresult */
	if (moveresult)
		push_collision_move_result(L, *moveresult);
	else
		lua_pushnil(L);

	setOriginFromTable(object);
	PCALL_RES(lua_pcall(L, 3, 0, error_handler));

	lua_pop(L, 2); // Pop object and error handler
}

// Calls entity:on_punch(ObjectRef puncher, time_from_last_punch,
//                       tool_capabilities, direction, damage)
bool ScriptApiEntity::luaentity_Punch(u16 id,
		ServerActiveObject *puncher, float time_from_last_punch,
		const ToolCapabilities *toolcap, v3f dir, s32 damage)
{
	SCRIPTAPI_PRECHECKHEADER

	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Get core.luaentities[id]
	luaentity_get(L,id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get function
	lua_getfield(L, -1, "on_punch");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2); // Pop on_punch and entity
		return false;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object);  // self
	objectrefGetOrCreate(L, puncher);  // Clicker reference
	lua_pushnumber(L, time_from_last_punch);
	push_tool_capabilities(L, *toolcap);
	push_v3f(L, dir);
	lua_pushnumber(L, damage);

	setOriginFromTable(object);
	PCALL_RES(lua_pcall(L, 6, 1, error_handler));

	bool retval = readParam<bool>(L, -1);
	lua_pop(L, 2); // Pop object and error handler
	return retval;
}

// Calls entity[field](ObjectRef self, ObjectRef sao)
bool ScriptApiEntity::luaentity_run_simple_callback(u16 id,
	ServerActiveObject *sao, const char *field)
{
	SCRIPTAPI_PRECHECKHEADER

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Get core.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get function
	lua_getfield(L, -1, field);
	if (lua_isnil(L, -1)) {
		lua_pop(L, 2); // Pop callback field and entity
		return false;
	}
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object);  // self
	objectrefGetOrCreate(L, sao);  // killer reference

	setOriginFromTable(object);
	PCALL_RES(lua_pcall(L, 2, 1, error_handler));

	bool retval = readParam<bool>(L, -1);
	lua_pop(L, 2); // Pop object and error handler
	return retval;
}

bool ScriptApiEntity::luaentity_on_death(u16 id, ServerActiveObject *killer)
{
	return luaentity_run_simple_callback(id, killer, "on_death");
}

// Calls entity:on_rightclick(ObjectRef clicker)
void ScriptApiEntity::luaentity_Rightclick(u16 id, ServerActiveObject *clicker)
{
	luaentity_run_simple_callback(id, clicker, "on_rightclick");
}

void ScriptApiEntity::luaentity_on_attach_child(u16 id, ServerActiveObject *child)
{
	luaentity_run_simple_callback(id, child, "on_attach_child");
}

void ScriptApiEntity::luaentity_on_detach_child(u16 id, ServerActiveObject *child)
{
	luaentity_run_simple_callback(id, child, "on_detach_child");
}

void ScriptApiEntity::luaentity_on_detach(u16 id, ServerActiveObject *parent)
{
	luaentity_run_simple_callback(id, parent, "on_detach");
}
