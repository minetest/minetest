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

#include <algorithm>
#include "lua_api/l_env.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_nodemeta.h"
#include "lua_api/l_nodetimer.h"
#include "lua_api/l_noise.h"
#include "lua_api/l_vmanip.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "scripting_server.h"
#include "environment.h"
#include "mapblock.h"
#include "server.h"
#include "nodedef.h"
#include "daynightratio.h"
#include "util/pointedthing.h"
#include "mapgen/treegen.h"
#include "emerge.h"
#include "pathfinder.h"
#include "face_position_cache.h"
#include "remoteplayer.h"
#include "server/luaentity_sao.h"
#include "server/player_sao.h"
#include "util/string.h"
#include "translation.h"
#ifndef SERVER
#include "client/client.h"
#endif

struct EnumString ModApiEnvMod::es_ClearObjectsMode[] =
{
	{CLEAR_OBJECTS_MODE_FULL,  "full"},
	{CLEAR_OBJECTS_MODE_QUICK, "quick"},
	{0, NULL},
};

///////////////////////////////////////////////////////////////////////////////


void LuaABM::trigger(ServerEnvironment *env, v3s16 p, MapNode n,
		u32 active_object_count, u32 active_object_count_wider)
{
	ServerScripting *scriptIface = env->getScriptIface();
	scriptIface->realityCheck();

	lua_State *L = scriptIface->getStack();
	sanity_check(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Get registered_abms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_abms");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_remove(L, -2); // Remove core

	// Get registered_abms[m_id]
	lua_pushinteger(L, m_id);
	lua_gettable(L, -2);
	if(lua_isnil(L, -1))
		FATAL_ERROR("");
	lua_remove(L, -2); // Remove registered_abms

	scriptIface->setOriginFromTable(-1);

	// Call action
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, "action");
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_remove(L, -2); // Remove registered_abms[m_id]
	push_v3s16(L, p);
	pushnode(L, n, env->getGameDef()->ndef());
	lua_pushnumber(L, active_object_count);
	lua_pushnumber(L, active_object_count_wider);

	int result = lua_pcall(L, 4, 0, error_handler);
	if (result)
		scriptIface->scriptError(result, "LuaABM::trigger");

	lua_pop(L, 1); // Pop error handler
}

void LuaLBM::trigger(ServerEnvironment *env, v3s16 p, MapNode n)
{
	ServerScripting *scriptIface = env->getScriptIface();
	scriptIface->realityCheck();

	lua_State *L = scriptIface->getStack();
	sanity_check(lua_checkstack(L, 20));
	StackUnroller stack_unroller(L);

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Get registered_lbms
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_lbms");
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_remove(L, -2); // Remove core

	// Get registered_lbms[m_id]
	lua_pushinteger(L, m_id);
	lua_gettable(L, -2);
	FATAL_ERROR_IF(lua_isnil(L, -1), "Entry with given id not found in registered_lbms table");
	lua_remove(L, -2); // Remove registered_lbms

	scriptIface->setOriginFromTable(-1);

	// Call action
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_getfield(L, -1, "action");
	luaL_checktype(L, -1, LUA_TFUNCTION);
	lua_remove(L, -2); // Remove registered_lbms[m_id]
	push_v3s16(L, p);
	pushnode(L, n, env->getGameDef()->ndef());

	int result = lua_pcall(L, 2, 0, error_handler);
	if (result)
		scriptIface->scriptError(result, "LuaLBM::trigger");

	lua_pop(L, 1); // Pop error handler
}

int LuaRaycast::l_next(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	bool csm = false;
#ifndef SERVER
	csm = getClient(L) != nullptr;
#endif

	LuaRaycast *o = checkobject(L, 1);
	PointedThing pointed;
	env->continueRaycast(&o->state, &pointed);
	if (pointed.type == POINTEDTHING_NOTHING)
		lua_pushnil(L);
	else
		push_pointed_thing(L, pointed, csm, true);

	return 1;
}

int LuaRaycast::create_object(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	bool objects = true;
	bool liquids = false;

	v3f pos1 = checkFloatPos(L, 1);
	v3f pos2 = checkFloatPos(L, 2);
	if (lua_isboolean(L, 3)) {
		objects = readParam<bool>(L, 3);
	}
	if (lua_isboolean(L, 4)) {
		liquids = readParam<bool>(L, 4);
	}

	LuaRaycast *o = new LuaRaycast(core::line3d<f32>(pos1, pos2),
		objects, liquids);

	*(void **) (lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

LuaRaycast *LuaRaycast::checkobject(lua_State *L, int narg)
{
	NO_MAP_LOCK_REQUIRED;

	luaL_checktype(L, narg, LUA_TUSERDATA);
	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	return *(LuaRaycast **) ud;
}

int LuaRaycast::gc_object(lua_State *L)
{
	LuaRaycast *o = *(LuaRaycast **) (lua_touserdata(L, 1));
	delete o;
	return 0;
}

void LuaRaycast::Register(lua_State *L)
{
	lua_newtable(L);
	int methodtable = lua_gettop(L);
	luaL_newmetatable(L, className);
	int metatable = lua_gettop(L);

	lua_pushliteral(L, "__metatable");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__index");
	lua_pushvalue(L, methodtable);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__gc");
	lua_pushcfunction(L, gc_object);
	lua_settable(L, metatable);

	lua_pushliteral(L, "__call");
	lua_pushcfunction(L, l_next);
	lua_settable(L, metatable);

	lua_pop(L, 1);

	luaL_openlib(L, 0, methods, 0);
	lua_pop(L, 1);

	lua_register(L, className, create_object);
}

const char LuaRaycast::className[] = "Raycast";
const luaL_Reg LuaRaycast::methods[] =
{
	luamethod(LuaRaycast, next),
	{ 0, 0 }
};

void LuaEmergeAreaCallback(v3s16 blockpos, EmergeAction action, void *param)
{
	ScriptCallbackState *state = (ScriptCallbackState *)param;
	assert(state != NULL);
	assert(state->script != NULL);
	assert(state->refcount > 0);

	// state must be protected by envlock
	Server *server = state->script->getServer();
	MutexAutoLock envlock(server->m_env_mutex);

	state->refcount--;

	state->script->on_emerge_area_completion(blockpos, action, state);

	if (state->refcount == 0)
		delete state;
}

// Exported functions

// set_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_set_node(lua_State *L)
{
	GET_ENV_PTR;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, ndef);
	// Do it
	bool succeeded = env->setNode(pos, n);
	lua_pushboolean(L, succeeded);
	return 1;
}

// bulk_set_node([pos1, pos2, ...], node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_bulk_set_node(lua_State *L)
{
	GET_ENV_PTR;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	if (!lua_istable(L, 1)) {
		return 0;
	}

	s32 len = lua_objlen(L, 1);
	if (len == 0) {
		lua_pushboolean(L, true);
		return 1;
	}

	MapNode n = readnode(L, 2, ndef);

	// Do it
	bool succeeded = true;
	for (s32 i = 1; i <= len; i++) {
		lua_rawgeti(L, 1, i);
		if (!env->setNode(read_v3s16(L, -1), n))
			succeeded = false;
		lua_pop(L, 1);
	}

	lua_pushboolean(L, succeeded);
	return 1;
}

int ModApiEnvMod::l_add_node(lua_State *L)
{
	return l_set_node(L);
}

// remove_node(pos)
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

// swap_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_swap_node(lua_State *L)
{
	GET_ENV_PTR;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	// parameters
	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, ndef);
	// Do it
	bool succeeded = env->swapNode(pos, n);
	lua_pushboolean(L, succeeded);
	return 1;
}

// get_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	MapNode n = env->getMap().getNode(pos);
	// Return node
	pushnode(L, n, env->getGameDef()->ndef());
	return 1;
}

// get_node_or_nil(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_or_nil(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	v3s16 pos = read_v3s16(L, 1);
	// Do it
	bool pos_ok;
	MapNode n = env->getMap().getNode(pos, &pos_ok);
	if (pos_ok) {
		// Return node
		pushnode(L, n, env->getGameDef()->ndef());
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// get_node_light(pos, timeofday)
// pos = {x=num, y=num, z=num}
// timeofday: nil = current time, 0 = night, 0.5 = day
int ModApiEnvMod::l_get_node_light(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	// Do it
	v3s16 pos = read_v3s16(L, 1);
	u32 time_of_day = env->getTimeOfDay();
	if(lua_isnumber(L, 2))
		time_of_day = 24000.0 * lua_tonumber(L, 2);
	time_of_day %= 24000;
	u32 dnr = time_to_daynight_ratio(time_of_day, true);

	bool is_position_ok;
	MapNode n = env->getMap().getNode(pos, &is_position_ok);
	if (is_position_ok) {
		const NodeDefManager *ndef = env->getGameDef()->ndef();
		lua_pushinteger(L, n.getLightBlend(dnr, ndef));
	} else {
		lua_pushnil(L);
	}
	return 1;
}

// place_node(pos, node)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_place_node(lua_State *L)
{
	GET_ENV_PTR;

	ScriptApiItem *scriptIfaceItem = getScriptApi<ScriptApiItem>(L);
	Server *server = getServer(L);
	const NodeDefManager *ndef = server->ndef();
	IItemDefManager *idef = server->idef();

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = readnode(L, 2, ndef);

	// Don't attempt to load non-loaded area as of now
	MapNode n_old = env->getMap().getNode(pos);
	if(n_old.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Create item to place
	ItemStack item(ndef->get(n).name, 1, 0, idef);
	// Make pointed position
	PointedThing pointed;
	pointed.type = POINTEDTHING_NODE;
	pointed.node_abovesurface = pos;
	pointed.node_undersurface = pos + v3s16(0,-1,0);
	// Place it with a NULL placer (appears in Lua as nil)
	bool success = scriptIfaceItem->item_OnPlace(item, nullptr, pointed);
	lua_pushboolean(L, success);
	return 1;
}

// dig_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_dig_node(lua_State *L)
{
	GET_ENV_PTR;

	ScriptApiNode *scriptIfaceNode = getScriptApi<ScriptApiNode>(L);

	v3s16 pos = read_v3s16(L, 1);

	// Don't attempt to load non-loaded area as of now
	MapNode n = env->getMap().getNode(pos);
	if(n.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Dig it out with a NULL digger (appears in Lua as a
	// non-functional ObjectRef)
	bool success = scriptIfaceNode->node_on_dig(pos, n, NULL);
	lua_pushboolean(L, success);
	return 1;
}

// punch_node(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_punch_node(lua_State *L)
{
	GET_ENV_PTR;

	ScriptApiNode *scriptIfaceNode = getScriptApi<ScriptApiNode>(L);

	v3s16 pos = read_v3s16(L, 1);

	// Don't attempt to load non-loaded area as of now
	MapNode n = env->getMap().getNode(pos);
	if(n.getContent() == CONTENT_IGNORE){
		lua_pushboolean(L, false);
		return 1;
	}
	// Punch it with a NULL puncher (appears in Lua as a non-functional
	// ObjectRef)
	bool success = scriptIfaceNode->node_on_punch(pos, n, NULL, PointedThing());
	lua_pushboolean(L, success);
	return 1;
}

// get_node_max_level(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_max_level(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.getMaxLevel(env->getGameDef()->ndef()));
	return 1;
}

// get_node_level(pos)
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_get_node_level(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.getLevel(env->getGameDef()->ndef()));
	return 1;
}

// set_node_level(pos, level)
// pos = {x=num, y=num, z=num}
// level: 0..63
int ModApiEnvMod::l_set_node_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	u8 level = 1;
	if(lua_isnumber(L, 2))
		level = lua_tonumber(L, 2);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.setLevel(env->getGameDef()->ndef(), level));
	env->setNode(pos, n);
	return 1;
}

// add_node_level(pos, level)
// pos = {x=num, y=num, z=num}
// level: -127..127
int ModApiEnvMod::l_add_node_level(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos = read_v3s16(L, 1);
	s16 level = 1;
	if(lua_isnumber(L, 2))
		level = lua_tonumber(L, 2);
	MapNode n = env->getMap().getNode(pos);
	lua_pushnumber(L, n.addLevel(env->getGameDef()->ndef(), level));
	env->setNode(pos, n);
	return 1;
}

// find_nodes_with_meta(pos1, pos2)
int ModApiEnvMod::l_find_nodes_with_meta(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	std::vector<v3s16> positions = env->getMap().findNodesWithMetadata(
		check_v3s16(L, 1), check_v3s16(L, 2));

	lua_createtable(L, positions.size(), 0);
	for (size_t i = 0; i != positions.size(); i++) {
		push_v3s16(L, positions[i]);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

// get_meta(pos)
int ModApiEnvMod::l_get_meta(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	v3s16 p = read_v3s16(L, 1);
	NodeMetaRef::create(L, p, env);
	return 1;
}

// get_node_timer(pos)
int ModApiEnvMod::l_get_node_timer(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	v3s16 p = read_v3s16(L, 1);
	NodeTimerRef::create(L, p, &env->getServerMap());
	return 1;
}

// add_entity(pos, entityname, [staticdata]) -> ObjectRef or nil
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_add_entity(lua_State *L)
{
	GET_ENV_PTR;

	v3f pos = checkFloatPos(L, 1);
	const char *name = luaL_checkstring(L, 2);
	const char *staticdata = luaL_optstring(L, 3, "");

	ServerActiveObject *obj = new LuaEntitySAO(env, pos, name, staticdata);
	int objectid = env->addActiveObject(obj);
	// If failed to add, return nothing (reads as nil)
	if(objectid == 0)
		return 0;

	// If already deleted (can happen in on_activate), return nil
	if (obj->isGone())
		return 0;
	getScriptApiBase(L)->objectrefGetOrCreate(L, obj);
	return 1;
}

// add_item(pos, itemstack or itemstring or table) -> ObjectRef or nil
// pos = {x=num, y=num, z=num}
int ModApiEnvMod::l_add_item(lua_State *L)
{
	GET_ENV_PTR;

	// pos
	//v3f pos = checkFloatPos(L, 1);
	// item
	ItemStack item = read_item(L, 2,getServer(L)->idef());
	if(item.empty() || !item.isKnown(getServer(L)->idef()))
		return 0;

	int error_handler = PUSH_ERROR_HANDLER(L);

	// Use spawn_item to spawn a __builtin:item
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "spawn_item");
	lua_remove(L, -2); // Remove core
	if(lua_isnil(L, -1))
		return 0;
	lua_pushvalue(L, 1);
	lua_pushstring(L, item.getItemString().c_str());

	PCALL_RESL(L, lua_pcall(L, 2, 1, error_handler));

	lua_remove(L, error_handler);
	return 1;
}

// get_connected_players()
int ModApiEnvMod::l_get_connected_players(lua_State *L)
{
	ServerEnvironment *env = (ServerEnvironment *) getEnv(L);
	if (!env) {
		log_deprecated(L, "Calling get_connected_players() at mod load time"
				" is deprecated");
		lua_createtable(L, 0, 0);
		return 1;
	}

	lua_createtable(L, env->getPlayerCount(), 0);
	u32 i = 0;
	for (RemotePlayer *player : env->getPlayers()) {
		if (player->getPeerId() == PEER_ID_INEXISTENT)
			continue;
		PlayerSAO *sao = player->getPlayerSAO();
		if (sao && !sao->isGone()) {
			getScriptApiBase(L)->objectrefGetOrCreate(L, sao);
			lua_rawseti(L, -2, ++i);
		}
	}
	return 1;
}

// get_player_by_name(name)
int ModApiEnvMod::l_get_player_by_name(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	const char *name = luaL_checkstring(L, 1);
	RemotePlayer *player = env->getPlayer(name);
	if (!player || player->getPeerId() == PEER_ID_INEXISTENT)
		return 0;
	PlayerSAO *sao = player->getPlayerSAO();
	if (!sao || sao->isGone())
		return 0;
	// Put player on stack
	getScriptApiBase(L)->objectrefGetOrCreate(L, sao);
	return 1;
}

// get_objects_inside_radius(pos, radius)
int ModApiEnvMod::l_get_objects_inside_radius(lua_State *L)
{
	GET_ENV_PTR;
	ScriptApiBase *script = getScriptApiBase(L);

	// Do it
	v3f pos = checkFloatPos(L, 1);
	float radius = readParam<float>(L, 2) * BS;
	std::vector<ServerActiveObject *> objs;

	auto include_obj_cb = [](ServerActiveObject *obj){ return !obj->isGone(); };
	env->getObjectsInsideRadius(objs, pos, radius, include_obj_cb);

	int i = 0;
	lua_createtable(L, objs.size(), 0);
	for (const auto obj : objs) {
		// Insert object reference into table
		script->objectrefGetOrCreate(L, obj);
		lua_rawseti(L, -2, ++i);
	}
	return 1;
}

// set_timeofday(val)
// val = 0...1
int ModApiEnvMod::l_set_timeofday(lua_State *L)
{
	GET_ENV_PTR;

	// Do it
	float timeofday_f = readParam<float>(L, 1);
	sanity_check(timeofday_f >= 0.0 && timeofday_f <= 1.0);
	int timeofday_mh = (int)(timeofday_f * 24000.0);
	// This should be set directly in the environment but currently
	// such changes aren't immediately sent to the clients, so call
	// the server instead.
	//env->setTimeOfDay(timeofday_mh);
	getServer(L)->setTimeOfDay(timeofday_mh);
	return 0;
}

// get_timeofday() -> 0...1
int ModApiEnvMod::l_get_timeofday(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	// Do it
	int timeofday_mh = env->getTimeOfDay();
	float timeofday_f = (float)timeofday_mh / 24000.0f;
	lua_pushnumber(L, timeofday_f);
	return 1;
}

// get_day_count() -> int
int ModApiEnvMod::l_get_day_count(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	lua_pushnumber(L, env->getDayCount());
	return 1;
}

// get_gametime()
int ModApiEnvMod::l_get_gametime(lua_State *L)
{
	GET_ENV_PTR;

	int game_time = env->getGameTime();
	lua_pushnumber(L, game_time);
	return 1;
}

void ModApiEnvMod::collectNodeIds(lua_State *L, int idx, const NodeDefManager *ndef,
	std::vector<content_t> &filter)
{
	if (lua_istable(L, idx)) {
		lua_pushnil(L);
		while (lua_next(L, idx) != 0) {
			// key at index -2 and value at index -1
			luaL_checktype(L, -1, LUA_TSTRING);
			ndef->getIds(readParam<std::string>(L, -1), filter);
			// removes value, keeps key for next iteration
			lua_pop(L, 1);
		}
	} else if (lua_isstring(L, idx)) {
		ndef->getIds(readParam<std::string>(L, 3), filter);
	}
}

// find_node_near(pos, radius, nodenames, [search_center]) -> pos or nil
// nodenames: eg. {"ignore", "group:tree"} or "default:dirt"
int ModApiEnvMod::l_find_node_near(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

	v3s16 pos = read_v3s16(L, 1);
	int radius = luaL_checkinteger(L, 2);
	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	int start_radius = (lua_isboolean(L, 4) && readParam<bool>(L, 4)) ? 0 : 1;

#ifndef SERVER
	// Client API limitations
	if (Client *client = getClient(L))
		radius = client->CSMClampRadius(pos, radius);
#endif

	for (int d = start_radius; d <= radius; d++) {
		const std::vector<v3s16> &list = FacePositionCache::getFacePositions(d);
		for (const v3s16 &i : list) {
			v3s16 p = pos + i;
			content_t c = map.getNode(p).getContent();
			if (CONTAINS(filter, c)) {
				push_v3s16(L, p);
				return 1;
			}
		}
	}
	return 0;
}

// find_nodes_in_area(minp, maxp, nodenames, [grouped])
int ModApiEnvMod::l_find_nodes_in_area(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	sortBoxVerticies(minp, maxp);

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

#ifndef SERVER
	if (Client *client = getClient(L)) {
		minp = client->CSMClampPos(minp);
		maxp = client->CSMClampPos(maxp);
	}
#endif

	v3s16 cube = maxp - minp + 1;
	// Volume limit equal to 8 default mapchunks, (80 * 2) ^ 3 = 4,096,000
	if ((u64)cube.X * (u64)cube.Y * (u64)cube.Z > 4096000) {
		luaL_error(L, "find_nodes_in_area(): area volume"
				" exceeds allowed value of 4096000");
		return 0;
	}

	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	bool grouped = lua_isboolean(L, 4) && readParam<bool>(L, 4);

	if (grouped) {
		// create the table we will be returning
		lua_createtable(L, 0, filter.size());
		int base = lua_gettop(L);

		// create one table for each filter
		std::vector<u32> idx;
		idx.resize(filter.size());
		for (u32 i = 0; i < filter.size(); i++)
			lua_newtable(L);

		v3s16 p;
		for (p.X = minp.X; p.X <= maxp.X; p.X++)
		for (p.Y = minp.Y; p.Y <= maxp.Y; p.Y++)
		for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
			content_t c = map.getNode(p).getContent();

			auto it = std::find(filter.begin(), filter.end(), c);
			if (it != filter.end()) {
				// Calculate index of the table and append the position
				u32 filt_index = it - filter.begin();
				push_v3s16(L, p);
				lua_rawseti(L, base + 1 + filt_index, ++idx[filt_index]);
			}
		}

		// last filter table is at top of stack
		u32 i = filter.size() - 1;
		do {
			if (idx[i] == 0) {
				// No such node found -> drop the empty table
				lua_pop(L, 1);
			} else {
				// This node was found -> put table into the return table
				lua_setfield(L, base, ndef->get(filter[i]).name.c_str());
			}
		} while (i-- != 0);

		assert(lua_gettop(L) == base);
		return 1;
	} else {
		std::vector<u32> individual_count;
		individual_count.resize(filter.size());

		lua_newtable(L);
		u32 i = 0;
		v3s16 p;
		for (p.X = minp.X; p.X <= maxp.X; p.X++)
		for (p.Y = minp.Y; p.Y <= maxp.Y; p.Y++)
		for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
			content_t c = env->getMap().getNode(p).getContent();

			auto it = std::find(filter.begin(), filter.end(), c);
			if (it != filter.end()) {
				push_v3s16(L, p);
				lua_rawseti(L, -2, ++i);

				u32 filt_index = it - filter.begin();
				individual_count[filt_index]++;
			}
		}

		lua_createtable(L, 0, filter.size());
		for (u32 i = 0; i < filter.size(); i++) {
			lua_pushinteger(L, individual_count[i]);
			lua_setfield(L, -2, ndef->get(filter[i]).name.c_str());
		}
		return 2;
	}
}

// find_nodes_in_area_under_air(minp, maxp, nodenames) -> list of positions
// nodenames: e.g. {"ignore", "group:tree"} or "default:dirt"
int ModApiEnvMod::l_find_nodes_in_area_under_air(lua_State *L)
{
	/* Note: A similar but generalized (and therefore slower) version of this
	 * function could be created -- e.g. find_nodes_in_area_under -- which
	 * would accept a node name (or ID?) or list of names that the "above node"
	 * should be.
	 * TODO
	 */

	GET_PLAIN_ENV_PTR;

	v3s16 minp = read_v3s16(L, 1);
	v3s16 maxp = read_v3s16(L, 2);
	sortBoxVerticies(minp, maxp);

	const NodeDefManager *ndef = env->getGameDef()->ndef();
	Map &map = env->getMap();

#ifndef SERVER
	if (Client *client = getClient(L)) {
		minp = client->CSMClampPos(minp);
		maxp = client->CSMClampPos(maxp);
	}
#endif

	v3s16 cube = maxp - minp + 1;
	// Volume limit equal to 8 default mapchunks, (80 * 2) ^ 3 = 4,096,000
	if ((u64)cube.X * (u64)cube.Y * (u64)cube.Z > 4096000) {
		luaL_error(L, "find_nodes_in_area_under_air(): area volume"
				" exceeds allowed value of 4096000");
		return 0;
	}

	std::vector<content_t> filter;
	collectNodeIds(L, 3, ndef, filter);

	lua_newtable(L);
	u32 i = 0;
	v3s16 p;
	for (p.X = minp.X; p.X <= maxp.X; p.X++)
	for (p.Z = minp.Z; p.Z <= maxp.Z; p.Z++) {
		p.Y = minp.Y;
		content_t c = map.getNode(p).getContent();
		for (; p.Y <= maxp.Y; p.Y++) {
			v3s16 psurf(p.X, p.Y + 1, p.Z);
			content_t csurf = map.getNode(psurf).getContent();
			if (c != CONTENT_AIR && csurf == CONTENT_AIR &&
					CONTAINS(filter, c)) {
				push_v3s16(L, p);
				lua_rawseti(L, -2, ++i);
			}
			c = csurf;
		}
	}
	return 1;
}

// get_perlin(seeddiff, octaves, persistence, scale)
// returns world-specific PerlinNoise
int ModApiEnvMod::l_get_perlin(lua_State *L)
{
	GET_ENV_PTR_NO_MAP_LOCK;

	NoiseParams params;

	if (lua_istable(L, 1)) {
		read_noiseparams(L, 1, &params);
	} else {
		params.seed    = luaL_checkint(L, 1);
		params.octaves = luaL_checkint(L, 2);
		params.persist = readParam<float>(L, 3);
		params.spread  = v3f(1, 1, 1) * readParam<float>(L, 4);
	}

	params.seed += (int)env->getServerMap().getSeed();

	LuaPerlinNoise *n = new LuaPerlinNoise(&params);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
	luaL_getmetatable(L, "PerlinNoise");
	lua_setmetatable(L, -2);
	return 1;
}

// get_perlin_map(noiseparams, size)
// returns world-specific PerlinNoiseMap
int ModApiEnvMod::l_get_perlin_map(lua_State *L)
{
	GET_ENV_PTR_NO_MAP_LOCK;

	NoiseParams np;
	if (!read_noiseparams(L, 1, &np))
		return 0;
	v3s16 size = read_v3s16(L, 2);

	s32 seed = (s32)(env->getServerMap().getSeed());
	LuaPerlinNoiseMap *n = new LuaPerlinNoiseMap(&np, seed, size);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = n;
	luaL_getmetatable(L, "PerlinNoiseMap");
	lua_setmetatable(L, -2);
	return 1;
}

// get_voxel_manip()
// returns voxel manipulator
int ModApiEnvMod::l_get_voxel_manip(lua_State *L)
{
	GET_ENV_PTR;

	Map *map = &(env->getMap());
	LuaVoxelManip *o = (lua_istable(L, 1) && lua_istable(L, 2)) ?
		new LuaVoxelManip(map, read_v3s16(L, 1), read_v3s16(L, 2)) :
		new LuaVoxelManip(map);

	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, "VoxelManip");
	lua_setmetatable(L, -2);
	return 1;
}

// clear_objects([options])
// clear all objects in the environment
// where options = {mode = "full" or "quick"}
int ModApiEnvMod::l_clear_objects(lua_State *L)
{
	GET_ENV_PTR;

	ClearObjectsMode mode = CLEAR_OBJECTS_MODE_QUICK;
	if (lua_istable(L, 1)) {
		mode = (ClearObjectsMode)getenumfield(L, 1, "mode",
			ModApiEnvMod::es_ClearObjectsMode, mode);
	}

	env->clearObjects(mode);
	return 0;
}

// line_of_sight(pos1, pos2) -> true/false, pos
int ModApiEnvMod::l_line_of_sight(lua_State *L)
{
	GET_PLAIN_ENV_PTR;

	// read position 1 from lua
	v3f pos1 = checkFloatPos(L, 1);
	// read position 2 from lua
	v3f pos2 = checkFloatPos(L, 2);

	v3s16 p;

	bool success = env->line_of_sight(pos1, pos2, &p);
	lua_pushboolean(L, success);
	if (!success) {
		push_v3s16(L, p);
		return 2;
	}
	return 1;
}

// fix_light(p1, p2)
int ModApiEnvMod::l_fix_light(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 blockpos1 = getContainerPos(read_v3s16(L, 1), MAP_BLOCKSIZE);
	v3s16 blockpos2 = getContainerPos(read_v3s16(L, 2), MAP_BLOCKSIZE);
	ServerMap &map = env->getServerMap();
	std::map<v3s16, MapBlock *> modified_blocks;
	bool success = true;
	v3s16 blockpos;
	for (blockpos.X = blockpos1.X; blockpos.X <= blockpos2.X; blockpos.X++)
	for (blockpos.Y = blockpos1.Y; blockpos.Y <= blockpos2.Y; blockpos.Y++)
	for (blockpos.Z = blockpos1.Z; blockpos.Z <= blockpos2.Z; blockpos.Z++) {
		success = success & map.repairBlockLight(blockpos, &modified_blocks);
	}
	if (!modified_blocks.empty()) {
		MapEditEvent event;
		event.type = MEET_OTHER;
		for (auto &modified_block : modified_blocks)
			event.modified_blocks.insert(modified_block.first);

		map.dispatchEvent(event);
	}
	lua_pushboolean(L, success);

	return 1;
}

int ModApiEnvMod::l_raycast(lua_State *L)
{
	return LuaRaycast::create_object(L);
}

// load_area(p1, [p2])
// load mapblocks in area p1..p2, but do not generate map
int ModApiEnvMod::l_load_area(lua_State *L)
{
	GET_ENV_PTR;
	MAP_LOCK_REQUIRED;

	Map *map = &(env->getMap());
	v3s16 bp1 = getNodeBlockPos(check_v3s16(L, 1));
	if (!lua_istable(L, 2)) {
		map->emergeBlock(bp1);
	} else {
		v3s16 bp2 = getNodeBlockPos(check_v3s16(L, 2));
		sortBoxVerticies(bp1, bp2);
		for (s16 z = bp1.Z; z <= bp2.Z; z++)
		for (s16 y = bp1.Y; y <= bp2.Y; y++)
		for (s16 x = bp1.X; x <= bp2.X; x++) {
			map->emergeBlock(v3s16(x, y, z));
		}
	}

	return 0;
}

// emerge_area(p1, p2, [callback, context])
// emerge mapblocks in area p1..p2, calls callback with context upon completion
int ModApiEnvMod::l_emerge_area(lua_State *L)
{
	GET_ENV_PTR;

	EmergeCompletionCallback callback = NULL;
	ScriptCallbackState *state = NULL;

	EmergeManager *emerge = getServer(L)->getEmergeManager();

	v3s16 bpmin = getNodeBlockPos(read_v3s16(L, 1));
	v3s16 bpmax = getNodeBlockPos(read_v3s16(L, 2));
	sortBoxVerticies(bpmin, bpmax);

	size_t num_blocks = VoxelArea(bpmin, bpmax).getVolume();
	assert(num_blocks != 0);

	if (lua_isfunction(L, 3)) {
		callback = LuaEmergeAreaCallback;

		lua_pushvalue(L, 3);
		int callback_ref = luaL_ref(L, LUA_REGISTRYINDEX);

		lua_pushvalue(L, 4);
		int args_ref = luaL_ref(L, LUA_REGISTRYINDEX);

		state = new ScriptCallbackState;
		state->script       = getServer(L)->getScriptIface();
		state->callback_ref = callback_ref;
		state->args_ref     = args_ref;
		state->refcount     = num_blocks;
		state->origin       = getScriptApiBase(L)->getOrigin();
	}

	for (s16 z = bpmin.Z; z <= bpmax.Z; z++)
	for (s16 y = bpmin.Y; y <= bpmax.Y; y++)
	for (s16 x = bpmin.X; x <= bpmax.X; x++) {
		emerge->enqueueBlockEmergeEx(v3s16(x, y, z), PEER_ID_INEXISTENT,
			BLOCK_EMERGE_ALLOW_GEN | BLOCK_EMERGE_FORCE_QUEUE, callback, state);
	}

	return 0;
}

// delete_area(p1, p2)
// delete mapblocks in area p1..p2
int ModApiEnvMod::l_delete_area(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 bpmin = getNodeBlockPos(read_v3s16(L, 1));
	v3s16 bpmax = getNodeBlockPos(read_v3s16(L, 2));
	sortBoxVerticies(bpmin, bpmax);

	ServerMap &map = env->getServerMap();

	MapEditEvent event;
	event.type = MEET_OTHER;

	bool success = true;
	for (s16 z = bpmin.Z; z <= bpmax.Z; z++)
	for (s16 y = bpmin.Y; y <= bpmax.Y; y++)
	for (s16 x = bpmin.X; x <= bpmax.X; x++) {
		v3s16 bp(x, y, z);
		if (map.deleteBlock(bp)) {
			env->setStaticForActiveObjectsInBlock(bp, false);
			event.modified_blocks.insert(bp);
		} else {
			success = false;
		}
	}

	map.dispatchEvent(event);
	lua_pushboolean(L, success);
	return 1;
}

// find_path(pos1, pos2, searchdistance,
//     max_jump, max_drop, algorithm) -> table containing path
int ModApiEnvMod::l_find_path(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 pos1                  = read_v3s16(L, 1);
	v3s16 pos2                  = read_v3s16(L, 2);
	unsigned int searchdistance = luaL_checkint(L, 3);
	unsigned int max_jump       = luaL_checkint(L, 4);
	unsigned int max_drop       = luaL_checkint(L, 5);
	PathAlgorithm algo          = PA_PLAIN_NP;
	if (!lua_isnoneornil(L, 6)) {
		std::string algorithm = luaL_checkstring(L,6);

		if (algorithm == "A*")
			algo = PA_PLAIN;

		if (algorithm == "Dijkstra")
			algo = PA_DIJKSTRA;
	}

	std::vector<v3s16> path = get_path(&env->getServerMap(), env->getGameDef()->ndef(), pos1, pos2,
		searchdistance, max_jump, max_drop, algo);

	if (!path.empty()) {
		lua_createtable(L, path.size(), 0);
		int top = lua_gettop(L);
		unsigned int index = 1;
		for (const v3s16 &i : path) {
			lua_pushnumber(L,index);
			push_v3s16(L, i);
			lua_settable(L, top);
			index++;
		}
		return 1;
	}

	return 0;
}

// spawn_tree(pos, treedef)
int ModApiEnvMod::l_spawn_tree(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 p0 = read_v3s16(L, 1);

	treegen::TreeDef tree_def;
	std::string trunk,leaves,fruit;
	const NodeDefManager *ndef = env->getGameDef()->ndef();

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
		if (!leaves.empty()) {
			tree_def.leaves2node=ndef->getId(leaves);
			getintfield(L, 2, "leaves2_chance", tree_def.leaves2_chance);
		}
		getintfield(L, 2, "angle", tree_def.angle);
		getintfield(L, 2, "iterations", tree_def.iterations);
		if (!getintfield(L, 2, "random_level", tree_def.iterations_random_level))
			tree_def.iterations_random_level = 0;
		getstringfield(L, 2, "trunk_type", tree_def.trunk_type);
		getboolfield(L, 2, "thin_branches", tree_def.thin_branches);
		tree_def.fruit_chance=0;
		getstringfield(L, 2, "fruit", fruit);
		if (!fruit.empty()) {
			tree_def.fruitnode=ndef->getId(fruit);
			getintfield(L, 2, "fruit_chance",tree_def.fruit_chance);
		}
		tree_def.explicit_seed = getintfield(L, 2, "seed", tree_def.seed);
	}
	else
		return 0;

	ServerMap *map = &env->getServerMap();
	treegen::error e;
	if ((e = treegen::spawn_ltree (map, p0, ndef, tree_def)) != treegen::SUCCESS) {
		if (e == treegen::UNBALANCED_BRACKETS) {
			luaL_error(L, "spawn_tree(): closing ']' has no matching opening bracket");
		} else {
			luaL_error(L, "spawn_tree(): unknown error");
		}
	}

	return 1;
}

// transforming_liquid_add(pos)
int ModApiEnvMod::l_transforming_liquid_add(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 p0 = read_v3s16(L, 1);
	env->getMap().transforming_liquid_add(p0);
	return 1;
}

// forceload_block(blockpos)
// blockpos = {x=num, y=num, z=num}
int ModApiEnvMod::l_forceload_block(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 blockpos = read_v3s16(L, 1);
	env->getForceloadedBlocks()->insert(blockpos);
	return 0;
}

// forceload_free_block(blockpos)
// blockpos = {x=num, y=num, z=num}
int ModApiEnvMod::l_forceload_free_block(lua_State *L)
{
	GET_ENV_PTR;

	v3s16 blockpos = read_v3s16(L, 1);
	env->getForceloadedBlocks()->erase(blockpos);
	return 0;
}

// get_translated_string(lang_code, string)
int ModApiEnvMod::l_get_translated_string(lua_State * L)
{
	GET_ENV_PTR;
	std::string lang_code = luaL_checkstring(L, 1);
	std::string string = luaL_checkstring(L, 2);

	auto *translations = getServer(L)->getTranslationLanguage(lang_code);
	string = wide_to_utf8(translate_string(utf8_to_wide(string), translations));
	lua_pushstring(L, string.c_str());
	return 1;
}

void ModApiEnvMod::Initialize(lua_State *L, int top)
{
	API_FCT(set_node);
	API_FCT(bulk_set_node);
	API_FCT(add_node);
	API_FCT(swap_node);
	API_FCT(add_item);
	API_FCT(remove_node);
	API_FCT(get_node);
	API_FCT(get_node_or_nil);
	API_FCT(get_node_light);
	API_FCT(place_node);
	API_FCT(dig_node);
	API_FCT(punch_node);
	API_FCT(get_node_max_level);
	API_FCT(get_node_level);
	API_FCT(set_node_level);
	API_FCT(add_node_level);
	API_FCT(add_entity);
	API_FCT(find_nodes_with_meta);
	API_FCT(get_meta);
	API_FCT(get_node_timer);
	API_FCT(get_connected_players);
	API_FCT(get_player_by_name);
	API_FCT(get_objects_inside_radius);
	API_FCT(set_timeofday);
	API_FCT(get_timeofday);
	API_FCT(get_gametime);
	API_FCT(get_day_count);
	API_FCT(find_node_near);
	API_FCT(find_nodes_in_area);
	API_FCT(find_nodes_in_area_under_air);
	API_FCT(fix_light);
	API_FCT(load_area);
	API_FCT(emerge_area);
	API_FCT(delete_area);
	API_FCT(get_perlin);
	API_FCT(get_perlin_map);
	API_FCT(get_voxel_manip);
	API_FCT(clear_objects);
	API_FCT(spawn_tree);
	API_FCT(find_path);
	API_FCT(line_of_sight);
	API_FCT(raycast);
	API_FCT(transforming_liquid_add);
	API_FCT(forceload_block);
	API_FCT(forceload_free_block);
	API_FCT(get_translated_string);
}

void ModApiEnvMod::InitializeClient(lua_State *L, int top)
{
	API_FCT(get_node_light);
	API_FCT(get_timeofday);
	API_FCT(get_node_max_level);
	API_FCT(get_node_level);
	API_FCT(find_nodes_with_meta);
	API_FCT(find_node_near);
	API_FCT(find_nodes_in_area);
	API_FCT(find_nodes_in_area_under_air);
	API_FCT(line_of_sight);
	API_FCT(raycast);
}
