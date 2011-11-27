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
#include "luaentity_common.h"
#include "content_sao.h" // For LuaEntitySAO
#include "tooldef.h"
#include "nodedef.h"
#include "craftdef.h"
#include "main.h" // For g_settings
#include "settings.h" // For accessing g_settings

/*
TODO:
- Random node triggers (like grass growth)
- All kinds of callbacks
- Object visual client-side stuff
	- Blink effect
	- Spritesheets and animation
- LuaNodeMetadata
	blockdef.metadata_name =
		""
		"sign"
		"furnace"
		"chest"
		"locked_chest"
		"lua"
	- Stores an inventory and stuff in a Settings object
	meta.inventory_add_list("main")
	blockdef.on_inventory_modified
	meta.set("owner", playername)
	meta.get("owner")
- Item definition (actually, only CraftItem)
- (not scripting) Putting items in node metadata (virtual)
*/

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

static v3f readFloatPos(lua_State *L, int index)
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
	pos *= BS; // Scale to internal format
	return pos;
}

static void pushFloatPos(lua_State *L, v3f p)
{
	p /= BS;
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, p.Z);
	lua_setfield(L, -2, "z");
}

static void pushpos(lua_State *L, v3s16 p)
{
	lua_newtable(L);
	lua_pushnumber(L, p.X);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, p.Y);
	lua_setfield(L, -2, "y");
	lua_pushnumber(L, p.Z);
	lua_setfield(L, -2, "z");
}

static v3s16 readpos(lua_State *L, int index)
{
	// Correct rounding at <0
	v3f pf = readFloatPos(L, index);
	return floatToInt(pf, BS);
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
	const char *name = lua_tostring(L, -1);
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

static core::aabbox3d<f32> read_aabbox3df32(lua_State *L, int index, f32 scale)
{
	core::aabbox3d<f32> box;
	if(lua_istable(L, -1)){
		lua_rawgeti(L, -1, 1);
		box.MinEdge.X = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 2);
		box.MinEdge.Y = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 3);
		box.MinEdge.Z = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 4);
		box.MaxEdge.X = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 5);
		box.MaxEdge.Y = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
		lua_rawgeti(L, -1, 6);
		box.MaxEdge.Z = lua_tonumber(L, -1) * scale;
		lua_pop(L, 1);
	}
	return box;
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

static bool getstringfield(lua_State *L, int table,
		const char *fieldname, std::string &result)
{
	lua_getfield(L, table, fieldname);
	bool got = false;
	if(lua_isstring(L, -1)){
		result = lua_tostring(L, -1);
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

/*static float getfloatfield_default(lua_State *L, int table,
		const char *fieldname, float default_)
{
	float result = default_;
	getfloatfield(L, table, fieldname, result);
	return result;
}*/

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
	{0, NULL},
};

struct EnumString es_ContentParamType[] =
{
	{CPT_NONE, "none"},
	{CPT_LIGHT, "light"},
	{CPT_MINERAL, "mineral"},
	{CPT_FACEDIR_SIMPLE, "facedir_simple"},
};

struct EnumString es_LiquidType[] =
{
	{LIQUID_NONE, "none"},
	{LIQUID_FLOWING, "flowing"},
	{LIQUID_SOURCE, "source"},
};

struct EnumString es_NodeBoxType[] =
{
	{NODEBOX_REGULAR, "regular"},
	{NODEBOX_FIXED, "fixed"},
	{NODEBOX_WALLMOUNTED, "wallmounted"},
};

struct EnumString es_Diggability[] =
{
	{DIGGABLE_NOT, "not"},
	{DIGGABLE_NORMAL, "normal"},
	{DIGGABLE_CONSTANT, "constant"},
};

/*
	Global functions
*/

static int l_register_nodedef_defaults(lua_State *L)
{
	luaL_checktype(L, 1, LUA_TTABLE);

	lua_pushvalue(L, 1); // Explicitly put parameter 1 on top of stack
	lua_setfield(L, LUA_REGISTRYINDEX, "minetest_nodedef_default");

	return 0;
}

// Register new object prototype
// register_entity(name, prototype)
static int l_register_entity(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	infostream<<"register_entity: "<<name<<std::endl;
	luaL_checktype(L, 2, LUA_TTABLE);

	// Get minetest.registered_entities
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_entities");
	luaL_checktype(L, -1, LUA_TTABLE);
	int registered_entities = lua_gettop(L);
	lua_pushvalue(L, 2); // Object = param 2 -> stack top
	// registered_entities[name] = object
	lua_setfield(L, registered_entities, name);
	
	// Get registered object to top of stack
	lua_pushvalue(L, 2);
	
	// Set __index to point to itself
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	// Set metatable.__index = metatable
	luaL_getmetatable(L, "minetest.entity");
	lua_pushvalue(L, -1); // duplicate metatable
	lua_setfield(L, -2, "__index");
	// Set object metatable
	lua_setmetatable(L, -2);

	return 0; /* number of results */
}

// register_tool(name, {lots of stuff})
static int l_register_tool(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	infostream<<"register_tool: "<<name<<std::endl;
	luaL_checktype(L, 2, LUA_TTABLE);
	int table = 2;

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// And get the writable tool definition manager from the server
	IWritableToolDefManager *tooldef =
			server->getWritableToolDefManager();
	
	ToolDefinition def;
	
	getstringfield(L, table, "image", def.imagename);
	getfloatfield(L, table, "basetime", def.properties.basetime);
	getfloatfield(L, table, "dt_weight", def.properties.dt_weight);
	getfloatfield(L, table, "dt_crackiness", def.properties.dt_crackiness);
	getfloatfield(L, table, "dt_crumbliness", def.properties.dt_crumbliness);
	getfloatfield(L, table, "dt_cuttability", def.properties.dt_cuttability);
	getfloatfield(L, table, "basedurability", def.properties.basedurability);
	getfloatfield(L, table, "dd_weight", def.properties.dd_weight);
	getfloatfield(L, table, "dd_crackiness", def.properties.dd_crackiness);
	getfloatfield(L, table, "dd_crumbliness", def.properties.dd_crumbliness);
	getfloatfield(L, table, "dd_cuttability", def.properties.dd_cuttability);

	tooldef->registerTool(name, def);
	return 0; /* number of results */
}

// register_node(name, {lots of stuff})
static int l_register_node(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);
	infostream<<"register_node: "<<name<<std::endl;
	luaL_checktype(L, 2, LUA_TTABLE);
	int nodedef_table = 2;

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// And get the writable node definition manager from the server
	IWritableNodeDefManager *nodedef =
			server->getWritableNodeDefManager();
	
	// Get default node definition from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_nodedef_default");
	int nodedef_default = lua_gettop(L);

	/*
		Add to minetest.registered_nodes with default as metatable
	*/
	
	// Get the node definition table given as parameter
	lua_pushvalue(L, nodedef_table);

	// Set __index to point to itself
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");

	// Set nodedef_default as metatable for the definition
	lua_pushvalue(L, nodedef_default);
	lua_setmetatable(L, nodedef_table);
	
	// minetest.registered_nodes[name] = nodedef
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_nodes");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushstring(L, name);
	lua_pushvalue(L, nodedef_table);
	lua_settable(L, -3);

	/*
		Create definition
	*/
	
	ContentFeatures f;

	// Default to getting the corresponding NodeItem when dug
	f.dug_item = std::string("NodeItem \"")+name+"\" 1";
	
	// Default to unknown_block.png as all textures
	f.setAllTextures("unknown_block.png");

	/*
		Read definiton from Lua
	*/

	f.name = name;
	
	/* Visual definition */

	f.drawtype = (NodeDrawType)getenumfield(L, nodedef_table, "drawtype", es_DrawType,
			NDT_NORMAL);
	getfloatfield(L, nodedef_table, "visual_scale", f.visual_scale);

	lua_getfield(L, nodedef_table, "tile_images");
	if(lua_istable(L, -1)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			if(lua_isstring(L, -1))
				f.tname_tiles[i] = lua_tostring(L, -1);
			else
				f.tname_tiles[i] = "";
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
			std::string lastname = f.tname_tiles[i-1];
			while(i < 6){
				f.tname_tiles[i] = lastname;
				i++;
			}
		}
	}
	lua_pop(L, 1);

	getstringfield(L, nodedef_table, "inventory_image", f.tname_inventory);

	lua_getfield(L, nodedef_table, "special_materials");
	if(lua_istable(L, -1)){
		int table = lua_gettop(L);
		lua_pushnil(L);
		int i = 0;
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			int smtable = lua_gettop(L);
			std::string tname = getstringfield_default(
					L, smtable, "image", "");
			bool backface_culling = getboolfield_default(
					L, smtable, "backface_culling", true);
			MaterialSpec mspec(tname, backface_culling);
			f.setSpecialMaterial(i, mspec);
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

	f.alpha = getintfield_default(L, nodedef_table, "alpha", 255);

	/* Other stuff */
	
	lua_getfield(L, nodedef_table, "post_effect_color");
	if(!lua_isnil(L, -1))
		f.post_effect_color = readARGB8(L, -1);
	lua_pop(L, 1);

	f.param_type = (ContentParamType)getenumfield(L, nodedef_table, "paramtype",
			es_ContentParamType, CPT_NONE);
	
	// True for all ground-like things like stone and mud, false for eg. trees
	getboolfield(L, nodedef_table, "is_ground_content", f.is_ground_content);
	getboolfield(L, nodedef_table, "light_propagates", f.light_propagates);
	getboolfield(L, nodedef_table, "sunlight_propagates", f.sunlight_propagates);
	// This is used for collision detection.
	// Also for general solidness queries.
	getboolfield(L, nodedef_table, "walkable", f.walkable);
	// Player can point to these
	getboolfield(L, nodedef_table, "pointable", f.pointable);
	// Player can dig these
	getboolfield(L, nodedef_table, "diggable", f.diggable);
	// Player can climb these
	getboolfield(L, nodedef_table, "climbable", f.climbable);
	// Player can build on these
	getboolfield(L, nodedef_table, "buildable_to", f.buildable_to);
	// If true, param2 is set to direction when placed. Used for torches.
	// NOTE: the direction format is quite inefficient and should be changed
	getboolfield(L, nodedef_table, "wall_mounted", f.wall_mounted);
	// Whether this content type often contains mineral.
	// Used for texture atlas creation.
	// Currently only enabled for CONTENT_STONE.
	getboolfield(L, nodedef_table, "often_contains_mineral", f.often_contains_mineral);
	// Inventory item string as which the node appears in inventory when dug.
	// Mineral overrides this.
	getstringfield(L, nodedef_table, "dug_item", f.dug_item);
	// Extra dug item and its rarity
	getstringfield(L, nodedef_table, "extra_dug_item", f.extra_dug_item);
	// Usual get interval for extra dug item
	getintfield(L, nodedef_table, "extra_dug_item_rarity", f.extra_dug_item_rarity);
	// Metadata name of node (eg. "furnace")
	getstringfield(L, nodedef_table, "metadata_name", f.metadata_name);
	// Whether the node is non-liquid, source liquid or flowing liquid
	f.liquid_type = (LiquidType)getenumfield(L, nodedef_table, "liquidtype",
			es_LiquidType, LIQUID_NONE);
	// If the content is liquid, this is the flowing version of the liquid.
	getstringfield(L, nodedef_table, "liquid_alternative_flowing",
			f.liquid_alternative_flowing);
	// If the content is liquid, this is the source version of the liquid.
	getstringfield(L, nodedef_table, "liquid_alternative_source",
			f.liquid_alternative_source);
	// Viscosity for fluid flow, ranging from 1 to 7, with
	// 1 giving almost instantaneous propagation and 7 being
	// the slowest possible
	f.liquid_viscosity = getintfield_default(L, nodedef_table,
			"liquid_viscosity", f.liquid_viscosity);
	// Amount of light the node emits
	f.light_source = getintfield_default(L, nodedef_table,
			"light_source", f.light_source);
	f.damage_per_second = getintfield_default(L, nodedef_table,
			"damage_per_second", f.damage_per_second);
	
	lua_getfield(L, nodedef_table, "selection_box");
	if(lua_istable(L, -1)){
		f.selection_box.type = (NodeBoxType)getenumfield(L, -1, "type",
				es_NodeBoxType, NODEBOX_REGULAR);

		lua_getfield(L, -1, "fixed");
		if(lua_istable(L, -1))
			f.selection_box.fixed = read_aabbox3df32(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, -1, "wall_top");
		if(lua_istable(L, -1))
			f.selection_box.wall_top = read_aabbox3df32(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, -1, "wall_bottom");
		if(lua_istable(L, -1))
			f.selection_box.wall_bottom = read_aabbox3df32(L, -1, BS);
		lua_pop(L, 1);

		lua_getfield(L, -1, "wall_side");
		if(lua_istable(L, -1))
			f.selection_box.wall_side = read_aabbox3df32(L, -1, BS);
		lua_pop(L, 1);
	}
	lua_pop(L, 1);

	lua_getfield(L, nodedef_table, "material");
	if(lua_istable(L, -1)){
		f.material.diggability = (Diggability)getenumfield(L, -1, "diggability",
				es_Diggability, DIGGABLE_NORMAL);
		
		getfloatfield(L, -1, "constant_time", f.material.constant_time);
		getfloatfield(L, -1, "weight", f.material.weight);
		getfloatfield(L, -1, "crackiness", f.material.crackiness);
		getfloatfield(L, -1, "crumbliness", f.material.crumbliness);
		getfloatfield(L, -1, "cuttability", f.material.cuttability);
		getfloatfield(L, -1, "flammability", f.material.flammability);
	}
	lua_pop(L, 1);

	getstringfield(L, nodedef_table, "cookresult_item", f.cookresult_item);
	getfloatfield(L, nodedef_table, "furnace_cooktime", f.furnace_cooktime);
	getfloatfield(L, nodedef_table, "furnace_burntime", f.furnace_burntime);
	
	/*
		Register it
	*/
	
	nodedef->set(name, f);
	
	return 0; /* number of results */
}

// register_craft({output=item, recipe={{item00,item10},{item01,item11}})
static int l_register_craft(lua_State *L)
{
	infostream<<"register_craft"<<std::endl;
	luaL_checktype(L, 1, LUA_TTABLE);
	int table0 = 1;

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// And get the writable craft definition manager from the server
	IWritableCraftDefManager *craftdef =
			server->getWritableCraftDefManager();
	
	std::string output;
	int width = 0;
	std::vector<std::string> input;

	lua_getfield(L, table0, "output");
	luaL_checktype(L, -1, LUA_TSTRING);
	if(lua_isstring(L, -1))
		output = lua_tostring(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, table0, "recipe");
	luaL_checktype(L, -1, LUA_TTABLE);
	if(lua_istable(L, -1)){
		int table1 = lua_gettop(L);
		lua_pushnil(L);
		int rowcount = 0;
		while(lua_next(L, table1) != 0){
			int colcount = 0;
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TTABLE);
			if(lua_istable(L, -1)){
				int table2 = lua_gettop(L);
				lua_pushnil(L);
				while(lua_next(L, table2) != 0){
					// key at index -2 and value at index -1
					luaL_checktype(L, -1, LUA_TSTRING);
					input.push_back(lua_tostring(L, -1));
					// removes value, keeps key for next iteration
					lua_pop(L, 1);
					colcount++;
				}
			}
			if(rowcount == 0){
				width = colcount;
			} else {
				if(colcount != width){
					script_error(L, "error: %s\n", "Invalid crafting recipe");
				}
			}
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
			rowcount++;
		}
	}
	lua_pop(L, 1);

	CraftDefinition def(output, width, input);
	craftdef->registerCraft(def);

	return 0; /* number of results */
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
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
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
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// Send
	server->notifyPlayer(name, narrow_to_wide(text));
	return 0;
}

static const struct luaL_Reg minetest_f [] = {
	{"register_nodedef_defaults", l_register_nodedef_defaults},
	{"register_entity", l_register_entity},
	{"register_tool", l_register_tool},
	{"register_node", l_register_node},
	{"register_craft", l_register_craft},
	{"setting_get", l_setting_get},
	{"setting_getbool", l_setting_getbool},
	{"chat_send_all", l_chat_send_all},
	{"chat_send_player", l_chat_send_player},
	{NULL, NULL}
};

/*
	LuaEntity functions
*/

static const struct luaL_Reg minetest_entity_m [] = {
	{NULL, NULL}
};

/*
	Getters for stuff in main tables
*/

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
	Reference objects
*/
#define method(class, name) {#name, class::l_##name}

class EnvRef
{
private:
	ServerEnvironment *m_env;

	static const char className[];
	static const luaL_reg methods[];

	static EnvRef *checkobject(lua_State *L, int narg)
	{
		luaL_checktype(L, narg, LUA_TUSERDATA);
		void *ud = luaL_checkudata(L, narg, className);
		if(!ud) luaL_typerror(L, narg, className);
		return *(EnvRef**)ud;  // unbox pointer
	}
	
	// Exported functions

	// EnvRef:add_node(pos, node)
	// pos = {x=num, y=num, z=num}
	static int l_add_node(lua_State *L)
	{
		infostream<<"EnvRef::l_add_node()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3s16 pos = readpos(L, 2);
		// content
		MapNode n = readnode(L, 3, env->getGameDef()->ndef());
		// Do it
		bool succeeded = env->getMap().addNodeWithEvent(pos, n);
		lua_pushboolean(L, succeeded);
		return 1;
	}

	// EnvRef:remove_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_remove_node(lua_State *L)
	{
		infostream<<"EnvRef::l_remove_node()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3s16 pos = readpos(L, 2);
		// Do it
		bool succeeded = env->getMap().removeNodeWithEvent(pos);
		lua_pushboolean(L, succeeded);
		return 1;
	}

	// EnvRef:get_node(pos)
	// pos = {x=num, y=num, z=num}
	static int l_get_node(lua_State *L)
	{
		infostream<<"EnvRef::l_get_node()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3s16 pos = readpos(L, 2);
		// Do it
		MapNode n = env->getMap().getNodeNoEx(pos);
		// Return node
		pushnode(L, n, env->getGameDef()->ndef());
		return 1;
	}

	// EnvRef:add_luaentity(pos, entityname)
	// pos = {x=num, y=num, z=num}
	static int l_add_luaentity(lua_State *L)
	{
		infostream<<"EnvRef::l_add_luaentity()"<<std::endl;
		EnvRef *o = checkobject(L, 1);
		ServerEnvironment *env = o->m_env;
		if(env == NULL) return 0;
		// pos
		v3f pos = readFloatPos(L, 2);
		// content
		const char *name = lua_tostring(L, 3);
		// Do it
		ServerActiveObject *obj = new LuaEntitySAO(env, pos, name, "");
		env->addActiveObject(obj);
		return 0;
	}

	static int gc_object(lua_State *L) {
		EnvRef *o = *(EnvRef **)(lua_touserdata(L, 1));
		delete o;
		return 0;
	}

public:
	EnvRef(ServerEnvironment *env):
		m_env(env)
	{
		infostream<<"EnvRef created"<<std::endl;
	}

	~EnvRef()
	{
		infostream<<"EnvRef destructing"<<std::endl;
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
	method(EnvRef, add_node),
	method(EnvRef, remove_node),
	method(EnvRef, get_node),
	method(EnvRef, add_luaentity),
	{0,0}
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
	
	static ServerActiveObject* getobject(ObjectRef *ref)
	{
		ServerActiveObject *co = ref->m_object;
		return co;
	}
	
	static LuaEntitySAO* getluaobject(ObjectRef *ref)
	{
		ServerActiveObject *obj = getobject(ref);
		if(obj == NULL)
			return NULL;
		if(obj->getType() != ACTIVEOBJECT_TYPE_LUAENTITY)
			return NULL;
		return (LuaEntitySAO*)obj;
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
		infostream<<"ObjectRef::l_remove(): id="<<co->getId()<<std::endl;
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
		v3f pos = readFloatPos(L, 2);
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
		v3f pos = readFloatPos(L, 2);
		// continuous
		bool continuous = lua_toboolean(L, 3);
		// Do it
		co->moveTo(pos, continuous);
		return 0;
	}

	// setvelocity(self, velocity)
	static int l_setvelocity(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// pos
		v3f pos = readFloatPos(L, 2);
		// Do it
		co->setVelocity(pos);
		return 0;
	}
	
	// setacceleration(self, acceleration)
	static int l_setacceleration(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// pos
		v3f pos = readFloatPos(L, 2);
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
	
	// add_to_inventory(self, itemstring)
	// returns: true if item was added, false otherwise
	static int l_add_to_inventory(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		luaL_checkstring(L, 2);
		ServerActiveObject *co = getobject(ref);
		if(co == NULL) return 0;
		// itemstring
		const char *itemstring = lua_tostring(L, 2);
		infostream<<"ObjectRef::l_add_to_inventory(): id="<<co->getId()
				<<" itemstring=\""<<itemstring<<"\""<<std::endl;
		// Do it
		std::istringstream is(itemstring, std::ios::binary);
		ServerEnvironment *env = co->getEnv();
		assert(env);
		IGameDef *gamedef = env->getGameDef();
		InventoryItem *item = InventoryItem::deSerialize(is, gamedef);
		infostream<<"item="<<env<<std::endl;
		bool fits = co->addToInventory(item);
		// Return
		lua_pushboolean(L, fits);
		return 1;
	}

	// settexturemod(self, mod)
	static int l_settexturemod(lua_State *L)
	{
		ObjectRef *ref = checkobject(L, 1);
		LuaEntitySAO *co = getluaobject(ref);
		if(co == NULL) return 0;
		// Do it
		std::string mod = lua_tostring(L, 2);
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
	method(ObjectRef, remove),
	method(ObjectRef, getpos),
	method(ObjectRef, setpos),
	method(ObjectRef, moveto),
	method(ObjectRef, setvelocity),
	method(ObjectRef, setacceleration),
	method(ObjectRef, add_to_inventory),
	method(ObjectRef, settexturemod),
	method(ObjectRef, setsprite),
	{0,0}
};

// Creates a new anonymous reference if id=0
static void objectref_get_or_create(lua_State *L,
		ServerActiveObject *cobj)
{
	if(cobj->getId() == 0){
		ObjectRef::create(L, cobj);
	} else {
		objectref_get(L, cobj->getId());
	}
}

/*
	Main export function
*/

void scriptapi_export(lua_State *L, Server *server)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_export"<<std::endl;
	StackUnroller stack_unroller(L);

	// Store server as light userdata in registry
	lua_pushlightuserdata(L, server);
	lua_setfield(L, LUA_REGISTRYINDEX, "minetest_server");

	// Store nil as minetest_nodedef_defaults in registry
	lua_pushnil(L);
	lua_setfield(L, LUA_REGISTRYINDEX, "minetest_nodedef_default");
	
	// Register global functions in table minetest
	lua_newtable(L);
	luaL_register(L, NULL, minetest_f);
	lua_setglobal(L, "minetest");
	
	// Get the main minetest table
	lua_getglobal(L, "minetest");

	// Add tables to minetest
	
	lua_newtable(L);
	lua_setfield(L, -2, "registered_nodes");
	lua_newtable(L);
	lua_setfield(L, -2, "registered_entities");
	
	lua_newtable(L);
	lua_setfield(L, -2, "object_refs");
	lua_newtable(L);
	lua_setfield(L, -2, "luaentities");

	// Create entity prototype
	luaL_newmetatable(L, "minetest.entity");
	// metatable.__index = metatable
	lua_pushvalue(L, -1); // Duplicate metatable
	lua_setfield(L, -2, "__index");
	// Put functions in metatable
	luaL_register(L, NULL, minetest_entity_m);
	// Put other stuff in metatable

	// Environment C reference
	EnvRef::Register(L);

	// Object C reference
	ObjectRef::Register(L);
}

void scriptapi_add_environment(lua_State *L, ServerEnvironment *env)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_add_environment"<<std::endl;
	StackUnroller stack_unroller(L);

	// Create EnvRef on stack
	EnvRef::create(L, env);
	int envref = lua_gettop(L);

	// minetest.env = envref
	lua_getglobal(L, "minetest");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushvalue(L, envref);
	lua_setfield(L, -2, "env");
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
		script_error(L, "error: %s\n", lua_tostring(L, -1));
}
#endif

/*
	object_reference
*/

void scriptapi_add_object_reference(lua_State *L, ServerActiveObject *cobj)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_add_object_reference: id="<<cobj->getId()<<std::endl;
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
	infostream<<"scriptapi_rm_object_reference: id="<<cobj->getId()<<std::endl;
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

bool scriptapi_on_chat_message(lua_State *L, const std::string &name,
		const std::string &message)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_chat_messages
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_chat_messages");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		lua_pushstring(L, name.c_str());
		lua_pushstring(L, message.c_str());
		if(lua_pcall(L, 2, 1, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		bool ate = lua_toboolean(L, -1);
		lua_pop(L, 1);
		if(ate)
			return true;
		// value removed, keep key for next iteration
	}
	return false;
}

/*
	misc
*/

void scriptapi_on_newplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_newplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_newplayers");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		objectref_get_or_create(L, player);
		if(lua_pcall(L, 1, 0, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		// value removed, keep key for next iteration
	}
}
bool scriptapi_on_respawnplayer(lua_State *L, ServerActiveObject *player)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	bool positioning_handled_by_some = false;

	// Get minetest.registered_on_respawnplayers
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_respawnplayers");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		objectref_get_or_create(L, player);
		if(lua_pcall(L, 1, 1, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		bool positioning_handled = lua_toboolean(L, -1);
		lua_pop(L, 1);
		if(positioning_handled)
			positioning_handled_by_some = true;
		// value removed, keep key for next iteration
	}
	return positioning_handled_by_some;
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
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		lua_pushnumber(L, dtime);
		if(lua_pcall(L, 1, 0, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		// value removed, keep key for next iteration
	}
}

void scriptapi_environment_on_placenode(lua_State *L, v3s16 p, MapNode newnode,
		ServerActiveObject *placer)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_on_placenode"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// And get the writable node definition manager from the server
	IWritableNodeDefManager *ndef =
			server->getWritableNodeDefManager();
	
	// Get minetest.registered_on_placenodes
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_placenodes");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		pushpos(L, p);
		pushnode(L, newnode, ndef);
		objectref_get_or_create(L, placer);
		if(lua_pcall(L, 3, 0, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		// value removed, keep key for next iteration
	}
}

void scriptapi_environment_on_dignode(lua_State *L, v3s16 p, MapNode oldnode,
		ServerActiveObject *digger)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_on_dignode"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// And get the writable node definition manager from the server
	IWritableNodeDefManager *ndef =
			server->getWritableNodeDefManager();
	
	// Get minetest.registered_on_dignodes
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_dignodes");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		pushpos(L, p);
		pushnode(L, oldnode, ndef);
		objectref_get_or_create(L, digger);
		if(lua_pcall(L, 3, 0, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		// value removed, keep key for next iteration
	}
}

void scriptapi_environment_on_punchnode(lua_State *L, v3s16 p, MapNode node,
		ServerActiveObject *puncher)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_on_punchnode"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get server from registry
	lua_getfield(L, LUA_REGISTRYINDEX, "minetest_server");
	Server *server = (Server*)lua_touserdata(L, -1);
	// And get the writable node definition manager from the server
	IWritableNodeDefManager *ndef =
			server->getWritableNodeDefManager();
	
	// Get minetest.registered_on_punchnodes
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_punchnodes");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		pushpos(L, p);
		pushnode(L, node, ndef);
		objectref_get_or_create(L, puncher);
		if(lua_pcall(L, 3, 0, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		// value removed, keep key for next iteration
	}
}

void scriptapi_environment_on_generated(lua_State *L, v3s16 minp, v3s16 maxp)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	//infostream<<"scriptapi_environment_on_generated"<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.registered_on_generateds
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "registered_on_generateds");
	luaL_checktype(L, -1, LUA_TTABLE);
	int table = lua_gettop(L);
	// Foreach
	lua_pushnil(L);
	while(lua_next(L, table) != 0){
		// key at index -2 and value at index -1
		luaL_checktype(L, -1, LUA_TFUNCTION);
		// Call function
		pushpos(L, minp);
		pushpos(L, maxp);
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s\n", lua_tostring(L, -1));
		// value removed, keep key for next iteration
	}
}

/*
	luaentity
*/

bool scriptapi_luaentity_add(lua_State *L, u16 id, const char *name,
		const std::string &staticdata)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_add: id="<<id<<" name=\""
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
	
	// Get on_activate function
	lua_pushvalue(L, object);
	lua_getfield(L, -1, "on_activate");
	if(!lua_isnil(L, -1)){
		luaL_checktype(L, -1, LUA_TFUNCTION);
		lua_pushvalue(L, object); // self
		lua_pushlstring(L, staticdata.c_str(), staticdata.size());
		// Call with 2 arguments, 0 results
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error running function %s:on_activate: %s\n",
					name, lua_tostring(L, -1));
	}
	
	return true;
}

void scriptapi_luaentity_rm(lua_State *L, u16 id)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_rm: id="<<id<<std::endl;

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
	infostream<<"scriptapi_luaentity_get_staticdata: id="<<id<<std::endl;
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
		LuaEntityProperties *prop)
{
	realitycheck(L);
	assert(lua_checkstack(L, 20));
	infostream<<"scriptapi_luaentity_get_properties: id="<<id<<std::endl;
	StackUnroller stack_unroller(L);

	// Get minetest.luaentities[id]
	luaentity_get(L, id);
	//int object = lua_gettop(L);

	/* Read stuff */
	
	getboolfield(L, -1, "physical", prop->physical);

	getfloatfield(L, -1, "weight", prop->weight);

	lua_getfield(L, -1, "collisionbox");
	if(lua_istable(L, -1))
		prop->collisionbox = read_aabbox3df32(L, -1, 1.0);
	lua_pop(L, 1);

	getstringfield(L, -1, "visual", prop->visual);
	
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
	
	lua_getfield(L, -1, "spritediv");
	if(lua_istable(L, -1))
		prop->spritediv = read_v2s16(L, -1);
	lua_pop(L, 1);

	lua_getfield(L, -1, "initial_sprite_basepos");
	if(lua_istable(L, -1))
		prop->initial_sprite_basepos = read_v2s16(L, -1);
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

// Calls entity:on_punch(ObjectRef puncher)
void scriptapi_luaentity_punch(lua_State *L, u16 id,
		ServerActiveObject *puncher)
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
	// Call with 2 arguments, 0 results
	if(lua_pcall(L, 2, 0, 0))
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

