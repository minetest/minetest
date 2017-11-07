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

#include "solid.h"
#include "client.h"
#include "collector.h"
#include "helpers.h"
#include "mesh.h"
#include "mesh.h"
#include "params.h"
#include "profiler.h"

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
	if (dir == v3s16(0, 0, 1)) {
		// If looking towards z+, this is the face that is behind
		// the center point, facing towards z+.
		vertex_dirs[0] = v3s16(-1,-1, 1);
		vertex_dirs[1] = v3s16( 1,-1, 1);
		vertex_dirs[2] = v3s16( 1, 1, 1);
		vertex_dirs[3] = v3s16(-1, 1, 1);
	} else if (dir == v3s16(0, 0, -1)) {
		// faces towards Z-
		vertex_dirs[0] = v3s16( 1,-1,-1);
		vertex_dirs[1] = v3s16(-1,-1,-1);
		vertex_dirs[2] = v3s16(-1, 1,-1);
		vertex_dirs[3] = v3s16( 1, 1,-1);
	} else if (dir == v3s16(1, 0, 0)) {
		// faces towards X+
		vertex_dirs[0] = v3s16( 1,-1, 1);
		vertex_dirs[1] = v3s16( 1,-1,-1);
		vertex_dirs[2] = v3s16( 1, 1,-1);
		vertex_dirs[3] = v3s16( 1, 1, 1);
	} else if (dir == v3s16(-1, 0, 0)) {
		// faces towards X-
		vertex_dirs[0] = v3s16(-1,-1,-1);
		vertex_dirs[1] = v3s16(-1,-1, 1);
		vertex_dirs[2] = v3s16(-1, 1, 1);
		vertex_dirs[3] = v3s16(-1, 1,-1);
	} else if (dir == v3s16(0, 1, 0)) {
		// faces towards Y+ (assume Z- as "down" in texture)
		vertex_dirs[0] = v3s16( 1, 1,-1);
		vertex_dirs[1] = v3s16(-1, 1,-1);
		vertex_dirs[2] = v3s16(-1, 1, 1);
		vertex_dirs[3] = v3s16( 1, 1, 1);
	} else if (dir == v3s16(0, -1, 0)) {
		// faces towards Y- (assume Z+ as "down" in texture)
		vertex_dirs[0] = v3s16( 1,-1, 1);
		vertex_dirs[1] = v3s16(-1,-1, 1);
		vertex_dirs[2] = v3s16(-1,-1,-1);
		vertex_dirs[3] = v3s16( 1,-1,-1);
	}
}

static void getNodeTextureCoords(v3f base, const v3f &scale, v3s16 dir, float *u, float *v)
{
	if (dir.X > 0 || dir.Y > 0 || dir.Z < 0)
		base -= scale;
	if (dir == v3s16(0,0,1)) {
		*u = -base.X - 1;
		*v = -base.Y - 1;
	} else if (dir == v3s16(0,0,-1)) {
		*u = base.X + 1;
		*v = -base.Y - 2;
	} else if (dir == v3s16(1,0,0)) {
		*u = base.Z + 1;
		*v = -base.Y - 2;
	} else if (dir == v3s16(-1,0,0)) {
		*u = -base.Z - 1;
		*v = -base.Y - 1;
	} else if (dir == v3s16(0,1,0)) {
		*u = base.X + 1;
		*v = -base.Z - 2;
	} else if (dir == v3s16(0,-1,0)) {
		*u = base.X;
		*v = base.Z;
	}
}

static void makeFastFace(const TileSpec &tile, u16 li0, u16 li1, u16 li2, u16 li3,
	v3f tp, v3f p, v3s16 dir, v3f scale, std::vector<FastFace> &dest)
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

	for (int layernum = 0; layernum < MAX_TILE_LAYERS; layernum++) {
		const TileLayer *layer = &tile.layers[layernum];
		if (layer->texture_id == 0)
			continue;

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

		face.layer = *layer;
		face.layernum = layernum;

		face.world_aligned = tile.world_aligned;
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

	if (m1 == m2 || m1 == CONTENT_IGNORE || m2 == CONTENT_IGNORE)
		return 0;

	const ContentFeatures &f1 = ndef->get(m1);
	const ContentFeatures &f2 = ndef->get(m2);

	// Contents don't differ for different forms of same liquid
	if (f1.sameLiquid(f2))
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
		if (f1.isLiquid())
			return 1;
		if (f2.isLiquid())
			return 2;
	}

	if (c1 > c2)
		return 1;

	return 2;
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
	v3s16 p = startpos;

	u16 continuous_tiles_count = 1;

	bool makes_face = false;
	v3s16 p_corrected;
	v3s16 face_dir_corrected;
	u16 lights[4] = {0, 0, 0, 0};
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
		u16 next_lights[4] = {0, 0, 0, 0};

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

				g_profiler->avg("Meshgen: faces drawn by tiling", 0);
				for (int i = 1; i < continuous_tiles_count; i++)
					g_profiler->avg("Meshgen: faces drawn by tiling", 1);
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

void updateAllFastFaceRows(MeshMakeData *data,
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
