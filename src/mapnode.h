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

#include "irrlichttypes.h"
#include "light.h"

class INodeDefManager;

/*
	Naming scheme:
	- Material = irrlicht's Material class
	- Content = (content_t) content of a node
	- Tile = TileSpec at some side of a node of some content type

	Content ranges:
	  0x000...0x07f: param2 is fully usable
	  0x800...0xfff: param2 lower 4 bits are free
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
	
	// Create directly from a nodename
	// If name is unknown, sets CONTENT_IGNORE
	MapNode(INodeDefManager *ndef, const std::string &name,
			u8 a_param1=0, u8 a_param2=0);

	bool operator==(const MapNode &other)
	{
		return (param0 == other.param0
				&& param1 == other.param1
				&& param2 == other.param2);
	}
	
	// To be used everywhere
	content_t getContent() const
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
	u8 getParam1() const
	{
		return param1;
	}
	void setParam1(u8 p)
	{
		param1 = p;
	}
	u8 getParam2() const
	{
		if(param0 < 0x80)
			return param2;
		else
			return param2 & 0x0f;
	}
	void setParam2(u8 p)
	{
		if(param0 < 0x80)
			param2 = p;
		else{
			param2 &= 0xf0;
			param2 |= (p&0x0f);
		}
	}
	
	void setLight(enum LightBank bank, u8 a_light, INodeDefManager *nodemgr);
	u8 getLight(enum LightBank bank, INodeDefManager *nodemgr) const;
	bool getLightBanks(u8 &lightday, u8 &lightnight, INodeDefManager *nodemgr) const;
	
	// 0 <= daylight_factor <= 1000
	// 0 <= return value <= LIGHT_SUN
	u8 getLightBlend(u32 daylight_factor, INodeDefManager *nodemgr) const
	{
		u8 lightday = 0;
		u8 lightnight = 0;
		getLightBanks(lightday, lightnight, nodemgr);
		return blend_light(daylight_factor, lightday, lightnight);
	}

	u8 getFaceDir(INodeDefManager *nodemgr) const;
	u8 getWallMounted(INodeDefManager *nodemgr) const;
	v3s16 getWallMountedDir(INodeDefManager *nodemgr) const;

	/*
		Serialization functions
	*/

	static u32 serializedLength(u8 version);
	void serialize(u8 *dest, u8 version);
	void deSerialize(u8 *source, u8 version);
	
	// Serializes or deserializes a list of nodes in bulk format (first the
	// content of all nodes, then the param1 of all nodes, then the param2
	// of all nodes).
	//   version = serialization version. Must be >= 22
	//   content_width = the number of bytes of content per node
	//   params_width = the number of bytes of params per node
	//   compressed = true to zlib-compress output
	static void serializeBulk(std::ostream &os, int version,
			const MapNode *nodes, u32 nodecount,
			u8 content_width, u8 params_width, bool compressed);
	static void deSerializeBulk(std::istream &is, int version,
			MapNode *nodes, u32 nodecount,
			u8 content_width, u8 params_width, bool compressed);

private:
	// Deprecated serialization methods
	void serialize_pre22(u8 *dest, u8 version);
	void deSerialize_pre22(u8 *source, u8 version);
};


/*
	MapNode helpers for mesh making stuff
*/

#ifndef SERVER

/*
	Nodes make a face if contents differ and solidness differs.
	Return value:
		0: No face
		1: Face uses m1's content
		2: Face uses m2's content
	equivalent: Whether the blocks share the same face (eg. water and glass)
*/
u8 face_contents(content_t m1, content_t m2, bool *equivalent,
		INodeDefManager *nodemgr);

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
		v3s16 face_dir, INodeDefManager *nodemgr);

#endif


#endif

