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

#ifndef MAPNODE_HEADER
#define MAPNODE_HEADER

#include <iostream>
#include "irrlichttypes.h"
#include "light.h"
#include "exceptions.h"
#include "serialization.h"
#ifndef SERVER
#include "tile.h"
#endif

/*
	Naming scheme:
	- Material = irrlicht's Material class
	- Content = (content_t) content of a node
	- Tile = TileSpec at some side of a node of some content type

	Content ranges:
		0x000...0x07f: param2 is fully usable
		0x800...0xfff: param2 lower 4 bytes are free
*/
typedef u16 content_t;
#define MAX_CONTENT 0xfff

/*
	Ignored node.

	Anything that stores MapNodes doesn't have to preserve parameters
	associated with this material.
	
	Doesn't create faces with anything and is considered being
	out-of-map in the game map.
*/
#define CONTENT_IGNORE 127
#define CONTENT_IGNORE_DEFAULT_PARAM 0

/*
	The common material through which the player can walk and which
	is transparent to light
*/
#define CONTENT_AIR 126

/*
	Nodes make a face if contents differ and solidness differs.
	Return value:
		0: No face
		1: Face uses m1's content
		2: Face uses m2's content
	equivalent: Whether the blocks share the same face (eg. water and glass)
*/
u8 face_contents(content_t m1, content_t m2, bool *equivalent);

/*
	Packs directions like (1,0,0), (1,-1,0) in six bits.
	NOTE: This wastes way too much space for most purposes.
*/
u8 packDir(v3s16 dir);
v3s16 unpackDir(u8 b);

/*
	facedir: CPT_FACEDIR_SIMPLE param1 value
	dir: The face for which stuff is wanted
	return value: The face from which the stuff is actually found

	NOTE: Currently this uses 2 bits for Z-,X-,Z+,X+, should there be Y+
	      and Y- too?
*/
v3s16 facedir_rotate(u8 facedir, v3s16 dir);

enum LightBank
{
	LIGHTBANK_DAY,
	LIGHTBANK_NIGHT
};

/*
	Masks for MapNode.param2 of flowing liquids
 */
#define LIQUID_LEVEL_MASK 0x07
#define LIQUID_FLOW_DOWN_MASK 0x08

/* maximum amount of liquid in a block */
#define LIQUID_LEVEL_MAX LIQUID_LEVEL_MASK
#define LIQUID_LEVEL_SOURCE (LIQUID_LEVEL_MAX+1)

/*
	This is the stuff what the whole world consists of.
*/


struct MapNode
{
	/*
		Main content
		0x00-0x7f: Short content type
		0x80-0xff: Long content type (param2>>4 makes up low bytes)
	*/
	u8 param0;

	/*
		Misc parameter. Initialized to 0.
		- For light_propagates() blocks, this is light intensity,
		  stored logarithmically from 0 to LIGHT_MAX.
		  Sunlight is LIGHT_SUN, which is LIGHT_MAX+1.
		  - Contains 2 values, day- and night lighting. Each takes 4 bits.
		- Mineral content (should be removed from here)
		- Uhh... well, most blocks have light or nothing in here.
	*/
	u8 param1;
	
	/*
		The second parameter. Initialized to 0.
		E.g. direction for torches and flowing water.
		If param0 >= 0x80, bits 0xf0 of this is extended content type data
	*/
	u8 param2;

	MapNode(const MapNode & n)
	{
		*this = n;
	}
	
	MapNode(content_t content=CONTENT_AIR, u8 a_param1=0, u8 a_param2=0)
	{
		param1 = a_param1;
		param2 = a_param2;
		// Set content (param0 and (param2&0xf0)) after other params
		// because this needs to override part of param2
		setContent(content);
	}

	bool operator==(const MapNode &other)
	{
		return (param0 == other.param0
				&& param1 == other.param1
				&& param2 == other.param2);
	}
	
	// To be used everywhere
	content_t getContent()
	{
		if(param0 < 0x80)
			return param0;
		else
			return (param0<<4) + (param2>>4);
	}
	void setContent(content_t c)
	{
		if(c < 0x80)
		{
			if(param0 >= 0x80)
				param2 &= ~(0xf0);
			param0 = c;
		}
		else
		{
			param0 = c>>4;
			param2 &= ~(0xf0);
			param2 |= (c&0x0f)<<4;
		}
	}
	
	/*
		These four are DEPRECATED I guess. -c55
	*/
	bool light_propagates();
	bool sunlight_propagates();
	u8 solidness();
	u8 light_source();

	void setLight(enum LightBank bank, u8 a_light);
	u8 getLight(enum LightBank bank);
	u8 getLightBanksWithSource();
	
	// 0 <= daylight_factor <= 1000
	// 0 <= return value <= LIGHT_SUN
	u8 getLightBlend(u32 daylight_factor)
	{
		u8 l = ((daylight_factor * getLight(LIGHTBANK_DAY)
			+ (1000-daylight_factor) * getLight(LIGHTBANK_NIGHT))
			)/1000;
		u8 max = LIGHT_MAX;
		if(getLight(LIGHTBANK_DAY) == LIGHT_SUN)
			max = LIGHT_SUN;
		if(l > max)
			l = max;
		return l;
	}
	/*// 0 <= daylight_factor <= 1000
	// 0 <= return value <= 255
	u8 getLightBlend(u32 daylight_factor)
	{
		u8 daylight = decode_light(getLight(LIGHTBANK_DAY));
		u8 nightlight = decode_light(getLight(LIGHTBANK_NIGHT));
		u8 mix = ((daylight_factor * daylight
			+ (1000-daylight_factor) * nightlight)
			)/1000;
		return mix;
	}*/

	// In mapnode.cpp
#ifndef SERVER
	/*
		Get tile of a face of the node.
		dir: direction of face
		Returns: TileSpec. Can contain miscellaneous texture coordinates,
		         which must be obeyed so that the texture atlas can be used.
	*/
	TileSpec getTile(v3s16 dir);
#endif
	
	/*
		Gets mineral content of node, if there is any.
		MINERAL_NONE if doesn't contain or isn't able to contain mineral.
	*/
	u8 getMineral();
	
	/*
		Serialization functions
	*/

	static u32 serializedLength(u8 version);
	void serialize(u8 *dest, u8 version);
	void deSerialize(u8 *source, u8 version);
	
};

/*
	Gets lighting value at face of node
	
	Parameters must consist of air and !air.
	Order doesn't matter.

	If either of the nodes doesn't exist, light is 0.
	
	parameters:
		daynight_ratio: 0...1000
		n: getNode(p) (uses only the lighting value)
		n2: getNode(p + face_dir) (uses only the lighting value)
		face_dir: axis oriented unit vector from p to p2
	
	returns encoded light value.
*/
u8 getFaceLight(u32 daynight_ratio, MapNode n, MapNode n2,
		v3s16 face_dir);

#endif

