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
#include "client/tile.h"
#include "voxel.h"
#include <array>
#include <map>

class Client;
class IShaderSource;

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
	MapBlockMesh(MeshMakeData *data, v3s16 camera_offset);
	~MapBlockMesh();

	// Main animation function, parameters:
	//   faraway: whether the block is far away from the camera (~50 nodes)
	//   time: the global animation time, 0 .. 60 (repeats every minute)
	//   daynight_ratio: 0 .. 1000
	//   crack: -1 .. CRACK_ANIMATION_LENGTH-1 (-1 for off)
	// Returns true if anything has been changed.
	bool animate(bool faraway, float time, int crack, u32 daynight_ratio);

	scene::IMesh *getMesh()
	{
		return m_mesh[0];
	}

	scene::IMesh *getMesh(u8 layer)
	{
		return m_mesh[layer];
	}

	MinimapMapblock *moveMinimapMapblock()
	{
		MinimapMapblock *p = m_minimap_mapblock;
		m_minimap_mapblock = NULL;
		return p;
	}

	bool isAnimationForced() const
	{
		return m_animation_force_timer == 0;
	}

	void decreaseAnimationForceTimer()
	{
		if(m_animation_force_timer > 0)
			m_animation_force_timer--;
	}

	void updateCameraOffset(v3s16 camera_offset);

private:
	scene::IMesh *m_mesh[MAX_TILE_LAYERS];
	MinimapMapblock *m_minimap_mapblock;
	ITextureSource *m_tsrc;
	IShaderSource *m_shdrsrc;

	bool m_enable_shaders;
	bool m_use_tangent_vertices;
	bool m_enable_vbo;

	// Must animate() be called before rendering?
	bool m_has_animation;
	int m_animation_force_timer;

	// Animation info: cracks
	// Last crack value passed to animate()
	int m_last_crack;
	// Maps mesh and mesh buffer (i.e. material) indices to base texture names
	std::map<std::pair<u8, u32>, std::string> m_crack_materials;

	// Animation info: texture animationi
	// Maps mesh and mesh buffer indices to TileSpecs
	// Keys are pairs of (mesh index, buffer index in the mesh)
	std::map<std::pair<u8, u32>, TileLayer> m_animation_tiles;
	std::map<std::pair<u8, u32>, int> m_animation_frames; // last animation frame
	std::map<std::pair<u8, u32>, int> m_animation_frame_offsets;

	// Animation info: day/night transitions
	// Last daynight_ratio value passed to animate()
	u32 m_last_daynight_ratio;
	// For each mesh and mesh buffer, stores pre-baked colors
	// of sunlit vertices
	// Keys are pairs of (mesh index, buffer index in the mesh)
	std::map<std::pair<u8, u32>, std::map<u32, video::SColor > > m_daynight_diffs;

	// Camera offset info -> do we have to translate the mesh?
	v3s16 m_camera_offset;
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
u16 getFaceLight(MapNode n, MapNode n2, const v3s16 &face_dir,
	const NodeDefManager *ndef);
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
