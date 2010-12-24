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

#include "tile.h"
#include "main.h"

// A mapping from tiles to paths of textures
const char * g_tile_texture_paths[TILES_COUNT] =
{
	NULL,
	"../data/stone.png",
	"../data/water.png",
	"../data/grass.png",
	"../data/tree.png",
	"../data/leaves.png",
	"../data/grass_footsteps.png",
	"../data/mese.png",
	"../data/mud.png",
	"../data/tree_top.png",
	"../data/mud_with_grass.png",
	"../data/cloud.png",
	"../data/coalstone.png",
	"../data/wood.png",
};

const char * tile_texture_path_get(u32 i)
{
	assert(i < TILES_COUNT);

	return g_tile_texture_paths[i];
}

// A mapping from tiles to materials
// Initialized at run-time.
video::SMaterial g_tile_materials[TILES_COUNT];

void tile_materials_preload(IrrlichtWrapper *irrlicht)
{
	for(s32 i=0; i<TILES_COUNT; i++)
	{
		const char *path = g_tile_texture_paths[i];

		video::ITexture *t = NULL;

		if(path != NULL)
		{
			t = irrlicht->getTexture(path);
			assert(t != NULL);
		}

		g_tile_materials[i].Lighting = false;
		g_tile_materials[i].BackfaceCulling = false;
		g_tile_materials[i].setFlag(video::EMF_BILINEAR_FILTER, false);
		g_tile_materials[i].setFlag(video::EMF_ANTI_ALIASING, video::EAAM_OFF);
		//if(i != TILE_WATER)
		g_tile_materials[i].setFlag(video::EMF_FOG_ENABLE, true);
		
		//g_tile_materials[i].setFlag(video::EMF_TEXTURE_WRAP, video::ETC_REPEAT);
		//g_tile_materials[i].setFlag(video::EMF_ANISOTROPIC_FILTER, false);

		g_tile_materials[i].setTexture(0, t);
	}
	
	g_tile_materials[TILE_WATER].MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	//g_tile_materials[TILE_WATER].MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
}

video::SMaterial & tile_material_get(u32 i)
{
	assert(i < TILES_COUNT);

	return g_tile_materials[i];
}

