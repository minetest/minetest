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

#include "collector.h"
#include "log.h"

template <bool use_tangent_vertices>
auto MeshCollector<use_tangent_vertices>::getBuffer(u8 layernum,
		const TileLayer &layer, u32 free_indices_needed) -> PreBuffer *
{
	if (free_indices_needed > 65535) {
		dstream << "FIXME: MeshCollector: " << free_indices_needed
			<< " indices requested (limit 65535)" << std::endl;
		return nullptr;
	}
	std::vector<PreBuffer> &buffers = prebuffers[layernum];
	for (PreBuffer &p : buffers)
		if (p.layer == layer && p.indices.size() + free_indices_needed <= 65535)
			return &p;
	buffers.push_back({layer});
	return &buffers.back();
}

template <bool use_tangent_vertices>
void MeshCollector<use_tangent_vertices>::append(const TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer &layer = tile.layers[layernum];
		if (layer.texture_id == 0)
			continue;
		append(layer, vertices, numVertices, indices, numIndices,
			layernum, tile.world_aligned);
	}
}

template <bool use_tangent_vertices>
void MeshCollector<use_tangent_vertices>::append(const TileLayer &layer,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices, u8 layernum,
		bool use_scale)
{
	PreBuffer *p = getBuffer(layernum, layer, numIndices);
	if (!p)
		return;
	f32 scale = use_scale ? 1.0 / layer.scale : 1.0;
	u32 vertex_count = p->vertices.size();
	for (u32 i = 0; i < numVertices; i++) {
		const video::S3DVertex &v = vertices[i];
		p->vertices.emplace_back(v.Pos, v.Normal, v.Color, scale * v.TCoords);
	}
	for (u32 i = 0; i < numIndices; i++)
		p->indices.push_back(vertex_count + indices[i]);
}

template <bool use_tangent_vertices>
void MeshCollector<use_tangent_vertices>::append(const TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer &layer = tile.layers[layernum];
		if (layer.texture_id == 0)
			continue;
		append(layer, vertices, numVertices, indices, numIndices, pos,
				c, light_source, layernum, tile.world_aligned);
	}
}

template <bool use_tangent_vertices>
void MeshCollector<use_tangent_vertices>::append(const TileLayer &layer,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source, u8 layernum,
		bool use_scale)
{
	PreBuffer *p = getBuffer(layernum, layer, numIndices);
	if (!p)
		return;
	f32 scale = use_scale ? 1.0 / layer.scale : 1.0;
	video::SColor original_c = c;
	u32 vertex_count = p->vertices.size();
	for (u32 i = 0; i < numVertices; i++) {
		const video::S3DVertex &v = vertices[i];
		if (!light_source) {
			c = original_c;
			applyFacesShading(c, v.Normal);
		}
		p->vertices.emplace_back(v.Pos + pos, v.Normal, c, scale * v.TCoords);
	}
	for (u32 i = 0; i < numIndices; i++)
		p->indices.push_back(vertex_count + indices[i]);
}

template <bool use_tangent_vertices>
void MeshCollector<use_tangent_vertices>::applyTileColors()
{
	for (auto &prebuffer : prebuffers)
		for (PreBuffer &pmb : prebuffer) {
			video::SColor tc = pmb.layer.color;
			if (tc == video::SColor(0xFFFFFFFF))
				continue;
			for (Vertex &vertex : pmb.vertices) {
				video::SColor *c = &vertex.Color;
				c->set(c->getAlpha(),
						c->getRed() * tc.getRed() / 255,
						c->getGreen() * tc.getGreen() / 255,
						c->getBlue() * tc.getBlue() / 255);
			}
		}
}

template struct MeshCollector<false>;
template struct MeshCollector<true>;
