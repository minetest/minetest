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

#include "cpp_api/scriptapi.h"
#include "lua_api/l_base.h"
#include "lua_api/l_env.h"
#include "lua_api/l_vmanip.h"
#include "environment.h"
#include "server.h"
#include "daynightratio.h"
#include "util/pointedthing.h"
#include "content_sao.h"

#include "common/c_converter.h"
#include "common/c_content.h"
#include "common/c_internal.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "treegen.h"
#include "pathfinder.h"
#include "emerge.h"
#include "mapgen_v7.h"


#define GET_ENV_PTR ServerEnvironment* env =                                   \
				dynamic_cast<ServerEnvironment*>(getEnv(L));                   \
				if( env == NULL) return 0
				
struct EnumString ModApiEnvMod::es_MapgenObject[] =
{
	{MGOBJ_VMANIP,    "voxelmanip"},
	{MGOBJ_HEIGHTMAP, "heightmap"},
	{MGOBJ_BIOMEMAP,  "biomemap"},
	{MGOBJ_HEATMAP,   "heatmap"},
	{MGOBJ_HUMIDMAP,  "humiditymap"},
	{0, NULL},
};


///////////////////////////////////////////////////////////////////////////////


void LuaABM::trigger(ServerEnvironment *env, v3s16 p, MapNode n,
		u32 active_object_count, u32 active_object_count_wider)
{
	ScriptApi* scriptIface = SERVER_TO_SA(env);
	scriptIface->realityCheck();

	lua_State* L = scriptIface->getStack();
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

// Exported functions

// minetest.set_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_set_node(lua_State *L)
{
	GET_ENV_PTR;

	INodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, ndef);
	// Do it
	bool succeeded = env->setNode(pos, n);
	lua_pushboolean(L, succeeded);
	return 1;
}

int ModApiEnvMod::l_add_node(lua_State *L)
{
	return l_set_node(L);
}

// minetest.remove_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_remove_node(lua_State *L)
{
	GET_ENV_PTR;

	// parameters
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool succeeded = env->removeNode(pos);
	lua_pushboolean(L, succeeded);
	return 1;
}

// minetest.get_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	MapNode n = env->getMap().getNodeNoEx(pos);
	// Return node
	pushnode(L, n, env->getGameDef()->ndef());
	return 1;
}

// minetest.get_node_or_nil(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_or_nil(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3s16 pos = read_v3s16(L, 1);
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

// minetest.get_node_light(pos, timeofday)
// pos = {x=num, y=num, z=num}
// timeofday: nil = current time, 0 = night, 0.5 = day
int ModApiEnvMod::l_get_node_light(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	v3s16 pos = read_v3s16(L, 1);
	u32 time_of_day = env->getTimeOfDay();
	if(lua_isnumber(L, 2))
		time_of_day = 24000.0 * lua_tonumber(L, 2);
	time_of_day %= 24000;
	u32 dnr = time_to_daynight_ratio(time_of_day, true);
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

// minetest.place_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_place_node(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, env->getGameDef()->ndef());

	// Don't attempt to load non-loaded area as of now
	MapNode n_old = env->getMap().getNodeNoEx(pos);
	if(n_old.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Create item to place
	INodeDefManager *ndef = getServer(L)->ndef();
	IItemDefManager *idef = getServer(L)->idef();
	ItemStack item(ndef->get(n).name, 1, 0, "", idef);
	// Make pointed position
	PointedThing pointed;
	pointed.type = POINTEDTHING_NODE;
	pointed.node_abovesurface = pos;
	pointed.node_undersurface = pos + v3s16(0,-1,0);
	// Place it with a NULL placer (appears in Lua as a non-functional
	// ObjectRef)
	bool success = get_scriptapi(L)->item_OnPlace(item, NULL, pointed);
	lua_pushboolean(L, success);
	return 1;
}

// minetest.dig_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_dig_node(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);

	// Don't attempt to load non-loaded area as of now
	MapNode n = env->getMap().getNodeNoEx(pos);
	if(n.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Dig it out with a NULL digger (appears in Lua as a
	// non-functional ObjectRef)
	bool success = get_scriptapi(L)->node_on_dig(pos, n, NULL);
	lua_pushboolean(L, success);
	return 1;
}

// minetest.punch_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_punch_node(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);

	// Don't attempt to load non-loaded area as of now
	MapNode n = env->getMap().getNodeNoEx(pos);
	if(n.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Punch it with a NULL puncher (appears in Lua as a non-functional
	// ObjectRef)
	bool success = get_scriptapi(L)->node_on_punch(pos, n, NULL);
	lua_pushboolean(L, success);
	return 1;
}

// minetest.get_node_max_level(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_max_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = env->getMap().getNodeNoEx(pos);
	lua_pushnumber(L, n.getMaxLevel(env->getGameDef()->ndef()));
	return 1;
}

// minetest.get_node_level(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = env->getMap().getNodeNoEx(pos);
	lua_pushnumber(L, n.getLevel(env->getGameDef()->ndef()));
	return 1;
}

// minetest.add_node_level(pos, level)
// pos = {x=num, y=num, z=num}
// level: 0..8
int ModApiEnvMod::l_add_node_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	u8 level = 1;
	if(lua_isnumber(L, 2))
		level = lua_tonumber(L, 2);
	MapNode n = env->getMap().getNodeNoEx(pos);
	lua_pushnumber(L, n.addLevel(env->getGameDef()->ndef(), level));
	env->setNode(pos, n);
	return 1;
}


// minetest.get_meta(pos)
int ModApiEnvMod::l_get_meta(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	v3s16 p = read_v3s16(L, 1);
	NodeMetaRef::create(L, p, env);
	return 1;
}

// minetest.get_node_timer(pos)
int ModApiEnvMod::l_get_node_timer(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	v3s16 p = read_v3s16(L, 1);
	NodeTimerRef::create(L, p, env);
	return 1;
}

// minetest.add_entity(pos, entityname) -> ObjectRef or nil
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_add_entity(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3f pos = checkFloatPos(L, 1);
	// content
	const char *name = luaL_checkstring(L, 2);
	// Do it
	ServerActiveObject *obj = new LuaEntitySAO(env, pos, name, "");
	int objectid = env->addActiveObject(obj);
	// If failed to add, return nothing (reads as nil)
	if(objectid == 0)
		return 0;
	// Return ObjectRef
	get_scriptapi(L)->objectrefGetOrCreate(obj);
	return 1;
}

// minetest.add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_add_item(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	//v3f pos = checkFloatPos(L, 1);
	// item
	ItemStack item = read_item(L, 2,getServer(L));
	if(item.empty() || !item.isKnown(getServer(L)->idef()))
		return 0;
	// Use minetest.spawn_item to spawn a __builtin:item
	lua_getglobal(L, "minetest");
	lua_getfield(L, -1, "spawn_item");
	if(lua_isnil(L, -1))
		return 0;
	lua_pushvalue(L, 1);
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
	objectrefGetOrCreate(L, obj);
	return 1;*/
}

// minetest.get_player_by_name(name)
int ModApiEnvMod::l_get_player_by_name(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	const char *name = luaL_checkstring(L, 1);
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
	get_scriptapi(L)->objectrefGetOrCreate(sao);
	return 1;
}

// minetest.get_objects_inside_radius(pos, radius)
int ModApiEnvMod::l_get_objects_inside_radius(lua_State *L)
{
	// Get the table insert function
	lua_getglobal(L, "table");
	lua_getfield(L, -1, "insert");
	int table_insert = lua_gettop(L);

	GET_ENV_PTR;

	// Do it
	v3f pos = checkFloatPos(L, 1);
	float radius = luaL_checknumber(L, 2) * BS;
	std::set<u16> ids = env->getObjectsInsideRadius(pos, radius);
	lua_newtable(L);
	int table = lua_gettop(L);
	for(std::set<u16>::const_iterator
			i = ids.begin(); i != ids.end(); i++){
		ServerActiveObject *obj = env->getActiveObject(*i);
		// Insert object reference into table
		lua_pushvalue(L, table_insert);
		lua_pushvalue(L, table);
		get_scriptapi(L)->objectrefGetOrCreate(obj);
		if(lua_pcall(L, 2, 0, 0))
			script_error(L, "error: %s", lua_tostring(L, -1));
	}
	return 1;
}

// minetest.set_timeofday(val)
// val = 0...1
int ModApiEnvMod::l_set_timeofday(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	float timeofday_f = luaL_checknumber(L, 1);
	assert(timeofday_f >= 0.0 && timeofday_f <= 1.0);
	int timeofday_mh = (int)(timeofday_f * 24000.0);
	// This should be set directly in the environment but currently
	// such changes aren't immediately sent to the clients, so call
	// the server instead.
	//env->setTimeOfDay(timeofday_mh);
	getServer(L)->setTimeOfDay(timeofday_mh);
	return 0;
}

// minetest.get_timeofday() -> 0...1
int ModApiEnvMod::l_get_timeofday(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	int timeofday_mh = env->getTimeOfDay();
	float timeofday_f = (float)timeofday_mh / 24000.0;
	lua_pushnumber(L, timeofday_f);
	return 1;
}


// minetest.find_node_near(pos, radius, nodenames) -> pos or nil
// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
int ModApiEnvMod::l_find_node_near(lua_State *L)
{
	GET_ENV_PTR;

	INodeDefManager *ndef = getServer(L)->ndef();
	v3s16 pos = read_v3s16(L, 1);
	int radius = luaL_checkinteger(L, 2);
	std::set<content_t> filter;
	if(lua_istable(L, 3)){
		int table = 3;
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TSTRING);
			ndef->getIds(lua_tostring(L, -1), filter);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	} else if(lua_isstring(L, 3)){
		ndef->getIds(lua_tostring(L, 3), filter);
	}

	for(int d=1; d<=radius; d++){
		std::list<v3s16> list;
		getFacePositions(list, d);
		for(std::list<v3s16>::iterator i = list.begin();
				i != list.end(); ++i){
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

// minetest.find_nodes_in_area(minp, maxp, nodenames) -> list of positions
// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
int ModApiEnvMod::l_find_nodes_in_area(lua_State *L)
{
	GET_ENV_PTR;

	INodeDefManager *ndef = getServer(L)->ndef();
	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	std::set<content_t> filter;
	if(lua_istable(L, 3)){
		int table = 3;
		lua_pushnil(L);
		while(lua_next(L, table) != 0){
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TSTRING);
			ndef->getIds(lua_tostring(L, -1), filter);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	} else if(lua_isstring(L, 3)){
		ndef->getIds(lua_tostring(L, 3), filter);
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

// minetest.get_perlin(seeddiff, octaves, persistence, scale)
// returns world-specific PerlinNoise
int ModApiEnvMod::l_get_perlin(lua_State *L)
{
	GET_ENV_PTR;

	int seeddiff = luaL_checkint(L, 1);
	int octaves = luaL_checkint(L, 2);
	float persistence = luaL_checknumber(L, 3);
	float scale = luaL_checknumber(L, 4);

	LuaPerlinNoise *n = new LuaPerlinNoise(seeddiff + int(env->getServerMap().getSeed()), octaves, persistence, scale);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
	luaL_getmetatable(L, "PerlinNoise");
	lua_setmetatable(L, -2);
	return 1;
}

// minetest.get_perlin_map(noiseparams, size)
// returns world-specific PerlinNoiseMap
int ModApiEnvMod::l_get_perlin_map(lua_State *L)
{
	GET_ENV_PTR;

	NoiseParams *np = read_noiseparams(L, 1);
	if (!np)
		return 0;
	v3s16 size = read_v3s16(L, 2);

	int seed = (int)(env->getServerMap().getSeed());
	LuaPerlinNoiseMap *n = new LuaPerlinNoiseMap(np, seed, size);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
	luaL_getmetatable(L, "PerlinNoiseMap");
	lua_setmetatable(L, -2);
	return 1;
}

// minetest.get_voxel_manip()
// returns voxel manipulator
int ModApiEnvMod::l_get_voxel_manip(lua_State *L)
{
	GET_ENV_PTR;

	Map *map = &(env->getMap());
	LuaVoxelManip *o = new LuaVoxelManip(map);
	
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, "VoxelManip");
	lua_setmetatable(L, -2);
	return 1;
}

// minetest.get_mapgen_object(objectname)
// returns the requested object used during map generation
int ModApiEnvMod::l_get_mapgen_object(lua_State *L)
{
	const char *mgobjstr = lua_tostring(L, 1);
	
	int mgobjint;
	if (!string_to_enum(es_MapgenObject, mgobjint, mgobjstr ? mgobjstr : ""))
		return 0;
		
	enum MapgenObject mgobj = (MapgenObject)mgobjint;

	EmergeManager *emerge = getServer(L)->getEmergeManager();
	Mapgen *mg = emerge->getCurrentMapgen();
	if (!mg)
		return 0;
	
	size_t maplen = mg->csize.X * mg->csize.Z;
	
	int nargs = 1;
	
	switch (mgobj) {
		case MGOBJ_VMANIP: {
			ManualMapVoxelManipulator *vm = mg->vm;
			
			// VoxelManip object
			LuaVoxelManip *o = new LuaVoxelManip(vm, true);
			*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
			luaL_getmetatable(L, "VoxelManip");
			lua_setmetatable(L, -2);
			
			// emerged min pos
			push_v3s16(L, vm->m_area.MinEdge);

			// emerged max pos
			push_v3s16(L, vm->m_area.MaxEdge);
			
			nargs = 3;
			
			break; }
		case MGOBJ_HEIGHTMAP: {
			if (!mg->heightmap)
				return 0;
			
			lua_newtable(L);
			for (size_t i = 0; i != maplen; i++) {
				lua_pushinteger(L, mg->heightmap[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break; }
		case MGOBJ_BIOMEMAP: {
			if (!mg->biomemap)
				return 0;
			
			lua_newtable(L);
			for (size_t i = 0; i != maplen; i++) {
				lua_pushinteger(L, mg->biomemap[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break; }
		case MGOBJ_HEATMAP: { // Mapgen V7 specific objects
		case MGOBJ_HUMIDMAP:
			if (strcmp(emerge->params->mg_name.c_str(), "v7"))
				return 0;
			
			MapgenV7 *mgv7 = (MapgenV7 *)mg;

			float *arr = (mgobj == MGOBJ_HEATMAP) ? 
				mgv7->noise_heat->result : mgv7->noise_humidity->result;
			if (!arr)
				return 0;
			
			lua_newtable(L);
			for (size_t i = 0; i != maplen; i++) {
				lua_pushnumber(L, arr[i]);
				lua_rawseti(L, -2, i + 1);
			}
			break; }
	}
	
	return nargs;
}

// minetest.set_mapgen_params(params)
// set mapgen parameters
int ModApiEnvMod::l_set_mapgen_params(lua_State *L)
{
	if (!lua_istable(L, 1))
		return 0;

	EmergeManager *emerge = getServer(L)->getEmergeManager();
	if (emerge->mapgen.size())
		return 0;
	
	MapgenParams *oparams = new MapgenParams;
	u32 paramsmodified = 0;
	u32 flagmask = 0;
	
	lua_getfield(L, 1, "mgname");
	if (lua_isstring(L, -1)) {
		oparams->mg_name = std::string(lua_tostring(L, -1));
		paramsmodified |= MGPARAMS_SET_MGNAME;
	}
	
	lua_getfield(L, 1, "seed");
	if (lua_isnumber(L, -1)) {
		oparams->seed = lua_tointeger(L, -1);
		paramsmodified |= MGPARAMS_SET_SEED;
	}
	
	lua_getfield(L, 1, "water_level");
	if (lua_isnumber(L, -1)) {
		oparams->water_level = lua_tointeger(L, -1);
		paramsmodified |= MGPARAMS_SET_WATER_LEVEL;
	}

	lua_getfield(L, 1, "flags");
	if (lua_isstring(L, -1)) {
		std::string flagstr = std::string(lua_tostring(L, -1));
		oparams->flags = readFlagString(flagstr, flagdesc_mapgen);
		paramsmodified |= MGPARAMS_SET_FLAGS;
	
		lua_getfield(L, 1, "flagmask");
		if (lua_isstring(L, -1)) {
			flagstr = std::string(lua_tostring(L, -1));
			flagmask = readFlagString(flagstr, flagdesc_mapgen);
		}
	}
	
	emerge->luaoverride_params          = oparams;
	emerge->luaoverride_params_modified = paramsmodified;
	emerge->luaoverride_flagmask        = flagmask;
	
	return 0;
}

// minetest.clear_objects()
// clear all objects in the environment
int ModApiEnvMod::l_clear_objects(lua_State *L)
{
	GET_ENV_PTR;

	env->clearAllObjects();
	return 0;
}

// minetest.line_of_sight(pos1, pos2, stepsize) -> true/false
int ModApiEnvMod::l_line_of_sight(lua_State *L) {
	float stepsize = 1.0;

	GET_ENV_PTR;

	// read position 1 from lua
	v3f pos1 = checkFloatPos(L, 1);
	// read position 2 from lua
	v3f pos2 = checkFloatPos(L, 2);
	//read step size from lua
	if (lua_isnumber(L, 3))
		stepsize = lua_tonumber(L, 3);

	return (env->line_of_sight(pos1,pos2,stepsize));
}

// minetest.find_path(pos1, pos2, searchdistance,
//     max_jump, max_drop, algorithm) -> table containing path
int ModApiEnvMod::l_find_path(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos1                  = read_v3s16(L, 1);
	v3s16 pos2                  = read_v3s16(L, 2);
	unsigned int searchdistance = luaL_checkint(L, 3);
	unsigned int max_jump       = luaL_checkint(L, 4);
	unsigned int max_drop       = luaL_checkint(L, 5);
	algorithm algo              = A_PLAIN_NP;
	if (!lua_isnil(L, 6)) {
		std::string algorithm = luaL_checkstring(L,6);

		if (algorithm == "A*")
			algo = A_PLAIN;

		if (algorithm == "Dijkstra")
			algo = DIJKSTRA;
	}

	std::vector<v3s16> path =
			get_Path(env,pos1,pos2,searchdistance,max_jump,max_drop,algo);

	if (path.size() > 0)
	{
		lua_newtable(L);
		int top = lua_gettop(L);
		unsigned int index = 1;
		for (std::vector<v3s16>::iterator i = path.begin(); i != path.end();i++)
		{
			lua_pushnumber(L,index);
			push_v3s16(L, *i);
			lua_settable(L, top);
			index++;
		}
		return 1;
	}

	return 0;
}

// minetest.spawn_tree(pos, treedef)
int ModApiEnvMod::l_spawn_tree(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 p0 = read_v3s16(L, 1);

	treegen::TreeDef tree_def;
	std::string trunk,leaves,fruit;
	INodeDefManager *ndef = env->getGameDef()->ndef();

	if(lua_istable(L, 2))
	{
		getstringfield(L, 2, "axiom", tree_def.initial_axiom);
		getstringfield(L, 2, "rules_a", tree_def.rules_a);
		getstringfield(L, 2, "rules_b", tree_def.rules_b);
		getstringfield(L, 2, "rules_c", tree_def.rules_c);
		getstringfield(L, 2, "rules_d", tree_def.rules_d);
		getstringfield(L, 2, "trunk", trunk);
		tree_def.trunknode=ndef->getId(trunk);
		getstringfield(L, 2, "leaves", leaves);
		tree_def.leavesnode=ndef->getId(leaves);
		tree_def.leaves2_chance=0;
		getstringfield(L, 2, "leaves2", leaves);
		if (leaves !="")
		{
			tree_def.leaves2node=ndef->getId(leaves);
			getintfield(L, 2, "leaves2_chance", tree_def.leaves2_chance);
		}
		getintfield(L, 2, "angle", tree_def.angle);
		getintfield(L, 2, "iterations", tree_def.iterations);
		getintfield(L, 2, "random_level", tree_def.iterations_random_level);
		getstringfield(L, 2, "trunk_type", tree_def.trunk_type);
		getboolfield(L, 2, "thin_branches", tree_def.thin_branches);
		tree_def.fruit_chance=0;
		getstringfield(L, 2, "fruit", fruit);
		if (fruit != "")
		{
			tree_def.fruitnode=ndef->getId(fruit);
			getintfield(L, 2, "fruit_chance",tree_def.fruit_chance);
		}
		getintfield(L, 2, "seed", tree_def.seed);
	}
	else
		return 0;
	treegen::spawn_ltree (env, p0, ndef, tree_def);
	return 1;
}


// minetest.transforming_liquid_add(pos)
int ModApiEnvMod::l_transforming_liquid_add(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 p0 = read_v3s16(L, 1);
	env->getMap().transforming_liquid_add(p0);
	return 1;
}

// minetest.get_heat(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_heat(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	lua_pushnumber(L, env->getServerMap().getHeat(env, pos));
	return 1;
}

// minetest.get_humidity(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_humidity(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	lua_pushnumber(L, env->getServerMap().getHumidity(env, pos));
	return 1;
}


bool ModApiEnvMod::Initialize(lua_State *L,int top)
{

	bool retval = true;

	retval &= API_FCT(set_node);
	retval &= API_FCT(add_node);
	retval &= API_FCT(add_item);
	retval &= API_FCT(remove_node);
	retval &= API_FCT(get_node);
	retval &= API_FCT(get_node_or_nil);
	retval &= API_FCT(get_node_light);
	retval &= API_FCT(place_node);
	retval &= API_FCT(dig_node);
	retval &= API_FCT(punch_node);
	retval &= API_FCT(get_node_max_level);
	retval &= API_FCT(get_node_level);
	retval &= API_FCT(add_node_level);
	retval &= API_FCT(add_entity);
	retval &= API_FCT(get_meta);
	retval &= API_FCT(get_node_timer);
	retval &= API_FCT(get_player_by_name);
	retval &= API_FCT(get_objects_inside_radius);
	retval &= API_FCT(set_timeofday);
	retval &= API_FCT(get_timeofday);
	retval &= API_FCT(find_node_near);
	retval &= API_FCT(find_nodes_in_area);
	retval &= API_FCT(get_perlin);
	retval &= API_FCT(get_perlin_map);
	retval &= API_FCT(get_voxel_manip);
	retval &= API_FCT(get_mapgen_object);
	retval &= API_FCT(set_mapgen_params);
	retval &= API_FCT(clear_objects);
	retval &= API_FCT(spawn_tree);
	retval &= API_FCT(find_path);
	retval &= API_FCT(line_of_sight);
	retval &= API_FCT(transforming_liquid_add);
	retval &= API_FCT(get_heat);
	retval &= API_FCT(get_humidity);

	return retval;
}

ModApiEnvMod modapienv_prototype;
