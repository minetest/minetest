/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "main.h" // For g_settings
#include "mapblock_mesh.h" // For MapBlock_LightColor() and MeshCollector
#include "settings.h"
#include "nodedef.h"
#include "tile.h"
#include "gamedef.h"
#include "util/numeric.h"
#include "util/serialize.h"
#include "util/directiontables.h"

// Create a cuboid.
//  collector - the MeshCollector for the resulting polygons
//  box       - the position and size of the box
//  tiles     - the tiles (materials) to use (for all 6 faces)
//  tilecount - number of entries in tiles, 1<=tilecount<=6
//  c         - vertex colour - used for all
//  txc       - texture coordinates - this is a list of texture coordinates
//              for the opposite corners of each face - therefore, there
//              should be (2+2)*6=24 values in the list. Alternatively, pass
//              NULL to use the entire texture for each face. The order of
//              the faces in the list is up-down-right-left-back-front
//              (compatible with ContentFeatures). If you specified 0,0,1,1
//              for each face, that would be the same as passing NULL.
void makeCuboid(MeshCollector *collector, const aabb3f &box,
	const TileSpec *tiles, int tilecount,
	video::SColor &c, const f32* txc)
{
	assert(tilecount >= 1 && tilecount <= 6);

	v3f min = box.MinEdge;
	v3f max = box.MaxEdge;

	if(txc == NULL)
	{
		static const f32 txc_default[24] = {
			0,0,1,1,
			0,0,1,1,
			0,0,1,1,
			0,0,1,1,
			0,0,1,1,
			0,0,1,1
		};
		txc = txc_default;
	}

	video::S3DVertex vertices[24] =
	{
		// up
		video::S3DVertex(min.X,max.Y,max.Z, 0,1,0, c, txc[0],txc[1]),
		video::S3DVertex(max.X,max.Y,max.Z, 0,1,0, c, txc[2],txc[1]),
		video::S3DVertex(max.X,max.Y,min.Z, 0,1,0, c, txc[2],txc[3]),
		video::S3DVertex(min.X,max.Y,min.Z, 0,1,0, c, txc[0],txc[3]),
		// down
		video::S3DVertex(min.X,min.Y,min.Z, 0,-1,0, c, txc[4],txc[5]),
		video::S3DVertex(max.X,min.Y,min.Z, 0,-1,0, c, txc[6],txc[5]),
		video::S3DVertex(max.X,min.Y,max.Z, 0,-1,0, c, txc[6],txc[7]),
		video::S3DVertex(min.X,min.Y,max.Z, 0,-1,0, c, txc[4],txc[7]),
		// right
		video::S3DVertex(max.X,max.Y,min.Z, 1,0,0, c, txc[ 8],txc[9]),
		video::S3DVertex(max.X,max.Y,max.Z, 1,0,0, c, txc[10],txc[9]),
		video::S3DVertex(max.X,min.Y,max.Z, 1,0,0, c, txc[10],txc[11]),
		video::S3DVertex(max.X,min.Y,min.Z, 1,0,0, c, txc[ 8],txc[11]),
		// left
		video::S3DVertex(min.X,max.Y,max.Z, -1,0,0, c, txc[12],txc[13]),
		video::S3DVertex(min.X,max.Y,min.Z, -1,0,0, c, txc[14],txc[13]),
		video::S3DVertex(min.X,min.Y,min.Z, -1,0,0, c, txc[14],txc[15]),
		video::S3DVertex(min.X,min.Y,max.Z, -1,0,0, c, txc[12],txc[15]),
		// back
		video::S3DVertex(max.X,max.Y,max.Z, 0,0,1, c, txc[16],txc[17]),
		video::S3DVertex(min.X,max.Y,max.Z, 0,0,1, c, txc[18],txc[17]),
		video::S3DVertex(min.X,min.Y,max.Z, 0,0,1, c, txc[18],txc[19]),
		video::S3DVertex(max.X,min.Y,max.Z, 0,0,1, c, txc[16],txc[19]),
		// front
		video::S3DVertex(min.X,max.Y,min.Z, 0,0,-1, c, txc[20],txc[21]),
		video::S3DVertex(max.X,max.Y,min.Z, 0,0,-1, c, txc[22],txc[21]),
		video::S3DVertex(max.X,min.Y,min.Z, 0,0,-1, c, txc[22],txc[23]),
		video::S3DVertex(min.X,min.Y,min.Z, 0,0,-1, c, txc[20],txc[23]),
	};

	for(s32 j=0; j<24; j++)
	{
		int tileindex = MYMIN(j/4, tilecount-1);
		vertices[j].TCoords *= tiles[tileindex].texture.size;
		vertices[j].TCoords += tiles[tileindex].texture.pos;
	}

	u16 indices[] = {0,1,2,2,3,0};

	// Add to mesh collector
	for(s32 j=0; j<24; j+=4)
	{
		int tileindex = MYMIN(j/4, tilecount-1);
		collector->append(tiles[tileindex],
				vertices+j, 4, indices, 6);
	}
}

void mapblock_mesh_generate_special(MeshMakeData *data,
		MeshCollector &collector)
{
	INodeDefManager *nodedef = data->m_gamedef->ndef();

	// 0ms
	//TimeTaker timer("mapblock_mesh_generate_special()");

	/*
		Some settings
	*/
	bool new_style_water = g_settings->getBool("new_style_water");
	
	float node_liquid_level = 1.0;
	if(new_style_water)
		node_liquid_level = 0.85;
	
	v3s16 blockpos_nodes = data->m_blockpos*MAP_BLOCKSIZE;

	for(s16 z=0; z<MAP_BLOCKSIZE; z++)
	for(s16 y=0; y<MAP_BLOCKSIZE; y++)
	for(s16 x=0; x<MAP_BLOCKSIZE; x++)
	{
		v3s16 p(x,y,z);

		MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes+p);
		const ContentFeatures &f = nodedef->get(n);

		// Only solidness=0 stuff is drawn here
		if(f.solidness != 0)
			continue;
		
		switch(f.drawtype){
		default:
			infostream<<"Got "<<f.drawtype<<std::endl;
			assert(0);
			break;
		case NDT_AIRLIKE:
			break;
		case NDT_LIQUID:
		{
			/*
				Add water sources to mesh if using new style
			*/
			TileSpec tile_liquid = f.special_tiles[0];
			AtlasPointer &pa_liquid = tile_liquid.texture;

			bool top_is_air = false;
			MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y+1,z));
			if(n.getContent() == CONTENT_AIR)
				top_is_air = true;
			
			if(top_is_air == false)
				continue;

			u16 l = getInteriorLight(n, 0, data);
			video::SColor c = MapBlock_LightColor(f.alpha, l);
			
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
						pa_liquid.x0(), pa_liquid.y1()),
				video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
						pa_liquid.x1(), pa_liquid.y1()),
				video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c,
						pa_liquid.x1(), pa_liquid.y0()),
				video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c,
						pa_liquid.x0(), pa_liquid.y0()),
			};

			v3f offset(p.X, p.Y + (-0.5+node_liquid_level)*BS, p.Z);
			for(s32 i=0; i<4; i++)
			{
				vertices[i].Pos += offset;
			}

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(tile_liquid, vertices, 4, indices, 6);
		break;}
		case NDT_FLOWINGLIQUID:
		{
			/*
				Add flowing liquid to mesh
			*/
			TileSpec tile_liquid = f.special_tiles[0];
			TileSpec tile_liquid_bfculled = f.special_tiles[1];
			AtlasPointer &pa_liquid = tile_liquid.texture;

			bool top_is_same_liquid = false;
			MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y+1,z));
			content_t c_flowing = nodedef->getId(f.liquid_alternative_flowing);
			content_t c_source = nodedef->getId(f.liquid_alternative_source);
			if(ntop.getContent() == c_flowing || ntop.getContent() == c_source)
				top_is_same_liquid = true;
			
			u16 l = 0;
			// If this liquid emits light and doesn't contain light, draw
			// it at what it emits, for an increased effect
			u8 light_source = nodedef->get(n).light_source;
			if(light_source != 0){
				//l = decode_light(undiminish_light(light_source));
				l = decode_light(light_source);
				l = l | (l<<8);
			}
			// Use the light of the node on top if possible
			else if(nodedef->get(ntop).param_type == CPT_LIGHT)
				l = getInteriorLight(ntop, 0, data);
			// Otherwise use the light of this node (the liquid)
			else
				l = getInteriorLight(n, 0, data);
			video::SColor c = MapBlock_LightColor(f.alpha, l);
			
			// Neighbor liquid levels (key = relative position)
			// Includes current node
			core::map<v3s16, f32> neighbor_levels;
			core::map<v3s16, content_t> neighbor_contents;
			core::map<v3s16, u8> neighbor_flags;
			const u8 neighborflag_top_is_same_liquid = 0x01;
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
				content_t content = CONTENT_AIR;
				float level = -0.5 * BS;
				u8 flags = 0;
				// Check neighbor
				v3s16 p2 = p + neighbor_dirs[i];
				MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
				if(n2.getContent() != CONTENT_IGNORE)
				{
					content = n2.getContent();

					if(n2.getContent() == c_source)
						level = (-0.5+node_liquid_level) * BS;
					else if(n2.getContent() == c_flowing)
						level = (-0.5 + ((float)(n2.param2&LIQUID_LEVEL_MASK)
								+ 0.5) / 8.0 * node_liquid_level) * BS;

					// Check node above neighbor.
					// NOTE: This doesn't get executed if neighbor
					//       doesn't exist
					p2.Y += 1;
					n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
					if(n2.getContent() == c_source ||
							n2.getContent() == c_flowing)
						flags |= neighborflag_top_is_same_liquid;
				}
				
				neighbor_levels.insert(neighbor_dirs[i], level);
				neighbor_contents.insert(neighbor_dirs[i], content);
				neighbor_flags.insert(neighbor_dirs[i], flags);
			}

			// Corner heights (average between four liquids)
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
				u32 air_count = 0;
				for(u32 j=0; j<4; j++)
				{
					v3s16 neighbordir = cornerdir - halfdirs[j];
					content_t content = neighbor_contents[neighbordir];
					// If top is liquid, draw starting from top of node
					if(neighbor_flags[neighbordir] &
							neighborflag_top_is_same_liquid)
					{
						cornerlevel = 0.5*BS;
						valid_count = 1;
						break;
					}
					// Source is always the same height
					else if(content == c_source)
					{
						cornerlevel = (-0.5+node_liquid_level)*BS;
						valid_count = 1;
						break;
					}
					// Flowing liquid has level information
					else if(content == c_flowing)
					{
						cornerlevel += neighbor_levels[neighbordir];
						valid_count++;
					}
					else if(content == CONTENT_AIR)
					{
						air_count++;
					}
				}
				if(air_count >= 2)
					cornerlevel = -0.5*BS;
				else if(valid_count > 0)
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
					If our topside is liquid and neighbor's topside
					is liquid, don't draw side face
				*/
				if(top_is_same_liquid &&
						neighbor_flags[dir] & neighborflag_top_is_same_liquid)
					continue;

				content_t neighbor_content = neighbor_contents[dir];
				const ContentFeatures &n_feat = nodedef->get(neighbor_content);
				
				// Don't draw face if neighbor is blocking the view
				if(n_feat.solidness == 2)
					continue;
				
				bool neighbor_is_same_liquid = (neighbor_content == c_source
						|| neighbor_content == c_flowing);
				
				// Don't draw any faces if neighbor same is liquid and top is
				// same liquid
				if(neighbor_is_same_liquid == true
						&& top_is_same_liquid == false)
					continue;

				// Use backface culled material if neighbor doesn't have a
				// solidness of 0
				const TileSpec *current_tile = &tile_liquid;
				if(n_feat.solidness != 0 || n_feat.visual_solidness != 0)
					current_tile = &tile_liquid_bfculled;
				
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							pa_liquid.x0(), pa_liquid.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							pa_liquid.x1(), pa_liquid.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							pa_liquid.x1(), pa_liquid.y0()),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							pa_liquid.x0(), pa_liquid.y0()),
				};
				
				/*
					If our topside is liquid, set upper border of face
					at upper border of node
				*/
				if(top_is_same_liquid)
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
					If neighbor is liquid, lower border of face is corner
					liquid levels
				*/
				if(neighbor_is_same_liquid)
				{
					vertices[0].Pos.Y = corner_levels[side_corners[i][1]];
					vertices[1].Pos.Y = corner_levels[side_corners[i][0]];
				}
				/*
					If neighbor is not liquid, lower border of face is
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
						
					// Do this to not cause glitches when two liquids are
					// side-by-side
					/*if(neighbor_is_same_liquid == false){
						vertices[j].Pos.X *= 0.98;
						vertices[j].Pos.Z *= 0.98;
					}*/

					vertices[j].Pos += intToFloat(p, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(*current_tile, vertices, 4, indices, 6);
			}
			
			/*
				Generate top side, if appropriate
			*/
			
			if(top_is_same_liquid == false)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c,
							pa_liquid.x0(), pa_liquid.y1()),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c,
							pa_liquid.x1(), pa_liquid.y1()),
					video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c,
							pa_liquid.x1(), pa_liquid.y0()),
					video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c,
							pa_liquid.x0(), pa_liquid.y0()),
				};
				
				// To get backface culling right, the vertices need to go
				// clockwise around the front of the face. And we happened to
				// calculate corner levels in exact reverse order.
				s32 corner_resolve[4] = {3,2,1,0};

				for(s32 i=0; i<4; i++)
				{
					//vertices[i].Pos.Y += liquid_level;
					//vertices[i].Pos.Y += neighbor_levels[v3s16(0,0,0)];
					s32 j = corner_resolve[i];
					vertices[i].Pos.Y += corner_levels[j];
					vertices[i].Pos += intToFloat(p, BS);
				}
				
				// Default downwards-flowing texture animation goes from 
				// -Z towards +Z, thus the direction is +Z.
				// Rotate texture to make animation go in flow direction
				// Positive if liquid moves towards +Z
				int dz = (corner_levels[side_corners[2][0]] +
						corner_levels[side_corners[2][1]] <
						corner_levels[side_corners[3][0]] +
						corner_levels[side_corners[3][1]]);
				// Positive if liquid moves towards +X
				int dx = (corner_levels[side_corners[0][0]] +
						corner_levels[side_corners[0][1]] <
						corner_levels[side_corners[1][0]] +
						corner_levels[side_corners[1][1]]);
				// -X
				if(-dx >= abs(dz))
				{
					v2f t = vertices[0].TCoords;
					vertices[0].TCoords = vertices[1].TCoords;
					vertices[1].TCoords = vertices[2].TCoords;
					vertices[2].TCoords = vertices[3].TCoords;
					vertices[3].TCoords = t;
				}
				// +X
				if(dx >= abs(dz))
				{
					v2f t = vertices[0].TCoords;
					vertices[0].TCoords = vertices[3].TCoords;
					vertices[3].TCoords = vertices[2].TCoords;
					vertices[2].TCoords = vertices[1].TCoords;
					vertices[1].TCoords = t;
				}
				// -Z
				if(-dz >= abs(dx))
				{
					v2f t = vertices[0].TCoords;
					vertices[0].TCoords = vertices[3].TCoords;
					vertices[3].TCoords = vertices[2].TCoords;
					vertices[2].TCoords = vertices[1].TCoords;
					vertices[1].TCoords = t;
					t = vertices[0].TCoords;
					vertices[0].TCoords = vertices[3].TCoords;
					vertices[3].TCoords = vertices[2].TCoords;
					vertices[2].TCoords = vertices[1].TCoords;
					vertices[1].TCoords = t;
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(tile_liquid, vertices, 4, indices, 6);
			}
		break;}
		case NDT_GLASSLIKE:
		{
			TileSpec tile = getNodeTile(n, p, v3s16(0,0,0), data);
			AtlasPointer ap = tile.texture;

			u16 l = getInteriorLight(n, 1, data);
			video::SColor c = MapBlock_LightColor(255, l);

			for(u32 j=0; j<6; j++)
			{
				// Check this neighbor
				v3s16 n2p = blockpos_nodes + p + g_6dirs[j];
				MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
				// Don't make face if neighbor is of same type
				if(n2.getContent() == n.getContent())
					continue;

				// The face at Z+
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c,
						ap.x0(), ap.y1()),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c,
						ap.x1(), ap.y1()),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c,
						ap.x1(), ap.y0()),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c,
						ap.x0(), ap.y0()),
				};
				
				// Rotations in the g_6dirs format
				if(j == 0) // Z+
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(0);
				else if(j == 1) // Y+
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateYZBy(-90);
				else if(j == 2) // X+
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(-90);
				else if(j == 3) // Z-
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(180);
				else if(j == 4) // Y-
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateYZBy(90);
				else if(j == 5) // X-
					for(u16 i=0; i<4; i++)
						vertices[i].Pos.rotateXZBy(90);

				for(u16 i=0; i<4; i++){
					vertices[i].Pos += intToFloat(p, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(tile, vertices, 4, indices, 6);
			}
		break;}
		case NDT_ALLFACES:
		{
			TileSpec tile_leaves = getNodeTile(n, p,
					v3s16(0,0,0), data);
			AtlasPointer pa_leaves = tile_leaves.texture;

			u16 l = getInteriorLight(n, 1, data);
			video::SColor c = MapBlock_LightColor(255, l);

			v3f pos = intToFloat(p, BS);
			aabb3f box(-BS/2,-BS/2,-BS/2,BS/2,BS/2,BS/2);
			box.MinEdge += pos;
			box.MaxEdge += pos;
			makeCuboid(&collector, box, &tile_leaves, 1, c, NULL);
		break;}
		case NDT_ALLFACES_OPTIONAL:
			// This is always pre-converted to something else
			assert(0);
			break;
		case NDT_TORCHLIKE:
		{
			v3s16 dir = n.getWallMountedDir(nodedef);
			
			u8 tileindex = 0;
			if(dir == v3s16(0,-1,0)){
				tileindex = 0; // floor
			} else if(dir == v3s16(0,1,0)){
				tileindex = 1; // ceiling
			// For backwards compatibility
			} else if(dir == v3s16(0,0,0)){
				tileindex = 0; // floor
			} else {
				tileindex = 2; // side
			}

			TileSpec tile = getNodeTileN(n, p, tileindex, data);
			tile.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
			tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;

			AtlasPointer ap = tile.texture;

			video::SColor c(255,255,255,255);

			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,-BS/2,0, 0,0,0, c,
						ap.x0(), ap.y1()),
				video::S3DVertex(BS/2,-BS/2,0, 0,0,0, c,
						ap.x1(), ap.y1()),
				video::S3DVertex(BS/2,BS/2,0, 0,0,0, c,
						ap.x1(), ap.y0()),
				video::S3DVertex(-BS/2,BS/2,0, 0,0,0, c,
						ap.x0(), ap.y0()),
			};

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

				vertices[i].Pos += intToFloat(p, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(tile, vertices, 4, indices, 6);
		break;}
		case NDT_SIGNLIKE:
		{
			TileSpec tile = getNodeTileN(n, p, 0, data);
			tile.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
			tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;
			AtlasPointer ap = tile.texture;

			u16 l = getInteriorLight(n, 0, data);
			video::SColor c = MapBlock_LightColor(255, l);
				
			float d = (float)BS/16;
			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(BS/2-d,BS/2,BS/2, 0,0,0, c,
						ap.x0(), ap.y0()),
				video::S3DVertex(BS/2-d,BS/2,-BS/2, 0,0,0, c,
						ap.x1(), ap.y0()),
				video::S3DVertex(BS/2-d,-BS/2,-BS/2, 0,0,0, c,
						ap.x1(), ap.y1()),
				video::S3DVertex(BS/2-d,-BS/2,BS/2, 0,0,0, c,
						ap.x0(), ap.y1()),
			};

			v3s16 dir = n.getWallMountedDir(nodedef);

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

				vertices[i].Pos += intToFloat(p, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			// Add to mesh collector
			collector.append(tile, vertices, 4, indices, 6);
		break;}
		case NDT_PLANTLIKE:
		{
			TileSpec tile = getNodeTileN(n, p, 0, data);
			tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;
			AtlasPointer ap = tile.texture;
			
			u16 l = getInteriorLight(n, 1, data);
			video::SColor c = MapBlock_LightColor(255, l);

			for(u32 j=0; j<4; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2*f.visual_scale,-BS/2,0, 0,0,0, c,
						ap.x0(), ap.y1()),
					video::S3DVertex( BS/2*f.visual_scale,-BS/2,0, 0,0,0, c,
						ap.x1(), ap.y1()),
					video::S3DVertex( BS/2*f.visual_scale,
						-BS/2 + f.visual_scale*BS,0, 0,0,0, c,
						ap.x1(), ap.y0()),
					video::S3DVertex(-BS/2*f.visual_scale,
						-BS/2 + f.visual_scale*BS,0, 0,0,0, c,
						ap.x0(), ap.y0()),
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
					vertices[i].Pos *= f.visual_scale;
					vertices[i].Pos += intToFloat(p, BS);
				}

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(tile, vertices, 4, indices, 6);
			}
		break;}
		case NDT_FENCELIKE:
		{
			TileSpec tile = getNodeTile(n, p, v3s16(0,0,0), data);
			TileSpec tile_nocrack = tile;
			tile_nocrack.material_flags &= ~MATERIAL_FLAG_CRACK;
			
			// A hack to put wood the right way around in the posts
			ITextureSource *tsrc = data->m_gamedef->tsrc();
			TileSpec tile_rot = tile;
			tile_rot.texture = tsrc->getTexture(tsrc->getTextureName(
					tile.texture.id) + "^[transformR90");
					
			u16 l = getInteriorLight(n, 1, data);
			video::SColor c = MapBlock_LightColor(255, l);

			const f32 post_rad=(f32)BS/8;
			const f32 bar_rad=(f32)BS/16;
			const f32 bar_len=(f32)(BS/2)-post_rad;

			v3f pos = intToFloat(p, BS);

			// The post - always present
			aabb3f post(-post_rad,-BS/2,-post_rad,post_rad,BS/2,post_rad);
			post.MinEdge += pos;
			post.MaxEdge += pos;
			f32 postuv[24]={
					6/16.,6/16.,10/16.,10/16.,
					6/16.,6/16.,10/16.,10/16.,
					0/16.,0,4/16.,1,
					4/16.,0,8/16.,1,
					8/16.,0,12/16.,1,
					12/16.,0,16/16.,1};
			makeCuboid(&collector, post, &tile_rot, 1, c, postuv);

			// Now a section of fence, +X, if there's a post there
			v3s16 p2 = p;
			p2.X++;
			MapNode n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			const ContentFeatures *f2 = &nodedef->get(n2);
			if(f2->drawtype == NDT_FENCELIKE)
			{
				aabb3f bar(-bar_len+BS/2,-bar_rad+BS/4,-bar_rad,
						bar_len+BS/2,bar_rad+BS/4,bar_rad);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 xrailuv[24]={
					0/16.,2/16.,16/16.,4/16.,
					0/16.,4/16.,16/16.,6/16.,
					6/16.,6/16.,8/16.,8/16.,
					10/16.,10/16.,12/16.,12/16.,
					0/16.,8/16.,16/16.,10/16.,
					0/16.,14/16.,16/16.,16/16.};
				makeCuboid(&collector, bar, &tile_nocrack, 1,
						c, xrailuv);
				bar.MinEdge.Y -= BS/2;
				bar.MaxEdge.Y -= BS/2;
				makeCuboid(&collector, bar, &tile_nocrack, 1,
						c, xrailuv);
			}

			// Now a section of fence, +Z, if there's a post there
			p2 = p;
			p2.Z++;
			n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
			f2 = &nodedef->get(n2);
			if(f2->drawtype == NDT_FENCELIKE)
			{
				aabb3f bar(-bar_rad,-bar_rad+BS/4,-bar_len+BS/2,
						bar_rad,bar_rad+BS/4,bar_len+BS/2);
				bar.MinEdge += pos;
				bar.MaxEdge += pos;
				f32 zrailuv[24]={
					3/16.,1/16.,5/16.,5/16., // cannot rotate; stretch
					4/16.,1/16.,6/16.,5/16., // for wood texture instead
					0/16.,9/16.,16/16.,11/16.,
					0/16.,6/16.,16/16.,8/16.,
					6/16.,6/16.,8/16.,8/16.,
					10/16.,10/16.,12/16.,12/16.};
				makeCuboid(&collector, bar, &tile_nocrack, 1,
						c, zrailuv);
				bar.MinEdge.Y -= BS/2;
				bar.MaxEdge.Y -= BS/2;
				makeCuboid(&collector, bar, &tile_nocrack, 1,
						c, zrailuv);
			}
		break;}
		case NDT_RAILLIKE:
		{
			bool is_rail_x [] = { false, false };  /* x-1, x+1 */
			bool is_rail_z [] = { false, false };  /* z-1, z+1 */

			bool is_rail_z_minus_y [] = { false, false };  /* z-1, z+1; y-1 */
			bool is_rail_x_minus_y [] = { false, false };  /* x-1, z+1; y-1 */
			bool is_rail_z_plus_y [] = { false, false };  /* z-1, z+1; y+1 */
			bool is_rail_x_plus_y [] = { false, false };  /* x-1, x+1; y+1 */

			MapNode n_minus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1,y,z));
			MapNode n_plus_x = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1,y,z));
			MapNode n_minus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z-1));
			MapNode n_plus_z = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y,z+1));
			MapNode n_plus_x_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y+1, z));
			MapNode n_plus_x_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x+1, y-1, z));
			MapNode n_minus_x_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y+1, z));
			MapNode n_minus_x_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x-1, y-1, z));
			MapNode n_plus_z_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z+1));
			MapNode n_minus_z_plus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y+1, z-1));
			MapNode n_plus_z_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y-1, z+1));
			MapNode n_minus_z_minus_y = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x, y-1, z-1));
			
			content_t thiscontent = n.getContent();
			if(n_minus_x.getContent() == thiscontent)
				is_rail_x[0] = true;
			if (n_minus_x_minus_y.getContent() == thiscontent)
				is_rail_x_minus_y[0] = true;
			if(n_minus_x_plus_y.getContent() == thiscontent)
				is_rail_x_plus_y[0] = true;

			if(n_plus_x.getContent() == thiscontent)
				is_rail_x[1] = true;
			if (n_plus_x_minus_y.getContent() == thiscontent)
				is_rail_x_minus_y[1] = true;
			if(n_plus_x_plus_y.getContent() == thiscontent)
				is_rail_x_plus_y[1] = true;

			if(n_minus_z.getContent() == thiscontent)
				is_rail_z[0] = true;
			if (n_minus_z_minus_y.getContent() == thiscontent)
				is_rail_z_minus_y[0] = true;
			if(n_minus_z_plus_y.getContent() == thiscontent)
				is_rail_z_plus_y[0] = true;

			if(n_plus_z.getContent() == thiscontent)
				is_rail_z[1] = true;
			if (n_plus_z_minus_y.getContent() == thiscontent)
				is_rail_z_minus_y[1] = true;
			if(n_plus_z_plus_y.getContent() == thiscontent)
				is_rail_z_plus_y[1] = true;

			bool is_rail_x_all[] = {false, false};
			bool is_rail_z_all[] = {false, false};
			is_rail_x_all[0]=is_rail_x[0] || is_rail_x_minus_y[0] || is_rail_x_plus_y[0];
			is_rail_x_all[1]=is_rail_x[1] || is_rail_x_minus_y[1] || is_rail_x_plus_y[1];
			is_rail_z_all[0]=is_rail_z[0] || is_rail_z_minus_y[0] || is_rail_z_plus_y[0];
			is_rail_z_all[1]=is_rail_z[1] || is_rail_z_minus_y[1] || is_rail_z_plus_y[1];

			// reasonable default, flat straight unrotated rail
			bool is_straight = true;
			int adjacencies = 0;
			int angle = 0;
			u8 tileindex = 0;

			// check for sloped rail
			if (is_rail_x_plus_y[0] || is_rail_x_plus_y[1] || is_rail_z_plus_y[0] || is_rail_z_plus_y[1])
			{
				adjacencies = 5; //5 means sloped
				is_straight = true; // sloped is always straight
			}
			else
			{
				// is really straight, rails on both sides
				is_straight = (is_rail_x_all[0] && is_rail_x_all[1]) || (is_rail_z_all[0] && is_rail_z_all[1]);
				adjacencies = is_rail_x_all[0] + is_rail_x_all[1] + is_rail_z_all[0] + is_rail_z_all[1];
			}

			switch (adjacencies) {
			case 1:
				if(is_rail_x_all[0] || is_rail_x_all[1])
					angle = 90;
				break;
			case 2:
				if(!is_straight)
					tileindex = 1; // curved
				if(is_rail_x_all[0] && is_rail_x_all[1])
					angle = 90;
				if(is_rail_z_all[0] && is_rail_z_all[1]){
					if (n_minus_z_plus_y.getContent() == thiscontent) angle = 180;
				}
				else if(is_rail_x_all[0] && is_rail_z_all[0])
					angle = 270;
				else if(is_rail_x_all[0] && is_rail_z_all[1])
					angle = 180;
				else if(is_rail_x_all[1] && is_rail_z_all[1])
					angle = 90;
				break;
			case 3:
				// here is where the potential to 'switch' a junction is, but not implemented at present
				tileindex = 2; // t-junction
				if(!is_rail_x_all[1])
					angle=180;
				if(!is_rail_z_all[0])
					angle=90;
				if(!is_rail_z_all[1])
					angle=270;
				break;
			case 4:
				tileindex = 3; // crossing
				break;
			case 5: //sloped
				if(is_rail_z_plus_y[0])
					angle = 180;
				if(is_rail_x_plus_y[0])
					angle = 90;
				if(is_rail_x_plus_y[1])
					angle = -90;
				break;
			default:
				break;
			}

			TileSpec tile = getNodeTileN(n, p, tileindex, data);
			tile.material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
			tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;

			AtlasPointer ap = tile.texture;
			
			u16 l = getInteriorLight(n, 0, data);
			video::SColor c = MapBlock_LightColor(255, l);

			float d = (float)BS/64;
			
			char g=-1;
			if (is_rail_x_plus_y[0] || is_rail_x_plus_y[1] || is_rail_z_plus_y[0] || is_rail_z_plus_y[1])
				g=1; //Object is at a slope

			video::S3DVertex vertices[4] =
			{
					video::S3DVertex(-BS/2,-BS/2+d,-BS/2, 0,0,0, c,
							ap.x0(), ap.y1()),
					video::S3DVertex(BS/2,-BS/2+d,-BS/2, 0,0,0, c,
							ap.x1(), ap.y1()),
					video::S3DVertex(BS/2,g*BS/2+d,BS/2, 0,0,0, c,
							ap.x1(), ap.y0()),
					video::S3DVertex(-BS/2,g*BS/2+d,BS/2, 0,0,0, c,
							ap.x0(), ap.y0()),
			};

			for(s32 i=0; i<4; i++)
			{
				if(angle != 0)
					vertices[i].Pos.rotateXZBy(angle);
				vertices[i].Pos += intToFloat(p, BS);
			}

			u16 indices[] = {0,1,2,2,3,0};
			collector.append(tile, vertices, 4, indices, 6);
		break;}
		case NDT_NODEBOX:
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
			for(int i = 0; i < 6; i++)
			{
				// Handles facedir rotation for textures
				tiles[i] = getNodeTile(n, p, tile_dirs[i], data);
			}

			u16 l = getInteriorLight(n, 0, data);
			video::SColor c = MapBlock_LightColor(255, l);

			v3f pos = intToFloat(p, BS);

			std::vector<aabb3f> boxes = n.getNodeBoxes(nodedef);
			for(std::vector<aabb3f>::iterator
					i = boxes.begin();
					i != boxes.end(); i++)
			{
				aabb3f box = *i;
				box.MinEdge += pos;
				box.MaxEdge += pos;

				// Compute texture coords
				f32 tx1 = (i->MinEdge.X/BS)+0.5;
				f32 ty1 = (i->MinEdge.Y/BS)+0.5;
				f32 tz1 = (i->MinEdge.Z/BS)+0.5;
				f32 tx2 = (i->MaxEdge.X/BS)+0.5;
				f32 ty2 = (i->MaxEdge.Y/BS)+0.5;
				f32 tz2 = (i->MaxEdge.Z/BS)+0.5;
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

				makeCuboid(&collector, box, tiles, 6, c, txc);
			}
		break;}
		}
	}
}

