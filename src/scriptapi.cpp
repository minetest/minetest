/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <iostream>
#include <list>
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
#include "object_properties.h"
#include "content_sao.h" // For LuaEntitySAO and PlayerSAO
#include "itemdef.h"
#include "nodedef.h"
#include "craftdef.h"
#include "main.h" // For g_settings
#include "settings.h" // For accessing g_settings
#include "nodemetadata.h"
#include "mapblock.h" // For getNodeBlockPos
#include "content_nodemeta.h"
#include "tool.h"
#include "daynightratio.h"
#include "noise.h" // PseudoRandom for LuaPseudoRandom
#include "util/pointedthing.h"
#include "rollback.h"

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

class StackUnroller
{
private:
	lua_State *m_lua;
	int m_original_top;
public:
	StackUnroller(lua_State *L):
		m_lua(L),
		m_original_top(-1)
	{
		m_original_top = lua_gettop(m_lua); // store stack height
	}
	~StackUnroller()
	{
		lua_settop(m_lua, m_original_top); // restore stack height
	}
};

class ModNameStorer
{
private:
	lua_State *L;
public:
	ModNameStorer(lua_State *L_, const std::string modname):
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
	Getters for stuff in main tables
*/

static Server* get_server(lua_State *L)
{
	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return server;
}

static ServerEnvironment* get_env(lua_State *L)
{
	// Get environment from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_env");
	ServerEnvironment *env = (ServerEnvironment*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	return env;
}

static void objectref_get(lua_State *L, u16 id)
{
	// Get minetest.object_refs[i]
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "object_refs");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // object_refs
	lua_remove(L, -2); // minetest
}

static void luaentity_get(lua_State *L, u16 id)
{
	// Get minetest.luaentities[i]
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // luaentities
	lua_remove(L, -2); // minetest
}

/*
	Table field getters
*/

static bool getstringfield(lua_State *L, int table,
		const char *fieldname, std::string &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isstring(L, -1)){
		size_t len = 0;
		const char *ptr = lua_tolstring(L, -1, &len);
		result.assign(ptr, len);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

static bool getintfield(lua_State *L, int table,
		const char *fieldname, int &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isnumber(L, -1)){
		result = lua_tonumber(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

static bool getfloatfield(lua_State *L, int table,
		const char *fieldname, float &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isnumber(L, -1)){
		result = lua_tonumber(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

static bool getboolfield(lua_State *L, int table,
		const char *fieldname, bool &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isboolean(L, -1)){
		result = lua_toboolean(L, -1);
		got = true;
	}
	lua_pop(L, 1);
	return got;
}

static std::string checkstringfield(lua_State *L, int table,
		const char *fieldname)
{
	lua_getfield(L, table, fieldname);
	std::string s = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	return s;
}

static std::string getstringfield_default(lua_State *L, int table,
		const char *fieldname, const std::string &default_)
{
	std::string result = default_;
	getstringfield(L, table, fieldname, result);
	return result;
}

static int getintfield_default(lua_State *L, int table,
		const char *fieldname, int default_)
{
	int result = default_;
	getintfield(L, table, fieldname, result);
	return result;
}

static float getfloatfield_default(lua_State *L, int table,
		const char *fieldname, float default_)
{
	float result = default_;
	getfloatfield(L, table, fieldname, result);
	return result;
}

static bool getboolfield_default(lua_State *L, int table,
		const char *fieldname, bool default_)
{
	bool result = default_;
	getboolfield(L, table, fieldname, result);
	return result;
}

struct EnumString
{
	int num;
	const char *str;
};

static bool string_to_enum(const EnumString *spec, int &result,
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

/*static bool enum_to_string(const EnumString *spec, std::string &result,
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

static int getenumfield(lua_State *L, int table,
		const char *fieldname, const EnumString *spec, int default_)
{
	int result = default_;
	string_to_enum(spec, result,
			getstringfield_default(L, table, fieldname, ""));
	return result;
}

static void setintfield(lua_State *L, int table,
		const char *fieldname, int value)
{
	lua_pushinteger(L, value);
	if(table < 0)
		table -= 1;
	lua_setfield(L, table, fieldname);
}

static void setfloatfield(lua_State *L, int table,
		const char *fieldname, float value)
{
	lua_pushnumber(L, value);
	if(table < 0)
		table -= 1;
	lua_setfield(L, table, fieldname);
}

static void setboolfield(lua_State *L, int table,
		const char *fieldname, bool value)
{
	lua_pushboolean(L, value);
	if(table < 0)
		table -= 1;
	lua_setfield(L, table, fieldname);
}

static void warn_if_field_exists(lua_State *L, int table,
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
	EnumString definitions
*/

struct EnumString es_ItemType[] =
{
	{ITEM_NONE, "none"},
	{ITEM_NODE, "node"},
	{ITEM_CRAFT, "craft"},
	{ITEM_TOOL, "tool"},
	{0, NULL},
};

struct EnumString es_DrawType[] =
{
	{NDT_NORMAL, "normal"},
	{NDT_AIRLIKE, "airlike"},
	{NDT_LIQUID, "liquid"},
	{NDT_FLOWINGLIQUID, "flowingliquid"},
	{NDT_GLASSLIKE, "glasslike"},
	{NDT_ALLFACES, "allfaces"},
	{NDT_ALLFACES_OPTIONAL, "allfaces_optional"},
	{NDT_TORCHLIKE, "torchlike"},
	{NDT_SIGNLIKE, "signlike"},
	{NDT_PLANTLIKE, "plantlike"},
	{NDT_FENCELIKE, "fencelike"},
	{NDT_RAILLIKE, "raillike"},
	{NDT_NODEBOX, "nodebox"},
	{0, NULL},
};

struct EnumString es_ContentParamType[] =
{
	{CPT_NONE, "none"},
	{CPT_LIGHT, "light"},
	{0, NULL},
};

struct EnumString es_ContentParamType2[] =
{
	{CPT2_NONE, "none"},
	{CPT2_FULL, "full"},
	{CPT2_FLOWINGLIQUID, "flowingliquid"},
	{CPT2_FACEDIR, "facedir"},
	{CPT2_WALLMOUNTED, "wallmounted"},
	{0, NULL},
};

struct EnumString es_LiquidType[] =
{
	{LIQUID_NONE, "none"},
	{LIQUID_FLOWING, "flowing"},
	{LIQUID_SOURCE, "source"},
	{0, NULL},
};

struct EnumString es_NodeBoxType[] =
{
	{NODEBOX_REGULAR, "regular"},
	{NODEBOX_FIXED, "fixed"},
	{NODEBOX_WALLMOUNTED, "wallmounted"},
	{0, NULL},
};

struct EnumString es_CraftMethod[] =
{
	{CRAFT_METHOD_NORMAL, "normal"},
	{CRAFT_METHOD_COOKING, "cooking"},
	{CRAFT_METHOD_FUEL, "fuel"},
	{0, NULL},
};

struct EnumString es_TileAnimationType[] =
{
	{TAT_NONE, "none"},
	{TAT_VERTICAL_FRAMES, "vertical_frames"},
	{0, NULL},
};

/*
	C struct <-> Lua table converter functions
*/

static void push_v3f(lua_State *L, v3f p)
{
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, p.Z);
	lua_setfield(L, -2, "z");
}

static v2s16 read_v2s16(lua_State *L, int index)
{
	v2s16 p;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	p.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	p.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return p;
}

static v2f read_v2f(lua_State *L, int index)
{
	v2f p;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	p.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	p.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return p;
}

static v3f read_v3f(lua_State *L, int index)
{
	v3f pos;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	pos.X = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	pos.Y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "z");
	pos.Z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return pos;
}

static v3f check_v3f(lua_State *L, int index)
{
	v3f pos;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "x");
	pos.X = luaL_checknumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "y");
	pos.Y = luaL_checknumber(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, index, "z");
	pos.Z = luaL_checknumber(L, -1);
	lua_pop(L, 1);
	return pos;
}

static void pushFloatPos(lua_State *L, v3f p)
{
	p /= BS;
	push_v3f(L, p);
}

static v3f checkFloatPos(lua_State *L, int index)
{
	return check_v3f(L, index) * BS;
}

static void push_v3s16(lua_State *L, v3s16 p)
{
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, p.Z);
	lua_setfield(L, -2, "z");
}

static v3s16 read_v3s16(lua_State *L, int index)
{
	// Correct rounding at <0
	v3f pf = read_v3f(L, index);
	return floatToInt(pf, 1.0);
}

static v3s16 check_v3s16(lua_State *L, int index)
{
	// Correct rounding at <0
	v3f pf = check_v3f(L, index);
	return floatToInt(pf, 1.0);
}

static void pushnode(lua_State *L, const MapNode &n, INodeDefManager *ndef)
{
	lua_newtable(L);
	lua_pushstring(L, ndef->get(n).name.c_str());
	lua_setfield(L, -2, "name");
	lua_pushnumber(L, n.getParam1());
	lua_setfield(L, -2, "param1");
	lua_pushnumber(L, n.getParam2());
	lua_setfield(L, -2, "param2");
}

static MapNode readnode(lua_State *L, int index, INodeDefManager *ndef)
{
	lua_getfield(L, index, "name");
	const char *name = luaL_checkstring(L, -1);
	lua_pop(L, 1);
	u8 param1;
	lua_getfield(L, index, "param1");
	if(lua_isnil(L, -1))
		param1 = 0;
	else
		param1 = lua_tonumber(L, -1);
	lua_pop(L, 1);
	u8 param2;
	lua_getfield(L, index, "param2");
	if(lua_isnil(L, -1))
		param2 = 0;
	else
		param2 = lua_tonumber(L, -1);
	lua_pop(L, 1);
	return MapNode(ndef, name, param1, param2);
}

static video::SColor readARGB8(lua_State *L, int index)
{
	video::SColor color;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_getfield(L, index, "a");
	if(lua_isnumber(L, -1))
		color.setAlpha(lua_tonumber(L, -1));
	lua_pop(L, 1);
	lua_getfield(L, index, "r");
	color.setRed(lua_tonumber(L, -1));
	lua_pop(L, 1);
	lua_getfield(L, index, "g");
	color.setGreen(lua_tonumber(L, -1));
	lua_pop(L, 1);
	lua_getfield(L, index, "b");
	color.setBlue(lua_tonumber(L, -1));
	lua_pop(L, 1);
	return color;
}

static aabb3f read_aabb3f(lua_State *L, int index, f32 scale)
{
	aabb3f box;
	if(lua_istable(L, index)){
		lua_rawgeti(L, index, 1);
		box.MinEdge.X = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 2);
		box.MinEdge.Y = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 3);
		box.MinEdge.Z = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 4);
		box.MaxEdge.X = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 5);
		box.MaxEdge.Y = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, index, 6);
		box.MaxEdge.Z = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
	}
	return box;
}

static std::vector<aabb3f> read_aabb3f_vector(lua_State *L, int index, f32 scale)
{
	std::vector<aabb3f> boxes;
	if(lua_istable(L, index)){
		int n = lua_objlen(L, index);
		// Check if it's a single box or a list of boxes
		bool possibly_single_box = (n == 6);
		for(int i = 1; i <= n && possibly_single_box; i++){
			lua_rawgeti(L, index, i);
			if(!lua_isnumber(L, -1))
				possibly_single_box = false;
			lua_pop(L, 1);
		}
		if(possibly_single_box){
			// Read a single box
			boxes.push_back(read_aabb3f(L, index, scale));
		} else {
			// Read a list of boxes
			for(int i = 1; i <= n; i++){
				lua_rawgeti(L, index, i);
				boxes.push_back(read_aabb3f(L, -1, scale));
				lua_pop(L, 1);
			}
		}
	}
	return boxes;
}

static NodeBox read_nodebox(lua_State *L, int index)
{
	NodeBox nodebox;
	if(lua_istable(L, -1)){
		nodebox.type = (NodeBoxType)getenumfield(L, index, "type",
				es_NodeBoxType, NODEBOX_REGULAR);

		lua_getfield(L, index, "fixed");
		if(lua_istable(L, -1))
			nodebox.fixed = read_aabb3f_vector(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, index, "wall_top");
		if(lua_istable(L, -1))
			nodebox.wall_top = read_aabb3f(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, index, "wall_bottom");
		if(lua_istable(L, -1))
			nodebox.wall_bottom = read_aabb3f(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, index, "wall_side");
		if(lua_istable(L, -1))
			nodebox.wall_side = read_aabb3f(L, -1, BS);
		lua_pop(L, 1);
	}
	return nodebox;
}

/*
	Groups
*/
static void read_groups(lua_State *L, int index,
		std::map<std::string, int> &result)
{
	if (!lua_istable(L,index))
		return;
	result.clear();
	lua_pushnil(L);
	if(index < 0)
		index -= 1;
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		std::string name = luaL_checkstring(L, -2);
		int rating = luaL_checkinteger(L, -1);
		result[name] = rating;
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
}

/*
	Privileges
*/
static void read_privileges(lua_State *L, int index,
		std::set<std::string> &result)
{
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

/*
	ToolCapabilities
*/

static ToolCapabilities read_tool_capabilities(
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
	return toolcap;
}

static void set_tool_capabilities(lua_State *L, int table,
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
}

static void push_tool_capabilities(lua_State *L,
		const ToolCapabilities &prop)
{
	lua_newtable(L);
	set_tool_capabilities(L, -1, prop);
}

/*
	DigParams
*/

static void set_dig_params(lua_State *L, int table,
		const DigParams &params)
{
	setboolfield(L, table, "diggable", params.diggable);
	setfloatfield(L, table, "time", params.time);
	setintfield(L, table, "wear", params.wear);
}

static void push_dig_params(lua_State *L,
		const DigParams &params)
{
	lua_newtable(L);
	set_dig_params(L, -1, params);
}

/*
	HitParams
*/

static void set_hit_params(lua_State *L, int table,
		const HitParams &params)
{
	setintfield(L, table, "hp", params.hp);
	setintfield(L, table, "wear", params.wear);
}

static void push_hit_params(lua_State *L,
		const HitParams &params)
{
	lua_newtable(L);
	set_hit_params(L, -1, params);
}

/*
	PointedThing
*/

static void push_pointed_thing(lua_State *L, const PointedThing& pointed)
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

/*
	SimpleSoundSpec
*/

static void read_soundspec(lua_State *L, int index, SimpleSoundSpec &spec)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	if(lua_isnil(L, index)){
	} else if(lua_istable(L, index)){
		getstringfield(L, index, "name", spec.name);
		getfloatfield(L, index, "gain", spec.gain);
	} else if(lua_isstring(L, index)){
		spec.name = lua_tostring(L, index);
	}
}

/*
	ObjectProperties
*/

static void read_object_properties(lua_State *L, int index,
		ObjectProperties *prop)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	if(!lua_istable(L, index))
		return;

	prop->hp_max = getintfield_default(L, -1, "hp_max", 10);

	getboolfield(L, -1, "physical", prop->physical);

	getfloatfield(L, -1, "weight", prop->weight);

	lua_getfield(L, -1, "collisionbox");
	if(lua_istable(L, -1))
		prop->collisionbox = read_aabb3f(L, -1, 1.0);
	lua_pop(L, 1);

	getstringfield(L, -1, "visual", prop->visual);

	getstringfield(L, -1, "mesh", prop->mesh);
	
	lua_getfield(L, -1, "visual_size");
	if(lua_istable(L, -1))
		prop->visual_size = read_v2f(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "textures");
	if(lua_istable(L, -1)){
		prop->textures.clear();
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			if(lua_isstring(L, -1))
				prop->textures.push_back(lua_tostring(L, -1));
			else
				prop->textures.push_back("");
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "colors");
	if(lua_istable(L, -1)){
		prop->colors.clear();
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			if(lua_isstring(L, -1))
				prop->colors.push_back(readARGB8(L, -1));
			else
				prop->colors.push_back(video::SColor(255, 255, 255, 255));
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
	
	lua_getfield(L, -1, "spritediv");
	if(lua_istable(L, -1))
		prop->spritediv = read_v2s16(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "initial_sprite_basepos");
	if(lua_istable(L, -1))
		prop->initial_sprite_basepos = read_v2s16(L, -1);
	lua_pop(L, 1);
	
	getboolfield(L, -1, "is_visible", prop->is_visible);
	getboolfield(L, -1, "makes_footstep_sound", prop->makes_footstep_sound);
	getfloatfield(L, -1, "automatic_rotate", prop->automatic_rotate);
}

/*
	ItemDefinition
*/

static ItemDefinition read_item_definition(lua_State *L, int index,
		ItemDefinition default_def = ItemDefinition())
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	// Read the item definition
	ItemDefinition def = default_def;

	def.type = (ItemType)getenumfield(L, index, "type",
			es_ItemType, ITEM_NONE);
	getstringfield(L, index, "name", def.name);
	getstringfield(L, index, "description", def.description);
	getstringfield(L, index, "inventory_image", def.inventory_image);
	getstringfield(L, index, "wield_image", def.wield_image);

	lua_getfield(L, index, "wield_scale");
	if(lua_istable(L, -1)){
		def.wield_scale = check_v3f(L, -1);
	}
	lua_pop(L, 1);

	def.stack_max = getintfield_default(L, index, "stack_max", def.stack_max);
	if(def.stack_max == 0)
		def.stack_max = 1;

	lua_getfield(L, index, "on_use");
	def.usable = lua_isfunction(L, -1);
	lua_pop(L, 1);

	getboolfield(L, index, "liquids_pointable", def.liquids_pointable);

	warn_if_field_exists(L, index, "tool_digging_properties",
			"deprecated: use tool_capabilities");
	
	lua_getfield(L, index, "tool_capabilities");
	if(lua_istable(L, -1)){
		def.tool_capabilities = new ToolCapabilities(
				read_tool_capabilities(L, -1));
	}

	// If name is "" (hand), ensure there are ToolCapabilities
	// because it will be looked up there whenever any other item has
	// no ToolCapabilities
	if(def.name == "" && def.tool_capabilities == NULL){
		def.tool_capabilities = new ToolCapabilities();
	}

	lua_getfield(L, index, "groups");
	read_groups(L, -1, def.groups);
	lua_pop(L, 1);

	// Client shall immediately place this node when player places the item.
	// Server will update the precise end result a moment later.
	// "" = no prediction
	getstringfield(L, index, "node_placement_prediction",
			def.node_placement_prediction);

	return def;
}

/*
	TileDef
*/

static TileDef read_tiledef(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	
	TileDef tiledef;

	// key at index -2 and value at index
	if(lua_isstring(L, index)){
		// "default_lava.png"
		tiledef.name = lua_tostring(L, index);
	}
	else if(lua_istable(L, index))
	{
		// {name="default_lava.png", animation={}}
		tiledef.name = "";
		getstringfield(L, index, "name", tiledef.name);
		getstringfield(L, index, "image", tiledef.name); // MaterialSpec compat.
		tiledef.backface_culling = getboolfield_default(
					L, index, "backface_culling", true);
		// animation = {}
		lua_getfield(L, index, "animation");
		if(lua_istable(L, -1)){
			// {type="vertical_frames", aspect_w=16, aspect_h=16, length=2.0}
			tiledef.animation.type = (TileAnimationType)
					getenumfield(L, -1, "type", es_TileAnimationType,
					TAT_NONE);
			tiledef.animation.aspect_w =
					getintfield_default(L, -1, "aspect_w", 16);
			tiledef.animation.aspect_h =
					getintfield_default(L, -1, "aspect_h", 16);
			tiledef.animation.length =
					getfloatfield_default(L, -1, "length", 1.0);
		}
		lua_pop(L, 1);
	}

	return tiledef;
}

/*
	ContentFeatures
*/

static ContentFeatures read_content_features(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	ContentFeatures f;
	
	/* Cache existence of some callbacks */
	lua_getfield(L, index, "on_construct");
	if(!lua_isnil(L, -1)) f.has_on_construct = true;
	lua_pop(L, 1);
	lua_getfield(L, index, "on_destruct");
	if(!lua_isnil(L, -1)) f.has_on_destruct = true;
	lua_pop(L, 1);
	lua_getfield(L, index, "after_destruct");
	if(!lua_isnil(L, -1)) f.has_after_destruct = true;
	lua_pop(L, 1);

	/* Name */
	getstringfield(L, index, "name", f.name);

	/* Groups */
	lua_getfield(L, index, "groups");
	read_groups(L, -1, f.groups);
	lua_pop(L, 1);

	/* Visual definition */

	f.drawtype = (NodeDrawType)getenumfield(L, index, "drawtype", es_DrawType,
			NDT_NORMAL);
	getfloatfield(L, index, "visual_scale", f.visual_scale);
	
	// tiles = {}
	lua_getfield(L, index, "tiles");
	// If nil, try the deprecated name "tile_images" instead
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		warn_if_field_exists(L, index, "tile_images",
				"Deprecated; new name is \"tiles\".");
		lua_getfield(L, index, "tile_images");
	}
	if(lua_istable(L, -1)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while(lua_next(L, table) != 0){
			// Read tiledef from value
			f.tiledef[i] = read_tiledef(L, -1);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			i++;
			if(i==6){
				lua_pop(L, 1);
				break;
			}
		}
		// Copy last value to all remaining textures
		if(i >= 1){
			TileDef lasttile = f.tiledef[i-1];
			while(i < 6){
				f.tiledef[i] = lasttile;
				i++;
			}
		}
	}
	lua_pop(L, 1);
	
	// special_tiles = {}
	lua_getfield(L, index, "special_tiles");
	// If nil, try the deprecated name "special_materials" instead
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		warn_if_field_exists(L, index, "special_materials",
				"Deprecated; new name is \"special_tiles\".");
		lua_getfield(L, index, "special_materials");
	}
	if(lua_istable(L, -1)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while(lua_next(L, table) != 0){
			// Read tiledef from value
			f.tiledef_special[i] = read_tiledef(L, -1);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			i++;
			if(i==6){
				lua_pop(L, 1);
				break;
			}
		}
	}
	lua_pop(L, 1);

	f.alpha = getintfield_default(L, index, "alpha", 255);

	/* Other stuff */
	
	lua_getfield(L, index, "post_effect_color");
	if(!lua_isnil(L, -1))
		f.post_effect_color = readARGB8(L, -1);
	lua_pop(L, 1);

	f.param_type = (ContentParamType)getenumfield(L, index, "paramtype",
			es_ContentParamType, CPT_NONE);
	f.param_type_2 = (ContentParamType2)getenumfield(L, index, "paramtype2",
			es_ContentParamType2, CPT2_NONE);

	// Warn about some deprecated fields
	warn_if_field_exists(L, index, "wall_mounted",
			"deprecated: use paramtype2 = 'wallmounted'");
	warn_if_field_exists(L, index, "light_propagates",
			"deprecated: determined from paramtype");
	warn_if_field_exists(L, index, "dug_item",
			"deprecated: use 'drop' field");
	warn_if_field_exists(L, index, "extra_dug_item",
			"deprecated: use 'drop' field");
	warn_if_field_exists(L, index, "extra_dug_item_rarity",
			"deprecated: use 'drop' field");
	warn_if_field_exists(L, index, "metadata_name",
			"deprecated: use on_add and metadata callbacks");
	
	// True for all ground-like things like stone and mud, false for eg. trees
	getboolfield(L, index, "is_ground_content", f.is_ground_content);
	f.light_propagates = (f.param_type == CPT_LIGHT);
	getboolfield(L, index, "sunlight_propagates", f.sunlight_propagates);
	// This is used for collision detection.
	// Also for general solidness queries.
	getboolfield(L, index, "walkable", f.walkable);
	// Player can point to these
	getboolfield(L, index, "pointable", f.pointable);
	// Player can dig these
	getboolfield(L, index, "diggable", f.diggable);
	// Player can climb these
	getboolfield(L, index, "climbable", f.climbable);
	// Player can build on these
	getboolfield(L, index, "buildable_to", f.buildable_to);
	// Whether the node is non-liquid, source liquid or flowing liquid
	f.liquid_type = (LiquidType)getenumfield(L, index, "liquidtype",
			es_LiquidType, LIQUID_NONE);
	// If the content is liquid, this is the flowing version of the liquid.
	getstringfield(L, index, "liquid_alternative_flowing",
			f.liquid_alternative_flowing);
	// If the content is liquid, this is the source version of the liquid.
	getstringfield(L, index, "liquid_alternative_source",
			f.liquid_alternative_source);
	// Viscosity for fluid flow, ranging from 1 to 7, with
	// 1 giving almost instantaneous propagation and 7 being
	// the slowest possible
	f.liquid_viscosity = getintfield_default(L, index,
			"liquid_viscosity", f.liquid_viscosity);
	getboolfield(L, index, "liquid_renewable", f.liquid_renewable);
	// Amount of light the node emits
	f.light_source = getintfield_default(L, index,
			"light_source", f.light_source);
	f.damage_per_second = getintfield_default(L, index,
			"damage_per_second", f.damage_per_second);
	
	lua_getfield(L, index, "node_box");
	if(lua_istable(L, -1))
		f.node_box = read_nodebox(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, index, "selection_box");
	if(lua_istable(L, -1))
		f.selection_box = read_nodebox(L, -1);
 	lua_pop(L, 1);

	// Set to true if paramtype used to be 'facedir_simple'
	getboolfield(L, index, "legacy_facedir_simple", f.legacy_facedir_simple);
	// Set to true if wall_mounted used to be set to true
	getboolfield(L, index, "legacy_wallmounted", f.legacy_wallmounted);
	
	// Sound table
	lua_getfield(L, index, "sounds");
	if(lua_istable(L, -1)){
		lua_getfield(L, -1, "footstep");
		read_soundspec(L, -1, f.sound_footstep);
		lua_pop(L, 1);
		lua_getfield(L, -1, "dig");
		read_soundspec(L, -1, f.sound_dig);
		lua_pop(L, 1);
		lua_getfield(L, -1, "dug");
		read_soundspec(L, -1, f.sound_dug);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	return f;
}

/*
	Inventory stuff
*/

static ItemStack read_item(lua_State *L, int index);
static std::vector<ItemStack> read_items(lua_State *L, int index);
// creates a table of ItemStacks
static void push_items(lua_State *L, const std::vector<ItemStack> &items);

static void inventory_set_list_from_lua(Inventory *inv, const char *name,
		lua_State *L, int tableindex, int forcesize=-1)
{
	if(tableindex < 0)
		tableindex = lua_gettop(L) + 1 + tableindex;
	// If nil, delete list
	if(lua_isnil(L, tableindex)){
		inv->deleteList(name);
		return;
	}
	// Otherwise set list
	std::vector<ItemStack> items = read_items(L, tableindex);
	int listsize = (forcesize != -1) ? forcesize : items.size();
	InventoryList *invlist = inv->addList(name, listsize);
	int index = 0;
	for(std::vector<ItemStack>::const_iterator
			i = items.begin(); i != items.end(); i++){
		if(forcesize != -1 && index == forcesize)
			break;
		invlist->changeItem(index, *i);
		index++;
	}
	while(forcesize != -1 && index < forcesize){
		invlist->deleteItem(index);
		index++;
	}
}

static void inventory_get_list_to_lua(Inventory *inv, const char *name,
		lua_State *L)
{
	InventoryList *invlist = inv->getList(name);
	if(invlist == NULL){
		lua_pushnil(L);
		return;
	}
	std::vector<ItemStack> items;
	for(u32 i=0; i<invlist->getSize(); i++)
		items.push_back(invlist->getItem(i));
	push_items(L, items);
}

/*
	Helpful macros for userdata classes
*/

#define method(class, name) {#name, class::l_##name}

/*
	LuaItemStack
*/

class LuaItemStack
{
private:
	ItemStack m_stack;

	static const char className[];
	static const luaL_reg methods[];

	// Exported functions
	
	// garbage collector
	static int gc_object(lua_State *L)
	{
		LuaItemStack *o = *(LuaItemStack **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	// is_empty(self) -> true/false
	static int l_is_empty(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushboolean(L, item.empty());
		return 1;
	}

	// get_name(self) -> string
	static int l_get_name(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushstring(L, item.name.c_str());
		return 1;
	}

	// get_count(self) -> number
	static int l_get_count(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushinteger(L, item.count);
		return 1;
	}

	// get_wear(self) -> number
	static int l_get_wear(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushinteger(L, item.wear);
		return 1;
	}

	// get_metadata(self) -> string
	static int l_get_metadata(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushlstring(L, item.metadata.c_str(), item.metadata.size());
		return 1;
	}

	// clear(self) -> true
	static int l_clear(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		o->m_stack.clear();
		lua_pushboolean(L, true);
		return 1;
	}

	// replace(self, itemstack or itemstring or table or nil) -> true
	static int l_replace(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		o->m_stack = read_item(L, 2);
		lua_pushboolean(L, true);
		return 1;
	}

	// to_string(self) -> string
	static int l_to_string(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		std::string itemstring = o->m_stack.getItemString();
		lua_pushstring(L, itemstring.c_str());
		return 1;
	}

	// to_table(self) -> table or nil
	static int l_to_table(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		const ItemStack &item = o->m_stack;
		if(item.empty())
		{
			lua_pushnil(L);
		}
		else
		{
			lua_newtable(L);
			lua_pushstring(L, item.name.c_str());
			lua_setfield(L, -2, "name");
			lua_pushinteger(L, item.count);
			lua_setfield(L, -2, "count");
			lua_pushinteger(L, item.wear);
			lua_setfield(L, -2, "wear");
			lua_pushlstring(L, item.metadata.c_str(), item.metadata.size());
			lua_setfield(L, -2, "metadata");
		}
		return 1;
	}

	// get_stack_max(self) -> number
	static int l_get_stack_max(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushinteger(L, item.getStackMax(get_server(L)->idef()));
		return 1;
	}

	// get_free_space(self) -> number
	static int l_get_free_space(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		lua_pushinteger(L, item.freeSpace(get_server(L)->idef()));
		return 1;
	}

	// is_known(self) -> true/false
	// Checks if the item is defined.
	static int l_is_known(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		bool is_known = item.isKnown(get_server(L)->idef());
		lua_pushboolean(L, is_known);
		return 1;
	}

	// get_definition(self) -> table
	// Returns the item definition table from minetest.registered_items,
	// or a fallback one (name="unknown")
	static int l_get_definition(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;

		// Get minetest.registered_items[name]
		lua_getglobal(L, "minetest");
		lua_getfield(L, -1, "registered_items");
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, item.name.c_str());
		if(lua_isnil(L, -1))
		{
			lua_pop(L, 1);
			lua_getfield(L, -1, "unknown");
		}
		return 1;
	}

	// get_tool_capabilities(self) -> table
	// Returns the effective tool digging properties.
	// Returns those of the hand ("") if this item has none associated.
	static int l_get_tool_capabilities(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		const ToolCapabilities &prop =
			item.getToolCapabilities(get_server(L)->idef());
		push_tool_capabilities(L, prop);
		return 1;
	}

	// add_wear(self, amount) -> true/false
	// The range for "amount" is [0,65535]. Wear is only added if the item
	// is a tool. Adding wear might destroy the item.
	// Returns true if the item is (or was) a tool.
	static int l_add_wear(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		int amount = lua_tointeger(L, 2);
		bool result = item.addWear(amount, get_server(L)->idef());
		lua_pushboolean(L, result);
		return 1;
	}

	// add_item(self, itemstack or itemstring or table or nil) -> itemstack
	// Returns leftover item stack
	static int l_add_item(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		ItemStack newitem = read_item(L, 2);
		ItemStack leftover = item.addItem(newitem, get_server(L)->idef());
		create(L, leftover);
		return 1;
	}

	// item_fits(self, itemstack or itemstring or table or nil) -> true/false, itemstack
	// First return value is true iff the new item fits fully into the stack
	// Second return value is the would-be-left-over item stack
	static int l_item_fits(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		ItemStack newitem = read_item(L, 2);
		ItemStack restitem;
		bool fits = item.itemFits(newitem, &restitem, get_server(L)->idef());
		lua_pushboolean(L, fits);  // first return value
		create(L, restitem);       // second return value
		return 2;
	}

	// take_item(self, takecount=1) -> itemstack
	static int l_take_item(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		u32 takecount = 1;
		if(!lua_isnone(L, 2))
			takecount = luaL_checkinteger(L, 2);
		ItemStack taken = item.takeItem(takecount);
		create(L, taken);
		return 1;
	}

	// peek_item(self, peekcount=1) -> itemstack
	static int l_peek_item(lua_State *L)
	{
		LuaItemStack *o = checkobject(L, 1);
		ItemStack &item = o->m_stack;
		u32 peekcount = 1;
		if(!lua_isnone(L, 2))
			peekcount = lua_tointeger(L, 2);
		ItemStack peekaboo = item.peekItem(peekcount);
		create(L, peekaboo);
		return 1;
	}

public:
	LuaItemStack(const ItemStack &item):
		m_stack(item)
	{
	}

	~LuaItemStack()
	{
	}

	const ItemStack& getItem() const
	{
		return m_stack;
	}
	ItemStack& getItem()
	{
		return m_stack;
	}
	
	// LuaItemStack(itemstack or itemstring or table or nil)
	// Creates an LuaItemStack and leaves it on top of stack
	static int create_object(lua_State *L)
	{
		ItemStack item = read_item(L, 1);
		LuaItemStack *o = new LuaItemStack(item);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
		return 1;
	}
	// Not callable from Lua
	static int create(lua_State *L, const ItemStack &item)
	{
		LuaItemStack *o = new LuaItemStack(item);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
		return 1;
	}

	static LuaItemStack* checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(LuaItemStack**)ud;  // unbox pointer
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

		// Can be created from Lua (LuaItemStack(itemstack or itemstring or table or nil))
		lua_register(L, className, create_object);
	}
};
const char LuaItemStack::className[] = "ItemStack";
const luaL_reg LuaItemStack::methods[] = {
	method(LuaItemStack, is_empty),
	method(LuaItemStack, get_name),
	method(LuaItemStack, get_count),
	method(LuaItemStack, get_wear),
	method(LuaItemStack, get_metadata),
	method(LuaItemStack, clear),
	method(LuaItemStack, replace),
	method(LuaItemStack, to_string),
	method(LuaItemStack, to_table),
	method(LuaItemStack, get_stack_max),
	method(LuaItemStack, get_free_space),
	method(LuaItemStack, is_known),
	method(LuaItemStack, get_definition),
	method(LuaItemStack, get_tool_capabilities),
	method(LuaItemStack, add_wear),
	method(LuaItemStack, add_item),
	method(LuaItemStack, item_fits),
	method(LuaItemStack, take_item),
	method(LuaItemStack, peek_item),
	{0,0}
};

static ItemStack read_item(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(lua_isnil(L, index))
	{
		return ItemStack();
	}
	else if(lua_isuserdata(L, index))
	{
		// Convert from LuaItemStack
		LuaItemStack *o = LuaItemStack::checkobject(L, index);
		return o->getItem();
	}
	else if(lua_isstring(L, index))
	{
		// Convert from itemstring
		std::string itemstring = lua_tostring(L, index);
		IItemDefManager *idef = get_server(L)->idef();
		try
		{
			ItemStack item;
			item.deSerialize(itemstring, idef);
			return item;
		}
		catch(SerializationError &e)
		{
			infostream<<"WARNING: unable to create item from itemstring"
					<<": "<<itemstring<<std::endl;
			return ItemStack();
		}
	}
	else if(lua_istable(L, index))
	{
		// Convert from table
		IItemDefManager *idef = get_server(L)->idef();
		std::string name = getstringfield_default(L, index, "name", "");
		int count = getintfield_default(L, index, "count", 1);
		int wear = getintfield_default(L, index, "wear", 0);
		std::string metadata = getstringfield_default(L, index, "metadata", "");
		return ItemStack(name, count, wear, metadata, idef);
	}
	else
	{
		throw LuaError(L, "Expecting itemstack, itemstring, table or nil");
	}
}

static std::vector<ItemStack> read_items(lua_State *L, int index)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	std::vector<ItemStack> items;
	luaL_checktype(L, index, LUA_TTABLE);
	lua_pushnil(L);
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		items.push_back(read_item(L, -1));
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	return items;
}

// creates a table of ItemStacks
static void push_items(lua_State *L, const std::vector<ItemStack> &items)
{
	// Get the table insert function
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	// Create and fill table
	lua_newtable(L);
	int table = lua_gettop(L);
	for(u32 i=0; i<items.size(); i++){
		ItemStack item = items[i];
		lua_pushvalue(L, table_insert);
		lua_pushvalue(L, table);
		LuaItemStack::create(L, item);
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
	lua_remove(L, -2); // Remove table
	lua_remove(L, -2); // Remove insert
}

/*
	InvRef
*/

class InvRef
{
private:
	InventoryLocation m_loc;

	static const char className[];
	static const luaL_reg methods[];

	static InvRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(InvRef**)ud;  // unbox pointer
	}
	
	static Inventory* getinv(lua_State *L, InvRef *ref)
	{
		return get_server(L)->getInventory(ref->m_loc);
	}

	static InventoryList* getlist(lua_State *L, InvRef *ref,
			const char *listname)
	{
		Inventory *inv = getinv(L, ref);
		if(!inv)
			return NULL;
		return inv->getList(listname);
	}

	static void reportInventoryChange(lua_State *L, InvRef *ref)
	{
		// Inform other things that the inventory has changed
		get_server(L)->setInventoryModified(ref->m_loc);
	}
	
	// Exported functions
	
	// garbage collector
	static int gc_object(lua_State *L) {
		InvRef *o = *(InvRef **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	// is_empty(self, listname) -> true/false
	static int l_is_empty(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		InventoryList *list = getlist(L, ref, listname);
		if(list && list->getUsedSlots() > 0){
			lua_pushboolean(L, false);
		} else {
			lua_pushboolean(L, true);
		}
		return 1;
	}

	// get_size(self, listname)
	static int l_get_size(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		InventoryList *list = getlist(L, ref, listname);
		if(list){
			lua_pushinteger(L, list->getSize());
		} else {
			lua_pushinteger(L, 0);
		}
		return 1;
	}

	// get_width(self, listname)
	static int l_get_width(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		InventoryList *list = getlist(L, ref, listname);
		if(list){
			lua_pushinteger(L, list->getWidth());
		} else {
			lua_pushinteger(L, 0);
		}
		return 1;
	}

	// set_size(self, listname, size)
	static int l_set_size(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		int newsize = luaL_checknumber(L, 3);
		Inventory *inv = getinv(L, ref);
		if(newsize == 0){
			inv->deleteList(listname);
			reportInventoryChange(L, ref);
			return 0;
		}
		InventoryList *list = inv->getList(listname);
		if(list){
			list->setSize(newsize);
		} else {
			list = inv->addList(listname, newsize);
		}
		reportInventoryChange(L, ref);
		return 0;
	}

	// set_width(self, listname, size)
	static int l_set_width(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		int newwidth = luaL_checknumber(L, 3);
		Inventory *inv = getinv(L, ref);
		InventoryList *list = inv->getList(listname);
		if(list){
			list->setWidth(newwidth);
		} else {
			return 0;
		}
		reportInventoryChange(L, ref);
		return 0;
	}

	// get_stack(self, listname, i) -> itemstack
	static int l_get_stack(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		int i = luaL_checknumber(L, 3) - 1;
		InventoryList *list = getlist(L, ref, listname);
		ItemStack item;
		if(list != NULL && i >= 0 && i < (int) list->getSize())
			item = list->getItem(i);
		LuaItemStack::create(L, item);
		return 1;
	}

	// set_stack(self, listname, i, stack) -> true/false
	static int l_set_stack(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		int i = luaL_checknumber(L, 3) - 1;
		ItemStack newitem = read_item(L, 4);
		InventoryList *list = getlist(L, ref, listname);
		if(list != NULL && i >= 0 && i < (int) list->getSize()){
			list->changeItem(i, newitem);
			reportInventoryChange(L, ref);
			lua_pushboolean(L, true);
		} else {
			lua_pushboolean(L, false);
		}
		return 1;
	}

	// get_list(self, listname) -> list or nil
	static int l_get_list(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		Inventory *inv = getinv(L, ref);
		inventory_get_list_to_lua(inv, listname, L);
		return 1;
	}

	// set_list(self, listname, list)
	static int l_set_list(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		Inventory *inv = getinv(L, ref);
		InventoryList *list = inv->getList(listname);
		if(list)
			inventory_set_list_from_lua(inv, listname, L, 3,
					list->getSize());
		else
			inventory_set_list_from_lua(inv, listname, L, 3);
		reportInventoryChange(L, ref);
		return 0;
	}

	// add_item(self, listname, itemstack or itemstring or table or nil) -> itemstack
	// Returns the leftover stack
	static int l_add_item(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		ItemStack item = read_item(L, 3);
		InventoryList *list = getlist(L, ref, listname);
		if(list){
			ItemStack leftover = list->addItem(item);
			if(leftover.count != item.count)
				reportInventoryChange(L, ref);
			LuaItemStack::create(L, leftover);
		} else {
			LuaItemStack::create(L, item);
		}
		return 1;
	}

	// room_for_item(self, listname, itemstack or itemstring or table or nil) -> true/false
	// Returns true if the item completely fits into the list
	static int l_room_for_item(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		ItemStack item = read_item(L, 3);
		InventoryList *list = getlist(L, ref, listname);
		if(list){
			lua_pushboolean(L, list->roomForItem(item));
		} else {
			lua_pushboolean(L, false);
		}
		return 1;
	}

	// contains_item(self, listname, itemstack or itemstring or table or nil) -> true/false
	// Returns true if the list contains the given count of the given item name
	static int l_contains_item(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		ItemStack item = read_item(L, 3);
		InventoryList *list = getlist(L, ref, listname);
		if(list){
			lua_pushboolean(L, list->containsItem(item));
		} else {
			lua_pushboolean(L, false);
		}
		return 1;
	}

	// remove_item(self, listname, itemstack or itemstring or table or nil) -> itemstack
	// Returns the items that were actually removed
	static int l_remove_item(lua_State *L)
	{
		InvRef *ref = checkobject(L, 1);
		const char *listname = luaL_checkstring(L, 2);
		ItemStack item = read_item(L, 3);
		InventoryList *list = getlist(L, ref, listname);
		if(list){
			ItemStack removed = list->removeItem(item);
			if(!removed.empty())
				reportInventoryChange(L, ref);
			LuaItemStack::create(L, removed);
		} else {
			LuaItemStack::create(L, ItemStack());
		}
		return 1;
	}

public:
	InvRef(const InventoryLocation &loc):
		m_loc(loc)
	{
	}

	~InvRef()
	{
	}

	// Creates an InvRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, const InventoryLocation &loc)
	{
		InvRef *o = new InvRef(loc);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
	}
	static void createPlayer(lua_State *L, Player *player)
	{
		InventoryLocation loc;
		loc.setPlayer(player->getName());
		create(L, loc);
	}
	static void createNodeMeta(lua_State *L, v3s16 p)
	{
		InventoryLocation loc;
		loc.setNodeMeta(p);
		create(L, loc);
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
const char InvRef::className[] = "InvRef";
const luaL_reg InvRef::methods[] = {
	method(InvRef, is_empty),
	method(InvRef, get_size),
	method(InvRef, set_size),
	method(InvRef, get_width),
	method(InvRef, set_width),
	method(InvRef, get_stack),
	method(InvRef, set_stack),
	method(InvRef, get_list),
	method(InvRef, set_list),
	method(InvRef, add_item),
	method(InvRef, room_for_item),
	method(InvRef, contains_item),
	method(InvRef, remove_item),
	{0,0}
};

/*
	NodeMetaRef
*/

class NodeMetaRef
{
private:
	v3s16 m_p;
	ServerEnvironment *m_env;

	static const char className[];
	static const luaL_reg methods[];

	static NodeMetaRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(NodeMetaRef**)ud;  // unbox pointer
	}
	
	static NodeMetadata* getmeta(NodeMetaRef *ref, bool auto_create)
	{
		NodeMetadata *meta = ref->m_env->getMap().getNodeMetadata(ref->m_p);
		if(meta == NULL && auto_create)
		{
			meta = new NodeMetadata(ref->m_env->getGameDef());
			ref->m_env->getMap().setNodeMetadata(ref->m_p, meta);
		}
		return meta;
	}

	static void reportMetadataChange(NodeMetaRef *ref)
	{
		// NOTE: This same code is in rollback_interface.cpp
		// Inform other things that the metadata has changed
		v3s16 blockpos = getNodeBlockPos(ref->m_p);
		MapEditEvent event;
		event.type = MEET_BLOCK_NODE_METADATA_CHANGED;
		event.p = blockpos;
		ref->m_env->getMap().dispatchEvent(&event);
		// Set the block to be saved
		MapBlock *block = ref->m_env->getMap().getBlockNoCreateNoEx(blockpos);
		if(block)
			block->raiseModified(MOD_STATE_WRITE_NEEDED,
					"NodeMetaRef::reportMetadataChange");
	}
	
	// Exported functions
	
	// garbage collector
	static int gc_object(lua_State *L) {
		NodeMetaRef *o = *(NodeMetaRef **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	// get_string(self, name)
	static int l_get_string(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		std::string name = luaL_checkstring(L, 2);

		NodeMetadata *meta = getmeta(ref, false);
		if(meta == NULL){
			lua_pushlstring(L, "", 0);
			return 1;
		}
		std::string str = meta->getString(name);
		lua_pushlstring(L, str.c_str(), str.size());
		return 1;
	}

	// set_string(self, name, var)
	static int l_set_string(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		std::string name = luaL_checkstring(L, 2);
		size_t len = 0;
		const char *s = lua_tolstring(L, 3, &len);
		std::string str(s, len);

		NodeMetadata *meta = getmeta(ref, !str.empty());
		if(meta == NULL || str == meta->getString(name))
			return 0;
		meta->setString(name, str);
		reportMetadataChange(ref);
		return 0;
	}

	// get_int(self, name)
	static int l_get_int(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		std::string name = lua_tostring(L, 2);

		NodeMetadata *meta = getmeta(ref, false);
		if(meta == NULL){
			lua_pushnumber(L, 0);
			return 1;
		}
		std::string str = meta->getString(name);
		lua_pushnumber(L, stoi(str));
		return 1;
	}

	// set_int(self, name, var)
	static int l_set_int(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		std::string name = lua_tostring(L, 2);
		int a = lua_tointeger(L, 3);
		std::string str = itos(a);

		NodeMetadata *meta = getmeta(ref, true);
		if(meta == NULL || str == meta->getString(name))
			return 0;
		meta->setString(name, str);
		reportMetadataChange(ref);
		return 0;
	}

	// get_float(self, name)
	static int l_get_float(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		std::string name = lua_tostring(L, 2);

		NodeMetadata *meta = getmeta(ref, false);
		if(meta == NULL){
			lua_pushnumber(L, 0);
			return 1;
		}
		std::string str = meta->getString(name);
		lua_pushnumber(L, stof(str));
		return 1;
	}

	// set_float(self, name, var)
	static int l_set_float(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		std::string name = lua_tostring(L, 2);
		float a = lua_tonumber(L, 3);
		std::string str = ftos(a);

		NodeMetadata *meta = getmeta(ref, true);
		if(meta == NULL || str == meta->getString(name))
			return 0;
		meta->setString(name, str);
		reportMetadataChange(ref);
		return 0;
	}

	// get_inventory(self)
	static int l_get_inventory(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		getmeta(ref, true);  // try to ensure the metadata exists
		InvRef::createNodeMeta(L, ref->m_p);
		return 1;
	}
	
	// to_table(self)
	static int l_to_table(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);

		NodeMetadata *meta = getmeta(ref, true);
		if(meta == NULL){
			lua_pushnil(L);
			return 1;
		}
		lua_newtable(L);
		// fields
		lua_newtable(L);
		{
			std::map<std::string, std::string> fields = meta->getStrings();
			for(std::map<std::string, std::string>::const_iterator
					i = fields.begin(); i != fields.end(); i++){
				const std::string &name = i->first;
				const std::string &value = i->second;
				lua_pushlstring(L, name.c_str(), name.size());
				lua_pushlstring(L, value.c_str(), value.size());
				lua_settable(L, -3);
			}
		}
		lua_setfield(L, -2, "fields");
		// inventory
		lua_newtable(L);
		Inventory *inv = meta->getInventory();
		if(inv){
			std::vector<const InventoryList*> lists = inv->getLists();
			for(std::vector<const InventoryList*>::const_iterator
					i = lists.begin(); i != lists.end(); i++){
				inventory_get_list_to_lua(inv, (*i)->getName().c_str(), L);
				lua_setfield(L, -2, (*i)->getName().c_str());
			}
		}
		lua_setfield(L, -2, "inventory");
		return 1;
	}

	// from_table(self, table)
	static int l_from_table(lua_State *L)
	{
		NodeMetaRef *ref = checkobject(L, 1);
		int base = 2;
		
		if(lua_isnil(L, base)){
			// No metadata
			ref->m_env->getMap().removeNodeMetadata(ref->m_p);
			lua_pushboolean(L, true);
			return 1;
		}

		// Has metadata; clear old one first
		ref->m_env->getMap().removeNodeMetadata(ref->m_p);
		// Create new metadata
		NodeMetadata *meta = getmeta(ref, true);
		// Set fields
		lua_getfield(L, base, "fields");
		int fieldstable = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, fieldstable) != 0){
			// key at index -2 and value at index -1
			std::string name = lua_tostring(L, -2);
			size_t cl;
			const char *cs = lua_tolstring(L, -1, &cl);
			std::string value(cs, cl);
			meta->setString(name, value);
			lua_pop(L, 1); // removes value, keeps key for next iteration
		}
		// Set inventory
		Inventory *inv = meta->getInventory();
		lua_getfield(L, base, "inventory");
		int inventorytable = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, inventorytable) != 0){
			// key at index -2 and value at index -1
			std::string name = lua_tostring(L, -2);
			inventory_set_list_from_lua(inv, name.c_str(), L, -1);
			lua_pop(L, 1); // removes value, keeps key for next iteration
		}
		reportMetadataChange(ref);
		lua_pushboolean(L, true);
		return 1;
	}

public:
	NodeMetaRef(v3s16 p, ServerEnvironment *env):
		m_p(p),
		m_env(env)
	{
	}

	~NodeMetaRef()
	{
	}

	// Creates an NodeMetaRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, v3s16 p, ServerEnvironment *env)
	{
		NodeMetaRef *o = new NodeMetaRef(p, env);
		//infostream<<"NodeMetaRef::create: o="<<o<<std::endl;
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
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
const char NodeMetaRef::className[] = "NodeMetaRef";
const luaL_reg NodeMetaRef::methods[] = {
	method(NodeMetaRef, get_string),
	method(NodeMetaRef, set_string),
	method(NodeMetaRef, get_int),
	method(NodeMetaRef, set_int),
	method(NodeMetaRef, get_float),
	method(NodeMetaRef, set_float),
	method(NodeMetaRef, get_inventory),
	method(NodeMetaRef, to_table),
	method(NodeMetaRef, from_table),
	{0,0}
};

/*
	ObjectRef
*/

class ObjectRef
{
private:
	ServerActiveObject *m_object;

	static const char className[];
	static const luaL_reg methods[];
public:
	static ObjectRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(ObjectRef**)ud;  // unbox pointer
	}
	
	static ServerActiveObject* getobject(ObjectRef *ref)
	{
		ServerActiveObject *co = ref->m_object;
		return co;
	}
private:
	static LuaEntitySAO* getluaobject(ObjectRef *ref)
	{
		ServerActiveObject *obj = getobject(ref);
		if(obj == NULL)
			return NULL;
		if(obj->getType() != ACTIVEOBJECT_TYPE_LUAENTITY)
			return NULL;
		return (LuaEntitySAO*)obj;
	}
	
	static PlayerSAO* getplayersao(ObjectRef *ref)
	{
		ServerActiveObject *obj = getobject(ref);
		if(obj == NULL)
			return NULL;
		if(obj->getType() != ACTIVEOBJECT_TYPE_PLAYER)
			return NULL;
		return (PlayerSAO*)obj;
	}
	
	static Player* getplayer(ObjectRef *ref)
	{
		PlayerSAO *playersao = getplayersao(ref);
		if(playersao == NULL)
			return NULL;
		return playersao->getPlayer();
	}
	
	// Exported functions
	
	// garbage collector
	static int gc_object(lua_State *L) {
		ObjectRef *o = *(ObjectRef **)(lua_touserdata(L, 1));
		//infostream<<"ObjectRef::gc_object: o="<<o<<std::endl;
		delete o;
		return 0;
	}

	// remove(self)
	static int l_remove(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		verbosestream<<"ObjectRef::l_remove(): id="<<co->getId()<<std::endl;
		co->m_removed = true;
		return 0;
	}
	
	// getpos(self)
	// returns: {x=num, y=num, z=num}
	static int l_getpos(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		v3f pos = co->getBasePosition() / BS;
		lua_newtable(L);
		lua_pushnumber(L, pos.X);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, pos.Y);
		lua_setfield(L, -2, "y");
		lua_pushnumber(L, pos.Z);
		lua_setfield(L, -2, "z");
		return 1;
	}
	
	// setpos(self, pos)
	static int l_setpos(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		//LuaEntitySAO *co = getluaobject(ref);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// pos
		v3f pos = checkFloatPos(L, 2);
		// Do it
		co->setPos(pos);
		return 0;
	}
	
	// moveto(self, pos, continuous=false)
	static int l_moveto(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		//LuaEntitySAO *co = getluaobject(ref);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// pos
		v3f pos = checkFloatPos(L, 2);
		// continuous
		bool continuous = lua_toboolean(L, 3);
		// Do it
		co->moveTo(pos, continuous);
		return 0;
	}

	// punch(self, puncher, time_from_last_punch, tool_capabilities, dir)
	static int l_punch(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ObjectRef *puncher_ref = checkobject(L, 2);
		ServerActiveObject *co = getobject(ref);
		ServerActiveObject *puncher = getobject(puncher_ref);
		if(co == NULL) return 0;
		if(puncher == NULL) return 0;
		v3f dir;
		if(lua_type(L, 5) != LUA_TTABLE)
			dir = co->getBasePosition() - puncher->getBasePosition();
		else
			dir = read_v3f(L, 5);
		float time_from_last_punch = 1000000;
		if(lua_isnumber(L, 3))
			time_from_last_punch = lua_tonumber(L, 3);
		ToolCapabilities toolcap = read_tool_capabilities(L, 4);
		dir.normalize();
		// Do it
		co->punch(dir, &toolcap, puncher, time_from_last_punch);
		return 0;
	}

	// right_click(self, clicker); clicker = an another ObjectRef
	static int l_right_click(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ObjectRef *ref2 = checkobject(L, 2);
		ServerActiveObject *co = getobject(ref);
		ServerActiveObject *co2 = getobject(ref2);
		if(co == NULL) return 0;
		if(co2 == NULL) return 0;
		// Do it
		co->rightClick(co2);
		return 0;
	}

	// set_hp(self, hp)
	// hp = number of hitpoints (2 * number of hearts)
	// returns: nil
	static int l_set_hp(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		luaL_checknumber(L, 2);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		int hp = lua_tonumber(L, 2);
		/*infostream<<"ObjectRef::l_set_hp(): id="<<co->getId()
				<<" hp="<<hp<<std::endl;*/
		// Do it
		co->setHP(hp);
		// Return
		return 0;
	}

	// get_hp(self)
	// returns: number of hitpoints (2 * number of hearts)
	// 0 if not applicable to this type of object
	static int l_get_hp(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL){
			// Default hp is 1
			lua_pushnumber(L, 1);
			return 1;
		}
		int hp = co->getHP();
		/*infostream<<"ObjectRef::l_get_hp(): id="<<co->getId()
				<<" hp="<<hp<<std::endl;*/
		// Return
		lua_pushnumber(L, hp);
		return 1;
	}

	// get_inventory(self)
	static int l_get_inventory(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		InventoryLocation loc = co->getInventoryLocation();
		if(get_server(L)->getInventory(loc) != NULL)
			InvRef::create(L, loc);
		else
			lua_pushnil(L); // An object may have no inventory (nil)
		return 1;
	}

	// get_wield_list(self)
	static int l_get_wield_list(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		lua_pushstring(L, co->getWieldList().c_str());
		return 1;
	}

	// get_wield_index(self)
	static int l_get_wield_index(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		lua_pushinteger(L, co->getWieldIndex() + 1);
		return 1;
	}

	// get_wielded_item(self)
	static int l_get_wielded_item(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL){
			// Empty ItemStack
			LuaItemStack::create(L, ItemStack());
			return 1;
		}
		// Do it
		LuaItemStack::create(L, co->getWieldedItem());
		return 1;
	}

	// set_wielded_item(self, itemstack or itemstring or table or nil)
	static int l_set_wielded_item(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		ItemStack item = read_item(L, 2);
		bool success = co->setWieldedItem(item);
		lua_pushboolean(L, success);
		return 1;
	}

	// set_armor_groups(self, groups)
	static int l_set_armor_groups(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		ItemGroupList groups;
		read_groups(L, 2, groups);
		co->setArmorGroups(groups);
		return 0;
	}

	// set_animation(self, frame_range, frame_speed, frame_blend)
	static int l_set_animation(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		v2f frames = v2f(1, 1);
		if(!lua_isnil(L, 2))
			frames = read_v2f(L, 2);
		float frame_speed = 15;
		if(!lua_isnil(L, 3))
			frame_speed = lua_tonumber(L, 3);
		float frame_blend = 0;
		if(!lua_isnil(L, 4))
			frame_blend = lua_tonumber(L, 4);
		co->setAnimation(frames, frame_speed, frame_blend);
		return 0;
	}

	// set_bone_position(self, std::string bone, v3f position, v3f rotation)
	static int l_set_bone_position(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		std::string bone = "";
		if(!lua_isnil(L, 2))
			bone = lua_tostring(L, 2);
		v3f position = v3f(0, 0, 0);
		if(!lua_isnil(L, 3))
			position = read_v3f(L, 3);
		v3f rotation = v3f(0, 0, 0);
		if(!lua_isnil(L, 4))
			rotation = read_v3f(L, 4);
		co->setBonePosition(bone, position, rotation);
		return 0;
	}

	// set_attach(self, parent, bone, position, rotation)
	static int l_set_attach(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ObjectRef *parent_ref = checkobject(L, 2);
		ServerActiveObject *co = getobject(ref);
		ServerActiveObject *parent = getobject(parent_ref);
		if(co == NULL) return 0;
		if(parent == NULL) return 0;
		// Do it
		std::string bone = "";
		if(!lua_isnil(L, 3))
			bone = lua_tostring(L, 3);
		v3f position = v3f(0, 0, 0);
		if(!lua_isnil(L, 4))
			position = read_v3f(L, 4);
		v3f rotation = v3f(0, 0, 0);
		if(!lua_isnil(L, 5))
			rotation = read_v3f(L, 5);
		co->setAttachment(parent->getId(), bone, position, rotation);
		return 0;
	}

	// set_detach(self)
	static int l_set_detach(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// Do it
		co->setAttachment(0, "", v3f(0,0,0), v3f(0,0,0));
		return 0;
	}

	// set_properties(self, properties)
	static int l_set_properties(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		ObjectProperties *prop = co->accessObjectProperties();
		if(!prop)
			return 0;
		read_object_properties(L, 2, prop);
		co->notifyObjectPropertiesModified();
		return 0;
	}

	/* LuaEntitySAO-only */

	// setvelocity(self, {x=num, y=num, z=num})
	static int l_setvelocity(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		v3f pos = checkFloatPos(L, 2);
		// Do it
		co->setVelocity(pos);
		return 0;
	}
	
	// getvelocity(self)
	static int l_getvelocity(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		v3f v = co->getVelocity();
		pushFloatPos(L, v);
		return 1;
	}
	
	// setacceleration(self, {x=num, y=num, z=num})
	static int l_setacceleration(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// pos
		v3f pos = checkFloatPos(L, 2);
		// Do it
		co->setAcceleration(pos);
		return 0;
	}
	
	// getacceleration(self)
	static int l_getacceleration(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		v3f v = co->getAcceleration();
		pushFloatPos(L, v);
		return 1;
	}
	
	// setyaw(self, radians)
	static int l_setyaw(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		float yaw = luaL_checknumber(L, 2) * core::RADTODEG;
		// Do it
		co->setYaw(yaw);
		return 0;
	}
	
	// getyaw(self)
	static int l_getyaw(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		float yaw = co->getYaw() * core::DEGTORAD;
		lua_pushnumber(L, yaw);
		return 1;
	}
	
	// settexturemod(self, mod)
	static int l_settexturemod(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		std::string mod = luaL_checkstring(L, 2);
		co->setTextureMod(mod);
		return 0;
	}
	
	// setsprite(self, p={x=0,y=0}, num_frames=1, framelength=0.2,
	//           select_horiz_by_yawpitch=false)
	static int l_setsprite(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		v2s16 p(0,0);
		if(!lua_isnil(L, 2))
			p = read_v2s16(L, 2);
		int num_frames = 1;
		if(!lua_isnil(L, 3))
			num_frames = lua_tonumber(L, 3);
		float framelength = 0.2;
		if(!lua_isnil(L, 4))
			framelength = lua_tonumber(L, 4);
		bool select_horiz_by_yawpitch = false;
		if(!lua_isnil(L, 5))
			select_horiz_by_yawpitch = lua_toboolean(L, 5);
		co->setSprite(p, num_frames, framelength, select_horiz_by_yawpitch);
		return 0;
	}

	// DEPRECATED
	// get_entity_name(self)
	static int l_get_entity_name(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		std::string name = co->getName();
		lua_pushstring(L, name.c_str());
		return 1;
	}
	
	// get_luaentity(self)
	static int l_get_luaentity(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		luaentity_get(L, co->getId());
		return 1;
	}
	
	/* Player-only */

	// is_player(self)
	static int l_is_player(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		lua_pushboolean(L, (player != NULL));
		return 1;
	}
	
	// get_player_name(self)
	static int l_get_player_name(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL){
			lua_pushlstring(L, "", 0);
			return 1;
		}
		// Do it
		lua_pushstring(L, player->getName());
		return 1;
	}
	
	// get_look_dir(self)
	static int l_get_look_dir(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL) return 0;
		// Do it
		float pitch = player->getRadPitch();
		float yaw = player->getRadYaw();
		v3f v(cos(pitch)*cos(yaw), sin(pitch), cos(pitch)*sin(yaw));
		push_v3f(L, v);
		return 1;
	}

	// get_look_pitch(self)
	static int l_get_look_pitch(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL) return 0;
		// Do it
		lua_pushnumber(L, player->getRadPitch());
		return 1;
	}

	// get_look_yaw(self)
	static int l_get_look_yaw(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL) return 0;
		// Do it
		lua_pushnumber(L, player->getRadYaw());
		return 1;
	}

	// set_inventory_formspec(self, formspec)
	static int l_set_inventory_formspec(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL) return 0;
		std::string formspec = luaL_checkstring(L, 2);

		player->inventory_formspec = formspec;
		get_server(L)->reportInventoryFormspecModified(player->getName());
		lua_pushboolean(L, true);
		return 1;
	}

	// get_inventory_formspec(self) -> formspec
	static int l_get_inventory_formspec(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL) return 0;

		std::string formspec = player->inventory_formspec;
		lua_pushlstring(L, formspec.c_str(), formspec.size());
		return 1;
	}
	
	// get_player_control(self)
	static int l_get_player_control(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL){
			lua_pushlstring(L, "", 0);
			return 1;
		}
		// Do it
		PlayerControl control = player->getPlayerControl();
		lua_newtable(L);
		lua_pushboolean(L, control.up);
		lua_setfield(L, -2, "up");
		lua_pushboolean(L, control.down);
		lua_setfield(L, -2, "down");
		lua_pushboolean(L, control.left);
		lua_setfield(L, -2, "left");
		lua_pushboolean(L, control.right);
		lua_setfield(L, -2, "right");
		lua_pushboolean(L, control.jump);
		lua_setfield(L, -2, "jump");
		lua_pushboolean(L, control.aux1);
		lua_setfield(L, -2, "aux1");
		lua_pushboolean(L, control.sneak);
		lua_setfield(L, -2, "sneak");
		lua_pushboolean(L, control.LMB);
		lua_setfield(L, -2, "LMB");
		lua_pushboolean(L, control.RMB);
		lua_setfield(L, -2, "RMB");
		return 1;
	}
	
	// get_player_control_bits(self)
	static int l_get_player_control_bits(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		Player *player = getplayer(ref);
		if(player == NULL){
			lua_pushlstring(L, "", 0);
			return 1;
		}
		// Do it	
		lua_pushnumber(L, player->keyPressed);
		return 1;
	}
	
public:
	ObjectRef(ServerActiveObject *object):
		m_object(object)
	{
		//infostream<<"ObjectRef created for id="<<m_object->getId()<<std::endl;
	}

	~ObjectRef()
	{
		/*if(m_object)
			infostream<<"ObjectRef destructing for id="
					<<m_object->getId()<<std::endl;
		else
			infostream<<"ObjectRef destructing for id=unknown"<<std::endl;*/
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
const luaL_reg ObjectRef::methods[] = {
	// ServerActiveObject
	method(ObjectRef, remove),
	method(ObjectRef, getpos),
	method(ObjectRef, setpos),
	method(ObjectRef, moveto),
	method(ObjectRef, punch),
	method(ObjectRef, right_click),
	method(ObjectRef, set_hp),
	method(ObjectRef, get_hp),
	method(ObjectRef, get_inventory),
	method(ObjectRef, get_wield_list),
	method(ObjectRef, get_wield_index),
	method(ObjectRef, get_wielded_item),
	method(ObjectRef, set_wielded_item),
	method(ObjectRef, set_armor_groups),
	method(ObjectRef, set_animation),
	method(ObjectRef, set_bone_position),
	method(ObjectRef, set_attach),
	method(ObjectRef, set_detach),
	method(ObjectRef, set_properties),
	// LuaEntitySAO-only
	method(ObjectRef, setvelocity),
	method(ObjectRef, getvelocity),
	method(ObjectRef, setacceleration),
	method(ObjectRef, getacceleration),
	method(ObjectRef, setyaw),
	method(ObjectRef, getyaw),
	method(ObjectRef, settexturemod),
	method(ObjectRef, setsprite),
	method(ObjectRef, get_entity_name),
	method(ObjectRef, get_luaentity),
	// Player-only
	method(ObjectRef, is_player),
	method(ObjectRef, get_player_name),
	method(ObjectRef, get_look_dir),
	method(ObjectRef, get_look_pitch),
	method(ObjectRef, get_look_yaw),
	method(ObjectRef, set_inventory_formspec),
	method(ObjectRef, get_inventory_formspec),
	method(ObjectRef, get_player_control),
	method(ObjectRef, get_player_control_bits),
	{0,0}
};

// Creates a new anonymous reference if cobj=NULL or id=0
static void objectref_get_or_create(lua_State *L,
		ServerActiveObject *cobj)
{
	if(cobj == NULL || cobj->getId() == 0){
		ObjectRef::create(L, cobj);
	} else {
		objectref_get(L, cobj->getId());
	}
}


/*
  PerlinNoise
 */

class LuaPerlinNoise
{
private:
	int seed;
	int octaves;
	double persistence;
	double scale;
	static const char className[];
	static const luaL_reg methods[];

	// Exported functions

	// garbage collector
	static int gc_object(lua_State *L)
	{
		LuaPerlinNoise *o = *(LuaPerlinNoise **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	static int l_get2d(lua_State *L)
	{
		LuaPerlinNoise *o = checkobject(L, 1);
		v2f pos2d = read_v2f(L,2);
		lua_Number val = noise2d_perlin(pos2d.X/o->scale, pos2d.Y/o->scale, o->seed, o->octaves, o->persistence);
		lua_pushnumber(L, val);
		return 1;
	}
	static int l_get3d(lua_State *L)
	{
		LuaPerlinNoise *o = checkobject(L, 1);
		v3f pos3d = read_v3f(L,2);
		lua_Number val = noise3d_perlin(pos3d.X/o->scale, pos3d.Y/o->scale, pos3d.Z/o->scale, o->seed, o->octaves, o->persistence);
		lua_pushnumber(L, val);
		return 1;
	}

public:
	LuaPerlinNoise(int a_seed, int a_octaves, double a_persistence,
			double a_scale):
		seed(a_seed),
		octaves(a_octaves),
		persistence(a_persistence),
		scale(a_scale)
	{
	}

	~LuaPerlinNoise()
	{
	}

	// LuaPerlinNoise(seed, octaves, persistence, scale)
	// Creates an LuaPerlinNoise and leaves it on top of stack
	static int create_object(lua_State *L)
	{
		int seed = luaL_checkint(L, 1);
		int octaves = luaL_checkint(L, 2);
		double persistence = luaL_checknumber(L, 3);
		double scale = luaL_checknumber(L, 4);
		LuaPerlinNoise *o = new LuaPerlinNoise(seed, octaves, persistence, scale);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
		return 1;
	}

	static LuaPerlinNoise* checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(LuaPerlinNoise**)ud;  // unbox pointer
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

		// Can be created from Lua (PerlinNoise(seed, octaves, persistence)
		lua_register(L, className, create_object);
	}
};
const char LuaPerlinNoise::className[] = "PerlinNoise";
const luaL_reg LuaPerlinNoise::methods[] = {
	method(LuaPerlinNoise, get2d),
	method(LuaPerlinNoise, get3d),
	{0,0}
};

/*
	NodeTimerRef
*/

class NodeTimerRef
{
private:
	v3s16 m_p;
	ServerEnvironment *m_env;

	static const char className[];
	static const luaL_reg methods[];

	static int gc_object(lua_State *L) {
		NodeTimerRef *o = *(NodeTimerRef **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	static NodeTimerRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(NodeTimerRef**)ud;  // unbox pointer
	}
	
	static int l_set(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		f32 t = luaL_checknumber(L,2);
		f32 e = luaL_checknumber(L,3);
		env->getMap().setNodeTimer(o->m_p,NodeTimer(t,e));
		return 0;
	}
	
	static int l_start(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		f32 t = luaL_checknumber(L,2);
		env->getMap().setNodeTimer(o->m_p,NodeTimer(t,0));
		return 0;
	}
	
	static int l_stop(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		env->getMap().removeNodeTimer(o->m_p);
		return 0;
	}
	
	static int l_is_started(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;

		NodeTimer t = env->getMap().getNodeTimer(o->m_p);
		lua_pushboolean(L,(t.timeout != 0));
		return 1;
	}
	
	static int l_get_timeout(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;

		NodeTimer t = env->getMap().getNodeTimer(o->m_p);
		lua_pushnumber(L,t.timeout);
		return 1;
	}
	
	static int l_get_elapsed(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;

		NodeTimer t = env->getMap().getNodeTimer(o->m_p);
		lua_pushnumber(L,t.elapsed);
		return 1;
	}

public:
	NodeTimerRef(v3s16 p, ServerEnvironment *env):
		m_p(p),
		m_env(env)
	{
	}

	~NodeTimerRef()
	{
	}

	// Creates an NodeTimerRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, v3s16 p, ServerEnvironment *env)
	{
		NodeTimerRef *o = new NodeTimerRef(p, env);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
	}

	static void set_null(lua_State *L)
	{
		NodeTimerRef *o = checkobject(L, -1);
		o->m_env = NULL;
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
const char NodeTimerRef::className[] = "NodeTimerRef";
const luaL_reg NodeTimerRef::methods[] = {
	method(NodeTimerRef, start),
	method(NodeTimerRef, set),
	method(NodeTimerRef, stop),
	method(NodeTimerRef, is_started),
	method(NodeTimerRef, get_timeout),
	method(NodeTimerRef, get_elapsed),
	{0,0}
};

/*
	EnvRef
*/

class EnvRef
{
private:
	ServerEnvironment *m_env;

	static const char className[];
	static const luaL_reg methods[];

	static int gc_object(lua_State *L) {
		EnvRef *o = *(EnvRef **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	static EnvRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(EnvRef**)ud;  // unbox pointer
	}
	
	// Exported functions

	// EnvRef:set_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_set_node(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		INodeDefManager *ndef = env->getGameDef()->ndef();
		// parameters
		v3s16 pos = read_v3s16(L, 2);
		MapNode n = readnode(L, 3, ndef);
		// Do it
		MapNode n_old = env->getMap().getNodeNoEx(pos);
		// Call destructor
		if(ndef->get(n_old).has_on_destruct)
			scriptapi_node_on_destruct(L, pos, n_old);
		// Replace node
		bool succeeded = env->getMap().addNodeWithEvent(pos, n);
		if(succeeded){
			// Call post-destructor
			if(ndef->get(n_old).has_after_destruct)
				scriptapi_node_after_destruct(L, pos, n_old);
			// Call constructor
			if(ndef->get(n).has_on_construct)
				scriptapi_node_on_construct(L, pos, n);
		}
		lua_pushboolean(L, succeeded);
		return 1;
	}

	static int l_add_node(lua_State *L)
	{
		return l_set_node(L);
	}

	// EnvRef:remove_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_remove_node(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		INodeDefManager *ndef = env->getGameDef()->ndef();
		// parameters
		v3s16 pos = read_v3s16(L, 2);
		// Do it
		MapNode n_old = env->getMap().getNodeNoEx(pos);
		// Call destructor
		if(ndef->get(n_old).has_on_destruct)
			scriptapi_node_on_destruct(L, pos, n_old);
		// Replace with air
		// This is slightly optimized compared to addNodeWithEvent(air)
		bool succeeded = env->getMap().removeNodeWithEvent(pos);
		if(succeeded){
			// Call post-destructor
			if(ndef->get(n_old).has_after_destruct)
				scriptapi_node_after_destruct(L, pos, n_old);
		}
		lua_pushboolean(L, succeeded);
		// Air doesn't require constructor
		return 1;
	}

	// EnvRef:get_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3s16 pos = read_v3s16(L, 2);
		// Do it
		MapNode n = env->getMap().getNodeNoEx(pos);
		// Return node
		pushnode(L, n, env->getGameDef()->ndef());
		return 1;
	}

	// EnvRef:get_node_or_nil(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node_or_nil(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3s16 pos = read_v3s16(L, 2);
		// Do it
		try{
			MapNode n = env->getMap().getNode(pos);
			// Return node
			pushnode(L, n, env->getGameDef()->ndef());
			return 1;
		} catch(InvalidPositionException &e)
		{
			lua_pushnil(L);
			return 1;
		}
	}

	// EnvRef:get_node_light(pos, timeofday)
	// pos = {x=num, y=num, z=num}
	// timeofday: nil = current time, 0 = night, 0.5 = day
	static int l_get_node_light(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		v3s16 pos = read_v3s16(L, 2);
		u32 time_of_day = env->getTimeOfDay();
		if(lua_isnumber(L, 3))
			time_of_day = 24000.0 * lua_tonumber(L, 3);
		time_of_day %= 24000;
		u32 dnr = time_to_daynight_ratio(time_of_day);
		MapNode n = env->getMap().getNodeNoEx(pos);
		try{
			MapNode n = env->getMap().getNode(pos);
			INodeDefManager *ndef = env->getGameDef()->ndef();
			lua_pushinteger(L, n.getLightBlend(dnr, ndef));
			return 1;
		} catch(InvalidPositionException &e)
		{
			lua_pushnil(L);
			return 1;
		}
	}

	// EnvRef:place_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_place_node(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		v3s16 pos = read_v3s16(L, 2);
		MapNode n = readnode(L, 3, env->getGameDef()->ndef());

		// Don't attempt to load non-loaded area as of now
		MapNode n_old = env->getMap().getNodeNoEx(pos);
		if(n_old.getContent() == CONTENT_IGNORE){
			lua_pushboolean(L, false);
			return 1;
		}
		// Create item to place
		INodeDefManager *ndef = get_server(L)->ndef();
		IItemDefManager *idef = get_server(L)->idef();
		ItemStack item(ndef->get(n).name, 1, 0, "", idef);
		// Make pointed position
		PointedThing pointed;
		pointed.type = POINTEDTHING_NODE;
		pointed.node_abovesurface = pos;
		pointed.node_undersurface = pos + v3s16(0,-1,0);
		// Place it with a NULL placer (appears in Lua as a non-functional
		// ObjectRef)
		bool success = scriptapi_item_on_place(L, item, NULL, pointed);
		lua_pushboolean(L, success);
		return 1;
	}

	// EnvRef:dig_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_dig_node(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		v3s16 pos = read_v3s16(L, 2);

		// Don't attempt to load non-loaded area as of now
		MapNode n = env->getMap().getNodeNoEx(pos);
		if(n.getContent() == CONTENT_IGNORE){
			lua_pushboolean(L, false);
			return 1;
		}
		// Dig it out with a NULL digger (appears in Lua as a
		// non-functional ObjectRef)
		bool success = scriptapi_node_on_dig(L, pos, n, NULL);
		lua_pushboolean(L, success);
		return 1;
	}

	// EnvRef:punch_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_punch_node(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		v3s16 pos = read_v3s16(L, 2);

		// Don't attempt to load non-loaded area as of now
		MapNode n = env->getMap().getNodeNoEx(pos);
		if(n.getContent() == CONTENT_IGNORE){
			lua_pushboolean(L, false);
			return 1;
		}
		// Punch it with a NULL puncher (appears in Lua as a non-functional
		// ObjectRef)
		bool success = scriptapi_node_on_punch(L, pos, n, NULL);
		lua_pushboolean(L, success);
		return 1;
	}

	// EnvRef:get_meta(pos)
	static int l_get_meta(lua_State *L)
	{
		//infostream<<"EnvRef::l_get_meta()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		v3s16 p = read_v3s16(L, 2);
		NodeMetaRef::create(L, p, env);
		return 1;
	}

	// EnvRef:get_node_timer(pos)
	static int l_get_node_timer(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		v3s16 p = read_v3s16(L, 2);
		NodeTimerRef::create(L, p, env);
		return 1;
	}

	// EnvRef:add_entity(pos, entityname) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	static int l_add_entity(lua_State *L)
	{
		//infostream<<"EnvRef::l_add_entity()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3f pos = checkFloatPos(L, 2);
		// content
		const char *name = luaL_checkstring(L, 3);
		// Do it
		ServerActiveObject *obj = new LuaEntitySAO(env, pos, name, "");
		int objectid = env->addActiveObject(obj);
		// If failed to add, return nothing (reads as nil)
		if(objectid == 0)
			return 0;
		// Return ObjectRef
		objectref_get_or_create(L, obj);
		return 1;
	}

	// EnvRef:add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
	// pos = {x=num, y=num, z=num}
	static int l_add_item(lua_State *L)
	{
		//infostream<<"EnvRef::l_add_item()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3f pos = checkFloatPos(L, 2);
		// item
		ItemStack item = read_item(L, 3);
		if(item.empty() || !item.isKnown(get_server(L)->idef()))
			return 0;
		// Use minetest.spawn_item to spawn a __builtin:item
		lua_getglobal(L, "minetest");
		lua_getfield(L, -1, "spawn_item");
		if(lua_isnil(L, -1))
			return 0;
		lua_pushvalue(L, 2);
		lua_pushstring(L, item.getItemString().c_str());
		if(lua_pcall(L, 2, 1, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
		return 1;
		/*lua_pushvalue(L, 1);
		lua_pushstring(L, "__builtin:item");
		lua_pushstring(L, item.getItemString().c_str());
		return l_add_entity(L);*/
		/*// Do it
		ServerActiveObject *obj = createItemSAO(env, pos, item.getItemString());
		int objectid = env->addActiveObject(obj);
		// If failed to add, return nothing (reads as nil)
		if(objectid == 0)
			return 0;
		// Return ObjectRef
		objectref_get_or_create(L, obj);
		return 1;*/
	}

	// EnvRef:add_rat(pos)
	// pos = {x=num, y=num, z=num}
	static int l_add_rat(lua_State *L)
	{
		infostream<<"EnvRef::l_add_rat(): C++ mobs have been removed."
				<<" Doing nothing."<<std::endl;
		return 0;
	}

	// EnvRef:add_firefly(pos)
	// pos = {x=num, y=num, z=num}
	static int l_add_firefly(lua_State *L)
	{
		infostream<<"EnvRef::l_add_firefly(): C++ mobs have been removed."
				<<" Doing nothing."<<std::endl;
		return 0;
	}

	// EnvRef:get_player_by_name(name)
	static int l_get_player_by_name(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		const char *name = luaL_checkstring(L, 2);
		Player *player = env->getPlayer(name);
		if(player == NULL){
			lua_pushnil(L);
			return 1;
		}
		PlayerSAO *sao = player->getPlayerSAO();
		if(sao == NULL){
			lua_pushnil(L);
			return 1;
		}
		// Put player on stack
		objectref_get_or_create(L, sao);
		return 1;
	}

	// EnvRef:get_objects_inside_radius(pos, radius)
	static int l_get_objects_inside_radius(lua_State *L)
	{
		// Get the table insert function
		lua_getglobal(L, "table");
		lua_getfield(L, -1, "insert");
		int table_insert = lua_gettop(L);
		// Get environemnt
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		v3f pos = checkFloatPos(L, 2);
		float radius = luaL_checknumber(L, 3) * BS;
		std::set<u16> ids = env->getObjectsInsideRadius(pos, radius);
		lua_newtable(L);
		int table = lua_gettop(L);
		for(std::set<u16>::const_iterator
				i = ids.begin(); i != ids.end(); i++){
			ServerActiveObject *obj = env->getActiveObject(*i);
			// Insert object reference into table
			lua_pushvalue(L, table_insert);
			lua_pushvalue(L, table);
			objectref_get_or_create(L, obj);
			if(lua_pcall(L, 2, 0, 0))
				script_error(L, "error: %s", lua_tostring(L, -1));
		}
		return 1;
	}

	// EnvRef:set_timeofday(val)
	// val = 0...1
	static int l_set_timeofday(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		float timeofday_f = luaL_checknumber(L, 2);
		assert(timeofday_f >= 0.0 && timeofday_f <= 1.0);
		int timeofday_mh = (int)(timeofday_f * 24000.0);
		// This should be set directly in the environment but currently
		// such changes aren't immediately sent to the clients, so call
		// the server instead.
		//env->setTimeOfDay(timeofday_mh);
		get_server(L)->setTimeOfDay(timeofday_mh);
		return 0;
	}

	// EnvRef:get_timeofday() -> 0...1
	static int l_get_timeofday(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// Do it
		int timeofday_mh = env->getTimeOfDay();
		float timeofday_f = (float)timeofday_mh / 24000.0;
		lua_pushnumber(L, timeofday_f);
		return 1;
	}


	// EnvRef:find_node_near(pos, radius, nodenames) -> pos or nil
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	static int l_find_node_near(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		INodeDefManager *ndef = get_server(L)->ndef();
		v3s16 pos = read_v3s16(L, 2);
		int radius = luaL_checkinteger(L, 3);
		std::set<content_t> filter;
		if(lua_istable(L, 4)){
			int table = 4;
			lua_pushnil(L);
			while(lua_next(L, table) != 0){
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				ndef->getIds(lua_tostring(L, -1), filter);
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if(lua_isstring(L, 4)){
			ndef->getIds(lua_tostring(L, 4), filter);
		}

		for(int d=1; d<=radius; d++){
			core::list<v3s16> list;
			getFacePositions(list, d);
			for(core::list<v3s16>::Iterator i = list.begin();
					i != list.end(); i++){
				v3s16 p = pos + (*i);
				content_t c = env->getMap().getNodeNoEx(p).getContent();
				if(filter.count(c) != 0){
					push_v3s16(L, p);
					return 1;
				}
			}
		}
		return 0;
	}

	// EnvRef:find_nodes_in_area(minp, maxp, nodenames) -> list of positions
	// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
	static int l_find_nodes_in_area(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		INodeDefManager *ndef = get_server(L)->ndef();
		v3s16 minp = read_v3s16(L, 2);
		v3s16 maxp = read_v3s16(L, 3);
		std::set<content_t> filter;
		if(lua_istable(L, 4)){
			int table = 4;
			lua_pushnil(L);
			while(lua_next(L, table) != 0){
				// key at index -2 and value at index -1
				luaL_checktype(L, -1, LUA_TSTRING);
				ndef->getIds(lua_tostring(L, -1), filter);
				// removes value, keeps key for next iteration
				lua_pop(L, 1);
			}
		} else if(lua_isstring(L, 4)){
			ndef->getIds(lua_tostring(L, 4), filter);
		}

		// Get the table insert function
		lua_getglobal(L, "table");
		lua_getfield(L, -1, "insert");
		int table_insert = lua_gettop(L);
		
		lua_newtable(L);
		int table = lua_gettop(L);
		for(s16 x=minp.X; x<=maxp.X; x++)
		for(s16 y=minp.Y; y<=maxp.Y; y++)
		for(s16 z=minp.Z; z<=maxp.Z; z++)
		{
			v3s16 p(x,y,z);
			content_t c = env->getMap().getNodeNoEx(p).getContent();
			if(filter.count(c) != 0){
				lua_pushvalue(L, table_insert);
				lua_pushvalue(L, table);
				push_v3s16(L, p);
				if(lua_pcall(L, 2, 0, 0))
					script_error(L, "error: %s", lua_tostring(L, -1));
			}
		}
		return 1;
	}

	//	EnvRef:get_perlin(seeddiff, octaves, persistence, scale)
	//  returns world-specific PerlinNoise
	static int l_get_perlin(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;

		int seeddiff = luaL_checkint(L, 2);
		int octaves = luaL_checkint(L, 3);
		double persistence = luaL_checknumber(L, 4);
		double scale = luaL_checknumber(L, 5);

		LuaPerlinNoise *n = new LuaPerlinNoise(seeddiff + int(env->getServerMap().getSeed()), octaves, persistence, scale);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
		luaL_getmetatable(L, "PerlinNoise");
		lua_setmetatable(L, -2);
		return 1;
	}

	// EnvRef:clear_objects()
	// clear all objects in the environment
	static int l_clear_objects(lua_State *L)
	{
		EnvRef *o = checkobject(L, 1);
		o->m_env->clearAllObjects();
		return 0;
	}

public:
	EnvRef(ServerEnvironment *env):
		m_env(env)
	{
		//infostream<<"EnvRef created"<<std::endl;
	}

	~EnvRef()
	{
		//infostream<<"EnvRef destructing"<<std::endl;
	}

	// Creates an EnvRef and leaves it on top of stack
	// Not callable from Lua; all references are created on the C side.
	static void create(lua_State *L, ServerEnvironment *env)
	{
		EnvRef *o = new EnvRef(env);
		//infostream<<"EnvRef::create: o="<<o<<std::endl;
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
	}

	static void set_null(lua_State *L)
	{
		EnvRef *o = checkobject(L, -1);
		o->m_env = NULL;
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
const char EnvRef::className[] = "EnvRef";
const luaL_reg EnvRef::methods[] = {
	method(EnvRef, set_node),
	method(EnvRef, add_node),
	method(EnvRef, remove_node),
	method(EnvRef, get_node),
	method(EnvRef, get_node_or_nil),
	method(EnvRef, get_node_light),
	method(EnvRef, place_node),
	method(EnvRef, dig_node),
	method(EnvRef, punch_node),
	method(EnvRef, add_entity),
	method(EnvRef, add_item),
	method(EnvRef, add_rat),
	method(EnvRef, add_firefly),
	method(EnvRef, get_meta),
	method(EnvRef, get_node_timer),
	method(EnvRef, get_player_by_name),
	method(EnvRef, get_objects_inside_radius),
	method(EnvRef, set_timeofday),
	method(EnvRef, get_timeofday),
	method(EnvRef, find_node_near),
	method(EnvRef, find_nodes_in_area),
	method(EnvRef, get_perlin),
	method(EnvRef, clear_objects),
	{0,0}
};

/*
	LuaPseudoRandom
*/


class LuaPseudoRandom
{
private:
	PseudoRandom m_pseudo;

	static const char className[];
	static const luaL_reg methods[];

	// Exported functions
	
	// garbage collector
	static int gc_object(lua_State *L)
	{
		LuaPseudoRandom *o = *(LuaPseudoRandom **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

	// next(self, min=0, max=32767) -> get next value
	static int l_next(lua_State *L)
	{
		LuaPseudoRandom *o = checkobject(L, 1);
		int min = 0;
		int max = 32767;
		lua_settop(L, 3); // Fill 2 and 3 with nil if they don't exist
		if(!lua_isnil(L, 2))
			min = luaL_checkinteger(L, 2);
		if(!lua_isnil(L, 3))
			max = luaL_checkinteger(L, 3);
		if(max < min){
			errorstream<<"PseudoRandom.next(): max="<<max<<" min="<<min<<std::endl;
			throw LuaError(L, "PseudoRandom.next(): max < min");
		}
		if(max - min != 32767 && max - min > 32767/5)
			throw LuaError(L, "PseudoRandom.next() max-min is not 32767 and is > 32768/5. This is disallowed due to the bad random distribution the implementation would otherwise make.");
		PseudoRandom &pseudo = o->m_pseudo;
		int val = pseudo.next();
		val = (val % (max-min+1)) + min;
		lua_pushinteger(L, val);
		return 1;
	}

public:
	LuaPseudoRandom(int seed):
		m_pseudo(seed)
	{
	}

	~LuaPseudoRandom()
	{
	}

	const PseudoRandom& getItem() const
	{
		return m_pseudo;
	}
	PseudoRandom& getItem()
	{
		return m_pseudo;
	}
	
	// LuaPseudoRandom(seed)
	// Creates an LuaPseudoRandom and leaves it on top of stack
	static int create_object(lua_State *L)
	{
		int seed = luaL_checknumber(L, 1);
		LuaPseudoRandom *o = new LuaPseudoRandom(seed);
		*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
		luaL_getmetatable(L, className);
		lua_setmetatable(L, -2);
		return 1;
	}

	static LuaPseudoRandom* checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(LuaPseudoRandom**)ud;  // unbox pointer
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

		// Can be created from Lua (LuaPseudoRandom(seed))
		lua_register(L, className, create_object);
	}
};
const char LuaPseudoRandom::className[] = "PseudoRandom";
const luaL_reg LuaPseudoRandom::methods[] = {
	method(LuaPseudoRandom, next),
	{0,0}
};



/*
	LuaABM
*/

class LuaABM : public ActiveBlockModifier
{
private:
	lua_State *m_lua;
	int m_id;

	std::set<std::string> m_trigger_contents;
	std::set<std::string> m_required_neighbors;
	float m_trigger_interval;
	u32 m_trigger_chance;
public:
	LuaABM(lua_State *L, int id,
			const std::set<std::string> &trigger_contents,
			const std::set<std::string> &required_neighbors,
			float trigger_interval, u32 trigger_chance):
		m_lua(L),
		m_id(id),
		m_trigger_contents(trigger_contents),
		m_required_neighbors(required_neighbors),
		m_trigger_interval(trigger_interval),
		m_trigger_chance(trigger_chance)
	{
	}
	virtual std::set<std::string> getTriggerContents()
	{
		return m_trigger_contents;
	}
	virtual std::set<std::string> getRequiredNeighbors()
	{
		return m_required_neighbors;
	}
	virtual float getTriggerInterval()
	{
		return m_trigger_interval;
	}
	virtual u32 getTriggerChance()
	{
		return m_trigger_chance;
	}
	virtual void trigger(ServerEnvironment *env, v3s16 p, MapNode n,
			u32 active_object_count, u32 active_object_count_wider)
	{
		lua_State *L = m_lua;
	
		realitycheck(L);
		assert(lua_checkstack(L, 20));
		StackUnroller stack_unroller(L);

		// Get minetest.registered_abms
		lua_getglobal(L, "minetest");
		lua_getfield(L, -1, "registered_abms");
		luaL_checktype(L, -1, LUA_TTABLE);
		int registered_abms = lua_gettop(L);

		// Get minetest.registered_abms[m_id]
		lua_pushnumber(L, m_id);
		lua_gettable(L, registered_abms);
		if(lua_isnil(L, -1))
			assert(0);
		
		// Call action
		luaL_checktype(L, -1, LUA_TTABLE);
		lua_getfield(L, -1, "action");
		luaL_checktype(L, -1, LUA_TFUNCTION);
		push_v3s16(L, p);
		pushnode(L, n, env->getGameDef()->ndef());
		lua_pushnumber(L, active_object_count);
		lua_pushnumber(L, active_object_count_wider);
		if(lua_pcall(L, 4, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
};

/*
	ServerSoundParams
*/

static void read_server_sound_params(lua_State *L, int index,
		ServerSoundParams &params)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;
	// Clear
	params = ServerSoundParams();
	if(lua_istable(L, index)){
		getfloatfield(L, index, "gain", params.gain);
		getstringfield(L, index, "to_player", params.to_player);
		lua_getfield(L, index, "pos");
		if(!lua_isnil(L, -1)){
			v3f p = read_v3f(L, -1)*BS;
			params.pos = p;
			params.type = ServerSoundParams::SSP_POSITIONAL;
		}
		lua_pop(L, 1);
		lua_getfield(L, index, "object");
		if(!lua_isnil(L, -1)){
			ObjectRef *ref = ObjectRef::checkobject(L, -1);
			ServerActiveObject *sao = ObjectRef::getobject(ref);
			if(sao){
				params.object = sao->getId();
				params.type = ServerSoundParams::SSP_OBJECT;
			}
		}
		lua_pop(L, 1);
		params.max_hear_distance = BS*getfloatfield_default(L, index,
				"max_hear_distance", params.max_hear_distance/BS);
		getboolfield(L, index, "loop", params.loop);
	}
}

/*
	Global functions
*/

// debug(text)
// Writes a line to dstream
static int l_debug(lua_State *L)
{
	std::string text = lua_tostring(L, 1);
	dstream << text << std::endl;
	return 0;
}

// log([level,] text)
// Writes a line to the logger.
// The one-argument version logs to infostream.
// The two-argument version accept a log level: error, action, info, or verbose.
static int l_log(lua_State *L)
{
	std::string text;
	LogMessageLevel level = LMT_INFO;
	if(lua_isnone(L, 2))
	{
		text = lua_tostring(L, 1);
	}
	else
	{
		std::string levelname = lua_tostring(L, 1);
		text = lua_tostring(L, 2);
		if(levelname == "error")
			level = LMT_ERROR;
		else if(levelname == "action")
			level = LMT_ACTION;
		else if(levelname == "verbose")
			level = LMT_VERBOSE;
	}
	log_printline(level, text);
	return 0;
}

// request_shutdown()
static int l_request_shutdown(lua_State *L)
{
	get_server(L)->requestShutdown();
	return 0;
}

// get_server_status()
static int l_get_server_status(lua_State *L)
{
	lua_pushstring(L, wide_to_narrow(get_server(L)->getStatusString()).c_str());
	return 1;
}

// register_item_raw({lots of stuff})
static int l_register_item_raw(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	int table = 1;

	// Get the writable item and node definition managers from the server
	IWritableItemDefManager *idef =
			get_server(L)->getWritableItemDefManager();
	IWritableNodeDefManager *ndef =
			get_server(L)->getWritableNodeDefManager();

	// Check if name is defined
	std::string name;
	lua_getfield(L, table, "name");
	if(lua_isstring(L, -1)){
		name = lua_tostring(L, -1);
		verbosestream<<"register_item_raw: "<<name<<std::endl;
	} else {
		throw LuaError(L, "register_item_raw: name is not defined or not a string");
	}

	// Check if on_use is defined

	ItemDefinition def;
	// Set a distinctive default value to check if this is set
	def.node_placement_prediction = "__default";

	// Read the item definition
	def = read_item_definition(L, table, def);

	// Default to having client-side placement prediction for nodes
	// ("" in item definition sets it off)
	if(def.node_placement_prediction == "__default"){
		if(def.type == ITEM_NODE)
			def.node_placement_prediction = name;
		else
			def.node_placement_prediction = "";
	}
	
	// Register item definition
	idef->registerItem(def);

	// Read the node definition (content features) and register it
	if(def.type == ITEM_NODE)
	{
		ContentFeatures f = read_content_features(L, table);
		ndef->set(f.name, f);
	}

	return 0; /* number of results */
}

// register_alias_raw(name, convert_to_name)
static int l_register_alias_raw(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	std::string convert_to = luaL_checkstring(L, 2);

	// Get the writable item definition manager from the server
	IWritableItemDefManager *idef =
			get_server(L)->getWritableItemDefManager();
	
	idef->registerAlias(name, convert_to);
	
	return 0; /* number of results */
}

// helper for register_craft
static bool read_craft_recipe_shaped(lua_State *L, int index,
		int &width, std::vector<std::string> &recipe)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(!lua_istable(L, index))
		return false;

	lua_pushnil(L);
	int rowcount = 0;
	while(lua_next(L, index) != 0){
		int colcount = 0;
		// key at index -2 and value at index -1
		if(!lua_istable(L, -1))
			return false;
		int table2 = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table2) != 0){
			// key at index -2 and value at index -1
			if(!lua_isstring(L, -1))
				return false;
			recipe.push_back(lua_tostring(L, -1));
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			colcount++;
		}
		if(rowcount == 0){
			width = colcount;
		} else {
			if(colcount != width)
				return false;
		}
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
		rowcount++;
	}
	return width != 0;
}

// helper for register_craft
static bool read_craft_recipe_shapeless(lua_State *L, int index,
		std::vector<std::string> &recipe)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(!lua_istable(L, index))
		return false;

	lua_pushnil(L);
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		if(!lua_isstring(L, -1))
			return false;
		recipe.push_back(lua_tostring(L, -1));
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	return true;
}

// helper for register_craft
static bool read_craft_replacements(lua_State *L, int index,
		CraftReplacements &replacements)
{
	if(index < 0)
		index = lua_gettop(L) + 1 + index;

	if(!lua_istable(L, index))
		return false;

	lua_pushnil(L);
	while(lua_next(L, index) != 0){
		// key at index -2 and value at index -1
		if(!lua_istable(L, -1))
			return false;
		lua_rawgeti(L, -1, 1);
		if(!lua_isstring(L, -1))
			return false;
		std::string replace_from = lua_tostring(L, -1);
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 2);
		if(!lua_isstring(L, -1))
			return false;
		std::string replace_to = lua_tostring(L, -1);
		lua_pop(L, 1);
		replacements.pairs.push_back(
				std::make_pair(replace_from, replace_to));
		// removes value, keeps key for next iteration
		lua_pop(L, 1);
	}
	return true;
}
// register_craft({output=item, recipe={{item00,item10},{item01,item11}})
static int l_register_craft(lua_State *L)
{
	//infostream<<"register_craft"<<std::endl;
	luaL_checktype(L, 1, LUA_TTABLE);
	int table = 1;

	// Get the writable craft definition manager from the server
	IWritableCraftDefManager *craftdef =
			get_server(L)->getWritableCraftDefManager();
	
	std::string type = getstringfield_default(L, table, "type", "shaped");

	/*
		CraftDefinitionShaped
	*/
	if(type == "shaped"){
		std::string output = getstringfield_default(L, table, "output", "");
		if(output == "")
			throw LuaError(L, "Crafting definition is missing an output");

		int width = 0;
		std::vector<std::string> recipe;
		lua_getfield(L, table, "recipe");
		if(lua_isnil(L, -1))
			throw LuaError(L, "Crafting definition is missing a recipe"
					" (output=\"" + output + "\")");
		if(!read_craft_recipe_shaped(L, -1, width, recipe))
			throw LuaError(L, "Invalid crafting recipe"
					" (output=\"" + output + "\")");

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (output=\"" + output + "\")");
		}

		CraftDefinition *def = new CraftDefinitionShaped(
				output, width, recipe, replacements);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionShapeless
	*/
	else if(type == "shapeless"){
		std::string output = getstringfield_default(L, table, "output", "");
		if(output == "")
			throw LuaError(L, "Crafting definition (shapeless)"
					" is missing an output");

		std::vector<std::string> recipe;
		lua_getfield(L, table, "recipe");
		if(lua_isnil(L, -1))
			throw LuaError(L, "Crafting definition (shapeless)"
					" is missing a recipe"
					" (output=\"" + output + "\")");
		if(!read_craft_recipe_shapeless(L, -1, recipe))
			throw LuaError(L, "Invalid crafting recipe"
					" (output=\"" + output + "\")");

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (output=\"" + output + "\")");
		}

		CraftDefinition *def = new CraftDefinitionShapeless(
				output, recipe, replacements);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionToolRepair
	*/
	else if(type == "toolrepair"){
		float additional_wear = getfloatfield_default(L, table,
				"additional_wear", 0.0);

		CraftDefinition *def = new CraftDefinitionToolRepair(
				additional_wear);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionCooking
	*/
	else if(type == "cooking"){
		std::string output = getstringfield_default(L, table, "output", "");
		if(output == "")
			throw LuaError(L, "Crafting definition (cooking)"
					" is missing an output");

		std::string recipe = getstringfield_default(L, table, "recipe", "");
		if(recipe == "")
			throw LuaError(L, "Crafting definition (cooking)"
					" is missing a recipe"
					" (output=\"" + output + "\")");

		float cooktime = getfloatfield_default(L, table, "cooktime", 3.0);

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (cooking output=\"" + output + "\")");
		}

		CraftDefinition *def = new CraftDefinitionCooking(
				output, recipe, cooktime, replacements);
		craftdef->registerCraft(def);
	}
	/*
		CraftDefinitionFuel
	*/
	else if(type == "fuel"){
		std::string recipe = getstringfield_default(L, table, "recipe", "");
		if(recipe == "")
			throw LuaError(L, "Crafting definition (fuel)"
					" is missing a recipe");

		float burntime = getfloatfield_default(L, table, "burntime", 1.0);

		CraftReplacements replacements;
		lua_getfield(L, table, "replacements");
		if(!lua_isnil(L, -1))
		{
			if(!read_craft_replacements(L, -1, replacements))
				throw LuaError(L, "Invalid replacements"
						" (fuel recipe=\"" + recipe + "\")");
		}

		CraftDefinition *def = new CraftDefinitionFuel(
				recipe, burntime, replacements);
		craftdef->registerCraft(def);
	}
	else
	{
		throw LuaError(L, "Unknown crafting definition type: \"" + type + "\"");
	}

	lua_pop(L, 1);
	return 0; /* number of results */
}

// setting_set(name, value)
static int l_setting_set(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	const char *value = luaL_checkstring(L, 2);
	g_settings->set(name, value);
	return 0;
}

// setting_get(name)
static int l_setting_get(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	try{
		std::string value = g_settings->get(name);
		lua_pushstring(L, value.c_str());
	} catch(SettingNotFoundException &e){
		lua_pushnil(L);
	}
	return 1;
}

// setting_getbool(name)
static int l_setting_getbool(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	try{
		bool value = g_settings->getBool(name);
		lua_pushboolean(L, value);
	} catch(SettingNotFoundException &e){
		lua_pushnil(L);
	}
	return 1;
}

// chat_send_all(text)
static int l_chat_send_all(lua_State *L)
{
	const char *text = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = get_server(L);
	// Send
	server->notifyPlayers(narrow_to_wide(text));
	return 0;
}

// chat_send_player(name, text)
static int l_chat_send_player(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	const char *text = luaL_checkstring(L, 2);
	// Get server from registry
	Server *server = get_server(L);
	// Send
	server->notifyPlayer(name, narrow_to_wide(text));
	return 0;
}

// get_player_privs(name, text)
static int l_get_player_privs(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	// Get server from registry
	Server *server = get_server(L);
	// Do it
	lua_newtable(L);
	int table = lua_gettop(L);
	std::set<std::string> privs_s = server->getPlayerEffectivePrivs(name);
	for(std::set<std::string>::const_iterator
			i = privs_s.begin(); i != privs_s.end(); i++){
		lua_pushboolean(L, true);
		lua_setfield(L, table, i->c_str());
	}
	lua_pushvalue(L, table);
	return 1;
}

// get_ban_list()
static int l_get_ban_list(lua_State *L)
{
	lua_pushstring(L, get_server(L)->getBanDescription("").c_str());
	return 1;
}

// get_ban_description()
static int l_get_ban_description(lua_State *L)
{
	const char * ip_or_name = luaL_checkstring(L, 1);
	lua_pushstring(L, get_server(L)->getBanDescription(std::string(ip_or_name)).c_str());
	return 1;
}

// ban_player()
static int l_ban_player(lua_State *L)
{
	const char * name = luaL_checkstring(L, 1);
	Player *player = get_env(L)->getPlayer(name);
	if(player == NULL)
	{
		lua_pushboolean(L, false); // no such player
		return 1;
	}
	try
	{
		Address addr = get_server(L)->getPeerAddress(get_env(L)->getPlayer(name)->peer_id);
		std::string ip_str = addr.serializeString();
		get_server(L)->setIpBanned(ip_str, name);
	}
	catch(con::PeerNotFoundException) // unlikely
	{
		dstream << __FUNCTION_NAME << ": peer was not found" << std::endl;
		lua_pushboolean(L, false); // error
		return 1;
	}
	lua_pushboolean(L, true);
	return 1;
}

// unban_player_or_ip()
static int l_unban_player_of_ip(lua_State *L)
{
	const char * ip_or_name = luaL_checkstring(L, 1);
	get_server(L)->unsetIpBanned(ip_or_name);
	lua_pushboolean(L, true);
	return 1;
}

// get_inventory(location)
static int l_get_inventory(lua_State *L)
{
	InventoryLocation loc;

	std::string type = checkstringfield(L, 1, "type");
	if(type == "player"){
		std::string name = checkstringfield(L, 1, "name");
		loc.setPlayer(name);
	} else if(type == "node"){
		lua_getfield(L, 1, "pos");
		v3s16 pos = check_v3s16(L, -1);
		loc.setNodeMeta(pos);
	} else if(type == "detached"){
		std::string name = checkstringfield(L, 1, "name");
		loc.setDetached(name);
	}
	
	if(get_server(L)->getInventory(loc) != NULL)
		InvRef::create(L, loc);
	else
		lua_pushnil(L);
	return 1;
}

// create_detached_inventory_raw(name)
static int l_create_detached_inventory_raw(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	if(get_server(L)->createDetachedInventory(name) != NULL){
		InventoryLocation loc;
		loc.setDetached(name);
		InvRef::create(L, loc);
	}else{
		lua_pushnil(L);
	}
	return 1;
}

// get_dig_params(groups, tool_capabilities[, time_from_last_punch])
static int l_get_dig_params(lua_State *L)
{
	std::map<std::string, int> groups;
	read_groups(L, 1, groups);
	ToolCapabilities tp = read_tool_capabilities(L, 2);
	if(lua_isnoneornil(L, 3))
		push_dig_params(L, getDigParams(groups, &tp));
	else
		push_dig_params(L, getDigParams(groups, &tp,
					luaL_checknumber(L, 3)));
	return 1;
}

// get_hit_params(groups, tool_capabilities[, time_from_last_punch])
static int l_get_hit_params(lua_State *L)
{
	std::map<std::string, int> groups;
	read_groups(L, 1, groups);
	ToolCapabilities tp = read_tool_capabilities(L, 2);
	if(lua_isnoneornil(L, 3))
		push_hit_params(L, getHitParams(groups, &tp));
	else
		push_hit_params(L, getHitParams(groups, &tp,
					luaL_checknumber(L, 3)));
	return 1;
}

// get_current_modname()
static int l_get_current_modname(lua_State *L)
{
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_current_modname");
	return 1;
}

// get_modpath(modname)
static int l_get_modpath(lua_State *L)
{
	std::string modname = luaL_checkstring(L, 1);
	// Do it
	if(modname == "__builtin"){
		std::string path = get_server(L)->getBuiltinLuaPath();
		lua_pushstring(L, path.c_str());
		return 1;
	}
	const ModSpec *mod = get_server(L)->getModSpec(modname);
	if(!mod){
		lua_pushnil(L);
		return 1;
	}
	lua_pushstring(L, mod->path.c_str());
	return 1;
}

// get_modnames()
// the returned list is sorted alphabetically for you
static int l_get_modnames(lua_State *L)
{
	// Get a list of mods
	core::list<std::string> mods_unsorted, mods_sorted;
	get_server(L)->getModNames(mods_unsorted);

	// Take unsorted items from mods_unsorted and sort them into
	// mods_sorted; not great performance but the number of mods on a
	// server will likely be small.
	for(core::list<std::string>::Iterator i = mods_unsorted.begin();
	    i != mods_unsorted.end(); i++)
	{
		bool added = false;
		for(core::list<std::string>::Iterator x = mods_sorted.begin();
		    x != mods_unsorted.end(); x++)
		{
			// I doubt anybody using Minetest will be using
			// anything not ASCII based :)
			if((*i).compare(*x) <= 0)
			{
				mods_sorted.insert_before(x, *i);
				added = true;
				break;
			}
		}
		if(!added)
			mods_sorted.push_back(*i);
	}

	// Get the table insertion function from Lua.
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int insertion_func = lua_gettop(L);

	// Package them up for Lua
	lua_newtable(L);
	int new_table = lua_gettop(L);
	core::list<std::string>::Iterator i = mods_sorted.begin();
	while(i != mods_sorted.end())
	{
		lua_pushvalue(L, insertion_func);
		lua_pushvalue(L, new_table);
		lua_pushstring(L, (*i).c_str());
		if(lua_pcall(L, 2, 0, 0) != 0)
		{
			script_error(L, "error: %s", lua_tostring(L, -1));
		}
		i++;
	}
	return 1;
}

// get_worldpath()
static int l_get_worldpath(lua_State *L)
{
	std::string worldpath = get_server(L)->getWorldPath();
	lua_pushstring(L, worldpath.c_str());
	return 1;
}

// sound_play(spec, parameters)
static int l_sound_play(lua_State *L)
{
	SimpleSoundSpec spec;
	read_soundspec(L, 1, spec);
	ServerSoundParams params;
	read_server_sound_params(L, 2, params);
	s32 handle = get_server(L)->playSound(spec, params);
	lua_pushinteger(L, handle);
	return 1;
}

// sound_stop(handle)
static int l_sound_stop(lua_State *L)
{
	int handle = luaL_checkinteger(L, 1);
	get_server(L)->stopSound(handle);
	return 0;
}

// is_singleplayer()
static int l_is_singleplayer(lua_State *L)
{
	lua_pushboolean(L, get_server(L)->isSingleplayer());
	return 1;
}

// get_password_hash(name, raw_password)
static int l_get_password_hash(lua_State *L)
{
	std::string name = luaL_checkstring(L, 1);
	std::string raw_password = luaL_checkstring(L, 2);
	std::string hash = translatePassword(name,
			narrow_to_wide(raw_password));
	lua_pushstring(L, hash.c_str());
	return 1;
}

// notify_authentication_modified(name)
static int l_notify_authentication_modified(lua_State *L)
{
	std::string name = "";
	if(lua_isstring(L, 1))
		name = lua_tostring(L, 1);
	get_server(L)->reportPrivsModified(name);
	return 0;
}

// get_craft_result(input)
static int l_get_craft_result(lua_State *L)
{
	int input_i = 1;
	std::string method_s = getstringfield_default(L, input_i, "method", "normal");
	enum CraftMethod method = (CraftMethod)getenumfield(L, input_i, "method",
				es_CraftMethod, CRAFT_METHOD_NORMAL);
	int width = 1;
	lua_getfield(L, input_i, "width");
	if(lua_isnumber(L, -1))
		width = luaL_checkinteger(L, -1);
	lua_pop(L, 1);
	lua_getfield(L, input_i, "items");
	std::vector<ItemStack> items = read_items(L, -1);
	lua_pop(L, 1); // items
	
	IGameDef *gdef = get_server(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input(method, width, items);
	CraftOutput output;
	bool got = cdef->getCraftResult(input, output, true, gdef);
	lua_newtable(L); // output table
	if(got){
		ItemStack item;
		item.deSerialize(output.item, gdef->idef());
		LuaItemStack::create(L, item);
		lua_setfield(L, -2, "item");
		setintfield(L, -1, "time", output.time);
	} else {
		LuaItemStack::create(L, ItemStack());
		lua_setfield(L, -2, "item");
		setintfield(L, -1, "time", 0);
	}
	lua_newtable(L); // decremented input table
	lua_pushstring(L, method_s.c_str());
	lua_setfield(L, -2, "method");
	lua_pushinteger(L, width);
	lua_setfield(L, -2, "width");
	push_items(L, input.items);
	lua_setfield(L, -2, "items");
	return 2;
}

// get_craft_recipe(result item)
static int l_get_craft_recipe(lua_State *L)
{
	int k = 0;
	char tmp[20];
	int input_i = 1;
	std::string o_item = luaL_checkstring(L,input_i);
	
	IGameDef *gdef = get_server(L);
	ICraftDefManager *cdef = gdef->cdef();
	CraftInput input;
	CraftOutput output(o_item,0);
	bool got = cdef->getCraftRecipe(input, output, gdef);
	lua_newtable(L); // output table
	if(got){
		lua_newtable(L);
		for(std::vector<ItemStack>::const_iterator
			i = input.items.begin();
			i != input.items.end(); i++, k++)
		{
			if (i->empty())
			{
				continue;
			}
			sprintf(tmp,"%d",k);
			lua_pushstring(L,tmp);
			lua_pushstring(L,i->name.c_str());
			lua_settable(L, -3);
		}
		lua_setfield(L, -2, "items");
		setintfield(L, -1, "width", input.width);
		switch (input.method) {
		case CRAFT_METHOD_NORMAL:
			lua_pushstring(L,"normal");
			break;
		case CRAFT_METHOD_COOKING:
			lua_pushstring(L,"cooking");
			break;
		case CRAFT_METHOD_FUEL:
			lua_pushstring(L,"fuel");
			break;
		default:
			lua_pushstring(L,"unknown");
		}
		lua_setfield(L, -2, "type");
	} else {
		lua_pushnil(L);
		lua_setfield(L, -2, "items");
		setintfield(L, -1, "width", 0);
	}
	return 1;
}

// rollback_get_last_node_actor(p, range, seconds) -> actor, p, seconds
static int l_rollback_get_last_node_actor(lua_State *L)
{
	v3s16 p = read_v3s16(L, 1);
	int range = luaL_checknumber(L, 2);
	int seconds = luaL_checknumber(L, 3);
	Server *server = get_server(L);
	IRollbackManager *rollback = server->getRollbackManager();
	v3s16 act_p;
	int act_seconds = 0;
	std::string actor = rollback->getLastNodeActor(p, range, seconds, &act_p, &act_seconds);
	lua_pushstring(L, actor.c_str());
	push_v3s16(L, act_p);
	lua_pushnumber(L, act_seconds);
	return 3;
}

// rollback_revert_actions_by(actor, seconds) -> bool, log messages
static int l_rollback_revert_actions_by(lua_State *L)
{
	std::string actor = luaL_checkstring(L, 1);
	int seconds = luaL_checknumber(L, 2);
	Server *server = get_server(L);
	IRollbackManager *rollback = server->getRollbackManager();
	std::list<RollbackAction> actions = rollback->getRevertActions(actor, seconds);
	std::list<std::string> log;
	bool success = server->rollbackRevertActions(actions, &log);
	// Push boolean result
	lua_pushboolean(L, success);
	// Get the table insert function and push the log table
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);
	lua_newtable(L);
	int table = lua_gettop(L);
	for(std::list<std::string>::const_iterator i = log.begin();
			i != log.end(); i++)
	{
		lua_pushvalue(L, table_insert);
		lua_pushvalue(L, table);
		lua_pushstring(L, i->c_str());
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
	lua_remove(L, -2); // Remove table
	lua_remove(L, -2); // Remove insert
	return 2;
}

static const struct luaL_Reg minetest_f [] = {
	{"debug", l_debug},
	{"log", l_log},
	{"request_shutdown", l_request_shutdown},
	{"get_server_status", l_get_server_status},
	{"register_item_raw", l_register_item_raw},
	{"register_alias_raw", l_register_alias_raw},
	{"register_craft", l_register_craft},
	{"setting_set", l_setting_set},
	{"setting_get", l_setting_get},
	{"setting_getbool", l_setting_getbool},
	{"chat_send_all", l_chat_send_all},
	{"chat_send_player", l_chat_send_player},
	{"get_player_privs", l_get_player_privs},
	{"get_ban_list", l_get_ban_list},
	{"get_ban_description", l_get_ban_description},
	{"ban_player", l_ban_player},
	{"unban_player_or_ip", l_unban_player_of_ip},
	{"get_inventory", l_get_inventory},
	{"create_detached_inventory_raw", l_create_detached_inventory_raw},
	{"get_dig_params", l_get_dig_params},
	{"get_hit_params", l_get_hit_params},
	{"get_current_modname", l_get_current_modname},
	{"get_modpath", l_get_modpath},
	{"get_modnames", l_get_modnames},
	{"get_worldpath", l_get_worldpath},
	{"sound_play", l_sound_play},
	{"sound_stop", l_sound_stop},
	{"is_singleplayer", l_is_singleplayer},
	{"get_password_hash", l_get_password_hash},
	{"notify_authentication_modified", l_notify_authentication_modified},
	{"get_craft_result", l_get_craft_result},
	{"get_craft_recipe", l_get_craft_recipe},
	{"rollback_get_last_node_actor", l_rollback_get_last_node_actor},
	{"rollback_revert_actions_by", l_rollback_revert_actions_by},
	{NULL, NULL}
};

/*
	Main export function
*/

void scriptapi_export(lua_State *L, Server *server)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	verbosestream<<"scriptapi_export()"<<std::endl;
	StackUnroller stack_unroller(L);

	// Store server as light userdata in registry
	lua_pushlightuserdata(L, server);
	lua_setfield(L, LUA_REGISTRYINDEX, "minetest_server");

	// Register global functions in table minetest
	lua_newtable(L);
	luaL_register(L, NULL, minetest_f);
	lua_setglobal(L, "minetest");
	
	// Get the main minetest table
	lua_getglobal(L, "minetest");

	// Add tables to minetest
	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");
	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");

	// Register wrappers
	LuaItemStack::Register(L);
	InvRef::Register(L);
	NodeMetaRef::Register(L);
	NodeTimerRef::Register(L);
	ObjectRef::Register(L);
	EnvRef::Register(L);
	LuaPseudoRandom::Register(L);
	LuaPerlinNoise::Register(L);
}

bool scriptapi_loadmod(lua_State *L, const std::string &scriptpath,
		const std::string &modname)
{
	ModNameStorer modnamestorer(L, modname);

	if(!string_allowed(modname, "abcdefghijklmnopqrstuvwxyz"
			"0123456789_")){
		errorstream<<"Error loading mod \""<<modname
				<<"\": modname does not follow naming conventions: "
				<<"Only chararacters [a-z0-9_] are allowed."<<std::endl;
		return false;
	}
	
	bool success = false;

	try{
		success = script_load(L, scriptpath.c_str());
	}
	catch(LuaError &e){
		errorstream<<"Error loading mod \""<<modname
				<<"\": "<<e.what()<<std::endl;
	}

	return success;
}

void scriptapi_add_environment(lua_State *L, ServerEnvironment *env)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	verbosestream<<"scriptapi_add_environment"<<std::endl;
	StackUnroller stack_unroller(L);

	// Create EnvRef on stack
	EnvRef::create(L, env);
	int envref = lua_gettop(L);

	// minetest.env = envref
	lua_getglobal(L, "minetest");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushvalue(L, envref);
	lua_setfield(L, -2, "env");

	// Store environment as light userdata in registry
	lua_pushlightuserdata(L, env);
	lua_setfield(L, LUA_REGISTRYINDEX, "minetest_env");

	/*
		Add ActiveBlockModifiers to environment
	*/

	// Get minetest.registered_abms
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_abms");
	luaL_checktype(L, -1, LUA_TTABLE);
	int registered_abms = lua_gettop(L);
	
	if(lua_istable(L, registered_abms)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			int id = lua_tonumber(L, -2);
			int current_abm = lua_gettop(L);

			std::set<std::string> trigger_contents;
			lua_getfield(L, current_abm, "nodenames");
			if(lua_istable(L, -1)){
				int table = lua_gettop(L);
				lua_pushnil(L);
				while(lua_next(L, table) != 0){
					// key at index -2 and value at index -1
					luaL_checktype(L, -1, LUA_TSTRING);
					trigger_contents.insert(lua_tostring(L, -1));
					// removes value, keeps key for next iteration
					lua_pop(L, 1);
				}
			} else if(lua_isstring(L, -1)){
				trigger_contents.insert(lua_tostring(L, -1));
			}
			lua_pop(L, 1);

			std::set<std::string> required_neighbors;
			lua_getfield(L, current_abm, "neighbors");
			if(lua_istable(L, -1)){
				int table = lua_gettop(L);
				lua_pushnil(L);
				while(lua_next(L, table) != 0){
					// key at index -2 and value at index -1
					luaL_checktype(L, -1, LUA_TSTRING);
					required_neighbors.insert(lua_tostring(L, -1));
					// removes value, keeps key for next iteration
					lua_pop(L, 1);
				}
			} else if(lua_isstring(L, -1)){
				required_neighbors.insert(lua_tostring(L, -1));
			}
			lua_pop(L, 1);

			float trigger_interval = 10.0;
			getfloatfield(L, current_abm, "interval", trigger_interval);

			int trigger_chance = 50;
			getintfield(L, current_abm, "chance", trigger_chance);

			LuaABM *abm = new LuaABM(L, id, trigger_contents,
					required_neighbors, trigger_interval, trigger_chance);
			
			env->addActiveBlockModifier(abm);

			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	}
	lua_pop(L, 1);
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

/*
	object_reference
*/

void scriptapi_add_object_reference(lua_State *L, ServerActiveObject *cobj)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_add_object_reference: id="<<cobj->getId()<<std::endl;
	StackUnroller stack_unroller(L);

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

void scriptapi_rm_object_reference(lua_State *L, ServerActiveObject *cobj)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_rm_object_reference: id="<<cobj->getId()<<std::endl;
	StackUnroller stack_unroller(L);

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

/*
	misc
*/

// What scriptapi_run_callbacks does with the return values of callbacks.
// Regardless of the mode, if only one callback is defined,
// its return value is the total return value.
// Modes only affect the case where 0 or >= 2 callbacks are defined.
enum RunCallbacksMode
{
	// Returns the return value of the first callback
	// Returns nil if list of callbacks is empty
	RUN_CALLBACKS_MODE_FIRST,
	// Returns the return value of the last callback
	// Returns nil if list of callbacks is empty
	RUN_CALLBACKS_MODE_LAST,
	// If any callback returns a false value, the first such is returned
	// Otherwise, the first callback's return value (trueish) is returned
	// Returns true if list of callbacks is empty
	RUN_CALLBACKS_MODE_AND,
	// Like above, but stops calling callbacks (short circuit)
	// after seeing the first false value
	RUN_CALLBACKS_MODE_AND_SC,
	// If any callback returns a true value, the first such is returned
	// Otherwise, the first callback's return value (falseish) is returned
	// Returns false if list of callbacks is empty
	RUN_CALLBACKS_MODE_OR,
	// Like above, but stops calling callbacks (short circuit)
	// after seeing the first true value
	RUN_CALLBACKS_MODE_OR_SC,
	// Note: "a true value" and "a false value" refer to values that
	// are converted by lua_toboolean to true or false, respectively.
};

// Push the list of callbacks (a lua table).
// Then push nargs arguments.
// Then call this function, which
// - runs the callbacks
// - removes the table and arguments from the lua stack
// - pushes the return value, computed depending on mode
static void scriptapi_run_callbacks(lua_State *L, int nargs,
		RunCallbacksMode mode)
{
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
			script_error(L, "error: %s", lua_tostring(L, -1));

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
				if(lua_toboolean(L, rv) == true &&
						lua_toboolean(L, -1) == false)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else if(mode == RUN_CALLBACKS_MODE_OR ||
					mode == RUN_CALLBACKS_MODE_OR_SC){
				if(lua_toboolean(L, rv) == false &&
						lua_toboolean(L, -1) == true)
					lua_replace(L, rv);
				else
					lua_pop(L, 1);
			}
			else
				assert(0);
		}

		// Handle short circuit modes
		if(mode == RUN_CALLBACKS_MODE_AND_SC &&
				lua_toboolean(L, rv) == false)
			break;
		else if(mode == RUN_CALLBACKS_MODE_OR_SC &&
				lua_toboolean(L, rv) == true)
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

bool scriptapi_on_chat_message(lua_State *L, const std::string &name,
		const std::string &message)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_chat_messages
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_chat_messages");
	// Call callbacks
	lua_pushstring(L, name.c_str());
	lua_pushstring(L, message.c_str());
	scriptapi_run_callbacks(L, 2, RUN_CALLBACKS_MODE_OR_SC);
	bool ate = lua_toboolean(L, -1);
	return ate;
}

void scriptapi_on_newplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_newplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_newplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void scriptapi_on_dieplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_dieplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_dieplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

bool scriptapi_on_respawnplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_respawnplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_respawnplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_OR);
	bool positioning_handled_by_some = lua_toboolean(L, -1);
	return positioning_handled_by_some;
}

void scriptapi_on_joinplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_joinplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_joinplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void scriptapi_on_leaveplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_leaveplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_leaveplayers");
	// Call callbacks
	objectref_get_or_create(L, player);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

static void get_auth_handler(lua_State *L)
{
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_auth_handler");
	if(lua_isnil(L, -1)){
		lua_pop(L, 1);
		lua_getfield(L, -1, "builtin_auth_handler");
	}
	if(lua_type(L, -1) != LUA_TTABLE)
		throw LuaError(L, "Authentication handler table not valid");
}

bool scriptapi_get_auth(lua_State *L, const std::string &playername,
		std::string *dst_password, std::set<std::string> *dst_privs)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);
	
	get_auth_handler(L);
	lua_getfield(L, -1, "get_auth");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing get_auth");
	lua_pushstring(L, playername.c_str());
	if(lua_pcall(L, 1, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	
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
		read_privileges(L, -1, *dst_privs);
	lua_pop(L, 1);
	
	return true;
}

void scriptapi_create_auth(lua_State *L, const std::string &playername,
		const std::string &password)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);
	
	get_auth_handler(L);
	lua_getfield(L, -1, "create_auth");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing create_auth");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

bool scriptapi_set_password(lua_State *L, const std::string &playername,
		const std::string &password)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);
	
	get_auth_handler(L);
	lua_getfield(L, -1, "set_password");
	if(lua_type(L, -1) != LUA_TFUNCTION)
		throw LuaError(L, "Authentication handler missing set_password");
	lua_pushstring(L, playername.c_str());
	lua_pushstring(L, password.c_str());
	if(lua_pcall(L, 2, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	return lua_toboolean(L, -1);
}

/*
	player
*/

void scriptapi_on_player_receive_fields(lua_State *L, 
		ServerActiveObject *player,
		const std::string &formname,
		const std::map<std::string, std::string> &fields)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_chat_messages
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_player_receive_fields");
	// Call callbacks
	// param 1
	objectref_get_or_create(L, player);
	// param 2
	lua_pushstring(L, formname.c_str());
	// param 3
	lua_newtable(L);
	for(std::map<std::string, std::string>::const_iterator
			i = fields.begin(); i != fields.end(); i++){
		const std::string &name = i->first;
		const std::string &value = i->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}
	scriptapi_run_callbacks(L, 3, RUN_CALLBACKS_MODE_OR_SC);
}

/*
	item callbacks and node callbacks
*/

// Retrieves minetest.registered_items[name][callbackname]
// If that is nil or on error, return false and stack is unchanged
// If that is a function, returns true and pushes the
// function onto the stack
// If minetest.registered_items[name] doesn't exist, minetest.nodedef_default
// is tried instead so unknown items can still be manipulated to some degree
static bool get_item_callback(lua_State *L,
		const char *name, const char *callbackname)
{
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_items");
	lua_remove(L, -2);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, name);
	lua_remove(L, -2);
	// Should be a table
	if(lua_type(L, -1) != LUA_TTABLE)
	{
		// Report error and clean up
		errorstream<<"Item \""<<name<<"\" not defined"<<std::endl;
		lua_pop(L, 1);

		// Try minetest.nodedef_default instead
		lua_getglobal(L, "minetest");
		lua_getfield(L, -1, "nodedef_default");
		lua_remove(L, -2);
		luaL_checktype(L, -1, LUA_TTABLE);
	}
	lua_getfield(L, -1, callbackname);
	lua_remove(L, -2);
	// Should be a function or nil
	if(lua_type(L, -1) == LUA_TFUNCTION)
	{
		return true;
	}
	else if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}
	else
	{
		errorstream<<"Item \""<<name<<"\" callback \""
			<<callbackname<<" is not a function"<<std::endl;
		lua_pop(L, 1);
		return false;
	}
}

bool scriptapi_item_on_drop(lua_State *L, ItemStack &item,
		ServerActiveObject *dropper, v3f pos)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_item_callback(L, item.name.c_str(), "on_drop"))
		return false;

	// Call function
	LuaItemStack::create(L, item);
	objectref_get_or_create(L, dropper);
	pushFloatPos(L, pos);
	if(lua_pcall(L, 3, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnil(L, -1))
		item = read_item(L, -1);
	return true;
}

bool scriptapi_item_on_place(lua_State *L, ItemStack &item,
		ServerActiveObject *placer, const PointedThing &pointed)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_item_callback(L, item.name.c_str(), "on_place"))
		return false;

	// Call function
	LuaItemStack::create(L, item);
	objectref_get_or_create(L, placer);
	push_pointed_thing(L, pointed);
	if(lua_pcall(L, 3, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnil(L, -1))
		item = read_item(L, -1);
	return true;
}

bool scriptapi_item_on_use(lua_State *L, ItemStack &item,
		ServerActiveObject *user, const PointedThing &pointed)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_item_callback(L, item.name.c_str(), "on_use"))
		return false;

	// Call function
	LuaItemStack::create(L, item);
	objectref_get_or_create(L, user);
	push_pointed_thing(L, pointed);
	if(lua_pcall(L, 3, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnil(L, -1))
		item = read_item(L, -1);
	return true;
}

bool scriptapi_node_on_punch(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *puncher)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_punch"))
		return false;

	// Call function
	push_v3s16(L, p);
	pushnode(L, node, ndef);
	objectref_get_or_create(L, puncher);
	if(lua_pcall(L, 3, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	return true;
}

bool scriptapi_node_on_dig(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *digger)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_dig"))
		return false;

	// Call function
	push_v3s16(L, p);
	pushnode(L, node, ndef);
	objectref_get_or_create(L, digger);
	if(lua_pcall(L, 3, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	return true;
}

void scriptapi_node_on_construct(lua_State *L, v3s16 p, MapNode node)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_construct"))
		return;

	// Call function
	push_v3s16(L, p);
	if(lua_pcall(L, 1, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

void scriptapi_node_on_destruct(lua_State *L, v3s16 p, MapNode node)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_destruct"))
		return;

	// Call function
	push_v3s16(L, p);
	if(lua_pcall(L, 1, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

void scriptapi_node_after_destruct(lua_State *L, v3s16 p, MapNode node)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "after_destruct"))
		return;

	// Call function
	push_v3s16(L, p);
	pushnode(L, node, ndef);
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

bool scriptapi_node_on_timer(lua_State *L, v3s16 p, MapNode node, f32 dtime)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_timer"))
		return false;

	// Call function
	push_v3s16(L, p);
	lua_pushnumber(L,dtime);
	if(lua_pcall(L, 2, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(lua_isboolean(L,-1) && lua_toboolean(L,-1) == true)
		return true;
	
	return false;
}

void scriptapi_node_on_receive_fields(lua_State *L, v3s16 p,
		const std::string &formname,
		const std::map<std::string, std::string> &fields,
		ServerActiveObject *sender)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();
	
	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(), "on_receive_fields"))
		return;

	// Call function
	// param 1
	push_v3s16(L, p);
	// param 2
	lua_pushstring(L, formname.c_str());
	// param 3
	lua_newtable(L);
	for(std::map<std::string, std::string>::const_iterator
			i = fields.begin(); i != fields.end(); i++){
		const std::string &name = i->first;
		const std::string &value = i->second;
		lua_pushstring(L, name.c_str());
		lua_pushlstring(L, value.c_str(), value.size());
		lua_settable(L, -3);
	}
	// param 4
	objectref_get_or_create(L, sender);
	if(lua_pcall(L, 4, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

/*
	Node metadata inventory callbacks
*/

// Return number of accepted items to be moved
int scriptapi_nodemeta_inventory_allow_move(lua_State *L, v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"allow_metadata_inventory_move"))
		return count;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	// pos
	push_v3s16(L, p);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_move should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be put
int scriptapi_nodemeta_inventory_allow_put(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"allow_metadata_inventory_put"))
		return stack.count;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_put should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be taken
int scriptapi_nodemeta_inventory_allow_take(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return 0;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"allow_metadata_inventory_take"))
		return stack.count;

	// Call function(pos, listname, index, count, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_metadata_inventory_take should return a number");
	return luaL_checkinteger(L, -1);
}

// Report moved items
void scriptapi_nodemeta_inventory_on_move(lua_State *L, v3s16 p,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"on_metadata_inventory_move"))
		return;

	// function(pos, from_list, from_index, to_list, to_index, count, player)
	// pos
	push_v3s16(L, p);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report put items
void scriptapi_nodemeta_inventory_on_put(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"on_metadata_inventory_put"))
		return;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report taken items
void scriptapi_nodemeta_inventory_on_take(lua_State *L, v3s16 p,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	INodeDefManager *ndef = get_server(L)->ndef();

	// If node doesn't exist, we don't know what callback to call
	MapNode node = get_env(L)->getMap().getNodeNoEx(p);
	if(node.getContent() == CONTENT_IGNORE)
		return;

	// Push callback function on stack
	if(!get_item_callback(L, ndef->get(node).name.c_str(),
			"on_metadata_inventory_take"))
		return;

	// Call function(pos, listname, index, stack, player)
	// pos
	push_v3s16(L, p);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

/*
	Detached inventory callbacks
*/

// Retrieves minetest.detached_inventories[name][callbackname]
// If that is nil or on error, return false and stack is unchanged
// If that is a function, returns true and pushes the
// function onto the stack
static bool get_detached_inventory_callback(lua_State *L,
		const std::string &name, const char *callbackname)
{
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "detached_inventories");
	lua_remove(L, -2);
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, name.c_str());
	lua_remove(L, -2);
	// Should be a table
	if(lua_type(L, -1) != LUA_TTABLE)
	{
		errorstream<<"Item \""<<name<<"\" not defined"<<std::endl;
		lua_pop(L, 1);
		return false;
	}
	lua_getfield(L, -1, callbackname);
	lua_remove(L, -2);
	// Should be a function or nil
	if(lua_type(L, -1) == LUA_TFUNCTION)
	{
		return true;
	}
	else if(lua_isnil(L, -1))
	{
		lua_pop(L, 1);
		return false;
	}
	else
	{
		errorstream<<"Detached inventory \""<<name<<"\" callback \""
			<<callbackname<<"\" is not a function"<<std::endl;
		lua_pop(L, 1);
		return false;
	}
}

// Return number of accepted items to be moved
int scriptapi_detached_inventory_allow_move(lua_State *L,
		const std::string &name,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "allow_move"))
		return count;

	// function(inv, from_list, from_index, to_list, to_index, count, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_move should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be put
int scriptapi_detached_inventory_allow_put(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "allow_put"))
		return stack.count; // All will be accepted

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_put should return a number");
	return luaL_checkinteger(L, -1);
}

// Return number of accepted items to be taken
int scriptapi_detached_inventory_allow_take(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "allow_take"))
		return stack.count; // All will be accepted

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 1, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
	if(!lua_isnumber(L, -1))
		throw LuaError(L, "allow_take should return a number");
	return luaL_checkinteger(L, -1);
}

// Report moved items
void scriptapi_detached_inventory_on_move(lua_State *L,
		const std::string &name,
		const std::string &from_list, int from_index,
		const std::string &to_list, int to_index,
		int count, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "on_move"))
		return;

	// function(inv, from_list, from_index, to_list, to_index, count, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// from_list
	lua_pushstring(L, from_list.c_str());
	// from_index
	lua_pushinteger(L, from_index + 1);
	// to_list
	lua_pushstring(L, to_list.c_str());
	// to_index
	lua_pushinteger(L, to_index + 1);
	// count
	lua_pushinteger(L, count);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 7, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report put items
void scriptapi_detached_inventory_on_put(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "on_put"))
		return;

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

// Report taken items
void scriptapi_detached_inventory_on_take(lua_State *L,
		const std::string &name,
		const std::string &listname, int index, ItemStack &stack,
		ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Push callback function on stack
	if(!get_detached_inventory_callback(L, name, "on_take"))
		return;

	// Call function(inv, listname, index, stack, player)
	// inv
	InventoryLocation loc;
	loc.setDetached(name);
	InvRef::create(L, loc);
	// listname
	lua_pushstring(L, listname.c_str());
	// index
	lua_pushinteger(L, index + 1);
	// stack
	LuaItemStack::create(L, stack);
	// player
	objectref_get_or_create(L, player);
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error: %s", lua_tostring(L, -1));
}

/*
	environment
*/

void scriptapi_environment_step(lua_State *L, float dtime)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_step"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.registered_globalsteps
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_globalsteps");
	// Call callbacks
	lua_pushnumber(L, dtime);
	scriptapi_run_callbacks(L, 1, RUN_CALLBACKS_MODE_FIRST);
}

void scriptapi_environment_on_generated(lua_State *L, v3s16 minp, v3s16 maxp,
		u32 blockseed)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_on_generated"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_generateds
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_generateds");
	// Call callbacks
	push_v3s16(L, minp);
	push_v3s16(L, maxp);
	lua_pushnumber(L, blockseed);
	scriptapi_run_callbacks(L, 3, RUN_CALLBACKS_MODE_FIRST);
}

/*
	luaentity
*/

bool scriptapi_luaentity_add(lua_State *L, u16 id, const char *name)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	verbosestream<<"scriptapi_luaentity_add: id="<<id<<" name=\""
			<<name<<"\""<<std::endl;
	StackUnroller stack_unroller(L);
	
	// Get minetest.registered_entities[name]
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_entities");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushstring(L, name);
	lua_gettable(L, -2);
	// Should be a table, which we will use as a prototype
	//luaL_checktype(L, -1, LUA_TTABLE);
	if(lua_type(L, -1) != LUA_TTABLE){
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
	objectref_get(L, id);
	luaL_checktype(L, -1, LUA_TUSERDATA);
	if(!luaL_checkudata(L, -1, "ObjectRef"))
		luaL_typerror(L, -1, "ObjectRef");
	lua_setfield(L, -2, "object");

	// minetest.luaentities[id] = object
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushnumber(L, id); // Push id
	lua_pushvalue(L, object); // Copy object to top of stack
	lua_settable(L, -3);
	
	return true;
}

void scriptapi_luaentity_activate(lua_State *L, u16 id,
		const std::string &staticdata, u32 dtime_s)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	verbosestream<<"scriptapi_luaentity_activate: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);
	
	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	
	// Get on_activate function
	lua_pushvalue(L, object);
	lua_getfield(L, -1, "on_activate");
	if(!lua_isnil(L, -1)){
		luaL_checktype(L, -1, LUA_TFUNCTION);
		lua_pushvalue(L, object); // self
		lua_pushlstring(L, staticdata.c_str(), staticdata.size());
		lua_pushinteger(L, dtime_s);
		// Call with 3 arguments, 0 results
		if(lua_pcall(L, 3, 0, 0))
			script_error(L, "error running function on_activate: %s\n",
					lua_tostring(L, -1));
	}
}

void scriptapi_luaentity_rm(lua_State *L, u16 id)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	verbosestream<<"scriptapi_luaentity_rm: id="<<id<<std::endl;

	// Get minetest.luaentities table
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "luaentities");
	luaL_checktype(L, -1, LUA_TTABLE);
	int objectstable = lua_gettop(L);
	
	// Set luaentities[id] = nil
	lua_pushnumber(L, id); // Push id
	lua_pushnil(L);
	lua_settable(L, objectstable);
	
	lua_pop(L, 2); // pop luaentities, minetest
}

std::string scriptapi_luaentity_get_staticdata(lua_State *L, u16 id)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_luaentity_get_staticdata: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	
	// Get get_staticdata function
	lua_pushvalue(L, object);
	lua_getfield(L, -1, "get_staticdata");
	if(lua_isnil(L, -1))
		return "";
	
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	// Call with 1 arguments, 1 results
	if(lua_pcall(L, 1, 1, 0))
		script_error(L, "error running function get_staticdata: %s\n",
				lua_tostring(L, -1));
	
	size_t len=0;
	const char *s = lua_tolstring(L, -1, &len);
	return std::string(s, len);
}

void scriptapi_luaentity_get_properties(lua_State *L, u16 id,
		ObjectProperties *prop)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_luaentity_get_properties: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	//int object = lua_gettop(L);

	// Set default values that differ from ObjectProperties defaults
	prop->hp_max = 10;
	
	/* Read stuff */
	
	prop->hp_max = getintfield_default(L, -1, "hp_max", 10);

	getboolfield(L, -1, "physical", prop->physical);

	getfloatfield(L, -1, "weight", prop->weight);

	lua_getfield(L, -1, "collisionbox");
	if(lua_istable(L, -1))
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

void scriptapi_luaentity_step(lua_State *L, u16 id, float dtime)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get step function
	lua_getfield(L, -1, "on_step");
	if(lua_isnil(L, -1))
		return;
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	lua_pushnumber(L, dtime); // dtime
	// Call with 2 arguments, 0 results
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error running function 'on_step': %s\n", lua_tostring(L, -1));
}

// Calls entity:on_punch(ObjectRef puncher, time_from_last_punch,
//                       tool_capabilities, direction)
void scriptapi_luaentity_punch(lua_State *L, u16 id,
		ServerActiveObject *puncher, float time_from_last_punch,
		const ToolCapabilities *toolcap, v3f dir)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get function
	lua_getfield(L, -1, "on_punch");
	if(lua_isnil(L, -1))
		return;
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	objectref_get_or_create(L, puncher); // Clicker reference
	lua_pushnumber(L, time_from_last_punch);
	push_tool_capabilities(L, *toolcap);
	push_v3f(L, dir);
	// Call with 5 arguments, 0 results
	if(lua_pcall(L, 5, 0, 0))
		script_error(L, "error running function 'on_punch': %s\n", lua_tostring(L, -1));
}

// Calls entity:on_rightclick(ObjectRef clicker)
void scriptapi_luaentity_rightclick(lua_State *L, u16 id,
		ServerActiveObject *clicker)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_luaentity_step: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	int object = lua_gettop(L);
	// State: object is at top of stack
	// Get function
	lua_getfield(L, -1, "on_rightclick");
	if(lua_isnil(L, -1))
		return;
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_pushvalue(L, object); // self
	objectref_get_or_create(L, clicker); // Clicker reference
	// Call with 2 arguments, 0 results
	if(lua_pcall(L, 2, 0, 0))
		script_error(L, "error running function 'on_rightclick': %s\n", lua_tostring(L, -1));
}

