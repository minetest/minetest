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

#pragma once
#include <array>
#include <vector>
#include "irrlichttypes.h"
#include <S3DVertex.h>
#include "client/tile.h"

struct PreMeshBuffer
{
	TileLayer layer;
	std::vector<u16> indices;
	std::vector<video::S3DVertex> vertices;

	PreMeshBuffer() = default;
	explicit PreMeshBuffer(const TileLayer &layer) : layer(layer) {}
};

struct MeshCollector
{
	std::array<std::vector<PreMeshBuffer>, MAX_TILE_LAYERS> prebuffers;
	// bounding sphere radius and center
	f32 m_bounding_radius_sq = 0.0f;
	v3f m_center_pos;

	// center_pos: pos to use for bounding-sphere, in BS-space
	MeshCollector(const v3f center_pos) : m_center_pos(center_pos) {}

	// clang-format off
	void append(const TileSpec &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices);
	void append(const TileSpec &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices,
			v3f pos, video::SColor c, u8 light_source);
	// clang-format on

private:
	// clang-format off
	void append(const TileLayer &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices,
			u8 layernum, bool use_scale = false);
	void append(const TileLayer &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices,
			v3f pos, video::SColor c, u8 light_source,
			u8 layernum, bool use_scale = false);
	// clang-format on

	PreMeshBuffer &findBuffer(const TileLayer &layer, u8 layernum, u32 numVertices);
};
