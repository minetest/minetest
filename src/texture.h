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

#ifndef TEXTURE_HEADER
#define TEXTURE_HEADER

// This file now contains all that was here
#include "tile.h"

// TODO: Remove this
typedef u16 textureid_t;

#if 0

#include "common_irrlicht.h"
//#include "utility.h"
#include "debug.h"

/*
	All textures are given a "texture id".
	0 = nothing (a NULL pointer texture)
*/
typedef u16 textureid_t;

/*
	Every texture in the game can be specified by this.
	
	It exists instead of specification strings because arbitary
	texture combinations for map nodes are handled using this,
	and strings are too slow for that purpose.

	Plain texture pointers are not used because they don't contain
	content information by themselves. A texture can be completely
	reconstructed by just looking at this, while this also is a
	fast unique key to containers.
*/

#define TEXTURE_SPEC_TEXTURE_COUNT 4

struct TextureSpec
{
	TextureSpec()
	{
		clear();
	}

	TextureSpec(textureid_t id0)
	{
		clear();
		tids[0] = id0;
	}

	TextureSpec(textureid_t id0, textureid_t id1)
	{
		clear();
		tids[0] = id0;
		tids[1] = id1;
	}

	void clear()
	{
		for(u32 i=0; i<TEXTURE_SPEC_TEXTURE_COUNT; i++)
		{
			tids[i] = 0;
		}
	}

	bool empty() const
	{
		for(u32 i=0; i<TEXTURE_SPEC_TEXTURE_COUNT; i++)
		{
			if(tids[i] != 0)
				return false;
		}
		return true;
	}

	void addTid(textureid_t tid)
	{
		for(u32 i=0; i<TEXTURE_SPEC_TEXTURE_COUNT; i++)
		{
			if(tids[i] == 0)
			{
				tids[i] = tid;
				return;
			}
		}
		// Too many textures
		assert(0);
	}

	bool operator==(const TextureSpec &other) const
	{
		for(u32 i=0; i<TEXTURE_SPEC_TEXTURE_COUNT; i++)
		{
			if(tids[i] != other.tids[i])
				return false;
		}
		return true;
	}

	bool operator<(const TextureSpec &other) const
	{
		for(u32 i=0; i<TEXTURE_SPEC_TEXTURE_COUNT; i++)
		{
			if(tids[i] >= other.tids[i])
				return false;
		}
		return true;
	}

	// Ids of textures. They are blit on each other.
	textureid_t tids[TEXTURE_SPEC_TEXTURE_COUNT];
};

#endif

#endif
