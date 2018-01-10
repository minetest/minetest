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
};

struct MeshCollector
{
	std::array<std::vector<PreMeshBuffer>, MAX_TILE_LAYERS> prebuffers;

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

	PreMeshBuffer &findBuffer(const TileLayer &layer, u8 layernum,
			u32 numIndices);
};
