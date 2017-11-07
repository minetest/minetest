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

#include "irr_v3d.h"
#include "mapnode.h"
#include "voxel.h"

class Client;
class MapBlock;

struct MeshMakeData
{
	VoxelManipulator m_vmanip;
	v3s16 m_blockpos = v3s16(-1337,-1337,-1337);
	v3s16 m_crack_pos_relative = v3s16(-1337,-1337,-1337);
	bool m_smooth_lighting = false;

	Client *m_client;
	bool m_use_shaders;
	bool m_use_tangent_vertices;

	MeshMakeData(Client *client, bool use_shaders,
			bool use_tangent_vertices = false);

	/*
		Copy block data manually (to allow optimizations by the caller)
	*/
	void fillBlockDataBegin(const v3s16 &blockpos);
	void fillBlockData(const v3s16 &block_offset, MapNode *data);

	/*
		Copy central data directly from block, and other data from
		parent of block.
	*/
	void fill(MapBlock *block);

	/*
		Set up with only a single node at (1,1,1)
	*/
	void fillSingleNode(MapNode *node);

	/*
		Set the (node) position of a crack
	*/
	void setCrack(int crack_level, v3s16 crack_pos);

	/*
		Enable or disable smooth lighting
	*/
	void setSmoothLighting(bool smooth_lighting);
};
