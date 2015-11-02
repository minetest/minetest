/*
Minetest
Copyright (C) 2015 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "far_map_server.h"
#include "constants.h"
#include "nodedef.h"
#include "profiler.h"
#include "util/numeric.h"
#include "util/string.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

/*
	ServerFarBlock
*/

ServerFarBlock::ServerFarBlock(v3s16 p):
	p(p),
	modification_counter(0)
{
	// Block's effective origin in FarNodes based on block_div
	v3s16 fnp0 = v3s16(
			p.X * FMP_SCALE * SERVER_FB_MB_DIV,
			p.Y * FMP_SCALE * SERVER_FB_MB_DIV,
			p.Z * FMP_SCALE * SERVER_FB_MB_DIV);
	
	v3s16 area_size_fns = v3s16(
			FMP_SCALE * SERVER_FB_MB_DIV,
			FMP_SCALE * SERVER_FB_MB_DIV,
			FMP_SCALE * SERVER_FB_MB_DIV);

	v3s16 fnp1 = fnp0 + area_size_fns - v3s16(1,1,1); // Inclusive

	content_area = VoxelArea(fnp0, fnp1);

	size_t content_size_n = content_area.getVolume();

	content.resize(content_size_n, CONTENT_IGNORE);

	VoxelArea blocks_area(p * FMP_SCALE, (p+1) * FMP_SCALE - v3s16(1,1,1));
	loaded_mapblocks.reset(blocks_area, ServerFarBlock::LS_UNKNOWN);
}

ServerFarBlock::~ServerFarBlock()
{
}

std::string analyze_far_block(ServerFarBlock *b)
{
	if (b == NULL)
		return "NULL";
	std::string s = "["+analyze_far_block(
			b->p, b->content, b->content_area, b->content_area);
	s += ", modification_counter="+itos(b->modification_counter);
	s += ", loaded_mapblocks=";
	s += itos(b->loaded_mapblocks.count(ServerFarBlock::LS_UNKNOWN))+"U+";
	s += itos(b->loaded_mapblocks.count(ServerFarBlock::LS_NOT_LOADED))+"N+";
	s += itos(b->loaded_mapblocks.count(ServerFarBlock::LS_NOT_GENERATED))+"L+";
	s += itos(b->loaded_mapblocks.count(ServerFarBlock::LS_GENERATED))+"G";
	s += "/"+itos(b->loaded_mapblocks.size());
	return s+"]";
}

/*
	ServerFarMapPiece
*/

void ServerFarMapPiece::generateFrom(VoxelManipulator &vm, INodeDefManager *ndef)
{
	TimeTaker tt(NULL, NULL, PRECISION_MICRO);

	v3s16 fnp0 = getContainerPos(vm.m_area.MinEdge, SERVER_FN_SIZE);
	v3s16 fnp1 = getContainerPos(vm.m_area.MaxEdge, SERVER_FN_SIZE);
	content_area = VoxelArea(fnp0, fnp1);

	size_t content_size_n = content_area.getVolume();

	content.resize(content_size_n, CONTENT_IGNORE);

#if 0
	v3s16 fnp;
	for (fnp.Y=content_area.MinEdge.Y; fnp.Y<=content_area.MaxEdge.Y; fnp.Y++)
	for (fnp.X=content_area.MinEdge.X; fnp.X<=content_area.MaxEdge.X; fnp.X++)
	for (fnp.Z=content_area.MinEdge.Z; fnp.Z<=content_area.MaxEdge.Z; fnp.Z++) {
		size_t i = content_area.index(fnp);

		u16 node_id = CONTENT_IGNORE;
		u8 light = 0;

		// Node at center of FarNode (horizontally)
		v3s16 np(
			fnp.X * SERVER_FN_SIZE + SERVER_FN_SIZE/2,
			fnp.Y * SERVER_FN_SIZE + SERVER_FN_SIZE/2,
			fnp.Z * SERVER_FN_SIZE + SERVER_FN_SIZE/2);
		for(s32 i=0; i<SERVER_FN_SIZE+1; i++){
			MapNode n = vm.getNodeNoEx(np);
			if(n.getContent() == CONTENT_IGNORE)
				break;
			const ContentFeatures &f = ndef->get(n);
			if (!f.name.empty() && f.param_type == CPT_LIGHT) {
				light = n.param1;
				if(node_id == CONTENT_IGNORE){
					node_id = n.getContent();
				}
				break;
			} else {
				// TODO: Get light of a nearby node; something that defines
				//       how brightly this division should be rendered
				// (day | (night << 4))
				light = (15) | (0<<4);
				node_id = n.getContent();
			}
			np.Y++;
		}

		content[i].id = node_id;
		content[i].light = light;
	}
#endif

#if 0
	v3s16 fnp;
	for (fnp.Z=content_area.MinEdge.Z; fnp.Z<=content_area.MaxEdge.Z; fnp.Z++)
	for (fnp.Y=content_area.MinEdge.Y; fnp.Y<=content_area.MaxEdge.Y; fnp.Y++)
	for (fnp.X=content_area.MinEdge.X; fnp.X<=content_area.MaxEdge.X; fnp.X++) {
		size_t i = content_area.index(fnp);

		std::map<u16, u16> node_amounts;
		// (day | (night << 4))
		u8 light = (15) | (0<<4);
		bool light_found = false;

		// Node at center of FarNode (horizontally)
		v3s16 np0(
			fnp.X * SERVER_FN_SIZE,
			fnp.Y * SERVER_FN_SIZE,
			fnp.Z * SERVER_FN_SIZE);
		v3s16 np1(
			fnp.X * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Y * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Z * SERVER_FN_SIZE + SERVER_FN_SIZE - 1);
		v3s16 np;

		for (np.Z = np0.Z; np.Z <= np1.Z; np.Z++)
		for (np.Y = np0.Y; np.Y <= np1.Y; np.Y++)
		for (np.X = np0.X; np.X <= np1.X; np.X++) {
			MapNode n = vm.getNodeNoEx(np);
			if(n.getContent() == CONTENT_IGNORE)
				break;

			node_amounts[n.getContent()]++;

			if (!light_found) {
				const ContentFeatures &f = ndef->get(n);
				if (!f.name.empty() && f.param_type == CPT_LIGHT) {
					light = n.param1;
					light_found = true;
				}
			}
		}

		u16 max_id = CONTENT_IGNORE;
		u16 max_amount = 0;
		for (std::map<u16, u16>::const_iterator i = node_amounts.begin();
				i != node_amounts.end(); i++) {
			if (max_id == CONTENT_IGNORE || i->second > max_amount) {
				max_id = i->first;
				max_amount = i->second;
				break;
			}
		}

		content[i].id = max_id;
		content[i].light = light;
	}
#endif

#if 0
	v3s16 fnp;
	for (fnp.Z=content_area.MinEdge.Z; fnp.Z<=content_area.MaxEdge.Z; fnp.Z++)
	for (fnp.Y=content_area.MinEdge.Y; fnp.Y<=content_area.MaxEdge.Y; fnp.Y++)
	for (fnp.X=content_area.MinEdge.X; fnp.X<=content_area.MaxEdge.X; fnp.X++) {
		size_t i = content_area.index(fnp);

		std::map<u16, u16> node_amounts;
		// (day | (night << 4))
		u8 light = (15) | (0<<4);
		bool light_found = false;

		// Node at center of FarNode (horizontally)
		v3s16 np0(
			fnp.X * SERVER_FN_SIZE,
			fnp.Y * SERVER_FN_SIZE,
			fnp.Z * SERVER_FN_SIZE);
		v3s16 np1(
			fnp.X * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Y * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Z * SERVER_FN_SIZE + SERVER_FN_SIZE - 1);
		v3s16 np;

		for (np.Z = np0.Z; np.Z <= np1.Z; np.Z++)
		for (np.X = np0.X; np.X <= np1.X; np.X++) {
			// Find the topmost solid-ish node from this Y column and add it in
			// node_amounts
			for (np.Y = np1.Y; np.Y >= np0.Y; np.Y--) {
				MapNode n = vm.getNodeNoEx(np);
				if(n.getContent() == CONTENT_IGNORE)
					break;
				const ContentFeatures &f = ndef->get(n);
				// Skip if definition doesn't exist
				if (f.name.empty())
					continue;
				// Get a light level if available
				if (!light_found) {
					if (!f.name.empty() && f.param_type == CPT_LIGHT) {
						light = n.param1;
						light_found = true;
					}
				}
				// Skip if drawtype isn't something that a relatively solid node
				// would have
				if (
					f.drawtype != NDT_NORMAL &&
					f.drawtype != NDT_AIRLIKE &&
					f.drawtype != NDT_LIQUID &&
					f.drawtype != NDT_FLOWINGLIQUID &&
					f.drawtype != NDT_GLASSLIKE &&
					f.drawtype != NDT_ALLFACES &&
					f.drawtype != NDT_ALLFACES_OPTIONAL &&
					f.drawtype != NDT_NODEBOX &&
					f.drawtype != NDT_GLASSLIKE_FRAMED &&
					f.drawtype != NDT_GLASSLIKE_FRAMED_OPTIONAL
				) {
					continue;
				}
				// We guess this is good then
				node_amounts[n.getContent()]++;
				// Next Y column
				break;
			}
		}

		u16 max_id = CONTENT_IGNORE;
		u16 max_amount = 0;
		for (std::map<u16, u16>::const_iterator i = node_amounts.begin();
				i != node_amounts.end(); i++) {
			if (max_id == CONTENT_IGNORE || i->second > max_amount) {
				max_id = i->first;
				max_amount = i->second;
				break;
			}
		}

		content[i].id = max_id;
		content[i].light = light;
	}
#endif

#if 1
	/*
		Just... don't touch this. This is insanely delicate. -celeron55
	*/
	std::vector<v3s16> missing_light_fnps;
	v3s16 fnp;
	for (fnp.Z=content_area.MinEdge.Z; fnp.Z<=content_area.MaxEdge.Z; fnp.Z++)
	for (fnp.Y=content_area.MinEdge.Y; fnp.Y<=content_area.MaxEdge.Y; fnp.Y++)
	for (fnp.X=content_area.MinEdge.X; fnp.X<=content_area.MaxEdge.X; fnp.X++) {
		size_t content_i = content_area.index(fnp);

		u32 light_day_sum = 0;
		u32 light_night_sum = 0;
		u16 light_count = 0;

		std::map<u16, u16> node_amounts;

		u16 amount_air = 0;
		u16 amount_non_air = 0;

		// Half of whatever the loops below do
		const u16 half_amount_limit =
				SERVER_FN_SIZE * SERVER_FN_SIZE * SERVER_FN_SIZE / 2 / 2;

		// FarNode corners in MapNodes
		v3s16 np0(
			fnp.X * SERVER_FN_SIZE,
			fnp.Y * SERVER_FN_SIZE,
			fnp.Z * SERVER_FN_SIZE);
		v3s16 np1(
			fnp.X * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Y * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Z * SERVER_FN_SIZE + SERVER_FN_SIZE - 1);
		v3s16 np;

		// Divide only the other axis in half; that gives us enough speed for
		// enough detail
		for (np.Z = np0.Z; np.Z <= np1.Z; np.Z+=2)
		for (np.X = np0.X; np.X <= np1.X; np.X++) {
			// Find the topmost solid-ish node from this Y column and add it in
			// node_amounts
			// Start higher and go only halfway down because that looks better
			for (np.Y = np1.Y; np.Y >= np0.Y; np.Y--) {
				MapNode n = vm.getNodeNoEx(np);
				if(n.getContent() == CONTENT_IGNORE)
					continue;
				const ContentFeatures &f = ndef->get(n);
				// Skip if definition doesn't exist
				if (f.name.empty())
					continue;
				// Get a light level if available
				if (f.param_type == CPT_LIGHT) {
					light_day_sum += (n.param1 & 0x0f);
					light_night_sum += (n.param1 & 0xf0) >> 4;
					light_count++;
				}
				// Continue through air, but still collect them
				if (f.drawtype == NDT_AIRLIKE) {
					amount_air++;
					if (amount_air >= half_amount_limit &&
							amount_air >= amount_non_air * 3) {
						// Fast path
						content[content_i].id = n.getContent();
						goto already_chosen;
					}
					continue;
				}
				// Skip if drawtype isn't something that a relatively solid node
				// would have
				if (
					f.drawtype != NDT_NORMAL &&
					f.drawtype != NDT_LIQUID &&
					f.drawtype != NDT_FLOWINGLIQUID &&
					f.drawtype != NDT_GLASSLIKE &&
					f.drawtype != NDT_ALLFACES &&
					f.drawtype != NDT_ALLFACES_OPTIONAL &&
					f.drawtype != NDT_NODEBOX &&
					f.drawtype != NDT_GLASSLIKE_FRAMED &&
					f.drawtype != NDT_GLASSLIKE_FRAMED_OPTIONAL &&
					f.drawtype != NDT_MESH
				) {
					continue;
				}
				// We guess this is good then
				amount_non_air++;
				u16 amount_now = ++node_amounts[n.getContent()];
				// Check fast path
				if (amount_now >= half_amount_limit) {
					content[content_i].id = n.getContent();
					goto already_chosen;
				}
				// Next Y column
				break;
			}
		}

		// We don't really want to show air because it generally reveals dirt or
		// stone from underneath but if there's a huge amount of it, then just
		// show air.
		if (amount_air >= amount_non_air * 3) {
			content[content_i].id = CONTENT_AIR;
		} else {
			u16 max_id = CONTENT_IGNORE;
			u16 max_amount = 0;
			for (std::map<u16, u16>::const_iterator i = node_amounts.begin();
					i != node_amounts.end(); i++) {
				if (max_id == CONTENT_IGNORE || i->second > max_amount) {
					max_id = i->first;
					max_amount = i->second;
				}
			}
			content[content_i].id = max_id;
		}
already_chosen:

		if (light_count > 0) {
			u8 light_day = light_day_sum / light_count;
			u8 light_night = light_night_sum / light_count;
			// (day | (night << 4))
			content[content_i].light = (light_day) | (light_night<<4);
			//content[content_i].light = (2) | (0<<4); // TODO: Remove
		} else {
			// Guess something that isn't too striking in neither when showing
			// up inside a brightly lit surface or a completely black surface.
			// Well, it's going to look kind of bad, but let's hope for the
			// best.
			static const u8 light_day = 8;
			static const u8 light_night = 0;
			// (day | (night << 4))
			content[content_i].light = (light_day) | (light_night<<4);
			//content[content_i].light = (11) | (0<<4); // TODO: Remove

			// Try to figure a proper light value for this by special methods
			missing_light_fnps.push_back(fnp);
		}
	}
	// This loop is entered mainly for FarNodes that are buried completely
	// underground, but not entirely due to inaccuracy of heuristic-based
	// optimizations.
	// The lighting this generates is used in special occasions like at FarBlock
	// edges, as the nearby FarBlock may not be available for lighting
	// information and the FarNode internal value of solid ground is used
	// instead.
	for (size_t fnp_i=0; fnp_i<missing_light_fnps.size(); fnp_i++) {
		const v3s16 &fnp = missing_light_fnps[fnp_i];
		size_t content_i = content_area.index(fnp);

		// FarNode corners in MapNodes
		v3s16 np0(
			fnp.X * SERVER_FN_SIZE,
			fnp.Y * SERVER_FN_SIZE,
			fnp.Z * SERVER_FN_SIZE);
		v3s16 np1(
			fnp.X * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Y * SERVER_FN_SIZE + SERVER_FN_SIZE - 1,
			fnp.Z * SERVER_FN_SIZE + SERVER_FN_SIZE - 1);

		u32 light_day_sum = 0;
		u32 light_night_sum = 0;
		u16 light_count = 0;

		static const v3s16 dps_to_try[] = {
			// Middle of FarNode
			v3s16(SERVER_FN_SIZE/2, SERVER_FN_SIZE/2, SERVER_FN_SIZE/2),
			// Every inside corner of the FarNode
			v3s16(0               , 0               , 0),
			v3s16(SERVER_FN_SIZE-1, 0               , 0),
			v3s16(0               , SERVER_FN_SIZE-1, 0),
			v3s16(SERVER_FN_SIZE-1, SERVER_FN_SIZE-1, 0),
			v3s16(0               , 0               , SERVER_FN_SIZE-1),
			v3s16(SERVER_FN_SIZE-1, 0               , SERVER_FN_SIZE-1),
			v3s16(0               , SERVER_FN_SIZE-1, SERVER_FN_SIZE-1),
			v3s16(SERVER_FN_SIZE-1, SERVER_FN_SIZE-1, SERVER_FN_SIZE-1),
		};
		for (size_t i=0; i<ARRLEN(dps_to_try); i++) {
			MapNode n = vm.getNodeNoEx(np0 + dps_to_try[i]);
			if(n.getContent() == CONTENT_IGNORE)
				continue;
			const ContentFeatures &f = ndef->get(n);
			if (f.param_type == CPT_LIGHT) {
				light_day_sum += (n.param1 & 0x0f);
				light_night_sum += (n.param1 & 0xf0) >> 4;
				light_count++;
			}
		}
		if (light_count > 0) {
			u8 light_day = light_day_sum / light_count;
			u8 light_night = light_night_sum / light_count;
			// (day | (night << 4))
			content[content_i].light = (light_day) | (light_night<<4);
			//content[content_i].light = (15) | (0<<4); // TODO: Remove
			// Figure out next missing light
			continue;
		}
	}
#endif

	u32 t_us = tt.stop(true);
	g_profiler->avg("Far: gen from MapBlock (avg us)", t_us);
	g_profiler->add("Far: gen from MapBlock (total s)",
			(float)t_us / 1000000.0f);
}

void ServerFarMapPiece::generateEmpty(const VoxelArea &area_nodes)
{
	v3s16 fnp0 = getContainerPos(area_nodes.MinEdge, SERVER_FN_SIZE);
	v3s16 fnp1 = getContainerPos(area_nodes.MaxEdge, SERVER_FN_SIZE);
	content_area = VoxelArea(fnp0, fnp1);

	size_t content_size_n = content_area.getVolume();

	content.resize(content_size_n, CONTENT_IGNORE);

	v3s16 fnp;
	for (fnp.Y=content_area.MinEdge.Y; fnp.Y<=content_area.MaxEdge.Y; fnp.Y++)
	for (fnp.X=content_area.MinEdge.X; fnp.X<=content_area.MaxEdge.X; fnp.X++)
	for (fnp.Z=content_area.MinEdge.Z; fnp.Z<=content_area.MaxEdge.Z; fnp.Z++) {
		size_t i = content_area.index(fnp);
		content[i].id = CONTENT_IGNORE;
		content[i].light = 0; // (day | (night << 4))
	}
}

/*
	ServerFarMap
*/

ServerFarMap::ServerFarMap()
{
}

ServerFarMap::~ServerFarMap()
{
	for (std::map<v3s16, ServerFarBlock*>::iterator i = m_blocks.begin();
			i != m_blocks.end(); i++) {
		delete i->second;
	}
}

ServerFarBlock* ServerFarMap::getBlock(v3s16 p)
{
	std::map<v3s16, ServerFarBlock*>::iterator i = m_blocks.find(p);
	if(i != m_blocks.end())
		return i->second;
	return NULL;
}

ServerFarBlock* ServerFarMap::getOrCreateBlock(v3s16 p)
{
	std::map<v3s16, ServerFarBlock*>::iterator i = m_blocks.find(p);
	if(i != m_blocks.end())
		return i->second;
	ServerFarBlock *b = new ServerFarBlock(p);
	m_blocks[p] = b;
	return b;
}

void ServerFarMap::updateFrom(const ServerFarMapPiece &piece,
		ServerFarBlock::LoadState load_state)
{
	v3s16 fnp000 = piece.content_area.MinEdge;
	v3s16 fnp001 = piece.content_area.MaxEdge;

	// TODO: Assert that this is the area of a single MapBlock because otherwise
	//       load states are not handled properly

	// Convert to ServerFarBlock positions (this can cover extra area)
	VoxelArea fb_area(
		getContainerPos(fnp000, FMP_SCALE * SERVER_FB_MB_DIV),
		getContainerPos(fnp001, FMP_SCALE * SERVER_FB_MB_DIV)
	);

	v3s16 fbp;
	for (fbp.Y=fb_area.MinEdge.Y; fbp.Y<=fb_area.MaxEdge.Y; fbp.Y++)
	for (fbp.X=fb_area.MinEdge.X; fbp.X<=fb_area.MaxEdge.X; fbp.X++)
	for (fbp.Z=fb_area.MinEdge.Z; fbp.Z<=fb_area.MaxEdge.Z; fbp.Z++) {
		ServerFarBlock *b = getOrCreateBlock(fbp);

		v3s16 fp00 = piece.content_area.MinEdge;
		v3s16 fp01 = piece.content_area.MaxEdge;
		v3s16 fp1;
		for (fp1.Y=fp00.Y; fp1.Y<=fp01.Y; fp1.Y++)
		for (fp1.X=fp00.X; fp1.X<=fp01.X; fp1.X++)
		for (fp1.Z=fp00.Z; fp1.Z<=fp01.Z; fp1.Z++) {
			// The source area does not necessarily contain all positions that
			// the matching blocks contain
			if(!piece.content_area.contains(fp1))
				continue;
			size_t source_i = piece.content_area.index(fp1);
			size_t dst_i = b->content_area.index(fp1);
			b->content[dst_i] = piece.content[source_i];

			v3s16 mbp = getContainerPos(fp1, SERVER_FB_MB_DIV);
			b->loaded_mapblocks.set(mbp, load_state);
		}

		b->modification_counter++;

		/*// Call analyze_far_block before starting the line to keep lines
		// mostly intact when multiple threads are printing
		std::string s = analyze_far_block(b);
		dstream<<"ServerFarMap: Updated block: "<<s<<std::endl;*/
	}
}

