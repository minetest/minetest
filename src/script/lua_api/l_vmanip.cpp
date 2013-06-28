/*
Minetest
Copyright (C) 2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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


#include "lua_api/l_base.h"
#include "lua_api/l_vmanip.h"

///////

#include "cpp_api/scriptapi.h"
#include "common/c_converter.h"
#include "server.h"
#include "emerge.h"
#include "common/c_internal.h"

// garbage collector
int LuaVoxelManip::gc_object(lua_State *L)
{
	LuaVoxelManip *o = *(LuaVoxelManip **)(lua_touserdata(L, 1));
	if (o->do_gc)
		delete o;
	
	return 0;
}

int LuaVoxelManip::l_read_from_map(lua_State *L)
{
	LuaVoxelManip *o = checkobject(L, 1);
	ManualMapVoxelManipulator *vm = o->vm;
	
	v3s16 bp1 = getNodeBlockPos(read_v3s16(L, 2));
	v3s16 bp2 = getNodeBlockPos(read_v3s16(L, 3));
	sortBoxVerticies(bp1, bp2);
	
	vm->initialEmerge(bp1, bp2);
	
	push_v3s16(L, vm->m_area.MinEdge);
	push_v3s16(L, vm->m_area.MaxEdge);
	
	return 2;
}

int LuaVoxelManip::l_get_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkobject(L, 1);
	ManualMapVoxelManipulator *vm = o->vm;
	
	int volume = vm->m_area.getVolume();
	
	lua_newtable(L);
	for (int i = 0; i != volume; i++) {
		lua_Integer cid = vm->m_data[i].getContent();
		lua_pushinteger(L, cid);
		lua_rawseti(L, -2, i + 1);
	}
	
	return 1;
}

int LuaVoxelManip::l_set_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	
	LuaVoxelManip *o = checkobject(L, 1);
	ManualMapVoxelManipulator *vm = o->vm;
	
	if (!lua_istable(L, 2))
		return 0;
	
	int volume = vm->m_area.getVolume();
	for (int i = 0; i != volume; i++) {
		lua_rawgeti(L, 2, i + 1);
		content_t c = lua_tointeger(L, -1);
		
		vm->m_data[i].setContent(c);

		lua_pop(L, 1);
	}
		
	return 0;
}

int LuaVoxelManip::l_write_to_map(lua_State *L)
{
	LuaVoxelManip *o = checkobject(L, 1);
	ManualMapVoxelManipulator *vm = o->vm;

	vm->blitBackAll(&o->modified_blocks);

	return 0;	
}

int LuaVoxelManip::l_update_liquids(lua_State *L)
{
	LuaVoxelManip *o = checkobject(L, 1);
	
	ManualMapVoxelManipulator *vm = o->vm;
	INodeDefManager *ndef = STACK_TO_SERVER(L)->getNodeDefManager();
	Map *map = &(get_scriptapi(L)->getEnv()->getMap());

	Mapgen mg;
	mg.vm   = vm;
	mg.ndef = ndef;

	mg.updateLiquid(&map->m_transforming_liquid,
			vm->m_area.MinEdge, vm->m_area.MaxEdge);

	return 0;
}

int LuaVoxelManip::l_calc_lighting(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	
	LuaVoxelManip *o = checkobject(L, 1);
	v3s16 p1 = read_v3s16(L, 2);
	v3s16 p2 = read_v3s16(L, 3);
	sortBoxVerticies(p1, p2);
	
	ManualMapVoxelManipulator *vm = o->vm;
	INodeDefManager *ndef = STACK_TO_SERVER(L)->getNodeDefManager();
	EmergeManager *emerge = STACK_TO_SERVER(L)->getEmergeManager();
	
	Mapgen mg;
	mg.vm          = vm;
	mg.ndef        = ndef;
	mg.water_level = emerge->params->water_level;
	
	mg.calcLighting(p1, p2);

	return 0;
}

int LuaVoxelManip::l_set_lighting(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	
	LuaVoxelManip *o = checkobject(L, 1);
	v3s16 p1 = read_v3s16(L, 2);
	v3s16 p2 = read_v3s16(L, 3);
	sortBoxVerticies(p1, p2);
	
	u8 light;
	if (!lua_istable(L, 4))
		return 0;

	light  = getintfield_default(L, 4, "day", 0);
	light |= getintfield_default(L, 4, "night", 0);
	
	ManualMapVoxelManipulator *vm = o->vm;
	
	Mapgen mg;
	mg.vm = vm;
	
	mg.setLighting(p1, p2, light);

	return 0;
}

int LuaVoxelManip::l_update_map(lua_State *L)
{
	LuaVoxelManip *o = checkobject(L, 1);
	
	// TODO: Optimize this by using Mapgen::calcLighting() instead
	std::map<v3s16, MapBlock *> lighting_mblocks;
	std::map<v3s16, MapBlock *> *mblocks = &o->modified_blocks;
	
	lighting_mblocks.insert(mblocks->begin(), mblocks->end());
	
	Map *map = &(get_scriptapi(L)->getEnv()->getMap());
	map->updateLighting(lighting_mblocks, *mblocks);

	MapEditEvent event;
	event.type = MEET_OTHER;
	for (std::map<v3s16, MapBlock *>::iterator
		it = mblocks->begin();
		it != mblocks->end(); ++it)
		event.modified_blocks.insert(it->first);
		
	map->dispatchEvent(&event);

	mblocks->clear();

	return 0;	
}

LuaVoxelManip::LuaVoxelManip(ManualMapVoxelManipulator *mmvm, bool dogc)
{
	this->vm    = mmvm;
	this->do_gc = dogc;
}

LuaVoxelManip::LuaVoxelManip(Map *map)
{
	vm = new ManualMapVoxelManipulator(map);
}

LuaVoxelManip::~LuaVoxelManip()
{
	delete vm;
}

// LuaVoxelManip()
// Creates an LuaVoxelManip and leaves it on top of stack
int LuaVoxelManip::create_object(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;
	
	Environment *env = get_scriptapi(L)->getEnv();
	if (!env)
		return 0;
		
	Map *map = &(env->getMap());
	LuaVoxelManip *o = new LuaVoxelManip(map);
	
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

LuaVoxelManip *LuaVoxelManip::checkobject(lua_State *L, int narg)
{
	NO_MAP_LOCK_REQUIRED;
	
	luaL_checktype(L, narg, LUA_TUSERDATA);

	void *ud = luaL_checkudata(L, narg, className);
	if (!ud)
		luaL_typerror(L, narg, className);
	
	return *(LuaVoxelManip **)ud;  // unbox pointer
}

void LuaVoxelManip::Register(lua_State *L)
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

	// Can be created from Lua (VoxelManip()
	lua_register(L, className, create_object);
}

const char LuaVoxelManip::className[] = "VoxelManip";
const luaL_reg LuaVoxelManip::methods[] = {
	luamethod(LuaVoxelManip, read_from_map),
	luamethod(LuaVoxelManip, get_data),
	luamethod(LuaVoxelManip, set_data),
	luamethod(LuaVoxelManip, write_to_map),
	luamethod(LuaVoxelManip, update_map),
	luamethod(LuaVoxelManip, update_liquids),
	luamethod(LuaVoxelManip, calc_lighting),
	luamethod(LuaVoxelManip, set_lighting),
	{0,0}
};

REGISTER_LUA_REF(LuaVoxelManip);
