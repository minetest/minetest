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

#ifndef TILE_HEADER
#define TILE_HEADER

#include "common_irrlicht.h"
//#include "utility.h"
#include "texture.h"
#include <string>

enum MaterialType{
	MATERIAL_ALPHA_NONE,
	MATERIAL_ALPHA_VERTEX,
	MATERIAL_ALPHA_SIMPLE, // >127 = opaque
	MATERIAL_ALPHA_BLEND,
};

// Material flags
#define MATERIAL_FLAG_BACKFACE_CULLING 0x01

/*
	This fully defines the looks of a tile.
	The SMaterial of a tile is constructed according to this.
*/
struct TileSpec
{
	TileSpec():
		alpha(255),
		material_type(MATERIAL_ALPHA_NONE),
		material_flags(
			MATERIAL_FLAG_BACKFACE_CULLING
		)
	{
	}

	bool operator==(TileSpec &other)
	{
		return (
			spec == other.spec &&
			alpha == other.alpha &&
			material_type == other.material_type &&
			material_flags == other.material_flags
		);
	}
	
	// Sets everything else except the texture in the material
	void applyMaterialOptions(video::SMaterial &material)
	{
		if(alpha != 255 && material_type != MATERIAL_ALPHA_VERTEX)
			dstream<<"WARNING: TileSpec: alpha != 255 "
					"but not MATERIAL_ALPHA_VERTEX"
					<<std::endl;

		if(material_type == MATERIAL_ALPHA_NONE)
			material.MaterialType = video::EMT_SOLID;
		else if(material_type == MATERIAL_ALPHA_VERTEX)
			material.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
		else if(material_type == MATERIAL_ALPHA_SIMPLE)
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
		else if(material_type == MATERIAL_ALPHA_BLEND)
			material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;

		material.BackfaceCulling = (material_flags & MATERIAL_FLAG_BACKFACE_CULLING) ? true : false;
	}
	
	// Specification of texture
	TextureSpec spec;
	// Vertex alpha
	u8 alpha;
	// Material type
	u8 material_type;
	// Material flags
	u8 material_flags;
};

#endif
