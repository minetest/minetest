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
#include "light.h"
#include "mapblock.h"
#include "map.h"
#include "profiler.h"
#include "nodedef.h"
#include "mesh.h"
#include "minimap.h"
#include "content_mapblock.h"
#include "noise.h"
#include "shader.h"
#include "settings.h"
#include "util/directiontables.h"
#include <IMeshManipulator.h>

/*
	MeshMakeData
*/

MeshMakeData::MeshMakeData(Client *client, bool use_shaders,
		bool use_tangent_vertices):
	m_vmanip(),
	m_blockpos(-1337,-1337,-1337),
	m_crack_pos_relative(-1337, -1337, -1337),
	m_smooth_lighting(false),
	m_show_hud(false),
	m_client(client),
	m_use_shaders(use_shaders),
	m_use_tangent_vertices(use_tangent_vertices)
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

	// Get map for reading neigbhor blocks
	Map *map = block->getParent();

	for (u16 i=0; i<26; i++) {
		const v3s16 &dir = g_26dirs[i];
		v3s16 bp = m_blockpos + dir;
		MapBlock *b = map->getBlockNoCreateNoEx(bp);
		if(b)
			fillBlockData(dir, b->getData());
	}
}

void MeshMakeData::fillSingleNode(MapNode *node)
{
	m_blockpos = v3s16(0,0,0);

	v3s16 blockpos_nodes = v3s16(0,0,0);
	VoxelArea area(blockpos_nodes-v3s16(1,1,1)*MAP_BLOCKSIZE,
			blockpos_nodes+v3s16(1,1,1)*MAP_BLOCKSIZE*2-v3s16(1,1,1));
	s32 volume = area.getVolume();
	s32 our_node_index = area.index(1,1,1);

	// Allocate this block + neighbors
	m_vmanip.clear();
	m_vmanip.addArea(area);

	// Fill in data
	MapNode *data = new MapNode[volume];
	for(s32 i = 0; i < volume; i++)
	{
		if(i == our_node_index)
		{
			data[i] = *node;
		}
		else
		{
			data[i] = MapNode(CONTENT_AIR, LIGHT_MAX, 0);
		}
	}
	m_vmanip.copyFrom(data, area, area.MinEdge, area.MinEdge, area.getExtent());
	delete[] data;
}

void MeshMakeData::setCrack(int crack_level, v3s16 crack_pos)
{
	if(crack_level >= 0)
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
		INodeDefManager *ndef)
{
	u8 light = n.getLight(bank, ndef);

	while(increment > 0)
	{
		light = undiminish_light(light);
		--increment;
	}
	while(increment < 0)
	{
		light = diminish_light(light);
		++increment;
	}

	return decode_light(light);
}

/*
	Calculate non-smooth lighting at interior of node.
	Both light banks.
*/
u16 getInteriorLight(MapNode n, s32 increment, INodeDefManager *ndef)
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
		v3s16 face_dir, INodeDefManager *ndef)
{
	u8 light;
	u8 l1 = n.getLight(bank, ndef);
	u8 l2 = n2.getLight(bank, ndef);
	if(l1 > l2)
		light = l1;
	else
		light = l2;

	// Boost light level for light sources
	u8 light_source = MYMAX(ndef->get(n).light_source,
			ndef->get(n2).light_source);
	if(light_source > light)
		light = light_source;

	return decode_light(light);
}

/*
	Calculate non-smooth lighting at face of node.
	Both light banks.
*/
u16 getFaceLight(MapNode n, MapNode n2, v3s16 face_dir, INodeDefManager *ndef)
{
	u16 day = getFaceLight(LIGHTBANK_DAY, n, n2, face_dir, ndef);
	u16 night = getFaceLight(LIGHTBANK_NIGHT, n, n2, face_dir, ndef);
	return day | (night << 8);
}

/*
	Calculate smooth lighting at the XYZ- corner of p.
	Both light banks
*/
static u16 getSmoothLightCombined(v3s16 p, MeshMakeData *data)
{
	static const v3s16 dirs8[8] = {
		v3s16(0,0,0),
		v3s16(0,0,1),
		v3s16(0,1,0),
		v3s16(0,1,1),
		v3s16(1,0,0),
		v3s16(1,1,0),
		v3s16(1,0,1),
		v3s16(1,1,1),
	};

	INodeDefManager *ndef = data->m_client->ndef();

	u16 ambient_occlusion = 0;
	u16 light_count = 0;
	u8 light_source_max = 0;
	u16 light_day = 0;
	u16 light_night = 0;

	for (u32 i = 0; i < 8; i++)
	{
		MapNode n = data->m_vmanip.getNodeNoExNoEmerge(p - dirs8[i]);

		// if it's CONTENT_IGNORE we can't do any light calculations
		if (n.getContent() == CONTENT_IGNORE) {
			continue;
		}

		const ContentFeatures &f = ndef->get(n);
		if (f.light_source > light_source_max)
			light_source_max = f.light_source;
		// Check f.solidness because fast-style leaves look better this way
		if (f.param_type == CPT_LIGHT && f.solidness != 2) {
			light_day += decode_light(n.getLightNoChecks(LIGHTBANK_DAY, &f));
			light_night += decode_light(n.getLightNoChecks(LIGHTBANK_NIGHT, &f));
			light_count++;
		} else {
			ambient_occlusion++;
		}
	}

	if(light_count == 0)
		return 0xffff;

	light_day /= light_count;
	light_night /= light_count;

	// Boost brightness around light sources
	bool skip_ambient_occlusion_day = false;
	if(decode_light(light_source_max) >= light_day) {
		light_day = decode_light(light_source_max);
		skip_ambient_occlusion_day = true;
	}

	bool skip_ambient_occlusion_night = false;
	if(decode_light(light_source_max) >= light_night) {
		light_night = decode_light(light_source_max);
		skip_ambient_occlusion_night = true;
	}

	if (ambient_occlusion > 4)
	{
		static const float ao_gamma = rangelim(
			g_settings->getFloat("ambient_occlusion_gamma"), 0.25, 4.0);

		// Table of gamma space multiply factors.
		static const float light_amount[3] = {
			powf(0.75, 1.0 / ao_gamma),
			powf(0.5,  1.0 / ao_gamma),
			powf(0.25, 1.0 / ao_gamma)
		};

		//calculate table index for gamma space multiplier
		ambient_occlusion -= 5;

		if (!skip_ambient_occlusion_day)
			light_day = rangelim(core::round32(light_day*light_amount[ambient_occlusion]), 0, 255);
		if (!skip_ambient_occlusion_night)
			light_night = rangelim(core::round32(light_night*light_amount[ambient_occlusion]), 0, 255);
	}

	return light_day | (light_night << 8);
}

/*
	Calculate smooth lighting at the given corner of p.
	Both light banks.
*/
u16 getSmoothLight(v3s16 p, v3s16 corner, MeshMakeData *data)
{
	if(corner.X == 1) p.X += 1;
	// else corner.X == -1
	if(corner.Y == 1) p.Y += 1;
	// else corner.Y == -1
	if(corner.Z == 1) p.Z += 1;
	// else corner.Z == -1

	return getSmoothLightCombined(p, data);
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

/*
	vertex_dirs: v3s16[4]
*/
static void getNodeVertexDirs(v3s16 dir, v3s16 *vertex_dirs)
{
	/*
		If looked from outside the node towards the face, the corners are:
		0: bottom-right
		1: bottom-left
		2: top-left
		3: top-right
	*/
	if(dir == v3s16(0,0,1))
	{
		// If looking towards z+, this is the face that is behind
		// the center point, facing towards z+.
		vertex_dirs[0] = v3s16(-1,-1, 1);
		vertex_dirs[1] = v3s16( 1,-1, 1);
		vertex_dirs[2] = v3s16( 1, 1, 1);
		vertex_dirs[3] = v3s16(-1, 1, 1);
	}
	else if(dir == v3s16(0,0,-1))
	{
		// faces towards Z-
		vertex_dirs[0] = v3s16( 1,-1,-1);
		vertex_dirs[1] = v3s16(-1,-1,-1);
		vertex_dirs[2] = v3s16(-1, 1,-1);
		vertex_dirs[3] = v3s16( 1, 1,-1);
	}
	else if(dir == v3s16(1,0,0))
	{
		// faces towards X+
		vertex_dirs[0] = v3s16( 1,-1, 1);
		vertex_dirs[1] = v3s16( 1,-1,-1);
		vertex_dirs[2] = v3s16( 1, 1,-1);
		vertex_dirs[3] = v3s16( 1, 1, 1);
	}
	else if(dir == v3s16(-1,0,0))
	{
		// faces towards X-
		vertex_dirs[0] = v3s16(-1,-1,-1);
		vertex_dirs[1] = v3s16(-1,-1, 1);
		vertex_dirs[2] = v3s16(-1, 1, 1);
		vertex_dirs[3] = v3s16(-1, 1,-1);
	}
	else if(dir == v3s16(0,1,0))
	{
		// faces towards Y+ (assume Z- as "down" in texture)
		vertex_dirs[0] = v3s16( 1, 1,-1);
		vertex_dirs[1] = v3s16(-1, 1,-1);
		vertex_dirs[2] = v3s16(-1, 1, 1);
		vertex_dirs[3] = v3s16( 1, 1, 1);
	}
	else if(dir == v3s16(0,-1,0))
	{
		// faces towards Y- (assume Z+ as "down" in texture)
		vertex_dirs[0] = v3s16( 1,-1, 1);
		vertex_dirs[1] = v3s16(-1,-1, 1);
		vertex_dirs[2] = v3s16(-1,-1,-1);
		vertex_dirs[3] = v3s16( 1,-1,-1);
	}
}

struct FastFace
{
	TileLayer layer;
	video::S3DVertex vertices[4]; // Precalculated vertices
	/*!
	 * The face is divided into two triangles. If this is true,
	 * vertices 0 and 2 are connected, othervise vertices 1 and 3
	 * are connected.
	 */
	bool vertex_0_2_connected;
	u8 layernum;
};

static void makeFastFace(const TileSpec &tile, u16 li0, u16 li1, u16 li2, u16 li3,
	v3f p, v3s16 dir, v3f scale, std::vector<FastFace> &dest)
{
	// Position is at the center of the cube.
	v3f pos = p * BS;

	float x0 = 0.0;
	float y0 = 0.0;
	float w = 1.0;
	float h = 1.0;

	v3f vertex_pos[4];
	v3s16 vertex_dirs[4];
	getNodeVertexDirs(dir, vertex_dirs);

	v3s16 t;
	u16 t1;
	switch (tile.rotation)
	{
	case 0:
		break;
	case 1: //R90
		t = vertex_dirs[0];
		vertex_dirs[0] = vertex_dirs[3];
		vertex_dirs[3] = vertex_dirs[2];
		vertex_dirs[2] = vertex_dirs[1];
		vertex_dirs[1] = t;
		t1=li0;
		li0=li3;
		li3=li2;
		li2=li1;
		li1=t1;
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

	for(u16 i=0; i<4; i++)
	{
		vertex_pos[i] = v3f(
				BS/2*vertex_dirs[i].X,
				BS/2*vertex_dirs[i].Y,
				BS/2*vertex_dirs[i].Z
		);
	}

	for(u16 i=0; i<4; i++)
	{
		vertex_pos[i].X *= scale.X;
		vertex_pos[i].Y *= scale.Y;
		vertex_pos[i].Z *= scale.Z;
		vertex_pos[i] += pos;
	}

	f32 abs_scale = 1.0;
	if     (scale.X < 0.999 || scale.X > 1.001) abs_scale = scale.X;
	else if(scale.Y < 0.999 || scale.Y > 1.001) abs_scale = scale.Y;
	else if(scale.Z < 0.999 || scale.Z > 1.001) abs_scale = scale.Z;

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

	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;

		dest.push_back(FastFace());
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

		face.layer = *layer;
		face.layernum = layernum;
	}
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
		INodeDefManager *ndef)
{
	*equivalent = false;

	if(m1 == CONTENT_IGNORE || m2 == CONTENT_IGNORE)
		return 0;

	bool contents_differ = (m1 != m2);

	const ContentFeatures &f1 = ndef->get(m1);
	const ContentFeatures &f2 = ndef->get(m2);

	// Contents don't differ for different forms of same liquid
	if(f1.sameLiquid(f2))
		contents_differ = false;

	u8 c1 = f1.solidness;
	u8 c2 = f2.solidness;

	bool solidness_differs = (c1 != c2);
	bool makes_face = contents_differ && solidness_differs;

	if(makes_face == false)
		return 0;

	if(c1 == 0)
		c1 = f1.visual_solidness;
	if(c2 == 0)
		c2 = f2.visual_solidness;

	if(c1 == c2){
		*equivalent = true;
		// If same solidness, liquid takes precense
		if(f1.isLiquid())
			return 1;
		if(f2.isLiquid())
			return 2;
	}

	if(c1 > c2)
		return 1;
	else
		return 2;
}

/*
	Gets nth node tile (0 <= n <= 5).
*/
void getNodeTileN(MapNode mn, v3s16 p, u8 tileindex, MeshMakeData *data, TileSpec &tile)
{
	INodeDefManager *ndef = data->m_client->ndef();
	const ContentFeatures &f = ndef->get(mn);
	tile = f.tiles[tileindex];
	TileLayer *top_layer = NULL;
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		top_layer = layer;
		if (!layer->has_color)
			mn.getColor(f, &(layer->color));
	}
	// Apply temporary crack
	if (p == data->m_crack_pos_relative)
		top_layer->material_flags |= MATERIAL_FLAG_CRACK;
}

/*
	Gets node tile given a face direction.
*/
void getNodeTile(MapNode mn, v3s16 p, v3s16 dir, MeshMakeData *data, TileSpec &tile)
{
	INodeDefManager *ndef = data->m_client->ndef();

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
	u8 dir_i = ((dir.X + 2 * dir.Y + 3 * dir.Z) & 7)*2;

	// Get rotation for things like chests
	u8 facedir = mn.getFaceDir(ndef);

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
	u16 tile_index=facedir*16 + dir_i;
	getNodeTileN(mn, p, dir_to_tile[tile_index], data, tile);
	tile.rotation = dir_to_tile[tile_index + 1];
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
		TileSpec &tile
	)
{
	VoxelManipulator &vmanip = data->m_vmanip;
	INodeDefManager *ndef = data->m_client->ndef();
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
	tile.emissive_light = f.light_source;

	// eg. water and glass
	if (equivalent) {
		for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++)
			tile.layers[layernum].material_flags |=
				MATERIAL_FLAG_BACKFACE_CULLING;
	}

	if (!data->m_smooth_lighting) {
		lights[0] = lights[1] = lights[2] = lights[3] =
				getFaceLight(n0, n1, face_dir, ndef);
	}
	else {
		v3s16 vertex_dirs[4];
		getNodeVertexDirs(face_dir_corrected, vertex_dirs);

		v3s16 light_p = blockpos_nodes + p_corrected;
		for (u16 i = 0; i < 4; i++) {
			lights[i] = getSmoothLight(light_p, vertex_dirs[i], data);
		}
	}
}

/*
	startpos:
	translate_dir: unit vector with only one of x, y or z
	face_dir: unit vector with only one of x, y or z
*/
static void updateFastFaceRow(
		MeshMakeData *data,
		v3s16 startpos,
		v3s16 translate_dir,
		v3f translate_dir_f,
		v3s16 face_dir,
		v3f face_dir_f,
		std::vector<FastFace> &dest)
{
	v3s16 p = startpos;

	u16 continuous_tiles_count = 1;

	bool makes_face = false;
	v3s16 p_corrected;
	v3s16 face_dir_corrected;
	u16 lights[4] = {0,0,0,0};
	TileSpec tile;
	getTileInfo(data, p, face_dir,
			makes_face, p_corrected, face_dir_corrected,
			lights, tile);

	// Unroll this variable which has a significant build cost
	TileSpec next_tile;
	for (u16 j = 0; j < MAP_BLOCKSIZE; j++) {
		// If tiling can be done, this is set to false in the next step
		bool next_is_different = true;

		v3s16 p_next;

		bool next_makes_face = false;
		v3s16 next_p_corrected;
		v3s16 next_face_dir_corrected;
		u16 next_lights[4] = {0,0,0,0};


		// If at last position, there is nothing to compare to and
		// the face must be drawn anyway
		if (j != MAP_BLOCKSIZE - 1) {
			p_next = p + translate_dir;

			getTileInfo(data, p_next, face_dir,
					next_makes_face, next_p_corrected,
					next_face_dir_corrected, next_lights,
					next_tile);

			if (next_makes_face == makes_face
					&& next_p_corrected == p_corrected + translate_dir
					&& next_face_dir_corrected == face_dir_corrected
					&& memcmp(next_lights, lights, ARRLEN(lights) * sizeof(u16)) == 0
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
				v3f sp = pf -
					((f32)continuous_tiles_count / 2.0f - 0.5f) * translate_dir_f;
				v3f scale(1,1,1);

				if(translate_dir.X != 0) {
					scale.X = continuous_tiles_count;
				}
				if(translate_dir.Y != 0) {
					scale.Y = continuous_tiles_count;
				}
				if(translate_dir.Z != 0) {
					scale.Z = continuous_tiles_count;
				}

				makeFastFace(tile, lights[0], lights[1], lights[2], lights[3],
						sp, face_dir_corrected, scale, dest);

				g_profiler->avg("Meshgen: faces drawn by tiling", 0);
				for(int i = 1; i < continuous_tiles_count; i++){
					g_profiler->avg("Meshgen: faces drawn by tiling", 1);
				}
			}

			continuous_tiles_count = 1;
		}

		makes_face = next_makes_face;
		p_corrected = next_p_corrected;
		face_dir_corrected = next_face_dir_corrected;
		std::memcpy(lights, next_lights, ARRLEN(lights) * sizeof(u16));
		if (next_is_different)
			tile = next_tile;
		p = p_next;
	}
}

static void updateAllFastFaceRows(MeshMakeData *data,
		std::vector<FastFace> &dest)
{
	/*
		Go through every y,z and get top(y+) faces in rows of x+
	*/
	for(s16 y = 0; y < MAP_BLOCKSIZE; y++) {
		for(s16 z = 0; z < MAP_BLOCKSIZE; z++) {
			updateFastFaceRow(data,
					v3s16(0,y,z),
					v3s16(1,0,0), //dir
					v3f  (1,0,0),
					v3s16(0,1,0), //face dir
					v3f  (0,1,0),
					dest);
		}
	}

	/*
		Go through every x,y and get right(x+) faces in rows of z+
	*/
	for(s16 x = 0; x < MAP_BLOCKSIZE; x++) {
		for(s16 y = 0; y < MAP_BLOCKSIZE; y++) {
			updateFastFaceRow(data,
					v3s16(x,y,0),
					v3s16(0,0,1), //dir
					v3f  (0,0,1),
					v3s16(1,0,0), //face dir
					v3f  (1,0,0),
					dest);
		}
	}

	/*
		Go through every y,z and get back(z+) faces in rows of x+
	*/
	for(s16 z = 0; z < MAP_BLOCKSIZE; z++) {
		for(s16 y = 0; y < MAP_BLOCKSIZE; y++) {
			updateFastFaceRow(data,
					v3s16(0,y,z),
					v3s16(1,0,0), //dir
					v3f  (1,0,0),
					v3s16(0,0,1), //face dir
					v3f  (0,0,1),
					dest);
		}
	}
}

/*
	MapBlockMesh
*/

MapBlockMesh::MapBlockMesh(MeshMakeData *data, v3s16 camera_offset):
	m_minimap_mapblock(NULL),
	m_client(data->m_client),
	m_driver(m_client->tsrc()->getDevice()->getVideoDriver()),
	m_tsrc(m_client->getTextureSource()),
	m_shdrsrc(m_client->getShaderSource()),
	m_animation_force_timer(0), // force initial animation
	m_last_crack(-1),
	m_crack_materials(),
	m_last_daynight_ratio((u32) -1),
	m_daynight_diffs()
{
	for (int m = 0; m < MAX_TILE_LAYERS; m++)
		m_mesh[m] = new scene::SMesh();
	m_enable_shaders = data->m_use_shaders;
	m_use_tangent_vertices = data->m_use_tangent_vertices;
	m_enable_vbo = g_settings->getBool("enable_vbo");

	if (g_settings->getBool("enable_minimap")) {
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

	MeshCollector collector(m_use_tangent_vertices);

	{
		// avg 0ms (100ms spikes when loading textures the first time)
		// (NOTE: probably outdated)
		//TimeTaker timer2("MeshCollector building");

		for (u32 i = 0; i < fastfaces_new.size(); i++) {
			FastFace &f = fastfaces_new[i];

			const u16 indices[] = {0,1,2,2,3,0};
			const u16 indices_alternate[] = {0,1,3,2,3,1};

			if (f.layer.texture == NULL)
				continue;

			const u16 *indices_p =
				f.vertex_0_2_connected ? indices : indices_alternate;

			collector.append(f.layer, f.vertices, 4, indices_p, 6,
				f.layernum);
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
		MapblockMeshGenerator generator(data, &collector);
		generator.generate();
	}

	collector.applyTileColors();

	/*
		Convert MeshCollector to SMesh
	*/

	for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
		for(u32 i = 0; i < collector.prebuffers[layer].size(); i++)
		{
			PreMeshBuffer &p = collector.prebuffers[layer][i];

			// Generate animation data
			// - Cracks
			if(p.layer.material_flags & MATERIAL_FLAG_CRACK)
			{
				// Find the texture name plus ^[crack:N:
				std::ostringstream os(std::ios::binary);
				os<<m_tsrc->getTextureName(p.layer.texture_id)<<"^[crack";
				if(p.layer.material_flags & MATERIAL_FLAG_CRACK_OVERLAY)
					os<<"o";  // use ^[cracko
				os<<":"<<(u32)p.layer.animation_frame_count<<":";
				m_crack_materials.insert(std::make_pair(std::pair<u8, u32>(layer, i), os.str()));
				// Replace tile texture with the cracked one
				p.layer.texture = m_tsrc->getTextureForMesh(
						os.str()+"0",
						&p.layer.texture_id);
			}
			// - Texture animation
			if (p.layer.material_flags & MATERIAL_FLAG_ANIMATION) {
				// Add to MapBlockMesh in order to animate these tiles
				m_animation_tiles[std::pair<u8, u32>(layer, i)] = p.layer;
				m_animation_frames[std::pair<u8, u32>(layer, i)] = 0;
				if(g_settings->getBool("desynchronize_mapblock_texture_animation")){
					// Get starting position from noise
					m_animation_frame_offsets[std::pair<u8, u32>(layer, i)] = 100000 * (2.0 + noise3d(
							data->m_blockpos.X, data->m_blockpos.Y,
							data->m_blockpos.Z, 0));
				} else {
					// Play all synchronized
					m_animation_frame_offsets[std::pair<u8, u32>(layer, i)] = 0;
				}
				// Replace tile texture with the first animation frame
				p.layer.texture = p.layer.frames[0].texture;
			}

			if (!m_enable_shaders) {
				// Extract colors for day-night animation
				// Dummy sunlight to handle non-sunlit areas
				video::SColorf sunlight;
				get_sunlight_color(&sunlight, 0);
				u32 vertex_count =
					m_use_tangent_vertices ?
						p.tangent_vertices.size() : p.vertices.size();
				for (u32 j = 0; j < vertex_count; j++) {
					video::SColor *vc;
					if (m_use_tangent_vertices) {
						vc = &p.tangent_vertices[j].Color;
					} else {
						vc = &p.vertices[j].Color;
					}
					video::SColor copy(*vc);
					if (vc->getAlpha() == 0) // No sunlight - no need to animate
						final_color_blend(vc, copy, sunlight); // Finalize color
					else // Record color to animate
						m_daynight_diffs[std::pair<u8, u32>(layer, i)][j] = copy;

					// The sunlight ratio has been stored,
					// delete alpha (for the final rendering).
					vc->setAlpha(255);
				}
			}

			// Create material
			video::SMaterial material;
			material.setFlag(video::EMF_LIGHTING, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, true);
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_FOG_ENABLE, true);
			material.setTexture(0, p.layer.texture);

			if (m_enable_shaders) {
				material.MaterialType = m_shdrsrc->getShaderInfo(p.layer.shader_id).material;
				p.layer.applyMaterialOptionsWithShaders(material);
				if (p.layer.normal_texture) {
					material.setTexture(1, p.layer.normal_texture);
				}
				material.setTexture(2, p.layer.flags_texture);
			} else {
				p.layer.applyMaterialOptions(material);
			}

			scene::SMesh *mesh = (scene::SMesh *)m_mesh[layer];

			// Create meshbuffer, add to mesh
			if (m_use_tangent_vertices) {
				scene::SMeshBufferTangents *buf = new scene::SMeshBufferTangents();
				// Set material
				buf->Material = material;
				// Add to mesh
				mesh->addMeshBuffer(buf);
				// Mesh grabbed it
				buf->drop();
				buf->append(&p.tangent_vertices[0], p.tangent_vertices.size(),
					&p.indices[0], p.indices.size());
			} else {
				scene::SMeshBuffer *buf = new scene::SMeshBuffer();
				// Set material
				buf->Material = material;
				// Add to mesh
				mesh->addMeshBuffer(buf);
				// Mesh grabbed it
				buf->drop();
				buf->append(&p.vertices[0], p.vertices.size(),
					&p.indices[0], p.indices.size());
			}
		}


		/*
			Do some stuff to the mesh
		*/
		m_camera_offset = camera_offset;
		translateMesh(m_mesh[layer],
			intToFloat(data->m_blockpos * MAP_BLOCKSIZE - camera_offset, BS));

		if (m_use_tangent_vertices) {
			scene::IMeshManipulator* meshmanip =
				m_client->getSceneManager()->getMeshManipulator();
			meshmanip->recalculateTangents(m_mesh[layer], true, false, false);
		}

		if (m_mesh[layer])
		{
#if 0
			// Usually 1-700 faces and 1-7 materials
			std::cout<<"Updated MapBlock has "<<fastfaces_new.size()<<" faces "
					<<"and uses "<<m_mesh[layer]->getMeshBufferCount()
					<<" materials (meshbuffers)"<<std::endl;
#endif

			// Use VBO for mesh (this just would set this for ever buffer)
			if (m_enable_vbo) {
				m_mesh[layer]->setHardwareMappingHint(scene::EHM_STATIC);
			}
		}
	}

	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;

	// Check if animation is required for this mesh
	m_has_animation =
		!m_crack_materials.empty() ||
		!m_daynight_diffs.empty() ||
		!m_animation_tiles.empty();
}

MapBlockMesh::~MapBlockMesh()
{
	for (int m = 0; m < MAX_TILE_LAYERS; m++) {
		if (m_enable_vbo && m_mesh[m])
			for (u32 i = 0; i < m_mesh[m]->getMeshBufferCount(); i++) {
				scene::IMeshBuffer *buf = m_mesh[m]->getMeshBuffer(i);
				m_driver->removeHardwareBuffer(buf);
			}
		m_mesh[m]->drop();
		m_mesh[m] = NULL;
	}
	delete m_minimap_mapblock;
}

bool MapBlockMesh::animate(bool faraway, float time, int crack, u32 daynight_ratio)
{
	if(!m_has_animation)
	{
		m_animation_force_timer = 100000;
		return false;
	}

	m_animation_force_timer = myrand_range(5, 100);

	// Cracks
	if(crack != m_last_crack)
	{
		for (std::map<std::pair<u8, u32>, std::string>::iterator i =
				m_crack_materials.begin(); i != m_crack_materials.end(); ++i) {
			scene::IMeshBuffer *buf = m_mesh[i->first.first]->
				getMeshBuffer(i->first.second);
			std::string basename = i->second;

			// Create new texture name from original
			std::ostringstream os;
			os<<basename<<crack;
			u32 new_texture_id = 0;
			video::ITexture *new_texture =
				m_tsrc->getTextureForMesh(os.str(), &new_texture_id);
			buf->getMaterial().setTexture(0, new_texture);

			// If the current material is also animated,
			// update animation info
			std::map<std::pair<u8, u32>, TileLayer>::iterator anim_iter =
				m_animation_tiles.find(i->first);
			if (anim_iter != m_animation_tiles.end()){
				TileLayer &tile = anim_iter->second;
				tile.texture = new_texture;
				tile.texture_id = new_texture_id;
				// force animation update
				m_animation_frames[i->first] = -1;
			}
		}

		m_last_crack = crack;
	}

	// Texture animation
	for (std::map<std::pair<u8, u32>, TileLayer>::iterator i =
			m_animation_tiles.begin(); i != m_animation_tiles.end(); ++i) {
		const TileLayer &tile = i->second;
		// Figure out current frame
		int frameoffset = m_animation_frame_offsets[i->first];
		int frame = (int)(time * 1000 / tile.animation_frame_length_ms
				+ frameoffset) % tile.animation_frame_count;
		// If frame doesn't change, skip
		if(frame == m_animation_frames[i->first])
			continue;

		m_animation_frames[i->first] = frame;

		scene::IMeshBuffer *buf = m_mesh[i->first.first]->
			getMeshBuffer(i->first.second);

		const FrameSpec &animation_frame = tile.frames[frame];
		buf->getMaterial().setTexture(0, animation_frame.texture);
		if (m_enable_shaders) {
			if (animation_frame.normal_texture) {
				buf->getMaterial().setTexture(1, animation_frame.normal_texture);
			}
			buf->getMaterial().setTexture(2, animation_frame.flags_texture);
		}
	}

	// Day-night transition
	if(!m_enable_shaders && (daynight_ratio != m_last_daynight_ratio))
	{
		// Force reload mesh to VBO
		if (m_enable_vbo)
			for (int m = 0; m < MAX_TILE_LAYERS; m++)
				m_mesh[m]->setDirty();
		video::SColorf day_color;
		get_sunlight_color(&day_color, daynight_ratio);
		for(std::map<std::pair<u8, u32>, std::map<u32, video::SColor > >::iterator
				i = m_daynight_diffs.begin();
				i != m_daynight_diffs.end(); ++i)
		{
			scene::IMeshBuffer *buf = m_mesh[i->first.first]->
				getMeshBuffer(i->first.second);
			video::S3DVertex *vertices = (video::S3DVertex *)buf->getVertices();
			for(std::map<u32, video::SColor >::iterator
					j = i->second.begin();
					j != i->second.end(); ++j)
			{
				final_color_blend(&(vertices[j->first].Color),
					j->second, day_color);
			}
		}
		m_last_daynight_ratio = daynight_ratio;
	}

	return true;
}

void MapBlockMesh::updateCameraOffset(v3s16 camera_offset)
{
	if (camera_offset != m_camera_offset) {
		for (u8 layer = 0; layer < 2; layer++) {
			translateMesh(m_mesh[layer],
				intToFloat(m_camera_offset - camera_offset, BS));
			if (m_enable_vbo) {
				m_mesh[layer]->setDirty();
			}
		}
		m_camera_offset = camera_offset;
	}
}

/*
	MeshCollector
*/

void MeshCollector::append(const TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices,
			layernum);
	}
}

void MeshCollector::append(const TileLayer &layer,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices, u8 layernum)
{
	if (numIndices > 65535) {
		dstream<<"FIXME: MeshCollector::append() called with numIndices="<<numIndices<<" (limit 65535)"<<std::endl;
		return;
	}
	std::vector<PreMeshBuffer> *buffers = &prebuffers[layernum];

	PreMeshBuffer *p = NULL;
	for (u32 i = 0; i < buffers->size(); i++) {
		PreMeshBuffer &pp = (*buffers)[i];
		if (pp.layer != layer)
			continue;
		if (pp.indices.size() + numIndices > 65535)
			continue;

		p = &pp;
		break;
	}

	if (p == NULL) {
		PreMeshBuffer pp;
		pp.layer = layer;
		buffers->push_back(pp);
		p = &(*buffers)[buffers->size() - 1];
	}

	u32 vertex_count;
	if (m_use_tangent_vertices) {
		vertex_count = p->tangent_vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			video::S3DVertexTangents vert(vertices[i].Pos, vertices[i].Normal,
				vertices[i].Color, vertices[i].TCoords);
			p->tangent_vertices.push_back(vert);
		}
	} else {
		vertex_count = p->vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			video::S3DVertex vert(vertices[i].Pos, vertices[i].Normal,
				vertices[i].Color, vertices[i].TCoords);
			p->vertices.push_back(vert);
		}
	}

	for (u32 i = 0; i < numIndices; i++) {
		u32 j = indices[i] + vertex_count;
		p->indices.push_back(j);
	}
}

/*
	MeshCollector - for meshnodes and converted drawtypes.
*/

void MeshCollector::append(const TileSpec &tile,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source)
{
	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;
		append(*layer, vertices, numVertices, indices, numIndices, pos,
			c, light_source, layernum);
	}
}

void MeshCollector::append(const TileLayer &layer,
		const video::S3DVertex *vertices, u32 numVertices,
		const u16 *indices, u32 numIndices,
		v3f pos, video::SColor c, u8 light_source, u8 layernum)
{
	if (numIndices > 65535) {
		dstream<<"FIXME: MeshCollector::append() called with numIndices="<<numIndices<<" (limit 65535)"<<std::endl;
		return;
	}
	std::vector<PreMeshBuffer> *buffers = &prebuffers[layernum];

	PreMeshBuffer *p = NULL;
	for (u32 i = 0; i < buffers->size(); i++) {
		PreMeshBuffer &pp = (*buffers)[i];
		if(pp.layer != layer)
			continue;
		if(pp.indices.size() + numIndices > 65535)
			continue;

		p = &pp;
		break;
	}

	if (p == NULL) {
		PreMeshBuffer pp;
		pp.layer = layer;
		buffers->push_back(pp);
		p = &(*buffers)[buffers->size() - 1];
	}

	video::SColor original_c = c;
	u32 vertex_count;
	if (m_use_tangent_vertices) {
		vertex_count = p->tangent_vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			if (!light_source) {
				c = original_c;
				applyFacesShading(c, vertices[i].Normal);
			}
			video::S3DVertexTangents vert(vertices[i].Pos + pos,
				vertices[i].Normal, c, vertices[i].TCoords);
			p->tangent_vertices.push_back(vert);
		}
	} else {
		vertex_count = p->vertices.size();
		for (u32 i = 0; i < numVertices; i++) {
			if (!light_source) {
				c = original_c;
				applyFacesShading(c, vertices[i].Normal);
			}
			video::S3DVertex vert(vertices[i].Pos + pos, vertices[i].Normal, c,
				vertices[i].TCoords);
			p->vertices.push_back(vert);
		}
	}

	for (u32 i = 0; i < numIndices; i++) {
		u32 j = indices[i] + vertex_count;
		p->indices.push_back(j);
	}
}

void MeshCollector::applyTileColors()
{
	if (m_use_tangent_vertices)
		for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
			std::vector<PreMeshBuffer> *p = &prebuffers[layer];
			for (std::vector<PreMeshBuffer>::iterator it = p->begin();
					it != p->end(); ++it) {
				video::SColor tc = it->layer.color;
				if (tc == video::SColor(0xFFFFFFFF))
					continue;
				for (u32 index = 0; index < it->tangent_vertices.size(); index++) {
					video::SColor *c = &it->tangent_vertices[index].Color;
					c->set(c->getAlpha(), c->getRed() * tc.getRed() / 255,
						c->getGreen() * tc.getGreen() / 255,
						c->getBlue() * tc.getBlue() / 255);
				}
			}
		}
	else
		for (int layer = 0; layer < MAX_TILE_LAYERS; layer++) {
			std::vector<PreMeshBuffer> *p = &prebuffers[layer];
			for (std::vector<PreMeshBuffer>::iterator it = p->begin();
					it != p->end(); ++it) {
				video::SColor tc = it->layer.color;
				if (tc == video::SColor(0xFFFFFFFF))
					continue;
				for (u32 index = 0; index < it->vertices.size(); index++) {
					video::SColor *c = &it->vertices[index].Color;
					c->set(c->getAlpha(), c->getRed() * tc.getRed() / 255,
						c->getGreen() * tc.getGreen() / 255,
						c->getBlue() * tc.getBlue() / 255);
				}
			}
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
