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

#include "irrlichttypes_extrabloated.h"
#include "irr_ptr.h"
#include "util/numeric.h"
#include "client/tile.h"
#include "client/meshgen/collector.h"
#include "voxel.h"
#include <array>
#include <map>
#include <unordered_map>

class Client;
class NodeDefManager;
class IShaderSource;
class ITextureSource;

/*
	Mesh making stuff
*/


class MapBlock;
struct MinimapMapblock;

struct MeshMakeData
{
	VoxelManipulator m_vmanip;
	v3s16 m_blockpos = v3s16(-1337,-1337,-1337);
	v3s16 m_crack_pos_relative = v3s16(-1337,-1337,-1337);
	bool m_smooth_lighting = false;
	u16 side_length;

	const NodeDefManager *nodedef;
	bool m_use_shaders;

	MeshMakeData(const NodeDefManager *ndef, u16 side_length, bool use_shaders);

	/*
		Copy block data manually (to allow optimizations by the caller)
	*/
	void fillBlockDataBegin(const v3s16 &blockpos);
	void fillBlockData(const v3s16 &bp, MapNode *data);

	/*
		Set the (node) position of a crack
	*/
	void setCrack(int crack_level, v3s16 crack_pos);

	/*
		Enable or disable smooth lighting
	*/
	void setSmoothLighting(bool smooth_lighting);
};

/**
 * Implements a binary space partitioning tree
 * See also: https://en.wikipedia.org/wiki/Binary_space_partitioning
 */
class MapBlockBspTree
{
public:
	MapBlockBspTree() {}

	void buildTree(const std::vector<MeshTriangle> *triangles, u16 side_lingth);

	void traverse(v3f viewpoint, std::vector<s32> &output) const
	{
		traverse(root, viewpoint, output);
	}

private:
	// Tree node definition;
	struct TreeNode
	{
		v3f normal;
		v3f origin;
		std::vector<s32> triangle_refs;
		s32 front_ref;
		s32 back_ref;

		TreeNode() = default;
		TreeNode(v3f normal, v3f origin, const std::vector<s32> &triangle_refs, s32 front_ref, s32 back_ref) :
				normal(normal), origin(origin), triangle_refs(triangle_refs), front_ref(front_ref), back_ref(back_ref)
		{}
	};


	s32 buildTree(v3f normal, v3f origin, float delta, const std::vector<s32> &list, u32 depth);
	void traverse(s32 node, v3f viewpoint, std::vector<s32> &output) const;

	const std::vector<MeshTriangle> *triangles = nullptr; // this reference is managed externally
	std::vector<TreeNode> nodes; // list of nodes
	s32 root = -1; // index of the root node
};


class MapblockMeshCollector;

/*
	Holds a mesh for a mapblock.

	Besides the SMesh*, this contains information used for animating
	the vertex positions, colors and texture coordinates of the mesh.
	For example:
	- cracks [implemented]
	- day/night transitions [implemented]
	- animated flowing liquids [not implemented]
	- animating vertex positions for e.g. axles [not implemented]
*/
class MapBlockMesh
{
public:
	// Builds the mesh given
	MapBlockMesh(Client *client, MeshMakeData *data, v3s16 camera_offset);
	~MapBlockMesh();

	// Update the light in vertices, parameters:
	//   daynight_ratio: 0 .. 1000
	// Returns true if anything has been changed.
	bool updateLighting(u32 daynight_ratio);

	MapblockMeshCollector *getMesh()
	{
        	return m_mesh;
	}

	std::vector<MinimapMapblock*> moveMinimapMapblocks()
	{
		std::vector<MinimapMapblock*> minimap_mapblocks;
		minimap_mapblocks.swap(m_minimap_mapblocks);
		return minimap_mapblocks;
	}

	bool isUpdateLightForced() const
	{
		return m_update_light_force_timer == 0;
	}

	void decreaseUpdateLightForceTimer()
	{
		if(m_update_light_force_timer > 0)
			m_update_light_force_timer--;
	}

	/// Radius of the bounding-sphere, in BS-space.
	f32 getBoundingRadius() const { return m_bounding_radius; }

	/// Center of the bounding-sphere, in BS-space, relative to block pos.
	v3f getBoundingSphereCenter() const { return m_bounding_sphere_center; }

	void addPartialBuffer(scene::SMeshBuffer *current_buffer, std::vector<u16> &current_strain);

	/// update transparent buffers to render towards the camera
	void updateTransparentBuffers(v3f camera_pos, v3s16 block_pos);
	void consolidateTransparentBuffers();

	/// get the list of transparent buffers
	/*const std::vector<PartialMeshBuffer> &getTransparentBuffers() const
	{
		return this->m_transparent_buffers;
	}*/

private:

	MapblockMeshCollector *m_mesh = nullptr;
	std::vector<MinimapMapblock*> m_minimap_mapblocks;
	ITextureSource *m_tsrc;
	IShaderSource *m_shdrsrc;

	f32 m_bounding_radius;
	v3f m_bounding_sphere_center;

	bool m_enable_shaders;

	int m_update_light_force_timer;

	// Animation info: day/night transitions
	// Last daynight_ratio value passed to updateLighting()
	u32 m_last_daynight_ratio;

	// Binary Space Partitioning tree for the block
	MapBlockBspTree m_bsp_tree;
	// Ordered list of references to parts of transparent buffers to draw
	//std::vector<PartialMeshBuffer> m_transparent_buffers;
	// Is m_transparent_buffers currently in consolidated form?
	bool m_transparent_buffers_consolidated = false;
};

/*!
 * Encodes light of a node.
 * The result is not the final color, but a
 * half-baked vertex color.
 * You have to multiply the resulting color
 * with the node's color.
 *
 * \param light the first 8 bits are day light,
 * the last 8 bits are night light
 * \param emissive_light amount of light the surface emits,
 * from 0 to LIGHT_SUN.
 */
video::SColor encode_light(u16 light, u8 emissive_light);

// Compute light at node
u16 getInteriorLight(MapNode n, s32 increment, const NodeDefManager *ndef);
u16 getFaceLight(MapNode n, MapNode n2, const NodeDefManager *ndef);
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data);
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data);

/*!
 * Returns the sunlight's color from the current
 * day-night ratio.
 */
void get_sunlight_color(video::SColorf *sunlight, u32 daynight_ratio);

/*!
 * Gives the final  SColor shown on screen.
 *
 * \param result output color
 * \param light first 8 bits are day light, second 8 bits are
 * night light
 */
void final_color_blend(video::SColor *result,
		u16 light, u32 daynight_ratio);

/*!
 * Gives the final  SColor shown on screen.
 *
 * \param result output color
 * \param data the half-baked vertex color
 * \param dayLight color of the sunlight
 */
void final_color_blend(video::SColor *result,
		const video::SColor &data, const video::SColorf &dayLight);

// Retrieves the TileSpec of a face of a node
// Adds MATERIAL_FLAG_CRACK if the node is cracked
// TileSpec should be passed as reference due to the underlying TileFrame and its vector
// TileFrame vector copy cost very much to client
void getNodeTileN(MapNode mn, const v3s16 &p, u8 tileindex, MeshMakeData *data, TileSpec &tile);
void getNodeTile(MapNode mn, const v3s16 &p, const v3s16 &dir, MeshMakeData *data, TileSpec &tile);

/// Return bitset of the sides of the mesh that consist of solid nodes only
/// Bits:
/// 0 0 -Z +Z -X +X -Y +Y
u8 get_solid_sides(MeshMakeData *data);
