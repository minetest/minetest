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
#include <map>
#include <vector>
#include "client/tile.h"
#include "irrlichttypes_extrabloated.h"
#include "mesh.h"

template <bool use_tangent_vertices = false>
struct VertexType {
	typedef video::S3DVertex type;
};

template <>
struct VertexType<true> {
	typedef video::S3DVertexTangents type;
};

template <bool use_tangent_vertices>
struct PreMeshBuffer {
	typedef typename VertexType<use_tangent_vertices>::type Vertex;
	TileLayer layer;
	std::vector<u16> indices;
	std::vector<Vertex> vertices;
};

template <bool use_tangent_vertices>
struct MeshCollector {
	typedef typename VertexType<use_tangent_vertices>::type Vertex;
	typedef PreMeshBuffer<use_tangent_vertices> PreBuffer;
	static constexpr bool m_use_tangent_vertices = use_tangent_vertices;
	std::vector<PreBuffer> prebuffers[MAX_TILE_LAYERS];

	void append(const TileSpec &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices);
	void append(const TileLayer &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices, u8 layernum,
			bool use_scale = false);
	void append(const TileSpec &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices, v3f pos,
			video::SColor c, u8 light_source);
	void append(const TileLayer &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices, v3f pos,
			video::SColor c, u8 light_source, u8 layernum,
			bool use_scale = false);
	/*!
	 * Colorizes all vertices in the collector.
	 */
	void applyTileColors();

private:
	PreBuffer *getBuffer(u8 layernum, const TileLayer &layer,
			u32 free_indices_needed);
};
