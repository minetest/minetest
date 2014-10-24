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

#include "content_mapblock.h"
#include "util/numeric.h"
#include "util/directiontables.h"
#include "main.h" // For g_settings
#include "mapblock_mesh.h" // For MapBlock_LightColor() and MeshCollector
#include "settings.h"
#include "nodedef.h"
#include "tile.h"
#include "gamedef.h"
#include "log.h"


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
	TileSpec *tiles, int tilecount,
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

	for(int i = 0; i < 6; i++)
				{
				switch (tiles[MYMIN(i, tilecount-1)].rotation)
				{
				case 0:
					break;
				case 1: //R90
					for (int x = 0; x < 4; x++)
						vertices[i*4+x].TCoords.rotateBy(90,irr::core::vector2df(0, 0));
					break;
				case 2: //R180
					for (int x = 0; x < 4; x++)
						vertices[i*4+x].TCoords.rotateBy(180,irr::core::vector2df(0, 0));
					break;
				case 3: //R270
					for (int x = 0; x < 4; x++)
						vertices[i*4+x].TCoords.rotateBy(270,irr::core::vector2df(0, 0));
					break;
				case 4: //FXR90
					for (int x = 0; x < 4; x++){
						vertices[i*4+x].TCoords.X = 1.0 - vertices[i*4+x].TCoords.X;
						vertices[i*4+x].TCoords.rotateBy(90,irr::core::vector2df(0, 0));
					}
					break;
				case 5: //FXR270
					for (int x = 0; x < 4; x++){
						vertices[i*4+x].TCoords.X = 1.0 - vertices[i*4+x].TCoords.X;
						vertices[i*4+x].TCoords.rotateBy(270,irr::core::vector2df(0, 0));
					}
					break;
				case 6: //FYR90
					for (int x = 0; x < 4; x++){
						vertices[i*4+x].TCoords.Y = 1.0 - vertices[i*4+x].TCoords.Y;
						vertices[i*4+x].TCoords.rotateBy(90,irr::core::vector2df(0, 0));
					}
					break;
				case 7: //FYR270
					for (int x = 0; x < 4; x++){
						vertices[i*4+x].TCoords.Y = 1.0 - vertices[i*4+x].TCoords.Y;
						vertices[i*4+x].TCoords.rotateBy(270,irr::core::vector2df(0, 0));
					}
					break;
				case 8: //FX
					for (int x = 0; x < 4; x++){
						vertices[i*4+x].TCoords.X = 1.0 - vertices[i*4+x].TCoords.X;
					}
					break;
				case 9: //FY
					for (int x = 0; x < 4; x++){
						vertices[i*4+x].TCoords.Y = 1.0 - vertices[i*4+x].TCoords.Y;
					}
					break;
				default:
					break;
				}
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
	ITextureSource *tsrc = data->m_gamedef->tsrc();

	// 0ms
	//TimeTaker timer("mapblock_mesh_generate_special()");

	/*
		Some settings
	*/
	bool new_style_water = g_settings->getBool("new_style_water");

	float node_liquid_level = 1.0;
	if (new_style_water)
		node_liquid_level = 0.85;

	v3s16 blockpos_nodes = data->m_blockpos*MAP_BLOCKSIZE;

	// Create selection mesh
	v3s16 p = data->m_highlighted_pos_relative;
	if (data->m_show_hud &&
			(p.X >= 0) && (p.X < MAP_BLOCKSIZE) &&
			(p.Y >= 0) && (p.Y < MAP_BLOCKSIZE) &&
			(p.Z >= 0) && (p.Z < MAP_BLOCKSIZE)) {

		MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes + p);
		if(n.getContent() != CONTENT_AIR) {
			// Get selection mesh light level
			static const v3s16 dirs[7] = {
					v3s16( 0, 0, 0),
					v3s16( 0, 1, 0),
					v3s16( 0,-1, 0),
					v3s16( 1, 0, 0),
					v3s16(-1, 0, 0),
					v3s16( 0, 0, 1),
					v3s16( 0, 0,-1)
			};

			u16 l = 0;
			u16 l1 = 0;
			for (u8 i = 0; i < 7; i++) {
				MapNode n1 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p + dirs[i]);	
				l1 = getInteriorLight(n1, -4, nodedef);
				if (l1 > l) 
					l = l1;
			}
			video::SColor c = MapBlock_LightColor(255, l, 0);
			data->m_highlight_mesh_color = c;
			std::vector<aabb3f> boxes = n.getSelectionBoxes(nodedef);
			TileSpec h_tile;			
			h_tile.material_flags |= MATERIAL_FLAG_HIGHLIGHTED;
			h_tile.texture = tsrc->getTexture("halo.png",&h_tile.texture_id);
			v3f pos = intToFloat(p, BS);
			f32 d = 0.05 * BS;
			for(std::vector<aabb3f>::iterator
					i = boxes.begin();
					i != boxes.end(); i++)
			{
				aabb3f box = *i;
				box.MinEdge += v3f(-d, -d, -d) + pos;
				box.MaxEdge += v3f(d, d, d) + pos;
				makeCuboid(&collector, box, &h_tile, 1, c, NULL);
			}
		}
	}

	for(s16 z = 0; z < MAP_BLOCKSIZE; z++)
	for(s16 y = 0; y < MAP_BLOCKSIZE; y++)
	for(s16 x = 0; x < MAP_BLOCKSIZE; x++)
	{
		v3s16 p(x,y,z);

		MapNode n = data->m_vmanip.getNodeNoEx(blockpos_nodes + p);
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
			TileSpec tile_liquid_bfculled = getNodeTile(n, p, v3s16(0,0,0), data);

			bool top_is_same_liquid = false;
			MapNode ntop = data->m_vmanip.getNodeNoEx(blockpos_nodes + v3s16(x,y+1,z));
			content_t c_flowing = nodedef->getId(f.liquid_alternative_flowing);
			content_t c_source = nodedef->getId(f.liquid_alternative_source);
			if(ntop.getContent() == c_flowing || ntop.getContent() == c_source)
				top_is_same_liquid = true;

			u16 l = getInteriorLight(n, 0, nodedef);
			video::SColor c = MapBlock_LightColor(f.alpha, l, f.light_source);

			/*
				Generate sides
			 */
			v3s16 side_dirs[4] = {
				v3s16(1,0,0),
				v3s16(-1,0,0),
				v3s16(0,0,1),
				v3s16(0,0,-1),
			};
			for(u32 i=0; i<4; i++)
			{
				v3s16 dir = side_dirs[i];

				MapNode neighbor = data->m_vmanip.getNodeNoEx(blockpos_nodes + p + dir);
				content_t neighbor_content = neighbor.getContent();
				const ContentFeatures &n_feat = nodedef->get(neighbor_content);
				MapNode n_top = data->m_vmanip.getNodeNoEx(blockpos_nodes + p + dir+ v3s16(0,1,0));
				content_t n_top_c = n_top.getContent();

				if(neighbor_content == CONTENT_IGNORE)
					continue;

				/*
					If our topside is liquid and neighbor's topside
					is liquid, don't draw side face
				*/
				if(top_is_same_liquid && (n_top_c == c_flowing ||
						n_top_c == c_source || n_top_c == CONTENT_IGNORE))
					continue;

				// Don't draw face if neighbor is blocking the view
				if(n_feat.solidness == 2)
					continue;

				bool neighbor_is_same_liquid = (neighbor_content == c_source
						|| neighbor_content == c_flowing);

				// Don't draw any faces if neighbor same is liquid and top is
				// same liquid
				if(neighbor_is_same_liquid && !top_is_same_liquid)
					continue;

				// Use backface culled material if neighbor doesn't have a
				// solidness of 0
				const TileSpec *current_tile = &tile_liquid;
				if(n_feat.solidness != 0 || n_feat.visual_solidness != 0)
					current_tile = &tile_liquid_bfculled;

				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-BS/2,0,BS/2,0,0,0, c, 0,1),
					video::S3DVertex(BS/2,0,BS/2,0,0,0, c, 1,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,0),
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
					Otherwise upper position of face is liquid level
				*/
				else
				{
					vertices[2].Pos.Y = (node_liquid_level-0.5)*BS;
					vertices[3].Pos.Y = (node_liquid_level-0.5)*BS;
				}
				/*
					If neighbor is liquid, lower border of face is liquid level
				*/
				if(neighbor_is_same_liquid)
				{
					vertices[0].Pos.Y = (node_liquid_level-0.5)*BS;
					vertices[1].Pos.Y = (node_liquid_level-0.5)*BS;
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
				Generate top
			 */
			if(top_is_same_liquid)
				continue;
			
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,1),
				video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c, 1,0),
				video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c, 0,0),
			};

			v3f offset(p.X*BS, p.Y*BS + (-0.5+node_liquid_level)*BS, p.Z*BS);
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
				l = decode_light(light_source);
				l = l | (l<<8);
			}
			// Use the light of the node on top if possible
			else if(nodedef->get(ntop).param_type == CPT_LIGHT)
				l = getInteriorLight(ntop, 0, nodedef);
			// Otherwise use the light of this node (the liquid)
			else
				l = getInteriorLight(n, 0, nodedef);
			video::SColor c = MapBlock_LightColor(f.alpha, l, f.light_source);
			
			u8 range = rangelim(nodedef->get(c_flowing).liquid_range, 1, 8);

			// Neighbor liquid levels (key = relative position)
			// Includes current node
			std::map<v3s16, f32> neighbor_levels;
			std::map<v3s16, content_t> neighbor_contents;
			std::map<v3s16, u8> neighbor_flags;
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
					else if(n2.getContent() == c_flowing){
						u8 liquid_level = (n2.param2&LIQUID_LEVEL_MASK);
						if (liquid_level <= LIQUID_LEVEL_MAX+1-range)
							liquid_level = 0;
						else
							liquid_level -= (LIQUID_LEVEL_MAX+1-range);
						level = (-0.5 + ((float)liquid_level+ 0.5) / (float)range * node_liquid_level) * BS;
					}

					// Check node above neighbor.
					// NOTE: This doesn't get executed if neighbor
					//       doesn't exist
					p2.Y += 1;
					n2 = data->m_vmanip.getNodeNoEx(blockpos_nodes + p2);
					if(n2.getContent() == c_source ||
							n2.getContent() == c_flowing)
						flags |= neighborflag_top_is_same_liquid;
				}
				
				neighbor_levels[neighbor_dirs[i]] = level;
				neighbor_contents[neighbor_dirs[i]] = content;
				neighbor_flags[neighbor_dirs[i]] = flags;
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
					cornerlevel = -0.5*BS+0.2;
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
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,0),
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
					video::S3DVertex(-BS/2,0,BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,0,BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,0,-BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,0,-BS/2, 0,0,0, c, 0,0),
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
				f32 dz = (corner_levels[side_corners[3][0]] +
						corner_levels[side_corners[3][1]]) -
						(corner_levels[side_corners[2][0]] +
						corner_levels[side_corners[2][1]]);
				// Positive if liquid moves towards +X
				f32 dx = (corner_levels[side_corners[1][0]] +
						corner_levels[side_corners[1][1]]) -
						(corner_levels[side_corners[0][0]] +
						corner_levels[side_corners[0][1]]);
				f32 tcoord_angle = atan2(dz, dx) * core::RADTODEG ;
				v2f tcoord_center(0.5, 0.5);
				v2f tcoord_translate(
						blockpos_nodes.Z + z,
						blockpos_nodes.X + x);
				tcoord_translate.rotateBy(tcoord_angle);
				tcoord_translate.X -= floor(tcoord_translate.X);
				tcoord_translate.Y -= floor(tcoord_translate.Y);

				for(s32 i=0; i<4; i++)
				{
					vertices[i].TCoords.rotateBy(
							tcoord_angle,
							tcoord_center);
					vertices[i].TCoords += tcoord_translate;
				}

				v2f t = vertices[0].TCoords;
				vertices[0].TCoords = vertices[2].TCoords;
				vertices[2].TCoords = t;

				u16 indices[] = {0,1,2,2,3,0};
				// Add to mesh collector
				collector.append(tile_liquid, vertices, 4, indices, 6);
			}
		break;}
		case NDT_GLASSLIKE:
		{
			TileSpec tile = getNodeTile(n, p, v3s16(0,0,0), data);

			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

			for(u32 j=0; j<6; j++)
			{
				// Check this neighbor
				v3s16 n2p = blockpos_nodes + p + g_6dirs[j];
				MapNode n2 = data->m_vmanip.getNodeNoEx(n2p);
				// Don't make face if neighbor is of same type
				if(n2.getContent() == n.getContent())
					continue;

				// The face at Z+
				video::S3DVertex vertices[4] = {
					video::S3DVertex(-BS/2,-BS/2,BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,-BS/2,BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,BS/2,BS/2, 0,0,0, c, 0,0),
					video::S3DVertex(-BS/2,BS/2,BS/2, 0,0,0, c, 1,0),
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
		case NDT_GLASSLIKE_FRAMED_OPTIONAL:
			// This is always pre-converted to something else
			assert(0);
			break;
		case NDT_GLASSLIKE_FRAMED:
		{
			static const v3s16 dirs[6] = {
				v3s16( 0, 1, 0),
				v3s16( 0,-1, 0),
				v3s16( 1, 0, 0),
				v3s16(-1, 0, 0),
				v3s16( 0, 0, 1),
				v3s16( 0, 0,-1)
			};

			u8 i;
			TileSpec tiles[6];
			for (i = 0; i < 6; i++)
				tiles[i] = getNodeTile(n, p, dirs[i], data);
			
			TileSpec glass_tiles[6];
			if (tiles[1].texture && tiles[2].texture && tiles[3].texture) {
				glass_tiles[0] = tiles[2];
				glass_tiles[1] = tiles[3];
				glass_tiles[2] = tiles[1];
				glass_tiles[3] = tiles[1];
				glass_tiles[4] = tiles[1];
				glass_tiles[5] = tiles[1];
			} else {
				for (i = 0; i < 6; i++)
					glass_tiles[i] = tiles[1];	
			}
			
			u8 param2 = n.getParam2();
			bool H_merge = ! bool(param2 & 128);
			bool V_merge = ! bool(param2 & 64);
			param2  = param2 & 63;
			
			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);
			v3f pos = intToFloat(p, BS);
			static const float a = BS / 2;
			static const float g = a - 0.003;
			static const float b = .876 * ( BS / 2 );
			
			static const aabb3f frame_edges[12] = {
				aabb3f( b, b,-a, a, a, a), // y+
				aabb3f(-a, b,-a,-b, a, a), // y+
				aabb3f( b,-a,-a, a,-b, a), // y-
				aabb3f(-a,-a,-a,-b,-b, a), // y-
				aabb3f( b,-a, b, a, a, a), // x+
				aabb3f( b,-a,-a, a, a,-b), // x+
				aabb3f(-a,-a, b,-b, a, a), // x-
				aabb3f(-a,-a,-a,-b, a,-b), // x-
				aabb3f(-a, b, b, a, a, a), // z+
				aabb3f(-a,-a, b, a,-b, a), // z+
				aabb3f(-a,-a,-a, a,-b,-b), // z-
				aabb3f(-a, b,-a, a, a,-b)  // z-
			};
			static const aabb3f glass_faces[6] = {
				aabb3f(-g, g,-g, g, g, g), // y+
				aabb3f(-g,-g,-g, g,-g, g), // y-
				aabb3f( g,-g,-g, g, g, g), // x+
				aabb3f(-g,-g,-g,-g, g, g), // x-
				aabb3f(-g,-g, g, g, g, g), // z+
				aabb3f(-g,-g,-g, g, g,-g)  // z-
			};
			
			// table of node visible faces, 0 = invisible
			int visible_faces[6] = {0,0,0,0,0,0};
			
			// table of neighbours, 1 = same type, checked with g_26dirs
			int nb[18] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
			
			// g_26dirs to check when only horizontal merge is allowed
			int nb_H_dirs[8] = {0,2,3,5,10,11,12,13};
			
			content_t current = n.getContent();
			content_t n2c;
			MapNode n2;
			v3s16 n2p;

			// neighbours checks for frames visibility

			if (!H_merge && V_merge) {
				n2p = blockpos_nodes + p + g_26dirs[1];
				n2 = data->m_vmanip.getNodeNoEx(n2p);
				n2c = n2.getContent();
				if (n2c == current || n2c == CONTENT_IGNORE)
					nb[1] = 1;
				n2p = blockpos_nodes + p + g_26dirs[4];
				n2 = data->m_vmanip.getNodeNoEx(n2p);
				n2c = n2.getContent();
				if (n2c == current || n2c == CONTENT_IGNORE)
					nb[4] = 1;	
			} else if (H_merge && !V_merge) {
				for(i = 0; i < 8; i++) {
					n2p = blockpos_nodes + p + g_26dirs[nb_H_dirs[i]];
					n2 = data->m_vmanip.getNodeNoEx(n2p);
					n2c = n2.getContent();
					if (n2c == current || n2c == CONTENT_IGNORE)
						nb[nb_H_dirs[i]] = 1;		
				}
			} else if (H_merge && V_merge) {
				for(i = 0; i < 18; i++)	{
					n2p = blockpos_nodes + p + g_26dirs[i];
					n2 = data->m_vmanip.getNodeNoEx(n2p);
					n2c = n2.getContent();
					if (n2c == current || n2c == CONTENT_IGNORE)
						nb[i] = 1;
				}
			}

			// faces visibility checks

			if (!V_merge) {
				visible_faces[0] = 1;
				visible_faces[1] = 1;
			} else {
				for(i = 0; i < 2; i++) {
					n2p = blockpos_nodes + p + dirs[i];
					n2 = data->m_vmanip.getNodeNoEx(n2p);
					n2c = n2.getContent();
					if (n2c != current)
						visible_faces[i] = 1;
				}
			}
				
			if (!H_merge) {
				visible_faces[2] = 1;
				visible_faces[3] = 1;
				visible_faces[4] = 1;
				visible_faces[5] = 1;
			} else {
				for(i = 2; i < 6; i++) {
					n2p = blockpos_nodes + p + dirs[i];
					n2 = data->m_vmanip.getNodeNoEx(n2p);
					n2c = n2.getContent();
					if (n2c != current)
						visible_faces[i] = 1;
				}
			}
	
			static const u8 nb_triplet[12*3] = {
				1,2, 7,  1,5, 6,  4,2,15,  4,5,14,
				2,0,11,  2,3,13,  5,0,10,  5,3,12,
				0,1, 8,  0,4,16,  3,4,17,  3,1, 9
			};

			f32 tx1, ty1, tz1, tx2, ty2, tz2;
			aabb3f box;

			for(i = 0; i < 12; i++)
			{
				int edge_invisible;
				if (nb[nb_triplet[i*3+2]])
					edge_invisible = nb[nb_triplet[i*3]] & nb[nb_triplet[i*3+1]];
				else
					edge_invisible = nb[nb_triplet[i*3]] ^ nb[nb_triplet[i*3+1]];
				if (edge_invisible)
					continue;
				box = frame_edges[i];
				box.MinEdge += pos;
				box.MaxEdge += pos;
				tx1 = (box.MinEdge.X / BS) + 0.5;
				ty1 = (box.MinEdge.Y / BS) + 0.5;
				tz1 = (box.MinEdge.Z / BS) + 0.5;
				tx2 = (box.MaxEdge.X / BS) + 0.5;
				ty2 = (box.MaxEdge.Y / BS) + 0.5;
				tz2 = (box.MaxEdge.Z / BS) + 0.5;
				f32 txc1[24] = {
					tx1,   1-tz2,   tx2, 1-tz1,
					tx1,     tz1,   tx2,   tz2,
					tz1,   1-ty2,   tz2, 1-ty1,
					1-tz2, 1-ty2, 1-tz1, 1-ty1,
					1-tx2, 1-ty2, 1-tx1, 1-ty1,
					tx1,   1-ty2,   tx2, 1-ty1,
				};
				makeCuboid(&collector, box, &tiles[0], 1, c, txc1);
			}

			for(i = 0; i < 6; i++)
			{
				if (!visible_faces[i])
					continue;
				box = glass_faces[i];
				box.MinEdge += pos;
				box.MaxEdge += pos;
				tx1 = (box.MinEdge.X / BS) + 0.5;
				ty1 = (box.MinEdge.Y / BS) + 0.5;
				tz1 = (box.MinEdge.Z / BS) + 0.5;
				tx2 = (box.MaxEdge.X / BS) + 0.5;
				ty2 = (box.MaxEdge.Y / BS) + 0.5;
				tz2 = (box.MaxEdge.Z / BS) + 0.5;
				f32 txc2[24] = {
					tx1,   1-tz2,   tx2, 1-tz1,
					tx1,     tz1,   tx2,   tz2,
					tz1,   1-ty2,   tz2, 1-ty1,
					1-tz2, 1-ty2, 1-tz1, 1-ty1,
					1-tx2, 1-ty2, 1-tx1, 1-ty1,
					tx1,   1-ty2,   tx2, 1-ty1,
				};
				makeCuboid(&collector, box, &glass_tiles[i], 1, c, txc2);
			}

			if (param2 > 0 && f.special_tiles[0].texture) {
				// Interior volume level is in range 0 .. 63,
				// convert it to -0.5 .. 0.5
				float vlev = (((float)param2 / 63.0 ) * 2.0 - 1.0);
				TileSpec interior_tiles[6];
				for (i = 0; i < 6; i++)
					interior_tiles[i] = f.special_tiles[0];
				float offset = 0.003;
				box = aabb3f(visible_faces[3] ? -b : -a + offset,
							 visible_faces[1] ? -b : -a + offset,
							 visible_faces[5] ? -b : -a + offset,
							 visible_faces[2] ? b : a - offset,
							 visible_faces[0] ? b * vlev : a * vlev - offset,
							 visible_faces[4] ? b : a - offset);
				box.MinEdge += pos;
				box.MaxEdge += pos;
				tx1 = (box.MinEdge.X / BS) + 0.5;
				ty1 = (box.MinEdge.Y / BS) + 0.5;
				tz1 = (box.MinEdge.Z / BS) + 0.5;
				tx2 = (box.MaxEdge.X / BS) + 0.5;
				ty2 = (box.MaxEdge.Y / BS) + 0.5;
				tz2 = (box.MaxEdge.Z / BS) + 0.5;
				f32 txc3[24] = {
					tx1,   1-tz2,   tx2, 1-tz1,
					tx1,     tz1,   tx2,   tz2,
					tz1,   1-ty2,   tz2, 1-ty1,
					1-tz2, 1-ty2, 1-tz1, 1-ty1,
					1-tx2, 1-ty2, 1-tx1, 1-ty1,
					tx1,   1-ty2,   tx2, 1-ty1,
				};
				makeCuboid(&collector, box, interior_tiles, 6, c,  txc3);
			}
		break;}
		case NDT_ALLFACES:
		{
			TileSpec tile_leaves = getNodeTile(n, p,
					v3s16(0,0,0), data);

			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

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

			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

			float s = BS/2*f.visual_scale;
			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(-s,-s,0, 0,0,0, c, 0,1),
				video::S3DVertex( s,-s,0, 0,0,0, c, 1,1),
				video::S3DVertex( s, s,0, 0,0,0, c, 1,0),
				video::S3DVertex(-s, s,0, 0,0,0, c, 0,0),
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

			u16 l = getInteriorLight(n, 0, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);
				
			float d = (float)BS/16;
			float s = BS/2*f.visual_scale;
			// Wall at X+ of node
			video::S3DVertex vertices[4] =
			{
				video::S3DVertex(BS/2-d,  s,  s, 0,0,0, c, 0,0),
				video::S3DVertex(BS/2-d,  s, -s, 0,0,0, c, 1,0),
				video::S3DVertex(BS/2-d, -s, -s, 0,0,0, c, 1,1),
				video::S3DVertex(BS/2-d, -s,  s, 0,0,0, c, 0,1),
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

			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);
			
			float s = BS / 2;
			for(u32 j = 0; j < 2; j++)
			{
				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-s,-s, 0, 0,0,0, c, 0,1),
					video::S3DVertex( s,-s, 0, 0,0,0, c, 1,1),
					video::S3DVertex( s, s, 0, 0,0,0, c, 1,0),
					video::S3DVertex(-s, s, 0, 0,0,0, c, 0,0),
				};

				if(j == 0)
				{
					for(u16 i = 0; i < 4; i++)
						vertices[i].Pos.rotateXZBy(46 + n.param2 * 2);
				}
				else if(j == 1)
				{
					for(u16 i = 0; i < 4; i++)
						vertices[i].Pos.rotateXZBy(-44 + n.param2 * 2);
				}

				for(u16 i = 0; i < 4; i++)
				{
					vertices[i].Pos *= f.visual_scale;
					vertices[i].Pos.Y -= s * (1 - f.visual_scale);
					vertices[i].Pos += intToFloat(p, BS);
				}

				u16 indices[] = {0, 1, 2, 2, 3, 0};
				// Add to mesh collector
				collector.append(tile, vertices, 4, indices, 6);
			}
		break;}
		case NDT_FIRELIKE:
		{
			TileSpec tile = getNodeTileN(n, p, 0, data);
			tile.material_flags |= MATERIAL_FLAG_CRACK_OVERLAY;

			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

			float s = BS/2*f.visual_scale;

			content_t current = n.getContent();
			content_t n2c;
			MapNode n2;
			v3s16 n2p;

			static const v3s16 dirs[6] = {
				v3s16( 0, 1, 0),
				v3s16( 0,-1, 0),
				v3s16( 1, 0, 0),
				v3s16(-1, 0, 0),
				v3s16( 0, 0, 1),
				v3s16( 0, 0,-1)
			};

			int doDraw[6] = {0,0,0,0,0,0};

			bool drawAllFaces = true;

			bool drawBottomFacesOnly = false; // Currently unused

			// Check for adjacent nodes
			for(int i = 0; i < 6; i++)
			{
				n2p = blockpos_nodes + p + dirs[i];
				n2 = data->m_vmanip.getNodeNoEx(n2p);
				n2c = n2.getContent();
				if (n2c != CONTENT_IGNORE && n2c != CONTENT_AIR && n2c != current) {
					doDraw[i] = 1;
					if(drawAllFaces)
						drawAllFaces = false;

				}
			}

			for(int j = 0; j < 6; j++)
			{
				int vOffset = 0; // Vertical offset of faces after rotation
				int hOffset = 4; // Horizontal offset of faces to reach the edge

				video::S3DVertex vertices[4] =
				{
					video::S3DVertex(-s,-BS/2,      0, 0,0,0, c, 0,1),
					video::S3DVertex( s,-BS/2,      0, 0,0,0, c, 1,1),
					video::S3DVertex( s,-BS/2 + s*2,0, 0,0,0, c, 1,0),
					video::S3DVertex(-s,-BS/2 + s*2,0, 0,0,0, c, 0,0),
				};

				// Calculate which faces should be drawn, (top or sides)
				if(j == 0 && (drawAllFaces || (doDraw[3] == 1 || doDraw[1] == 1)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateXZBy(90 + n.param2 * 2);
						vertices[i].Pos.rotateXYBy(-10);
						vertices[i].Pos.Y -= vOffset;
						vertices[i].Pos.X -= hOffset;
					}
				}
				else if(j == 1 && (drawAllFaces || (doDraw[5] == 1 || doDraw[1] == 1)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateXZBy(180 + n.param2 * 2);
						vertices[i].Pos.rotateYZBy(10);
						vertices[i].Pos.Y -= vOffset;
						vertices[i].Pos.Z -= hOffset;
					}
				}
				else if(j == 2 && (drawAllFaces || (doDraw[2] == 1 || doDraw[1] == 1)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateXZBy(270 + n.param2 * 2);
						vertices[i].Pos.rotateXYBy(10);
						vertices[i].Pos.Y -= vOffset;
						vertices[i].Pos.X += hOffset;
					}
				}
				else if(j == 3 && (drawAllFaces || (doDraw[4] == 1 || doDraw[1] == 1)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateYZBy(-10);
						vertices[i].Pos.Y -= vOffset;
						vertices[i].Pos.Z += hOffset;
					}
				}

				// Center cross-flames
				else if(j == 4 && (drawAllFaces || doDraw[1] == 1))
				{
					for(int i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(45 + n.param2 * 2);
						vertices[i].Pos.Y -= vOffset;
					}
				}
				else if(j == 5 && (drawAllFaces || doDraw[1] == 1))
				{
					for(int i=0; i<4; i++) {
						vertices[i].Pos.rotateXZBy(-45 + n.param2 * 2);
						vertices[i].Pos.Y -= vOffset;
					}
				}

				// Render flames on bottom
				else if(j == 0 && (drawBottomFacesOnly || (doDraw[0] == 1 && doDraw[1] == 0)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateYZBy(70);
						vertices[i].Pos.rotateXZBy(90 + n.param2 * 2);
						vertices[i].Pos.Y += 4.84;
						vertices[i].Pos.X -= hOffset+0.7;
					}
				}
				else if(j == 1 && (drawBottomFacesOnly || (doDraw[0] == 1 && doDraw[1] == 0)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateYZBy(70);
						vertices[i].Pos.rotateXZBy(180 + n.param2 * 2);
						vertices[i].Pos.Y += 4.84;
						vertices[i].Pos.Z -= hOffset+0.7;
					}
				}
				else if(j == 2 && (drawBottomFacesOnly || (doDraw[0] == 1 && doDraw[1] == 0)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateYZBy(70);
						vertices[i].Pos.rotateXZBy(270 + n.param2 * 2);
						vertices[i].Pos.Y += 4.84;
						vertices[i].Pos.X += hOffset+0.7;
					}
				}
				else if(j == 3 && (drawBottomFacesOnly || (doDraw[0] == 1 && doDraw[1] == 0)))
				{
					for(int i = 0; i < 4; i++) {
						vertices[i].Pos.rotateYZBy(70);
						vertices[i].Pos.Y += 4.84;
						vertices[i].Pos.Z += hOffset+0.7;
					}
				}
				else {
					// Skip faces that aren't adjacent to a node
					continue;
				}

				for(int i=0; i<4; i++)
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

			// Put wood the right way around in the posts
			TileSpec tile_rot = tile;
			tile_rot.rotation = 1;

			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

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
			std::string groupname = "connect_to_raillike"; // name of the group that enables connecting to raillike nodes of different kind
			bool self_connect_to_raillike = ((ItemGroupList) nodedef->get(n).groups)[groupname] != 0;

			if ((nodedef->get(n_minus_x).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_minus_x).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_minus_x.getContent() == thiscontent)
				is_rail_x[0] = true;

			if ((nodedef->get(n_minus_x_minus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_minus_x_minus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_minus_x_minus_y.getContent() == thiscontent)
				is_rail_x_minus_y[0] = true;

			if ((nodedef->get(n_minus_x_plus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_minus_x_plus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_minus_x_plus_y.getContent() == thiscontent)
				is_rail_x_plus_y[0] = true;

			if ((nodedef->get(n_plus_x).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_plus_x).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_plus_x.getContent() == thiscontent)
				is_rail_x[1] = true;

			if ((nodedef->get(n_plus_x_minus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_plus_x_minus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_plus_x_minus_y.getContent() == thiscontent)
				is_rail_x_minus_y[1] = true;

			if ((nodedef->get(n_plus_x_plus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_plus_x_plus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_plus_x_plus_y.getContent() == thiscontent)
				is_rail_x_plus_y[1] = true;

			if ((nodedef->get(n_minus_z).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_minus_z).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_minus_z.getContent() == thiscontent)
				is_rail_z[0] = true;

			if ((nodedef->get(n_minus_z_minus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_minus_z_minus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_minus_z_minus_y.getContent() == thiscontent)
				is_rail_z_minus_y[0] = true;

			if ((nodedef->get(n_minus_z_plus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_minus_z_plus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_minus_z_plus_y.getContent() == thiscontent)
				is_rail_z_plus_y[0] = true;

			if ((nodedef->get(n_plus_z).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_plus_z).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_plus_z.getContent() == thiscontent)
				is_rail_z[1] = true;

			if ((nodedef->get(n_plus_z_minus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_plus_z_minus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_plus_z_minus_y.getContent() == thiscontent)
				is_rail_z_minus_y[1] = true;

			if ((nodedef->get(n_plus_z_plus_y).drawtype == NDT_RAILLIKE
					&& ((ItemGroupList) nodedef->get(n_plus_z_plus_y).groups)[groupname] != 0
					&& self_connect_to_raillike)
					|| n_plus_z_plus_y.getContent() == thiscontent)
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
					if (is_rail_z_plus_y[0])
						angle = 180;
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

			u16 l = getInteriorLight(n, 0, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

			float d = (float)BS/64;
			
			char g=-1;
			if (is_rail_x_plus_y[0] || is_rail_x_plus_y[1] || is_rail_z_plus_y[0] || is_rail_z_plus_y[1])
				g=1; //Object is at a slope

			video::S3DVertex vertices[4] =
			{
					video::S3DVertex(-BS/2,-BS/2+d,-BS/2, 0,0,0, c, 0,1),
					video::S3DVertex(BS/2,-BS/2+d,-BS/2, 0,0,0, c, 1,1),
					video::S3DVertex(BS/2,g*BS/2+d,BS/2, 0,0,0, c, 1,0),
					video::S3DVertex(-BS/2,g*BS/2+d,BS/2, 0,0,0, c, 0,0),
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
			
			u16 l = getInteriorLight(n, 1, nodedef);
			video::SColor c = MapBlock_LightColor(255, l, f.light_source);

			v3f pos = intToFloat(p, BS);

			std::vector<aabb3f> boxes = n.getNodeBoxes(nodedef);
			for(std::vector<aabb3f>::iterator
					i = boxes.begin();
					i != boxes.end(); i++)
			{
			for(int j = 0; j < 6; j++)
				{
				// Handles facedir rotation for textures
				tiles[j] = getNodeTile(n, p, tile_dirs[j], data);
				}
				aabb3f box = *i;
				box.MinEdge += pos;
				box.MaxEdge += pos;
				
				f32 temp;
				if (box.MinEdge.X > box.MaxEdge.X)
				{
					temp=box.MinEdge.X;
					box.MinEdge.X=box.MaxEdge.X;
					box.MaxEdge.X=temp;
				}
				if (box.MinEdge.Y > box.MaxEdge.Y)
				{
					temp=box.MinEdge.Y;
					box.MinEdge.Y=box.MaxEdge.Y;
					box.MaxEdge.Y=temp;
				}
				if (box.MinEdge.Z > box.MaxEdge.Z)
				{
					temp=box.MinEdge.Z;
					box.MinEdge.Z=box.MaxEdge.Z;
					box.MaxEdge.Z=temp;
				}

				//
				// Compute texture coords
				f32 tx1 = (box.MinEdge.X/BS)+0.5;
				f32 ty1 = (box.MinEdge.Y/BS)+0.5;
				f32 tz1 = (box.MinEdge.Z/BS)+0.5;
				f32 tx2 = (box.MaxEdge.X/BS)+0.5;
				f32 ty2 = (box.MaxEdge.Y/BS)+0.5;
				f32 tz2 = (box.MaxEdge.Z/BS)+0.5;
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
		case NDT_MESH:
		{
			v3f pos = intToFloat(p, BS);
			video::SColor c = MapBlock_LightColor(255, getInteriorLight(n, 1, nodedef), f.light_source);
			u8 facedir = n.getFaceDir(nodedef);
			if (f.mesh_ptr[facedir]) {
				for(u16 j = 0; j < f.mesh_ptr[facedir]->getMeshBufferCount(); j++) {
					scene::IMeshBuffer *buf = f.mesh_ptr[facedir]->getMeshBuffer(j);
					collector.append(getNodeTileN(n, p, j, data),
						(video::S3DVertex *)buf->getVertices(), buf->getVertexCount(),
						buf->getIndices(), buf->getIndexCount(), pos, c);
				}
			}
		break;}
		}
	}
}

