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
#include <sstream>
#include "client.h"
#include "client/renderingengine.h"
#include "collector.h"
#include "helpers.h"
#include "mesh.h"
#include "minimap.h"
#include "noise.h"
#include "params.h"
#include "settings.h"
#include "shader.h"
#include "solid.h"
#include "special.h"

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

	if (m_use_tangent_vertices)
		generate<true>(data, camera_offset);
	else
		generate<false>(data, camera_offset);

	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;

	// Check if animation is required for this mesh
	m_has_animation =
		!m_crack_materials.empty() ||
		!m_daynight_diffs.empty() ||
		!m_animation_tiles.empty();
}

MapBlockMesh::~MapBlockMesh()
{
	for (scene::SMesh *m : m_mesh) {
		if (m_enable_vbo)
			for (u32 i = 0; i < m->getMeshBufferCount(); i++) {
				scene::IMeshBuffer *buf = m->getMeshBuffer(i);
				RenderingEngine::get_video_driver()->removeHardwareBuffer(buf);
			}
		m->drop();
	}
	delete m_minimap_mapblock;
}

template <bool use_tangent_vertices>
void MapBlockMesh::generate(MeshMakeData *data, v3s16 camera_offset)
{
	// 4-21ms for MAP_BLOCKSIZE=16  (NOTE: probably outdated)
	// 24-155ms for MAP_BLOCKSIZE=32  (NOTE: probably outdated)
	//TimeTaker timer1("MapBlockMesh()");

	std::vector<FastFace> fastfaces_new;
	fastfaces_new.reserve(512);

	/*
		We are including the faces of the trailing edges of the block.
		This means that when something changes, the caller must
		also update the meshes of the blocks at the leading edges.

		NOTE: This is the slowest part of this method.
	*/
	{
		// 4-23ms for MAP_BLOCKSIZE=16  (NOTE: probably outdated)
		//TimeTaker timer2("updateAllFastFaceRows()");
		updateAllFastFaceRows(data, fastfaces_new);
	}
	// End of slow part

	/*
		Convert FastFaces to MeshCollector
	*/

	MeshCollector<use_tangent_vertices> collector;

	{
		// avg 0ms (100ms spikes when loading textures the first time)
		// (NOTE: probably outdated)
		//TimeTaker timer2("MeshCollector building");

		for (const FastFace &f : fastfaces_new) {
			static const u16 indices[] = {0, 1, 2, 2, 3, 0};
			static const u16 indices_alternate[] = {0, 1, 3, 2, 3, 1};

			if (!f.layer.texture)
				continue;

			const u16 *indices_p =
				f.vertex_0_2_connected ? indices : indices_alternate;

			collector.append(f.layer, f.vertices, 4, indices_p, 6,
				f.layernum, f.world_aligned);
		}
	}

	/*
		Add special graphics:
		- torches
		- flowing water
		- fences
		- whatever
	*/

	{
		MapblockMeshGenerator<use_tangent_vertices> generator(data, &collector);
		generator.generate();
	}

	collector.applyTileColors();

	/*
		Convert MeshCollector to SMesh
	*/

	for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
		for(u32 i = 0; i < collector.prebuffers[layer].size(); i++)
		{
			auto &p = collector.prebuffers[layer][i];

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
				u32 vertex_count = p.vertices.size();
				for (u32 j = 0; j < vertex_count; j++) {
					video::SColor *vc = &p.vertices[j].Color;
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

			// Create meshbuffer, add to mesh
			scene::SMesh *mesh = m_mesh[layer];
			auto buf = new scene::CMeshBuffer<typename MeshCollector<use_tangent_vertices>::Vertex>();
			buf->Material = material;
			buf->append(&p.vertices[0], p.vertices.size(),
				&p.indices[0], p.indices.size());
			mesh->addMeshBuffer(buf);
			buf->drop();
		}

		/*
			Do some stuff to the mesh
		*/
		m_camera_offset = camera_offset;
		translateMesh(m_mesh[layer],
			intToFloat(data->m_blockpos * MAP_BLOCKSIZE - camera_offset, BS));

		if (use_tangent_vertices) {
			scene::IMeshManipulator* meshmanip =
				RenderingEngine::get_scene_manager()->getMeshManipulator();
			meshmanip->recalculateTangents(m_mesh[layer], true, false, false);
		}

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
