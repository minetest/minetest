#include "collector.h"
#include <stdexcept>
#include "log.h"
#include "mesh.h"

void MeshCollector::append(const TileRef &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++)
		append(tile[layernum], vertices, numVertices, indices, numIndices);
}

void MeshCollector::append(const LayerRef &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices)
{
	if (!layer)
		return;
	PreMeshBuffer &p = findBuffer(layer, numVertices);

	f32 scale = 1.0f;
	if (layer.tile->world_aligned)
		scale = 1.0f / layer->scale;

	u32 vertex_count = p.vertices.size();
	for (u32 i = 0; i < numVertices; i++)
		p.vertices.emplace_back(vertices[i].Pos, vertices[i].Normal,
				vertices[i].Color, scale * vertices[i].TCoords);

	for (u32 i = 0; i < numIndices; i++)
		p.indices.push_back(indices[i] + vertex_count);
}

void MeshCollector::append(const TileRef &tile, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++)
		append(tile[layernum], vertices, numVertices, indices, numIndices, pos, c, light_source);
}

void MeshCollector::append(const LayerRef &layer, const video::S3DVertex *vertices,
		u32 numVertices, const u16 *indices, u32 numIndices, v3f pos,
		video::SColor c, u8 light_source)
{
	if (!layer)
		return;
	PreMeshBuffer &p = findBuffer(layer, numVertices);

	f32 scale = 1.0f;
	if (layer.tile->world_aligned)
		scale = 1.0f / layer->scale;

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

PreMeshBuffer &MeshCollector::findBuffer(const LayerRef &layer, u32 numVertices)
{
	if (numVertices > U16_MAX)
		throw std::invalid_argument(
				"Mesh can't contain more than 65536 vertices");
	std::vector<PreMeshBuffer> &buffers = prebuffers[layer.layer];
	for (PreMeshBuffer &p : buffers)
		if (p.vertices.size() + numVertices <= U16_MAX && p.layer == layer)
			return p;
	buffers.emplace_back(layer);
	return buffers.back();
}
