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

#include "lua_api/l_mapgen.h"
#include "lua_api/l_internal.h"
#include "lua_api/l_vmanip.h"
#include "common/c_converter.h"
#include "common/c_content.h"
#include "util/serialize.h"
#include "server.h"
#include "environment.h"
#include "emerge.h"
#include "mg_biome.h"
#include "mg_ore.h"
#include "mg_decoration.h"
#include "mg_schematic.h"
#include "mapgen_v5.h"
#include "mapgen_v7.h"
#include "settings.h"
#include "main.h"
#include "log.h"


struct EnumString ModApiMapgen::es_BiomeTerrainType[] =
{
	{BIOME_TYPE_NORMAL, "normal"},
	{BIOME_TYPE_LIQUID, "liquid"},
	{BIOME_TYPE_NETHER, "nether"},
	{BIOME_TYPE_AETHER, "aether"},
	{BIOME_TYPE_FLAT,   "flat"},
	{0, NULL},
};

struct EnumString ModApiMapgen::es_DecorationType[] =
{
	{DECO_SIMPLE,    "simple"},
	{DECO_SCHEMATIC, "schematic"},
	{DECO_LSYSTEM,   "lsystem"},
	{0, NULL},
};

struct EnumString ModApiMapgen::es_MapgenObject[] =
{
	{MGOBJ_VMANIP,    "voxelmanip"},
	{MGOBJ_HEIGHTMAP, "heightmap"},
	{MGOBJ_BIOMEMAP,  "biomemap"},
	{MGOBJ_HEATMAP,   "heatmap"},
	{MGOBJ_HUMIDMAP,  "humiditymap"},
	{MGOBJ_GENNOTIFY, "gennotify"},
	{0, NULL},
};

struct EnumString ModApiMapgen::es_OreType[] =
{
	{ORE_SCATTER,  "scatter"},
	{ORE_SHEET,    "sheet"},
	{ORE_CLAYLIKE, "claylike"},
	{0, NULL},
};

struct EnumString ModApiMapgen::es_Rotation[] =
{
	{ROTATE_0,    "0"},
	{ROTATE_90,   "90"},
	{ROTATE_180,  "180"},
	{ROTATE_270,  "270"},
	{ROTATE_RAND, "random"},
	{0, NULL},
};


///////////////////////////////////////////////////////////////////////////////


bool read_schematic(lua_State *L, int index, Schematic *schem,
	INodeDefManager *ndef, std::map<std::string, std::string> &replace_names)
{
	//// Get schematic size
	lua_getfield(L, index, "size");
	v3s16 size = read_v3s16(L, -1);
	lua_pop(L, 1);

	//// Get schematic data
	lua_getfield(L, index, "data");
	luaL_checktype(L, -1, LUA_TTABLE);

	int numnodes = size.X * size.Y * size.Z;
	MapNode *schemdata = new MapNode[numnodes];
	int i = 0;

	lua_pushnil(L);
	while (lua_next(L, -2)) {
		if (i >= numnodes) {
			i++;
			lua_pop(L, 1);
			continue;
		}

		// same as readnode, except param1 default is MTSCHEM_PROB_CONST
		lua_getfield(L, -1, "name");
		std::string name = luaL_checkstring(L, -1);
		lua_pop(L, 1);

		u8 param1;
		lua_getfield(L, -1, "param1");
		param1 = !lua_isnil(L, -1) ? lua_tonumber(L, -1) : MTSCHEM_PROB_ALWAYS;
		lua_pop(L, 1);

		u8 param2;
		lua_getfield(L, -1, "param2");
		param2 = !lua_isnil(L, -1) ? lua_tonumber(L, -1) : 0;
		lua_pop(L, 1);

		std::map<std::string, std::string>::iterator it;
		it = replace_names.find(name);
		if (it != replace_names.end())
			name = it->second;

		schemdata[i] = MapNode(ndef, name, param1, param2);

		i++;
		lua_pop(L, 1);
	}

	if (i != numnodes) {
		errorstream << "read_schematic: incorrect number of "
			"nodes provided in raw schematic data (got " << i <<
			", expected " << numnodes << ")." << std::endl;
		delete schemdata;
		return false;
	}

	//// Get Y-slice probability values (if present)
	u8 *slice_probs = new u8[size.Y];
	for (i = 0; i != size.Y; i++)
		slice_probs[i] = MTSCHEM_PROB_ALWAYS;

	lua_getfield(L, index, "yslice_prob");
	if (lua_istable(L, -1)) {
		lua_pushnil(L);
		while (lua_next(L, -2)) {
			if (getintfield(L, -1, "ypos", i) && i >= 0 && i < size.Y) {
				slice_probs[i] = getintfield_default(L, -1,
					"prob", MTSCHEM_PROB_ALWAYS);
			}
			lua_pop(L, 1);
		}
	}

	// Here, we read the nodes directly from the INodeDefManager - there is no
	// need for pending node resolutions so we'll mark this schematic as updated
	schem->flags       = SCHEM_CIDS_UPDATED;

	schem->size        = size;
	schem->schemdata   = schemdata;
	schem->slice_probs = slice_probs;
	return true;
}


bool get_schematic(lua_State *L, int index, Schematic *schem,
	INodeDefManager *ndef, std::map<std::string, std::string> &replace_names)
{
	if (index < 0)
		index = lua_gettop(L) + 1 + index;

	if (lua_istable(L, index)) {
		return read_schematic(L, index, schem, ndef, replace_names);
	} else if (lua_isstring(L, index)) {
		NodeResolver *resolver = ndef->getResolver();
		const char *filename = lua_tostring(L, index);
		return schem->loadSchematicFromFile(filename, resolver, replace_names);
	} else {
		return false;
	}
}


void read_schematic_replacements(lua_State *L,
	std::map<std::string, std::string> &replace_names, int index)
{
	lua_pushnil(L);
	while (lua_next(L, index)) {
		std::string replace_from;
		std::string replace_to;

		if (lua_istable(L, -1)) { // Old {{"x", "y"}, ...} format
			lua_rawgeti(L, -1, 1);
			replace_from = lua_tostring(L, -1);
			lua_pop(L, 1);

			lua_rawgeti(L, -1, 2);
			replace_to = lua_tostring(L, -1);
			lua_pop(L, 1);
		} else { // New {x = "y", ...} format
			replace_from = lua_tostring(L, -2);
			replace_to = lua_tostring(L, -1);
		}

		replace_names[replace_from] = replace_to;
		lua_pop(L, 1);
	}
}


// get_mapgen_object(objectname)
// returns the requested object used during map generation
int ModApiMapgen::l_get_mapgen_object(lua_State *L)
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

			return 3;
		}
		case MGOBJ_HEIGHTMAP: {
			if (!mg->heightmap)
				return 0;

			lua_newtable(L);
			for (size_t i = 0; i != maplen; i++) {
				lua_pushinteger(L, mg->heightmap[i]);
				lua_rawseti(L, -2, i + 1);
			}

			return 1;
		}
		case MGOBJ_BIOMEMAP: {
			if (!mg->biomemap)
				return 0;

			lua_newtable(L);
			for (size_t i = 0; i != maplen; i++) {
				lua_pushinteger(L, mg->biomemap[i]);
				lua_rawseti(L, -2, i + 1);
			}

			return 1;
		}
		case MGOBJ_HEATMAP: { // Mapgen V7 specific objects
		case MGOBJ_HUMIDMAP:
			if (strcmp(emerge->params.mg_name.c_str(), "v7"))
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

			return 1;
		}
		case MGOBJ_GENNOTIFY: {
			std::map<std::string, std::vector<v3s16> >event_map;
			std::map<std::string, std::vector<v3s16> >::iterator it;

			mg->gennotify.getEvents(event_map);

			lua_newtable(L);
			for (it = event_map.begin(); it != event_map.end(); ++it) {
				lua_newtable(L);

				for (size_t j = 0; j != it->second.size(); j++) {
					push_v3s16(L, it->second[j]);
					lua_rawseti(L, -2, j + 1);
				}

				lua_setfield(L, -2, it->first.c_str());
			}

			return 1;
		}
	}

	return 0;
}

// set_mapgen_params(params)
// set mapgen parameters
int ModApiMapgen::l_set_mapgen_params(lua_State *L)
{
	if (!lua_istable(L, 1))
		return 0;

	EmergeManager *emerge = getServer(L)->getEmergeManager();
	assert(emerge);

	std::string flagstr;
	u32 flags = 0, flagmask = 0;

	lua_getfield(L, 1, "mgname");
	if (lua_isstring(L, -1)) {
		emerge->params.mg_name = std::string(lua_tostring(L, -1));
		delete emerge->params.sparams;
		emerge->params.sparams = NULL;
	}

	lua_getfield(L, 1, "seed");
	if (lua_isnumber(L, -1))
		emerge->params.seed = lua_tointeger(L, -1);

	lua_getfield(L, 1, "water_level");
	if (lua_isnumber(L, -1))
		emerge->params.water_level = lua_tointeger(L, -1);

	lua_getfield(L, 1, "flagmask");
	if (lua_isstring(L, -1)) {
		flagstr = lua_tostring(L, -1);
		emerge->params.flags &= ~readFlagString(flagstr, flagdesc_mapgen, NULL);
		errorstream << "set_mapgen_params(): flagmask field is deprecated, "
			"see lua_api.txt" << std::endl;
	}

	if (getflagsfield(L, 1, "flags", flagdesc_mapgen, &flags, &flagmask)) {
		emerge->params.flags &= ~flagmask;
		emerge->params.flags |= flags;
	}

	return 0;
}

// set_noiseparams(name, noiseparams, set_default)
// set global config values for noise parameters
int ModApiMapgen::l_set_noiseparams(lua_State *L)
{
	const char *name = luaL_checkstring(L, 1);

	NoiseParams np;
	if (!read_noiseparams(L, 2, &np))
		return 0;

	bool set_default = lua_isboolean(L, 3) ? lua_toboolean(L, 3) : true;

	g_settings->setNoiseParams(name, np, set_default);

	return 0;
}

// set_gen_notify(flags, {deco_id_table})
int ModApiMapgen::l_set_gen_notify(lua_State *L)
{
	u32 flags = 0, flagmask = 0;
	EmergeManager *emerge = getServer(L)->getEmergeManager();

	if (read_flags(L, 1, flagdesc_gennotify, &flags, &flagmask)) {
		emerge->gen_notify_on &= ~flagmask;
		emerge->gen_notify_on |= flags;
	}

	if (lua_istable(L, 2)) {
		lua_pushnil(L);
		while (lua_next(L, 2)) {
			if (lua_isnumber(L, -1))
				emerge->gen_notify_on_deco_ids.insert(lua_tonumber(L, -1));
			lua_pop(L, 1);
		}
	}

	return 0;
}

// register_biome({lots of stuff})
int ModApiMapgen::l_register_biome(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	NodeResolver *resolver = getServer(L)->getNodeDefManager()->getResolver();
	BiomeManager *bmgr     = getServer(L)->getEmergeManager()->biomemgr;

	enum BiomeType biometype = (BiomeType)getenumfield(L, index, "type",
		es_BiomeTerrainType, BIOME_TYPE_NORMAL);
	Biome *b = bmgr->create(biometype);

	b->name           = getstringfield_default(L, index, "name", "");
	b->depth_top      = getintfield_default(L, index, "depth_top",    1);
	b->depth_filler   = getintfield_default(L, index, "depth_filler", 3);
	b->height_min     = getintfield_default(L, index, "height_min",   0);
	b->height_max     = getintfield_default(L, index, "height_max",   0);
	b->heat_point     = getfloatfield_default(L, index, "heat_point",     0.);
	b->humidity_point = getfloatfield_default(L, index, "humidity_point", 0.);
	b->flags          = 0; //reserved

	u32 id = bmgr->add(b);
	if (id == (u32)-1) {
		delete b;
		return 0;
	}

	// Pend node resolutions only if insertion succeeded
	resolver->addNode(getstringfield_default(L, index, "node_top", ""),
		 "mapgen_dirt_with_grass", CONTENT_AIR, &b->c_top);
	resolver->addNode(getstringfield_default(L, index, "node_filler", ""),
		"mapgen_dirt", CONTENT_AIR, &b->c_filler);
	resolver->addNode(getstringfield_default(L, index, "node_stone", ""),
		"mapgen_stone", CONTENT_AIR, &b->c_stone);
	resolver->addNode(getstringfield_default(L, index, "node_water", ""),
		"mapgen_water_source", CONTENT_AIR, &b->c_water);
	resolver->addNode(getstringfield_default(L, index, "node_dust", ""),
		"air", CONTENT_IGNORE, &b->c_dust);
	resolver->addNode(getstringfield_default(L, index, "node_dust_water", ""),
		"mapgen_water_source", CONTENT_IGNORE, &b->c_dust_water);

	verbosestream << "register_biome: " << b->name << std::endl;

	lua_pushinteger(L, id);
	return 1;
}

int ModApiMapgen::l_clear_registered_biomes(lua_State *L)
{
	BiomeManager *bmgr = getServer(L)->getEmergeManager()->biomemgr;
	bmgr->clear();
	return 0;
}

int ModApiMapgen::l_clear_registered_decorations(lua_State *L)
{
	DecorationManager *dmgr = getServer(L)->getEmergeManager()->decomgr;
	dmgr->clear();
	return 0;
}

int ModApiMapgen::l_clear_registered_ores(lua_State *L)
{
	OreManager *omgr = getServer(L)->getEmergeManager()->oremgr;
	omgr->clear();
	return 0;
}

// register_decoration({lots of stuff})
int ModApiMapgen::l_register_decoration(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	INodeDefManager *ndef      = getServer(L)->getNodeDefManager();
	NodeResolver *resolver     = getServer(L)->getNodeDefManager()->getResolver();
	DecorationManager *decomgr = getServer(L)->getEmergeManager()->decomgr;
	BiomeManager *biomemgr     = getServer(L)->getEmergeManager()->biomemgr;

	enum DecorationType decotype = (DecorationType)getenumfield(L, index,
				"deco_type", es_DecorationType, -1);

	Decoration *deco = decomgr->create(decotype);
	if (!deco) {
		errorstream << "register_decoration: decoration placement type "
			<< decotype << " not implemented";
		return 0;
	}

	deco->name       = getstringfield_default(L, index, "name", "");
	deco->fill_ratio = getfloatfield_default(L, index, "fill_ratio", 0.02);
	deco->sidelen    = getintfield_default(L, index, "sidelen", 8);
	if (deco->sidelen <= 0) {
		errorstream << "register_decoration: sidelen must be "
			"greater than 0" << std::endl;
		delete deco;
		return 0;
	}

	//// Get node name(s) to place decoration on
	std::vector<const char *> place_on_names;
	getstringlistfield(L, index, "place_on", place_on_names);
	for (size_t i = 0; i != place_on_names.size(); i++)
		resolver->addNodeList(place_on_names[i], &deco->c_place_on);

	getflagsfield(L, index, "flags", flagdesc_deco, &deco->flags, NULL);

	//// Get NoiseParams to define how decoration is placed
	lua_getfield(L, index, "noise_params");
	if (read_noiseparams(L, -1, &deco->np))
		deco->flags |= DECO_USE_NOISE;
	lua_pop(L, 1);

	//// Get biomes associated with this decoration (if any)
	std::vector<const char *> biome_list;
	getstringlistfield(L, index, "biomes", biome_list);
	for (size_t i = 0; i != biome_list.size(); i++) {
		Biome *b = (Biome *)biomemgr->getByName(biome_list[i]);
		if (!b)
			continue;

		deco->biomes.insert(b->id);
	}

	//// Handle decoration type-specific parameters
	bool success = false;
	switch (decotype) {
		case DECO_SIMPLE:
			success = regDecoSimple(L, resolver, (DecoSimple *)deco);
			break;
		case DECO_SCHEMATIC:
			success = regDecoSchematic(L, ndef, (DecoSchematic *)deco);
			break;
		case DECO_LSYSTEM:
			break;
	}

	if (!success) {
		delete deco;
		return 0;
	}

	u32 id = decomgr->add(deco);
	if (id == (u32)-1) {
		delete deco;
		return 0;
	}

	lua_pushinteger(L, id);
	return 1;
}

bool ModApiMapgen::regDecoSimple(lua_State *L,
		NodeResolver *resolver, DecoSimple *deco)
{
	int index = 1;

	deco->deco_height     = getintfield_default(L, index, "height", 1);
	deco->deco_height_max = getintfield_default(L, index, "height_max", 0);
	deco->nspawnby        = getintfield_default(L, index, "num_spawn_by", -1);

	if (deco->deco_height <= 0) {
		errorstream << "register_decoration: simple decoration height"
			" must be greater than 0" << std::endl;
		return false;
	}

	std::vector<const char *> deco_names;
	getstringlistfield(L, index, "decoration", deco_names);
	if (deco_names.size() == 0) {
		errorstream << "register_decoration: no decoration nodes "
			"defined" << std::endl;
		return false;
	}

	std::vector<const char *> spawnby_names;
	getstringlistfield(L, index, "spawn_by", spawnby_names);
	if (deco->nspawnby != -1 && spawnby_names.size() == 0) {
		errorstream << "register_decoration: no spawn_by nodes defined,"
			" but num_spawn_by specified" << std::endl;
		return false;
	}

	for (size_t i = 0; i != deco_names.size(); i++)
		resolver->addNodeList(deco_names[i], &deco->c_decos);
	for (size_t i = 0; i != spawnby_names.size(); i++)
		resolver->addNodeList(spawnby_names[i], &deco->c_spawnby);

	return true;
}

bool ModApiMapgen::regDecoSchematic(lua_State *L, INodeDefManager *ndef,
	DecoSchematic *deco)
{
	int index = 1;

	deco->rotation = (Rotation)getenumfield(L, index, "rotation",
		es_Rotation, ROTATE_0);

	std::map<std::string, std::string> replace_names;
	lua_getfield(L, index, "replacements");
	if (lua_istable(L, -1))
		read_schematic_replacements(L, replace_names, lua_gettop(L));
	lua_pop(L, 1);

	Schematic *schem = new Schematic;
	lua_getfield(L, index, "schematic");
	if (!get_schematic(L, -1, schem, ndef, replace_names)) {
		lua_pop(L, 1);
		delete schem;
		return false;
	}
	lua_pop(L, 1);

	deco->schematic = schem;

	return true;
}

// register_ore({lots of stuff})
int ModApiMapgen::l_register_ore(lua_State *L)
{
	int index = 1;
	luaL_checktype(L, index, LUA_TTABLE);

	NodeResolver *resolver = getServer(L)->getNodeDefManager()->getResolver();
	OreManager *oremgr     = getServer(L)->getEmergeManager()->oremgr;

	enum OreType oretype = (OreType)getenumfield(L, index,
				"ore_type", es_OreType, ORE_SCATTER);
	Ore *ore = oremgr->create(oretype);
	if (!ore) {
		errorstream << "register_ore: ore_type " << oretype << " not implemented";
		return 0;
	}

	ore->name           = getstringfield_default(L, index, "name", "");
	ore->ore_param2     = (u8)getintfield_default(L, index, "ore_param2", 0);
	ore->clust_scarcity = getintfield_default(L, index, "clust_scarcity", 1);
	ore->clust_num_ores = getintfield_default(L, index, "clust_num_ores", 1);
	ore->clust_size     = getintfield_default(L, index, "clust_size", 0);
	ore->height_min     = getintfield_default(L, index, "height_min", 0);
	ore->height_max     = getintfield_default(L, index, "height_max", 0);
	ore->nthresh        = getfloatfield_default(L, index, "noise_threshhold", 0);
	ore->noise          = NULL;
	ore->flags          = 0;

	if (ore->clust_scarcity <= 0 || ore->clust_num_ores <= 0) {
		errorstream << "register_ore: clust_scarcity and clust_num_ores"
			"must be greater than 0" << std::endl;
		delete ore;
		return 0;
	}

	getflagsfield(L, index, "flags", flagdesc_ore, &ore->flags, NULL);

	lua_getfield(L, index, "noise_params");
	if (read_noiseparams(L, -1, &ore->np)) {
		ore->flags |= OREFLAG_USE_NOISE;
	} else if (ore->NEEDS_NOISE) {
		errorstream << "register_ore: specified ore type requires valid "
			"noise parameters" << std::endl;
		delete ore;
		return 0;
	}
	lua_pop(L, 1);

	u32 id = oremgr->add(ore);
	if (id == (u32)-1) {
		delete ore;
		return 0;
	}

	std::vector<const char *> wherein_names;
	getstringlistfield(L, index, "wherein", wherein_names);
	for (size_t i = 0; i != wherein_names.size(); i++)
		resolver->addNodeList(wherein_names[i], &ore->c_wherein);

	resolver->addNode(getstringfield_default(L, index, "ore", ""),
		"", CONTENT_AIR, &ore->c_ore);

	lua_pushinteger(L, id);
	return 1;
}

// create_schematic(p1, p2, probability_list, filename)
int ModApiMapgen::l_create_schematic(lua_State *L)
{
	Schematic schem;

	Map *map = &(getEnv(L)->getMap());
	INodeDefManager *ndef = getServer(L)->getNodeDefManager();

	v3s16 p1 = read_v3s16(L, 1);
	v3s16 p2 = read_v3s16(L, 2);
	sortBoxVerticies(p1, p2);

	std::vector<std::pair<v3s16, u8> > prob_list;
	if (lua_istable(L, 3)) {
		lua_pushnil(L);
		while (lua_next(L, 3)) {
			if (lua_istable(L, -1)) {
				lua_getfield(L, -1, "pos");
				v3s16 pos = read_v3s16(L, -1);
				lua_pop(L, 1);

				u8 prob = getintfield_default(L, -1, "prob", MTSCHEM_PROB_ALWAYS);
				prob_list.push_back(std::make_pair(pos, prob));
			}

			lua_pop(L, 1);
		}
	}

	std::vector<std::pair<s16, u8> > slice_prob_list;
	if (lua_istable(L, 5)) {
		lua_pushnil(L);
		while (lua_next(L, 5)) {
			if (lua_istable(L, -1)) {
				s16 ypos = getintfield_default(L, -1, "ypos", 0);
				u8 prob  = getintfield_default(L, -1, "prob", MTSCHEM_PROB_ALWAYS);
				slice_prob_list.push_back(std::make_pair(ypos, prob));
			}

			lua_pop(L, 1);
		}
	}

	const char *filename = luaL_checkstring(L, 4);

	if (!schem.getSchematicFromMap(map, p1, p2)) {
		errorstream << "create_schematic: failed to get schematic "
			"from map" << std::endl;
		return 0;
	}

	schem.applyProbabilities(p1, &prob_list, &slice_prob_list);

	schem.saveSchematicToFile(filename, ndef);
	actionstream << "create_schematic: saved schematic file '"
		<< filename << "'." << std::endl;

	return 1;
}

// place_schematic(p, schematic, rotation, replacement)
int ModApiMapgen::l_place_schematic(lua_State *L)
{
	Schematic schem;

	Map *map = &(getEnv(L)->getMap());
	INodeDefManager *ndef = getServer(L)->getNodeDefManager();

	//// Read position
	v3s16 p = read_v3s16(L, 1);

	//// Read rotation
	int rot = ROTATE_0;
	if (lua_isstring(L, 3))
		string_to_enum(es_Rotation, rot, std::string(lua_tostring(L, 3)));

	//// Read force placement
	bool force_placement = true;
	if (lua_isboolean(L, 5))
		force_placement = lua_toboolean(L, 5);

	//// Read node replacements
	std::map<std::string, std::string> replace_names;
	if (lua_istable(L, 4))
		read_schematic_replacements(L, replace_names, 4);

	//// Read schematic
	if (!get_schematic(L, 2, &schem, ndef, replace_names)) {
		errorstream << "place_schematic: failed to get schematic" << std::endl;
		return 0;
	}

	schem.placeStructure(map, p, 0, (Rotation)rot, force_placement, ndef);

	return 1;
}

void ModApiMapgen::Initialize(lua_State *L, int top)
{
	API_FCT(get_mapgen_object);

	API_FCT(set_mapgen_params);
	API_FCT(set_noiseparams);
	API_FCT(set_gen_notify);

	API_FCT(register_biome);
	API_FCT(register_decoration);
	API_FCT(register_ore);

	API_FCT(clear_registered_biomes);
	API_FCT(clear_registered_decorations);
	API_FCT(clear_registered_ores);

	API_FCT(create_schematic);
	API_FCT(place_schematic);
}
