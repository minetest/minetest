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
	
	// Count of tile ids
	TILES_COUNT
};

// A mapping from tiles to names of cached textures
extern const char * g_tile_texture_names[TILES_COUNT];

// A mapping from tiles to materials
// Initialized at run-time.
extern video::SMaterial g_tile_materials[TILES_COUNT];

/*
	Functions
*/

// Initializes g_tile_materials
void tile_materials_preload(TextureCache &cache);

#endif
