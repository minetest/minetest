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

#include "mapblock_mesh.h"
#include "client.h"
#include "mapblock.h"
#include "map.h"
#include "profiler.h"
#include "shader.h"
#include "mesh.h"
#include "minimap.h"
#include "content_mapblock.h"
#include "util/directiontables.h"
#include "client/renderingengine.h"
#include <array>

/*
	MeshMakeData
*/

MeshMakeData::MeshMakeData(Client *client, bool use_shaders,
		bool use_tangent_vertices):
	m_client(client),
	m_use_shaders(use_shaders),
	m_use_tangent_vertices(use_tangent_vertices)
{}

void MeshMakeData::fillBlockDataBegin(const v3s16 &blockpos)
{
	m_blockpos = blockpos;

	v3s16 blockpos_nodes = m_blockpos*MAP_BLOCKSIZE;

	m_vmanip.clear();
	VoxelArea voxel_area(blockpos_nodes - v3s16(1,1,1) * MAP_BLOCKSIZE,
			blockpos_nodes + v3s16(1,1,1) * MAP_BLOCKSIZE*2-v3s16(1,1,1));
	m_vmanip.addArea(voxel_area);
}

void MeshMakeData::fillBlockData(const v3s16 &block_offset, MapNode *data)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	v3s16 bp = m_blockpos + block_offset;
	v3s16 blockpos_nodes = bp * MAP_BLOCKSIZE;
	m_vmanip.copyFrom(data, data_area, v3s16(0,0,0), blockpos_nodes, data_size);
}

void MeshMakeData::fill(MapBlock *block)
{
	fillBlockDataBegin(block->getPos());

	fillBlockData(v3s16(0,0,0), block->getData());

	// Get map for reading neighbor blocks
	Map *map = block->getParent();

	for (const v3s16 &dir : g_26dirs) {
		v3s16 bp = m_blockpos + dir;
		MapBlock *b = map->getBlockNoCreateNoEx(bp);
		if(b)
			fillBlockData(dir, b->getData());
	}
}

void MeshMakeData::fillSingleNode(MapNode *node)
{
	m_blockpos = v3s16(0,0,0);

	v3s16 blockpos_nodes = v3s16(0,0,0);
	VoxelArea area(blockpos_nodes-v3s16(1,1,1)*MAP_BLOCKSIZE,
			blockpos_nodes+v3s16(1,1,1)*MAP_BLOCKSIZE*2-v3s16(1,1,1));
	s32 volume = area.getVolume();
	s32 our_node_index = area.index(1,1,1);

	// Allocate this block + neighbors
	m_vmanip.clear();
	m_vmanip.addArea(area);

	// Fill in data
	MapNode *data = new MapNode[volume];
	for(s32 i = 0; i < volume; i++)
	{
		if (i == our_node_index)
			data[i] = *node;
		else
			data[i] = MapNode(CONTENT_AIR, LIGHT_MAX, 0);
	}
	m_vmanip.copyFrom(data, area, area.MinEdge, area.MinEdge, area.getExtent());
	delete[] data;
}

void MeshMakeData::setCrack(int crack_level, v3s16 crack_pos)
{
	if (crack_level >= 0)
		m_crack_pos_relative = crack_pos - m_blockpos*MAP_BLOCKSIZE;
}

void MeshMakeData::setSmoothLighting(bool smooth_lighting)
{
	m_smooth_lighting = smooth_lighting;
}

/*
	Light and vertex color functions
*/

/*
	Calculate non-smooth lighting at interior of node.
	Single light bank.
*/
static u8 getInteriorLight(enum LightBank bank, MapNode n, s32 increment,
		INodeDefManager *ndef)
{
	u8 light = n.getLight(bank, ndef);

	while(increment > 0)
	{
		light = undiminish_light(light);
		--increment;
	}
	while(increment < 0)
	{
		light = diminish_light(light);
		++increment;
	}

	return decode_light(light);
}

/*
	Calculate non-smooth lighting at interior of node.
	Both light banks.
*/
u16 getInteriorLight(MapNode n, s32 increment, INodeDefManager *ndef)
{
	u16 day = getInteriorLight(LIGHTBANK_DAY, n, increment, ndef);
	u16 night = getInteriorLight(LIGHTBANK_NIGHT, n, increment, ndef);
	return day | (night << 8);
}

/*
	Calculate non-smooth lighting at face of node.
	Single light bank.
*/
static u8 getFaceLight(enum LightBank bank, MapNode n, MapNode n2,
		v3s16 face_dir, INodeDefManager *ndef)
{
	u8 light;
	u8 l1 = n.getLight(bank, ndef);
	u8 l2 = n2.getLight(bank, ndef);
	if(l1 > l2)
		light = l1;
	else
		light = l2;

	// Boost light level for light sources
	u8 light_source = MYMAX(ndef->get(n).light_source,
			ndef->get(n2).light_source);
	if(light_source > light)
		light = light_source;

	return decode_light(light);
}

/*
	Calculate non-smooth lighting at face of node.
	Both light banks.
*/
u16 getFaceLight(MapNode n, MapNode n2, v3s16 face_dir, INodeDefManager *ndef)
{
	u16 day = getFaceLight(LIGHTBANK_DAY, n, n2, face_dir, ndef);
	u16 night = getFaceLight(LIGHTBANK_NIGHT, n, n2, face_dir, ndef);
	return day | (night << 8);
}

/*
	Calculate smooth lighting at the XYZ- corner of p.
	Both light banks
*/
static u16 getSmoothLightCombined(const v3s16 &p,
	const std::array<v3s16,8> &dirs, MeshMakeData *data)
{
	INodeDefManager *ndef = data->m_client->ndef();

	u16 ambient_occlusion = 0;
	u16 light_count = 0;
	u8 light_source_max = 0;
	u16 light_day = 0;
	u16 light_night = 0;

	auto add_node = [&] (u8 i, bool obstructed = false) -> bool {
		if (obstructed) {
			ambient_occlusion++;
			return false;
		}
		MapNode n = data->m_vmanip.getNodeNoExNoEmerge(p + dirs[i]);
		if (n.getContent() == CONTENT_IGNORE)
			return true;
		const ContentFeatures &f = ndef->get(n);
		if (f.light_source > light_source_max)
			light_source_max = f.light_source;
		// Check f.solidness because fast-style leaves look better this way
		if (f.param_type == CPT_LIGHT && f.solidness != 2) {
			light_day += decode_light(n.getLightNoChecks(LIGHTBANK_DAY, &f));
			light_night += decode_light(n.getLightNoChecks(LIGHTBANK_NIGHT, &f));
			light_count++;
		} else {
			ambient_occlusion++;
		}
		return f.light_propagates;
	};

	std::array<bool, 4> obstructed = {{ 1, 1, 1, 1 }};
	add_node(0);
	bool opaque1 = !add_node(1);
	bool opaque2 = !add_node(2);
	bool opaque3 = !add_node(3);
	obstructed[0] = opaque1 && opaque2;
	obstructed[1] = opaque1 && opaque3;
	obstructed[2] = opaque2 && opaque3;
	for (u8 k = 0; k < 3; ++k)
		if (add_node(k + 4, obstructed[k]))
			obstructed[3] = false;
	if (add_node(7, obstructed[3])) { // wrap light around nodes
		ambient_occlusion -= 3;
		for (u8 k = 0; k < 3; ++k)
			add_node(k + 4, !obstructed[k]);
	}

	if (light_count == 0) {
		light_day = light_night = 0;
	} else {
		light_day /= light_count;
		light_night /= light_count;
	}

	// Boost brightness around light sources
	bool skip_ambient_occlusion_day = false;
	if (decode_light(light_source_max) >= light_day) {
		light_day = decode_light(light_source_max);
		skip_ambient_occlusion_day = true;
	}

	bool skip_ambient_occlusion_night = false;
	if(decode_light(light_source_max) >= light_night) {
		light_night = decode_light(light_source_max);
		skip_ambient_occlusion_night = true;
	}

	if (ambient_occlusion > 4) {
		static thread_local const float ao_gamma = rangelim(
			g_settings->getFloat("ambient_occlusion_gamma"), 0.25, 4.0);

		// Table of gamma space multiply factors.
		static thread_local const float light_amount[3] = {
			powf(0.75, 1.0 / ao_gamma),
			powf(0.5,  1.0 / ao_gamma),
			powf(0.25, 1.0 / ao_gamma)
		};

		//calculate table index for gamma space multiplier
		ambient_occlusion -= 5;

		if (!skip_ambient_occlusion_day)
			light_day = rangelim(core::round32(
					light_day * light_amount[ambient_occlusion]), 0, 255);
		if (!skip_ambient_occlusion_night)
			light_night = rangelim(core::round32(
					light_night * light_amount[ambient_occlusion]), 0, 255);
	}

	return light_day | (light_night << 8);
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
	Node at p is solid, and thus the lighting is face-dependent.
*/
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data)
{
	return getSmoothLightTransparent(p + face_dir, corner - 2 * face_dir, data);
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
	Node at p is not solid, and the lighting is not face-dependent.
*/
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data)
{
	const std::array<v3s16,8> dirs = {{
		// Always shine light
		v3s16(0,0,0),
		v3s16(corner.X,0,0),
		v3s16(0,corner.Y,0),
		v3s16(0,0,corner.Z),

		// Can be obstructed
		v3s16(corner.X,corner.Y,0),
		v3s16(corner.X,0,corner.Z),
		v3s16(0,corner.Y,corner.Z),
		v3s16(corner.X,corner.Y,corner.Z)
	}};
	return getSmoothLightCombined(p, dirs, data);
}

void get_sunlight_color(video::SColorf *sunlight, u32 daynight_ratio){
	f32 rg = daynight_ratio / 1000.0f - 0.04f;
	f32 b = (0.98f * daynight_ratio) / 1000.0f + 0.078f;
	sunlight->r = rg;
	sunlight->g = rg;
	sunlight->b = b;
}

void final_color_blend(video::SColor *result,
		u16 light, u32 daynight_ratio)
{
	video::SColorf dayLight;
	get_sunlight_color(&dayLight, daynight_ratio);
	final_color_blend(result,
		encode_light(light, 0), dayLight);
}

void final_color_blend(video::SColor *result,
		const video::SColor &data, const video::SColorf &dayLight)
{
	static const video::SColorf artificialColor(1.04f, 1.04f, 1.04f);

	video::SColorf c(data);
	f32 n = 1 - c.a;

	f32 r = c.r * (c.a * dayLight.r + n * artificialColor.r) * 2.0f;
	f32 g = c.g * (c.a * dayLight.g + n * artificialColor.g) * 2.0f;
	f32 b = c.b * (c.a * dayLight.b + n * artificialColor.b) * 2.0f;

	// Emphase blue a bit in darker places
	// Each entry of this array represents a range of 8 blue levels
	static const u8 emphase_blue_when_dark[32] = {
		1, 4, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	b += emphase_blue_when_dark[irr::core::clamp((s32) ((r + g + b) / 3 * 255),
		0, 255) / 8] / 255.0f;

	result->setRed(core::clamp((s32) (r * 255.0f), 0, 255));
	result->setGreen(core::clamp((s32) (g * 255.0f), 0, 255));
	result->setBlue(core::clamp((s32) (b * 255.0f), 0, 255));
}

/*
	Mesh generation helpers
*/

/*
	Gets nth node tile (0 <= n <= 5).
*/
void getNodeTileN(MapNode mn, v3s16 p, u8 tileindex, MeshMakeData *data, TileSpec &tile)
{
	INodeDefManager *ndef = data->m_client->ndef();
	const ContentFeatures &f = ndef->get(mn);
	tile = f.tiles[tileindex];
	bool has_crack = p == data->m_crack_pos_relative;
	for (TileLayer &layer : tile.layers) {
		if (layer.texture_id == 0)
			continue;
		if (!layer.has_color)
			mn.getColor(f, &(layer.color));
		// Apply temporary crack
		if (has_crack)
			layer.material_flags |= MATERIAL_FLAG_CRACK;
	}
}

/*
	Gets node tile given a face direction.
*/
void getNodeTile(MapNode mn, v3s16 p, v3s16 dir, MeshMakeData *data, TileSpec &tile)
{
	INodeDefManager *ndef = data->m_client->ndef();

	// Direction must be (1,0,0), (-1,0,0), (0,1,0), (0,-1,0),
	// (0,0,1), (0,0,-1) or (0,0,0)
	assert(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z <= 1);

	// Convert direction to single integer for table lookup
	//  0 = (0,0,0)
	//  1 = (1,0,0)
	//  2 = (0,1,0)
	//  3 = (0,0,1)
	//  4 = invalid, treat as (0,0,0)
	//  5 = (0,0,-1)
	//  6 = (0,-1,0)
	//  7 = (-1,0,0)
	u8 dir_i = ((dir.X + 2 * dir.Y + 3 * dir.Z) & 7) * 2;

	// Get rotation for things like chests
	u8 facedir = mn.getFaceDir(ndef);

	static const u16 dir_to_tile[24 * 16] =
	{
		// 0     +X    +Y    +Z           -Z    -Y    -X   ->   value=tile,rotation
		   0,0,  2,0 , 0,0 , 4,0 ,  0,0,  5,0 , 1,0 , 3,0 ,  // rotate around y+ 0 - 3
		   0,0,  4,0 , 0,3 , 3,0 ,  0,0,  2,0 , 1,1 , 5,0 ,
		   0,0,  3,0 , 0,2 , 5,0 ,  0,0,  4,0 , 1,2 , 2,0 ,
		   0,0,  5,0 , 0,1 , 2,0 ,  0,0,  3,0 , 1,3 , 4,0 ,

		   0,0,  2,3 , 5,0 , 0,2 ,  0,0,  1,0 , 4,2 , 3,1 ,  // rotate around z+ 4 - 7
		   0,0,  4,3 , 2,0 , 0,1 ,  0,0,  1,1 , 3,2 , 5,1 ,
		   0,0,  3,3 , 4,0 , 0,0 ,  0,0,  1,2 , 5,2 , 2,1 ,
		   0,0,  5,3 , 3,0 , 0,3 ,  0,0,  1,3 , 2,2 , 4,1 ,

		   0,0,  2,1 , 4,2 , 1,2 ,  0,0,  0,0 , 5,0 , 3,3 ,  // rotate around z- 8 - 11
		   0,0,  4,1 , 3,2 , 1,3 ,  0,0,  0,3 , 2,0 , 5,3 ,
		   0,0,  3,1 , 5,2 , 1,0 ,  0,0,  0,2 , 4,0 , 2,3 ,
		   0,0,  5,1 , 2,2 , 1,1 ,  0,0,  0,1 , 3,0 , 4,3 ,

		   0,0,  0,3 , 3,3 , 4,1 ,  0,0,  5,3 , 2,3 , 1,3 ,  // rotate around x+ 12 - 15
		   0,0,  0,2 , 5,3 , 3,1 ,  0,0,  2,3 , 4,3 , 1,0 ,
		   0,0,  0,1 , 2,3 , 5,1 ,  0,0,  4,3 , 3,3 , 1,1 ,
		   0,0,  0,0 , 4,3 , 2,1 ,  0,0,  3,3 , 5,3 , 1,2 ,

		   0,0,  1,1 , 2,1 , 4,3 ,  0,0,  5,1 , 3,1 , 0,1 ,  // rotate around x- 16 - 19
		   0,0,  1,2 , 4,1 , 3,3 ,  0,0,  2,1 , 5,1 , 0,0 ,
		   0,0,  1,3 , 3,1 , 5,3 ,  0,0,  4,1 , 2,1 , 0,3 ,
		   0,0,  1,0 , 5,1 , 2,3 ,  0,0,  3,1 , 4,1 , 0,2 ,

		   0,0,  3,2 , 1,2 , 4,2 ,  0,0,  5,2 , 0,2 , 2,2 ,  // rotate around y- 20 - 23
		   0,0,  5,2 , 1,3 , 3,2 ,  0,0,  2,2 , 0,1 , 4,2 ,
		   0,0,  2,2 , 1,0 , 5,2 ,  0,0,  4,2 , 0,0 , 3,2 ,
		   0,0,  4,2 , 1,1 , 2,2 ,  0,0,  3,2 , 0,3 , 5,2

	};
	u16 tile_index = facedir * 16 + dir_i;
	getNodeTileN(mn, p, dir_to_tile[tile_index], data, tile);
	tile.rotation = tile.world_aligned ? 0 : dir_to_tile[tile_index + 1];
}

/*
	MapBlockMesh
*/

MapBlockMesh::MapBlockMesh(MeshMakeData *data, v3s16 camera_offset):
	m_minimap_mapblock(NULL),
	m_tsrc(data->m_client->getTextureSource()),
	m_shdrsrc(data->m_client->getShaderSource()),
	m_animation_force_timer(0), // force initial animation
	m_last_crack(-1),
	m_last_daynight_ratio((u32) -1)
{
	for (auto &m : m_mesh)
		m = new scene::SMesh();
	m_enable_shaders = data->m_use_shaders;
	m_use_tangent_vertices = data->m_use_tangent_vertices;
	m_enable_vbo = g_settings->getBool("enable_vbo");

	if (g_settings->getBool("enable_minimap")) {
		m_minimap_mapblock = new MinimapMapblock;
		m_minimap_mapblock->getMinimapNodes(
			&data->m_vmanip, data->m_blockpos * MAP_BLOCKSIZE);
	}

	MeshCollector collector(m_use_tangent_vertices);
	MapblockMeshGenerator generator(data, &collector);
	generator.generate();
	collector.applyTileColors();

	/*
		Convert MeshCollector to SMesh
	*/

	for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
		for(u32 i = 0; i < collector.prebuffers[layer].size(); i++)
		{
			PreMeshBuffer &p = collector.prebuffers[layer][i];

			// Generate animation data
			// - Cracks
			if (p.layer.material_flags & MATERIAL_FLAG_CRACK) {
				// Find the texture name plus ^[crack:N:
				std::ostringstream os(std::ios::binary);
				os << m_tsrc->getTextureName(p.layer.texture_id) << "^[crack";
				if (p.layer.material_flags & MATERIAL_FLAG_CRACK_OVERLAY)
					os << "o";  // use ^[cracko
				u8 tiles = p.layer.scale;
				if (tiles > 1)
					os << ":" << (u32)tiles;
				os << ":" << (u32)p.layer.animation_frame_count << ":";
				m_crack_materials.insert(std::make_pair(
						std::pair<u8, u32>(layer, i), os.str()));
				// Replace tile texture with the cracked one
				p.layer.texture = m_tsrc->getTextureForMesh(
						os.str() + "0",
						&p.layer.texture_id);
			}
			// - Texture animation
			if (p.layer.material_flags & MATERIAL_FLAG_ANIMATION) {
				// Add to MapBlockMesh in order to animate these tiles
				m_animation_tiles[std::pair<u8, u32>(layer, i)] = p.layer;
				m_animation_frames[std::pair<u8, u32>(layer, i)] = 0;
				if (g_settings->getBool(
						"desynchronize_mapblock_texture_animation")) {
					// Get starting position from noise
					m_animation_frame_offsets[std::pair<u8, u32>(layer, i)] =
							100000 * (2.0 + noise3d(
							data->m_blockpos.X, data->m_blockpos.Y,
							data->m_blockpos.Z, 0));
				} else {
					// Play all synchronized
					m_animation_frame_offsets[std::pair<u8, u32>(layer, i)] = 0;
				}
				// Replace tile texture with the first animation frame
				p.layer.texture = (*p.layer.frames)[0].texture;
			}

			if (!m_enable_shaders) {
				// Extract colors for day-night animation
				// Dummy sunlight to handle non-sunlit areas
				video::SColorf sunlight;
				get_sunlight_color(&sunlight, 0);
				u32 vertex_count = m_use_tangent_vertices ?
						p.tangent_vertices.size() : p.vertices.size();
				for (u32 j = 0; j < vertex_count; j++) {
					video::SColor *vc;
					if (m_use_tangent_vertices) {
						vc = &p.tangent_vertices[j].Color;
					} else {
						vc = &p.vertices[j].Color;
					}
					video::SColor copy(*vc);
					if (vc->getAlpha() == 0) // No sunlight - no need to animate
						final_color_blend(vc, copy, sunlight); // Finalize color
					else // Record color to animate
						m_daynight_diffs[std::pair<u8, u32>(layer, i)][j] = copy;

					// The sunlight ratio has been stored,
					// delete alpha (for the final rendering).
					vc->setAlpha(255);
				}
			}

			// Create material
			video::SMaterial material;
			material.setFlag(video::EMF_LIGHTING, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_FOG_ENABLE, true);
			material.setTexture(0, p.layer.texture);

			if (m_enable_shaders) {
				material.MaterialType = m_shdrsrc->getShaderInfo(
						p.layer.shader_id).material;
				p.layer.applyMaterialOptionsWithShaders(material);
				if (p.layer.normal_texture)
					material.setTexture(1, p.layer.normal_texture);
				material.setTexture(2, p.layer.flags_texture);
			} else {
				p.layer.applyMaterialOptions(material);
			}

			scene::SMesh *mesh = (scene::SMesh *)m_mesh[layer];

			// Create meshbuffer, add to mesh
			if (m_use_tangent_vertices) {
				scene::SMeshBufferTangents *buf =
						new scene::SMeshBufferTangents();
				// Set material
				buf->Material = material;
				// Add to mesh
				mesh->addMeshBuffer(buf);
				// Mesh grabbed it
				buf->drop();
				buf->append(&p.tangent_vertices[0], p.tangent_vertices.size(),
					&p.indices[0], p.indices.size());
			} else {
				scene::SMeshBuffer *buf = new scene::SMeshBuffer();
				// Set material
				buf->Material = material;
				// Add to mesh
				mesh->addMeshBuffer(buf);
				// Mesh grabbed it
				buf->drop();
				buf->append(&p.vertices[0], p.vertices.size(),
					&p.indices[0], p.indices.size());
			}
		}

		/*
			Do some stuff to the mesh
		*/
		m_camera_offset = camera_offset;
		translateMesh(m_mesh[layer],
			intToFloat(data->m_blockpos * MAP_BLOCKSIZE - camera_offset, BS));

		if (m_use_tangent_vertices) {
			scene::IMeshManipulator* meshmanip =
				RenderingEngine::get_scene_manager()->getMeshManipulator();
			meshmanip->recalculateTangents(m_mesh[layer], true, false, false);
		}

		if (m_mesh[layer]) {
#if 0
			// Usually 1-700 faces and 1-7 materials
			std::cout << "Updated MapBlock has " << fastfaces_new.size()
					<< " faces and uses " << m_mesh[layer]->getMeshBufferCount()
					<< " materials (meshbuffers)" << std::endl;
#endif

			// Use VBO for mesh (this just would set this for ever buffer)
			if (m_enable_vbo)
				m_mesh[layer]->setHardwareMappingHint(scene::EHM_STATIC);
		}
	}

	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;

	// Check if animation is required for this mesh
	m_has_animation =
		!m_crack_materials.empty() ||
		!m_daynight_diffs.empty() ||
		!m_animation_tiles.empty();
}

MapBlockMesh::~MapBlockMesh()
{
	for (scene::IMesh *m : m_mesh) {
		if (m_enable_vbo && m)
			for (u32 i = 0; i < m->getMeshBufferCount(); i++) {
				scene::IMeshBuffer *buf = m->getMeshBuffer(i);
				RenderingEngine::get_video_driver()->removeHardwareBuffer(buf);
			}
		m->drop();
		m = NULL;
	}
	delete m_minimap_mapblock;
}

bool MapBlockMesh::animate(bool faraway, float time, int crack,
	u32 daynight_ratio)
{
	if (!m_has_animation) {
		m_animation_force_timer = 100000;
		return false;
	}

	m_animation_force_timer = myrand_range(5, 100);

	// Cracks
	if (crack != m_last_crack) {
		for (auto &crack_material : m_crack_materials) {
			scene::IMeshBuffer *buf = m_mesh[crack_material.first.first]->
				getMeshBuffer(crack_material.first.second);
			std::string basename = crack_material.second;

			// Create new texture name from original
			std::ostringstream os;
			os << basename << crack;
			u32 new_texture_id = 0;
			video::ITexture *new_texture =
					m_tsrc->getTextureForMesh(os.str(), &new_texture_id);
			buf->getMaterial().setTexture(0, new_texture);

			// If the current material is also animated,
			// update animation info
			auto anim_iter = m_animation_tiles.find(crack_material.first);
			if (anim_iter != m_animation_tiles.end()) {
				TileLayer &tile = anim_iter->second;
				tile.texture = new_texture;
				tile.texture_id = new_texture_id;
				// force animation update
				m_animation_frames[crack_material.first] = -1;
			}
		}

		m_last_crack = crack;
	}

	// Texture animation
	for (auto &animation_tile : m_animation_tiles) {
		const TileLayer &tile = animation_tile.second;
		// Figure out current frame
		int frameoffset = m_animation_frame_offsets[animation_tile.first];
		int frame = (int)(time * 1000 / tile.animation_frame_length_ms
				+ frameoffset) % tile.animation_frame_count;
		// If frame doesn't change, skip
		if (frame == m_animation_frames[animation_tile.first])
			continue;

		m_animation_frames[animation_tile.first] = frame;

		scene::IMeshBuffer *buf = m_mesh[animation_tile.first.first]->
			getMeshBuffer(animation_tile.first.second);

		const FrameSpec &animation_frame = (*tile.frames)[frame];
		buf->getMaterial().setTexture(0, animation_frame.texture);
		if (m_enable_shaders) {
			if (animation_frame.normal_texture)
				buf->getMaterial().setTexture(1,
					animation_frame.normal_texture);
			buf->getMaterial().setTexture(2, animation_frame.flags_texture);
		}
	}

	// Day-night transition
	if (!m_enable_shaders && (daynight_ratio != m_last_daynight_ratio)) {
		// Force reload mesh to VBO
		if (m_enable_vbo)
			for (scene::IMesh *m : m_mesh)
				m->setDirty();
		video::SColorf day_color;
		get_sunlight_color(&day_color, daynight_ratio);

		for (auto &daynight_diff : m_daynight_diffs) {
			scene::IMeshBuffer *buf = m_mesh[daynight_diff.first.first]->
				getMeshBuffer(daynight_diff.first.second);
			video::S3DVertex *vertices = (video::S3DVertex *)buf->getVertices();
			for (const auto &j : daynight_diff.second)
				final_color_blend(&(vertices[j.first].Color), j.second,
						day_color);
		}
		m_last_daynight_ratio = daynight_ratio;
	}

	return true;
}

void MapBlockMesh::updateCameraOffset(v3s16 camera_offset)
{
	if (camera_offset != m_camera_offset) {
		for (scene::IMesh *layer : m_mesh) {
			translateMesh(layer,
				intToFloat(m_camera_offset - camera_offset, BS));
			if (m_enable_vbo)
				layer->setDirty();
		}
		m_camera_offset = camera_offset;
	}
}

/*
	MeshCollector
*/

void MeshCollector::append(const TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices,
			layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices, u8 layernum,
		bool use_scale)
{
	if (numIndices > 65535) {
		dstream << "FIXME: MeshCollector::append() called with numIndices="
				<< numIndices << " (limit 65535)" << std::endl;
		return;
	}
	std::vector<PreMeshBuffer> *buffers = &prebuffers[layernum];

	PreMeshBuffer *p = NULL;
	for (PreMeshBuffer &pp : *buffers) {
		if (pp.layer == layer && pp.indices.size() + numIndices <= 65535) {
			p = &pp;
			break;
		}
	}

	if (p == NULL) {
		PreMeshBuffer pp;
		pp.layer = layer;
		buffers->push_back(pp);
		p = &(*buffers)[buffers->size() - 1];
	}

	f32 scale = 1.0;
	if (use_scale)
		scale = 1.0 / layer.scale;

	u32 vertex_count;
	if (m_use_tangent_vertices) {
		vertex_count = p->tangent_vertices.size();
		for (u32 i = 0; i < numVertices; i++) {

			video::S3DVertexTangents vert(vertices[i].Pos, vertices[i].Normal,
					vertices[i].Color, scale * vertices[i].TCoords);
			p->tangent_vertices.push_back(vert);
		}
	} else {
		vertex_count = p->vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			video::S3DVertex vert(vertices[i].Pos, vertices[i].Normal,
					vertices[i].Color, scale * vertices[i].TCoords);

			p->vertices.push_back(vert);
		}
	}

	for (u32 i = 0; i < numIndices; i++) {
		u32 j = indices[i] + vertex_count;
		p->indices.push_back(j);
	}
}

/*
	MeshCollector - for meshnodes and converted drawtypes.
*/

void MeshCollector::append(const TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, pos,
				c, light_source, layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source, u8 layernum,
		bool use_scale)
{
	if (numIndices > 65535) {
		dstream << "FIXME: MeshCollector::append() called with numIndices="
				<< numIndices << " (limit 65535)" << std::endl;
		return;
	}
	std::vector<PreMeshBuffer> *buffers = &prebuffers[layernum];

	PreMeshBuffer *p = NULL;
	for (PreMeshBuffer &pp : *buffers) {
		if (pp.layer == layer && pp.indices.size() + numIndices <= 65535) {
			p = &pp;
			break;
		}
	}

	if (p == NULL) {
		PreMeshBuffer pp;
		pp.layer = layer;
		buffers->push_back(pp);
		p = &(*buffers)[buffers->size() - 1];
	}

	f32 scale = 1.0;
	if (use_scale)
		scale = 1.0 / layer.scale;

	video::SColor original_c = c;
	u32 vertex_count;
	if (m_use_tangent_vertices) {
		vertex_count = p->tangent_vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			if (!light_source) {
				c = original_c;
				applyFacesShading(c, vertices[i].Normal);
			}
			video::S3DVertexTangents vert(vertices[i].Pos + pos,
					vertices[i].Normal, c, scale * vertices[i].TCoords);
			p->tangent_vertices.push_back(vert);
		}
	} else {
		vertex_count = p->vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			if (!light_source) {
				c = original_c;
				applyFacesShading(c, vertices[i].Normal);
			}
			video::S3DVertex vert(vertices[i].Pos + pos, vertices[i].Normal, c,
				scale * vertices[i].TCoords);
			p->vertices.push_back(vert);
		}
	}

	for (u32 i = 0; i < numIndices; i++) {
		u32 j = indices[i] + vertex_count;
		p->indices.push_back(j);
	}
}

void MeshCollector::applyTileColors()
{
	if (m_use_tangent_vertices)
		for (auto &prebuffer : prebuffers) {
			for (PreMeshBuffer &pmb : prebuffer) {
				video::SColor tc = pmb.layer.color;
				if (tc == video::SColor(0xFFFFFFFF))
					continue;
				for (video::S3DVertexTangents &tangent_vertex : pmb.tangent_vertices) {
					video::SColor *c = &tangent_vertex.Color;
					c->set(c->getAlpha(), c->getRed() * tc.getRed() / 255,
						c->getGreen() * tc.getGreen() / 255,
						c->getBlue() * tc.getBlue() / 255);
				}
			}
		}
	else
		for (auto &prebuffer : prebuffers) {
			for (PreMeshBuffer &pmb : prebuffer) {
				video::SColor tc = pmb.layer.color;
				if (tc == video::SColor(0xFFFFFFFF))
					continue;
				for (video::S3DVertex &vertex : pmb.vertices) {
					video::SColor *c = &vertex.Color;
					c->set(c->getAlpha(), c->getRed() * tc.getRed() / 255,
						c->getGreen() * tc.getGreen() / 255,
						c->getBlue() * tc.getBlue() / 255);
				}
			}
		}
}

video::SColor encode_light(u16 light, u8 emissive_light)
{
	// Get components
	u32 day = (light & 0xff);
	u32 night = (light >> 8);
	// Add emissive light
	night += emissive_light * 2.5f;
	if (night > 255)
		night = 255;
	// Since we don't know if the day light is sunlight or
	// artificial light, assume it is artificial when the night
	// light bank is also lit.
	if (day < night)
		day = 0;
	else
		day = day - night;
	u32 sum = day + night;
	// Ratio of sunlight:
	u32 r;
	if (sum > 0)
		r = day * 255 / sum;
	else
		r = 0;
	// Average light:
	float b = (day + night) / 2;
	return video::SColor(r, b, b, b);
}
