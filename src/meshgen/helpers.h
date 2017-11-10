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

#pragma once
#include "client/tile.h"
#include "irrlichttypes.h"
#include "mapnode.h"

struct MeshMakeData;
class INodeDefManager;

/*!
 * Encodes light of a node.
 * The result is not the final color, but a
 * half-baked vertex color.
 * You have to multiply the resulting color
 * with the node's color.
 *
 * \param light the first 8 bits are day light,
 * the last 8 bits are night light
 * \param emissive_light amount of light the surface emits,
 * from 0 to LIGHT_SUN.
 */
video::SColor encode_light(u16 light, u8 emissive_light);

// Compute light at node
u16 getInteriorLight(MapNode n, s32 increment, INodeDefManager *ndef);
u16 getFaceLight(MapNode n, MapNode n2, v3s16 face_dir, INodeDefManager *ndef);
u16 getSmoothLightSolid(const v3s16 &p, const v3s16 &face_dir, const v3s16 &corner, MeshMakeData *data);
u16 getSmoothLightTransparent(const v3s16 &p, const v3s16 &corner, MeshMakeData *data);

/*!
 * Returns the sunlight's color from the current
 * day-night ratio.
 */
void get_sunlight_color(video::SColorf *sunlight, u32 daynight_ratio);

/*!
 * Gives the final  SColor shown on screen.
 *
 * \param result output color
 * \param light first 8 bits are day light, second 8 bits are
 * night light
 */
void final_color_blend(video::SColor *result,
		u16 light, u32 daynight_ratio);

/*!
 * Gives the final  SColor shown on screen.
 *
 * \param result output color
 * \param data the half-baked vertex color
 * \param dayLight color of the sunlight
 */
void final_color_blend(video::SColor *result,
		const video::SColor &data, const video::SColorf &dayLight);

// Retrieves the TileSpec of a face of a node
// Adds MATERIAL_FLAG_CRACK if the node is cracked
// TileSpec should be passed as reference due to the underlying TileFrame and its vector
// TileFrame vector copy cost very much to client
void getNodeTileN(MapNode mn, v3s16 p, u8 tileindex, MeshMakeData *data, TileSpec &tile);
void getNodeTile(MapNode mn, v3s16 p, v3s16 dir, MeshMakeData *data, TileSpec &tile);
