#pragma once
#include <array>
#include <vector>
#include "irrlichttypes.h"
#include <S3DVertex.h>
#include "client/tile.h"
#include "client/tileref.h"

struct PreMeshBuffer
{
	LayerRef layer;
	std::vector<u16> indices;
	std::vector<video::S3DVertex> vertices;

	PreMeshBuffer() = default;
	explicit PreMeshBuffer(const LayerRef &layer) : layer(layer) {}
};

struct MeshCollector
{
	std::array<std::vector<PreMeshBuffer>, MAX_TILE_LAYERS> prebuffers;

	// clang-format off
	void append(const TileRef &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices);
	void append(const TileRef &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices,
			v3f pos, video::SColor c, u8 light_source);
	// clang-format on

private:
	// clang-format off
	void append(const LayerRef &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices);
	void append(const LayerRef &material,
			const video::S3DVertex *vertices, u32 numVertices,
			const u16 *indices, u32 numIndices,
			v3f pos, video::SColor c, u8 light_source);
	// clang-format on

	PreMeshBuffer &findBuffer(const LayerRef &layer, u32 numVertices);
};
