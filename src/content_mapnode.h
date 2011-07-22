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

void content_mapnode_init();

/*
	Node content type IDs
*/
#define CONTENT_STONE 0
#define CONTENT_GRASS 1
#define CONTENT_WATER 2
#define CONTENT_TORCH 3
#define CONTENT_TREE 4
#define CONTENT_LEAVES 5
#define CONTENT_GRASS_FOOTSTEPS 6
#define CONTENT_MESE 7
#define CONTENT_MUD 8
#define CONTENT_WATERSOURCE 9
// Pretty much useless, clouds won't be drawn this way
#define CONTENT_CLOUD 10
#define CONTENT_COALSTONE 11
#define CONTENT_WOOD 12
#define CONTENT_SAND 13
#define CONTENT_SIGN_WALL 14
#define CONTENT_CHEST 15
#define CONTENT_FURNACE 16
//#define CONTENT_WORKBENCH 17
#define CONTENT_COBBLE 18
#define CONTENT_STEEL 19
#define CONTENT_GLASS 20
#define CONTENT_FENCE 21
#define CONTENT_MOSSYCOBBLE 22
#define CONTENT_GRAVEL 23
#define CONTENT_SANDSTONE 24
#define CONTENT_CACTUS 25
#define CONTENT_BRICK 26
#define CONTENT_CLAY 27
#define CONTENT_PAPYRUS 28
#define CONTENT_BOOKSHELF 29
#define CONTENT_RAIL 30
#define CONTENT_JUNGLETREE 31
#define CONTENT_JUNGLEGRASS 32

#endif

