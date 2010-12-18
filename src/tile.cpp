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

const char * g_tile_texture_names[TILES_COUNT] =
{
	NULL,
	"stone",
	"water",
	"grass",
	"tree",
	"leaves",
	"grass_footsteps",
	"mese",
	"mud",
	"tree_top",
	"mud_with_grass",
	"cloud",
};

video::SMaterial g_tile_materials[TILES_COUNT];

void tile_materials_preload(TextureCache &cache)
{
	for(s32 i=0; i<TILES_COUNT; i++)
	{
		const char *name = g_tile_texture_names[i];

		video::ITexture *t = NULL;

		if(name != NULL)
		{
			t = cache.get(name);
			assert(t != NULL);
		}

		g_tile_materials[i].Lighting = false;
		g_tile_materials[i].BackfaceCulling = false;
		g_tile_materials[i].setFlag(video::EMF_BILINEAR_FILTER, false);
		g_tile_materials[i].setFlag(video::EMF_ANTI_ALIASING, video::EAAM_OFF);
		//if(i != TILE_WATER)
		//g_tile_materials[i].setFlag(video::EMF_FOG_ENABLE, true);
		
		//g_tile_materials[i].setFlag(video::EMF_TEXTURE_WRAP, video::ETC_REPEAT);
		//g_tile_materials[i].setFlag(video::EMF_ANISOTROPIC_FILTER, false);

		g_tile_materials[i].setTexture(0, t);
	}
	
	g_tile_materials[TILE_WATER].MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	//g_tile_materials[TILE_WATER].MaterialType = video::EMT_TRANSPARENT_ADD_COLOR;
}

