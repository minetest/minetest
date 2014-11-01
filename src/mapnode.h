/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef MAPNODE_HEADER
#define MAPNODE_HEADER

#include "irrlichttypes.h"
#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include "light.h"
#include <string>
#include <vector>

class INodeDefManager;

/*
	Naming scheme:
	- Material = irrlicht's Material class
	- Content = (content_t) content of a node
	- Tile = TileSpec at some side of a node of some content type
*/
typedef u16 content_t;

/*
	The maximum node ID that can be registered by mods. This must
	be significantly lower than the maximum content_t value, so that
	there is enough room for dummy node IDs, which are created when
	a MapBlock containing unknown node names is loaded from disk.
*/
#define MAX_REGISTERED_CONTENT 0x7fffU

/*
	A solid walkable node with the texture unknown_node.png.

	For example, used on the client to display unregistered node IDs
	(instead of expanding the vector of node definitions each time
	such a node is received).
*/
#define CONTENT_UNKNOWN 125

/*
	The common material through which the player can walk and which
	is transparent to light
*/
#define CONTENT_AIR 126

/*
	Ignored node.
	
	Unloaded chunks are considered to consist of this. Several other
	methods return this when an error occurs. Also, during
	map generation this means the node has not been set yet.
	
	Doesn't create faces with anything and is considered being
	out-of-map in the game map.
*/
#define CONTENT_IGNORE 127

enum LightBank
{
	LIGHTBANK_DAY,
	LIGHTBANK_NIGHT
};

/*
	Simple rotation enum.
*/
enum Rotation {
	ROTATE_0,
	ROTATE_90,
	ROTATE_180,
	ROTATE_270,
	ROTATE_RAND,
};

/*
	Masks for MapNode.param2 of flowing liquids
 */
#define LIQUID_LEVEL_MASK 0x07
#define LIQUID_FLOW_DOWN_MASK 0x08

//#define LIQUID_LEVEL_MASK 0x3f // better finite water
//#define LIQUID_FLOW_DOWN_MASK 0x40 // not used when finite water

/* maximum amount of liquid in a block */
#define LIQUID_LEVEL_MAX LIQUID_LEVEL_MASK
#define LIQUID_LEVEL_SOURCE (LIQUID_LEVEL_MAX+1)

#define LIQUID_INFINITY_MASK 0x80 //0b10000000

// mask for param2, now as for liquid
#define LEVELED_MASK 0x3F
#define LEVELED_MAX LEVELED_MASK

/*
	This is the stuff what the whole world consists of.
*/


struct MapNode
{
	/*
		Main content
	*/
	u16 param0;

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
	*/
	u8 param2;

	MapNode(const MapNode & n)
	{
		*this = n;
	}
	
	MapNode(content_t content=CONTENT_AIR, u8 a_param1=0, u8 a_param2=0)
	{
		param0 = content;
		param1 = a_param1;
		param2 = a_param2;
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
		return param0;
	}
	void setContent(content_t c)
	{
		param0 = c;
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
		return param2;
	}
	void setParam2(u8 p)
	{
		param2 = p;
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

	// 0.0 <= daylight_factor <= 1.0
	// 0 <= return value <= LIGHT_SUN
	u8 getLightBlendF1(float daylight_factor, INodeDefManager *nodemgr) const
	{
		u8 lightday = 0;
		u8 lightnight = 0;
		getLightBanks(lightday, lightnight, nodemgr);
		return blend_light_f1(daylight_factor, lightday, lightnight);
	}

	u8 getFaceDir(INodeDefManager *nodemgr) const;
	u8 getWallMounted(INodeDefManager *nodemgr) const;
	v3s16 getWallMountedDir(INodeDefManager *nodemgr) const;
	
	void rotateAlongYAxis(INodeDefManager *nodemgr, Rotation rot);

	/*
		Gets list of node boxes (used for rendering (NDT_NODEBOX))
	*/
	std::vector<aabb3f> getNodeBoxes(INodeDefManager *nodemgr) const;

	/*
		Gets list of selection boxes
	*/
	std::vector<aabb3f> getSelectionBoxes(INodeDefManager *nodemgr) const;

	/*
		Gets list of collision boxes
	*/
	std::vector<aabb3f> getCollisionBoxes(INodeDefManager *nodemgr) const;

	/* Liquid helpers */
	u8 getMaxLevel(INodeDefManager *nodemgr) const;
	u8 getLevel(INodeDefManager *nodemgr) const;
	u8 setLevel(INodeDefManager *nodemgr, s8 level = 1);
	u8 addLevel(INodeDefManager *nodemgr, s8 add = 1);
	void freezeMelt(INodeDefManager *nodemgr);

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
	void deSerialize_pre22(u8 *source, u8 version);
};

#endif

