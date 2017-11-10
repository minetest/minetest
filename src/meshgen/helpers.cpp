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

#include "helpers.h"
#include <array>
#include "client.h"
#include "mesh.h"
#include "params.h"
#include "settings.h"
#include "util/directiontables.h"

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
static u16 getSmoothLightCombined(const v3s16 &p,
	const std::array<v3s16,8> &dirs, MeshMakeData *data, bool node_solid)
{
	INodeDefManager *ndef = data->m_client->ndef();

	u16 ambient_occlusion = 0;
	u16 light_count = 0;
	u8 light_source_max = 0;
	u16 light_day = 0;
	u16 light_night = 0;

	auto add_node = [&] (int i) -> const ContentFeatures& {
		MapNode n = data->m_vmanip.getNodeNoExNoEmerge(p + dirs[i]);
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
		return f;
	};

	if (node_solid) {
		ambient_occlusion = 3;
		bool corner_obstructed = true;
		for (int i = 0; i < 2; ++i) {
			if (add_node(i).light_propagates)
				corner_obstructed = false;
		}
		add_node(2);
		add_node(3);
		if (corner_obstructed)
			ambient_occlusion++;
		else
			add_node(4);
	} else {
		std::array<bool, 4> obstructed = {{ 1, 1, 1, 1 }};
		add_node(0);
		bool opaque1 = !add_node(1).light_propagates;
		bool opaque2 = !add_node(2).light_propagates;
		bool opaque3 = !add_node(3).light_propagates;
		obstructed[0] = opaque1 && opaque2;
		obstructed[1] = opaque1 && opaque3;
		obstructed[2] = opaque2 && opaque3;
		for (int k = 0; k < 4; ++k) {
			if (obstructed[k])
				ambient_occlusion++;
			else if (add_node(k + 4).light_propagates)
				obstructed[3] = false;
		}
	}

	if (light_count == 0) {
		light_day = light_night = 0;
	} else {
		light_day /= light_count;
		light_night /= light_count;
	}

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
		static const float light_amount[3] = {
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
	v3s16 neighbor_offset1, neighbor_offset2;

	/*
	 * face_dir, neighbor_offset1 and neighbor_offset2 define an
	 * orthonormal basis which is used to define the offsets of the 8
	 * surrounding nodes and to differentiate the "distance" (by going only
	 * along directly neighboring nodes) relative to the node at p.
	 * Apart from the node at p, only the 4 nodes which contain face_dir
	 * can contribute light.
	 */
	if (face_dir.X != 0) {
		neighbor_offset1 = v3s16(0, corner.Y, 0);
		neighbor_offset2 = v3s16(0, 0, corner.Z);
	} else if (face_dir.Y != 0) {
		neighbor_offset1 = v3s16(0, 0, corner.Z);
		neighbor_offset2 = v3s16(corner.X, 0, 0);
	} else if (face_dir.Z != 0) {
		neighbor_offset1 = v3s16(corner.X,0,0);
		neighbor_offset2 = v3s16(0,corner.Y,0);
	}

	const std::array<v3s16,8> dirs = {{
		// Always shine light
		neighbor_offset1 + face_dir,
		neighbor_offset2 + face_dir,
		v3s16(0,0,0),
		face_dir,

		// Can be obstructed
		neighbor_offset1 + neighbor_offset2 + face_dir,

		// Do not shine light, only for ambient occlusion
		neighbor_offset1,
		neighbor_offset2,
		neighbor_offset1 + neighbor_offset2
	}};
	return getSmoothLightCombined(p, dirs, data, true);
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
	return getSmoothLightCombined(p, dirs, data, false);
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
	Gets nth node tile (0 <= n <= 5).
*/
void getNodeTileN(MapNode mn, v3s16 p, u8 tileindex, MeshMakeData *data, TileSpec &tile)
{
	INodeDefManager *ndef = data->m_client->ndef();
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
	u8 dir_i = ((dir.X + 2 * dir.Y + 3 * dir.Z) & 7) * 2;

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
	u16 tile_index = facedir * 16 + dir_i;
	getNodeTileN(mn, p, dir_to_tile[tile_index], data, tile);
	tile.rotation = tile.world_aligned ? 0 : dir_to_tile[tile_index + 1];
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
