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

#include "content_mapblock.h"
#include "content_mapblock_private.h"
#include "util/numeric.h"
#include "util/directiontables.h"
#include "mapblock_mesh.h" // For MapBlock_LightColor() and MeshCollector
#include "settings.h"
#include "nodedef.h"
#include "client/tile.h"
#include "mesh.h"
#include <IMeshManipulator.h>
#include "client.h"
#include "log.h"
#include "noise.h"

// Distance of light extrapolation (for oversized nodes)
// After this distance, it gives up and considers light level constant
#define SMOOTH_LIGHTING_OVERSIZE 1.0

static const v3s16 light_dirs[8] = {
	v3s16(-1, -1, -1),
	v3s16(-1, -1,  1),
	v3s16(-1,  1, -1),
	v3s16(-1,  1,  1),
	v3s16( 1, -1, -1),
	v3s16( 1, -1,  1),
	v3s16( 1,  1, -1),
	v3s16( 1,  1,  1),
};

const std::string MapblockMeshGenerator::raillike_groupname = "connect_to_raillike";

MapblockMeshGenerator::MapblockMeshGenerator(MeshMakeData *input, MeshCollector *output)
{
	data      = input;
	collector = output;

	nodedef   = data->m_client->ndef();
	smgr      = data->m_client->getSceneManager();
	meshmanip = smgr->getMeshManipulator();

	enable_mesh_cache = g_settings->getBool("enable_mesh_cache") &&
		!data->m_smooth_lighting; // Mesh cache is not supported with smooth lighting

	blockpos_nodes = data->m_blockpos * MAP_BLOCKSIZE;
}

// Create a cuboid.
//  tiles     - the tiles (materials) to use (for all 6 faces)
//  tilecount - number of entries in tiles, 1<=tilecount<=6
//  lights    - vertex light levels. The order is the same as in light_dirs.
//              NULL may be passed if smooth lighting is disabled.
//  txc       - texture coordinates - this is a list of texture coordinates
//              for the opposite corners of each face - therefore, there
//              should be (2+2)*6=24 values in the list. The order of
//              the faces in the list is up-down-right-left-back-front
//              (compatible with ContentFeatures).
void MapblockMeshGenerator::drawCuboid(const aabb3f &box,
	TileSpec *tiles, int tilecount, const u16 *lights, const f32 *txc)
{
	assert(tilecount >= 1 && tilecount <= 6); // pre-condition

	v3f min = box.MinEdge;
	v3f max = box.MaxEdge;

	video::SColor colors[6];
	if (!data->m_smooth_lighting) {
		for (int face = 0; face != 6; ++face) {
			int tileindex = MYMIN(face, tilecount - 1);
			colors[face] = encode_light_and_color(light, tiles[tileindex].color, frame.light_source);
		}
		if (!frame.light_source) {
			applyFacesShading(colors[0], v3f(0, 1, 0));
			applyFacesShading(colors[1], v3f(0, -1, 0));
			applyFacesShading(colors[2], v3f(1, 0, 0));
			applyFacesShading(colors[3], v3f(-1, 0, 0));
			applyFacesShading(colors[4], v3f(0, 0, 1));
			applyFacesShading(colors[5], v3f(0, 0, -1));
		}
	}

	video::S3DVertex vertices[24] = {
		// top
		video::S3DVertex(min.X,max.Y,max.Z, 0,1,0, colors[0], txc[0],txc[1]),
		video::S3DVertex(max.X,max.Y,max.Z, 0,1,0, colors[0], txc[2],txc[1]),
		video::S3DVertex(max.X,max.Y,min.Z, 0,1,0, colors[0], txc[2],txc[3]),
		video::S3DVertex(min.X,max.Y,min.Z, 0,1,0, colors[0], txc[0],txc[3]),
		// bottom
		video::S3DVertex(min.X,min.Y,min.Z, 0,-1,0, colors[1], txc[4],txc[5]),
		video::S3DVertex(max.X,min.Y,min.Z, 0,-1,0, colors[1], txc[6],txc[5]),
		video::S3DVertex(max.X,min.Y,max.Z, 0,-1,0, colors[1], txc[6],txc[7]),
		video::S3DVertex(min.X,min.Y,max.Z, 0,-1,0, colors[1], txc[4],txc[7]),
		// right
		video::S3DVertex(max.X,max.Y,min.Z, 1,0,0, colors[2], txc[ 8],txc[9]),
		video::S3DVertex(max.X,max.Y,max.Z, 1,0,0, colors[2], txc[10],txc[9]),
		video::S3DVertex(max.X,min.Y,max.Z, 1,0,0, colors[2], txc[10],txc[11]),
		video::S3DVertex(max.X,min.Y,min.Z, 1,0,0, colors[2], txc[ 8],txc[11]),
		// left
		video::S3DVertex(min.X,max.Y,max.Z, -1,0,0, colors[3], txc[12],txc[13]),
		video::S3DVertex(min.X,max.Y,min.Z, -1,0,0, colors[3], txc[14],txc[13]),
		video::S3DVertex(min.X,min.Y,min.Z, -1,0,0, colors[3], txc[14],txc[15]),
		video::S3DVertex(min.X,min.Y,max.Z, -1,0,0, colors[3], txc[12],txc[15]),
		// back
		video::S3DVertex(max.X,max.Y,max.Z, 0,0,1, colors[4], txc[16],txc[17]),
		video::S3DVertex(min.X,max.Y,max.Z, 0,0,1, colors[4], txc[18],txc[17]),
		video::S3DVertex(min.X,min.Y,max.Z, 0,0,1, colors[4], txc[18],txc[19]),
		video::S3DVertex(max.X,min.Y,max.Z, 0,0,1, colors[4], txc[16],txc[19]),
		// front
		video::S3DVertex(min.X,max.Y,min.Z, 0,0,-1, colors[5], txc[20],txc[21]),
		video::S3DVertex(max.X,max.Y,min.Z, 0,0,-1, colors[5], txc[22],txc[21]),
		video::S3DVertex(max.X,min.Y,min.Z, 0,0,-1, colors[5], txc[22],txc[23]),
		video::S3DVertex(min.X,min.Y,min.Z, 0,0,-1, colors[5], txc[20],txc[23]),
	};

	static const u8 light_indices[24] = {
		3, 7, 6, 2,
		0, 4, 5, 1,
		6, 7, 5, 4,
		3, 2, 0, 1,
		7, 3, 1, 5,
		2, 6, 4, 0
	};

	for(int i = 0; i < 6; i++) {
		switch (tiles[MYMIN(i, tilecount-1)].rotation) {
		case 0:
			break;
		case 1: //R90
			for (int x = 0; x < 4; x++)
				vertices[i*4+x].TCoords.rotateBy(90,irr::core::vector2df(0, 0));
			break;
		case 2: //R180
			for (int x = 0; x < 4; x++)
				vertices[i*4+x].TCoords.rotateBy(180,irr::core::vector2df(0, 0));
			break;
		case 3: //R270
			for (int x = 0; x < 4; x++)
				vertices[i*4+x].TCoords.rotateBy(270,irr::core::vector2df(0, 0));
			break;
		case 4: //FXR90
			for (int x = 0; x < 4; x++) {
				vertices[i*4+x].TCoords.X = 1.0 - vertices[i*4+x].TCoords.X;
				vertices[i*4+x].TCoords.rotateBy(90,irr::core::vector2df(0, 0));
			}
			break;
		case 5: //FXR270
			for (int x = 0; x < 4; x++) {
				vertices[i*4+x].TCoords.X = 1.0 - vertices[i*4+x].TCoords.X;
				vertices[i*4+x].TCoords.rotateBy(270,irr::core::vector2df(0, 0));
			}
			break;
		case 6: //FYR90
			for (int x = 0; x < 4; x++) {
				vertices[i*4+x].TCoords.Y = 1.0 - vertices[i*4+x].TCoords.Y;
				vertices[i*4+x].TCoords.rotateBy(90,irr::core::vector2df(0, 0));
			}
			break;
		case 7: //FYR270
			for (int x = 0; x < 4; x++) {
				vertices[i*4+x].TCoords.Y = 1.0 - vertices[i*4+x].TCoords.Y;
				vertices[i*4+x].TCoords.rotateBy(270,irr::core::vector2df(0, 0));
			}
			break;
		case 8: //FX
			for (int x = 0; x < 4; x++) {
				vertices[i*4+x].TCoords.X = 1.0 - vertices[i*4+x].TCoords.X;
			}
			break;
		case 9: //FY
			for (int x = 0; x < 4; x++) {
				vertices[i*4+x].TCoords.Y = 1.0 - vertices[i*4+x].TCoords.Y;
			}
			break;
		default:
			break;
		}
	}

	if (data->m_smooth_lighting) {
		for (int j = 0; j < 24; ++j) {
			int tileindex = MYMIN(j / 4, tilecount - 1);
			vertices[j].Color = encode_light_and_color(lights[light_indices[j]],
				tiles[tileindex].color, frame.light_source);
			if (!frame.light_source)
				applyFacesShading(vertices[j].Color, vertices[j].Normal);
		}
	}

	// Add to mesh collector
	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	for (int k = 0; k < 6; ++k) {
		int tileindex = MYMIN(k, tilecount - 1);
		collector->append(tiles[tileindex], vertices + 4 * k, 4, indices, 6);
	}
}

// Gets the base lighting values for a node
void MapblockMeshGenerator::getSmoothLightFrame()
{
	for (int k = 0; k < 8; ++k) {
		u16 light = getSmoothLight(blockpos_nodes + p, light_dirs[k], data);
		frame.lightsA[k] = light & 0xff;
		frame.lightsB[k] = light >> 8;
	}
	frame.light_source = f->light_source;
}

// Calculates vertex light level
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
u16 MapblockMeshGenerator::blendLight(const v3f &vertex_pos)
{
	f32 x = core::clamp(vertex_pos.X / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 y = core::clamp(vertex_pos.Y / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 z = core::clamp(vertex_pos.Z / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 lightA = 0.0;
	f32 lightB = 0.0;
	for (int k = 0; k < 8; ++k) {
		f32 dx = (k & 4) ? x : 1 - x;
		f32 dy = (k & 2) ? y : 1 - y;
		f32 dz = (k & 1) ? z : 1 - z;
		lightA += dx * dy * dz * frame.lightsA[k];
		lightB += dx * dy * dz * frame.lightsB[k];
	}
	return
		core::clamp(core::round32(lightA), 0, 255) |
		core::clamp(core::round32(lightB), 0, 255) << 8;
}

// Calculates vertex color to be used in mapblock mesh
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
//  tile_color - node's tile color
video::SColor MapblockMeshGenerator::blendLight(const v3f &vertex_pos,
	video::SColor tile_color)
{
	u16 light = blendLight(vertex_pos);
	return encode_light_and_color(light, tile_color, frame.light_source);
}

video::SColor MapblockMeshGenerator::blendLight(const v3f &vertex_pos,
	const v3f &vertex_normal, video::SColor tile_color)
{
	video::SColor color = blendLight(vertex_pos, tile_color);
	if (!frame.light_source)
			applyFacesShading(color, vertex_normal);
	return color;
}

static inline void getNeighborConnectingFace(v3s16 p, INodeDefManager *nodedef,
		MeshMakeData *data, MapNode n, int v, int *neighbors)
{
	MapNode n2 = data->m_vmanip.getNodeNoEx(p);
	if (nodedef->nodeboxConnects(n, n2, v))
		*neighbors |= v;
}

void MapblockMeshGenerator::drawAutoLightedCuboid(aabb3f box)
{
	f32 dx1 = box.MinEdge.X;
	f32 dy1 = box.MinEdge.Y;
	f32 dz1 = box.MinEdge.Z;
	f32 dx2 = box.MaxEdge.X;
	f32 dy2 = box.MaxEdge.Y;
	f32 dz2 = box.MaxEdge.Z;
	box.MinEdge += origin;
	box.MaxEdge += origin;
	f32 tx1 = (box.MinEdge.X / BS) + 0.5;
	f32 ty1 = (box.MinEdge.Y / BS) + 0.5;
	f32 tz1 = (box.MinEdge.Z / BS) + 0.5;
	f32 tx2 = (box.MaxEdge.X / BS) + 0.5;
	f32 ty2 = (box.MaxEdge.Y / BS) + 0.5;
	f32 tz2 = (box.MaxEdge.Z / BS) + 0.5;
	f32 txc[24] = {
		  tx1, 1-tz2,   tx2, 1-tz1, // up
		  tx1,   tz1,   tx2,   tz2, // down
		  tz1, 1-ty2,   tz2, 1-ty1, // right
		1-tz2, 1-ty2, 1-tz1, 1-ty1, // left
		1-tx2, 1-ty2, 1-tx1, 1-ty1, // back
		  tx1, 1-ty2,   tx2, 1-ty1, // front
	};
	if (data->m_smooth_lighting) {
		u16 lights[8];
		for (int j = 0; j < 8; ++j) {
			f32 x = (j & 4) ? dx2 : dx1;
			f32 y = (j & 2) ? dy2 : dy1;
			f32 z = (j & 1) ? dz2 : dz1;
			lights[j] = blendLight(v3f(x, y, z));
		}
		drawCuboid(box, &tile, 1, lights, txc);
	} else {
		drawCuboid(box, &tile, 1, NULL, txc);
	}
}

void MapblockMeshGenerator::drawAutoLightedCuboidEx(aabb3f box, const f32 *txc)
{
	f32 dx1 = box.MinEdge.X;
	f32 dy1 = box.MinEdge.Y;
	f32 dz1 = box.MinEdge.Z;
	f32 dx2 = box.MaxEdge.X;
	f32 dy2 = box.MaxEdge.Y;
	f32 dz2 = box.MaxEdge.Z;
	box.MinEdge += origin;
	box.MaxEdge += origin;
	if (data->m_smooth_lighting) {
		u16 lights[8];
		for (int j = 0; j < 8; ++j) {
			v3f d;
			d.X = (j & 4) ? dx2 : dx1;
			d.Y = (j & 2) ? dy2 : dy1;
			d.Z = (j & 1) ? dz2 : dz1;
			lights[j] = blendLight(d);
		}
		drawCuboid(box, &tile, 1, lights, txc);
	} else {
		drawCuboid(box, &tile, 1, NULL, txc);
	}
}

/*!
 * Returns the i-th special tile for a map node.
 */
static TileSpec getSpecialTile(const ContentFeatures &f,
	const MapNode &n, u8 i)
{
	TileSpec copy = f.special_tiles[i];
	if (!copy.has_color)
		n.getColor(f, &copy.color);
	return copy;
}

void MapblockMeshGenerator::prepareLiquidNodeDrawing(bool flowing)
{
	tile_liquid_top = getSpecialTile(*f, n, 0);
	tile_liquid = getSpecialTile(*f, n, flowing ? 1 : 0);

	MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(p.X,p.Y+1,p.Z));
	c_flowing = nodedef->getId(f->liquid_alternative_flowing);
	c_source = nodedef->getId(f->liquid_alternative_source);
	top_is_same_liquid = (ntop.getContent() == c_flowing) || (ntop.getContent() == c_source);

	if (data->m_smooth_lighting)
		return; // don't need to pre-compute anything in this case

	// If this liquid emits light and doesn't contain light, draw
	// it at what it emits, for an increased effect
	if (f->light_source != 0) {
		light = decode_light(f->light_source);
		light = light | (light << 8);
	}
	// Use the light of the node on top if possible
	else if (nodedef->get(ntop).param_type == CPT_LIGHT)
		light = getInteriorLight(ntop, 0, nodedef);

	color_liquid_top = encode_light_and_color(light, tile_liquid_top.color, f->light_source);
	color = encode_light_and_color(light, tile_liquid.color, f->light_source);
}

void MapblockMeshGenerator::getLiquidNeighborhood(bool flowing)
{
	u8 range = rangelim(nodedef->get(c_flowing).liquid_range, 1, 8);

	for (int w = -1; w <= 1; w++)
	for (int u = -1; u <= 1; u++)
	{
		// Skip getting unneeded data
		if (!flowing && u && w)
			continue;

		NeighborData &neighbor = liquid_neighbors[w + 1][u + 1];
		v3s16 p2 = p + v3s16(u, 0, w);
		MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
		neighbor.content = n2.getContent();
		neighbor.level = -0.5 * BS;
		neighbor.is_same_liquid = false;
		neighbor.top_is_same_liquid = false;

		if (neighbor.content == CONTENT_IGNORE)
			continue;

		if (neighbor.content == c_source) {
			neighbor.is_same_liquid = true;
			neighbor.level = 0.5 * BS;
		} else if (neighbor.content == c_flowing) {
			neighbor.is_same_liquid = true;
			u8 liquid_level = (n2.param2 & LIQUID_LEVEL_MASK);
			if (liquid_level <= LIQUID_LEVEL_MAX + 1 - range)
				liquid_level = 0;
			else
				liquid_level -= (LIQUID_LEVEL_MAX + 1 - range);
			neighbor.level = (-0.5 + (liquid_level + 0.5) / range) * BS;
		}

		// Check node above neighbor.
		// NOTE: This doesn't get executed if neighbor
		//       doesn't exist
		p2.Y += 1;
		n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
		if (n2.getContent() == c_source || n2.getContent() == c_flowing)
			neighbor.top_is_same_liquid = true;
	}
}

void MapblockMeshGenerator::resetCornerLevels()
{
	for (int k = 0; k < 2; k++)
	for (int i = 0; i < 2; i++)
		corner_levels[k][i] = 0.5 * BS;
}

void MapblockMeshGenerator::calculateCornerLevels()
{
	for (int k = 0; k < 2; k++)
	for (int i = 0; i < 2; i++)
		corner_levels[k][i] = getCornerLevel(i, k);
}

f32 MapblockMeshGenerator::getCornerLevel(int i, int k)
{
	float sum = 0;
	int count = 0;
	int air_count = 0;
	for (int dk = 0; dk < 2; dk++)
	for (int di = 0; di < 2; di++)
	{
		NeighborData &neighbor_data = liquid_neighbors[k + dk][i + di];
		content_t content = neighbor_data.content;

		// If top is liquid, draw starting from top of node
		if (neighbor_data.top_is_same_liquid)
			return 0.5 * BS;

		// Source always has the full height
		if(content == c_source)
			return sum = 0.5 * BS;

		// Flowing liquid has level information
		if(content == c_flowing) {
			sum += neighbor_data.level;
			count++;
		}
		else if(content == CONTENT_AIR) {
			air_count++;
			if(air_count >= 2)
				return -0.5 * BS + 0.2;
		}
	}
	if(count > 0)
		return sum / count;
	return 0;
}

void MapblockMeshGenerator::drawLiquidSides(bool flowing)
{
	struct LiquidFaceDesc {
		v3s16 dir; // XZ
		v3s16 p[2]; // XZ only; 1 means +, 0 means -
	};
	struct UV {
		int u, v;
	};
	static const LiquidFaceDesc base_faces[4] = {
		{ v3s16( 1, 0,  0), { v3s16(1, 0, 1), v3s16(1, 0, 0) }},
		{ v3s16(-1, 0,  0), { v3s16(0, 0, 0), v3s16(0, 0, 1) }},
		{ v3s16( 0, 0,  1), { v3s16(0, 0, 1), v3s16(1, 0, 1) }},
		{ v3s16( 0, 0, -1), { v3s16(1, 0, 0), v3s16(0, 0, 0) }},
	};
	static const UV base_vertices[4] = {
		{ 0, 1 },
		{ 1, 1 },
		{ 1, 0 },
		{ 0, 0 }
	};
	for (int i = 0; i < 4; i++) {
		const LiquidFaceDesc &face = base_faces[i];
		const NeighborData &neighbor = liquid_neighbors[face.dir.Z + 1][face.dir.X + 1];

		// No face between nodes of the same liquid, unless there is node
		// at the top to which it should be connected. Again, unless the face
		// there would be inside the liquid
		if (neighbor.is_same_liquid) {
			if (!flowing)
				continue;
			if (!top_is_same_liquid)
				continue;
			if (neighbor.top_is_same_liquid)
				continue;
		}

		if (!flowing && (neighbor.content == CONTENT_IGNORE))
			continue;

		const ContentFeatures &neighbor_features = nodedef->get(neighbor.content);
		// Don't draw face if neighbor is blocking the view
		if (neighbor_features.solidness == 2)
			continue;

		video::S3DVertex vertices[4];
		for (int j = 0; j < 4; j++) {
			const UV &vertex = base_vertices[j];
			const v3s16 &base = face.p[vertex.u];
			v3f pos;
			pos.X = (base.X - 0.5) * BS;
			pos.Z = (base.Z - 0.5) * BS;
			if (vertex.v)
				pos.Y = neighbor.is_same_liquid ? corner_levels[base.Z][base.X] : -0.5 * BS;
			else
				pos.Y =     !top_is_same_liquid ? corner_levels[base.Z][base.X] :  0.5 * BS;
			if (data->m_smooth_lighting)
				color = blendLight(pos, tile_liquid.color);
			pos += origin;
			vertices[j] = video::S3DVertex(pos.X, pos.Y, pos.Z, 0, 0, 0, color, vertex.u, vertex.v);
		};
		static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
		collector->append(tile_liquid, vertices, 4, indices, 6);
	}
}

void MapblockMeshGenerator::drawLiquidTop(bool flowing)
{
	// To get backface culling right, the vertices need to go
	// clockwise around the front of the face. And we happened to
	// calculate corner levels in exact reverse order.
	static const int corner_resolve[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

	video::S3DVertex vertices[4] = {
		video::S3DVertex(-BS/2, 0,  BS/2, 0,0,0, color_liquid_top, 0,1),
		video::S3DVertex( BS/2, 0,  BS/2, 0,0,0, color_liquid_top, 1,1),
		video::S3DVertex( BS/2, 0, -BS/2, 0,0,0, color_liquid_top, 1,0),
		video::S3DVertex(-BS/2, 0, -BS/2, 0,0,0, color_liquid_top, 0,0),
	};

	for (int i = 0; i < 4; i++) {
		int u = corner_resolve[i][0];
		int w = corner_resolve[i][1];
		vertices[i].Pos.Y += corner_levels[w][u];
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLight(vertices[i].Pos, tile_liquid_top.color);
		vertices[i].Pos += origin;
	}

	if (flowing) {
		// Default downwards-flowing texture animation goes from
		// -Z towards +Z, thus the direction is +Z.
		// Rotate texture to make animation go in flow direction
		// Positive if liquid moves towards +Z
		f32 dz = (corner_levels[0][0] + corner_levels[0][1]) -
				 (corner_levels[1][0] + corner_levels[1][1]);
		// Positive if liquid moves towards +X
		f32 dx = (corner_levels[0][0] + corner_levels[1][0]) -
				 (corner_levels[0][1] + corner_levels[1][1]);
		f32 tcoord_angle = atan2(dz, dx) * core::RADTODEG;
		v2f tcoord_center(0.5, 0.5);
		v2f tcoord_translate(
				blockpos_nodes.Z + p.Z,
				blockpos_nodes.X + p.X);
		tcoord_translate.rotateBy(tcoord_angle);
		tcoord_translate.X -= floor(tcoord_translate.X);
		tcoord_translate.Y -= floor(tcoord_translate.Y);

		for (int i = 0; i < 4; i++) {
			vertices[i].TCoords.rotateBy(tcoord_angle, tcoord_center);
			vertices[i].TCoords += tcoord_translate;
		}

		std::swap(vertices[0].TCoords, vertices[2].TCoords);
	}

	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	collector->append(tile_liquid_top, vertices, 4, indices, 6);
}

void MapblockMeshGenerator::drawLiquidNode(bool flowing)
{
	prepareLiquidNodeDrawing(flowing);
	getLiquidNeighborhood(flowing);
	if (flowing)
		calculateCornerLevels();
	else
		resetCornerLevels();
	drawLiquidSides(flowing);
	if (!top_is_same_liquid)
		drawLiquidTop(flowing);
}

void MapblockMeshGenerator::drawGlasslikeNode()
{
	TileSpec tile = getNodeTile(n, p, v3s16(0,0,0), data);
	if (!data->m_smooth_lighting)
		color = encode_light_and_color(light, tile.color, f->light_source);

	for (int face = 0; face < 6; face++) {
		// Check this neighbor
		v3s16 dir = g_6dirs[face];
		v3s16 neighbor_pos = blockpos_nodes + p + dir;
		MapNode neighbor = data->m_vmanip.getNodeNoExNoEmerge(neighbor_pos);
		// Don't make face if neighbor is of same type
		if (neighbor.getContent() == n.getContent())
			continue;

		video::SColor face_color = color;
		if (!data->m_smooth_lighting && !f->light_source)
			applyFacesShading(face_color, v3f(dir.X, dir.Y, dir.Z));

		// The face at Z+
		video::S3DVertex vertices[4] = {
			video::S3DVertex(-BS/2, -BS/2, BS/2, dir.X, dir.Y, dir.Z, face_color, 1,1),
			video::S3DVertex( BS/2, -BS/2, BS/2, dir.X, dir.Y, dir.Z, face_color, 0,1),
			video::S3DVertex( BS/2,  BS/2, BS/2, dir.X, dir.Y, dir.Z, face_color, 0,0),
			video::S3DVertex(-BS/2,  BS/2, BS/2, dir.X, dir.Y, dir.Z, face_color, 1,0),
		};

		for (int i = 0; i < 4; i++) {
			// Rotations in the g_6dirs format
			switch(face) {
				case 0: vertices[i].Pos.rotateXZBy(  0); break; // Z+
				case 1: vertices[i].Pos.rotateYZBy(-90); break; // Y+
				case 2: vertices[i].Pos.rotateXZBy(-90); break; // X+
				case 3: vertices[i].Pos.rotateXZBy(180); break; // Z-
				case 4: vertices[i].Pos.rotateYZBy( 90); break; // Y-
				case 5: vertices[i].Pos.rotateXZBy( 90); break; // X-
			};
			if (data->m_smooth_lighting)
				vertices[i].Color = blendLight(vertices[i].Pos, vertices[i].Normal, tile.color);
			vertices[i].Pos += origin;
		}

		static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
		// Add to mesh collector
		collector->append(tile, vertices, 4, indices, 6);
	}
}

void MapblockMeshGenerator::drawGlasslikeFramedNode()
{
	TileSpec tiles[6];
	for (int face = 0; face < 6; face++)
		tiles[face] = getNodeTile(n, p, g_6dirs[face], data);

	TileSpec glass_tiles[6];
	if (tiles[0].texture && tiles[3].texture && tiles[4].texture) {
		glass_tiles[0] = tiles[4];
		glass_tiles[1] = tiles[0];
		glass_tiles[2] = tiles[4];
		glass_tiles[3] = tiles[4];
		glass_tiles[4] = tiles[3];
		glass_tiles[5] = tiles[4];
	} else {
		for (int face = 0; face < 6; face++)
			glass_tiles[face] = tiles[4];
	}

	u8 param2 = n.getParam2();
	bool H_merge = !(param2 & 128);
	bool V_merge = !(param2 & 64);
	param2 &= 63;

	static const float a = BS / 2;
	static const float g = a - 0.003;
	static const float b = .876 * ( BS / 2 );

	static const aabb3f frame_edges[12] = {
		aabb3f( b, b,-a, a, a, a), // y+
		aabb3f(-a, b,-a,-b, a, a), // y+
		aabb3f( b,-a,-a, a,-b, a), // y-
		aabb3f(-a,-a,-a,-b,-b, a), // y-
		aabb3f( b,-a, b, a, a, a), // x+
		aabb3f( b,-a,-a, a, a,-b), // x+
		aabb3f(-a,-a, b,-b, a, a), // x-
		aabb3f(-a,-a,-a,-b, a,-b), // x-
		aabb3f(-a, b, b, a, a, a), // z+
		aabb3f(-a,-a, b, a,-b, a), // z+
		aabb3f(-a,-a,-a, a,-b,-b), // z-
		aabb3f(-a, b,-a, a, a,-b), // z-
	};
	static const aabb3f glass_faces[6] = {
		aabb3f(-g,-g, g, g, g, g), // z+
		aabb3f(-g, g,-g, g, g, g), // y+
		aabb3f( g,-g,-g, g, g, g), // x+
		aabb3f(-g,-g,-g, g, g,-g), // z-
		aabb3f(-g,-g,-g, g,-g, g), // y-
		aabb3f(-g,-g,-g,-g, g, g), // x-
	};

	// tables of neighbour (connect if same type and merge allowed),
	// checked with g_26dirs

	// 1 = connect, 0 = face visible
	bool nb[18] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	// 1 = check
	static const bool check_nb_vertical   [18] = { 0,1,0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0 };
	static const bool check_nb_horizontal [18] = { 1,0,1,1,0,1, 0,0,0,0, 1,1,1,1, 0,0,0,0 };
	static const bool check_nb_all        [18] = { 1,1,1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 };
	const bool *check_nb = check_nb_all;

	// neighbours checks for frames visibility
	if (H_merge || V_merge) {
		if (!H_merge)
			check_nb = check_nb_vertical; // vertical-only merge
		if (!V_merge)
			check_nb = check_nb_horizontal; // horizontal-only merge
		content_t current = n.getContent();
		for (int i = 0; i < 18; i++) {
			if (!check_nb[i])
				continue;
			v3s16 n2p = blockpos_nodes + p + g_26dirs[i];
			MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
			content_t n2c = n2.getContent();
			if (n2c == current || n2c == CONTENT_IGNORE)
				nb[i] = 1;
		}
	}

	// edge visibility

	static const u8 nb_triplet[12*3] = {
		1,2, 7,  1,5, 6,  4,2,15,  4,5,14,
		2,0,11,  2,3,13,  5,0,10,  5,3,12,
		0,1, 8,  0,4,16,  3,4,17,  3,1, 9
	};

	tile = tiles[1];
	for (int edge = 0; edge < 12; edge++) {
		bool edge_invisible;
		if (nb[nb_triplet[edge * 3 + 2]])
			edge_invisible = nb[nb_triplet[edge * 3]] & nb[nb_triplet[edge * 3 + 1]];
		else
			edge_invisible = nb[nb_triplet[edge * 3]] ^ nb[nb_triplet[edge * 3 + 1]];
		if (edge_invisible)
			continue;
		drawAutoLightedCuboid(frame_edges[edge]);
	}

	for (int face = 0; face < 6; face++) {
		if (nb[face])
			continue;
		tile = glass_tiles[face];
		drawAutoLightedCuboid(glass_faces[face]);
	}

	if (param2 > 0 && f->special_tiles[0].texture) {
		// Interior volume level is in range 0 .. 63,
		// convert it to -0.5 .. 0.5
		float vlev = (param2 / 63.0) * 2.0 - 1.0;
		tile = getSpecialTile(*f, n, 0);
		aabb3f box(-(nb[5] ? g : b),
		           -(nb[4] ? g : b),
		           -(nb[3] ? g : b),
		            (nb[2] ? g : b),
		            (nb[1] ? g : b) * vlev,
		            (nb[0] ? g : b));
		drawAutoLightedCuboid(box);
	}
}

void MapblockMeshGenerator::drawAllfacesNode()
{
	static const aabb3f box(-BS/2, -BS/2, -BS/2, BS/2, BS/2, BS/2);
	tile = getNodeTile(n, p, v3s16(0,0,0), data);
	drawAutoLightedCuboid(box);
}

void MapblockMeshGenerator::drawTorchlikeNode()
{
	u8 wall = n.getWallMounted(nodedef);
	u8 tileindex = 0;
	switch(wall) {
		case DWM_YP: tileindex = 1; break; // ceiling
		case DWM_YN: tileindex = 0; break; // floor
		default:     tileindex = 2; // side (or invalid—should we care?)
	}

	TileSpec tile = getNodeTileN(n, p, tileindex, data);
	tile.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
	tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;

	if (!data->m_smooth_lighting)
		color = encode_light_and_color(light, tile.color, f->light_source);

	float size = BS / 2 * f->visual_scale;
	// Wall at X+ of node
	video::S3DVertex vertices[4] = {
		video::S3DVertex(-size,-size,0, 0,0,0, color, 0,1),
		video::S3DVertex( size,-size,0, 0,0,0, color, 1,1),
		video::S3DVertex( size, size,0, 0,0,0, color, 1,0),
		video::S3DVertex(-size, size,0, 0,0,0, color, 0,0),
	};

	for (int i = 0; i < 4; i++) {
		switch(wall) {
			case DWM_YP: vertices[i].Pos.rotateXZBy(-45); break;
			case DWM_YN: vertices[i].Pos.rotateXZBy( 45); break;
			case DWM_XP: vertices[i].Pos.rotateXZBy(  0); break;
			case DWM_XN: vertices[i].Pos.rotateXZBy(180); break;
			case DWM_ZP: vertices[i].Pos.rotateXZBy( 90); break;
			case DWM_ZN: vertices[i].Pos.rotateXZBy(-90); break;
		}
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLight(vertices[i].Pos, tile.color);
		vertices[i].Pos += origin;
	}

	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	// Add to mesh collector
	collector->append(tile, vertices, 4, indices, 6);
}

void MapblockMeshGenerator::drawSignlikeNode()
{
	u8 wall = n.getWallMounted(nodedef);

	TileSpec tile = getNodeTileN(n, p, 0, data);
	tile.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
	tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;

	if (!data->m_smooth_lighting)
		color = encode_light_and_color(light, tile.color, f->light_source);

	float offset = BS/16;
	float size = BS/2 * f->visual_scale;
	// Wall at X+ of node
	video::S3DVertex vertices[4] = {
		video::S3DVertex(BS/2 - offset,  size,  size, 0,0,0, color, 0,0),
		video::S3DVertex(BS/2 - offset,  size, -size, 0,0,0, color, 1,0),
		video::S3DVertex(BS/2 - offset, -size, -size, 0,0,0, color, 1,1),
		video::S3DVertex(BS/2 - offset, -size,  size, 0,0,0, color, 0,1),
	};

	for (int i = 0; i < 4; i++) {
		switch(wall) {
			case DWM_YP: vertices[i].Pos.rotateXYBy( 90); break;
			case DWM_YN: vertices[i].Pos.rotateXYBy(-90); break;
			case DWM_XP: vertices[i].Pos.rotateXZBy(  0); break;
			case DWM_XN: vertices[i].Pos.rotateXZBy(180); break;
			case DWM_ZP: vertices[i].Pos.rotateXZBy( 90); break;
			case DWM_ZN: vertices[i].Pos.rotateXZBy(-90); break;
		}
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLight(vertices[i].Pos, tile.color);
		vertices[i].Pos += origin;
	}

	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	// Add to mesh collector
	collector->append(tile, vertices, 4, indices, 6);
}

void MapblockMeshGenerator::drawPlantlikeQuad(float rotation, float quad_offset,
	bool offset_top_only)
{
	video::S3DVertex vertices[4] = {
		video::S3DVertex(-scale, -BS/2, 0, 0,0,0, color, 0, 1),
		video::S3DVertex( scale, -BS/2, 0, 0,0,0, color, 1, 1),
		video::S3DVertex( scale, -BS/2 + scale * 2, 0, 0,0,0, color, 1, 0),
		video::S3DVertex(-scale, -BS/2 + scale * 2, 0, 0,0,0, color, 0, 0),
	};
	if (random_offset_Y) {
		PseudoRandom yrng(face_num++ | p.X << 16 | p.Z << 8 | p.Y << 24);
		offset.Y = BS * ((yrng.next() % 16 / 16.0) * 0.125);
	}
	int offset_first_index = offset_top_only ? 2 : 0;
	for (int i = 0; i < 4; i++) {
		if (i >= offset_first_index)
			vertices[i].Pos.Z += quad_offset;
		vertices[i].Pos.rotateXZBy(rotation + rotate_degree);
		vertices[i].Pos += offset;
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLight(vertices[i].Pos, tile.color);
		vertices[i].Pos += origin;
	}
	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	collector->append(tile, vertices, 4, indices, 6);
}

void MapblockMeshGenerator::drawPlantlikeNode()
{
	tile = getNodeTileN(n, p, 0, data);
	tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;
	if (!data->m_smooth_lighting)
		color = encode_light_and_color(light, tile.color, f->light_source);

	draw_style = PLANT_STYLE_CROSS;
	scale = BS / 2 * f->visual_scale;
	offset = v3f(0, 0, 0);
	rotate_degree = 0;
	random_offset_Y = false;
	face_num = 0;

	switch (f->param_type_2) {
	case CPT2_MESHOPTIONS:
		draw_style = PlantlikeStyle(n.param2 & MO_MASK_STYLE);
		if (n.param2 & MO_BIT_SCALE_SQRT2)
			scale *= 1.41421;
		if (n.param2 & MO_BIT_RANDOM_OFFSET) {
			PseudoRandom rng(p.X << 8 | p.Z | p.Y << 16);
			offset.X = BS * ((rng.next() % 16 / 16.0) * 0.29 - 0.145);
			offset.Z = BS * ((rng.next() % 16 / 16.0) * 0.29 - 0.145);
		}
		if (n.param2 & MO_BIT_RANDOM_OFFSET_Y)
			random_offset_Y = true;
		break;

	case CPT2_DEGROTATE:
		rotate_degree = n.param2 * 2;
		break;

	default:
		break;
	}

	switch (draw_style) {
	case PLANT_STYLE_CROSS:
		drawPlantlikeQuad(46);
		drawPlantlikeQuad(-44);
		break;

	case PLANT_STYLE_CROSS2:
		drawPlantlikeQuad(91);
		drawPlantlikeQuad(1);
		break;

	case PLANT_STYLE_STAR:
		drawPlantlikeQuad(121);
		drawPlantlikeQuad(241);
		drawPlantlikeQuad(1);
		break;

	case PLANT_STYLE_HASH:
		drawPlantlikeQuad(  1, BS/4);
		drawPlantlikeQuad( 91, BS/4);
		drawPlantlikeQuad(181, BS/4);
		drawPlantlikeQuad(271, BS/4);
		break;

	case PLANT_STYLE_HASH2:
		drawPlantlikeQuad(  1, -BS/2, true);
		drawPlantlikeQuad( 91, -BS/2, true);
		drawPlantlikeQuad(181, -BS/2, true);
		drawPlantlikeQuad(271, -BS/2, true);
		break;
	}
}

void MapblockMeshGenerator::drawFirelikeQuad(float rotation, float opening_angle,
	float offset_h, float offset_v)
{
	video::S3DVertex vertices[4] = {
		video::S3DVertex(-scale, -BS/2, 0, 0,0,0, color, 0, 1),
		video::S3DVertex( scale, -BS/2, 0, 0,0,0, color, 1, 1),
		video::S3DVertex( scale, -BS/2 + scale * 2, 0, 0,0,0, color, 1, 0),
		video::S3DVertex(-scale, -BS/2 + scale * 2, 0, 0,0,0, color, 0, 0),
	};
	for (int i = 0; i < 4; i++) {
		vertices[i].Pos.rotateYZBy(opening_angle);;
		vertices[i].Pos.Z += offset_h;
		vertices[i].Pos.rotateXZBy(rotation);
		vertices[i].Pos.Y += offset_v;
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLight(vertices[i].Pos, tile.color);
		vertices[i].Pos += origin;
	}
	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	collector->append(tile, vertices, 4, indices, 6);
}

void MapblockMeshGenerator::drawFirelikeNode()
{
	tile = getNodeTileN(n, p, 0, data);
	tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;
	if (!data->m_smooth_lighting)
		color = encode_light_and_color(light, tile.color, f->light_source);
	scale = BS / 2 * f->visual_scale;

	// Check for adjacent nodes
	bool neighbors = false;
	bool neighbor[6] = { 0, 0, 0, 0, 0, 0 };
	content_t current = n.getContent();
	for (int i = 0; i < 6; i++) {
		v3s16 n2p = blockpos_nodes + p + g_6dirs[i];
		MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
		content_t n2c = n2.getContent();
		if (n2c != CONTENT_IGNORE && n2c != CONTENT_AIR && n2c != current) {
			neighbor[i] = true;
			neighbors = true;
		}
	}
	bool drawBasicFire = neighbor[D6D_YN] || !neighbors;
	bool drawBottomFire = neighbor[D6D_YP];

	if (drawBasicFire || neighbor[D6D_ZP])
		drawFirelikeQuad(  0, -10, 0.4 * BS);
	else if (drawBottomFire)
		drawFirelikeQuad(  0, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_XN])
		drawFirelikeQuad( 90, -10, 0.4 * BS);
	else if (drawBottomFire)
		drawFirelikeQuad( 90, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_ZN])
		drawFirelikeQuad(180, -10, 0.4 * BS);
	else if (drawBottomFire)
		drawFirelikeQuad(180, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_XP])
		drawFirelikeQuad(270, -10, 0.4 * BS);
	else if (drawBottomFire)
		drawFirelikeQuad(270, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire) {
		drawFirelikeQuad(45, 0, 0.0);
		drawFirelikeQuad(-45, 0, 0.0);
	}
}

void MapblockMeshGenerator::drawFencelikeNode()
{
	tile = getNodeTile(n, p, v3s16(0,0,0), data);
	TileSpec tile_nocrack = tile;
	tile_nocrack.material_flags &= ~MATERIAL_FLAG_CRACK;

	// Put wood the right way around in the posts
	TileSpec tile_rot = tile;
	tile_rot.rotation = 1;

	static const f32 post_rad = BS/8;
	static const f32 bar_rad  = BS/16;
	static const f32 bar_len  = BS/2 - post_rad;

	// The post - always present
	static const aabb3f post(-post_rad, -BS/2, -post_rad,
	                          post_rad,  BS/2,  post_rad);
	static const f32 postuv[24] = {
		0.375, 0.375, 0.625, 0.625,
		0.375, 0.375, 0.625, 0.625,
		0.000, 0.000, 0.250, 1.000,
		0.250, 0.000, 0.500, 1.000,
		0.500, 0.000, 0.750, 1.000,
		0.750, 0.000, 1.000, 1.000,
	};
	tile = tile_rot;
	drawAutoLightedCuboidEx(post, postuv);

	tile = tile_nocrack;

	// Now a section of fence, +X, if there's a post there
	v3s16 p2 = p;
	p2.X++;
	MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
	const ContentFeatures *f2 = &nodedef->get(n2);
	if (f2->drawtype == NDT_FENCELIKE) {
		static const aabb3f bar_x1(BS/2 - bar_len,  BS/4 - bar_rad, -bar_rad,
		                           BS/2 + bar_len,  BS/4 + bar_rad,  bar_rad);
		static const aabb3f bar_x2(BS/2 - bar_len, -BS/4 - bar_rad, -bar_rad,
		                           BS/2 + bar_len, -BS/4 + bar_rad,  bar_rad);
		static const f32 xrailuv[24] = {
			0.000, 0.125, 1.000, 0.250,
			0.000, 0.250, 1.000, 0.375,
			0.375, 0.375, 0.500, 0.500,
			0.625, 0.625, 0.750, 0.750,
			0.000, 0.500, 1.000, 0.625,
			0.000, 0.875, 1.000, 1.000,
		};
		drawAutoLightedCuboidEx(bar_x1, xrailuv);
		drawAutoLightedCuboidEx(bar_x2, xrailuv);
	}

	// Now a section of fence, +Z, if there's a post there
	p2 = p;
	p2.Z++;
	n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
	f2 = &nodedef->get(n2);
	if(f2->drawtype == NDT_FENCELIKE) {
		static const aabb3f bar_z1(-bar_rad,  BS/4 - bar_rad, BS/2 - bar_len,
		                            bar_rad,  BS/4 + bar_rad, BS/2 + bar_len);
		static const aabb3f bar_z2(-bar_rad, -BS/4 - bar_rad, BS/2 - bar_len,
		                            bar_rad, -BS/4 + bar_rad, BS/2 + bar_len);
		static const f32 zrailuv[24] = {
			0.1875, 0.0625, 0.3125, 0.3125, // cannot rotate; stretch
			0.2500, 0.0625, 0.3750, 0.3125, // for wood texture instead
			0.0000, 0.5625, 1.0000, 0.6875,
			0.0000, 0.3750, 1.0000, 0.5000,
			0.3750, 0.3750, 0.5000, 0.5000,
			0.6250, 0.6250, 0.7500, 0.7500,
		};
		drawAutoLightedCuboidEx(bar_z1, zrailuv);
		drawAutoLightedCuboidEx(bar_z2, zrailuv);
	}
}

bool MapblockMeshGenerator::isSameRail(v3s16 dir)
{
	MapNode node2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p + dir);
	if (node2.getContent() == n.getContent())
		return true;
	const ContentFeatures &def2 = nodedef->get(node2);
	return (def2.drawtype == NDT_RAILLIKE) &&
	       (def2.getGroup(raillike_groupname) == raillike_group);
}

void MapblockMeshGenerator::drawRaillikeNode()
{
	static const v3s16 direction[4] = {
		v3s16( 0, 0,  1),
		v3s16( 0, 0, -1),
		v3s16(-1, 0,  0),
		v3s16( 1, 0,  0),
	};
	static const int slope_angle[4] = { 0, 180, 90, -90 };

	enum RailTile {
		straight,
		curved,
		junction,
		cross,
	};
	struct RailDesc {
		int tile_index;
		int angle;
	};
	static const RailDesc rail_kinds[16] = {
		                   // +x -x -z +z
		                   //-------------
		{ straight,   0 }, //  .  .  .  .
		{ straight,   0 }, //  .  .  . +Z
		{ straight,   0 }, //  .  . -Z  .
		{ straight,   0 }, //  .  . -Z +Z
		{ straight,  90 }, //  . -X  .  .
		{   curved, 180 }, //  . -X  . +Z
		{   curved, 270 }, //  . -X -Z  .
		{ junction, 180 }, //  . -X -Z +Z
		{ straight,  90 }, // +X  .  .  .
		{   curved,  90 }, // +X  .  . +Z
		{   curved,   0 }, // +X  . -Z  .
		{ junction,   0 }, // +X  . -Z +Z
		{ straight,  90 }, // +X -X  .  .
		{ junction,  90 }, // +X -X  . +Z
		{ junction, 270 }, // +X -X -Z  .
		{    cross,   0 }, // +X -X -Z +Z
	};

	raillike_group = nodedef->get(n).getGroup(raillike_groupname);

	int code = 0;
	int angle;
	int tile_index;
	bool sloped = false;
	for (int dir = 0; dir < 4; dir++) {
		bool rail_above = isSameRail(direction[dir] + v3s16(0, 1, 0));
		if (rail_above) {
			sloped = true;
			angle = slope_angle[dir];
		}
		if (rail_above ||
				isSameRail(direction[dir]) ||
				isSameRail(direction[dir] + v3s16(0, -1, 0)))
			code |= 1 << dir;
	}

	if (sloped) {
		tile_index = straight;
	} else {
		tile_index = rail_kinds[code].tile_index;
		angle = rail_kinds[code].angle;
	}

	TileSpec tile = getNodeTileN(n, p, tile_index, data);
	tile.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
	tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;

	if (!data->m_smooth_lighting)
		color = encode_light_and_color(light, tile.color, f->light_source);

	float offset = BS/64;
	float size   = BS/2;
	float y2     = sloped ? size : -size;

	video::S3DVertex vertices[4] = {
			video::S3DVertex(-size, -size + offset, -size, 0, 0, 0, color, 0, 1),
			video::S3DVertex( size, -size + offset, -size, 0, 0, 0, color, 1, 1),
			video::S3DVertex( size,    y2 + offset,  size, 0, 0, 0, color, 1, 0),
			video::S3DVertex(-size,    y2 + offset,  size, 0, 0, 0, color, 0, 0),
	};

	for (int i = 0; i < 4; i++) {
		if (angle)
			vertices[i].Pos.rotateXZBy(angle);
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLight(vertices[i].Pos, tile.color);
		vertices[i].Pos += origin;
	}
	static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };
	collector->append(tile, vertices, 4, indices, 6);
}

void MapblockMeshGenerator::drawNodeboxNode()
{
	static const v3s16 tile_dirs[6] = {
		v3s16(0, 1, 0),
		v3s16(0, -1, 0),
		v3s16(1, 0, 0),
		v3s16(-1, 0, 0),
		v3s16(0, 0, 1),
		v3s16(0, 0, -1)
	};

	TileSpec tiles[6];
	for (int j = 0; j < 6; j++) {
		// Handles facedir rotation for textures
		tiles[j] = getNodeTile(n, p, tile_dirs[j], data);
	}

	int neighbors = 0;

	// locate possible neighboring nodes to connect to
	if (f->node_box.type == NODEBOX_CONNECTED) {
		v3s16 p2 = p;

		p2.Y++;
		getNeighborConnectingFace(blockpos_nodes + p2, nodedef, data, n, 1, &neighbors);

		p2 = p;
		p2.Y--;
		getNeighborConnectingFace(blockpos_nodes + p2, nodedef, data, n, 2, &neighbors);

		p2 = p;
		p2.Z--;
		getNeighborConnectingFace(blockpos_nodes + p2, nodedef, data, n, 4, &neighbors);

		p2 = p;
		p2.X--;
		getNeighborConnectingFace(blockpos_nodes + p2, nodedef, data, n, 8, &neighbors);

		p2 = p;
		p2.Z++;
		getNeighborConnectingFace(blockpos_nodes + p2, nodedef, data, n, 16, &neighbors);

		p2 = p;
		p2.X++;
		getNeighborConnectingFace(blockpos_nodes + p2, nodedef, data, n, 32, &neighbors);
	}

	std::vector<aabb3f> boxes;
	n.getNodeBoxes(nodedef, &boxes, neighbors);
	for (std::vector<aabb3f>::iterator
			i = boxes.begin();
			i != boxes.end(); ++i) {
		aabb3f box = *i;

		f32 dx1 = box.MinEdge.X;
		f32 dy1 = box.MinEdge.Y;
		f32 dz1 = box.MinEdge.Z;
		f32 dx2 = box.MaxEdge.X;
		f32 dy2 = box.MaxEdge.Y;
		f32 dz2 = box.MaxEdge.Z;

		box.MinEdge += origin;
		box.MaxEdge += origin;

		if (box.MinEdge.X > box.MaxEdge.X)
			std::swap(box.MinEdge.X, box.MaxEdge.X);
		if (box.MinEdge.Y > box.MaxEdge.Y)
			std::swap(box.MinEdge.Y, box.MaxEdge.Y);
		if (box.MinEdge.Z > box.MaxEdge.Z)
			std::swap(box.MinEdge.Z, box.MaxEdge.Z);

		//
		// Compute texture coords
		f32 tx1 = (box.MinEdge.X/BS)+0.5;
		f32 ty1 = (box.MinEdge.Y/BS)+0.5;
		f32 tz1 = (box.MinEdge.Z/BS)+0.5;
		f32 tx2 = (box.MaxEdge.X/BS)+0.5;
		f32 ty2 = (box.MaxEdge.Y/BS)+0.5;
		f32 tz2 = (box.MaxEdge.Z/BS)+0.5;
		f32 txc[24] = {
			// up
			tx1, 1-tz2, tx2, 1-tz1,
			// down
			tx1, tz1, tx2, tz2,
			// right
			tz1, 1-ty2, tz2, 1-ty1,
			// left
			1-tz2, 1-ty2, 1-tz1, 1-ty1,
			// back
			1-tx2, 1-ty2, 1-tx1, 1-ty1,
			// front
			tx1, 1-ty2, tx2, 1-ty1,
		};
		if (data->m_smooth_lighting) {
			u16 lights[8];
			for (int j = 0; j < 8; ++j) {
				v3f d;
				d.X = (j & 4) ? dx2 : dx1;
				d.Y = (j & 2) ? dy2 : dy1;
				d.Z = (j & 1) ? dz2 : dz1;
				lights[j] = blendLight(d);
			}
			drawCuboid(box, tiles, 6, lights, txc);
		} else {
			drawCuboid(box, tiles, 6, NULL, txc);
		}
	}
}

void MapblockMeshGenerator::drawMeshNode()
{
	u16 l = getInteriorLight(n, 1, nodedef);
	u8 facedir = 0;
	if (f->param_type_2 == CPT2_FACEDIR ||
			f->param_type_2 == CPT2_COLORED_FACEDIR) {
		facedir = n.getFaceDir(nodedef);
	} else if (f->param_type_2 == CPT2_WALLMOUNTED ||
			f->param_type_2 == CPT2_COLORED_WALLMOUNTED) {
		//convert wallmounted to 6dfacedir.
		//when cache enabled, it is already converted
		facedir = n.getWallMounted(nodedef);
		if (!enable_mesh_cache) {
			static const u8 wm_to_6d[6] = {20, 0, 16+1, 12+3, 8, 4+2};
			facedir = wm_to_6d[facedir];
		}
	}

	if (!data->m_smooth_lighting && f->mesh_ptr[facedir]) {
		// use cached meshes
		for (u16 j = 0; j < f->mesh_ptr[0]->getMeshBufferCount(); j++) {
			const TileSpec &tile = getNodeTileN(n, p, j, data);
			scene::IMeshBuffer *buf = f->mesh_ptr[facedir]->getMeshBuffer(j);
			collector->append(tile, (video::S3DVertex *)
				buf->getVertices(), buf->getVertexCount(),
				buf->getIndices(), buf->getIndexCount(), origin,
				encode_light_and_color(l, tile.color, f->light_source),
				f->light_source);
		}
	} else if (f->mesh_ptr[0]) {
		// no cache, clone and rotate mesh
		scene::IMesh* mesh = cloneMesh(f->mesh_ptr[0]);
		rotateMeshBy6dFacedir(mesh, facedir);
		recalculateBoundingBox(mesh);
		meshmanip->recalculateNormals(mesh, true, false);
		for (u16 j = 0; j < mesh->getMeshBufferCount(); j++) {
			const TileSpec &tile = getNodeTileN(n, p, j, data);
			scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
			video::S3DVertex *vertices = (video::S3DVertex *)buf->getVertices();
			u32 vertex_count = buf->getVertexCount();
			if (data->m_smooth_lighting) {
				for (u16 m = 0; m < vertex_count; ++m) {
					video::S3DVertex &vertex = vertices[m];
					vertex.Color = blendLight(vertex.Pos, vertex.Normal, tile.color);
					vertex.Pos += origin;
				}
				collector->append(tile, vertices, vertex_count,
					buf->getIndices(), buf->getIndexCount());
			} else {
				collector->append(tile, vertices, vertex_count,
					buf->getIndices(), buf->getIndexCount(), origin,
					encode_light_and_color(l, tile.color, f->light_source),
					f->light_source);
			}
		}
		mesh->drop();
	}
}

void MapblockMeshGenerator::drawNode()
{
	if (data->m_smooth_lighting) {
		getSmoothLightFrame();
	} else {
		frame.light_source = f->light_source;
		light = getInteriorLight(n, 1, nodedef);
	}
}

/*
	TODO: Fix alpha blending for special nodes
	Currently only the last element rendered is blended correct
*/
void MapblockMeshGenerator::generate()
{
    for (p.Z = 0; p.Z < MAP_BLOCKSIZE; p.Z++)
    for (p.Y = 0; p.Y < MAP_BLOCKSIZE; p.Y++)
    for (p.X = 0; p.X < MAP_BLOCKSIZE; p.X++)
	{
		n = data->m_vmanip.getNodeNoEx(blockpos_nodes + p);
		f = &nodedef->get(n);

		// Only solidness=0 stuff is drawn here
		if (f->solidness != 0)
			continue;

		if (f->drawtype == NDT_AIRLIKE)
			continue;

		origin = intToFloat(p, BS);

		drawNode();

		switch(f->drawtype) {
		default: // pre-converted drawtypes go here too, if appear
			infostream << "Got drawtype " << f->drawtype << std::endl;
			FATAL_ERROR("Unknown drawtype");
			break;
		case NDT_LIQUID:
			drawLiquidNode(false);
			break;
		case NDT_FLOWINGLIQUID:
			drawLiquidNode(true);
			break;
		case NDT_GLASSLIKE:
			drawGlasslikeNode();
			break;
		case NDT_GLASSLIKE_FRAMED:
			drawGlasslikeFramedNode();
			break;
		case NDT_ALLFACES:
			drawAllfacesNode();
			break;
		case NDT_TORCHLIKE:
			drawTorchlikeNode();
			break;
		case NDT_SIGNLIKE:
			drawSignlikeNode();
			break;
		case NDT_PLANTLIKE:
			drawPlantlikeNode();
			break;
		case NDT_FIRELIKE:
			drawFirelikeNode();
			break;
		case NDT_FENCELIKE:
			drawFencelikeNode();
			break;
		case NDT_RAILLIKE:
			drawRaillikeNode();
			break;
		case NDT_NODEBOX:
			drawNodeboxNode();
			break;
		case NDT_MESH:
			drawMeshNode();
			break;
		}
	}
}

void mapblock_mesh_generate_special(MeshMakeData *data,
		MeshCollector &collector)
{
	MapblockMeshGenerator generator(data, &collector);
	generator.generate();
}
