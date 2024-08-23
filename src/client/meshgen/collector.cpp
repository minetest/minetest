/*
Minetest
Copyright (C) 2024 Andrey, Andrey2470T <andreyt2203@gmail.com>
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

#include "client/client.h"
#include "collector.h"
#include "settings.h"
#include "client/texture_atlas.h"
#include "client/tile.h"
#include <cassert>
#include <sstream>
#include <cmath>
#include <stdexcept>
#include "log.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "util/timetaker.h"
#include "log.h"

MapblockMeshCollector::MapblockMeshCollector(Client *_client, v3f _center_pos,
        v3f _offset, v3f _translation)
    : MeshCollector(_center_pos, _offset),
      client(_client),
      translation(_translation),
      atlas(_client->getNodeDefManager()->getAtlas()),
      shdrsrc(_client->getShaderSource()),
      tsrc(_client->getTextureSource())
{}

void MapblockMeshCollector::addTileMesh(const TileSpec &tile,
    const video::S3DVertex *vertices, u32 numVertices,
    const u16 *indices, u32 numIndices, v3f pos,
    video::SColor clr, u8 light_source, bool own_color)
{
	//TimeTaker add_tile_mesh_time("Add tile mesh", nullptr, PRECISION_MICRO);

    core::dimension2du atlas_size = atlas->getTextureSize();

	//infostream << "Add tile mesh 1 time: " << add_tile_mesh_time.getTimerTime() << "us" << std::endl;

    for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
        const TileLayer &layer = tile.layers[layernum];
        if (layer.texture_id == 0)
            continue;

        f32 scale = 1.0f;
        if (tile.world_aligned)
            scale = 1.0f / layer.scale;

        // Creating material
        video::SMaterial material;
		material.Lighting = false;
		material.FogEnable = true;
		material.setTexture(0, atlas->getTexture());
		material.forEachTexture([] (auto &tex) {
			setMaterialFilters(tex,
				g_settings->getBool("bilinear_filter"),
				g_settings->getBool("trilinear_filter"),
				g_settings->getBool("anisotropic_filter")
			);
		});

        if (g_settings->getBool("enable_shaders")) {
            material.MaterialType = shdrsrc->getShaderInfo(
                layer.shader_id).material;
            layer.applyMaterialOptionsWithShaders(material);
        }
        else
            layer.applyMaterialOptions(material);

        int tile_pos_shift = 0;

        // Check for crack animation of this layer
        if (layer.material_flags & MATERIAL_FLAG_CRACK) {
            std::ostringstream os(std::ios::binary);
            os << tsrc->getTextureName(layer.texture_id) << "^[crack";
            if (layer.material_flags & MATERIAL_FLAG_CRACK_OVERLAY)
                os << "o";
            u8 tiles = layer.scale;
            if (tiles > 1)
                os << ":" << (u32)tiles;
            os << ":" << (u32)layer.animation_frame_count << ":";

            atlas->insertCrackTile(layer.atlas_tile_info_index, os.str());

            // Shift each UV by the half-width of the atlas to locate it in the separate right side
            tile_pos_shift = atlas_size.Width / 2;
        }
        //infostream << "Add tile mesh 1.1 time: " << add_tile_mesh_time.getTimerTime() << "us" << std::endl;

        // Find already existent prelayer with such material, otherwise create it
        auto layer_it = std::find_if(layers.begin(), layers.end(),
            [&material] (std::pair<video::SMaterial, std::vector<scene::SMeshBuffer *>> &layer)
            {
                return layer.first == material;
            });

        u32 layer_d = (u32)(std::distance(layers.begin(), layer_it));
        if (layer_it == layers.end()) {
            layers.emplace_back(material, std::vector<scene::SMeshBuffer *>());
            layer_it = std::prev(layers.end());

            layer_d = (u32)(std::distance(layers.begin(), layer_it));
            layer_to_buf_v_map[layer_d] = std::map<u32, std::vector<u32>>();
        }

        auto buffer_it = std::find_if(layer_it->second.begin(), layer_it->second.end(),
			[&numVertices] (scene::SMeshBuffer *buffer)
			{
				return buffer->getVertexCount() + numVertices <= U16_MAX;
			});

        u32 buffer_d = (u32)(std::distance(layer_it->second.begin(), buffer_it));

		if (buffer_it == layer_it->second.end()) {
			scene::SMeshBuffer *new_buffer = new scene::SMeshBuffer();
			new_buffer->setHardwareMappingHint(scene::EHM_STATIC);
			new_buffer->Material = material;
			layer_it->second.push_back(new_buffer);
			buffer_it = std::prev(layer_it->second.end());

            buffer_d = (u32)(std::distance(layer_it->second.begin(), buffer_it));
            layer_to_buf_v_map[layer_d][buffer_d] = std::vector<u32>();
		}
        //infostream << "Add tile mesh 1.2 time: " << add_tile_mesh_time.getTimerTime() << "us" << std::endl;

        scene::SMeshBuffer *target_buffer = (*buffer_it);

        u32 vertex_count = target_buffer->getVertexCount();

        const TileInfo &tile_info = atlas->getTileInfo(layer.atlas_tile_info_index);
        video::SColor tc = layer.color;

		std::vector<video::S3DVertex> new_vertices;
		std::vector<u16> new_indices;

        // Modify the vertices
        for (u32 i = 0; i < numVertices; i++) {
            video::S3DVertex vertex(vertices[i]);

            vertex.Pos += pos + offset + translation;
            vertex.TCoords *= scale;

            // Re-calculate UV for linking to the necessary TileInfo pixels in the atlas
            int rel_x = core::round32(vertex.TCoords.X * tile_info.width);
            int rel_y = core::round32(vertex.TCoords.Y * tile_info.height);

            vertex.TCoords.X = f32(tile_info.x + rel_x + tile_pos_shift) / atlas_size.Width;
            vertex.TCoords.Y = f32(tile_info.y + rel_y) / atlas_size.Height;

			// Apply face shading
			video::SColor c = vertex.Color;
            if (own_color) {
				c = clr;
                if (!light_source)
                    applyFacesShading(c, vertex.Normal);
            }
            // Multiply the current color with the HW one
            if (tc != video::SColor(0xFFFFFFFF)) {
                c.setRed(c.getRed() * tc.getRed() / 255);
                c.setGreen(c.getGreen() * tc.getGreen() / 255);
                c.setBlue(c.getBlue() * tc.getBlue() / 255);
            }

            if (!g_settings->getBool("enable_shaders") && c.getAlpha() != 0) {
                layer_to_buf_v_map[layer_d][buffer_d].push_back(vertex_count + i);
            }

            vertex.Color = c;

            new_vertices.push_back(vertex);
            bounding_radius_sq = std::max(bounding_radius_sq,
                    (vertex.Pos - center_pos).getLengthSQ());
        }

        if (layer.isTransparent()) {
			target_buffer->append(new_vertices.data(),
				new_vertices.size(), nullptr, 0);

			MeshTriangle t;
			t.buffer = target_buffer;
			transparent_triangles.reserve(numIndices / 3);
			for (u32 i = 0; i < numIndices; i += 3) {
				t.p1 = vertex_count + indices[i];
				t.p2 = vertex_count + indices[i + 1];
				t.p3 = vertex_count + indices[i + 2];
				t.updateAttributes();
				transparent_triangles.push_back(t);
			}
		}
		else {
			// infostream << "Add tile mesh 2 time: " << add_tile_mesh_time.getTimerTime() << "us" << std::endl;
			for (u32 i = 0; i < numIndices; i++)
				new_indices.push_back(vertex_count + indices[i]);

			target_buffer->append(new_vertices.data(), new_vertices.size(),
				new_indices.data(), new_indices.size());
		}

		//infostream << "Add tile mesh 3 time: " << add_tile_mesh_time.getTimerTime() << "us" << std::endl;
    }
    //add_tile_mesh_time.stop(true);
}

void WieldMeshCollector::addTileMesh(const TileSpec &tile,
	const video::S3DVertex *vertices, u32 numVertices,
	const u16 *indices, u32 numIndices, v3f pos,
	video::SColor clr, u8 light_source, bool own_color)
{
	//TimeTaker add_tile_mesh_time("Add tile mesh", nullptr, PRECISION_MICRO);
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer &layer = tile.layers[layernum];
		if (layer.texture_id == 0)
			continue;

        WieldPreMeshBuffer &p = findBuffer(layer, numVertices);

		f32 scale = 1.0f;
		if (tile.world_aligned)
			scale = 1.0f / layer.scale;

		u32 vertex_count = p.vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			video::SColor color = vertices[i].Color;

			if (own_color) {
				color = clr;
				if (!light_source)
                    applyFacesShading(color, vertices[i].Normal);
			}

			auto vpos = vertices[i].Pos + pos + offset;
			p.vertices.emplace_back(vpos, vertices[i].Normal, color,
					scale * vertices[i].TCoords);
			bounding_radius_sq = std::max(bounding_radius_sq,
					(vpos - center_pos).getLengthSQ());
		}

		for (u32 i = 0; i < numIndices; i++)
			p.indices.push_back(indices[i] + vertex_count);
	}
	//add_tile_mesh_time.stop(false);
}

WieldPreMeshBuffer &WieldMeshCollector::findBuffer(
		const TileLayer &layer, u32 numVertices)
{
	if (numVertices > U16_MAX)
		throw std::invalid_argument(
				"Mesh can't contain more than 65536 vertices");

	for (WieldPreMeshBuffer &p : prebuffers)
		if (p.layer == layer && p.vertices.size() + numVertices <= U16_MAX)
			return p;
	prebuffers.emplace_back(layer);
	return prebuffers.back();
}
