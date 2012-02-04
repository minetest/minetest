/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "mapblock_mesh.h"
#include "light.h"
#include "mapblock.h"
#include "map.h"
#include "main.h" // For g_settings and g_texturesource
#include "settings.h"
#include "profiler.h"
#include "nodedef.h"
#include "gamedef.h"
#include "content_mapblock.h"

void MeshMakeData::fill(u32 daynight_ratio, MapBlock *block)
{
	m_daynight_ratio = daynight_ratio;
	m_blockpos = block->getPos();

	v3s16 blockpos_nodes = m_blockpos*MAP_BLOCKSIZE;
	
	/*
		There is no harm not copying the TempMods of the neighbors
		because they are already copied to this block
	*/
	m_temp_mods.clear();
	block->copyTempMods(m_temp_mods);
	
	/*
		Copy data
	*/

	// Allocate this block + neighbors
	m_vmanip.clear();
	m_vmanip.addArea(VoxelArea(blockpos_nodes-v3s16(1,1,1)*MAP_BLOCKSIZE,
			blockpos_nodes+v3s16(1,1,1)*MAP_BLOCKSIZE*2-v3s16(1,1,1)));

	{
		//TimeTaker timer("copy central block data");
		// 0ms

		// Copy our data
		block->copyTo(m_vmanip);
	}
	{
		//TimeTaker timer("copy neighbor block data");
		// 0ms

		/*
			Copy neighbors. This is lightning fast.
			Copying only the borders would be *very* slow.
		*/
		
		// Get map
		Map *map = block->getParent();

		for(u16 i=0; i<6; i++)
		{
			const v3s16 &dir = g_6dirs[i];
			v3s16 bp = m_blockpos + dir;
			MapBlock *b = map->getBlockNoCreateNoEx(bp);
			if(b)
				b->copyTo(m_vmanip);
		}
	}
}

void MeshMakeData::fillSingleNode(u32 daynight_ratio, MapNode *node)
{
	m_daynight_ratio = daynight_ratio;
	m_blockpos = v3s16(0,0,0);
	m_temp_mods.clear();
	
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

video::SColor MapBlock_LightColor(u8 alpha, u8 light)
{
#if 0
	return video::SColor(alpha,light,light,light);
#endif
	//return video::SColor(alpha,light,light,MYMAX(0, (s16)light-25)+25);
	/*return video::SColor(alpha,light,light,MYMAX(0,
			pow((float)light/255.0, 0.8)*255.0));*/
#if 1
	// Emphase blue a bit in darker places
	float lim = 80;
	float power = 0.8;
	if(light > lim)
		return video::SColor(alpha,light,light,light);
	else
		return video::SColor(alpha,light,light,MYMAX(0,
				pow((float)light/lim, power)*lim));
#endif
}

struct FastFace
{
	TileSpec tile;
	video::S3DVertex vertices[4]; // Precalculated vertices
};

static void makeFastFace(TileSpec tile, u8 li0, u8 li1, u8 li2, u8 li3, v3f p,
		v3s16 dir, v3f scale, v3f posRelative_f,
		core::array<FastFace> &dest)
{
	FastFace face;
	
	// Position is at the center of the cube.
	v3f pos = p * BS;
	posRelative_f *= BS;

	v3f vertex_pos[4];
	v3s16 vertex_dirs[4];
	getNodeVertexDirs(dir, vertex_dirs);
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
		vertex_pos[i] += pos + posRelative_f;
	}

	f32 abs_scale = 1.;
	if     (scale.X < 0.999 || scale.X > 1.001) abs_scale = scale.X;
	else if(scale.Y < 0.999 || scale.Y > 1.001) abs_scale = scale.Y;
	else if(scale.Z < 0.999 || scale.Z > 1.001) abs_scale = scale.Z;

	v3f normal(dir.X, dir.Y, dir.Z);

	u8 alpha = tile.alpha;
	/*u8 alpha = 255;
	if(tile.id == TILE_WATER)
		alpha = WATER_ALPHA;*/

	float x0 = tile.texture.pos.X;
	float y0 = tile.texture.pos.Y;
	float w = tile.texture.size.X;
	float h = tile.texture.size.Y;

	/*video::SColor c = MapBlock_LightColor(alpha, li);

	face.vertices[0] = video::S3DVertex(vertex_pos[0], v3f(0,1,0), c,
			core::vector2d<f32>(x0+w*abs_scale, y0+h));
	face.vertices[1] = video::S3DVertex(vertex_pos[1], v3f(0,1,0), c,
			core::vector2d<f32>(x0, y0+h));
	face.vertices[2] = video::S3DVertex(vertex_pos[2], v3f(0,1,0), c,
			core::vector2d<f32>(x0, y0));
	face.vertices[3] = video::S3DVertex(vertex_pos[3], v3f(0,1,0), c,
			core::vector2d<f32>(x0+w*abs_scale, y0));*/

	face.vertices[0] = video::S3DVertex(vertex_pos[0], normal,
			MapBlock_LightColor(alpha, li0),
			core::vector2d<f32>(x0+w*abs_scale, y0+h));
	face.vertices[1] = video::S3DVertex(vertex_pos[1], normal,
			MapBlock_LightColor(alpha, li1),
			core::vector2d<f32>(x0, y0+h));
	face.vertices[2] = video::S3DVertex(vertex_pos[2], normal,
			MapBlock_LightColor(alpha, li2),
			core::vector2d<f32>(x0, y0));
	face.vertices[3] = video::S3DVertex(vertex_pos[3], normal,
			MapBlock_LightColor(alpha, li3),
			core::vector2d<f32>(x0+w*abs_scale, y0));

	face.tile = tile;
	//DEBUG
	//f->tile = TILE_STONE;
	
	dest.push_back(face);
}
	
static TileSpec getTile(const MapNode &node, v3s16 dir, INodeDefManager *nodemgr)
{
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
	u8 dir_i = (dir.X + 2 * dir.Y + 3 * dir.Z) & 7;

	// Get rotation for things like chests
	u8 facedir = node.getFaceDir(nodemgr);
	assert(facedir <= 3);
	
	static const u8 dir_to_tile[4 * 8] =
	{
		// 0  +X  +Y  +Z   0  -Z  -Y  -X
		   0,  2,  0,  4,  0,  5,  1,  3,  // facedir = 0
		   0,  4,  0,  3,  0,  2,  1,  5,  // facedir = 1
		   0,  3,  0,  5,  0,  4,  1,  2,  // facedir = 2
		   0,  5,  0,  2,  0,  3,  1,  4,  // facedir = 3
	};

	return nodemgr->get(node).tiles[dir_to_tile[facedir*8 + dir_i]];
}

/*
	Gets node tile from any place relative to block.
	Returns TILE_NODE if doesn't exist or should not be drawn.
*/
TileSpec getNodeTile(MapNode mn, v3s16 p, v3s16 face_dir,
		NodeModMap *temp_mods, ITextureSource *tsrc, INodeDefManager *ndef)
{
	TileSpec spec;
	spec = getTile(mn, face_dir, ndef);
	
	/*
		Check temporary modifications on this node
	*/
	/*core::map<v3s16, NodeMod>::Node *n;
	n = m_temp_mods.find(p);
	// If modified
	if(n != NULL)
	{
		struct NodeMod mod = n->getValue();*/
	NodeMod mod;
	if(temp_mods && temp_mods->get(p, &mod))
	{
		#if 0  // NODEMOD_CHANGECONTENT isn't used at the moment
		if(mod.type == NODEMOD_CHANGECONTENT)
		{
			MapNode mn2(mod.param);
			spec = getTile(mn2, face_dir, ndef);
		}
		#endif
		if(mod.type == NODEMOD_CRACK)
		{
			/*
				Get texture id, translate it to name, append stuff to
				name, get texture id
			*/

			// Get original texture name
			u32 orig_id = spec.texture.id;
			std::string orig_name = tsrc->getTextureName(orig_id);

			// Create new texture name
			std::ostringstream os;
			os<<orig_name<<"^[crack"<<mod.param;

			// Get new texture
			u32 new_id = tsrc->getTextureId(os.str());
			
			/*dstream<<"MapBlock::getNodeTile(): Switching from "
					<<orig_name<<" to "<<os.str()<<" ("
					<<orig_id<<" to "<<new_id<<")"<<std::endl;*/
			
			spec.texture = tsrc->getTexture(new_id);
		}
	}
	
	return spec;
}

static content_t getNodeContent(v3s16 p, MapNode mn, NodeModMap *temp_mods)
{
	/*
		Check temporary modifications on this node
	*/
	#if 0  // NODEMOD_CHANGECONTENT isn't used at the moment
	NodeMod mod;
	if(temp_mods && temp_mods->get(p, &mod))
	{
		if(mod.type == NODEMOD_CHANGECONTENT)
		{
			// Overrides content
			return mod.param;
		}
		if(mod.type == NODEMOD_CRACK)
		{
			/*
				Content doesn't change.
				
				face_contents works just like it should, because
				there should not be faces between differently cracked
				nodes.

				If a semi-transparent node is cracked in front an
				another one, it really doesn't matter whether there
				is a cracked face drawn in between or not.
			*/
		}
	}
	#endif

	return mn.getContent();
}

v3s16 dirs8[8] = {
	v3s16(0,0,0),
	v3s16(0,0,1),
	v3s16(0,1,0),
	v3s16(0,1,1),
	v3s16(1,0,0),
	v3s16(1,1,0),
	v3s16(1,0,1),
	v3s16(1,1,1),
};

// Calculate lighting at the XYZ- corner of p
static u8 getSmoothLight(v3s16 p, VoxelManipulator &vmanip, u32 daynight_ratio,
		INodeDefManager *ndef)
{
	u16 ambient_occlusion = 0;
	u16 light = 0;
	u16 light_count = 0;
	for(u32 i=0; i<8; i++)
	{
		MapNode n = vmanip.getNodeNoEx(p - dirs8[i]);
		if(ndef->get(n).param_type == CPT_LIGHT
				// Fast-style leaves look better this way
				&& ndef->get(n).solidness != 2)
		{
			light += decode_light(n.getLightBlend(daynight_ratio, ndef));
			light_count++;
		}
		else
		{
			if(n.getContent() != CONTENT_IGNORE)
				ambient_occlusion++;
		}
	}

	if(light_count == 0)
		return 255;
	
	light /= light_count;

	if(ambient_occlusion > 4)
	{
		ambient_occlusion -= 4;
		light = (float)light / ((float)ambient_occlusion * 0.5 + 1.0);
	}

	return light;
}

// Calculate lighting at the given corner of p
static u8 getSmoothLight(v3s16 p, v3s16 corner,
		VoxelManipulator &vmanip, u32 daynight_ratio, INodeDefManager *ndef)
{
	if(corner.X == 1) p.X += 1;
	else              assert(corner.X == -1);
	if(corner.Y == 1) p.Y += 1;
	else              assert(corner.Y == -1);
	if(corner.Z == 1) p.Z += 1;
	else              assert(corner.Z == -1);
	
	return getSmoothLight(p, vmanip, daynight_ratio, ndef);
}

static void getTileInfo(
		// Input:
		v3s16 blockpos_nodes,
		v3s16 p,
		v3s16 face_dir,
		u32 daynight_ratio,
		VoxelManipulator &vmanip,
		NodeModMap *temp_mods,
		bool smooth_lighting,
		IGameDef *gamedef,
		// Output:
		bool &makes_face,
		v3s16 &p_corrected,
		v3s16 &face_dir_corrected,
		u8 *lights,
		TileSpec &tile
	)
{
	ITextureSource *tsrc = gamedef->tsrc();
	INodeDefManager *ndef = gamedef->ndef();

	MapNode n0 = vmanip.getNodeNoEx(blockpos_nodes + p);
	MapNode n1 = vmanip.getNodeNoEx(blockpos_nodes + p + face_dir);
	TileSpec tile0 = getNodeTile(n0, p, face_dir, temp_mods, tsrc, ndef);
	TileSpec tile1 = getNodeTile(n1, p + face_dir, -face_dir, temp_mods, tsrc, ndef);
	
	// This is hackish
	content_t content0 = getNodeContent(p, n0, temp_mods);
	content_t content1 = getNodeContent(p + face_dir, n1, temp_mods);
	bool equivalent = false;
	u8 mf = face_contents(content0, content1, &equivalent, ndef);

	if(mf == 0)
	{
		makes_face = false;
		return;
	}

	makes_face = true;
	
	if(mf == 1)
	{
		tile = tile0;
		p_corrected = p;
		face_dir_corrected = face_dir;
	}
	else
	{
		tile = tile1;
		p_corrected = p + face_dir;
		face_dir_corrected = -face_dir;
	}
	
	// eg. water and glass
	if(equivalent)
		tile.material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
	
	if(smooth_lighting == false)
	{
		lights[0] = lights[1] = lights[2] = lights[3] =
				decode_light(getFaceLight(daynight_ratio, n0, n1, face_dir, ndef));
	}
	else
	{
		v3s16 vertex_dirs[4];
		getNodeVertexDirs(face_dir_corrected, vertex_dirs);
		for(u16 i=0; i<4; i++)
		{
			lights[i] = getSmoothLight(blockpos_nodes + p_corrected,
					vertex_dirs[i], vmanip, daynight_ratio, ndef);
		}
	}
	
	return;
}

/*
	startpos:
	translate_dir: unit vector with only one of x, y or z
	face_dir: unit vector with only one of x, y or z
*/
static void updateFastFaceRow(
		u32 daynight_ratio,
		v3f posRelative_f,
		v3s16 startpos,
		u16 length,
		v3s16 translate_dir,
		v3f translate_dir_f,
		v3s16 face_dir,
		v3f face_dir_f,
		core::array<FastFace> &dest,
		NodeModMap *temp_mods,
		VoxelManipulator &vmanip,
		v3s16 blockpos_nodes,
		bool smooth_lighting,
		IGameDef *gamedef)
{
	v3s16 p = startpos;
	
	u16 continuous_tiles_count = 0;
	
	bool makes_face = false;
	v3s16 p_corrected;
	v3s16 face_dir_corrected;
	u8 lights[4] = {0,0,0,0};
	TileSpec tile;
	getTileInfo(blockpos_nodes, p, face_dir, daynight_ratio,
			vmanip, temp_mods, smooth_lighting, gamedef,
			makes_face, p_corrected, face_dir_corrected, lights, tile);

	for(u16 j=0; j<length; j++)
	{
		// If tiling can be done, this is set to false in the next step
		bool next_is_different = true;
		
		v3s16 p_next;
		
		bool next_makes_face = false;
		v3s16 next_p_corrected;
		v3s16 next_face_dir_corrected;
		u8 next_lights[4] = {0,0,0,0};
		TileSpec next_tile;
		
		// If at last position, there is nothing to compare to and
		// the face must be drawn anyway
		if(j != length - 1)
		{
			p_next = p + translate_dir;
			
			getTileInfo(blockpos_nodes, p_next, face_dir, daynight_ratio,
					vmanip, temp_mods, smooth_lighting, gamedef,
					next_makes_face, next_p_corrected,
					next_face_dir_corrected, next_lights,
					next_tile);
			
			if(next_makes_face == makes_face
					&& next_p_corrected == p_corrected + translate_dir
					&& next_face_dir_corrected == face_dir_corrected
					&& next_lights[0] == lights[0]
					&& next_lights[1] == lights[1]
					&& next_lights[2] == lights[2]
					&& next_lights[3] == lights[3]
					&& next_tile == tile)
			{
				next_is_different = false;
			}
			else{
				/*if(makes_face){
					g_profiler->add("Meshgen: diff: next_makes_face != makes_face",
							next_makes_face != makes_face ? 1 : 0);
					g_profiler->add("Meshgen: diff: n_p_corr != p_corr + t_dir",
							(next_p_corrected != p_corrected + translate_dir) ? 1 : 0);
					g_profiler->add("Meshgen: diff: next_f_dir_corr != f_dir_corr",
							next_face_dir_corrected != face_dir_corrected ? 1 : 0);
					g_profiler->add("Meshgen: diff: next_lights[] != lights[]",
							(next_lights[0] != lights[0] ||
							next_lights[0] != lights[0] ||
							next_lights[0] != lights[0] ||
							next_lights[0] != lights[0]) ? 1 : 0);
					g_profiler->add("Meshgen: diff: !(next_tile == tile)",
							!(next_tile == tile) ? 1 : 0);
				}*/
			}
			/*g_profiler->add("Meshgen: Total faces checked", 1);
			if(makes_face)
				g_profiler->add("Meshgen: Total makes_face checked", 1);*/
		} else {
			/*if(makes_face)
				g_profiler->add("Meshgen: diff: last position", 1);*/
		}

		continuous_tiles_count++;
		
		// This is set to true if the texture doesn't allow more tiling
		bool end_of_texture = false;
		/*
			If there is no texture, it can be tiled infinitely.
			If tiled==0, it means the texture can be tiled infinitely.
			Otherwise check tiled agains continuous_tiles_count.
		*/
		if(tile.texture.atlas != NULL && tile.texture.tiled != 0)
		{
			if(tile.texture.tiled <= continuous_tiles_count)
				end_of_texture = true;
		}
		
		// Do this to disable tiling textures
		//end_of_texture = true; //DEBUG
		
		if(next_is_different || end_of_texture)
		{
			/*
				Create a face if there should be one
			*/
			if(makes_face)
			{
				// Floating point conversion of the position vector
				v3f pf(p_corrected.X, p_corrected.Y, p_corrected.Z);
				// Center point of face (kind of)
				v3f sp = pf - ((f32)continuous_tiles_count / 2. - 0.5) * translate_dir_f;
				if(continuous_tiles_count != 1)
					sp += translate_dir_f;
				v3f scale(1,1,1);

				if(translate_dir.X != 0)
				{
					scale.X = continuous_tiles_count;
				}
				if(translate_dir.Y != 0)
				{
					scale.Y = continuous_tiles_count;
				}
				if(translate_dir.Z != 0)
				{
					scale.Z = continuous_tiles_count;
				}
				
				makeFastFace(tile, lights[0], lights[1], lights[2], lights[3],
						sp, face_dir_corrected, scale,
						posRelative_f, dest);
				
				g_profiler->avg("Meshgen: faces drawn by tiling", 0);
				for(int i=1; i<continuous_tiles_count; i++){
					g_profiler->avg("Meshgen: faces drawn by tiling", 1);
				}
			}

			continuous_tiles_count = 0;
			
			makes_face = next_makes_face;
			p_corrected = next_p_corrected;
			face_dir_corrected = next_face_dir_corrected;
			lights[0] = next_lights[0];
			lights[1] = next_lights[1];
			lights[2] = next_lights[2];
			lights[3] = next_lights[3];
			tile = next_tile;
		}
		
		p = p_next;
	}
}

scene::SMesh* makeMapBlockMesh(MeshMakeData *data, IGameDef *gamedef)
{
	// 4-21ms for MAP_BLOCKSIZE=16
	// 24-155ms for MAP_BLOCKSIZE=32
	//TimeTaker timer1("makeMapBlockMesh()");

	core::array<FastFace> fastfaces_new;

	v3s16 blockpos_nodes = data->m_blockpos*MAP_BLOCKSIZE;
	
	// floating point conversion
	v3f posRelative_f(blockpos_nodes.X, blockpos_nodes.Y, blockpos_nodes.Z);
	
	/*
		Some settings
	*/
	//bool new_style_water = g_settings->getBool("new_style_water");
	//bool new_style_leaves = g_settings->getBool("new_style_leaves");
	bool smooth_lighting = g_settings->getBool("smooth_lighting");
	
	/*
		We are including the faces of the trailing edges of the block.
		This means that when something changes, the caller must
		also update the meshes of the blocks at the leading edges.

		NOTE: This is the slowest part of this method.
	*/
	
	{
		// 4-23ms for MAP_BLOCKSIZE=16
		//TimeTaker timer2("updateMesh() collect");

		/*
			Go through every y,z and get top(y+) faces in rows of x+
		*/
		for(s16 y=0; y<MAP_BLOCKSIZE; y++){
			for(s16 z=0; z<MAP_BLOCKSIZE; z++){
				updateFastFaceRow(data->m_daynight_ratio, posRelative_f,
						v3s16(0,y,z), MAP_BLOCKSIZE,
						v3s16(1,0,0), //dir
						v3f  (1,0,0),
						v3s16(0,1,0), //face dir
						v3f  (0,1,0),
						fastfaces_new,
						&data->m_temp_mods,
						data->m_vmanip,
						blockpos_nodes,
						smooth_lighting,
						gamedef);
			}
		}
		/*
			Go through every x,y and get right(x+) faces in rows of z+
		*/
		for(s16 x=0; x<MAP_BLOCKSIZE; x++){
			for(s16 y=0; y<MAP_BLOCKSIZE; y++){
				updateFastFaceRow(data->m_daynight_ratio, posRelative_f,
						v3s16(x,y,0), MAP_BLOCKSIZE,
						v3s16(0,0,1),
						v3f  (0,0,1),
						v3s16(1,0,0),
						v3f  (1,0,0),
						fastfaces_new,
						&data->m_temp_mods,
						data->m_vmanip,
						blockpos_nodes,
						smooth_lighting,
						gamedef);
			}
		}
		/*
			Go through every y,z and get back(z+) faces in rows of x+
		*/
		for(s16 z=0; z<MAP_BLOCKSIZE; z++){
			for(s16 y=0; y<MAP_BLOCKSIZE; y++){
				updateFastFaceRow(data->m_daynight_ratio, posRelative_f,
						v3s16(0,y,z), MAP_BLOCKSIZE,
						v3s16(1,0,0),
						v3f  (1,0,0),
						v3s16(0,0,1),
						v3f  (0,0,1),
						fastfaces_new,
						&data->m_temp_mods,
						data->m_vmanip,
						blockpos_nodes,
						smooth_lighting,
						gamedef);
			}
		}
	}

	// End of slow part

	/*
		Convert FastFaces to SMesh
	*/

	MeshCollector collector;

	if(fastfaces_new.size() > 0)
	{
		// avg 0ms (100ms spikes when loading textures the first time)
		//TimeTaker timer2("updateMesh() mesh building");

		video::SMaterial material;
		material.setFlag(video::EMF_LIGHTING, false);
		material.setFlag(video::EMF_BACK_FACE_CULLING, true);
		material.setFlag(video::EMF_BILINEAR_FILTER, false);
		material.setFlag(video::EMF_FOG_ENABLE, true);
		//material.setFlag(video::EMF_ANTI_ALIASING, video::EAAM_OFF);
		//material.setFlag(video::EMF_ANTI_ALIASING, video::EAAM_SIMPLE);
		material.MaterialType
				= video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

		for(u32 i=0; i<fastfaces_new.size(); i++)
		{
			FastFace &f = fastfaces_new[i];

			const u16 indices[] = {0,1,2,2,3,0};
			const u16 indices_alternate[] = {0,1,3,2,3,1};
			
			video::ITexture *texture = f.tile.texture.atlas;
			if(texture == NULL)
				continue;

			material.setTexture(0, texture);
			
			f.tile.applyMaterialOptions(material);

			const u16 *indices_p = indices;
			
			/*
				Revert triangles for nicer looking gradient if vertices
				1 and 3 have same color or 0 and 2 have different color.
			*/
			if(f.vertices[0].Color != f.vertices[2].Color
					|| f.vertices[1].Color == f.vertices[3].Color)
				indices_p = indices_alternate;
			
			collector.append(material, f.vertices, 4, indices_p, 6);
		}
	}

	/*
		Add special graphics:
		- torches
		- flowing water
		- fences
		- whatever
	*/

	mapblock_mesh_generate_special(data, collector, gamedef);
	
	/*
		Add stuff from collector to mesh
	*/
	
	scene::SMesh *mesh_new = NULL;
	mesh_new = new scene::SMesh();
	
	collector.fillMesh(mesh_new);

	/*
		Do some stuff to the mesh
	*/

	mesh_new->recalculateBoundingBox();

	/*
		Delete new mesh if it is empty
	*/

	if(mesh_new->getMeshBufferCount() == 0)
	{
		mesh_new->drop();
		mesh_new = NULL;
	}

	if(mesh_new)
	{
#if 0
		// Usually 1-700 faces and 1-7 materials
		std::cout<<"Updated MapBlock has "<<fastfaces_new.size()<<" faces "
				<<"and uses "<<mesh_new->getMeshBufferCount()
				<<" materials (meshbuffers)"<<std::endl;
#endif

		// Use VBO for mesh (this just would set this for ever buffer)
		// This will lead to infinite memory usage because or irrlicht.
		//mesh_new->setHardwareMappingHint(scene::EHM_STATIC);

		/*
			NOTE: If that is enabled, some kind of a queue to the main
			thread should be made which would call irrlicht to delete
			the hardware buffer and then delete the mesh
		*/
	}

	return mesh_new;
	
	//std::cout<<"added "<<fastfaces.getSize()<<" faces."<<std::endl;
}

