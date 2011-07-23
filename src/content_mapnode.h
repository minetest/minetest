/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef CONTENT_MAPNODE_HEADER
#define CONTENT_MAPNODE_HEADER

#include "mapnode.h"

void content_mapnode_init();

MapNode mapnode_translate_from_internal(MapNode n_from, u8 version);
MapNode mapnode_translate_to_internal(MapNode n_from, u8 version);

/*
	Node content type IDs
	Ranges:
*/

// 0x000...0x07f (0...127): param2 is fully usable
// 126 and 127 are reserved.
// Use these sparingly, only when the extra space in param2 might be needed.
#define CONTENT_STONE 0
#define CONTENT_WATER 2
#define CONTENT_TORCH 3
#define CONTENT_WATERSOURCE 9
#define CONTENT_SIGN_WALL 14
#define CONTENT_CHEST 15
#define CONTENT_FURNACE 16
#define CONTENT_FENCE 21
#define CONTENT_RAIL 30

// 0x800...0xfff (2048...4095): higher 4 bytes of param2 are not usable
#define CONTENT_GRASS 0x800 //1
#define CONTENT_TREE 0x801 //4
#define CONTENT_LEAVES 0x802 //5
#define CONTENT_GRASS_FOOTSTEPS 0x803 //6
#define CONTENT_MESE 0x804 //7
#define CONTENT_MUD 0x805 //8
// Pretty much useless, clouds won't be drawn this way
#define CONTENT_CLOUD 0x806 //10
#define CONTENT_COALSTONE 0x807 //11
#define CONTENT_WOOD 0x808 //12
#define CONTENT_SAND 0x809 //13
#define CONTENT_COBBLE 0x80a //18
#define CONTENT_STEEL 0x80b //19
#define CONTENT_GLASS 0x80c //20
#define CONTENT_MOSSYCOBBLE 0x80d //22
#define CONTENT_GRAVEL 0x80e //23
#define CONTENT_SANDSTONE 0x80f //24
#define CONTENT_CACTUS 0x810 //25
#define CONTENT_BRICK 0x811 //26
#define CONTENT_CLAY 0x812 //27
#define CONTENT_PAPYRUS 0x813 //28
#define CONTENT_BOOKSHELF 0x814 //29


#endif

