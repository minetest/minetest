/*
Minetest
Copyright (C) 2018 numzero, Lobachevskiy Vitaliy <numzer0@yandex.ru>

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

#include "collector.h"
#include <stdexcept>
#include "log.h"
#include "client/mesh.h"

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer &layer = tile.layers[layernum];
		if (layer.texture_id == 0)
			continue;

		PreMeshBuffer &p = findBuffer(layer, numVertices);

		f32 scale = 1.0f;
		if (tile.world_aligned)
			scale = 1.0f / layer.scale;

		u32 vertex_count = p.vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			p.vertices.emplace_back(vertices[i].Pos + offset + translation, vertices[i].Normal,
					vertices[i].Color, scale * vertices[i].TCoords);
			bounding_radius_sq = std::max(bounding_radius_sq,
					(vertices[i].Pos - center_pos).getLengthSQ());
		}

		for (u32 i = 0; i < numIndices; i++)
			p.indices.push_back(indices[i] + vertex_count);
	}
}

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer &layer = tile.layers[layernum];
		if (layer.texture_id == 0)
			continue;

		PreMeshBuffer &p = findBuffer(layer, numVertices);

		f32 scale = 1.0f;
		if (tile.world_aligned)
			scale = 1.0f / layer.scale;

		u32 vertex_count = p.vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			video::SColor color = c;
			if (!light_source)
				applyFacesShading(color, vertices[i].Normal);
			auto vpos = vertices[i].Pos + pos + offset + translation;
			p.vertices.emplace_back(vpos, vertices[i].Normal, color,
					scale * vertices[i].TCoords);
			bounding_radius_sq = std::max(bounding_radius_sq,
					(vpos - center_pos).getLengthSQ());
		}

		for (u32 i = 0; i < numIndices; i++)
			p.indices.push_back(indices[i] + vertex_count);
	}
}

PreMeshBuffer &MeshCollector::findBuffer(
		const TileLayer &layer, u32 numVertices)
{
	if (numVertices > U16_MAX)
		throw std::invalid_argument(
				"Mesh can't contain more than 65536 vertices");

	for (PreMeshBuffer &p : prebuffers)
		if (p.layer == layer && p.vertices.size() + numVertices <= U16_MAX)
			return p;
	prebuffers.emplace_back(layer);
	return prebuffers.back();
}
