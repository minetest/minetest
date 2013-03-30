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

#include "scriptapi.h"
#include "scriptapi_common.h"

extern "C" {
#include "lauxlib.h"
}

#include "script.h"
#include "scriptapi_types.h"
#include "scriptapi_object.h"


Server* get_server(lua_State *L)
{
	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return server;
}

ServerEnvironment* get_env(lua_State *L)
{
	// Get environment from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_env");
	ServerEnvironment *env = (ServerEnvironment*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return env;
}

void warn_if_field_exists(lua_State *L, int table,
		const char *fieldname, const std::string &message)
{
	lua_getfield(L, table, fieldname);
	if(!lua_isnil(L, -1)){
		infostream<<script_get_backtrace(L)<<std::endl;
		infostream<<"WARNING: field \""<<fieldname<<"\": "
				<<message<<std::endl;
	}
	lua_pop(L, 1);
}

/*
	ToolCapabilities
*/

ToolCapabilities read_tool_capabilities(
		lua_State *L, int table)
{
	ToolCapabilities toolcap;
	getfloatfield(L, table, "full_punch_interval", toolcap.full_punch_interval);
	getintfield(L, table, "max_drop_level", toolcap.max_drop_level);
	lua_getfield(L, table, "groupcaps");
	if(lua_istable(L, -1)){
		int table_groupcaps = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table_groupcaps) != 0){
			// key at index -2 and value at index -1
			std::string groupname = luaL_checkstring(L, -2);
			if(lua_istable(L, -1)){
				int table_groupcap = lua_gettop(L);
				// This will be created
				ToolGroupCap groupcap;
				// Read simple parameters
				getintfield(L, table_groupcap, "maxlevel", groupcap.maxlevel);
				getintfield(L, table_groupcap, "uses", groupcap.uses);
				// DEPRECATED: maxwear
				float maxwear = 0;
				if(getfloatfield(L, table_groupcap, "maxwear", maxwear)){
					if(maxwear != 0)
						groupcap.uses = 1.0/maxwear;
					else
						groupcap.uses = 0;
					infostream<<script_get_backtrace(L)<<std::endl;
					infostream<<"WARNING: field \"maxwear\" is deprecated; "
							<<"should replace with uses=1/maxwear"<<std::endl;
				}
				// Read "times" table
				lua_getfield(L, table_groupcap, "times");
				if(lua_istable(L, -1)){
					int table_times = lua_gettop(L);
					lua_pushnil(L);
					while(lua_next(L, table_times) != 0){
						// key at index -2 and value at index -1
						int rating = luaL_checkinteger(L, -2);
						float time = luaL_checknumber(L, -1);
						groupcap.times[rating] = time;
						// removes value, keeps key for next iteration
						lua_pop(L, 1);
					}
				}
				lua_pop(L, 1);
				// Insert groupcap into toolcap
				toolcap.groupcaps[groupname] = groupcap;
			}
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	lua_getfield(L, table, "damage_groups");
	if(lua_istable(L, -1)){
		int table_damage_groups = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table_damage_groups) != 0){
			// key at index -2 and value at index -1
			std::string groupname = luaL_checkstring(L, -2);
			u16 value = luaL_checkinteger(L, -1);
			toolcap.damageGroups[groupname] = value;
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	return toolcap;
}

void set_tool_capabilities(lua_State *L, int table,
		const ToolCapabilities &toolcap)
{
	setfloatfield(L, table, "full_punch_interval", toolcap.full_punch_interval);
	setintfield(L, table, "max_drop_level", toolcap.max_drop_level);
	// Create groupcaps table
	lua_newtable(L);
	// For each groupcap
	for(std::map<std::string, ToolGroupCap>::const_iterator
			i = toolcap.groupcaps.begin(); i != toolcap.groupcaps.end(); i++){
		// Create groupcap table
		lua_newtable(L);
		const std::string &name = i->first;
		const ToolGroupCap &groupcap = i->second;
		// Create subtable "times"
		lua_newtable(L);
		for(std::map<int, float>::const_iterator
				i = groupcap.times.begin(); i != groupcap.times.end(); i++){
			int rating = i->first;
			float time = i->second;
			lua_pushinteger(L, rating);
			lua_pushnumber(L, time);
			lua_settable(L, -3);
		}
		// Set subtable "times"
		lua_setfield(L, -2, "times");
		// Set simple parameters
		setintfield(L, -1, "maxlevel", groupcap.maxlevel);
		setintfield(L, -1, "uses", groupcap.uses);
		// Insert groupcap table into groupcaps table
		lua_setfield(L, -2, name.c_str());
	}
	// Set groupcaps table
	lua_setfield(L, -2, "groupcaps");
	//Create damage_groups table
	lua_newtable(L);
	// For each damage group
	for(std::map<std::string, s16>::const_iterator
			i = toolcap.damageGroups.begin(); i != toolcap.damageGroups.end(); i++){
		// Create damage group table
		lua_pushinteger(L, i->second);
		lua_setfield(L, -2, i->first.c_str());
	}
	lua_setfield(L, -2, "damage_groups");
}

void push_tool_capabilities(lua_State *L,
		const ToolCapabilities &prop)
{
	lua_newtable(L);
	set_tool_capabilities(L, -1, prop);
}

void realitycheck(lua_State *L)
{
	int top = lua_gettop(L);
	if(top >= 30){
		dstream<<"Stack is over 30:"<<std::endl;
		stackDump(L, dstream);
		script_error(L, "Stack is over 30 (reality check)");
	}
}

/*
	PointedThing
*/

void push_pointed_thing(lua_State *L, const PointedThing& pointed)
{
	lua_newtable(L);
	if(pointed.type == POINTEDTHING_NODE)
	{
		lua_pushstring(L, "node");
		lua_setfield(L, -2, "type");
		push_v3s16(L, pointed.node_undersurface);
		lua_setfield(L, -2, "under");
		push_v3s16(L, pointed.node_abovesurface);
		lua_setfield(L, -2, "above");
	}
	else if(pointed.type == POINTEDTHING_OBJECT)
	{
		lua_pushstring(L, "object");
		lua_setfield(L, -2, "type");
		objectref_get(L, pointed.object_id);
		lua_setfield(L, -2, "ref");
	}
	else
	{
		lua_pushstring(L, "nothing");
		lua_setfield(L, -2, "type");
	}
}

void stackDump(lua_State *L, std::ostream &o)
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

#if 0
// Dump stack top with the dump2 function
static void dump2(lua_State *L, const char *name)
{
	// Dump object (debug)
	lua_getglobal(L, "dump2");
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, -2); // Get previous stack top as first parameter
	lua_pushstring(L, name);
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}
#endif

bool string_to_enum(const EnumString *spec, int &result,
		const std::string &str)
{
	const EnumString *esp = spec;
	while(esp->str){
		if(str == std::string(esp->str)){
			result = esp->num;
			return true;
		}
		esp++;
	}
	return false;
}

/*bool enum_to_string(const EnumString *spec, std::string &result,
		int num)
{
	const EnumString *esp = spec;
	while(esp){
		if(num == esp->num){
			result = esp->str;
			return true;
		}
		esp++;
	}
	return false;
}*/

int getenumfield(lua_State *L, int table,
		const char *fieldname, const EnumString *spec, int default_)
{
	int result = default_;
	string_to_enum(spec, result,
			getstringfield_default(L, table, fieldname, ""));
	return result;
}
