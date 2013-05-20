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
#include "scriptapi_env.h"
#include "nodedef.h"
#include "gamedef.h"
#include "map.h"
#include "daynightratio.h"
#include "content_sao.h"
#include "script.h"
#include "treegen.h"
#include "pathfinder.h"
#include "util/pointedthing.h"
#include "scriptapi_types.h"
#include "scriptapi_noise.h"
#include "scriptapi_nodemeta.h"
#include "scriptapi_nodetimer.h"
#include "scriptapi_object.h"
#include "scriptapi_common.h"
#include "scriptapi_item.h"
#include "scriptapi_node.h"


//TODO
extern void scriptapi_run_callbacks(lua_State *L, int nargs,
		RunCallbacksMode mode);


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
	EnvRef
*/

int EnvRef::gc_object(lua_State *L) {
	EnvRef *o = *(EnvRef **)(lua_touserdata(L, 1));
	delete o;
	return 0;
}

EnvRef* EnvRef::checkobject(lua_State *L, int narg)
{
	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if(!ud) luaL_typerror(L, narg, className);
	return *(EnvRef**)ud;  // unbox pointer
}

// Exported functions

// EnvRef:set_node(pos, node)
// pos = {x=num, y=num, z=num}
int EnvRef::l_set_node(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;
	INodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	v3s16 pos = read_v3s16(L, 2);
	MapNode n = readnode(L, 3, ndef);
	// Do it
	bool succeeded = env->setNode(pos, n);
	lua_pushboolean(L, succeeded);
	return 1;
}

int EnvRef::l_add_node(lua_State *L)
{
	return l_set_node(L);
}

// EnvRef:remove_node(pos)
// pos = {x=num, y=num, z=num}
int EnvRef::l_remove_node(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;

	// parameters
	v3s16 pos = read_v3s16(L, 2);
	// Do it
	bool succeeded = env->removeNode(pos);
	lua_pushboolean(L, succeeded);
	return 1;
}

// EnvRef:get_node(pos)
// pos = {x=num, y=num, z=num}
int EnvRef::l_get_node(lua_State *L)
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
int EnvRef::l_get_node_or_nil(lua_State *L)
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
int EnvRef::l_get_node_light(lua_State *L)
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

// EnvRef:place_node(pos, node)
// pos = {x=num, y=num, z=num}
int EnvRef::l_place_node(lua_State *L)
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
int EnvRef::l_dig_node(lua_State *L)
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
int EnvRef::l_punch_node(lua_State *L)
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
int EnvRef::l_get_meta(lua_State *L)
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
int EnvRef::l_get_node_timer(lua_State *L)
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
int EnvRef::l_add_entity(lua_State *L)
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
int EnvRef::l_add_item(lua_State *L)
{
	//infostream<<"EnvRef::l_add_item()"<<std::endl;
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;
	// pos
	//v3f pos = checkFloatPos(L, 2);
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
int EnvRef::l_add_rat(lua_State *L)
{
	infostream<<"EnvRef::l_add_rat(): C++ mobs have been removed."
			<<" Doing nothing."<<std::endl;
	return 0;
}

// EnvRef:add_firefly(pos)
// pos = {x=num, y=num, z=num}
int EnvRef::l_add_firefly(lua_State *L)
{
	infostream<<"EnvRef::l_add_firefly(): C++ mobs have been removed."
			<<" Doing nothing."<<std::endl;
	return 0;
}

// EnvRef:get_player_by_name(name)
int EnvRef::l_get_player_by_name(lua_State *L)
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
int EnvRef::l_get_objects_inside_radius(lua_State *L)
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
int EnvRef::l_set_timeofday(lua_State *L)
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
int EnvRef::l_get_timeofday(lua_State *L)
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
int EnvRef::l_find_node_near(lua_State *L)
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

// EnvRef:find_nodes_in_area(minp, maxp, nodenames) -> list of positions
// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
int EnvRef::l_find_nodes_in_area(lua_State *L)
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
int EnvRef::l_get_perlin(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;

	int seeddiff = luaL_checkint(L, 2);
	int octaves = luaL_checkint(L, 3);
	float persistence = luaL_checknumber(L, 4);
	float scale = luaL_checknumber(L, 5);

	LuaPerlinNoise *n = new LuaPerlinNoise(seeddiff + int(env->getServerMap().getSeed()), octaves, persistence, scale);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
	luaL_getmetatable(L, "PerlinNoise");
	lua_setmetatable(L, -2);
	return 1;
}

//  EnvRef:get_perlin_map(noiseparams, size)
//  returns world-specific PerlinNoiseMap
int EnvRef::l_get_perlin_map(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if (env == NULL)
		return 0;

	NoiseParams *np = read_noiseparams(L, 2);
	if (!np)
		return 0;
	v3s16 size = read_v3s16(L, 3);

	int seed = (int)(env->getServerMap().getSeed());
	LuaPerlinNoiseMap *n = new LuaPerlinNoiseMap(np, seed, size);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
	luaL_getmetatable(L, "PerlinNoiseMap");
	lua_setmetatable(L, -2);
	return 1;
}

// EnvRef:clear_objects()
// clear all objects in the environment
int EnvRef::l_clear_objects(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	o->m_env->clearAllObjects();
	return 0;
}

int EnvRef::l_line_of_sight(lua_State *L) {
	float stepsize = 1.0;

	//infostream<<"EnvRef::l_get_node()"<<std::endl;
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;

	// read position 1 from lua
	v3f pos1 = checkFloatPos(L, 2);
	// read position 2 from lua
	v3f pos2 = checkFloatPos(L, 2);
	//read step size from lua
	if(lua_isnumber(L, 3))
		stepsize = lua_tonumber(L, 3);

	lua_pushboolean(L, env->line_of_sight(pos1,pos2,stepsize));

	return 1;
}

int EnvRef::l_find_path(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;

	if(env == NULL) return 0;

	v3s16 pos1                  = read_v3s16(L, 2);
	v3s16 pos2                  = read_v3s16(L, 3);
	unsigned int searchdistance = luaL_checkint(L, 4);
	unsigned int max_jump       = luaL_checkint(L, 5);
	unsigned int max_drop       = luaL_checkint(L, 6);
	algorithm algo              = A_PLAIN_NP;
	if(! lua_isnil(L, 7)) {
		std::string algorithm       = luaL_checkstring(L,7);

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

int EnvRef::l_spawn_tree(lua_State *L)
{
	EnvRef *o = checkobject(L, 1);
	ServerEnvironment *env = o->m_env;
	if(env == NULL) return 0;
	v3s16 p0 = read_v3s16(L, 2);

	treegen::TreeDef tree_def;
	std::string trunk,leaves,fruit;
	INodeDefManager *ndef = env->getGameDef()->ndef();

	if(lua_istable(L, 3))
	{
		getstringfield(L, 3, "axiom", tree_def.initial_axiom);
		getstringfield(L, 3, "rules_a", tree_def.rules_a);
		getstringfield(L, 3, "rules_b", tree_def.rules_b);
		getstringfield(L, 3, "rules_c", tree_def.rules_c);
		getstringfield(L, 3, "rules_d", tree_def.rules_d);
		getstringfield(L, 3, "trunk", trunk);
		tree_def.trunknode=ndef->getId(trunk);
		getstringfield(L, 3, "leaves", leaves);
		tree_def.leavesnode=ndef->getId(leaves);
		tree_def.leaves2_chance=0;
		getstringfield(L, 3, "leaves2", leaves);
		if (leaves !="")
		{
			tree_def.leaves2node=ndef->getId(leaves);
			getintfield(L, 3, "leaves2_chance", tree_def.leaves2_chance);
		}
		getintfield(L, 3, "angle", tree_def.angle);
		getintfield(L, 3, "iterations", tree_def.iterations);
		getintfield(L, 3, "random_level", tree_def.iterations_random_level);
		getstringfield(L, 3, "trunk_type", tree_def.trunk_type);
		getboolfield(L, 3, "thin_branches", tree_def.thin_branches);
		tree_def.fruit_chance=0;
		getstringfield(L, 3, "fruit", fruit);
		if (fruit != "")
		{
			tree_def.fruitnode=ndef->getId(fruit);
			getintfield(L, 3, "fruit_chance",tree_def.fruit_chance);
		}
		getintfield(L, 3, "seed", tree_def.seed);
	}
	else
		return 0;
	treegen::spawn_ltree (env, p0, ndef, tree_def);
	return 1;
}


EnvRef::EnvRef(ServerEnvironment *env):
	m_env(env)
{
	//infostream<<"EnvRef created"<<std::endl;
}

EnvRef::~EnvRef()
{
	//infostream<<"EnvRef destructing"<<std::endl;
}

// Creates an EnvRef and leaves it on top of stack
// Not callable from Lua; all references are created on the C side.
void EnvRef::create(lua_State *L, ServerEnvironment *env)
{
	EnvRef *o = new EnvRef(env);
	//infostream<<"EnvRef::create: o="<<o<<std::endl;
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void EnvRef::set_null(lua_State *L)
{
	EnvRef *o = checkobject(L, -1);
	o->m_env = NULL;
}

void EnvRef::Register(lua_State *L)
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

const char EnvRef::className[] = "EnvRef";
const luaL_reg EnvRef::methods[] = {
	luamethod(EnvRef, set_node),
	luamethod(EnvRef, add_node),
	luamethod(EnvRef, remove_node),
	luamethod(EnvRef, get_node),
	luamethod(EnvRef, get_node_or_nil),
	luamethod(EnvRef, get_node_light),
	luamethod(EnvRef, place_node),
	luamethod(EnvRef, dig_node),
	luamethod(EnvRef, punch_node),
	luamethod(EnvRef, add_entity),
	luamethod(EnvRef, add_item),
	luamethod(EnvRef, add_rat),
	luamethod(EnvRef, add_firefly),
	luamethod(EnvRef, get_meta),
	luamethod(EnvRef, get_node_timer),
	luamethod(EnvRef, get_player_by_name),
	luamethod(EnvRef, get_objects_inside_radius),
	luamethod(EnvRef, set_timeofday),
	luamethod(EnvRef, get_timeofday),
	luamethod(EnvRef, find_node_near),
	luamethod(EnvRef, find_nodes_in_area),
	luamethod(EnvRef, get_perlin),
	luamethod(EnvRef, get_perlin_map),
	luamethod(EnvRef, clear_objects),
	luamethod(EnvRef, spawn_tree),
	luamethod(EnvRef, line_of_sight),
	luamethod(EnvRef, find_path),
	{0,0}
};

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
