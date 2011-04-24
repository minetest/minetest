/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "mapblock.h"
#include "map.h"
// For g_settings
#include "main.h"
#include "light.h"
#include <sstream>

#ifndef SERVER
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
		NodeContainer *parentcontainer = block->getParent();
		// This will only work if the parent is the map
		assert(parentcontainer->nodeContainerId() == NODECONTAINER_ID_MAP);
		// OK, we have the map!
		Map *map = (Map*)parentcontainer;

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
#endif

/*
	Parameters must consist of air and !air.
	Order doesn't matter.

	If either of the nodes doesn't exist, light is 0.
	
	parameters:
		daynight_ratio: 0...1000
		n: getNodeParent(p)
		n2: getNodeParent(p + face_dir)
		face_dir: axis oriented unit vector from p to p2
	
	returns encoded light value.
*/
u8 getFaceLight(u32 daynight_ratio, MapNode n, MapNode n2,
		v3s16 face_dir)
{
	try{
		u8 light;
		u8 l1 = n.getLightBlend(daynight_ratio);
		u8 l2 = n2.getLightBlend(daynight_ratio);
		if(l1 > l2)
			light = l1;
		else
			light = l2;

		// Make some nice difference to different sides

		// This makes light come from a corner
		/*if(face_dir.X == 1 || face_dir.Z == 1 || face_dir.Y == -1)
			light = diminish_light(diminish_light(light));
		else if(face_dir.X == -1 || face_dir.Z == -1)
			light = diminish_light(light);*/
		
		// All neighboring faces have different shade (like in minecraft)
		if(face_dir.X == 1 || face_dir.X == -1 || face_dir.Y == -1)
			light = diminish_light(diminish_light(light));
		else if(face_dir.Z == 1 || face_dir.Z == -1)
			light = diminish_light(light);

		return light;
	}
	catch(InvalidPositionException &e)
	{
		return 0;
	}
}

#ifndef SERVER

/*
	vertex_dirs: v3s16[4]
*/
void getNodeVertexDirs(v3s16 dir, v3s16 *vertex_dirs)
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

inline video::SColor lightColor(u8 alpha, u8 light)
{
	return video::SColor(alpha,light,light,light);
}

void makeFastFace(TileSpec tile, u8 li0, u8 li1, u8 li2, u8 li3, v3f p,
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

	v3f zerovector = v3f(0,0,0);
	
	u8 alpha = tile.alpha;
	/*u8 alpha = 255;
	if(tile.id == TILE_WATER)
		alpha = WATER_ALPHA;*/

	float x0 = tile.texture.pos.X;
	float y0 = tile.texture.pos.Y;
	float w = tile.texture.size.X;
	float h = tile.texture.size.Y;

	/*video::SColor c = lightColor(alpha, li);

	face.vertices[0] = video::S3DVertex(vertex_pos[0], v3f(0,1,0), c,
			core::vector2d<f32>(x0+w*abs_scale, y0+h));
	face.vertices[1] = video::S3DVertex(vertex_pos[1], v3f(0,1,0), c,
			core::vector2d<f32>(x0, y0+h));
	face.vertices[2] = video::S3DVertex(vertex_pos[2], v3f(0,1,0), c,
			core::vector2d<f32>(x0, y0));
	face.vertices[3] = video::S3DVertex(vertex_pos[3], v3f(0,1,0), c,
			core::vector2d<f32>(x0+w*abs_scale, y0));*/

	face.vertices[0] = video::S3DVertex(vertex_pos[0], v3f(0,1,0),
			lightColor(alpha, li0),
			core::vector2d<f32>(x0+w*abs_scale, y0+h));
	face.vertices[1] = video::S3DVertex(vertex_pos[1], v3f(0,1,0),
			lightColor(alpha, li1),
			core::vector2d<f32>(x0, y0+h));
	face.vertices[2] = video::S3DVertex(vertex_pos[2], v3f(0,1,0),
			lightColor(alpha, li2),
			core::vector2d<f32>(x0, y0));
	face.vertices[3] = video::S3DVertex(vertex_pos[3], v3f(0,1,0),
			lightColor(alpha, li3),
			core::vector2d<f32>(x0+w*abs_scale, y0));

	face.tile = tile;
	//DEBUG
	//f->tile = TILE_STONE;
	
	dest.push_back(face);
}
	
/*
	Gets node tile from any place relative to block.
	Returns TILE_NODE if doesn't exist or should not be drawn.
*/
TileSpec getNodeTile(MapNode mn, v3s16 p, v3s16 face_dir,
		NodeModMap &temp_mods)
{
	TileSpec spec;
	spec = mn.getTile(face_dir);
	
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
	if(temp_mods.get(p, &mod))
	{
		if(mod.type == NODEMOD_CHANGECONTENT)
		{
			MapNode mn2(mod.param);
			spec = mn2.getTile(face_dir);
		}
		if(mod.type == NODEMOD_CRACK)
		{
			/*
				Get texture id, translate it to name, append stuff to
				name, get texture id
			*/

			// Get original texture name
			u32 orig_id = spec.texture.id;
			std::string orig_name = g_texturesource->getTextureName(orig_id);

			// Create new texture name
			std::ostringstream os;
			os<<orig_name<<"^[crack"<<mod.param;

			// Get new texture
			u32 new_id = g_texturesource->getTextureId(os.str());
			
			/*dstream<<"MapBlock::getNodeTile(): Switching from "
					<<orig_name<<" to "<<os.str()<<" ("
					<<orig_id<<" to "<<new_id<<")"<<std::endl;*/
			
			spec.texture = g_texturesource->getTexture(new_id);
		}
	}
	
	return spec;
}

u8 getNodeContent(v3s16 p, MapNode mn, NodeModMap &temp_mods)
{
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
	if(temp_mods.get(p, &mod))
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

	return mn.d;
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
u8 getSmoothLight(v3s16 p, VoxelManipulator &vmanip, u32 daynight_ratio)
{
	u16 ambient_occlusion = 0;
	u16 light = 0;
	u16 light_count = 0;
	for(u32 i=0; i<8; i++)
	{
		MapNode n = vmanip.getNodeNoEx(p - dirs8[i]);
		if(content_features(n.d).param_type == CPT_LIGHT)
		{
			light += decode_light(n.getLightBlend(daynight_ratio));
			light_count++;
		}
		else
		{
			if(n.d != CONTENT_IGNORE)
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
u8 getSmoothLight(v3s16 p, v3s16 corner,
		VoxelManipulator &vmanip, u32 daynight_ratio)
{
	if(corner.X == 1) p.X += 1;
	else              assert(corner.X == -1);
	if(corner.Y == 1) p.Y += 1;
	else              assert(corner.Y == -1);
	if(corner.Z == 1) p.Z += 1;
	else              assert(corner.Z == -1);
	
	return getSmoothLight(p, vmanip, daynight_ratio);
}

void getTileInfo(
		// Input:
		v3s16 blockpos_nodes,
		v3s16 p,
		v3s16 face_dir,
		u32 daynight_ratio,
		VoxelManipulator &vmanip,
		NodeModMap &temp_mods,
		bool smooth_lighting,
		// Output:
		bool &makes_face,
		v3s16 &p_corrected,
		v3s16 &face_dir_corrected,
		u8 *lights,
		TileSpec &tile
	)
{
	MapNode n0 = vmanip.getNodeNoEx(blockpos_nodes + p);
	MapNode n1 = vmanip.getNodeNoEx(blockpos_nodes + p + face_dir);
	TileSpec tile0 = getNodeTile(n0, p, face_dir, temp_mods);
	TileSpec tile1 = getNodeTile(n1, p + face_dir, -face_dir, temp_mods);
	
	// This is hackish
	u8 content0 = getNodeContent(p, n0, temp_mods);
	u8 content1 = getNodeContent(p + face_dir, n1, temp_mods);
	u8 mf = face_contents(content0, content1);

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
	
	if(smooth_lighting == false)
	{
		lights[0] = lights[1] = lights[2] = lights[3] =
				decode_light(getFaceLight(daynight_ratio, n0, n1, face_dir));
	}
	else
	{
		v3s16 vertex_dirs[4];
		getNodeVertexDirs(face_dir_corrected, vertex_dirs);
		for(u16 i=0; i<4; i++)
		{
			lights[i] = getSmoothLight(blockpos_nodes + p_corrected,
					vertex_dirs[i], vmanip, daynight_ratio);
		}
	}
	
	return;
}

/*
	startpos:
	translate_dir: unit vector with only one of x, y or z
	face_dir: unit vector with only one of x, y or z
*/
void updateFastFaceRow(
		u32 daynight_ratio,
		v3f posRelative_f,
		v3s16 startpos,
		u16 length,
		v3s16 translate_dir,
		v3f translate_dir_f,
		v3s16 face_dir,
		v3f face_dir_f,
		core::array<FastFace> &dest,
		NodeModMap &temp_mods,
		VoxelManipulator &vmanip,
		v3s16 blockpos_nodes,
		bool smooth_lighting)
{
	v3s16 p = startpos;
	
	u16 continuous_tiles_count = 0;
	
	bool makes_face;
	v3s16 p_corrected;
	v3s16 face_dir_corrected;
	u8 lights[4];
	TileSpec tile;
	getTileInfo(blockpos_nodes, p, face_dir, daynight_ratio,
			vmanip, temp_mods, smooth_lighting,
			makes_face, p_corrected, face_dir_corrected, lights, tile);

	for(u16 j=0; j<length; j++)
	{
		// If tiling can be done, this is set to false in the next step
		bool next_is_different = true;
		
		v3s16 p_next;
		
		bool next_makes_face;
		v3s16 next_p_corrected;
		v3s16 next_face_dir_corrected;
		u8 next_lights[4];
		TileSpec next_tile;
		
		// If at last position, there is nothing to compare to and
		// the face must be drawn anyway
		if(j != length - 1)
		{
			p_next = p + translate_dir;
			
			getTileInfo(blockpos_nodes, p_next, face_dir, daynight_ratio,
					vmanip, temp_mods, smooth_lighting,
					next_makes_face, next_p_corrected,
					next_face_dir_corrected, next_lights,
					next_tile);
			
			if(next_makes_face == makes_face
					&& next_p_corrected == p_corrected
					&& next_face_dir_corrected == face_dir_corrected
					&& next_lights[0] == lights[0]
					&& next_lights[1] == lights[1]
					&& next_lights[2] == lights[2]
					&& next_lights[3] == lights[3]
					&& next_tile == tile)
			{
				next_is_different = false;
			}
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
		end_of_texture = true; //DEBUG
		
		// Disable tiling of textures if smooth lighting is used
		if(smooth_lighting)
			end_of_texture = true;
		
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

			#if 0
				// First node
				v3s16 p_first = p_corrected - (continuous_tiles_count-1)
						* translate_dir;

				v3s16 p_map_leftmost;
				v3s16 p_map_rightmost;
				p_map_leftmost = p_corrected + blockpos_nodes;
				p_map_rightmost = p_first + blockpos_nodes;
				
				/*if(p != p_corrected)
				{
					if(face_dir == v3s16(0,0,1))
					{
						v3s16 orig_leftmost = p_map_leftmost;
						v3s16 orig_rightmost = p_map_leftmost;
						p_map_leftmost = orig_rightmost;
						p_map_rightmost = orig_leftmost;
					}
				}*/

				if(smooth_lighting == false)
				{
					li0 = li1 = li2 = li3 = decode_light(light);
				}
				else
				{
					v3s16 vertex_dirs[4];
					getNodeVertexDirs(face_dir_corrected, vertex_dirs);
					
					li0 = getSmoothLight(p_map_rightmost, vertex_dirs[0],
							vmanip, daynight_ratio);
					li1 = getSmoothLight(p_map_leftmost, vertex_dirs[1],
							vmanip, daynight_ratio);
					li2 = getSmoothLight(p_map_leftmost, vertex_dirs[2],
							vmanip, daynight_ratio);
					li3 = getSmoothLight(p_map_rightmost, vertex_dirs[3],
							vmanip, daynight_ratio);
				}
				makeFastFace(tile, li0, li1, li2, li3,
						sp, face_dir_corrected, scale,
						posRelative_f, dest);
			#endif
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

/*
	This is used because CMeshBuffer::append() is very slow
*/
struct PreMeshBuffer
{
	video::SMaterial material;
	core::array<u16> indices;
	core::array<video::S3DVertex> vertices;
};

class MeshCollector
{
public:
	void append(
			video::SMaterial material,
			const video::S3DVertex* const vertices,
			u32 numVertices,
			const u16* const indices,
			u32 numIndices
		)
	{
		PreMeshBuffer *p = NULL;
		for(u32 i=0; i<m_prebuffers.size(); i++)
		{
			PreMeshBuffer &pp = m_prebuffers[i];
			if(pp.material != material)
				continue;

			p = &pp;
			break;
		}

		if(p == NULL)
		{
			PreMeshBuffer pp;
			pp.material = material;
			m_prebuffers.push_back(pp);
			p = &m_prebuffers[m_prebuffers.size()-1];
		}

		u32 vertex_count = p->vertices.size();
		for(u32 i=0; i<numIndices; i++)
		{
			u32 j = indices[i] + vertex_count;
			if(j > 65535)
			{
				dstream<<"FIXME: Meshbuffer ran out of indices"<<std::endl;
				// NOTE: Fix is to just add an another MeshBuffer
			}
			p->indices.push_back(j);
		}
		for(u32 i=0; i<numVertices; i++)
		{
			p->vertices.push_back(vertices[i]);
		}
	}

	void fillMesh(scene::SMesh *mesh)
	{
		/*dstream<<"Filling mesh with "<<m_prebuffers.size()
				<<" meshbuffers"<<std::endl;*/
		for(u32 i=0; i<m_prebuffers.size(); i++)
		{
			PreMeshBuffer &p = m_prebuffers[i];

			/*dstream<<"p.vertices.size()="<<p.vertices.size()
					<<", p.indices.size()="<<p.indices.size()
					<<std::endl;*/
			
			// Create meshbuffer
			
			// This is a "Standard MeshBuffer",
			// it's a typedeffed CMeshBuffer<video::S3DVertex>
			scene::SMeshBuffer *buf = new scene::SMeshBuffer();
			// Set material
			buf->Material = p.material;
			//((scene::SMeshBuffer*)buf)->Material = p.material;
			// Use VBO
			//buf->setHardwareMappingHint(scene::EHM_STATIC);
			// Add to mesh
			mesh->addMeshBuffer(buf);
			// Mesh grabbed it
			buf->drop();

			buf->append(p.vertices.pointer(), p.vertices.size(),
					p.indices.pointer(), p.indices.size());
		}
	}

private:
	core::array<PreMeshBuffer> m_prebuffers;
};

scene::SMesh* makeMapBlockMesh(MeshMakeData *data)
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
	bool new_style_water = g_settings.getBool("new_style_water");
	bool new_style_leaves = g_settings.getBool("new_style_leaves");
	bool smooth_lighting = g_settings.getBool("smooth_lighting");
	
	float node_water_level = 1.0;
	if(new_style_water)
		node_water_level = 0.85;
	
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
						data->m_temp_mods,
						data->m_vmanip,
						blockpos_nodes,
						smooth_lighting);
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
						data->m_temp_mods,
						data->m_vmanip,
						blockpos_nodes,
						smooth_lighting);
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
						data->m_temp_mods,
						data->m_vmanip,
						blockpos_nodes,
						smooth_lighting);
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
		material.setFlag(video::EMF_BILINEAR_FILTER, false);
		material.setFlag(video::EMF_FOG_ENABLE, true);
		//material.setFlag(video::EMF_ANTI_ALIASING, video::EAAM_OFF);
		//material.setFlag(video::EMF_ANTI_ALIASING, video::EAAM_SIMPLE);

		for(u32 i=0; i<fastfaces_new.size(); i++)
		{
			FastFace &f = fastfaces_new[i];

			const u16 indices[] = {0,1,2,2,3,0};
			
			video::ITexture *texture = f.tile.texture.atlas;
			if(texture == NULL)
				continue;

			material.setTexture(0, texture);
			
			f.tile.applyMaterialOptions(material);
			
			collector.append(material, f.vertices, 4, indices, 6);
		}
	}

	/*
		Add special graphics:
		- torches
		- flowing water
	*/

	// 0ms
	//TimeTaker timer2("updateMesh() adding special stuff");

	// Flowing water material
	video::SMaterial material_water1;
	material_water1.setFlag(video::EMF_LIGHTING, false);
	material_water1.setFlag(video::EMF_BACK_FACE_CULLING, false);
	material_water1.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_water1.setFlag(video::EMF_FOG_ENABLE, true);
	material_water1.MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
	AtlasPointer pa_water1 = g_texturesource->getTexture(
			g_texturesource->getTextureId("water.png"));
	material_water1.setTexture(0, pa_water1.atlas);

	// New-style leaves material
	video::SMaterial material_leaves1;
	material_leaves1.setFlag(video::EMF_LIGHTING, false);
	//material_leaves1.setFlag(video::EMF_BACK_FACE_CULLING, false);
	material_leaves1.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_leaves1.setFlag(video::EMF_FOG_ENABLE, true);
	material_leaves1.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	AtlasPointer pa_leaves1 = g_texturesource->getTexture(
			g_texturesource->getTextureId("leaves.png"));
	material_leaves1.setTexture(0, pa_leaves1.atlas);

	for(s16 z=0; z<MAP_BLOCKSIZE; z++)
	for(s16 y=0; y<MAP_BLOCKSIZE; y++)
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		v3s16 p(x,y,z);

		MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes+p);
		
		/*
			Add torches to mesh
		*/
		if(n.d == CONTENT_TORCH)
		{
			video::SColor c(255,255,255,255);

			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c, 0,1),
				video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2,BS/2,0, 0,0,0, c, 1,0),
				video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c, 0,0),
			};

			v3s16 dir = unpackDir(n.dir);

			for(s32 i=0; i<4; i++)
			{
				if(dir == v3s16(1,0,0))
					vertices[i].Pos.rotateXZBy(0);
				if(dir == v3s16(-1,0,0))
					vertices[i].Pos.rotateXZBy(180);
				if(dir == v3s16(0,0,1))
					vertices[i].Pos.rotateXZBy(90);
				if(dir == v3s16(0,0,-1))
					vertices[i].Pos.rotateXZBy(-90);
				if(dir == v3s16(0,-1,0))
					vertices[i].Pos.rotateXZBy(45);
				if(dir == v3s16(0,1,0))
					vertices[i].Pos.rotateXZBy(-45);

				vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
			}

			// Set material
			video::SMaterial material;
			material.setFlag(video::EMF_LIGHTING, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, false);
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			//material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			material.MaterialType
					= video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

			if(dir == v3s16(0,-1,0))
				material.setTexture(0,
						g_texturesource->getTextureRaw("torch_on_floor.png"));
			else if(dir == v3s16(0,1,0))
				material.setTexture(0,
						g_texturesource->getTextureRaw("torch_on_ceiling.png"));
			// For backwards compatibility
			else if(dir == v3s16(0,0,0))
				material.setTexture(0,
						g_texturesource->getTextureRaw("torch_on_floor.png"));
			else
				material.setTexture(0, 
						g_texturesource->getTextureRaw("torch.png"));

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(material, vertices, 4, indices, 6);
		}
		/*
			Signs on walls
		*/
		if(n.d == CONTENT_SIGN_WALL)
		{
			u8 l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			video::SColor c(255,l,l,l);
				
			float d = (float)BS/16;
			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(BS/2-d,-BS/2,-BS/2, 0,0,0, c, 0,1),
				video::S3DVertex(BS/2-d,-BS/2,BS/2, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2-d,BS/2,BS/2, 0,0,0, c, 1,0),
				video::S3DVertex(BS/2-d,BS/2,-BS/2, 0,0,0, c, 0,0),
			};

			v3s16 dir = unpackDir(n.dir);

			for(s32 i=0; i<4; i++)
			{
				if(dir == v3s16(1,0,0))
					vertices[i].Pos.rotateXZBy(0);
				if(dir == v3s16(-1,0,0))
					vertices[i].Pos.rotateXZBy(180);
				if(dir == v3s16(0,0,1))
					vertices[i].Pos.rotateXZBy(90);
				if(dir == v3s16(0,0,-1))
					vertices[i].Pos.rotateXZBy(-90);
				if(dir == v3s16(0,-1,0))
					vertices[i].Pos.rotateXYBy(-90);
				if(dir == v3s16(0,1,0))
					vertices[i].Pos.rotateXYBy(90);

				vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
			}

			// Set material
			video::SMaterial material;
			material.setFlag(video::EMF_LIGHTING, false);
			material.setFlag(video::EMF_BACK_FACE_CULLING, false);
			material.setFlag(video::EMF_BILINEAR_FILTER, false);
			material.setFlag(video::EMF_FOG_ENABLE, true);
			//material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			material.MaterialType
					= video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

			material.setTexture(0, 
					g_texturesource->getTextureRaw("sign_wall.png"));

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(material, vertices, 4, indices, 6);
		}
		/*
			Add flowing water to mesh
		*/
		else if(n.d == CONTENT_WATER)
		{
			bool top_is_water = false;
			MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y+1,z));
			if(ntop.d == CONTENT_WATER || ntop.d == CONTENT_WATERSOURCE)
				top_is_water = true;
			
			u8 l = 0;
			// Use the light of the node on top if possible
			if(content_features(ntop.d).param_type == CPT_LIGHT)
				l = decode_light(ntop.getLightBlend(data->m_daynight_ratio));
			// Otherwise use the light of this node (the water)
			else
				l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			video::SColor c(WATER_ALPHA,l,l,l);
			
			// Neighbor water levels (key = relative position)
			// Includes current node
			core::map<v3s16, f32> neighbor_levels;
			core::map<v3s16, u8> neighbor_contents;
			core::map<v3s16, u8> neighbor_flags;
			const u8 neighborflag_top_is_water = 0x01;
			v3s16 neighbor_dirs[9] = {
				v3s16(0,0,0),
				v3s16(0,0,1),
				v3s16(0,0,-1),
				v3s16(1,0,0),
				v3s16(-1,0,0),
				v3s16(1,0,1),
				v3s16(-1,0,-1),
				v3s16(1,0,-1),
				v3s16(-1,0,1),
			};
			for(u32 i=0; i<9; i++)
			{
				u8 content = CONTENT_AIR;
				float level = -0.5 * BS;
				u8 flags = 0;
				// Check neighbor
				v3s16 p2 = p + neighbor_dirs[i];
				MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				if(n2.d != CONTENT_IGNORE)
				{
					content = n2.d;

					if(n2.d == CONTENT_WATERSOURCE)
						level = (-0.5+node_water_level) * BS;
					else if(n2.d == CONTENT_WATER)
						level = (-0.5 + ((float)n2.param2 + 0.5) / 8.0
								* node_water_level) * BS;

					// Check node above neighbor.
					// NOTE: This doesn't get executed if neighbor
					//       doesn't exist
					p2.Y += 1;
					n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
					if(n2.d == CONTENT_WATERSOURCE || n2.d == CONTENT_WATER)
						flags |= neighborflag_top_is_water;
				}
				
				neighbor_levels.insert(neighbor_dirs[i], level);
				neighbor_contents.insert(neighbor_dirs[i], content);
				neighbor_flags.insert(neighbor_dirs[i], flags);
			}

			//float water_level = (-0.5 + ((float)n.param2 + 0.5) / 8.0) * BS;
			//float water_level = neighbor_levels[v3s16(0,0,0)];

			// Corner heights (average between four waters)
			f32 corner_levels[4];
			
			v3s16 halfdirs[4] = {
				v3s16(0,0,0),
				v3s16(1,0,0),
				v3s16(1,0,1),
				v3s16(0,0,1),
			};
			for(u32 i=0; i<4; i++)
			{
				v3s16 cornerdir = halfdirs[i];
				float cornerlevel = 0;
				u32 valid_count = 0;
				for(u32 j=0; j<4; j++)
				{
					v3s16 neighbordir = cornerdir - halfdirs[j];
					u8 content = neighbor_contents[neighbordir];
					// Special case for source nodes
					if(content == CONTENT_WATERSOURCE)
					{
						cornerlevel = (-0.5+node_water_level)*BS;
						valid_count = 1;
						break;
					}
					else if(content == CONTENT_WATER)
					{
						cornerlevel += neighbor_levels[neighbordir];
						valid_count++;
					}
					else if(content == CONTENT_AIR)
					{
						cornerlevel += -0.5*BS;
						valid_count++;
					}
				}
				if(valid_count > 0)
					cornerlevel /= valid_count;
				corner_levels[i] = cornerlevel;
			}

			/*
				Generate sides
			*/

			v3s16 side_dirs[4] = {
				v3s16(1,0,0),
				v3s16(-1,0,0),
				v3s16(0,0,1),
				v3s16(0,0,-1),
			};
			s16 side_corners[4][2] = {
				{1, 2},
				{3, 0},
				{2, 3},
				{0, 1},
			};
			for(u32 i=0; i<4; i++)
			{
				v3s16 dir = side_dirs[i];

				/*
					If our topside is water and neighbor's topside
					is water, don't draw side face
				*/
				if(top_is_water &&
						neighbor_flags[dir] & neighborflag_top_is_water)
					continue;

				u8 neighbor_content = neighbor_contents[dir];
				
				// Don't draw face if neighbor is not air or water
				if(neighbor_content != CONTENT_AIR
						&& neighbor_content != CONTENT_WATER)
					continue;
				
				bool neighbor_is_water = (neighbor_content == CONTENT_WATER);
				
				// Don't draw any faces if neighbor is water and top is water
				if(neighbor_is_water == true && top_is_water == false)
					continue;
				
				video::S3DVertex vertices[4] =
				{
					/*video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,0),*/
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							pa_water1.x0(), pa_water1.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							pa_water1.x1(), pa_water1.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							pa_water1.x1(), pa_water1.y0()),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							pa_water1.x0(), pa_water1.y0()),
				};
				
				/*
					If our topside is water, set upper border of face
					at upper border of node
				*/
				if(top_is_water)
				{
					vertices[2].Pos.Y = 0.5*BS;
					vertices[3].Pos.Y = 0.5*BS;
				}
				/*
					Otherwise upper position of face is corner levels
				*/
				else
				{
					vertices[2].Pos.Y = corner_levels[side_corners[i][0]];
					vertices[3].Pos.Y = corner_levels[side_corners[i][1]];
				}
				
				/*
					If neighbor is water, lower border of face is corner
					water levels
				*/
				if(neighbor_is_water)
				{
					vertices[0].Pos.Y = corner_levels[side_corners[i][1]];
					vertices[1].Pos.Y = corner_levels[side_corners[i][0]];
				}
				/*
					If neighbor is not water, lower border of face is
					lower border of node
				*/
				else
				{
					vertices[0].Pos.Y = -0.5*BS;
					vertices[1].Pos.Y = -0.5*BS;
				}
				
				for(s32 j=0; j<4; j++)
				{
					if(dir == v3s16(0,0,1))
						vertices[j].Pos.rotateXZBy(0);
					if(dir == v3s16(0,0,-1))
						vertices[j].Pos.rotateXZBy(180);
					if(dir == v3s16(-1,0,0))
						vertices[j].Pos.rotateXZBy(90);
					if(dir == v3s16(1,0,-0))
						vertices[j].Pos.rotateXZBy(-90);

					vertices[j].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(material_water1, vertices, 4, indices, 6);
			}
			
			/*
				Generate top side, if appropriate
			*/
			
			if(top_is_water == false)
			{
				video::S3DVertex vertices[4] =
				{
					/*video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,0),*/
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							pa_water1.x0(), pa_water1.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							pa_water1.x1(), pa_water1.y1()),
					video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c,
							pa_water1.x1(), pa_water1.y0()),
					video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c,
							pa_water1.x0(), pa_water1.y0()),
				};
				
				// This fixes a strange bug
				s32 corner_resolve[4] = {3,2,1,0};

				for(s32 i=0; i<4; i++)
				{
					//vertices[i].Pos.Y += water_level;
					//vertices[i].Pos.Y += neighbor_levels[v3s16(0,0,0)];
					s32 j = corner_resolve[i];
					vertices[i].Pos.Y += corner_levels[j];
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(material_water1, vertices, 4, indices, 6);
			}
		}
		/*
			Add water sources to mesh if using new style
		*/
		else if(n.d == CONTENT_WATERSOURCE && new_style_water)
		{
			//bool top_is_water = false;
			bool top_is_air = false;
			MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y+1,z));
			/*if(n.d == CONTENT_WATER || n.d == CONTENT_WATERSOURCE)
				top_is_water = true;*/
			if(n.d == CONTENT_AIR)
				top_is_air = true;
			
			/*if(top_is_water == true)
				continue;*/
			if(top_is_air == false)
				continue;

			u8 l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			video::SColor c(WATER_ALPHA,l,l,l);
			
			video::S3DVertex vertices[4] =
			{
				/*video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c, 0,1),
				video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,0),
				video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,0),*/
				video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
						pa_water1.x0(), pa_water1.y1()),
				video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
						pa_water1.x1(), pa_water1.y1()),
				video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c,
						pa_water1.x1(), pa_water1.y0()),
				video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c,
						pa_water1.x0(), pa_water1.y0()),
			};

			for(s32 i=0; i<4; i++)
			{
				vertices[i].Pos.Y += (-0.5+node_water_level)*BS;
				vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(material_water1, vertices, 4, indices, 6);
		}
		/*
			Add leaves if using new style
		*/
		else if(n.d == CONTENT_LEAVES && new_style_leaves)
		{
			/*u8 l = decode_light(n.getLightBlend(data->m_daynight_ratio));*/
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c(255,l,l,l);

			for(u32 j=0; j<6; j++)
			{
				video::S3DVertex vertices[4] =
				{
					/*video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c, 0,0),*/
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c,
						pa_leaves1.x0(), pa_leaves1.y1()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c,
						pa_leaves1.x1(), pa_leaves1.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c,
						pa_leaves1.x1(), pa_leaves1.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c,
						pa_leaves1.x0(), pa_leaves1.y0()),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(0);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(180);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-90);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(90);
				}
				else if(j == 4)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateYZBy(-90);
				}
				else if(j == 5)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateYZBy(90);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(material_leaves1, vertices, 4, indices, 6);
			}
		}
	}

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

#endif // !SERVER

/*
	MapBlock
*/

MapBlock::MapBlock(NodeContainer *parent, v3s16 pos, bool dummy):
		m_parent(parent),
		m_pos(pos),
		changed(true),
		is_underground(false),
		m_lighting_expired(true),
		m_day_night_differs(false),
		//m_not_fully_generated(false),
		m_objects(this)
{
	data = NULL;
	if(dummy == false)
		reallocate();
	
	//m_spawn_timer = -10000;

#ifndef SERVER
	m_mesh_expired = false;
	mesh_mutex.Init();
	mesh = NULL;
	m_temp_mods_mutex.Init();
#endif
}

MapBlock::~MapBlock()
{
#ifndef SERVER
	{
		JMutexAutoLock lock(mesh_mutex);
		
		if(mesh)
		{
			mesh->drop();
			mesh = NULL;
		}
	}
#endif

	if(data)
		delete[] data;
}

bool MapBlock::isValidPositionParent(v3s16 p)
{
	if(isValidPosition(p))
	{
		return true;
	}
	else{
		return m_parent->isValidPosition(getPosRelative() + p);
	}
}

MapNode MapBlock::getNodeParent(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		return m_parent->getNode(getPosRelative() + p);
	}
	else
	{
		if(data == NULL)
			throw InvalidPositionException();
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

void MapBlock::setNodeParent(v3s16 p, MapNode & n)
{
	if(isValidPosition(p) == false)
	{
		m_parent->setNode(getPosRelative() + p, n);
	}
	else
	{
		if(data == NULL)
			throw InvalidPositionException();
		data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X] = n;
	}
}

MapNode MapBlock::getNodeParentNoEx(v3s16 p)
{
	if(isValidPosition(p) == false)
	{
		try{
			return m_parent->getNode(getPosRelative() + p);
		}
		catch(InvalidPositionException &e)
		{
			return MapNode(CONTENT_IGNORE);
		}
	}
	else
	{
		if(data == NULL)
		{
			return MapNode(CONTENT_IGNORE);
		}
		return data[p.Z*MAP_BLOCKSIZE*MAP_BLOCKSIZE + p.Y*MAP_BLOCKSIZE + p.X];
	}
}

#ifndef SERVER

#if 1
void MapBlock::updateMesh(u32 daynight_ratio)
{
#if 0
	/*
		DEBUG: If mesh has been generated, don't generate it again
	*/
	{
		JMutexAutoLock meshlock(mesh_mutex);
		if(mesh != NULL)
			return;
	}
#endif

	MeshMakeData data;
	data.fill(daynight_ratio, this);
	
	scene::SMesh *mesh_new = makeMapBlockMesh(&data);
	
	/*
		Replace the mesh
	*/

	replaceMesh(mesh_new);

}
#endif

void MapBlock::replaceMesh(scene::SMesh *mesh_new)
{
	mesh_mutex.Lock();

	//scene::SMesh *mesh_old = mesh[daynight_i];
	//mesh[daynight_i] = mesh_new;

	scene::SMesh *mesh_old = mesh;
	mesh = mesh_new;
	setMeshExpired(false);
	
	if(mesh_old != NULL)
	{
		// Remove hardware buffers of meshbuffers of mesh
		// NOTE: No way, this runs in a different thread and everything
		/*u32 c = mesh_old->getMeshBufferCount();
		for(u32 i=0; i<c; i++)
		{
			IMeshBuffer *buf = mesh_old->getMeshBuffer(i);
		}*/
		
		/*dstream<<"mesh_old->getReferenceCount()="
				<<mesh_old->getReferenceCount()<<std::endl;
		u32 c = mesh_old->getMeshBufferCount();
		for(u32 i=0; i<c; i++)
		{
			scene::IMeshBuffer *buf = mesh_old->getMeshBuffer(i);
			dstream<<"buf->getReferenceCount()="
					<<buf->getReferenceCount()<<std::endl;
		}*/

		// Drop the mesh
		mesh_old->drop();

		//delete mesh_old;
	}

	mesh_mutex.Unlock();
}
	
#endif // !SERVER

/*
	Propagates sunlight down through the block.
	Doesn't modify nodes that are not affected by sunlight.
	
	Returns false if sunlight at bottom block is invalid
	Returns true if bottom block doesn't exist.

	If there is a block above, continues from it.
	If there is no block above, assumes there is sunlight, unless
	is_underground is set or highest node is water.

	At the moment, all sunlighted nodes are added to light_sources.
	- SUGG: This could be optimized

	Turns sunglighted mud into grass.

	if remove_light==true, sets non-sunlighted nodes black.

	if black_air_left!=NULL, it is set to true if non-sunlighted
	air is left in block.
*/
bool MapBlock::propagateSunlight(core::map<v3s16, bool> & light_sources,
		bool remove_light, bool *black_air_left,
		bool grow_grass)
{
	// Whether the sunlight at the top of the bottom block is valid
	bool block_below_is_valid = true;
	
	v3s16 pos_relative = getPosRelative();
	
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		for(s16 z=0; z<MAP_BLOCKSIZE; z++)
		{
#if 1
			bool no_sunlight = false;
			bool no_top_block = false;
			// Check if node above block has sunlight
			try{
				MapNode n = getNodeParent(v3s16(x, MAP_BLOCKSIZE, z));
				if(n.getLight(LIGHTBANK_DAY) != LIGHT_SUN)
				{
					no_sunlight = true;
				}
			}
			catch(InvalidPositionException &e)
			{
				no_top_block = true;
				
				// NOTE: This makes over-ground roofed places sunlighted
				// Assume sunlight, unless is_underground==true
				if(is_underground)
				{
					no_sunlight = true;
				}
				else
				{
					MapNode n = getNode(v3s16(x, MAP_BLOCKSIZE-1, z));
					if(n.d == CONTENT_WATER || n.d == CONTENT_WATERSOURCE)
					{
						no_sunlight = true;
					}
				}
				// NOTE: As of now, this just would make everything dark.
				// No sunlight here
				//no_sunlight = true;
			}
#endif
#if 0 // Doesn't work; nothing gets light.
			bool no_sunlight = true;
			bool no_top_block = false;
			// Check if node above block has sunlight
			try{
				MapNode n = getNodeParent(v3s16(x, MAP_BLOCKSIZE, z));
				if(n.getLight(LIGHTBANK_DAY) == LIGHT_SUN)
				{
					no_sunlight = false;
				}
			}
			catch(InvalidPositionException &e)
			{
				no_top_block = true;
			}
#endif

			/*std::cout<<"("<<x<<","<<z<<"): "
					<<"no_top_block="<<no_top_block
					<<", is_underground="<<is_underground
					<<", no_sunlight="<<no_sunlight
					<<std::endl;*/
		
			s16 y = MAP_BLOCKSIZE-1;
			
			// This makes difference to diminishing in water.
			bool stopped_to_solid_object = false;
			
			u8 current_light = no_sunlight ? 0 : LIGHT_SUN;

			for(; y >= 0; y--)
			{
				v3s16 pos(x, y, z);
				MapNode &n = getNodeRef(pos);
				
				if(current_light == 0)
				{
					// Do nothing
				}
				else if(current_light == LIGHT_SUN && n.sunlight_propagates())
				{
					// Do nothing: Sunlight is continued
				}
				else if(n.light_propagates() == false)
				{
					if(grow_grass)
					{
						bool upper_is_air = false;
						try
						{
							if(getNodeParent(pos+v3s16(0,1,0)).d == CONTENT_AIR)
								upper_is_air = true;
						}
						catch(InvalidPositionException &e)
						{
						}
						// Turn mud into grass
						if(upper_is_air && n.d == CONTENT_MUD
								&& current_light == LIGHT_SUN)
						{
							n.d = CONTENT_GRASS;
						}
					}

					// A solid object is on the way.
					stopped_to_solid_object = true;
					
					// Light stops.
					current_light = 0;
				}
				else
				{
					// Diminish light
					current_light = diminish_light(current_light);
				}

				u8 old_light = n.getLight(LIGHTBANK_DAY);

				if(current_light > old_light || remove_light)
				{
					n.setLight(LIGHTBANK_DAY, current_light);
				}
				
				if(diminish_light(current_light) != 0)
				{
					light_sources.insert(pos_relative + pos, true);
				}

				if(current_light == 0 && stopped_to_solid_object)
				{
					if(black_air_left)
					{
						*black_air_left = true;
					}
				}
			}

			// Whether or not the block below should see LIGHT_SUN
			bool sunlight_should_go_down = (current_light == LIGHT_SUN);

			/*
				If the block below hasn't already been marked invalid:

				Check if the node below the block has proper sunlight at top.
				If not, the block below is invalid.
				
				Ignore non-transparent nodes as they always have no light
			*/
			try
			{
			if(block_below_is_valid)
			{
				MapNode n = getNodeParent(v3s16(x, -1, z));
				if(n.light_propagates())
				{
					if(n.getLight(LIGHTBANK_DAY) == LIGHT_SUN
							&& sunlight_should_go_down == false)
						block_below_is_valid = false;
					else if(n.getLight(LIGHTBANK_DAY) != LIGHT_SUN
							&& sunlight_should_go_down == true)
						block_below_is_valid = false;
				}
			}//if
			}//try
			catch(InvalidPositionException &e)
			{
				/*std::cout<<"InvalidBlockException for bottom block node"
						<<std::endl;*/
				// Just no block below, no need to panic.
			}
		}
	}

	return block_below_is_valid;
}


void MapBlock::copyTo(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));
	
	// Copy from data to VoxelManipulator
	dst.copyFrom(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

void MapBlock::copyFrom(VoxelManipulator &dst)
{
	v3s16 data_size(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);
	VoxelArea data_area(v3s16(0,0,0), data_size - v3s16(1,1,1));
	
	// Copy from VoxelManipulator to data
	dst.copyTo(data, data_area, v3s16(0,0,0),
			getPosRelative(), data_size);
}

void MapBlock::stepObjects(float dtime, bool server, u32 daynight_ratio)
{
	/*
		Step objects
	*/
	m_objects.step(dtime, server, daynight_ratio);

#if 0
	/*
		Spawn some objects at random.

		Use dayNightDiffed() to approximate being near ground level
	*/
	if(m_spawn_timer < -999)
	{
		m_spawn_timer = 60;
	}
	if(dayNightDiffed() == true && getObjectCount() == 0)
	{
		m_spawn_timer -= dtime;
		if(m_spawn_timer <= 0.0)
		{
			m_spawn_timer += myrand() % 300;
			
			v2s16 p2d(
				(myrand()%(MAP_BLOCKSIZE-1))+0,
				(myrand()%(MAP_BLOCKSIZE-1))+0
			);

			s16 y = getGroundLevel(p2d);
			
			if(y >= 0)
			{
				v3s16 p(p2d.X, y+1, p2d.Y);

				if(getNode(p).d == CONTENT_AIR
						&& getNode(p).getLightBlend(daynight_ratio) <= 11)
				{
					RatObject *obj = new RatObject(NULL, -1, intToFloat(p, BS));
					addObject(obj);
				}
			}
		}
	}
#endif

	setChangedFlag();
}


void MapBlock::updateDayNightDiff()
{
	if(data == NULL)
	{
		m_day_night_differs = false;
		return;
	}

	bool differs = false;

	/*
		Check if any lighting value differs
	*/
	for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
	{
		MapNode &n = data[i];
		if(n.getLight(LIGHTBANK_DAY) != n.getLight(LIGHTBANK_NIGHT))
		{
			differs = true;
			break;
		}
	}

	/*
		If some lighting values differ, check if the whole thing is
		just air. If it is, differ = false
	*/
	if(differs)
	{
		bool only_air = true;
		for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
		{
			MapNode &n = data[i];
			if(n.d != CONTENT_AIR)
			{
				only_air = false;
				break;
			}
		}
		if(only_air)
			differs = false;
	}

	// Set member variable
	m_day_night_differs = differs;
}

s16 MapBlock::getGroundLevel(v2s16 p2d)
{
	if(isDummy())
		return -3;
	try
	{
		s16 y = MAP_BLOCKSIZE-1;
		for(; y>=0; y--)
		{
			//if(is_ground_content(getNodeRef(p2d.X, y, p2d.Y).d))
			if(content_features(getNodeRef(p2d.X, y, p2d.Y).d).walkable)
			{
				if(y == MAP_BLOCKSIZE-1)
					return -2;
				else
					return y;
			}
		}
		return -1;
	}
	catch(InvalidPositionException &e)
	{
		return -3;
	}
}

/*
	Serialization
*/

void MapBlock::serialize(std::ostream &os, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");
	
	if(data == NULL)
	{
		throw SerializationError("ERROR: Not writing dummy block.");
	}
	
	// These have no compression
	if(version <= 3 || version == 5 || version == 6)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
		
		u32 buflen = 1 + nodecount * MapNode::serializedLength(version);
		SharedBuffer<u8> dest(buflen);

		dest[0] = is_underground;
		for(u32 i=0; i<nodecount; i++)
		{
			u32 s = 1 + i * MapNode::serializedLength(version);
			data[i].serialize(&dest[s], version);
		}
		
		os.write((char*)*dest, dest.getSize());
	}
	else if(version <= 10)
	{
		/*
			With compression.
			Compress the materials and the params separately.
		*/
		
		// First byte
		os.write((char*)&is_underground, 1);

		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		// Get and compress materials
		SharedBuffer<u8> materialdata(nodecount);
		for(u32 i=0; i<nodecount; i++)
		{
			materialdata[i] = data[i].d;
		}
		compress(materialdata, os, version);

		// Get and compress lights
		SharedBuffer<u8> lightdata(nodecount);
		for(u32 i=0; i<nodecount; i++)
		{
			lightdata[i] = data[i].param;
		}
		compress(lightdata, os, version);
		
		if(version >= 10)
		{
			// Get and compress param2
			SharedBuffer<u8> param2data(nodecount);
			for(u32 i=0; i<nodecount; i++)
			{
				param2data[i] = data[i].param2;
			}
			compress(param2data, os, version);
		}
	}
	// All other versions (newest)
	else
	{
		// First byte
		u8 flags = 0;
		if(is_underground)
			flags |= 0x01;
		if(m_day_night_differs)
			flags |= 0x02;
		if(m_lighting_expired)
			flags |= 0x04;
		os.write((char*)&flags, 1);

		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		/*
			Get data
		*/

		SharedBuffer<u8> databuf(nodecount*3);

		// Get contents
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i] = data[i].d;
		}

		// Get params
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i+nodecount] = data[i].param;
		}

		// Get param2
		for(u32 i=0; i<nodecount; i++)
		{
			databuf[i+nodecount*2] = data[i].param2;
		}

		/*
			Compress data to output stream
		*/

		compress(databuf, os, version);
		
		/*
			NodeMetadata
		*/
		if(version >= 14)
		{
			if(version <= 15)
			{
				std::ostringstream oss(std::ios_base::binary);
				m_node_metadata.serialize(oss);
				os<<serializeString(oss.str());
			}
			else
			{
				std::ostringstream oss(std::ios_base::binary);
				m_node_metadata.serialize(oss);
				compressZlib(oss.str(), os);
				//os<<serializeLongString(oss.str());
			}
		}
	}
}

void MapBlock::deSerialize(std::istream &is, u8 version)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: MapBlock format not supported");

	// These have no lighting info
	if(version <= 1)
	{
		setLightingExpired(true);
	}

	// These have no compression
	if(version <= 3 || version == 5 || version == 6)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;
		char tmp;
		is.read(&tmp, 1);
		if(is.gcount() != 1)
			throw SerializationError
					("MapBlock::deSerialize: no enough input data");
		is_underground = tmp;
		for(u32 i=0; i<nodecount; i++)
		{
			s32 len = MapNode::serializedLength(version);
			SharedBuffer<u8> d(len);
			is.read((char*)*d, len);
			if(is.gcount() != len)
				throw SerializationError
						("MapBlock::deSerialize: no enough input data");
			data[i].deSerialize(*d, version);
		}
	}
	else if(version <= 10)
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		u8 t8;
		is.read((char*)&t8, 1);
		is_underground = t8;

		{
			// Uncompress and set material data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].d = s[i];
			}
		}
		{
			// Uncompress and set param data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].param = s[i];
			}
		}
	
		if(version >= 10)
		{
			// Uncompress and set param2 data
			std::ostringstream os(std::ios_base::binary);
			decompress(is, os, version);
			std::string s = os.str();
			if(s.size() != nodecount)
				throw SerializationError
						("MapBlock::deSerialize: invalid format");
			for(u32 i=0; i<s.size(); i++)
			{
				data[i].param2 = s[i];
			}
		}
	}
	// All other versions (newest)
	else
	{
		u32 nodecount = MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE;

		u8 flags;
		is.read((char*)&flags, 1);
		is_underground = (flags & 0x01) ? true : false;
		m_day_night_differs = (flags & 0x02) ? true : false;
		m_lighting_expired = (flags & 0x04) ? true : false;

		// Uncompress data
		std::ostringstream os(std::ios_base::binary);
		decompress(is, os, version);
		std::string s = os.str();
		if(s.size() != nodecount*3)
			throw SerializationError
					("MapBlock::deSerialize: invalid format");

		// Set contents
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].d = s[i];
		}
		// Set params
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].param = s[i+nodecount];
		}
		// Set param2
		for(u32 i=0; i<nodecount; i++)
		{
			data[i].param2 = s[i+nodecount*2];
		}
		
		/*
			NodeMetadata
		*/
		if(version >= 14)
		{
			// Ignore errors
			try{
				if(version <= 15)
				{
					std::string data = deSerializeString(is);
					std::istringstream iss(data, std::ios_base::binary);
					m_node_metadata.deSerialize(iss);
				}
				else
				{
					//std::string data = deSerializeLongString(is);
					std::ostringstream oss(std::ios_base::binary);
					decompressZlib(is, oss);
					std::istringstream iss(oss.str(), std::ios_base::binary);
					m_node_metadata.deSerialize(iss);
				}
			}
			catch(SerializationError &e)
			{
				dstream<<"WARNING: MapBlock::deSerialize(): Ignoring an error"
						<<" while deserializing node metadata"<<std::endl;
			}
		}
	}
	
	/*
		Translate nodes as specified in the translate_to fields of
		node features
	*/
	for(u32 i=0; i<MAP_BLOCKSIZE*MAP_BLOCKSIZE*MAP_BLOCKSIZE; i++)
	{
		MapNode &n = data[i];

		MapNode *translate_to = content_features(n.d).translate_to;
		if(translate_to)
		{
			dstream<<"MapBlock: WARNING: Translating node "<<n.d<<" to "
					<<translate_to->d<<std::endl;
			n = *translate_to;
		}
	}
}


//END
