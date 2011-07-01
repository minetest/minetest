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
#include "common_irrlicht.h"
#include "light.h"
#include "utility.h"
#include "exceptions.h"
#include "serialization.h"
#include "tile.h"
#include "materials.h"

/*
	Naming scheme:
	- Material = irrlicht's Material class
	- Content = (u8) content of a node
	- Tile = TileSpec at some side of a node of some content type
*/

/*
	Ranges:
		0x000...0x07f: param2 is fully usable
		0x800...0xfff: param2 lower 4 bytes are free
*/
typedef u16 content_t;

/*
	Initializes all kind of stuff in here.
	Many things depend on this.

	This accesses g_texturesource; if it is non-NULL, textures are set.

	Client first calls this with g_texturesource=NULL to run some
	unit tests and stuff, then it runs this again with g_texturesource
	defined to get the textures.

	Server only calls this once with g_texturesource=NULL.
*/
void init_mapnode();

/*
	Ignored node.

	Anything that stores MapNodes doesn't have to preserve parameters
	associated with this material.
	
	Doesn't create faces with anything and is considered being
	out-of-map in the game map.
*/
//#define CONTENT_IGNORE 255
#define CONTENT_IGNORE 127
#define CONTENT_IGNORE_DEFAULT_PARAM 0

/*
	The common material through which the player can walk and which
	is transparent to light
*/
//#define CONTENT_AIR 254
#define CONTENT_AIR 126

/*
	Content feature list
*/

enum ContentParamType
{
	CPT_NONE,
	CPT_LIGHT,
	CPT_MINERAL,
	// Direction for chests and furnaces and such
	CPT_FACEDIR_SIMPLE
};

enum LiquidType
{
	LIQUID_NONE,
	LIQUID_FLOWING,
	LIQUID_SOURCE
};

class MapNode;
class NodeMetadata;

struct ContentFeatures
{
	// If non-NULL, content is translated to this when deserialized
	//MapNode *translate_to;

	// Type of MapNode::param
	ContentParamType param_type;

	/*
		0: up
		1: down
		2: right
		3: left
		4: back
		5: front
	*/
	TileSpec tiles[6];
	
	video::ITexture *inventory_texture;

	bool is_ground_content;
	bool light_propagates;
	bool sunlight_propagates;
	u8 solidness; // Used when choosing which face is drawn
	// This is used for collision detection.
	// Also for general solidness queries.
	bool walkable;
	// Player can point to these
	bool pointable;
	// Player can dig these
	bool diggable;
	// Player can build on these
	bool buildable_to;
	// Whether the node has no liquid, source liquid or flowing liquid
	enum LiquidType liquid_type;
	// If true, param2 is set to direction when placed. Used for torches.
	// NOTE: the direction format is quite inefficient and should be changed
	bool wall_mounted;
	// If true, node is equivalent to air. Torches are, air is. Water is not.
	// Is used for example to check whether a mud block can have grass on.
	bool air_equivalent;
	
	// Inventory item string as which the node appears in inventory when dug.
	// Mineral overrides this.
	std::string dug_item;
	
	// Initial metadata is cloned from this
	NodeMetadata *initial_metadata;
	
	// If the content is liquid, this is the flowing version of the liquid.
	// If content is liquid, this is the same content.
	u8 liquid_alternative_flowing;
	
	// Amount of light the node emits
	u8 light_source;
	
	// Digging properties for different tools
	DiggingPropertiesList digging_properties;
	
	// NOTE: Move relevant properties to here from elsewhere

	void reset()
	{
		//translate_to = NULL;
		param_type = CPT_NONE;
		inventory_texture = NULL;
		is_ground_content = false;
		light_propagates = false;
		sunlight_propagates = false;
		solidness = 2;
		walkable = true;
		pointable = true;
		diggable = true;
		buildable_to = false;
		liquid_type = LIQUID_NONE;
		wall_mounted = false;
		air_equivalent = false;
		dug_item = "";
		initial_metadata = NULL;
		liquid_alternative_flowing = CONTENT_IGNORE;
		light_source = 0;
		digging_properties.clear();
	}

	ContentFeatures()
	{
		reset();
	}

	~ContentFeatures();
	
	/*
		Quickhands for simple materials
	*/
	
	void setTexture(u16 i, std::string name, u8 alpha=255);

	void setAllTextures(std::string name, u8 alpha=255)
	{
		for(u16 i=0; i<6; i++)
		{
			setTexture(i, name, alpha);
		}
		// Force inventory texture too
		setInventoryTexture(name);
	}

	void setTile(u16 i, const TileSpec &tile)
	{
		tiles[i] = tile;
	}
	void setAllTiles(const TileSpec &tile)
	{
		for(u16 i=0; i<6; i++)
		{
			setTile(i, tile);
		}
	}

	void setInventoryTexture(std::string imgname);
	
	void setInventoryTextureCube(std::string top,
			std::string left, std::string right);
};

/*
	Call this to access the ContentFeature list
*/
ContentFeatures & content_features(u8 i);


/*
	Here is a bunch of DEPRECATED functions.
*/

/*
	If true, the material allows light propagation and brightness is stored
	in param.
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline bool light_propagates_content(u8 m)
{
	return content_features(m).light_propagates;
}
/*
	If true, the material allows lossless sunlight propagation.
	NOTE: It doesn't seem to go through torches regardlessly of this
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline bool sunlight_propagates_content(u8 m)
{
	return content_features(m).sunlight_propagates;
}
/*
	On a node-node surface, the material of the node with higher solidness
	is used for drawing.
	0: Invisible
	1: Transparent
	2: Opaque
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline u8 content_solidness(u8 m)
{
	return content_features(m).solidness;
}
// Objects collide with walkable contents
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_walkable(u8 m)
{
	return content_features(m).walkable;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_liquid(u8 m)
{
	return content_features(m).liquid_type != LIQUID_NONE;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_flowing_liquid(u8 m)
{
	return content_features(m).liquid_type == LIQUID_FLOWING;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_liquid_source(u8 m)
{
	return content_features(m).liquid_type == LIQUID_SOURCE;
}
// CONTENT_WATER || CONTENT_WATERSOURCE -> CONTENT_WATER
// CONTENT_LAVA || CONTENT_LAVASOURCE -> CONTENT_LAVA
// NOTE: Don't use, use "content_features(m).whatever" instead
inline u8 make_liquid_flowing(u8 m)
{
	u8 c = content_features(m).liquid_alternative_flowing;
	assert(c != CONTENT_IGNORE);
	return c;
}
// Pointable contents can be pointed to in the map
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_pointable(u8 m)
{
	return content_features(m).pointable;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_diggable(u8 m)
{
	return content_features(m).diggable;
}
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_buildable_to(u8 m)
{
	return content_features(m).buildable_to;
}

/*
	Nodes make a face if contents differ and solidness differs.
	Return value:
		0: No face
		1: Face uses m1's content
		2: Face uses m2's content
*/
inline u8 face_contents(u8 m1, u8 m2)
{
	if(m1 == CONTENT_IGNORE || m2 == CONTENT_IGNORE)
		return 0;
	
	bool contents_differ = (m1 != m2);
	
	// Contents don't differ for different forms of same liquid
	if(content_liquid(m1) && content_liquid(m2)
			&& make_liquid_flowing(m1) == make_liquid_flowing(m2))
		contents_differ = false;
	
	bool solidness_differs = (content_solidness(m1) != content_solidness(m2));
	bool makes_face = contents_differ && solidness_differs;

	if(makes_face == false)
		return 0;

	if(content_solidness(m1) > content_solidness(m2))
		return 1;
	else
		return 2;
}

/*
	Packs directions like (1,0,0), (1,-1,0)
*/
inline u8 packDir(v3s16 dir)
{
	u8 b = 0;

	if(dir.X > 0)
		b |= (1<<0);
	else if(dir.X < 0)
		b |= (1<<1);

	if(dir.Y > 0)
		b |= (1<<2);
	else if(dir.Y < 0)
		b |= (1<<3);

	if(dir.Z > 0)
		b |= (1<<4);
	else if(dir.Z < 0)
		b |= (1<<5);
	
	return b;
}
inline v3s16 unpackDir(u8 b)
{
	v3s16 d(0,0,0);

	if(b & (1<<0))
		d.X = 1;
	else if(b & (1<<1))
		d.X = -1;

	if(b & (1<<2))
		d.Y = 1;
	else if(b & (1<<3))
		d.Y = -1;

	if(b & (1<<4))
		d.Z = 1;
	else if(b & (1<<5))
		d.Z = -1;
	
	return d;
}

/*
	facedir: CPT_FACEDIR_SIMPLE param1 value
	dir: The face for which stuff is wanted
	return value: The face from which the stuff is actually found
*/
v3s16 facedir_rotate(u8 facedir, v3s16 dir);

enum LightBank
{
	LIGHTBANK_DAY,
	LIGHTBANK_NIGHT
};

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
	union
	{
		u8 param0;
		u8 d;
	};

	/*
		Misc parameter. Initialized to 0.
		- For light_propagates() blocks, this is light intensity,
		  stored logarithmically from 0 to LIGHT_MAX.
		  Sunlight is LIGHT_SUN, which is LIGHT_MAX+1.
		  - Contains 2 values, day- and night lighting. Each takes 4 bits.
		- Mineral content (should be removed from here)
		- Uhh... well, most blocks have light or nothing in here.
	*/
	union
	{
		u8 param1;
		s8 param;
	};
	
	/*
		The second parameter. Initialized to 0.
		E.g. direction for torches and flowing water.
	*/
	union
	{
		u8 param2;
		u8 dir;
	};

	MapNode(const MapNode & n)
	{
		*this = n;
	}
	
	MapNode(u8 data=CONTENT_AIR, u8 a_param=0, u8 a_param2=0)
	{
		d = data;
		param = a_param;
		param2 = a_param2;
	}

	bool operator==(const MapNode &other)
	{
		return (d == other.d
				&& param == other.param
				&& param2 == other.param2);
	}
	
	// To be used everywhere
	content_t getContent()
	{
		return d;
	}
	void setContent(content_t c)
	{
		d = c;
	}
	
	/*
		These four are DEPRECATED I guess. -c55
	*/
	bool light_propagates()
	{
		return light_propagates_content(d);
	}
	bool sunlight_propagates()
	{
		return sunlight_propagates_content(d);
	}
	u8 solidness()
	{
		return content_solidness(d);
	}
	u8 light_source()
	{
		return content_features(d).light_source;
	}

	u8 getLightBanksWithSource()
	{
		// Select the brightest of [light source, propagated light]
		u8 lightday = 0;
		u8 lightnight = 0;
		if(content_features(d).param_type == CPT_LIGHT)
		{
			lightday = param & 0x0f;
			lightnight = (param>>4)&0x0f;
		}
		if(light_source() > lightday)
			lightday = light_source();
		if(light_source() > lightnight)
			lightnight = light_source();
		return (lightday&0x0f) | ((lightnight<<4)&0xf0);
	}

	u8 getLight(enum LightBank bank)
	{
		// Select the brightest of [light source, propagated light]
		u8 light = 0;
		if(content_features(d).param_type == CPT_LIGHT)
		{
			if(bank == LIGHTBANK_DAY)
				light = param & 0x0f;
			else if(bank == LIGHTBANK_NIGHT)
				light = (param>>4)&0x0f;
			else
				assert(0);
		}
		if(light_source() > light)
			light = light_source();
		return light;
	}
	
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

	void setLight(enum LightBank bank, u8 a_light)
	{
		// If node doesn't contain light data, ignore this
		if(content_features(d).param_type != CPT_LIGHT)
			return;
		if(bank == LIGHTBANK_DAY)
		{
			param &= 0xf0;
			param |= a_light & 0x0f;
		}
		else if(bank == LIGHTBANK_NIGHT)
		{
			param &= 0x0f;
			param |= (a_light & 0x0f)<<4;
		}
		else
			assert(0);
	}
	
	// In mapnode.cpp
	/*
		Get tile of a face of the node.
		dir: direction of face
		Returns: TileSpec. Can contain miscellaneous texture coordinates,
		         which must be obeyed so that the texture atlas can be used.
	*/
	TileSpec getTile(v3s16 dir);
	
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
		n: getNodeParent(p)
		n2: getNodeParent(p + face_dir)
		face_dir: axis oriented unit vector from p to p2
	
	returns encoded light value.
*/
u8 getFaceLight(u32 daynight_ratio, MapNode n, MapNode n2,
		v3s16 face_dir);

#endif

