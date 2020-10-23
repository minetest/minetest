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

#include <cmath>
#include "content_mapblock.h"
#include "util/numeric.h"
#include "util/directiontables.h"
#include "mapblock_mesh.h"
#include "settings.h"
#include "nodedef.h"
#include "client/tile.h"
#include "mesh.h"
#include <IMeshManipulator.h>
#include "client/meshgen/collector.h"
#include "client/renderingengine.h"
#include "client.h"
#include "noise.h"

// Distance of light extrapolation (for oversized nodes)
// After this distance, it gives up and considers light level constant
#define SMOOTH_LIGHTING_OVERSIZE 1.0

// Node edge count (for glasslike-framed)
#define FRAMED_EDGE_COUNT 12

// Node neighbor count, including edge-connected, but not vertex-connected
// (for glasslike-framed)
// Corresponding offsets are listed in g_27dirs
#define FRAMED_NEIGHBOR_COUNT 18

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

// Standard index set to make a quad on 4 vertices
static constexpr u16 quad_indices[] = {0, 1, 2, 2, 3, 0};

const std::string MapblockMeshGenerator::raillike_groupname = "connect_to_raillike";

MapblockMeshGenerator::MapblockMeshGenerator(MeshMakeData *input, MeshCollector *output)
{
	data      = input;
	collector = output;

	nodedef   = data->m_client->ndef();
	meshmanip = RenderingEngine::get_scene_manager()->getMeshManipulator();

	enable_mesh_cache = g_settings->getBool("enable_mesh_cache") &&
		!data->m_smooth_lighting; // Mesh cache is not supported with smooth lighting

	blockpos_nodes = data->m_blockpos * MAP_BLOCKSIZE;
}

void MapblockMeshGenerator::useTile(int index, u8 set_flags, u8 reset_flags, bool special)
{
	if (special)
		getSpecialTile(index, &tile, p == data->m_crack_pos_relative);
	else
		getTile(index, &tile);
	if (!data->m_smooth_lighting)
		color = encode_light(light, f->light_source);

	for (auto &layer : tile.layers) {
		layer.material_flags |= set_flags;
		layer.material_flags &= ~reset_flags;
	}
}

// Returns a tile, ready for use, non-rotated.
void MapblockMeshGenerator::getTile(int index, TileSpec *tile)
{
	getNodeTileN(n, p, index, data, *tile);
}

// Returns a tile, ready for use, rotated according to the node facedir.
void MapblockMeshGenerator::getTile(v3s16 direction, TileSpec *tile)
{
	getNodeTile(n, p, direction, data, *tile);
}

// Returns a special tile, ready for use, non-rotated.
void MapblockMeshGenerator::getSpecialTile(int index, TileSpec *tile, bool apply_crack)
{
	*tile = f->special_tiles[index];
	TileLayer *top_layer = nullptr;

	for (auto &layernum : tile->layers) {
		TileLayer *layer = &layernum;
		if (layer->texture_id == 0)
			continue;
		top_layer = layer;
		if (!layer->has_color)
			n.getColor(*f, &layer->color);
	}

	if (apply_crack)
		top_layer->material_flags |= MATERIAL_FLAG_CRACK;
}

void MapblockMeshGenerator::drawQuad(v3f *coords, const v3s16 &normal,
	float vertical_tiling)
{
	const v2f tcoords[4] = {v2f(0.0, 0.0), v2f(1.0, 0.0),
		v2f(1.0, vertical_tiling), v2f(0.0, vertical_tiling)};
	video::S3DVertex vertices[4];
	bool shade_face = !f->light_source && (normal != v3s16(0, 0, 0));
	v3f normal2(normal.X, normal.Y, normal.Z);
	for (int j = 0; j < 4; j++) {
		vertices[j].Pos = coords[j] + origin;
		vertices[j].Normal = normal2;
		if (data->m_smooth_lighting)
			vertices[j].Color = blendLightColor(coords[j]);
		else
			vertices[j].Color = color;
		if (shade_face)
			applyFacesShading(vertices[j].Color, normal2);
		vertices[j].TCoords = tcoords[j];
	}
	collector->append(tile, vertices, 4, quad_indices, 6);
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
	TileSpec *tiles, int tilecount, const LightInfo *lights, const f32 *txc)
{
	assert(tilecount >= 1 && tilecount <= 6); // pre-condition

	v3f min = box.MinEdge;
	v3f max = box.MaxEdge;

	video::SColor colors[6];
	if (!data->m_smooth_lighting) {
		for (int face = 0; face != 6; ++face) {
			colors[face] = encode_light(light, f->light_source);
		}
		if (!f->light_source) {
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
		video::S3DVertex(min.X, max.Y, max.Z, 0, 1, 0, colors[0], txc[0], txc[1]),
		video::S3DVertex(max.X, max.Y, max.Z, 0, 1, 0, colors[0], txc[2], txc[1]),
		video::S3DVertex(max.X, max.Y, min.Z, 0, 1, 0, colors[0], txc[2], txc[3]),
		video::S3DVertex(min.X, max.Y, min.Z, 0, 1, 0, colors[0], txc[0], txc[3]),
		// bottom
		video::S3DVertex(min.X, min.Y, min.Z, 0, -1, 0, colors[1], txc[4], txc[5]),
		video::S3DVertex(max.X, min.Y, min.Z, 0, -1, 0, colors[1], txc[6], txc[5]),
		video::S3DVertex(max.X, min.Y, max.Z, 0, -1, 0, colors[1], txc[6], txc[7]),
		video::S3DVertex(min.X, min.Y, max.Z, 0, -1, 0, colors[1], txc[4], txc[7]),
		// right
		video::S3DVertex(max.X, max.Y, min.Z, 1, 0, 0, colors[2], txc[ 8], txc[9]),
		video::S3DVertex(max.X, max.Y, max.Z, 1, 0, 0, colors[2], txc[10], txc[9]),
		video::S3DVertex(max.X, min.Y, max.Z, 1, 0, 0, colors[2], txc[10], txc[11]),
		video::S3DVertex(max.X, min.Y, min.Z, 1, 0, 0, colors[2], txc[ 8], txc[11]),
		// left
		video::S3DVertex(min.X, max.Y, max.Z, -1, 0, 0, colors[3], txc[12], txc[13]),
		video::S3DVertex(min.X, max.Y, min.Z, -1, 0, 0, colors[3], txc[14], txc[13]),
		video::S3DVertex(min.X, min.Y, min.Z, -1, 0, 0, colors[3], txc[14], txc[15]),
		video::S3DVertex(min.X, min.Y, max.Z, -1, 0, 0, colors[3], txc[12], txc[15]),
		// back
		video::S3DVertex(max.X, max.Y, max.Z, 0, 0, 1, colors[4], txc[16], txc[17]),
		video::S3DVertex(min.X, max.Y, max.Z, 0, 0, 1, colors[4], txc[18], txc[17]),
		video::S3DVertex(min.X, min.Y, max.Z, 0, 0, 1, colors[4], txc[18], txc[19]),
		video::S3DVertex(max.X, min.Y, max.Z, 0, 0, 1, colors[4], txc[16], txc[19]),
		// front
		video::S3DVertex(min.X, max.Y, min.Z, 0, 0, -1, colors[5], txc[20], txc[21]),
		video::S3DVertex(max.X, max.Y, min.Z, 0, 0, -1, colors[5], txc[22], txc[21]),
		video::S3DVertex(max.X, min.Y, min.Z, 0, 0, -1, colors[5], txc[22], txc[23]),
		video::S3DVertex(min.X, min.Y, min.Z, 0, 0, -1, colors[5], txc[20], txc[23]),
	};

	static const u8 light_indices[24] = {
		3, 7, 6, 2,
		0, 4, 5, 1,
		6, 7, 5, 4,
		3, 2, 0, 1,
		7, 3, 1, 5,
		2, 6, 4, 0
	};

	for (int face = 0; face < 6; face++) {
		int tileindex = MYMIN(face, tilecount - 1);
		const TileSpec &tile = tiles[tileindex];
		for (int j = 0; j < 4; j++) {
			video::S3DVertex &vertex = vertices[face * 4 + j];
			v2f &tcoords = vertex.TCoords;
			switch (tile.rotation) {
			case 0:
				break;
			case 1: // R90
				tcoords.rotateBy(90, irr::core::vector2df(0, 0));
				break;
			case 2: // R180
				tcoords.rotateBy(180, irr::core::vector2df(0, 0));
				break;
			case 3: // R270
				tcoords.rotateBy(270, irr::core::vector2df(0, 0));
				break;
			case 4: // FXR90
				tcoords.X = 1.0 - tcoords.X;
				tcoords.rotateBy(90, irr::core::vector2df(0, 0));
				break;
			case 5: // FXR270
				tcoords.X = 1.0 - tcoords.X;
				tcoords.rotateBy(270, irr::core::vector2df(0, 0));
				break;
			case 6: // FYR90
				tcoords.Y = 1.0 - tcoords.Y;
				tcoords.rotateBy(90, irr::core::vector2df(0, 0));
				break;
			case 7: // FYR270
				tcoords.Y = 1.0 - tcoords.Y;
				tcoords.rotateBy(270, irr::core::vector2df(0, 0));
				break;
			case 8: // FX
				tcoords.X = 1.0 - tcoords.X;
				break;
			case 9: // FY
				tcoords.Y = 1.0 - tcoords.Y;
				break;
			default:
				break;
			}
		}
	}

	if (data->m_smooth_lighting) {
		for (int j = 0; j < 24; ++j) {
			video::S3DVertex &vertex = vertices[j];
			vertex.Color = encode_light(
				lights[light_indices[j]].getPair(MYMAX(0.0f, vertex.Normal.Y)),
				f->light_source);
			if (!f->light_source)
				applyFacesShading(vertex.Color, vertex.Normal);
		}
	}

	// Add to mesh collector
	for (int k = 0; k < 6; ++k) {
		int tileindex = MYMIN(k, tilecount - 1);
		collector->append(tiles[tileindex], vertices + 4 * k, 4, quad_indices, 6);
	}
}

// Gets the base lighting values for a node
void MapblockMeshGenerator::getSmoothLightFrame()
{
	for (int k = 0; k < 8; ++k)
		frame.sunlight[k] = false;
	for (int k = 0; k < 8; ++k) {
		LightPair light(getSmoothLightTransparent(blockpos_nodes + p, light_dirs[k], data));
		frame.lightsDay[k] = light.lightDay;
		frame.lightsNight[k] = light.lightNight;
		// If there is direct sunlight and no ambient occlusion at some corner,
		// mark the vertical edge (top and bottom corners) containing it.
		if (light.lightDay == 255) {
			frame.sunlight[k] = true;
			frame.sunlight[k ^ 2] = true;
		}
	}
}

// Calculates vertex light level
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
LightInfo MapblockMeshGenerator::blendLight(const v3f &vertex_pos)
{
	// Light levels at (logical) node corners are known. Here,
	// trilinear interpolation is used to calculate light level
	// at a given point in the node.
	f32 x = core::clamp(vertex_pos.X / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 y = core::clamp(vertex_pos.Y / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 z = core::clamp(vertex_pos.Z / BS + 0.5, 0.0 - SMOOTH_LIGHTING_OVERSIZE, 1.0 + SMOOTH_LIGHTING_OVERSIZE);
	f32 lightDay = 0.0; // daylight
	f32 lightNight = 0.0;
	f32 lightBoosted = 0.0; // daylight + direct sunlight, if any
	for (int k = 0; k < 8; ++k) {
		f32 dx = (k & 4) ? x : 1 - x;
		f32 dy = (k & 2) ? y : 1 - y;
		f32 dz = (k & 1) ? z : 1 - z;
		// Use direct sunlight (255), if any; use daylight otherwise.
		f32 light_boosted = frame.sunlight[k] ? 255 : frame.lightsDay[k];
		lightDay += dx * dy * dz * frame.lightsDay[k];
		lightNight += dx * dy * dz * frame.lightsNight[k];
		lightBoosted += dx * dy * dz * light_boosted;
	}
	return LightInfo{lightDay, lightNight, lightBoosted};
}

// Calculates vertex color to be used in mapblock mesh
//  vertex_pos - vertex position in the node (coordinates are clamped to [0.0, 1.0] or so)
//  tile_color - node's tile color
video::SColor MapblockMeshGenerator::blendLightColor(const v3f &vertex_pos)
{
	LightInfo light = blendLight(vertex_pos);
	return encode_light(light.getPair(), f->light_source);
}

video::SColor MapblockMeshGenerator::blendLightColor(const v3f &vertex_pos,
	const v3f &vertex_normal)
{
	LightInfo light = blendLight(vertex_pos);
	video::SColor color = encode_light(light.getPair(MYMAX(0.0f, vertex_normal.Y)), f->light_source);
	if (!f->light_source)
		applyFacesShading(color, vertex_normal);
	return color;
}

void MapblockMeshGenerator::generateCuboidTextureCoords(const aabb3f &box, f32 *coords)
{
	f32 tx1 = (box.MinEdge.X / BS) + 0.5;
	f32 ty1 = (box.MinEdge.Y / BS) + 0.5;
	f32 tz1 = (box.MinEdge.Z / BS) + 0.5;
	f32 tx2 = (box.MaxEdge.X / BS) + 0.5;
	f32 ty2 = (box.MaxEdge.Y / BS) + 0.5;
	f32 tz2 = (box.MaxEdge.Z / BS) + 0.5;
	f32 txc[24] = {
		    tx1, 1 - tz2,     tx2, 1 - tz1, // up
		    tx1,     tz1,     tx2,     tz2, // down
		    tz1, 1 - ty2,     tz2, 1 - ty1, // right
		1 - tz2, 1 - ty2, 1 - tz1, 1 - ty1, // left
		1 - tx2, 1 - ty2, 1 - tx1, 1 - ty1, // back
		    tx1, 1 - ty2,     tx2, 1 - ty1, // front
	};
	for (int i = 0; i != 24; ++i)
		coords[i] = txc[i];
}

void MapblockMeshGenerator::drawAutoLightedCuboid(aabb3f box, const f32 *txc,
	TileSpec *tiles, int tile_count)
{
	bool scale = std::fabs(f->visual_scale - 1.0f) > 1e-3f;
	f32 texture_coord_buf[24];
	f32 dx1 = box.MinEdge.X;
	f32 dy1 = box.MinEdge.Y;
	f32 dz1 = box.MinEdge.Z;
	f32 dx2 = box.MaxEdge.X;
	f32 dy2 = box.MaxEdge.Y;
	f32 dz2 = box.MaxEdge.Z;
	if (scale) {
		if (!txc) { // generate texture coords before scaling
			generateCuboidTextureCoords(box, texture_coord_buf);
			txc = texture_coord_buf;
		}
		box.MinEdge *= f->visual_scale;
		box.MaxEdge *= f->visual_scale;
	}
	box.MinEdge += origin;
	box.MaxEdge += origin;
	if (!txc) {
		generateCuboidTextureCoords(box, texture_coord_buf);
		txc = texture_coord_buf;
	}
	if (!tiles) {
		tiles = &tile;
		tile_count = 1;
	}
	if (data->m_smooth_lighting) {
		LightInfo lights[8];
		for (int j = 0; j < 8; ++j) {
			v3f d;
			d.X = (j & 4) ? dx2 : dx1;
			d.Y = (j & 2) ? dy2 : dy1;
			d.Z = (j & 1) ? dz2 : dz1;
			lights[j] = blendLight(d);
		}
		drawCuboid(box, tiles, tile_count, lights, txc);
	} else {
		drawCuboid(box, tiles, tile_count, nullptr, txc);
	}
}

void MapblockMeshGenerator::prepareLiquidNodeDrawing()
{
	getSpecialTile(0, &tile_liquid_top);
	getSpecialTile(1, &tile_liquid);

	MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(p.X, p.Y + 1, p.Z));
	MapNode nbottom = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(p.X, p.Y - 1, p.Z));
	c_flowing = f->liquid_alternative_flowing_id;
	c_source = f->liquid_alternative_source_id;
	top_is_same_liquid = (ntop.getContent() == c_flowing) || (ntop.getContent() == c_source);
	draw_liquid_bottom = (nbottom.getContent() != c_flowing) && (nbottom.getContent() != c_source);
	if (draw_liquid_bottom) {
		const ContentFeatures &f2 = nodedef->get(nbottom.getContent());
		if (f2.solidness > 1)
			draw_liquid_bottom = false;
	}

	if (data->m_smooth_lighting)
		return; // don't need to pre-compute anything in this case

	if (f->light_source != 0) {
		// If this liquid emits light and doesn't contain light, draw
		// it at what it emits, for an increased effect
		u8 e = decode_light(f->light_source);
		light = LightPair(std::max(e, light.lightDay), std::max(e, light.lightNight));
	} else if (nodedef->get(ntop).param_type == CPT_LIGHT) {
		// Otherwise, use the light of the node on top if possible
		light = LightPair(getInteriorLight(ntop, 0, nodedef));
	}

	color_liquid_top = encode_light(light, f->light_source);
	color = encode_light(light, f->light_source);
}

void MapblockMeshGenerator::getLiquidNeighborhood()
{
	u8 range = rangelim(nodedef->get(c_flowing).liquid_range, 1, 8);

	for (int w = -1; w <= 1; w++)
	for (int u = -1; u <= 1; u++) {
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
		p2.Y++;
		n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
		if (n2.getContent() == c_source || n2.getContent() == c_flowing)
			neighbor.top_is_same_liquid = true;
	}
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
	for (int di = 0; di < 2; di++) {
		NeighborData &neighbor_data = liquid_neighbors[k + dk][i + di];
		content_t content = neighbor_data.content;

		// If top is liquid, draw starting from top of node
		if (neighbor_data.top_is_same_liquid)
			return 0.5 * BS;

		// Source always has the full height
		if (content == c_source)
			return 0.5 * BS;

		// Flowing liquid has level information
		if (content == c_flowing) {
			sum += neighbor_data.level;
			count++;
		} else if (content == CONTENT_AIR) {
			air_count++;
			if (air_count >= 2)
				return -0.5 * BS + 0.2;
		}
	}
	if (count > 0)
		return sum / count;
	return 0;
}

namespace {
	struct LiquidFaceDesc {
		v3s16 dir; // XZ
		v3s16 p[2]; // XZ only; 1 means +, 0 means -
	};
	struct UV {
		int u, v;
	};
	static const LiquidFaceDesc liquid_base_faces[4] = {
		{v3s16( 1, 0,  0), {v3s16(1, 0, 1), v3s16(1, 0, 0)}},
		{v3s16(-1, 0,  0), {v3s16(0, 0, 0), v3s16(0, 0, 1)}},
		{v3s16( 0, 0,  1), {v3s16(0, 0, 1), v3s16(1, 0, 1)}},
		{v3s16( 0, 0, -1), {v3s16(1, 0, 0), v3s16(0, 0, 0)}},
	};
	static const UV liquid_base_vertices[4] = {
		{0, 1},
		{1, 1},
		{1, 0},
		{0, 0}
	};
}

void MapblockMeshGenerator::drawLiquidSides()
{
	for (const auto &face : liquid_base_faces) {
		const NeighborData &neighbor = liquid_neighbors[face.dir.Z + 1][face.dir.X + 1];

		// No face between nodes of the same liquid, unless there is node
		// at the top to which it should be connected. Again, unless the face
		// there would be inside the liquid
		if (neighbor.is_same_liquid) {
			if (!top_is_same_liquid)
				continue;
			if (neighbor.top_is_same_liquid)
				continue;
		}

		const ContentFeatures &neighbor_features = nodedef->get(neighbor.content);
		// Don't draw face if neighbor is blocking the view
		if (neighbor_features.solidness == 2)
			continue;

		video::S3DVertex vertices[4];
		for (int j = 0; j < 4; j++) {
			const UV &vertex = liquid_base_vertices[j];
			const v3s16 &base = face.p[vertex.u];
			float v = vertex.v;

			v3f pos;
			pos.X = (base.X - 0.5f) * BS;
			pos.Z = (base.Z - 0.5f) * BS;
			if (vertex.v) {
				pos.Y = neighbor.is_same_liquid ? corner_levels[base.Z][base.X] : -0.5f * BS;
			} else if (top_is_same_liquid) {
				pos.Y = 0.5f * BS;
			} else {
				pos.Y = corner_levels[base.Z][base.X];
				v += (0.5f * BS - corner_levels[base.Z][base.X]) / BS;
			}

			if (data->m_smooth_lighting)
				color = blendLightColor(pos);
			pos += origin;
			vertices[j] = video::S3DVertex(pos.X, pos.Y, pos.Z, 0, 0, 0, color, vertex.u, v);
		};
		collector->append(tile_liquid, vertices, 4, quad_indices, 6);
	}
}

void MapblockMeshGenerator::drawLiquidTop()
{
	// To get backface culling right, the vertices need to go
	// clockwise around the front of the face. And we happened to
	// calculate corner levels in exact reverse order.
	static const int corner_resolve[4][2] = {{0, 1}, {1, 1}, {1, 0}, {0, 0}};

	video::S3DVertex vertices[4] = {
		video::S3DVertex(-BS / 2, 0,  BS / 2, 0, 0, 0, color_liquid_top, 0, 1),
		video::S3DVertex( BS / 2, 0,  BS / 2, 0, 0, 0, color_liquid_top, 1, 1),
		video::S3DVertex( BS / 2, 0, -BS / 2, 0, 0, 0, color_liquid_top, 1, 0),
		video::S3DVertex(-BS / 2, 0, -BS / 2, 0, 0, 0, color_liquid_top, 0, 0),
	};

	for (int i = 0; i < 4; i++) {
		int u = corner_resolve[i][0];
		int w = corner_resolve[i][1];
		vertices[i].Pos.Y += corner_levels[w][u];
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLightColor(vertices[i].Pos);
		vertices[i].Pos += origin;
	}

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
	v2f tcoord_translate(blockpos_nodes.Z + p.Z, blockpos_nodes.X + p.X);
	tcoord_translate.rotateBy(tcoord_angle);
	tcoord_translate.X -= floor(tcoord_translate.X);
	tcoord_translate.Y -= floor(tcoord_translate.Y);

	for (video::S3DVertex &vertex : vertices) {
		vertex.TCoords.rotateBy(tcoord_angle, tcoord_center);
		vertex.TCoords += tcoord_translate;
	}

	std::swap(vertices[0].TCoords, vertices[2].TCoords);

	collector->append(tile_liquid_top, vertices, 4, quad_indices, 6);
}

void MapblockMeshGenerator::drawLiquidBottom()
{
	video::S3DVertex vertices[4] = {
		video::S3DVertex(-BS / 2, -BS / 2, -BS / 2, 0, 0, 0, color_liquid_top, 0, 0),
		video::S3DVertex( BS / 2, -BS / 2, -BS / 2, 0, 0, 0, color_liquid_top, 1, 0),
		video::S3DVertex( BS / 2, -BS / 2,  BS / 2, 0, 0, 0, color_liquid_top, 1, 1),
		video::S3DVertex(-BS / 2, -BS / 2,  BS / 2, 0, 0, 0, color_liquid_top, 0, 1),
	};

	for (int i = 0; i < 4; i++) {
		if (data->m_smooth_lighting)
			vertices[i].Color = blendLightColor(vertices[i].Pos);
		vertices[i].Pos += origin;
	}

	collector->append(tile_liquid_top, vertices, 4, quad_indices, 6);
}

void MapblockMeshGenerator::drawLiquidNode()
{
	prepareLiquidNodeDrawing();
	getLiquidNeighborhood();
	calculateCornerLevels();
	drawLiquidSides();
	if (!top_is_same_liquid)
		drawLiquidTop();
	if (draw_liquid_bottom)
		drawLiquidBottom();
}

void MapblockMeshGenerator::drawGlasslikeNode()
{
	useTile(0, 0, 0);

	for (int face = 0; face < 6; face++) {
		// Check this neighbor
		v3s16 dir = g_6dirs[face];
		v3s16 neighbor_pos = blockpos_nodes + p + dir;
		MapNode neighbor = data->m_vmanip.getNodeNoExNoEmerge(neighbor_pos);
		// Don't make face if neighbor is of same type
		if (neighbor.getContent() == n.getContent())
			continue;
		// Face at Z-
		v3f vertices[4] = {
			v3f(-BS / 2,  BS / 2, -BS / 2),
			v3f( BS / 2,  BS / 2, -BS / 2),
			v3f( BS / 2, -BS / 2, -BS / 2),
			v3f(-BS / 2, -BS / 2, -BS / 2),
		};

		for (v3f &vertex : vertices) {
			switch (face) {
				case D6D_ZP:
					vertex.rotateXZBy(180); break;
				case D6D_YP:
					vertex.rotateYZBy( 90); break;
				case D6D_XP:
					vertex.rotateXZBy( 90); break;
				case D6D_ZN:
					vertex.rotateXZBy(  0); break;
				case D6D_YN:
					vertex.rotateYZBy(-90); break;
				case D6D_XN:
					vertex.rotateXZBy(-90); break;
			}
		}
		drawQuad(vertices, dir);
	}
}

void MapblockMeshGenerator::drawGlasslikeFramedNode()
{
	TileSpec tiles[6];
	for (int face = 0; face < 6; face++)
		getTile(g_6dirs[face], &tiles[face]);

	if (!data->m_smooth_lighting)
		color = encode_light(light, f->light_source);

	TileSpec glass_tiles[6];
	for (auto &glass_tile : glass_tiles)
		glass_tile = tiles[4];

	// Only respect H/V merge bits when paramtype2 = "glasslikeliquidlevel" (liquid tank)
	u8 param2 = (f->param_type_2 == CPT2_GLASSLIKE_LIQUID_LEVEL) ? n.getParam2() : 0;
	bool H_merge = !(param2 & 128);
	bool V_merge = !(param2 & 64);
	param2 &= 63;

	static const float a = BS / 2.0f;
	static const float g = a - 0.03f;
	static const float b = 0.876f * (BS / 2.0f);

	static const aabb3f frame_edges[FRAMED_EDGE_COUNT] = {
		aabb3f( b,  b, -a,  a,  a,  a), // y+
		aabb3f(-a,  b, -a, -b,  a,  a), // y+
		aabb3f( b, -a, -a,  a, -b,  a), // y-
		aabb3f(-a, -a, -a, -b, -b,  a), // y-
		aabb3f( b, -a,  b,  a,  a,  a), // x+
		aabb3f( b, -a, -a,  a,  a, -b), // x+
		aabb3f(-a, -a,  b, -b,  a,  a), // x-
		aabb3f(-a, -a, -a, -b,  a, -b), // x-
		aabb3f(-a,  b,  b,  a,  a,  a), // z+
		aabb3f(-a, -a,  b,  a, -b,  a), // z+
		aabb3f(-a, -a, -a,  a, -b, -b), // z-
		aabb3f(-a,  b, -a,  a,  a, -b), // z-
	};

	// tables of neighbour (connect if same type and merge allowed),
	// checked with g_26dirs

	// 1 = connect, 0 = face visible
	bool nb[FRAMED_NEIGHBOR_COUNT] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	// 1 = check
	static const bool check_nb_vertical [FRAMED_NEIGHBOR_COUNT] =
		{0,1,0,0,1,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	static const bool check_nb_horizontal [FRAMED_NEIGHBOR_COUNT] =
		{1,0,1,1,0,1, 0,0,0,0, 1,1,1,1, 0,0,0,0};
	static const bool check_nb_all [FRAMED_NEIGHBOR_COUNT] =
		{1,1,1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1};
	const bool *check_nb = check_nb_all;

	// neighbours checks for frames visibility
	if (H_merge || V_merge) {
		if (!H_merge)
			check_nb = check_nb_vertical; // vertical-only merge
		if (!V_merge)
			check_nb = check_nb_horizontal; // horizontal-only merge
		content_t current = n.getContent();
		for (int i = 0; i < FRAMED_NEIGHBOR_COUNT; i++) {
			if (!check_nb[i])
				continue;
			v3s16 n2p = blockpos_nodes + p + g_26dirs[i];
			MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
			content_t n2c = n2.getContent();
			if (n2c == current)
				nb[i] = 1;
		}
	}

	// edge visibility

	static const u8 nb_triplet[FRAMED_EDGE_COUNT][3] = {
		{1, 2,  7}, {1, 5,  6}, {4, 2, 15}, {4, 5, 14},
		{2, 0, 11}, {2, 3, 13}, {5, 0, 10}, {5, 3, 12},
		{0, 1,  8}, {0, 4, 16}, {3, 4, 17}, {3, 1,  9},
	};

	tile = tiles[1];
	for (int edge = 0; edge < FRAMED_EDGE_COUNT; edge++) {
		bool edge_invisible;
		if (nb[nb_triplet[edge][2]])
			edge_invisible = nb[nb_triplet[edge][0]] & nb[nb_triplet[edge][1]];
		else
			edge_invisible = nb[nb_triplet[edge][0]] ^ nb[nb_triplet[edge][1]];
		if (edge_invisible)
			continue;
		drawAutoLightedCuboid(frame_edges[edge]);
	}

	for (int face = 0; face < 6; face++) {
		if (nb[face])
			continue;

		tile = glass_tiles[face];
		// Face at Z-
		v3f vertices[4] = {
			v3f(-a,  a, -g),
			v3f( a,  a, -g),
			v3f( a, -a, -g),
			v3f(-a, -a, -g),
		};

		for (v3f &vertex : vertices) {
			switch (face) {
				case D6D_ZP:
					vertex.rotateXZBy(180); break;
				case D6D_YP:
					vertex.rotateYZBy( 90); break;
				case D6D_XP:
					vertex.rotateXZBy( 90); break;
				case D6D_ZN:
					vertex.rotateXZBy(  0); break;
				case D6D_YN:
					vertex.rotateYZBy(-90); break;
				case D6D_XN:
					vertex.rotateXZBy(-90); break;
			}
		}
		v3s16 dir = g_6dirs[face];
		drawQuad(vertices, dir);
	}

	// Optionally render internal liquid level defined by param2
	// Liquid is textured with 1 tile defined in nodedef 'special_tiles'
	if (param2 > 0 && f->param_type_2 == CPT2_GLASSLIKE_LIQUID_LEVEL &&
			f->special_tiles[0].layers[0].texture) {
		// Internal liquid level has param2 range 0 .. 63,
		// convert it to -0.5 .. 0.5
		float vlev = (param2 / 63.0f) * 2.0f - 1.0f;
		getSpecialTile(0, &tile);
		drawAutoLightedCuboid(aabb3f(-(nb[5] ? g : b),
		                             -(nb[4] ? g : b),
		                             -(nb[3] ? g : b),
		                              (nb[2] ? g : b),
		                              (nb[1] ? g : b) * vlev,
		                              (nb[0] ? g : b)));
	}
}

void MapblockMeshGenerator::drawAllfacesNode()
{
	static const aabb3f box(-BS / 2, -BS / 2, -BS / 2, BS / 2, BS / 2, BS / 2);
	useTile(0, 0, 0);
	drawAutoLightedCuboid(box);
}

void MapblockMeshGenerator::drawTorchlikeNode()
{
	u8 wall = n.getWallMounted(nodedef);
	u8 tileindex = 0;
	switch (wall) {
		case DWM_YP: tileindex = 1; break; // ceiling
		case DWM_YN: tileindex = 0; break; // floor
		default:     tileindex = 2; // side (or invalid—should we care?)
	}
	useTile(tileindex, MATERIAL_FLAG_CRACK_OVERLAY, MATERIAL_FLAG_BACKFACE_CULLING);

	float size = BS / 2 * f->visual_scale;
	v3f vertices[4] = {
		v3f(-size,  size, 0),
		v3f( size,  size, 0),
		v3f( size, -size, 0),
		v3f(-size, -size, 0),
	};

	for (v3f &vertex : vertices) {
		switch (wall) {
			case DWM_YP:
				vertex.Y += -size + BS/2;
				vertex.rotateXZBy(-45);
				break;
			case DWM_YN:
				vertex.Y += size - BS/2;
				vertex.rotateXZBy(45);
				break;
			case DWM_XP:
				vertex.X += -size + BS/2;
				break;
			case DWM_XN:
				vertex.X += -size + BS/2;
				vertex.rotateXZBy(180);
				break;
			case DWM_ZP:
				vertex.X += -size + BS/2;
				vertex.rotateXZBy(90);
				break;
			case DWM_ZN:
				vertex.X += -size + BS/2;
				vertex.rotateXZBy(-90);
		}
	}
	drawQuad(vertices);
}

void MapblockMeshGenerator::drawSignlikeNode()
{
	u8 wall = n.getWallMounted(nodedef);
	useTile(0, MATERIAL_FLAG_CRACK_OVERLAY, MATERIAL_FLAG_BACKFACE_CULLING);
	static const float offset = BS / 16;
	float size = BS / 2 * f->visual_scale;
	// Wall at X+ of node
	v3f vertices[4] = {
		v3f(BS / 2 - offset,  size,  size),
		v3f(BS / 2 - offset,  size, -size),
		v3f(BS / 2 - offset, -size, -size),
		v3f(BS / 2 - offset, -size,  size),
	};

	for (v3f &vertex : vertices) {
		switch (wall) {
			case DWM_YP:
				vertex.rotateXYBy( 90); break;
			case DWM_YN:
				vertex.rotateXYBy(-90); break;
			case DWM_XP:
				vertex.rotateXZBy(  0); break;
			case DWM_XN:
				vertex.rotateXZBy(180); break;
			case DWM_ZP:
				vertex.rotateXZBy( 90); break;
			case DWM_ZN:
				vertex.rotateXZBy(-90); break;
		}
	}
	drawQuad(vertices);
}

void MapblockMeshGenerator::drawPlantlikeQuad(float rotation, float quad_offset,
	bool offset_top_only)
{
	v3f vertices[4] = {
		v3f(-scale, -BS / 2 + 2.0 * scale * plant_height, 0),
		v3f( scale, -BS / 2 + 2.0 * scale * plant_height, 0),
		v3f( scale, -BS / 2, 0),
		v3f(-scale, -BS / 2, 0),
	};
	if (random_offset_Y) {
		PseudoRandom yrng(face_num++ | p.X << 16 | p.Z << 8 | p.Y << 24);
		offset.Y = -BS * ((yrng.next() % 16 / 16.0) * 0.125);
	}
	int offset_count = offset_top_only ? 2 : 4;
	for (int i = 0; i < offset_count; i++)
		vertices[i].Z += quad_offset;

	for (v3f &vertex : vertices) {
		vertex.rotateXZBy(rotation + rotate_degree);
		vertex += offset;
	}
	drawQuad(vertices, v3s16(0, 0, 0), plant_height);
}

void MapblockMeshGenerator::drawPlantlike()
{
	draw_style = PLANT_STYLE_CROSS;
	scale = BS / 2 * f->visual_scale;
	offset = v3f(0, 0, 0);
	rotate_degree = 0;
	random_offset_Y = false;
	face_num = 0;
	plant_height = 1.0;

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

	case CPT2_LEVELED:
		plant_height = n.param2 / 16.0;
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
		drawPlantlikeQuad(  1, BS / 4);
		drawPlantlikeQuad( 91, BS / 4);
		drawPlantlikeQuad(181, BS / 4);
		drawPlantlikeQuad(271, BS / 4);
		break;

	case PLANT_STYLE_HASH2:
		drawPlantlikeQuad(  1, -BS / 2, true);
		drawPlantlikeQuad( 91, -BS / 2, true);
		drawPlantlikeQuad(181, -BS / 2, true);
		drawPlantlikeQuad(271, -BS / 2, true);
		break;
	}
}

void MapblockMeshGenerator::drawPlantlikeNode()
{
	useTile();
	drawPlantlike();
}

void MapblockMeshGenerator::drawPlantlikeRootedNode()
{
	useTile(0, MATERIAL_FLAG_CRACK_OVERLAY, 0, true);
	origin += v3f(0.0, BS, 0.0);
	p.Y++;
	if (data->m_smooth_lighting) {
		getSmoothLightFrame();
	} else {
		MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + p);
		light = LightPair(getInteriorLight(ntop, 1, nodedef));
	}
	drawPlantlike();
	p.Y--;
}

void MapblockMeshGenerator::drawFirelikeQuad(float rotation, float opening_angle,
	float offset_h, float offset_v)
{
	v3f vertices[4] = {
		v3f(-scale, -BS / 2 + scale * 2, 0),
		v3f( scale, -BS / 2 + scale * 2, 0),
		v3f( scale, -BS / 2, 0),
		v3f(-scale, -BS / 2, 0),
	};

	for (v3f &vertex : vertices) {
		vertex.rotateYZBy(opening_angle);
		vertex.Z += offset_h;
		vertex.rotateXZBy(rotation);
		vertex.Y += offset_v;
	}
	drawQuad(vertices);
}

void MapblockMeshGenerator::drawFirelikeNode()
{
	useTile();
	scale = BS / 2 * f->visual_scale;

	// Check for adjacent nodes
	bool neighbors = false;
	bool neighbor[6] = {0, 0, 0, 0, 0, 0};
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
		drawFirelikeQuad(0, -10, 0.4 * BS);
	else if (drawBottomFire)
		drawFirelikeQuad(0, 70, 0.47 * BS, 0.484 * BS);

	if (drawBasicFire || neighbor[D6D_XN])
		drawFirelikeQuad(90, -10, 0.4 * BS);
	else if (drawBottomFire)
		drawFirelikeQuad(90, 70, 0.47 * BS, 0.484 * BS);

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
	useTile(0, 0, 0);
	TileSpec tile_nocrack = tile;

	for (auto &layer : tile_nocrack.layers)
		layer.material_flags &= ~MATERIAL_FLAG_CRACK;

	// Put wood the right way around in the posts
	TileSpec tile_rot = tile;
	tile_rot.rotation = 1;

	static const f32 post_rad = BS / 8;
	static const f32 bar_rad  = BS / 16;
	static const f32 bar_len  = BS / 2 - post_rad;

	// The post - always present
	static const aabb3f post(-post_rad, -BS / 2, -post_rad,
	                          post_rad,  BS / 2,  post_rad);
	static const f32 postuv[24] = {
		0.375, 0.375, 0.625, 0.625,
		0.375, 0.375, 0.625, 0.625,
		0.000, 0.000, 0.250, 1.000,
		0.250, 0.000, 0.500, 1.000,
		0.500, 0.000, 0.750, 1.000,
		0.750, 0.000, 1.000, 1.000,
	};
	tile = tile_rot;
	drawAutoLightedCuboid(post, postuv);

	tile = tile_nocrack;

	// Now a section of fence, +X, if there's a post there
	v3s16 p2 = p;
	p2.X++;
	MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
	const ContentFeatures *f2 = &nodedef->get(n2);
	if (f2->drawtype == NDT_FENCELIKE) {
		static const aabb3f bar_x1(BS / 2 - bar_len,  BS / 4 - bar_rad, -bar_rad,
		                           BS / 2 + bar_len,  BS / 4 + bar_rad,  bar_rad);
		static const aabb3f bar_x2(BS / 2 - bar_len, -BS / 4 - bar_rad, -bar_rad,
		                           BS / 2 + bar_len, -BS / 4 + bar_rad,  bar_rad);
		static const f32 xrailuv[24] = {
			0.000, 0.125, 1.000, 0.250,
			0.000, 0.250, 1.000, 0.375,
			0.375, 0.375, 0.500, 0.500,
			0.625, 0.625, 0.750, 0.750,
			0.000, 0.500, 1.000, 0.625,
			0.000, 0.875, 1.000, 1.000,
		};
		drawAutoLightedCuboid(bar_x1, xrailuv);
		drawAutoLightedCuboid(bar_x2, xrailuv);
	}

	// Now a section of fence, +Z, if there's a post there
	p2 = p;
	p2.Z++;
	n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
	f2 = &nodedef->get(n2);
	if (f2->drawtype == NDT_FENCELIKE) {
		static const aabb3f bar_z1(-bar_rad,  BS / 4 - bar_rad, BS / 2 - bar_len,
		                            bar_rad,  BS / 4 + bar_rad, BS / 2 + bar_len);
		static const aabb3f bar_z2(-bar_rad, -BS / 4 - bar_rad, BS / 2 - bar_len,
		                            bar_rad, -BS / 4 + bar_rad, BS / 2 + bar_len);
		static const f32 zrailuv[24] = {
			0.1875, 0.0625, 0.3125, 0.3125, // cannot rotate; stretch
			0.2500, 0.0625, 0.3750, 0.3125, // for wood texture instead
			0.0000, 0.5625, 1.0000, 0.6875,
			0.0000, 0.3750, 1.0000, 0.5000,
			0.3750, 0.3750, 0.5000, 0.5000,
			0.6250, 0.6250, 0.7500, 0.7500,
		};
		drawAutoLightedCuboid(bar_z1, zrailuv);
		drawAutoLightedCuboid(bar_z2, zrailuv);
	}
}

bool MapblockMeshGenerator::isSameRail(v3s16 dir)
{
	MapNode node2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p + dir);
	if (node2.getContent() == n.getContent())
		return true;
	const ContentFeatures &def2 = nodedef->get(node2);
	return ((def2.drawtype == NDT_RAILLIKE) &&
		(def2.getGroup(raillike_groupname) == raillike_group));
}

namespace {
	static const v3s16 rail_direction[4] = {
		v3s16( 0, 0,  1),
		v3s16( 0, 0, -1),
		v3s16(-1, 0,  0),
		v3s16( 1, 0,  0),
	};
	static const int rail_slope_angle[4] = {0, 180, 90, -90};

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
		{straight,   0}, //  .  .  .  .
		{straight,   0}, //  .  .  . +Z
		{straight,   0}, //  .  . -Z  .
		{straight,   0}, //  .  . -Z +Z
		{straight,  90}, //  . -X  .  .
		{  curved, 180}, //  . -X  . +Z
		{  curved, 270}, //  . -X -Z  .
		{junction, 180}, //  . -X -Z +Z
		{straight,  90}, // +X  .  .  .
		{  curved,  90}, // +X  .  . +Z
		{  curved,   0}, // +X  . -Z  .
		{junction,   0}, // +X  . -Z +Z
		{straight,  90}, // +X -X  .  .
		{junction,  90}, // +X -X  . +Z
		{junction, 270}, // +X -X -Z  .
		{   cross,   0}, // +X -X -Z +Z
	};
}

void MapblockMeshGenerator::drawRaillikeNode()
{
	raillike_group = nodedef->get(n).getGroup(raillike_groupname);

	int code = 0;
	int angle;
	int tile_index;
	bool sloped = false;
	for (int dir = 0; dir < 4; dir++) {
		bool rail_above = isSameRail(rail_direction[dir] + v3s16(0, 1, 0));
		if (rail_above) {
			sloped = true;
			angle = rail_slope_angle[dir];
		}
		if (rail_above ||
				isSameRail(rail_direction[dir]) ||
				isSameRail(rail_direction[dir] + v3s16(0, -1, 0)))
			code |= 1 << dir;
	}

	if (sloped) {
		tile_index = straight;
	} else {
		tile_index = rail_kinds[code].tile_index;
		angle = rail_kinds[code].angle;
	}

	useTile(tile_index, MATERIAL_FLAG_CRACK_OVERLAY, MATERIAL_FLAG_BACKFACE_CULLING);

	static const float offset = BS / 64;
	static const float size   = BS / 2;
	float y2 = sloped ? size : -size;
	v3f vertices[4] = {
		v3f(-size,    y2 + offset,  size),
		v3f( size,    y2 + offset,  size),
		v3f( size, -size + offset, -size),
		v3f(-size, -size + offset, -size),
	};
	if (angle)
		for (v3f &vertex : vertices)
			vertex.rotateXZBy(angle);
	drawQuad(vertices);
}

namespace {
	static const v3s16 nodebox_tile_dirs[6] = {
		v3s16(0, 1, 0),
		v3s16(0, -1, 0),
		v3s16(1, 0, 0),
		v3s16(-1, 0, 0),
		v3s16(0, 0, 1),
		v3s16(0, 0, -1)
	};

	// we have this order for some reason...
	static const v3s16 nodebox_connection_dirs[6] = {
		v3s16( 0,  1,  0), // top
		v3s16( 0, -1,  0), // bottom
		v3s16( 0,  0, -1), // front
		v3s16(-1,  0,  0), // left
		v3s16( 0,  0,  1), // back
		v3s16( 1,  0,  0), // right
	};
}

void MapblockMeshGenerator::drawNodeboxNode()
{
	TileSpec tiles[6];
	for (int face = 0; face < 6; face++) {
		// Handles facedir rotation for textures
		getTile(nodebox_tile_dirs[face], &tiles[face]);
	}

	// locate possible neighboring nodes to connect to
	u8 neighbors_set = 0;
	if (f->node_box.type == NODEBOX_CONNECTED) {
		for (int dir = 0; dir != 6; dir++) {
			u8 flag = 1 << dir;
			v3s16 p2 = blockpos_nodes + p + nodebox_connection_dirs[dir];
			MapNode n2 = data->m_vmanip.getNodeNoEx(p2);
			if (nodedef->nodeboxConnects(n, n2, flag))
				neighbors_set |= flag;
		}
	}

	std::vector<aabb3f> boxes;
	n.getNodeBoxes(nodedef, &boxes, neighbors_set);
	for (auto &box : boxes)
		drawAutoLightedCuboid(box, nullptr, tiles, 6);
}

void MapblockMeshGenerator::drawMeshNode()
{
	u8 facedir = 0;
	scene::IMesh* mesh;
	bool private_mesh; // as a grab/drop pair is not thread-safe

	if (f->param_type_2 == CPT2_FACEDIR ||
			f->param_type_2 == CPT2_COLORED_FACEDIR) {
		facedir = n.getFaceDir(nodedef);
	} else if (f->param_type_2 == CPT2_WALLMOUNTED ||
			f->param_type_2 == CPT2_COLORED_WALLMOUNTED) {
		// Convert wallmounted to 6dfacedir.
		// When cache enabled, it is already converted.
		facedir = n.getWallMounted(nodedef);
		if (!enable_mesh_cache)
			facedir = wallmounted_to_facedir[facedir];
	}

	if (!data->m_smooth_lighting && f->mesh_ptr[facedir]) {
		// use cached meshes
		private_mesh = false;
		mesh = f->mesh_ptr[facedir];
	} else if (f->mesh_ptr[0]) {
		// no cache, clone and rotate mesh
		private_mesh = true;
		mesh = cloneMesh(f->mesh_ptr[0]);
		rotateMeshBy6dFacedir(mesh, facedir);
		recalculateBoundingBox(mesh);
		meshmanip->recalculateNormals(mesh, true, false);
	} else
		return;

	int mesh_buffer_count = mesh->getMeshBufferCount();
	for (int j = 0; j < mesh_buffer_count; j++) {
		useTile(j);
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex *)buf->getVertices();
		int vertex_count = buf->getVertexCount();

		if (data->m_smooth_lighting) {
			// Mesh is always private here. So the lighting is applied to each
			// vertex right here.
			for (int k = 0; k < vertex_count; k++) {
				video::S3DVertex &vertex = vertices[k];
				vertex.Color = blendLightColor(vertex.Pos, vertex.Normal);
				vertex.Pos += origin;
			}
			collector->append(tile, vertices, vertex_count,
				buf->getIndices(), buf->getIndexCount());
		} else {
			// Don't modify the mesh, it may not be private here.
			// Instead, let the collector process colors, etc.
			collector->append(tile, vertices, vertex_count,
				buf->getIndices(), buf->getIndexCount(), origin,
				color, f->light_source);
		}
	}
	if (private_mesh)
		mesh->drop();
}

// also called when the drawtype is known but should have been pre-converted
void MapblockMeshGenerator::errorUnknownDrawtype()
{
	infostream << "Got drawtype " << f->drawtype << std::endl;
	FATAL_ERROR("Unknown drawtype");
}

void MapblockMeshGenerator::drawNode()
{
	// skip some drawtypes early
	switch (f->drawtype) {
		case NDT_NORMAL:   // Drawn by MapBlockMesh
		case NDT_AIRLIKE:  // Not drawn at all
		case NDT_LIQUID:   // Drawn by MapBlockMesh
			return;
		default:
			break;
	}
	origin = intToFloat(p, BS);
	if (data->m_smooth_lighting)
		getSmoothLightFrame();
	else
		light = LightPair(getInteriorLight(n, 1, nodedef));
	switch (f->drawtype) {
		case NDT_FLOWINGLIQUID:     drawLiquidNode(); break;
		case NDT_GLASSLIKE:         drawGlasslikeNode(); break;
		case NDT_GLASSLIKE_FRAMED:  drawGlasslikeFramedNode(); break;
		case NDT_ALLFACES:          drawAllfacesNode(); break;
		case NDT_TORCHLIKE:         drawTorchlikeNode(); break;
		case NDT_SIGNLIKE:          drawSignlikeNode(); break;
		case NDT_PLANTLIKE:         drawPlantlikeNode(); break;
		case NDT_PLANTLIKE_ROOTED:  drawPlantlikeRootedNode(); break;
		case NDT_FIRELIKE:          drawFirelikeNode(); break;
		case NDT_FENCELIKE:         drawFencelikeNode(); break;
		case NDT_RAILLIKE:          drawRaillikeNode(); break;
		case NDT_NODEBOX:           drawNodeboxNode(); break;
		case NDT_MESH:              drawMeshNode(); break;
		default:                    errorUnknownDrawtype(); break;
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
	for (p.X = 0; p.X < MAP_BLOCKSIZE; p.X++) {
		n = data->m_vmanip.getNodeNoEx(blockpos_nodes + p);
		f = &nodedef->get(n);
		drawNode();
	}
}

void MapblockMeshGenerator::renderSingle(content_t node, u8 param2)
{
	p = {0, 0, 0};
	n = MapNode(node, 0xff, param2);
	f = &nodedef->get(n);
	drawNode();
}
