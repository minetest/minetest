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
#include "profiler.h"

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, layernum,
				tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, u8 layernum,
		bool use_scale)
{
	PreMeshBuffer &p = findBuffer(layer, layernum, numVertices);

	f32 scale = 1.0f;
	if (use_scale)
		scale = 1.0f / layer.scale;

	u32 vertex_count = p.vertices.size();
	for (u32 i = 0; i < numVertices; i++)
		p.vertices.emplace_back(vertices[i].Pos, vertices[i].Normal,
				vertices[i].Color, scale * vertices[i].TCoords);

	for (u32 i = 0; i < numIndices; i++)
		p.indices.push_back(indices[i] + vertex_count);
}

void MeshCollector::append(const TileSpec &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, pos, c,
				light_source, layernum, tile.world_aligned);
	}
}

void MeshCollector::append(const TileLayer &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor c, u8 light_source, u8 layernum, bool use_scale)
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
		p.vertices.emplace_back(vertices[i].Pos + pos, vertices[i].Normal, color,
				scale * vertices[i].TCoords);
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

	// avoid scanning the entire list of buffers when filling with the same material
	if (latest_buffers[layernum].size() > 0 && 
			buffers[latest_buffers[layernum].back()].layer == layer && 
			buffers[latest_buffers[layernum].back()].vertices.size() + numVertices <= U16_MAX)
		return buffers[latest_buffers[layernum].back()];

	for (PreMeshBuffer &p : buffers)
		if (!p.closed && p.layer == layer && p.vertices.size() + numVertices <= U16_MAX)
			return p;

	buffers.emplace_back(layer);
	latest_buffers[layernum].push_back(buffers.size()-1);
	return buffers.back();
}

void MeshCollector::startNewMeshLayer()
{
	for (s16 tile_layer = 0; tile_layer < MAX_TILE_LAYERS; ++tile_layer) {
		// do not close mesh buffer if there is only one
		if (latest_buffers[tile_layer].size() == 1)
			continue;

		s16 latest_buffer = -1;
		if (latest_buffers[tile_layer].size() > 0)
			latest_buffer = latest_buffers[tile_layer].back();

		for (s16 index: latest_buffers[tile_layer])
			if (index != latest_buffer) {
				auto &buffer = prebuffers[tile_layer][index];
				if (buffer.layer.material_type == TILE_MATERIAL_ALPHA ||
						buffer.layer.material_type == TILE_MATERIAL_PLAIN_ALPHA ||
						buffer.layer.material_type == TILE_MATERIAL_LIQUID_TRANSPARENT ||
						buffer.layer.material_type == TILE_MATERIAL_WAVING_LIQUID_TRANSPARENT)
					prebuffers[tile_layer][index].closed = true;
			}

		latest_buffers[tile_layer].clear();
		if (latest_buffer != -1)
			latest_buffers[tile_layer].push_back(latest_buffer);
	}
}