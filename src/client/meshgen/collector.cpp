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
#include <algorithm>
#include "log.h"
#include "client/mesh.h"
#include "client/shader.h"
#include "util/timetaker.h"
#include "nodedef.h"
#include "log.h"

/*
	PartialMeshBuffer
*/

void PartialMeshBuffer::beforeDraw() const
{
	// Patch the indexes in the mesh buffer before draw
	m_buffer->Indices = std::move(m_vertex_indexes);
	m_buffer->setDirty(scene::EBT_INDEX);
}

void PartialMeshBuffer::afterDraw() const
{
	// Take the data back
	m_vertex_indexes = std::move(m_buffer->Indices);
}

MapblockMeshCollector::MapblockMeshCollector(Client *_client, v3f _center_pos,
        v3f _offset, v3f _translation)
    : MeshCollector(_center_pos, _offset),
      client(_client),
      translation(_translation)
{
	enable_shaders = g_settings->getBool("enable_shaders");
	bilinear_filter = g_settings->getBool("bilinear_filter");
	trilinear_filter = g_settings->getBool("trilinear_filter");
	anisotropic_filter = g_settings->getBool("anisotropic_filter");
}

void MapblockMeshCollector::addTileMesh(const TileSpec &tile,
	const video::S3DVertex *vertices, u32 numVertices,
	const u16 *indices, u32 numIndices, bool outside_uv,
	v3f pos, video::SColor clr,
	u8 light_source, bool own_color)
{
	TextureBuilder *tbuilder = client->getNodeDefManager()->getTextureBuilder();
	ITextureSource *tsrc = client->getTextureSource();
	IWritableShaderSource *shdrsrc = client->getShaderSource();

    for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
        const TileLayer &layer = tile.layers[layernum];
        if (layer.texture_id == 0)
            continue;

        f32 scale = 1.0f;
        if (tile.world_aligned)
            scale = 1.0f / layer.scale;

		TextureAtlas *atlas = nullptr;
		TextureSingleton *st = nullptr;
		core::dimension2du atlas_size;
		core::dimension2du st_size;

		if (outside_uv) {
			st = tbuilder->findSingleton(layer.texture);

			if (!st)
				st = tbuilder->buildSingleton(client, layer.texture);

			st_size = st->getTextureSize();
		}
		else {
			atlas = tbuilder->getAtlas(layer.atlas_index);
			atlas_size = atlas->getTextureSize();
		}

        // Creating material
        video::SMaterial material;
		material.Lighting = false;
		material.FogEnable = true;
		material.setTexture(0, outside_uv ? st->getTexture() : atlas->getTexture());
		material.forEachTexture([&] (auto &tex) {
			setMaterialFilters(tex,
				bilinear_filter,
				trilinear_filter,
				anisotropic_filter
			);
		});

        if (enable_shaders) {
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

			if (outside_uv)
				st->crack_tile = os.str();
			else {
				atlas->insertCrackTile(layer.atlas_tile_info_index, os.str());

				// Shift each UV by the half-width of the atlas to locate it in the separate right side
				tile_pos_shift = atlas_size.Width / 2;
			}
        }

        // Find already existent prelayer with such material, otherwise create it
        auto layer_it = std::find_if(layers.begin(), layers.end(),
            [&material] (std::pair<video::SMaterial, std::list<MeshPart>> &layer)
            {
                return layer.first == material;
            });

        if (layer_it == layers.end()) {
            layers.emplace_back(material, std::list<MeshPart>());
            layer_it = std::prev(layers.end());
        }

        auto buffer_it = std::find_if(layer_it->second.begin(), layer_it->second.end(),
			[&numVertices] (MeshPart &part)
			{
				return part.vertices.size() + numVertices <= U16_MAX;
			});

        if (buffer_it == layer_it->second.end()) {
			layer_it->second.emplace_back();
			buffer_it = std::prev(layer_it->second.end());
		}

		MeshPart &part = (*buffer_it);
        u32 vertex_count = part.vertices.size();

        video::SColor tc = layer.color;

        // Modify the vertices
        for (u32 i = 0; i < numVertices; i++) {
			video::S3DVertex vertex(vertices[i]);

            vertex.Pos += pos + offset + translation;
            vertex.TCoords *= scale;

			if (outside_uv)
				vertex.TCoords.X *= 0.5f;
			else {
				const TileInfo &tile_info = atlas->getTileInfo(layer.atlas_tile_info_index);
				// Re-calculate UV for linking to the necessary TileInfo pixels in the atlas
				int rel_x = core::round32(vertex.TCoords.X * tile_info.width);
				int rel_y = core::round32(vertex.TCoords.Y * tile_info.height);

				vertex.TCoords.X = f32(tile_info.x + rel_x + tile_pos_shift) / atlas_size.Width;
				vertex.TCoords.Y = f32(tile_info.y + rel_y) / atlas_size.Height;
			}

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

            vertex.Color = c;

			if (!enable_shaders && c.getAlpha() != 0)
				part.opaque_verts_refs.push_back(part.vertices.size());

            bounding_radius_sq = std::max(bounding_radius_sq,
                    (vertex.Pos - center_pos).getLengthSQ());

			part.vertices.push_back(std::move(vertex));
        }

		for (u32 i = 0; i < numIndices; i++)
			part.indices.push_back(vertex_count + indices[i]);
    }
}

void WieldMeshCollector::addTileMesh(const TileSpec &tile,
	const video::S3DVertex *vertices, u32 numVertices,
	const u16 *indices, u32 numIndices, bool outside_uv,
	v3f pos, video::SColor clr,
	u8 light_source, bool own_color)
{
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
