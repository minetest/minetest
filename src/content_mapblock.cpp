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

#include "content_mapblock.h"
#include "content_mapnode.h"
#include "main.h" // For g_settings and g_texturesource
#include "mineral.h"

#ifndef SERVER
// Create a cuboid.
//  material  - the material to use (for all 6 faces)
//  collector - the MeshCollector for the resulting polygons
//  pa        - texture atlas pointer for the material
//  c         - vertex colour - used for all
//  pos       - the position of the centre of the cuboid
//  rz,ry,rz  - the radius of the cuboid in each dimension
//  txc       - texture coordinates - this is a list of texture coordinates
//              for the opposite corners of each face - therefore, there
//              should be (2+2)*6=24 values in the list. Alternatively, pass
//              NULL to use the entire texture for each face. The order of
//              the faces in the list is top-backi-right-front-left-bottom
//              If you specified 0,0,1,1 for each face, that would be the
//              same as passing NULL.
void makeCuboid(video::SMaterial &material, MeshCollector *collector,
	AtlasPointer* pa, video::SColor &c,
	v3f &pos, f32 rx, f32 ry, f32 rz, f32* txc)
{
	f32 tu0=pa->x0();
	f32 tu1=pa->x1();
	f32 tv0=pa->y0();
	f32 tv1=pa->y1();
	f32 txus=tu1-tu0;
	f32 txvs=tv1-tv0;

	video::S3DVertex v[4] =
	{
		video::S3DVertex(0,0,0, 0,0,0, c, tu0, tv1),
		video::S3DVertex(0,0,0, 0,0,0, c, tu1, tv1),
		video::S3DVertex(0,0,0, 0,0,0, c, tu1, tv0),
		video::S3DVertex(0,0,0, 0,0,0, c, tu0, tv0)
	};

	for(int i=0;i<6;i++)
	{
		switch(i)
		{
			case 0:	// top
				v[0].Pos.X=-rx; v[0].Pos.Y= ry; v[0].Pos.Z=-rz;
				v[1].Pos.X=-rx; v[1].Pos.Y= ry; v[1].Pos.Z= rz;
				v[2].Pos.X= rx; v[2].Pos.Y= ry; v[2].Pos.Z= rz;
				v[3].Pos.X= rx; v[3].Pos.Y= ry, v[3].Pos.Z=-rz;
				break;
			case 1: // back
				v[0].Pos.X=-rx; v[0].Pos.Y= ry; v[0].Pos.Z=-rz;
				v[1].Pos.X= rx; v[1].Pos.Y= ry; v[1].Pos.Z=-rz;
				v[2].Pos.X= rx; v[2].Pos.Y=-ry; v[2].Pos.Z=-rz;
				v[3].Pos.X=-rx; v[3].Pos.Y=-ry, v[3].Pos.Z=-rz;
				break;
			case 2: //right
				v[0].Pos.X= rx; v[0].Pos.Y= ry; v[0].Pos.Z=-rz;
				v[1].Pos.X= rx; v[1].Pos.Y= ry; v[1].Pos.Z= rz;
				v[2].Pos.X= rx; v[2].Pos.Y=-ry; v[2].Pos.Z= rz;
				v[3].Pos.X= rx; v[3].Pos.Y=-ry, v[3].Pos.Z=-rz;
				break;
			case 3: // front
				v[0].Pos.X= rx; v[0].Pos.Y= ry; v[0].Pos.Z= rz;
				v[1].Pos.X=-rx; v[1].Pos.Y= ry; v[1].Pos.Z= rz;
				v[2].Pos.X=-rx; v[2].Pos.Y=-ry; v[2].Pos.Z= rz;
				v[3].Pos.X= rx; v[3].Pos.Y=-ry, v[3].Pos.Z= rz;
				break;
			case 4: // left
				v[0].Pos.X=-rx; v[0].Pos.Y= ry; v[0].Pos.Z= rz;
				v[1].Pos.X=-rx; v[1].Pos.Y= ry; v[1].Pos.Z=-rz;
				v[2].Pos.X=-rx; v[2].Pos.Y=-ry; v[2].Pos.Z=-rz;
				v[3].Pos.X=-rx; v[3].Pos.Y=-ry, v[3].Pos.Z= rz;
				break;
			case 5: // bottom
				v[0].Pos.X= rx; v[0].Pos.Y=-ry; v[0].Pos.Z= rz;
				v[1].Pos.X=-rx; v[1].Pos.Y=-ry; v[1].Pos.Z= rz;
				v[2].Pos.X=-rx; v[2].Pos.Y=-ry; v[2].Pos.Z=-rz;
				v[3].Pos.X= rx; v[3].Pos.Y=-ry, v[3].Pos.Z=-rz;
				break;
		}

		if(txc!=NULL)
		{
			v[0].TCoords.X=tu0+txus*txc[0]; v[0].TCoords.Y=tv0+txvs*txc[3];
			v[1].TCoords.X=tu0+txus*txc[2]; v[1].TCoords.Y=tv0+txvs*txc[3];
			v[2].TCoords.X=tu0+txus*txc[2]; v[2].TCoords.Y=tv0+txvs*txc[1];
			v[3].TCoords.X=tu0+txus*txc[0]; v[3].TCoords.Y=tv0+txvs*txc[1];
			txc+=4;
		}

		for(u16 i=0; i<4; i++)
			v[i].Pos += pos;
		u16 indices[] = {0,1,2,2,3,0};
		collector->append(material, v, 4, indices, 6);

	}

}
#endif

#ifndef SERVER
void mapblock_mesh_generate_special(MeshMakeData *data,
		MeshCollector &collector)
{
	// 0ms
	//TimeTaker timer("mapblock_mesh_generate_special()");

	/*
		Some settings
	*/
	bool new_style_water = g_settings.getBool("new_style_water");
	bool new_style_leaves = g_settings.getBool("new_style_leaves");
	//bool smooth_lighting = g_settings.getBool("smooth_lighting");
	bool invisible_stone = g_settings.getBool("invisible_stone");
	
	float node_water_level = 1.0;
	if(new_style_water)
		node_water_level = 0.85;
	
	v3s16 blockpos_nodes = data->m_blockpos*MAP_BLOCKSIZE;

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

	// Glass material
	video::SMaterial material_glass;
	material_glass.setFlag(video::EMF_LIGHTING, false);
	material_glass.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_glass.setFlag(video::EMF_FOG_ENABLE, true);
	material_glass.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	AtlasPointer pa_glass = g_texturesource->getTexture(
			g_texturesource->getTextureId("glass.png"));
	material_glass.setTexture(0, pa_glass.atlas);

	// Wood material
	video::SMaterial material_wood;
	material_wood.setFlag(video::EMF_LIGHTING, false);
	material_wood.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_wood.setFlag(video::EMF_FOG_ENABLE, true);
	material_wood.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	AtlasPointer pa_wood = g_texturesource->getTexture(
			g_texturesource->getTextureId("wood.png"));
	material_wood.setTexture(0, pa_wood.atlas);

	// General ground material for special output
	// Texture is modified just before usage
	video::SMaterial material_general;
	material_general.setFlag(video::EMF_LIGHTING, false);
	material_general.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_general.setFlag(video::EMF_FOG_ENABLE, true);
	material_general.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;


	// Papyrus material
	video::SMaterial material_papyrus;
	material_papyrus.setFlag(video::EMF_LIGHTING, false);
	material_papyrus.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_papyrus.setFlag(video::EMF_FOG_ENABLE, true);
	material_papyrus.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	AtlasPointer pa_papyrus = g_texturesource->getTexture(
			g_texturesource->getTextureId("papyrus.png"));
	material_papyrus.setTexture(0, pa_papyrus.atlas);

	// junglegrass material
	video::SMaterial material_junglegrass;
	material_junglegrass.setFlag(video::EMF_LIGHTING, false);
	material_junglegrass.setFlag(video::EMF_BILINEAR_FILTER, false);
	material_junglegrass.setFlag(video::EMF_FOG_ENABLE, true);
	material_junglegrass.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;
	AtlasPointer pa_junglegrass = g_texturesource->getTexture(
			g_texturesource->getTextureId("junglegrass.png"));
	material_junglegrass.setTexture(0, pa_junglegrass.atlas);

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
		else if(n.d == CONTENT_SIGN_WALL)
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
						level = (-0.5 + ((float)(n2.param2 & LIQUID_LEVEL_MASK) + 0.5) / 8.0
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
		/*
			Add glass
		*/
		else if(n.d == CONTENT_GLASS)
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c(255,l,l,l);

			for(u32 j=0; j<6; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c,
						pa_glass.x0(), pa_glass.y1()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c,
						pa_glass.x1(), pa_glass.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c,
						pa_glass.x1(), pa_glass.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c,
						pa_glass.x0(), pa_glass.y0()),
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
				collector.append(material_glass, vertices, 4, indices, 6);
			}
		}
		/*
			Add fence
		*/
		else if(n.d == CONTENT_FENCE)
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c(255,l,l,l);

			const f32 post_rad=(f32)BS/10;
			const f32 bar_rad=(f32)BS/20;
			const f32 bar_len=(f32)(BS/2)-post_rad;

			// The post - always present
			v3f pos = intToFloat(p+blockpos_nodes, BS);
			f32 postuv[24]={
					0.4,0.4,0.6,0.6,
					0.35,0,0.65,1,
					0.35,0,0.65,1,
					0.35,0,0.65,1,
					0.35,0,0.65,1,
					0.4,0.4,0.6,0.6};
			makeCuboid(material_wood, &collector,
				&pa_wood, c, pos,
				post_rad,BS/2,post_rad, postuv);

			// Now a section of fence, +X, if there's a post there
			v3s16 p2 = p;
			p2.X++;
			MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			if(n2.d == CONTENT_FENCE)
			{
				pos = intToFloat(p+blockpos_nodes, BS);
				pos.X += BS/2;
				pos.Y += BS/4;
				f32 xrailuv[24]={
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6};
				makeCuboid(material_wood, &collector,
					&pa_wood, c, pos,
					bar_len,bar_rad,bar_rad, xrailuv);

				pos.Y -= BS/2;
				makeCuboid(material_wood, &collector,
					&pa_wood, c, pos,
					bar_len,bar_rad,bar_rad, xrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			p2 = p;
			p2.Z++;
			n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			if(n2.d == CONTENT_FENCE)
			{
				pos = intToFloat(p+blockpos_nodes, BS);
				pos.Z += BS/2;
				pos.Y += BS/4;
				f32 zrailuv[24]={
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6,
					0,0.4,1,0.6};
				makeCuboid(material_wood, &collector,
					&pa_wood, c, pos,
					bar_rad,bar_rad,bar_len, zrailuv);
				pos.Y -= BS/2;
				makeCuboid(material_wood, &collector,
					&pa_wood, c, pos,
					bar_rad,bar_rad,bar_len, zrailuv);

			}

		}
#if 1
		/*
			Add stones with minerals if stone is invisible
		*/
		else if(n.d == CONTENT_STONE && invisible_stone && n.getMineral() != MINERAL_NONE)
		{
			for(u32 j=0; j<6; j++)
			{
				// NOTE: Hopefully g_6dirs[j] is the right direction...
				v3s16 dir = g_6dirs[j];
				/*u8 l = 0;
				MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + dir);
				if(content_features(n2.d).param_type == CPT_LIGHT)
					l = decode_light(n2.getLightBlend(data->m_daynight_ratio));
				else
					l = 255;*/
				u8 l = 255;
				video::SColor c(255,l,l,l);
				
				// Get the right texture
				TileSpec ts = n.getTile(dir);
				AtlasPointer ap = ts.texture;
				material_general.setTexture(0, ap.atlas);
				video::S3DVertex vertices[4] =
 				{
					/*video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c, 0,0),*/
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c,
						ap.x0(), ap.y1()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c,
						ap.x1(), ap.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c,
						ap.x1(), ap.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c,
						ap.x0(), ap.y0()),
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

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(material_general, vertices, 4, indices, 6);
			}
		}
#endif
		else if(n.d == CONTENT_PAPYRUS)
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c(255,l,l,l);

			for(u32 j=0; j<4; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,
						pa_papyrus.x0(), pa_papyrus.y1()),
					video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,
						pa_papyrus.x1(), pa_papyrus.y1()),
					video::S3DVertex(BS/2,BS/2,0, 0,0,0, c,
						pa_papyrus.x1(), pa_papyrus.y0()),
					video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c,
						pa_papyrus.x0(), pa_papyrus.y0()),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(material_papyrus, vertices, 4, indices, 6);
			}
		}
		else if(n.d == CONTENT_JUNGLEGRASS)
		{
			u8 l = decode_light(undiminish_light(n.getLightBlend(data->m_daynight_ratio)));
			video::SColor c(255,l,l,l);

			for(u32 j=0; j<4; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,
						pa_papyrus.x0(), pa_papyrus.y1()),
					video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,
						pa_papyrus.x1(), pa_papyrus.y1()),
					video::S3DVertex(BS/2,BS/1,0, 0,0,0, c,
						pa_papyrus.x1(), pa_papyrus.y0()),
					video::S3DVertex(-BS/2,BS/1,0, 0,0,0, c,
						pa_papyrus.x0(), pa_papyrus.y0()),
				};

				if(j == 0)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(45);
				}
				else if(j == 1)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-45);
				}
				else if(j == 2)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(135);
				}
				else if(j == 3)
				{
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-135);
				}

				for(u16 i=0; i<4; i++)
				{
					vertices[i].Pos *= 1.3;
					vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(material_junglegrass, vertices, 4, indices, 6);
			}
		}
		else if(n.d == CONTENT_RAIL)
		{
			u8 l = decode_light(n.getLightBlend(data->m_daynight_ratio));
			video::SColor c(255,l,l,l);

			bool is_rail_x [] = { false, false };  /* x-1, x+1 */
			bool is_rail_z [] = { false, false };  /* z-1, z+1 */

			MapNode n_minus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1,y,z));
			MapNode n_plus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1,y,z));
			MapNode n_minus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z-1));
			MapNode n_plus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z+1));

			if(n_minus_x.d == CONTENT_RAIL)
				is_rail_x[0] = true;
			if(n_plus_x.d == CONTENT_RAIL)
				is_rail_x[1] = true;
			if(n_minus_z.d == CONTENT_RAIL)
				is_rail_z[0] = true;
			if(n_plus_z.d == CONTENT_RAIL)
				is_rail_z[1] = true;

			float d = (float)BS/16;
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,-BS/2+d,-BS/2, 0,0,0, c,
					0, 1),
				video::S3DVertex(BS/2,-BS/2+d,-BS/2, 0,0,0, c,
					1, 1),
				video::S3DVertex(BS/2,-BS/2+d,BS/2, 0,0,0, c,
					1, 0),
				video::S3DVertex(-BS/2,-BS/2+d,BS/2, 0,0,0, c,
					0, 0),
			};

			video::SMaterial material_rail;
			material_rail.setFlag(video::EMF_LIGHTING, false);
			material_rail.setFlag(video::EMF_BACK_FACE_CULLING, false);
			material_rail.setFlag(video::EMF_BILINEAR_FILTER, false);
			material_rail.setFlag(video::EMF_FOG_ENABLE, true);
			material_rail.MaterialType
					= video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF;

			int adjacencies = is_rail_x[0] + is_rail_x[1] + is_rail_z[0] + is_rail_z[1];

			// Assign textures
			if(adjacencies < 2)
				material_rail.setTexture(0, g_texturesource->getTextureRaw("rail.png"));
			else if(adjacencies == 2)
			{
				if((is_rail_x[0] && is_rail_x[1]) || (is_rail_z[0] && is_rail_z[1]))
					material_rail.setTexture(0, g_texturesource->getTextureRaw("rail.png"));
				else
					material_rail.setTexture(0, g_texturesource->getTextureRaw("rail_curved.png"));
			}
			else if(adjacencies == 3)
				material_rail.setTexture(0, g_texturesource->getTextureRaw("rail_t_junction.png"));
			else if(adjacencies == 4)
				material_rail.setTexture(0, g_texturesource->getTextureRaw("rail_crossing.png"));

			// Rotate textures
			int angle = 0;

			if(adjacencies == 1)
			{
				if(is_rail_x[0] || is_rail_x[1])
					angle = 90;
			}
			else if(adjacencies == 2)
			{
				if(is_rail_x[0] && is_rail_x[1])
					angle = 90;
				else if(is_rail_x[0] && is_rail_z[0])
					angle = 270;
				else if(is_rail_x[0] && is_rail_z[1])
					angle = 180;
				else if(is_rail_x[1] && is_rail_z[1])
					angle = 90;
			}
			else if(adjacencies == 3)
			{
				if(!is_rail_x[0])
					angle=0;
				if(!is_rail_x[1])
					angle=180;
				if(!is_rail_z[0])
					angle=90;
				if(!is_rail_z[1])
					angle=270;
			}

			if(angle != 0) {
				for(u16 i=0; i<4; i++)
					vertices[i].Pos.rotateXZBy(angle);
			}

			for(s32 i=0; i<4; i++)
			{
				vertices[i].Pos += intToFloat(p + blockpos_nodes, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			collector.append(material_rail, vertices, 4, indices, 6);
		}
	}
}
#endif

