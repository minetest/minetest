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
#include "iirrlichtwrapper.h"

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

// Initializes g_content_inventory_texture_paths
void init_content_inventory_texture_paths();


// NOTE: This is not used appropriately everywhere.
#define MATERIALS_COUNT 256

/*
	Ignored node.

	Anything that stores MapNodes doesn't have to preserve parameters
	associated with this material.
	
	Doesn't create faces with anything and is considered being
	out-of-map in the game map.
*/
#define CONTENT_IGNORE 255
#define CONTENT_IGNORE_DEFAULT_PARAM 0

/*
	The common material through which the player can walk and which
	is transparent to light
*/
#define CONTENT_AIR 254

/*
	Suggested materials:
	- Gravel
	- Sand
	
	New naming scheme:
	- Material = irrlicht's Material class
	- Content = (u8) content of a node
	- Tile = (u16) Material ID at some side of a node
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

/*
	This is used by all kinds of things to allocate memory for all
	contents except CONTENT_AIR and CONTENT_IGNORE
*/
#define USEFUL_CONTENT_COUNT 14

/*
	Content feature list
*/

enum ContentParamType
{
	CPT_NONE,
	CPT_LIGHT,
	CPT_MINERAL
};

enum LiquidType
{
	LIQUID_NONE,
	LIQUID_FLOWING,
	LIQUID_SOURCE
};

class MapNode;

struct ContentFeatures
{
	// If non-NULL, content is translated to this when deserialized
	MapNode *translate_to;

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
	
	// TODO: Somehow specify inventory image
	//std::string inventory_image_path;
	//TextureSpec inventory_texture;
	//u32 inventory_texture_id;
	video::ITexture *inventory_texture;

	bool is_ground_content; //TODO: Remove, use walkable instead
	bool light_propagates;
	bool sunlight_propagates;
	u8 solidness; // Used when choosing which face is drawn
	bool walkable;
	bool pointable;
	bool diggable;
	bool buildable_to;
	enum LiquidType liquid_type;
	// If true, param2 is set to direction when placed
	// NOTE: the direction format is quite inefficient and should be changed
	bool wall_mounted;
	
	// Inventory item string as which the node appears in inventory when dug.
	// Mineral overrides this.
	std::string dug_item;

	//TODO: Move more properties here

	ContentFeatures()
	{
		translate_to = NULL;
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
		dug_item = "";
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
	}

	/*void setTexture(u16 i, AtlasPointer p, u8 alpha=255)
	{
		tiles[i].texture = p;
		if(alpha != 255)
		{
			tiles[i].alpha = alpha;
			tiles[i].material_type = MATERIAL_ALPHA_VERTEX;
		}
	}
	void setAllTextures(AtlasPointer p, u8 alpha=255)
	{
		for(u16 i=0; i<6; i++)
		{
			setTexture(i, p, alpha);
		}
	}*/

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
	If true, the material allows light propagation and brightness is stored
	in param.
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline bool light_propagates_content(u8 m)
{
	return content_features(m).light_propagates;
	//return (m == CONTENT_AIR || m == CONTENT_TORCH || m == CONTENT_WATER || m == CONTENT_WATERSOURCE);
}

/*
	If true, the material allows lossless sunlight propagation.
	NOTE: It doesn't seem to go through torches regardlessly of this
	NOTE: Don't use, use "content_features(m).whatever" instead
*/
inline bool sunlight_propagates_content(u8 m)
{
	return content_features(m).sunlight_propagates;
	//return (m == CONTENT_AIR || m == CONTENT_TORCH);
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
	/*// As of now, every pseudo node like torches are added to this
	if(m == CONTENT_AIR || m == CONTENT_TORCH || m == CONTENT_WATER)
		return 0;
	if(m == CONTENT_WATER || m == CONTENT_WATERSOURCE)
		return 1;
	return 2;*/
}

// Objects collide with walkable contents
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_walkable(u8 m)
{
	return content_features(m).walkable;
	//return (m != CONTENT_AIR && m != CONTENT_WATER && m != CONTENT_WATERSOURCE && m != CONTENT_TORCH);
}

// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_liquid(u8 m)
{
	return content_features(m).liquid_type != LIQUID_NONE;
	//return (m == CONTENT_WATER || m == CONTENT_WATERSOURCE);
}

// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_flowing_liquid(u8 m)
{
	return content_features(m).liquid_type == LIQUID_FLOWING;
	//return (m == CONTENT_WATER);
}

// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_liquid_source(u8 m)
{
	return content_features(m).liquid_type == LIQUID_SOURCE;
	//return (m == CONTENT_WATERSOURCE);
}

// CONTENT_WATER || CONTENT_WATERSOURCE -> CONTENT_WATER
// CONTENT_LAVA || CONTENT_LAVASOURCE -> CONTENT_LAVA
inline u8 make_liquid_flowing(u8 m)
{
	if(m == CONTENT_WATER || m == CONTENT_WATERSOURCE)
		return CONTENT_WATER;
	assert(0);
}

// Pointable contents can be pointed to in the map
// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_pointable(u8 m)
{
	return content_features(m).pointable;
	//return (m != CONTENT_AIR && m != CONTENT_WATER && m != CONTENT_WATERSOURCE);
}

// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_diggable(u8 m)
{
	return content_features(m).diggable;
	//return (m != CONTENT_AIR && m != CONTENT_WATER && m != CONTENT_WATERSOURCE);
}

// NOTE: Don't use, use "content_features(m).whatever" instead
inline bool content_buildable_to(u8 m)
{
	return content_features(m).buildable_to;
	//return (m == CONTENT_AIR || m == CONTENT_WATER || m == CONTENT_WATERSOURCE);
}

/*
	Returns true for contents that form the base ground that
	follows the main heightmap
*/
/*inline bool is_ground_content(u8 m)
{
	return content_features(m).is_ground_content;
}*/

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
	// Content
	u8 d;

	/*
		Misc parameter. Initialized to 0.
		- For light_propagates() blocks, this is light intensity,
		  stored logarithmically from 0 to LIGHT_MAX.
		  Sunlight is LIGHT_SUN, which is LIGHT_MAX+1.
		- Contains 2 values, day- and night lighting. Each takes 4 bits.
	*/
	s8 param;
	
	union
	{
		/*
			The second parameter. Initialized to 0.
			Direction for torches and flowing water.
		*/
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

	/*MapNode & operator=(const MapNode &other)
	{
		d = other.d;
		param = other.param;
		param2 = other.param2;
		return *this;
	}*/

	bool operator==(const MapNode &other)
	{
		return (d == other.d
				&& param == other.param
				&& param2 == other.param2);
	}

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
		/*
			Note that a block that isn't light_propagates() can be a light source.
		*/
		if(d == CONTENT_TORCH)
			return LIGHT_MAX;
		
		return 0;
	}

	u8 getLightBanksWithSource()
	{
		// Select the brightest of [light source, propagated light]
		u8 lightday = 0;
		u8 lightnight = 0;
		if(light_propagates())
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

	void setLightBanks(u8 a_light)
	{
		param = a_light;
	}

	u8 getLight(enum LightBank bank)
	{
		// Select the brightest of [light source, propagated light]
		u8 light = 0;
		if(light_propagates())
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
		// If not transparent, can't set light
		if(light_propagates() == false)
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
	TileSpec getTile(v3s16 dir);

	u8 getMineral();

	/*
		These serialization functions are used when informing client
		of a single node add
	*/

	static u32 serializedLength(u8 version)
	{
		if(!ser_ver_supported(version))
			throw VersionMismatchException("ERROR: MapNode format not supported");
			
		if(version == 0)
			return 1;
		else if(version <= 9)
			return 2;
		else
			return 3;
	}
	void serialize(u8 *dest, u8 version)
	{
		if(!ser_ver_supported(version))
			throw VersionMismatchException("ERROR: MapNode format not supported");
			
		if(version == 0)
		{
			dest[0] = d;
		}
		else if(version <= 9)
		{
			dest[0] = d;
			dest[1] = param;
		}
		else
		{
			dest[0] = d;
			dest[1] = param;
			dest[2] = param2;
		}
	}
	void deSerialize(u8 *source, u8 version)
	{
		if(!ser_ver_supported(version))
			throw VersionMismatchException("ERROR: MapNode format not supported");
			
		if(version == 0)
		{
			d = source[0];
		}
		else if(version == 1)
		{
			d = source[0];
			// This version doesn't support saved lighting
			if(light_propagates() || light_source() > 0)
				param = 0;
			else
				param = source[1];
		}
		else if(version <= 9)
		{
			d = source[0];
			param = source[1];
		}
		else
		{
			d = source[0];
			param = source[1];
			param2 = source[2];
		}

		// Translate deprecated stuff
		// NOTE: This doesn't get used because MapBlock handles node
		// parameters directly
		MapNode *translate_to = content_features(d).translate_to;
		if(translate_to)
		{
			dstream<<"MapNode: WARNING: Translating "<<d<<" to "
					<<translate_to->d<<std::endl;
			*this = *translate_to;
		}
	}
};



#endif

