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

#include "mapblock_mesh.h"
#include "client.h"
#include "mapblock.h"
#include "map.h"
#include "profiler.h"
#include "shader.h"
#include "mesh.h"
#include "minimap.h"
#include "content_mapblock.h"
#include "util/directiontables.h"
#include "client/meshgen/collector.h"
#include "client/renderingengine.h"
#include <array>
#include <algorithm>

/*
	MeshMakeData
*/

MeshMakeData::MeshMakeData(Client *client, bool use_shaders):
	m_client(client),
	m_use_shaders(use_shaders)
{}

void MeshMakeData::fillBlockDataBegin(const v3s16 &blockpos)
{
	m_blockpos = blockpos;

	v3s16 blockpos_nodes = m_blockpos*MAP_BLOCKSIZE;

	m_vmanip.clear();
	VoxelArea voxel_area(blockpos_nodes - v3s16(1,1,1) * MAP_BLOCKSIZE,
			blockpos_nodes + v3s16(1,1,1) * MAP_BLOCKSIZE*2-v3s16(1,1,1));
	m_vmanip.addArea(voxel_area);
}

void MeshMakeData::fillBlockData(const v3s16 &block_offset, MapNode *data)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));

	v3s16 bp = m_blockpos + block_offset;
	v3s16 blockpos_nodes = bp * MAP_BLOCKSIZE;
	m_vmanip.copyFrom(data, data_area, v3s16(0,0,0), blockpos_nodes, data_size);
}

void MeshMakeData::fill(MapBlock *block)
{
	fillBlockDataBegin(block->getPos());

	fillBlockData(v3s16(0,0,0), block->getData());

	// Get map for reading neighbor blocks
	Map *map = block->getParent();

	for (const v3s16 &dir : g_26dirs) {
		v3s16 bp = m_blockpos + dir;
		MapBlock *b = map->getBlockNoCreateNoEx(bp);
		if(b)
			fillBlockData(dir, b->getData());
	}
}

void MeshMakeData::setCrack(int crack_level, v3s16 crack_pos)
{
	if (crack_level >= 0)
		m_crack_pos_relative = crack_pos - m_blockpos*MAP_BLOCKSIZE;
}

void MeshMakeData::setSmoothLighting(bool smooth_lighting)
{
	m_smooth_lighting = smooth_lighting;
}

/*
	Light and vertex color functions
*/

/*
	Calculate non-smooth lighting at interior of node.
	Single light bank.
*/
static u8 getInteriorLight(enum LightBank bank, MapNode n, s32 increment,
	const NodeDefManager *ndef)
{
	u8 light = n.getLight(bank, ndef->getLightingFlags(n));
	light = rangelim(light + increment, 0, LIGHT_SUN);
	return decode_light(light);
}

/*
	Calculate non-smooth lighting at interior of node.
	Both light banks.
*/
u16 getInteriorLight(MapNode n, s32 increment, const NodeDefManager *ndef)
{
	u16 day = getInteriorLight(LIGHTBANK_DAY, n, increment, ndef);
	u16 night = getInteriorLight(LIGHTBANK_NIGHT, n, increment, ndef);
	return day | (night << 8);
}

/*
	Calculate non-smooth lighting at face of node.
	Single light bank.
*/
static u8 getFaceLight(enum LightBank bank, MapNode n, MapNode n2,
	v3s16 face_dir, const NodeDefManager *ndef)
{
	ContentLightingFlags f1 = ndef->getLightingFlags(n);
	ContentLightingFlags f2 = ndef->getLightingFlags(n2);

	u8 light;
	u8 l1 = n.getLight(bank, f1);
	u8 l2 = n2.getLight(bank, f2);
	if(l1 > l2)
		light = l1;
	else
		light = l2;

	// Boost light level for light sources
	u8 light_source = MYMAX(f1.light_source, f2.light_source);
	if(light_source > light)
		light = light_source;

	return decode_light(light);
}

/*
	Calculate non-smooth lighting at face of node.
	Both light banks.
*/
u16 getFaceLight(MapNode n, MapNode n2, const v3s16 &face_dir,
	const NodeDefManager *ndef)
{
	u16 day = getFaceLight(LIGHTBANK_DAY, n, n2, face_dir, ndef);
	u16 night = getFaceLight(LIGHTBANK_NIGHT, n, n2, face_dir, ndef);
	return day | (night << 8);
}

/*
	Calculate smooth lighting at the XYZ- corner of p.
	Both light banks
*/
static u16 getSmoothLightCombined(const v3s16 &p,
	const std::array<v3s16,8> &dirs, MeshMakeData *data)
{
	const NodeDefManager *ndef = data->m_client->ndef();

	u16 ambient_occlusion = 0;
	u16 light_count = 0;
	u8 light_source_max = 0;
	u16 light_day = 0;
	u16 light_night = 0;
	bool direct_sunlight = false;

	auto add_node = [&] (u8 i, bool obstructed = false) -> bool {
		if (obstructed) {
			ambient_occlusion++;
			return false;
		}
		MapNode n = data->m_vmanip.getNodeNoExNoEmerge(p + dirs[i]);
		if (n.getContent() == CONTENT_IGNORE)
			return true;
		const ContentFeatures &f = ndef->get(n);
		if (f.light_source > light_source_max)
			light_source_max = f.light_source;
		// Check f.solidness because fast-style leaves look better this way
		if (f.param_type == CPT_LIGHT && f.solidness != 2) {
			u8 light_level_day = n.getLight(LIGHTBANK_DAY, f.getLightingFlags());
			u8 light_level_night = n.getLight(LIGHTBANK_NIGHT, f.getLightingFlags());
			if (light_level_day == LIGHT_SUN)
				direct_sunlight = true;
			light_day += decode_light(light_level_day);
			light_night += decode_light(light_level_night);
			light_count++;
		} else {
			ambient_occlusion++;
		}
		return f.light_propagates;
	};

	bool obstructed[4] = { true, true, true, true };
	add_node(0);
	bool opaque1 = !add_node(1);
	bool opaque2 = !add_node(2);
	bool opaque3 = !add_node(3);
	obstructed[0] = opaque1 && opaque2;
	obstructed[1] = opaque1 && opaque3;
	obstructed[2] = opaque2 && opaque3;
	for (u8 k = 0; k < 3; ++k)
		if (add_node(k + 4, obstructed[k]))
			obstructed[3] = false;
	if (add_node(7, obstructed[3])) { // wrap light around nodes
		ambient_occlusion -= 3;
		for (u8 k = 0; k < 3; ++k)
			add_node(k + 4, !obstructed[k]);
	}

	if (light_count == 0) {
		light_day = light_night = 0;
	} else {
		light_day /= light_count;
		light_night /= light_count;
	}

	// boost direct sunlight, if any
	if (direct_sunlight)
		light_day = 0xFF;

	// Boost brightness around light sources
	bool skip_ambient_occlusion_day = false;
	if (decode_light(light_source_max) >= light_day) {
		light_day = decode_light(light_source_max);
		skip_ambient_occlusion_day = true;
	}

	bool skip_ambient_occlusion_night = false;
	if(decode_light(light_source_max) >= light_night) {
		light_night = decode_light(light_source_max);
		skip_ambient_occlusion_night = true;
	}

	if (ambient_occlusion > 4) {
		static thread_local const float ao_gamma = rangelim(
			g_settings->getFloat("ambient_occlusion_gamma"), 0.25, 4.0);

		// Table of gamma space multiply factors.
		static thread_local const float light_amount[3] = {
			powf(0.75, 1.0 / ao_gamma),
			powf(0.5,  1.0 / ao_gamma),
			powf(0.25, 1.0 / ao_gamma)
		};

		//calculate table index for gamma space multiplier
		ambient_occlusion -= 5;

		if (!skip_ambient_occlusion_day)
			light_day = rangelim(core::round32(
					light_day * light_amount[ambient_occlusion]), 0, 255);
		if (!skip_ambient_occlusion_night)
			light_night = rangelim(core::round32(
					light_night * light_amount[ambient_occlusion]), 0, 255);
	}

	return light_day | (light_night << 8);
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
	Node at p is solid, and thus the lighting is face-dependent.
*/
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data)
{
	return getSmoothLightTransparent(p + face_dir, corner - 2 * face_dir, data);
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
	Node at p is not solid, and the lighting is not face-dependent.
*/
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data)
{
	const std::array<v3s16,8> dirs = {{
		// Always shine light
		v3s16(0,0,0),
		v3s16(corner.X,0,0),
		v3s16(0,corner.Y,0),
		v3s16(0,0,corner.Z),

		// Can be obstructed
		v3s16(corner.X,corner.Y,0),
		v3s16(corner.X,0,corner.Z),
		v3s16(0,corner.Y,corner.Z),
		v3s16(corner.X,corner.Y,corner.Z)
	}};
	return getSmoothLightCombined(p, dirs, data);
}

void get_sunlight_color(video::SColorf *sunlight, u32 daynight_ratio){
	f32 rg = daynight_ratio / 1000.0f - 0.04f;
	f32 b = (0.98f * daynight_ratio) / 1000.0f + 0.078f;
	sunlight->r = rg;
	sunlight->g = rg;
	sunlight->b = b;
}

void final_color_blend(video::SColor *result,
		u16 light, u32 daynight_ratio)
{
	video::SColorf dayLight;
	get_sunlight_color(&dayLight, daynight_ratio);
	final_color_blend(result,
		encode_light(light, 0), dayLight);
}

void final_color_blend(video::SColor *result,
		const video::SColor &data, const video::SColorf &dayLight)
{
	static const video::SColorf artificialColor(1.04f, 1.04f, 1.04f);

	video::SColorf c(data);
	f32 n = 1 - c.a;

	f32 r = c.r * (c.a * dayLight.r + n * artificialColor.r) * 2.0f;
	f32 g = c.g * (c.a * dayLight.g + n * artificialColor.g) * 2.0f;
	f32 b = c.b * (c.a * dayLight.b + n * artificialColor.b) * 2.0f;

	// Emphase blue a bit in darker places
	// Each entry of this array represents a range of 8 blue levels
	static const u8 emphase_blue_when_dark[32] = {
		1, 4, 6, 6, 6, 5, 4, 3, 2, 1, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	};

	b += emphase_blue_when_dark[irr::core::clamp((s32) ((r + g + b) / 3 * 255),
		0, 255) / 8] / 255.0f;

	result->setRed(core::clamp((s32) (r * 255.0f), 0, 255));
	result->setGreen(core::clamp((s32) (g * 255.0f), 0, 255));
	result->setBlue(core::clamp((s32) (b * 255.0f), 0, 255));
}

/*
	Mesh generation helpers
*/

// This table is moved outside getNodeVertexDirs to avoid the compiler using
// a mutex to initialize this table at runtime right in the hot path.
// For details search the internet for "cxa_guard_acquire".
static const v3s16 vertex_dirs_table[] = {
	// ( 1, 0, 0)
	v3s16( 1,-1, 1), v3s16( 1,-1,-1),
	v3s16( 1, 1,-1), v3s16( 1, 1, 1),
	// ( 0, 1, 0)
	v3s16( 1, 1,-1), v3s16(-1, 1,-1),
	v3s16(-1, 1, 1), v3s16( 1, 1, 1),
	// ( 0, 0, 1)
	v3s16(-1,-1, 1), v3s16( 1,-1, 1),
	v3s16( 1, 1, 1), v3s16(-1, 1, 1),
	// invalid
	v3s16(), v3s16(), v3s16(), v3s16(),
	// ( 0, 0,-1)
	v3s16( 1,-1,-1), v3s16(-1,-1,-1),
	v3s16(-1, 1,-1), v3s16( 1, 1,-1),
	// ( 0,-1, 0)
	v3s16( 1,-1, 1), v3s16(-1,-1, 1),
	v3s16(-1,-1,-1), v3s16( 1,-1,-1),
	// (-1, 0, 0)
	v3s16(-1,-1,-1), v3s16(-1,-1, 1),
	v3s16(-1, 1, 1), v3s16(-1, 1,-1)
};

/*
	vertex_dirs: v3s16[4]
*/
static void getNodeVertexDirs(const v3s16 &dir, v3s16 *vertex_dirs)
{
	/*
		If looked from outside the node towards the face, the corners are:
		0: bottom-right
		1: bottom-left
		2: top-left
		3: top-right
	*/

	// Direction must be (1,0,0), (-1,0,0), (0,1,0), (0,-1,0),
	// (0,0,1), (0,0,-1)
	assert(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z == 1);

	// Convert direction to single integer for table lookup
	u8 idx = (dir.X + 2 * dir.Y + 3 * dir.Z) & 7;
	idx = (idx - 1) * 4;

#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic push
#if __GNUC__ > 7
#pragma GCC diagnostic ignored "-Wclass-memaccess"
#endif
#endif
	memcpy(vertex_dirs, &vertex_dirs_table[idx], 4 * sizeof(v3s16));
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic pop
#endif
}

static void getNodeTextureCoords(v3f base, const v3f &scale, const v3s16 &dir, float *u, float *v)
{
	if (dir.X > 0 || dir.Y != 0 || dir.Z < 0)
		base -= scale;
	if (dir == v3s16(0,0,1)) {
		*u = -base.X;
		*v = -base.Y;
	} else if (dir == v3s16(0,0,-1)) {
		*u = base.X + 1;
		*v = -base.Y - 1;
	} else if (dir == v3s16(1,0,0)) {
		*u = base.Z + 1;
		*v = -base.Y - 1;
	} else if (dir == v3s16(-1,0,0)) {
		*u = -base.Z;
		*v = -base.Y;
	} else if (dir == v3s16(0,1,0)) {
		*u = base.X + 1;
		*v = -base.Z - 1;
	} else if (dir == v3s16(0,-1,0)) {
		*u = base.X + 1;
		*v = base.Z + 1;
	}
}

struct FastFace
{
	TileSpec tile;
	video::S3DVertex vertices[4]; // Precalculated vertices
	/*!
	 * The face is divided into two triangles. If this is true,
	 * vertices 0 and 2 are connected, othervise vertices 1 and 3
	 * are connected.
	 */
	bool vertex_0_2_connected;
};

static void makeFastFace(const TileSpec &tile, u16 li0, u16 li1, u16 li2, u16 li3,
	const v3f &tp, const v3f &p, const v3s16 &dir, const v3f &scale, std::vector<FastFace> &dest)
{
	// Position is at the center of the cube.
	v3f pos = p * BS;

	float x0 = 0.0f;
	float y0 = 0.0f;
	float w = 1.0f;
	float h = 1.0f;

	v3f vertex_pos[4];
	v3s16 vertex_dirs[4];
	getNodeVertexDirs(dir, vertex_dirs);
	if (tile.world_aligned)
		getNodeTextureCoords(tp, scale, dir, &x0, &y0);

	v3s16 t;
	u16 t1;
	switch (tile.rotation) {
	case 0:
		break;
	case 1: //R90
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[3];
		vertex_dirs[3] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[1];
		vertex_dirs[1] = t;
		t1  = li0;
		li0 = li3;
		li3 = li2;
		li2 = li1;
		li1 = t1;
		break;
	case 2: //R180
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[2];
		vertex_dirs[2] = t;
		t = vertex_dirs[1];
		vertex_dirs[1] = vertex_dirs[3];
		vertex_dirs[3] = t;
		t1  = li0;
		li0 = li2;
		li2 = t1;
		t1  = li1;
		li1 = li3;
		li3 = t1;
		break;
	case 3: //R270
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[1];
		vertex_dirs[1] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[3];
		vertex_dirs[3] = t;
		t1  = li0;
		li0 = li1;
		li1 = li2;
		li2 = li3;
		li3 = t1;
		break;
	case 4: //FXR90
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[3];
		vertex_dirs[3] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[1];
		vertex_dirs[1] = t;
		t1  = li0;
		li0 = li3;
		li3 = li2;
		li2 = li1;
		li1 = t1;
		y0 += h;
		h *= -1;
		break;
	case 5: //FXR270
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[1];
		vertex_dirs[1] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[3];
		vertex_dirs[3] = t;
		t1  = li0;
		li0 = li1;
		li1 = li2;
		li2 = li3;
		li3 = t1;
		y0 += h;
		h *= -1;
		break;
	case 6: //FYR90
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[3];
		vertex_dirs[3] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[1];
		vertex_dirs[1] = t;
		t1  = li0;
		li0 = li3;
		li3 = li2;
		li2 = li1;
		li1 = t1;
		x0 += w;
		w *= -1;
		break;
	case 7: //FYR270
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[1];
		vertex_dirs[1] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[3];
		vertex_dirs[3] = t;
		t1  = li0;
		li0 = li1;
		li1 = li2;
		li2 = li3;
		li3 = t1;
		x0 += w;
		w *= -1;
		break;
	case 8: //FX
		y0 += h;
		h *= -1;
		break;
	case 9: //FY
		x0 += w;
		w *= -1;
		break;
	default:
		break;
	}

	for (u16 i = 0; i < 4; i++) {
		vertex_pos[i] = v3f(
				BS / 2 * vertex_dirs[i].X,
				BS / 2 * vertex_dirs[i].Y,
				BS / 2 * vertex_dirs[i].Z
		);
	}

	for (v3f &vpos : vertex_pos) {
		vpos.X *= scale.X;
		vpos.Y *= scale.Y;
		vpos.Z *= scale.Z;
		vpos += pos;
	}

	f32 abs_scale = 1.0f;
	if      (scale.X < 0.999f || scale.X > 1.001f) abs_scale = scale.X;
	else if (scale.Y < 0.999f || scale.Y > 1.001f) abs_scale = scale.Y;
	else if (scale.Z < 0.999f || scale.Z > 1.001f) abs_scale = scale.Z;

	v3f normal(dir.X, dir.Y, dir.Z);

	u16 li[4] = { li0, li1, li2, li3 };
	u16 day[4];
	u16 night[4];

	for (u8 i = 0; i < 4; i++) {
		day[i] = li[i] >> 8;
		night[i] = li[i] & 0xFF;
	}

	bool vertex_0_2_connected = abs(day[0] - day[2]) + abs(night[0] - night[2])
			< abs(day[1] - day[3]) + abs(night[1] - night[3]);

	v2f32 f[4] = {
		core::vector2d<f32>(x0 + w * abs_scale, y0 + h),
		core::vector2d<f32>(x0, y0 + h),
		core::vector2d<f32>(x0, y0),
		core::vector2d<f32>(x0 + w * abs_scale, y0) };

	// equivalent to dest.push_back(FastFace()) but faster
	dest.emplace_back();
	FastFace& face = *dest.rbegin();

	for (u8 i = 0; i < 4; i++) {
		video::SColor c = encode_light(li[i], tile.emissive_light);
		if (!tile.emissive_light)
			applyFacesShading(c, normal);

		face.vertices[i] = video::S3DVertex(vertex_pos[i], normal, c, f[i]);
	}

	/*
		Revert triangles for nicer looking gradient if the
		brightness of vertices 1 and 3 differ less than
		the brightness of vertices 0 and 2.
		*/
	face.vertex_0_2_connected = vertex_0_2_connected;
	face.tile = tile;
}

/*
	Nodes make a face if contents differ and solidness differs.
	Return value:
		0: No face
		1: Face uses m1's content
		2: Face uses m2's content
	equivalent: Whether the blocks share the same face (eg. water and glass)

	TODO: Add 3: Both faces drawn with backface culling, remove equivalent
*/
static u8 face_contents(content_t m1, content_t m2, bool *equivalent,
	const NodeDefManager *ndef)
{
	*equivalent = false;

	if (m1 == m2 || m1 == CONTENT_IGNORE || m2 == CONTENT_IGNORE)
		return 0;

	const ContentFeatures &f1 = ndef->get(m1);
	const ContentFeatures &f2 = ndef->get(m2);

	// Contents don't differ for different forms of same liquid
	if (f1.sameLiquidRender(f2))
		return 0;

	u8 c1 = f1.solidness;
	u8 c2 = f2.solidness;

	if (c1 == c2)
		return 0;

	if (c1 == 0)
		c1 = f1.visual_solidness;
	else if (c2 == 0)
		c2 = f2.visual_solidness;

	if (c1 == c2) {
		*equivalent = true;
		// If same solidness, liquid takes precense
		if (f1.isLiquidRender())
			return 1;
		if (f2.isLiquidRender())
			return 2;
	}

	if (c1 > c2)
		return 1;

	return 2;
}

/*
	Gets nth node tile (0 <= n <= 5).
*/
void getNodeTileN(MapNode mn, const v3s16 &p, u8 tileindex, MeshMakeData *data, TileSpec &tile)
{
	const NodeDefManager *ndef = data->m_client->ndef();
	const ContentFeatures &f = ndef->get(mn);
	tile = f.tiles[tileindex];
	bool has_crack = p == data->m_crack_pos_relative;
	for (TileLayer &layer : tile.layers) {
		if (layer.texture_id == 0)
			continue;
		if (!layer.has_color)
			mn.getColor(f, &(layer.color));
		// Apply temporary crack
		if (has_crack)
			layer.material_flags |= MATERIAL_FLAG_CRACK;
	}
}

/*
	Gets node tile given a face direction.
*/
void getNodeTile(MapNode mn, const v3s16 &p, const v3s16 &dir, MeshMakeData *data, TileSpec &tile)
{
	const NodeDefManager *ndef = data->m_client->ndef();

	// Direction must be (1,0,0), (-1,0,0), (0,1,0), (0,-1,0),
	// (0,0,1), (0,0,-1) or (0,0,0)
	assert(dir.X * dir.X + dir.Y * dir.Y + dir.Z * dir.Z <= 1);

	// Convert direction to single integer for table lookup
	//  0 = (0,0,0)
	//  1 = (1,0,0)
	//  2 = (0,1,0)
	//  3 = (0,0,1)
	//  4 = invalid, treat as (0,0,0)
	//  5 = (0,0,-1)
	//  6 = (0,-1,0)
	//  7 = (-1,0,0)
	u8 dir_i = ((dir.X + 2 * dir.Y + 3 * dir.Z) & 7) * 2;

	// Get rotation for things like chests
	u8 facedir = mn.getFaceDir(ndef, true);

	static const u16 dir_to_tile[24 * 16] =
	{
		// 0     +X    +Y    +Z           -Z    -Y    -X   ->   value=tile,rotation
		   0,0,  2,0 , 0,0 , 4,0 ,  0,0,  5,0 , 1,0 , 3,0 ,  // rotate around y+ 0 - 3
		   0,0,  4,0 , 0,3 , 3,0 ,  0,0,  2,0 , 1,1 , 5,0 ,
		   0,0,  3,0 , 0,2 , 5,0 ,  0,0,  4,0 , 1,2 , 2,0 ,
		   0,0,  5,0 , 0,1 , 2,0 ,  0,0,  3,0 , 1,3 , 4,0 ,

		   0,0,  2,3 , 5,0 , 0,2 ,  0,0,  1,0 , 4,2 , 3,1 ,  // rotate around z+ 4 - 7
		   0,0,  4,3 , 2,0 , 0,1 ,  0,0,  1,1 , 3,2 , 5,1 ,
		   0,0,  3,3 , 4,0 , 0,0 ,  0,0,  1,2 , 5,2 , 2,1 ,
		   0,0,  5,3 , 3,0 , 0,3 ,  0,0,  1,3 , 2,2 , 4,1 ,

		   0,0,  2,1 , 4,2 , 1,2 ,  0,0,  0,0 , 5,0 , 3,3 ,  // rotate around z- 8 - 11
		   0,0,  4,1 , 3,2 , 1,3 ,  0,0,  0,3 , 2,0 , 5,3 ,
		   0,0,  3,1 , 5,2 , 1,0 ,  0,0,  0,2 , 4,0 , 2,3 ,
		   0,0,  5,1 , 2,2 , 1,1 ,  0,0,  0,1 , 3,0 , 4,3 ,

		   0,0,  0,3 , 3,3 , 4,1 ,  0,0,  5,3 , 2,3 , 1,3 ,  // rotate around x+ 12 - 15
		   0,0,  0,2 , 5,3 , 3,1 ,  0,0,  2,3 , 4,3 , 1,0 ,
		   0,0,  0,1 , 2,3 , 5,1 ,  0,0,  4,3 , 3,3 , 1,1 ,
		   0,0,  0,0 , 4,3 , 2,1 ,  0,0,  3,3 , 5,3 , 1,2 ,

		   0,0,  1,1 , 2,1 , 4,3 ,  0,0,  5,1 , 3,1 , 0,1 ,  // rotate around x- 16 - 19
		   0,0,  1,2 , 4,1 , 3,3 ,  0,0,  2,1 , 5,1 , 0,0 ,
		   0,0,  1,3 , 3,1 , 5,3 ,  0,0,  4,1 , 2,1 , 0,3 ,
		   0,0,  1,0 , 5,1 , 2,3 ,  0,0,  3,1 , 4,1 , 0,2 ,

		   0,0,  3,2 , 1,2 , 4,2 ,  0,0,  5,2 , 0,2 , 2,2 ,  // rotate around y- 20 - 23
		   0,0,  5,2 , 1,3 , 3,2 ,  0,0,  2,2 , 0,1 , 4,2 ,
		   0,0,  2,2 , 1,0 , 5,2 ,  0,0,  4,2 , 0,0 , 3,2 ,
		   0,0,  4,2 , 1,1 , 2,2 ,  0,0,  3,2 , 0,3 , 5,2

	};
	u16 tile_index = facedir * 16 + dir_i;
	getNodeTileN(mn, p, dir_to_tile[tile_index], data, tile);
	tile.rotation = tile.world_aligned ? 0 : dir_to_tile[tile_index + 1];
}

static void getTileInfo(
		// Input:
		MeshMakeData *data,
		const v3s16 &p,
		const v3s16 &face_dir,
		// Output:
		bool &makes_face,
		v3s16 &p_corrected,
		v3s16 &face_dir_corrected,
		u16 *lights,
		u8 &waving,
		TileSpec &tile
	)
{
	VoxelManipulator &vmanip = data->m_vmanip;
	const NodeDefManager *ndef = data->m_client->ndef();
	v3s16 blockpos_nodes = data->m_blockpos * MAP_BLOCKSIZE;

	const MapNode &n0 = vmanip.getNodeRefUnsafe(blockpos_nodes + p);

	// Don't even try to get n1 if n0 is already CONTENT_IGNORE
	if (n0.getContent() == CONTENT_IGNORE) {
		makes_face = false;
		return;
	}

	const MapNode &n1 = vmanip.getNodeRefUnsafeCheckFlags(blockpos_nodes + p + face_dir);

	if (n1.getContent() == CONTENT_IGNORE) {
		makes_face = false;
		return;
	}

	// This is hackish
	bool equivalent = false;
	u8 mf = face_contents(n0.getContent(), n1.getContent(),
			&equivalent, ndef);

	if (mf == 0) {
		makes_face = false;
		return;
	}

	makes_face = true;

	MapNode n = n0;

	if (mf == 1) {
		p_corrected = p;
		face_dir_corrected = face_dir;
	} else {
		n = n1;
		p_corrected = p + face_dir;
		face_dir_corrected = -face_dir;
	}

	getNodeTile(n, p_corrected, face_dir_corrected, data, tile);
	const ContentFeatures &f = ndef->get(n);
	waving = f.waving;
	tile.emissive_light = f.light_source;

	// eg. water and glass
	if (equivalent) {
		for (TileLayer &layer : tile.layers)
			layer.material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
	}

	if (!data->m_smooth_lighting) {
		lights[0] = lights[1] = lights[2] = lights[3] =
				getFaceLight(n0, n1, face_dir, ndef);
	} else {
		v3s16 vertex_dirs[4];
		getNodeVertexDirs(face_dir_corrected, vertex_dirs);

		v3s16 light_p = blockpos_nodes + p_corrected;
		for (u16 i = 0; i < 4; i++)
			lights[i] = getSmoothLightSolid(light_p, face_dir_corrected, vertex_dirs[i], data);
	}
}

/*
	startpos:
	translate_dir: unit vector with only one of x, y or z
	face_dir: unit vector with only one of x, y or z
*/
static void updateFastFaceRow(
		MeshMakeData *data,
		const v3s16 &&startpos,
		v3s16 translate_dir,
		const v3f &&translate_dir_f,
		const v3s16 &&face_dir,
		std::vector<FastFace> &dest)
{
	static thread_local const bool waving_liquids =
		g_settings->getBool("enable_shaders") &&
		g_settings->getBool("enable_waving_water");

	static thread_local const bool force_not_tiling =
			g_settings->getBool("enable_dynamic_shadows");

	v3s16 p = startpos;

	u16 continuous_tiles_count = 1;

	bool makes_face = false;
	v3s16 p_corrected;
	v3s16 face_dir_corrected;
	u16 lights[4] = {0, 0, 0, 0};
	u8 waving = 0;
	TileSpec tile;

	// Get info of first tile
	getTileInfo(data, p, face_dir,
			makes_face, p_corrected, face_dir_corrected,
			lights, waving, tile);

	// Unroll this variable which has a significant build cost
	TileSpec next_tile;
	for (u16 j = 0; j < MAP_BLOCKSIZE; j++) {
		// If tiling can be done, this is set to false in the next step
		bool next_is_different = true;

		bool next_makes_face = false;
		v3s16 next_p_corrected;
		v3s16 next_face_dir_corrected;
		u16 next_lights[4] = {0, 0, 0, 0};

		// If at last position, there is nothing to compare to and
		// the face must be drawn anyway
		if (j != MAP_BLOCKSIZE - 1) {
			p += translate_dir;

			getTileInfo(data, p, face_dir,
					next_makes_face, next_p_corrected,
					next_face_dir_corrected, next_lights,
					waving,
					next_tile);

			if (!force_not_tiling
					&& next_makes_face == makes_face
					&& next_p_corrected == p_corrected + translate_dir
					&& next_face_dir_corrected == face_dir_corrected
					&& memcmp(next_lights, lights, sizeof(lights)) == 0
					// Don't apply fast faces to waving water.
					&& (waving != 3 || !waving_liquids)
					&& next_tile.isTileable(tile)) {
				next_is_different = false;
				continuous_tiles_count++;
			}
		}
		if (next_is_different) {
			/*
				Create a face if there should be one
			*/
			if (makes_face) {
				// Floating point conversion of the position vector
				v3f pf(p_corrected.X, p_corrected.Y, p_corrected.Z);
				// Center point of face (kind of)
				v3f sp = pf - ((f32)continuous_tiles_count * 0.5f - 0.5f)
					* translate_dir_f;
				v3f scale(1, 1, 1);

				if (translate_dir.X != 0)
					scale.X = continuous_tiles_count;
				if (translate_dir.Y != 0)
					scale.Y = continuous_tiles_count;
				if (translate_dir.Z != 0)
					scale.Z = continuous_tiles_count;

				makeFastFace(tile, lights[0], lights[1], lights[2], lights[3],
						pf, sp, face_dir_corrected, scale, dest);
				g_profiler->avg("Meshgen: Tiles per face [#]", continuous_tiles_count);
			}

			continuous_tiles_count = 1;
		}

		makes_face = next_makes_face;
		p_corrected = next_p_corrected;
		face_dir_corrected = next_face_dir_corrected;
		memcpy(lights, next_lights, sizeof(lights));
		if (next_is_different)
			tile = std::move(next_tile); // faster than copy
	}
}

static void updateAllFastFaceRows(MeshMakeData *data,
		std::vector<FastFace> &dest)
{
	/*
		Go through every y,z and get top(y+) faces in rows of x+
	*/
	for (s16 y = 0; y < MAP_BLOCKSIZE; y++)
	for (s16 z = 0; z < MAP_BLOCKSIZE; z++)
		updateFastFaceRow(data,
				v3s16(0, y, z),
				v3s16(1, 0, 0), //dir
				v3f  (1, 0, 0),
				v3s16(0, 1, 0), //face dir
				dest);

	/*
		Go through every x,y and get right(x+) faces in rows of z+
	*/
	for (s16 x = 0; x < MAP_BLOCKSIZE; x++)
	for (s16 y = 0; y < MAP_BLOCKSIZE; y++)
		updateFastFaceRow(data,
				v3s16(x, y, 0),
				v3s16(0, 0, 1), //dir
				v3f  (0, 0, 1),
				v3s16(1, 0, 0), //face dir
				dest);

	/*
		Go through every y,z and get back(z+) faces in rows of x+
	*/
	for (s16 z = 0; z < MAP_BLOCKSIZE; z++)
	for (s16 y = 0; y < MAP_BLOCKSIZE; y++)
		updateFastFaceRow(data,
				v3s16(0, y, z),
				v3s16(1, 0, 0), //dir
				v3f  (1, 0, 0),
				v3s16(0, 0, 1), //face dir
				dest);
}

static void applyTileColor(PreMeshBuffer &pmb)
{
	video::SColor tc = pmb.layer.color;
	if (tc == video::SColor(0xFFFFFFFF))
		return;
	for (video::S3DVertex &vertex : pmb.vertices) {
		video::SColor *c = &vertex.Color;
		c->set(c->getAlpha(),
			c->getRed() * tc.getRed() / 255,
			c->getGreen() * tc.getGreen() / 255,
			c->getBlue() * tc.getBlue() / 255);
	}
}

/*
	MapBlockBspTree
*/

void MapBlockBspTree::buildTree(const std::vector<MeshTriangle> *triangles)
{
	this->triangles = triangles;

	nodes.clear();

	// assert that triangle index can fit into s32
	assert(triangles->size() <= 0x7FFFFFFFL);
	std::vector<s32> indexes;
	indexes.reserve(triangles->size());
	for (u32 i = 0; i < triangles->size(); i++)
		indexes.push_back(i);

	if (!indexes.empty()) {
		// Start in the center of the block with increment of one quarter in each direction
		root = buildTree(v3f(1, 0, 0), v3f((MAP_BLOCKSIZE + 1) * 0.5f * BS), MAP_BLOCKSIZE * 0.25f * BS, indexes, 0);
	} else {
		root = -1;
	}
}

/**
 * @brief Find a candidate plane to split a set of triangles in two
 *
 * The candidate plane is represented by one of the triangles from the set.
 *
 * @param list Vector of indexes of the triangles in the set
 * @param triangles Vector of all triangles in the BSP tree
 * @return Address of the triangle that represents the proposed split plane
 */
static const MeshTriangle *findSplitCandidate(const std::vector<s32> &list, const std::vector<MeshTriangle> &triangles)
{
	// find the center of the cluster.
	v3f center(0, 0, 0);
	size_t n = list.size();
	for (s32 i : list) {
		center += triangles[i].centroid / n;
	}

	// find the triangle with the largest area and closest to the center
	const MeshTriangle *candidate_triangle = &triangles[list[0]];
	const MeshTriangle *ith_triangle;
	for (s32 i : list) {
		ith_triangle = &triangles[i];
		if (ith_triangle->areaSQ > candidate_triangle->areaSQ ||
				(ith_triangle->areaSQ == candidate_triangle->areaSQ &&
				ith_triangle->centroid.getDistanceFromSQ(center) < candidate_triangle->centroid.getDistanceFromSQ(center))) {
			candidate_triangle = ith_triangle;
		}
	}
	return candidate_triangle;
}

s32 MapBlockBspTree::buildTree(v3f normal, v3f origin, float delta, const std::vector<s32> &list, u32 depth)
{
	// if the list is empty, don't bother
	if (list.empty())
		return -1;

	// if there is only one triangle, or the delta is insanely small, this is a leaf node
	if (list.size() == 1 || delta < 0.01) {
		nodes.emplace_back(normal, origin, list, -1, -1);
		return nodes.size() - 1;
	}

	std::vector<s32> front_list;
	std::vector<s32> back_list;
	std::vector<s32> node_list;

	// split the list
	for (s32 i : list) {
		const MeshTriangle &triangle = (*triangles)[i];
		float factor = normal.dotProduct(triangle.centroid - origin);
		if (factor == 0)
			node_list.push_back(i);
		else if (factor > 0)
			front_list.push_back(i);
		else
			back_list.push_back(i);
	}

	// define the new split-plane
	v3f candidate_normal(normal.Z, normal.X, normal.Y);
	float candidate_delta = delta;
	if (depth % 3 == 2)
		candidate_delta /= 2;

	s32 front_index = -1;
	s32 back_index = -1;

	if (!front_list.empty()) {
		v3f next_normal = candidate_normal;
		v3f next_origin = origin + delta * normal;
		float next_delta = candidate_delta;
		if (next_delta < 5) {
			const MeshTriangle *candidate = findSplitCandidate(front_list, *triangles);
			next_normal = candidate->getNormal();
			next_origin = candidate->centroid;
		}
		front_index = buildTree(next_normal, next_origin, next_delta, front_list, depth + 1);

		// if there are no other triangles, don't create a new node
		if (back_list.empty() && node_list.empty())
			return front_index;
	}

	if (!back_list.empty()) {
		v3f next_normal = candidate_normal;
		v3f next_origin = origin - delta * normal;
		float next_delta = candidate_delta;
		if (next_delta < 5) {
			const MeshTriangle *candidate = findSplitCandidate(back_list, *triangles);
			next_normal = candidate->getNormal();
			next_origin = candidate->centroid;
		}

		back_index = buildTree(next_normal, next_origin, next_delta, back_list, depth + 1);

		// if there are no other triangles, don't create a new node
		if (front_list.empty() && node_list.empty())
			return back_index;
	}

	nodes.emplace_back(normal, origin, node_list, front_index, back_index);

	return nodes.size() - 1;
}

void MapBlockBspTree::traverse(s32 node, v3f viewpoint, std::vector<s32> &output) const
{
	if (node < 0) return; // recursion break;

	const TreeNode &n = nodes[node];
	float factor = n.normal.dotProduct(viewpoint - n.origin);

	if (factor > 0)
		traverse(n.back_ref, viewpoint, output);
	else
		traverse(n.front_ref, viewpoint, output);

	if (factor != 0)
		for (s32 i : n.triangle_refs)
			output.push_back(i);

	if (factor > 0)
		traverse(n.front_ref, viewpoint, output);
	else
		traverse(n.back_ref, viewpoint, output);
}



/*
	PartialMeshBuffer
*/

void PartialMeshBuffer::beforeDraw() const
{
	// Patch the indexes in the mesh buffer before draw
	m_buffer->Indices = std::move(m_vertex_indexes);
	m_buffer->setDirty(scene::EBT_INDEX);
}

void PartialMeshBuffer::afterDraw() const
{
	// Take the data back
	m_vertex_indexes = m_buffer->Indices.steal();
}

/*
	MapBlockMesh
*/

MapBlockMesh::MapBlockMesh(MeshMakeData *data, v3s16 camera_offset):
	m_minimap_mapblock(NULL),
	m_tsrc(data->m_client->getTextureSource()),
	m_shdrsrc(data->m_client->getShaderSource()),
	m_animation_force_timer(0), // force initial animation
	m_last_crack(-1),
	m_last_daynight_ratio((u32) -1)
{
	for (auto &m : m_mesh)
		m = new scene::SMesh();
	m_enable_shaders = data->m_use_shaders;
	m_enable_vbo = g_settings->getBool("enable_vbo");

	if (data->m_client->getMinimap()) {
		m_minimap_mapblock = new MinimapMapblock;
		m_minimap_mapblock->getMinimapNodes(
			&data->m_vmanip, data->m_blockpos * MAP_BLOCKSIZE);
	}

	// 4-21ms for MAP_BLOCKSIZE=16  (NOTE: probably outdated)
	// 24-155ms for MAP_BLOCKSIZE=32  (NOTE: probably outdated)
	//TimeTaker timer1("MapBlockMesh()");

	std::vector<FastFace> fastfaces_new;
	fastfaces_new.reserve(512);

	/*
		We are including the faces of the trailing edges of the block.
		This means that when something changes, the caller must
		also update the meshes of the blocks at the leading edges.

		NOTE: This is the slowest part of this method.
	*/
	{
		// 4-23ms for MAP_BLOCKSIZE=16  (NOTE: probably outdated)
		//TimeTaker timer2("updateAllFastFaceRows()");
		updateAllFastFaceRows(data, fastfaces_new);
	}
	// End of slow part

	/*
		Convert FastFaces to MeshCollector
	*/

	MeshCollector collector(m_bounding_sphere_center);

	{
		// avg 0ms (100ms spikes when loading textures the first time)
		// (NOTE: probably outdated)
		//TimeTaker timer2("MeshCollector building");

		for (const FastFace &f : fastfaces_new) {
			static const u16 indices[] = {0, 1, 2, 2, 3, 0};
			static const u16 indices_alternate[] = {0, 1, 3, 2, 3, 1};
			const u16 *indices_p =
				f.vertex_0_2_connected ? indices : indices_alternate;
			collector.append(f.tile, f.vertices, 4, indices_p, 6);
		}
	}

	/*
		Add special graphics:
		- torches
		- flowing water
		- fences
		- whatever
	*/

	{
		MapblockMeshGenerator(data, &collector,
			data->m_client->getSceneManager()->getMeshManipulator()).generate();
	}

	/*
		Convert MeshCollector to SMesh
	*/

	const bool desync_animations = g_settings->getBool(
		"desynchronize_mapblock_texture_animation");

	m_bounding_radius = std::sqrt(collector.m_bounding_radius_sq);

	for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
		for(u32 i = 0; i < collector.prebuffers[layer].size(); i++)
		{
			PreMeshBuffer &p = collector.prebuffers[layer][i];

			applyTileColor(p);

			// Generate animation data
			// - Cracks
			if (p.layer.material_flags & MATERIAL_FLAG_CRACK) {
				// Find the texture name plus ^[crack:N:
				std::ostringstream os(std::ios::binary);
				os << m_tsrc->getTextureName(p.layer.texture_id) << "^[crack";
				if (p.layer.material_flags & MATERIAL_FLAG_CRACK_OVERLAY)
					os << "o";  // use ^[cracko
				u8 tiles = p.layer.scale;
				if (tiles > 1)
					os << ":" << (u32)tiles;
				os << ":" << (u32)p.layer.animation_frame_count << ":";
				m_crack_materials.insert(std::make_pair(
						std::pair<u8, u32>(layer, i), os.str()));
				// Replace tile texture with the cracked one
				p.layer.texture = m_tsrc->getTextureForMesh(
						os.str() + "0",
						&p.layer.texture_id);
			}
			// - Texture animation
			if (p.layer.material_flags & MATERIAL_FLAG_ANIMATION) {
				// Add to MapBlockMesh in order to animate these tiles
				auto &info = m_animation_info[{layer, i}];
				info.tile = p.layer;
				info.frame = 0;
				if (desync_animations) {
					// Get starting position from noise
					info.frame_offset =
							100000 * (2.0 + noise3d(
							data->m_blockpos.X, data->m_blockpos.Y,
							data->m_blockpos.Z, 0));
				} else {
					// Play all synchronized
					info.frame_offset = 0;
				}
				// Replace tile texture with the first animation frame
				p.layer.texture = (*p.layer.frames)[0].texture;
			}

			if (!m_enable_shaders) {
				// Extract colors for day-night animation
				// Dummy sunlight to handle non-sunlit areas
				video::SColorf sunlight;
				get_sunlight_color(&sunlight, 0);

				std::map<u32, video::SColor> colors;
				const u32 vertex_count = p.vertices.size();
				for (u32 j = 0; j < vertex_count; j++) {
					video::SColor *vc = &p.vertices[j].Color;
					video::SColor copy = *vc;
					if (vc->getAlpha() == 0) // No sunlight - no need to animate
						final_color_blend(vc, copy, sunlight); // Finalize color
					else // Record color to animate
						colors[j] = copy;

					// The sunlight ratio has been stored,
					// delete alpha (for the final rendering).
					vc->setAlpha(255);
				}
				if (!colors.empty())
					m_daynight_diffs[{layer, i}] = std::move(colors);
			}

			// Create material
			video::SMaterial material;
			material.setFlag(video::EMF_LIGHTING, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_FOG_ENABLE, true);
			material.setTexture(0, p.layer.texture);

			if (m_enable_shaders) {
				material.MaterialType = m_shdrsrc->getShaderInfo(
						p.layer.shader_id).material;
				p.layer.applyMaterialOptionsWithShaders(material);
				if (p.layer.normal_texture)
					material.setTexture(1, p.layer.normal_texture);
				material.setTexture(2, p.layer.flags_texture);
			} else {
				p.layer.applyMaterialOptions(material);
			}

			scene::SMesh *mesh = (scene::SMesh *)m_mesh[layer];

			scene::SMeshBuffer *buf = new scene::SMeshBuffer();
			buf->Material = material;
			if (p.layer.isTransparent()) {
				buf->append(&p.vertices[0], p.vertices.size(), nullptr, 0);

				MeshTriangle t;
				t.buffer = buf;
				m_transparent_triangles.reserve(p.indices.size() / 3);
				for (u32 i = 0; i < p.indices.size(); i += 3) {
					t.p1 = p.indices[i];
					t.p2 = p.indices[i + 1];
					t.p3 = p.indices[i + 2];
					t.updateAttributes();
					m_transparent_triangles.push_back(t);
				}
			} else {
				buf->append(&p.vertices[0], p.vertices.size(),
					&p.indices[0], p.indices.size());
			}
			mesh->addMeshBuffer(buf);
			buf->drop();
		}

		if (m_mesh[layer]) {
			// Use VBO for mesh (this just would set this for ever buffer)
			if (m_enable_vbo)
				m_mesh[layer]->setHardwareMappingHint(scene::EHM_STATIC);
		}
	}

	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;
	m_bsp_tree.buildTree(&m_transparent_triangles);

	// Check if animation is required for this mesh
	m_has_animation =
		!m_crack_materials.empty() ||
		!m_daynight_diffs.empty() ||
		!m_animation_info.empty();
}

MapBlockMesh::~MapBlockMesh()
{
	for (scene::IMesh *m : m_mesh) {
#if IRRLICHT_VERSION_MT_REVISION < 5
		if (m_enable_vbo) {
			for (u32 i = 0; i < m->getMeshBufferCount(); i++) {
				scene::IMeshBuffer *buf = m->getMeshBuffer(i);
				RenderingEngine::get_video_driver()->removeHardwareBuffer(buf);
			}
		}
#endif
		m->drop();
	}
	delete m_minimap_mapblock;
}

bool MapBlockMesh::animate(bool faraway, float time, int crack,
	u32 daynight_ratio)
{
	if (!m_has_animation) {
		m_animation_force_timer = 100000;
		return false;
	}

	m_animation_force_timer = myrand_range(5, 100);

	// Cracks
	if (crack != m_last_crack) {
		for (auto &crack_material : m_crack_materials) {
			scene::IMeshBuffer *buf = m_mesh[crack_material.first.first]->
				getMeshBuffer(crack_material.first.second);

			// Create new texture name from original
			std::string s = crack_material.second + itos(crack);
			u32 new_texture_id = 0;
			video::ITexture *new_texture =
					m_tsrc->getTextureForMesh(s, &new_texture_id);
			buf->getMaterial().setTexture(0, new_texture);

			// If the current material is also animated, update animation info
			auto anim_it = m_animation_info.find(crack_material.first);
			if (anim_it != m_animation_info.end()) {
				TileLayer &tile = anim_it->second.tile;
				tile.texture = new_texture;
				tile.texture_id = new_texture_id;
				// force animation update
				anim_it->second.frame = -1;
			}
		}

		m_last_crack = crack;
	}

	// Texture animation
	for (auto &it : m_animation_info) {
		const TileLayer &tile = it.second.tile;
		// Figure out current frame
		int frameno = (int)(time * 1000 / tile.animation_frame_length_ms
				+ it.second.frame_offset) % tile.animation_frame_count;
		// If frame doesn't change, skip
		if (frameno == it.second.frame)
			continue;

		it.second.frame = frameno;

		scene::IMeshBuffer *buf = m_mesh[it.first.first]->getMeshBuffer(it.first.second);

		const FrameSpec &frame = (*tile.frames)[frameno];
		buf->getMaterial().setTexture(0, frame.texture);
		if (m_enable_shaders) {
			if (frame.normal_texture)
				buf->getMaterial().setTexture(1, frame.normal_texture);
			buf->getMaterial().setTexture(2, frame.flags_texture);
		}
	}

	// Day-night transition
	if (!m_enable_shaders && (daynight_ratio != m_last_daynight_ratio)) {
		// Force reload mesh to VBO
		if (m_enable_vbo)
			for (scene::IMesh *m : m_mesh)
				m->setDirty();
		video::SColorf day_color;
		get_sunlight_color(&day_color, daynight_ratio);

		for (auto &daynight_diff : m_daynight_diffs) {
			scene::IMeshBuffer *buf = m_mesh[daynight_diff.first.first]->
				getMeshBuffer(daynight_diff.first.second);
			video::S3DVertex *vertices = (video::S3DVertex *)buf->getVertices();
			for (const auto &j : daynight_diff.second)
				final_color_blend(&(vertices[j.first].Color), j.second,
						day_color);
		}
		m_last_daynight_ratio = daynight_ratio;
	}

	return true;
}

void MapBlockMesh::updateTransparentBuffers(v3f camera_pos, v3s16 block_pos)
{
	// nothing to do if the entire block is opaque
	if (m_transparent_triangles.empty())
		return;

	v3f block_posf = intToFloat(block_pos * MAP_BLOCKSIZE, BS);
	v3f rel_camera_pos = camera_pos - block_posf;

	std::vector<s32> triangle_refs;
	m_bsp_tree.traverse(rel_camera_pos, triangle_refs);

	// arrange index sequences into partial buffers
	m_transparent_buffers.clear();

	scene::SMeshBuffer *current_buffer = nullptr;
	std::vector<u16> current_strain;
	for (auto i : triangle_refs) {
		const auto &t = m_transparent_triangles[i];
		if (current_buffer != t.buffer) {
			if (current_buffer) {
				m_transparent_buffers.emplace_back(current_buffer, std::move(current_strain));
				current_strain.clear();
			}
			current_buffer = t.buffer;
		}
		current_strain.push_back(t.p1);
		current_strain.push_back(t.p2);
		current_strain.push_back(t.p3);
	}

	if (!current_strain.empty())
		m_transparent_buffers.emplace_back(current_buffer, std::move(current_strain));
}

void MapBlockMesh::consolidateTransparentBuffers()
{
	m_transparent_buffers.clear();

	scene::SMeshBuffer *current_buffer = nullptr;
	std::vector<u16> current_strain;

	// use the fact that m_transparent_triangles is already arranged by buffer
	for (const auto &t : m_transparent_triangles) {
		if (current_buffer != t.buffer) {
			if (current_buffer != nullptr) {
				this->m_transparent_buffers.emplace_back(current_buffer, std::move(current_strain));
				current_strain.clear();
			}
			current_buffer = t.buffer;
		}
		current_strain.push_back(t.p1);
		current_strain.push_back(t.p2);
		current_strain.push_back(t.p3);
	}

	if (!current_strain.empty()) {
		this->m_transparent_buffers.emplace_back(current_buffer, std::move(current_strain));
	}
}

video::SColor encode_light(u16 light, u8 emissive_light)
{
	// Get components
	u32 day = (light & 0xff);
	u32 night = (light >> 8);
	// Add emissive light
	night += emissive_light * 2.5f;
	if (night > 255)
		night = 255;
	// Since we don't know if the day light is sunlight or
	// artificial light, assume it is artificial when the night
	// light bank is also lit.
	if (day < night)
		day = 0;
	else
		day = day - night;
	u32 sum = day + night;
	// Ratio of sunlight:
	u32 r;
	if (sum > 0)
		r = day * 255 / sum;
	else
		r = 0;
	// Average light:
	float b = (day + night) / 2;
	return video::SColor(r, b, b, b);
}

u8 get_solid_sides(MeshMakeData *data)
{
	v3s16 blockpos_nodes = data->m_blockpos * MAP_BLOCKSIZE;
	const NodeDefManager *ndef = data->m_client->ndef();

	u8 result = 0x3F; // all sides solid;

	for (s16 i = 0; i < MAP_BLOCKSIZE && result != 0; i++)
	for (s16 j = 0; j < MAP_BLOCKSIZE && result != 0; j++) {
		v3s16 positions[6] = {
			v3s16(0, i, j),
			v3s16(MAP_BLOCKSIZE - 1, i, j),
			v3s16(i, 0, j),
			v3s16(i, MAP_BLOCKSIZE - 1, j),
			v3s16(i, j, 0),
			v3s16(i, j, MAP_BLOCKSIZE - 1)
		};

		for (u8 k = 0; k < 6; k++) {
			const MapNode &top = data->m_vmanip.getNodeRefUnsafe(blockpos_nodes + positions[k]);
			if (ndef->get(top).solidness != 2)
				result &= ~(1 << k);
		}
	}

	return result;
}
