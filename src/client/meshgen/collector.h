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

#pragma once

#include "irrlichttypes_extrabloated.h"
#include <vector>
#include <unordered_map>
#include <map>
#include <array>
#include "client/tile.h"
#include "client/texture_atlas.h"

class Client;
class IWritableShaderSource;
class ITextureSource;

class MeshCollector
{
public:
	// bounding sphere radius and center
	f32 bounding_radius_sq = 0.0f;
	v3f center_pos;
	v3f offset;

    MeshCollector(v3f _center_pos, v3f _offset)
        : center_pos(_center_pos), offset(_offset)
    {}

    virtual void addTileMesh(TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices, bool outside_uv=false,
		v3f pos = v3f(0.0f), video::SColor clr = video::SColor(),
		u8 light_source = 0, bool own_color=false) = 0;
};

// represents a triangle as indexes into the vertex buffer in SMeshBuffer
class MeshTriangle
{
public:
	scene::SMeshBuffer *buffer;
	u16 p1, p2, p3;
	v3f centroid;
	float areaSQ;

	void updateAttributes()
	{
		v3f v1 = buffer->getPosition(p1);
		v3f v2 = buffer->getPosition(p2);
		v3f v3 = buffer->getPosition(p3);

		centroid = (v1 + v2 + v3) / 3;
		areaSQ = (v2-v1).crossProduct(v3-v1).getLengthSQ() / 4;
	}

	v3f getNormal() const {
		v3f v1 = buffer->getPosition(p1);
		v3f v2 = buffer->getPosition(p2);
		v3f v3 = buffer->getPosition(p3);

		return (v2-v1).crossProduct(v3-v1);
	}
};

/*
 * PartialMeshBuffer
 *
 * Attach alternate `Indices` to an existing mesh buffer, to make it possible to use different
 * indices with the same vertex buffer.
 *
 * Irrlicht does not currently support this: `CMeshBuffer` ties together a single vertex buffer
 * and a single index buffer. There's no way to share these between mesh buffers.
 *
 */
class PartialMeshBuffer
{
public:
	PartialMeshBuffer(scene::SMeshBuffer *buffer, std::vector<u16> &&vertex_indexes) :
			m_buffer(buffer), m_vertex_indexes(std::move(vertex_indexes))
	{}

	scene::IMeshBuffer *getBuffer() const { return m_buffer; }
	const std::vector<u16> &getVertexIndexes() const { return m_vertex_indexes; }

	void beforeDraw() const;
	void afterDraw() const;
private:
	scene::SMeshBuffer *m_buffer;
	mutable std::vector<u16> m_vertex_indexes;
};

struct MeshPart
{
	std::vector<video::S3DVertex> vertices;
	std::vector<u16> indices;

	std::vector<u16> opaque_verts_refs;

	scene::SMeshBuffer *buffer;
};

class MapblockMeshCollector final : public MeshCollector
{
	Client *client;

public:
	std::list<std::pair<video::SMaterial, std::list<MeshPart>>> layers;
	std::list<std::pair<video::SMaterial, std::list<PartialMeshBuffer>>> transparent_layers;

	std::map<u32, AnimationInfo> animated_textures;
	std::map<u32, std::string> crack_textures;

	int last_crack;

	v3f translation;

	std::vector<MeshTriangle> transparent_triangles;

	bool enable_shaders;
	bool bilinear_filter;
	bool trilinear_filter;
	bool anisotropic_filter;

    MapblockMeshCollector(Client *_client, v3f _center_pos,
        v3f _offset, v3f _translation);

    ~MapblockMeshCollector()
    {
        for (auto &layer : layers)
            for (auto &part : layer.second)
                part.buffer->drop();
    }

    void addTileMesh(TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices, bool outside_uv=false,
		v3f pos = v3f(0.0f), video::SColor clr = video::SColor(),
		u8 light_source = 0, bool own_color=false) override;
};

struct WieldPreMeshBuffer
{
	TileLayer layer;
	std::vector<u16> indices;
	std::vector<video::S3DVertex> vertices;

	WieldPreMeshBuffer() = default;
	explicit WieldPreMeshBuffer(const TileLayer &layer) : layer(layer) {}
};

class WieldMeshCollector final : public MeshCollector
{
public:
	std::vector<WieldPreMeshBuffer> prebuffers;

	// center_pos: pos to use for bounding-sphere, in BS-space
	// offset: offset added to vertices
    WieldMeshCollector(v3f _center_pos, v3f _offset)
        : MeshCollector(_center_pos, _offset) {}

	void addTileMesh(TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices, bool outside_uv=false,
		v3f pos = v3f(0.0f), video::SColor clr = video::SColor(),
		u8 light_source = 0, bool own_color=false) override;

private:
	WieldPreMeshBuffer &findBuffer(const TileLayer &layer, u32 numVertices);
};
