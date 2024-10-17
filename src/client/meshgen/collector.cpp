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
#include <stdexcept>
#include "log.h"

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices,
			   layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices,
		u8 layernum, bool use_scale)
{
	PreMeshBuffer &p = findBuffer(layer, layernum, numVertices);

	f32 scale = 1.0f;
	if (use_scale)
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

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices,
			   pos, c, light_source, layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source, u8 layernum, bool use_scale)
{
	PreMeshBuffer &p = findBuffer(layer, layernum, numVertices);

	f32 scale = 1.0f;
	if (use_scale)
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

PreMeshBuffer &MeshCollector::findBuffer(
		const TileLayer &layer, u8 layernum, u32 numVertices)
{
	if (numVertices > U16_MAX)
		throw std::invalid_argument(
				"Mesh can't contain more than 65536 vertices");
	std::vector<PreMeshBuffer> &buffers = prebuffers[layernum];
	for (PreMeshBuffer &p : buffers)
		if (p.layer == layer && p.vertices.size() + numVertices <= U16_MAX)
			return p;
	buffers.emplace_back(layer);
	return buffers.back();
}
