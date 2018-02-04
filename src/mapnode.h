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

#pragma once

#include "irrlichttypes_bloated.h"
#include "light.h"
#include <string>
#include <vector>

class NodeDefManager;
class Map;

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

// mask for leveled nodebox param2
#define LEVELED_MASK 0x7F
#define LEVELED_MAX LEVELED_MASK


struct ContentFeatures;

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

	MapNode() = default;

	MapNode(content_t content, u8 a_param1=0, u8 a_param2=0) noexcept
		: param0(content),
		  param1(a_param1),
		  param2(a_param2)
	{ }

	bool operator==(const MapNode &other) const noexcept
	{
		return (param0 == other.param0
				&& param1 == other.param1
				&& param2 == other.param2);
	}

	// To be used everywhere
	content_t getContent() const noexcept
	{
		return param0;
	}
	void setContent(content_t c) noexcept
	{
		param0 = c;
	}
	u8 getParam1() const noexcept
	{
		return param1;
	}
	void setParam1(u8 p) noexcept
	{
		param1 = p;
	}
	u8 getParam2() const noexcept
	{
		return param2;
	}
	void setParam2(u8 p) noexcept
	{
		param2 = p;
	}

	/*!
	 * Returns the color of the node.
	 *
	 * \param f content features of this node
	 * \param color output, contains the node's color.
	 */
	void getColor(const ContentFeatures &f, video::SColor *color) const;

	void setLight(LightBank bank, u8 a_light, const ContentFeatures &f) noexcept;

	void setLight(LightBank bank, u8 a_light, const NodeDefManager *nodemgr);

	/**
	 * Check if the light value for night differs from the light value for day.
	 *
	 * @return If the light values are equal, returns true; otherwise false
	 */
	bool isLightDayNightEq(const NodeDefManager *nodemgr) const;

	u8 getLight(LightBank bank, const NodeDefManager *nodemgr) const;

	/*!
	 * Returns the node's light level from param1.
	 * If the node emits light, it is ignored.
	 * \param f the ContentFeatures of this node.
	 */
	u8 getLightRaw(LightBank bank, const ContentFeatures &f) const noexcept;

	/**
	 * This function differs from getLight(LightBank bank, NodeDefManager *nodemgr)
	 * in that the ContentFeatures of the node in question are not retrieved by
	 * the function itself.  Thus, if you have already called nodemgr->get() to
	 * get the ContentFeatures you pass it to this function instead of the
	 * function getting ContentFeatures itself.  Since NodeDefManager::get()
	 * is relatively expensive this can lead to significant performance
	 * improvements in some situations.  Call this function if (and only if)
	 * you have already retrieved the ContentFeatures by calling
	 * NodeDefManager::get() for the node you're working with and the
	 * pre-conditions listed are true.
	 *
	 * @pre f != NULL
	 * @pre f->param_type == CPT_LIGHT
	 */
	u8 getLightNoChecks(LightBank bank, const ContentFeatures *f) const noexcept;

	bool getLightBanks(u8 &lightday, u8 &lightnight,
		const NodeDefManager *nodemgr) const;

	// 0 <= daylight_factor <= 1000
	// 0 <= return value <= LIGHT_SUN
	u8 getLightBlend(u32 daylight_factor, const NodeDefManager *nodemgr) const
	{
		u8 lightday = 0;
		u8 lightnight = 0;
		getLightBanks(lightday, lightnight, nodemgr);
		return blend_light(daylight_factor, lightday, lightnight);
	}

	u8 getFaceDir(const NodeDefManager *nodemgr, bool allow_wallmounted = false) const;
	u8 getWallMounted(const NodeDefManager *nodemgr) const;
	v3s16 getWallMountedDir(const NodeDefManager *nodemgr) const;

	void rotateAlongYAxis(const NodeDefManager *nodemgr, Rotation rot);

	/*!
	 * Checks which neighbors does this node connect to.
	 *
	 * \param p coordinates of the node
	 */
	u8 getNeighbors(v3s16 p, Map *map) const;

	/*
		Gets list of node boxes (used for rendering (NDT_NODEBOX))
	*/
	void getNodeBoxes(const NodeDefManager *nodemgr, std::vector<aabb3f> *boxes,
		u8 neighbors = 0) const;

	/*
		Gets list of selection boxes
	*/
	void getSelectionBoxes(const NodeDefManager *nodemg,
		std::vector<aabb3f> *boxes, u8 neighbors = 0) const;

	/*
		Gets list of collision boxes
	*/
	void getCollisionBoxes(const NodeDefManager *nodemgr,
		std::vector<aabb3f> *boxes, u8 neighbors = 0) const;

	/*
		Liquid helpers
	*/
	u8 getMaxLevel(const NodeDefManager *nodemgr) const;
	u8 getLevel(const NodeDefManager *nodemgr) const;
	u8 setLevel(const NodeDefManager *nodemgr, s8 level = 1);
	u8 addLevel(const NodeDefManager *nodemgr, s8 add = 1);

	/*
		Serialization functions
	*/

	static u32 serializedLength(u8 version);
	void serialize(u8 *dest, u8 version) const;
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
	void deSerialize_pre22(const u8 *source, u8 version);
};
