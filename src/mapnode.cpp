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

#include "mapnode.h"
#include "tile.h"

/*
	Face directions:
		0: up
		1: down
		2: right
		3: left
		4: back
		5: front
*/
u16 g_content_tiles[USEFUL_CONTENT_COUNT][6] =
{
	{TILE_STONE,TILE_STONE,TILE_STONE,TILE_STONE,TILE_STONE,TILE_STONE},
	{TILE_GRASS,TILE_MUD,TILE_MUD_WITH_GRASS,TILE_MUD_WITH_GRASS,TILE_MUD_WITH_GRASS,TILE_MUD_WITH_GRASS},
	{TILE_WATER,TILE_WATER,TILE_WATER,TILE_WATER,TILE_WATER,TILE_WATER},
	{TILE_NONE,TILE_NONE,TILE_NONE,TILE_NONE,TILE_NONE,TILE_NONE},
	{TILE_TREE_TOP,TILE_TREE_TOP,TILE_TREE,TILE_TREE,TILE_TREE,TILE_TREE},
	{TILE_LEAVES,TILE_LEAVES,TILE_LEAVES,TILE_LEAVES,TILE_LEAVES,TILE_LEAVES},
	{TILE_GRASS_FOOTSTEPS,TILE_MUD,TILE_MUD_WITH_GRASS,TILE_MUD_WITH_GRASS,TILE_MUD_WITH_GRASS,TILE_MUD_WITH_GRASS},
	{TILE_MESE,TILE_MESE,TILE_MESE,TILE_MESE,TILE_MESE,TILE_MESE},
	{TILE_MUD,TILE_MUD,TILE_MUD,TILE_MUD,TILE_MUD,TILE_MUD},
	{TILE_WATER,TILE_WATER,TILE_WATER,TILE_WATER,TILE_WATER,TILE_WATER},
	{TILE_CLOUD,TILE_CLOUD,TILE_CLOUD,TILE_CLOUD,TILE_CLOUD,TILE_CLOUD},
	{TILE_COALSTONE,TILE_COALSTONE,TILE_COALSTONE,TILE_COALSTONE,TILE_COALSTONE,TILE_COALSTONE},
};

const char * g_content_inventory_textures[USEFUL_CONTENT_COUNT] =
{
	"../data/stone.png",
	"../data/grass.png",
	"../data/water.png",
	"../data/torch_on_floor.png",
	"../data/tree_top.png",
	"../data/leaves.png",
	"../data/grass_footsteps.png",
	"../data/mese.png",
	"../data/mud.png",
	"../data/water.png",
	"../data/cloud.png",
	"../data/coalstone.png",
};

