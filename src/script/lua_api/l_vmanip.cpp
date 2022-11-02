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

#include <map>
#include "lua_api/l_vmanip.h"
#include "lua_api/l_internal.h"
#include "common/c_content.h"
#include "common/c_converter.h"
#include "common/c_packer.h"
#include "emerge.h"
#include "environment.h"
#include "map.h"
#include "mapblock.h"
#include "server.h"
#include "mapgen/mapgen.h"
#include "voxelalgorithms.h"

// garbage collector
int LuaVoxelManip::gc_object(lua_State *L)
{
	LuaVoxelManip *o = *(LuaVoxelManip **)(lua_touserdata(L, 1));
	delete o;

	return 0;
}

int LuaVoxelManip::l_read_from_map(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	MMVManip *vm = o->vm;
	if (vm->isOrphan())
		return 0;

	v3s16 bp1 = getNodeBlockPos(check_v3s16(L, 2));
	v3s16 bp2 = getNodeBlockPos(check_v3s16(L, 3));
	sortBoxVerticies(bp1, bp2);

	vm->initialEmerge(bp1, bp2);

	push_v3s16(L, vm->m_area.MinEdge);
	push_v3s16(L, vm->m_area.MaxEdge);

	return 2;
}

int LuaVoxelManip::l_get_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	bool use_buffer  = lua_istable(L, 2);

	MMVManip *vm = o->vm;

	u32 volume = vm->m_area.getVolume();

	if (use_buffer)
		lua_pushvalue(L, 2);
	else
		lua_createtable(L, volume, 0);

	for (u32 i = 0; i != volume; i++) {
		lua_Integer cid = vm->m_data[i].getContent();
		lua_pushinteger(L, cid);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

int LuaVoxelManip::l_set_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	MMVManip *vm = o->vm;

	if (!lua_istable(L, 2))
		throw LuaError("VoxelManip:set_data called with missing parameter");

	u32 volume = vm->m_area.getVolume();
	for (u32 i = 0; i != volume; i++) {
		lua_rawgeti(L, 2, i + 1);
		content_t c = lua_tointeger(L, -1);

		vm->m_data[i].setContent(c);

		lua_pop(L, 1);
	}

	return 0;
}

int LuaVoxelManip::l_write_to_map(lua_State *L)
{
	MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	bool update_light = !lua_isboolean(L, 2) || readParam<bool>(L, 2);

	GET_ENV_PTR;
	ServerMap *map = &(env->getServerMap());

	std::map<v3s16, MapBlock*> modified_blocks;
	if (o->is_mapgen_vm || !update_light) {
		o->vm->blitBackAll(&modified_blocks);
	} else {
		voxalgo::blit_back_with_light(map, o->vm, &modified_blocks);
	}

	MapEditEvent event;
	event.type = MEET_OTHER;
	for (const auto &it : modified_blocks)
		event.modified_blocks.insert(it.first);
	map->dispatchEvent(event);

	return 0;
}

int LuaVoxelManip::l_get_node_at(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	v3s16 pos        = check_v3s16(L, 2);

	pushnode(L, o->vm->getNodeNoExNoEmerge(pos));
	return 1;
}

int LuaVoxelManip::l_set_node_at(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	v3s16 pos        = check_v3s16(L, 2);
	MapNode n        = readnode(L, 3);

	o->vm->setNodeNoEmerge(pos, n);

	return 0;
}

int LuaVoxelManip::l_update_liquids(lua_State *L)
{
	GET_ENV_PTR;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);

	ServerMap *map = &(env->getServerMap());
	const NodeDefManager *ndef = getServer(L)->getNodeDefManager();
	MMVManip *vm = o->vm;

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

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	if (!o->is_mapgen_vm) {
		warningstream << "VoxelManip:calc_lighting called for a non-mapgen "
			"VoxelManip object" << std::endl;
		return 0;
	}

	const NodeDefManager *ndef = getServer(L)->getNodeDefManager();
	EmergeManager *emerge = getServer(L)->getEmergeManager();
	MMVManip *vm = o->vm;

	v3s16 yblock = v3s16(0, 1, 0) * MAP_BLOCKSIZE;
	v3s16 fpmin  = vm->m_area.MinEdge;
	v3s16 fpmax  = vm->m_area.MaxEdge;
	v3s16 pmin   = lua_istable(L, 2) ? check_v3s16(L, 2) : fpmin + yblock;
	v3s16 pmax   = lua_istable(L, 3) ? check_v3s16(L, 3) : fpmax - yblock;
	bool propagate_shadow = !lua_isboolean(L, 4) || readParam<bool>(L, 4);

	sortBoxVerticies(pmin, pmax);
	if (!vm->m_area.contains(VoxelArea(pmin, pmax)))
		throw LuaError("Specified voxel area out of VoxelManipulator bounds");

	Mapgen mg;
	mg.vm          = vm;
	mg.ndef        = ndef;
	mg.water_level = emerge->mgparams->water_level;

	mg.calcLighting(pmin, pmax, fpmin, fpmax, propagate_shadow);

	return 0;
}

int LuaVoxelManip::l_set_lighting(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	if (!o->is_mapgen_vm) {
		warningstream << "VoxelManip:set_lighting called for a non-mapgen "
			"VoxelManip object" << std::endl;
		return 0;
	}

	if (!lua_istable(L, 2))
		throw LuaError("VoxelManip:set_lighting called with missing parameter");

	u8 light;
	light  = (getintfield_default(L, 2, "day",   0) & 0x0F);
	light |= (getintfield_default(L, 2, "night", 0) & 0x0F) << 4;

	MMVManip *vm = o->vm;

	v3s16 yblock = v3s16(0, 1, 0) * MAP_BLOCKSIZE;
	v3s16 pmin = lua_istable(L, 3) ? check_v3s16(L, 3) : vm->m_area.MinEdge + yblock;
	v3s16 pmax = lua_istable(L, 4) ? check_v3s16(L, 4) : vm->m_area.MaxEdge - yblock;

	sortBoxVerticies(pmin, pmax);
	if (!vm->m_area.contains(VoxelArea(pmin, pmax)))
		throw LuaError("Specified voxel area out of VoxelManipulator bounds");

	Mapgen mg;
	mg.vm = vm;

	mg.setLighting(light, pmin, pmax);

	return 0;
}

int LuaVoxelManip::l_get_light_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	bool use_buffer  = lua_istable(L, 2);

	MMVManip *vm = o->vm;

	u32 volume = vm->m_area.getVolume();

	if (use_buffer)
		lua_pushvalue(L, 2);
	else
		lua_createtable(L, volume, 0);

	for (u32 i = 0; i != volume; i++) {
		lua_Integer light = vm->m_data[i].param1;
		lua_pushinteger(L, light);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

int LuaVoxelManip::l_set_light_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	MMVManip *vm = o->vm;

	if (!lua_istable(L, 2))
		throw LuaError("VoxelManip:set_light_data called with missing "
				"parameter");

	u32 volume = vm->m_area.getVolume();
	for (u32 i = 0; i != volume; i++) {
		lua_rawgeti(L, 2, i + 1);
		u8 light = lua_tointeger(L, -1);

		vm->m_data[i].param1 = light;

		lua_pop(L, 1);
	}

	return 0;
}

int LuaVoxelManip::l_get_param2_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	bool use_buffer  = lua_istable(L, 2);

	MMVManip *vm = o->vm;

	u32 volume = vm->m_area.getVolume();

	if (use_buffer)
		lua_pushvalue(L, 2);
	else
		lua_createtable(L, volume, 0);

	for (u32 i = 0; i != volume; i++) {
		lua_Integer param2 = vm->m_data[i].param2;
		lua_pushinteger(L, param2);
		lua_rawseti(L, -2, i + 1);
	}

	return 1;
}

int LuaVoxelManip::l_set_param2_data(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	MMVManip *vm = o->vm;

	if (!lua_istable(L, 2))
		throw LuaError("VoxelManip:set_param2_data called with missing "
				"parameter");

	u32 volume = vm->m_area.getVolume();
	for (u32 i = 0; i != volume; i++) {
		lua_rawgeti(L, 2, i + 1);
		u8 param2 = lua_tointeger(L, -1);

		vm->m_data[i].param2 = param2;

		lua_pop(L, 1);
	}

	return 0;
}

int LuaVoxelManip::l_update_map(lua_State *L)
{
	return 0;
}

int LuaVoxelManip::l_was_modified(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);
	MMVManip *vm = o->vm;

	lua_pushboolean(L, vm->m_is_dirty);

	return 1;
}

int LuaVoxelManip::l_get_emerged_area(lua_State *L)
{
	NO_MAP_LOCK_REQUIRED;

	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, 1);

	push_v3s16(L, o->vm->m_area.MinEdge);
	push_v3s16(L, o->vm->m_area.MaxEdge);

	return 2;
}

LuaVoxelManip::LuaVoxelManip(MMVManip *mmvm, bool is_mg_vm) :
	is_mapgen_vm(is_mg_vm),
	vm(mmvm)
{
}

LuaVoxelManip::LuaVoxelManip(Map *map) : vm(new MMVManip(map))
{
}

LuaVoxelManip::LuaVoxelManip(Map *map, v3s16 p1, v3s16 p2)
{
	vm = new MMVManip(map);

	v3s16 bp1 = getNodeBlockPos(p1);
	v3s16 bp2 = getNodeBlockPos(p2);
	sortBoxVerticies(bp1, bp2);
	vm->initialEmerge(bp1, bp2);
}

LuaVoxelManip::~LuaVoxelManip()
{
	if (!is_mapgen_vm)
		delete vm;
}

// LuaVoxelManip()
// Creates an LuaVoxelManip and leaves it on top of stack
int LuaVoxelManip::create_object(lua_State *L)
{
	GET_ENV_PTR;

	Map *map = &(env->getMap());
	LuaVoxelManip *o = (lua_istable(L, 1) && lua_istable(L, 2)) ?
		new LuaVoxelManip(map, check_v3s16(L, 1), check_v3s16(L, 2)) :
		new LuaVoxelManip(map);

	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
	return 1;
}

void *LuaVoxelManip::packIn(lua_State *L, int idx)
{
	LuaVoxelManip *o = checkObject<LuaVoxelManip>(L, idx);

	if (o->is_mapgen_vm)
		throw LuaError("nope");
	return o->vm->clone();
}

void LuaVoxelManip::packOut(lua_State *L, void *ptr)
{
	MMVManip *vm = reinterpret_cast<MMVManip*>(ptr);
	if (!L) {
		delete vm;
		return;
	}

	// Associate vmanip with map if the Lua env has one
	Environment *env = getEnv(L);
	if (env)
		vm->reparent(&(env->getMap()));

	LuaVoxelManip *o = new LuaVoxelManip(vm, false);
	*(void **)(lua_newuserdata(L, sizeof(void *))) = o;
	luaL_getmetatable(L, className);
	lua_setmetatable(L, -2);
}

void LuaVoxelManip::Register(lua_State *L)
{
	static const luaL_Reg metamethods[] = {
		{"__gc", gc_object},
		{0, 0}
	};
	registerClass(L, className, methods, metamethods);

	// Can be created from Lua (VoxelManip())
	lua_register(L, className, create_object);

	script_register_packer(L, className, packIn, packOut);
}

const char LuaVoxelManip::className[] = "VoxelManip";
const luaL_Reg LuaVoxelManip::methods[] = {
	luamethod(LuaVoxelManip, read_from_map),
	luamethod(LuaVoxelManip, get_data),
	luamethod(LuaVoxelManip, set_data),
	luamethod(LuaVoxelManip, get_node_at),
	luamethod(LuaVoxelManip, set_node_at),
	luamethod(LuaVoxelManip, write_to_map),
	luamethod(LuaVoxelManip, update_map),
	luamethod(LuaVoxelManip, update_liquids),
	luamethod(LuaVoxelManip, calc_lighting),
	luamethod(LuaVoxelManip, set_lighting),
	luamethod(LuaVoxelManip, get_light_data),
	luamethod(LuaVoxelManip, set_light_data),
	luamethod(LuaVoxelManip, get_param2_data),
	luamethod(LuaVoxelManip, set_param2_data),
	luamethod(LuaVoxelManip, was_modified),
	luamethod(LuaVoxelManip, get_emerged_area),
	{0,0}
};
