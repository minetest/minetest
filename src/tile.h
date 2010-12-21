/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef TILE_HEADER
#define TILE_HEADER

#include "common_irrlicht.h"
#include "utility.h"

// TileID is supposed to be stored in a u16

enum TileID
{
	TILE_NONE, // Nothing shown
	TILE_STONE,
	TILE_WATER,
	TILE_GRASS,
	TILE_TREE,
	TILE_LEAVES,
	TILE_GRASS_FOOTSTEPS,
	TILE_MESE,
	TILE_MUD,
	TILE_TREE_TOP,
	TILE_MUD_WITH_GRASS,
	TILE_CLOUD,
	
	// Count of tile ids
	TILES_COUNT
};

enum TileSpecialFeature
{
	TILEFEAT_NONE,
	TILEFEAT_CRACK,
};

struct TileCrackParam
{
	bool operator==(TileCrackParam &other)
	{
		return progression == other.progression;
	}

	u16 progression;
};

struct TileSpec
{
	TileSpec()
	{
		id = TILE_NONE;
		feature = TILEFEAT_NONE;
	}

	bool operator==(TileSpec &other)
	{
		if(id != other.id)
			return false;
		if(feature != other.feature)
			return false;
		if(feature == TILEFEAT_NONE)
			return true;
		if(feature == TILEFEAT_CRACK)
		{
			return param.crack == other.param.crack;
		}
		// Invalid feature
		assert(0);
		return false;
	}

	u16 id; // Id in g_tile_materials, TILE_NONE=none
	enum TileSpecialFeature feature;
	union
	{
		TileCrackParam crack;
	} param;
};

/*extern const char * g_tile_texture_paths[TILES_COUNT];
extern video::SMaterial g_tile_materials[TILES_COUNT];*/

/*
	Functions
*/

const char * tile_texture_path_get(u32 i);

// Initializes g_tile_materials
void tile_materials_preload(IrrlichtWrapper *irrlicht);

video::SMaterial & tile_material_get(u32 i);

#endif
