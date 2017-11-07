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
#include <map>

class IShaderSource;
class MapBlock;
struct MinimapMapblock;
struct MeshMakeData;

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
	scene::SMesh *m_mesh[MAX_TILE_LAYERS];
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

	template <bool use_tangent_vertices>
	void generate(MeshMakeData *data, v3s16 camera_offset);
};
