/*
Minetest
Copyright (C) 2016 MillersMan <millersman@users.noreply.github.com>

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

#include "liquidlogicclassic.h"
#include "map.h"
#include "mapblock.h"
#include "nodedef.h"
#include "porting.h"
#include "util/directiontables.h"
#include "serverenvironment.h"
#include "script/scripting_server.h"
#include "rollback_interface.h"
#include "gamedef.h"
#include "voxelalgorithms.h"
#include "emerge.h"


#define WATER_DROP_BOOST 4

enum NeighborType : u8 {
	NEIGHBOR_UPPER,
	NEIGHBOR_SAME_LEVEL,
	NEIGHBOR_LOWER
};

struct NodeNeighbor {
	MapNode n;
	NeighborType t;
	v3s16 p;

	NodeNeighbor()
		: n(CONTENT_AIR), t(NEIGHBOR_SAME_LEVEL)
	{ }

	NodeNeighbor(const MapNode &node, NeighborType n_type, const v3s16 &pos)
		: n(node),
		  t(n_type),
		  p(pos)
	{ }
};

LiquidLogicClassic::LiquidLogicClassic(Map *map, IGameDef *gamedef) :
	LiquidLogic(map, gamedef)
{
}

void LiquidLogicClassic::addTransformingFromData(BlockMakeData *data)
{
	while (data->transforming_liquid.size()) {
		m_liquid_queue.push_back(data->transforming_liquid.front());
		data->transforming_liquid.pop_front();
	}
}

void LiquidLogicClassic::addTransforming(v3s16 p)
{
	m_liquid_queue.push_back(p);
}


void LiquidLogicClassic::scanBlock(MapBlock *block)
{
	m_block_pos = block->getPos();
	m_rel_block_pos = block->getPosRelative();

	// Prepare the lookup which is a 3x3x3 array of the blocks surrounding the
	// scanned block. Blocks are only added to the lookup if they are really
	// needed. The lookup is indexed manually to use the same index in a
	// bit-array (of uint32 type) which stores for unloaded blocks that they
	// were already fetched from Map but were actually nullptr.
	memset(m_lookup, 0, sizeof(m_lookup));
	int block_idx = 1 + (1 * 9) + (1 * 3);
	m_lookup[block_idx] = block;
	m_lookup_state_bitset = 1 << block_idx;

	// Scan the columns in the block
	for (s16 z = 0; z < MAP_BLOCKSIZE; z++)
	for (s16 x = 0; x < MAP_BLOCKSIZE; x++) {
		scanColumn(x, z);
	}

	// Scan neighbouring columns from the nearby blocks as they might contain
	// liquid nodes that weren't allowed to flow to prevent gaps.
	for (s16 i = 0; i < MAP_BLOCKSIZE; i++) {
		scanColumn(i, -1);
		scanColumn(i, MAP_BLOCKSIZE);
		scanColumn(-1, i);
		scanColumn(MAP_BLOCKSIZE, i);
	}
}

inline MapBlock *LiquidLogicClassic::lookupBlock(int x, int y, int z)
{
	// Gets the block that contains (x,y,z) relativ to the scanned block.
	// This uses a lookup as there might be many lookups into the same
	// neighbouring block which makes fetches from Map costly.
	int bx = (MAP_BLOCKSIZE + x) / MAP_BLOCKSIZE;
	int by = (MAP_BLOCKSIZE + y) / MAP_BLOCKSIZE;
	int bz = (MAP_BLOCKSIZE + z) / MAP_BLOCKSIZE;
	int idx = (bx + (by * 9) + (bz * 3));
	MapBlock *result = m_lookup[idx];
	if (!result && (m_lookup_state_bitset & (1 << idx)) == 0) {
		// The block wasn't requested yet so fetch it from Map and store it
		// in the lookup
		v3s16 pos = m_block_pos + v3s16(bx - 1, by - 1, bz - 1);
		m_lookup[idx] = result = m_map->getBlockNoCreateNoEx(pos);
		m_lookup_state_bitset |= (1 << idx);
	}
	return result;
}

inline bool LiquidLogicClassic::isLiquidFlowableTo(int x, int y, int z)
{
	// Tests whether (x,y,z) is a node to which liquid might flow.
	bool valid_position;
	MapBlock *block = lookupBlock(x, y, z);
	if (block) {
		int dx = (MAP_BLOCKSIZE + x) % MAP_BLOCKSIZE;
		int dy = (MAP_BLOCKSIZE + y) % MAP_BLOCKSIZE;
		int dz = (MAP_BLOCKSIZE + z) % MAP_BLOCKSIZE;
		MapNode node = block->getNodeNoCheck(dx, dy, dz, &valid_position);
		if (node.getContent() != CONTENT_IGNORE) {
			const ContentFeatures &f = m_ndef->get(node);
			// NOTE: No need to check for flowing nodes with lower liquid level
			// as they should only occur on top of other columns where they
			// will be added to the queue themselves.
			return f.floodable;
		}
	}
	return false;
}

inline bool LiquidLogicClassic::isLiquidHorizontallyFlowable(int x, int y, int z)
{
	// Check if the (x,y,z) might spread to one of the horizontally
	// neighbouring nodes
	return isLiquidFlowableTo(x - 1, y, z) ||
		isLiquidFlowableTo(x + 1, y, z) ||
		isLiquidFlowableTo(x, y, z - 1) ||
		isLiquidFlowableTo(x, y, z + 1);
}

void LiquidLogicClassic::scanColumn(int x, int z)
{
	bool valid_position;

	// Is the column inside a loaded block?
	MapBlock *block = lookupBlock(x, 0, z);
	if (!block)
		return;

	MapBlock *above = lookupBlock(x, MAP_BLOCKSIZE, z);
	int dx = (MAP_BLOCKSIZE + x) % MAP_BLOCKSIZE;
	int dz = (MAP_BLOCKSIZE + z) % MAP_BLOCKSIZE;

	// Get the state from the node above the scanned block
	bool was_ignore, was_liquid;
	if (above) {
		MapNode node = above->getNodeNoCheck(dx, 0, dz, &valid_position);
		was_ignore = node.getContent() == CONTENT_IGNORE;
		was_liquid = m_ndef->get(node).isLiquid();
	} else {
		was_ignore = true;
		was_liquid = false;
	}
	bool was_checked = false;
	bool was_pushed = false;

	// Scan through the whole block
	for (s16 y = MAP_BLOCKSIZE - 1; y >= 0; y--) {
		MapNode node = block->getNodeNoCheck(dx, y, dz, &valid_position);
		const ContentFeatures &f = m_ndef->get(node);
		bool is_ignore = node.getContent() == CONTENT_IGNORE;
		bool is_liquid = f.isLiquid();

		if (is_ignore || was_ignore || is_liquid == was_liquid) {
			// Neither topmost node of liquid column nor topmost node below column
			was_checked = false;
			was_pushed = false;
		} else if (is_liquid) {
			// This is the topmost node in the column
			bool is_pushed = false;
			if (f.liquid_type == LIQUID_FLOWING ||
					isLiquidHorizontallyFlowable(x, y, z)) {
				m_liquid_queue.push_back(m_rel_block_pos + v3s16(x, y, z));
				is_pushed = true;
			}
			// Remember waschecked and waspushed to avoid repeated
			// checks/pushes in case the column consists of only this node
			was_checked = true;
			was_pushed = is_pushed;
		} else {
			// This is the topmost node below a liquid column
			if (!was_pushed && (f.floodable ||
					(!was_checked && isLiquidHorizontallyFlowable(x, y + 1, z)))) {
				// Activate the lowest node in the column which is one
				// node above this one
				m_liquid_queue.push_back(m_rel_block_pos + v3s16(x, y + 1, z));
			}
		}

		was_liquid = is_liquid;
		was_ignore = is_ignore;
	}

	// Check the node below the current block
	MapBlock *below = lookupBlock(x, -1, z);
	if (below) {
		MapNode node = below->getNodeNoCheck(dx, MAP_BLOCKSIZE - 1, dz, &valid_position);
		const ContentFeatures &f = m_ndef->get(node);
		bool is_ignore = node.getContent() == CONTENT_IGNORE;
		bool is_liquid = f.isLiquid();

		if (is_ignore || was_ignore || is_liquid == was_liquid) {
			// Neither topmost node of liquid column nor topmost node below column
		} else if (is_liquid) {
			// This is the topmost node in the column and might want to flow away
			if (f.liquid_type == LIQUID_FLOWING ||
					isLiquidHorizontallyFlowable(x, -1, z)) {
				m_liquid_queue.push_back(m_rel_block_pos + v3s16(x, -1, z));
			}
		} else {
			// This is the topmost node below a liquid column
			if (!was_pushed && (f.floodable ||
					(!was_checked && isLiquidHorizontallyFlowable(x, 0, z)))) {
				// Activate the lowest node in the column which is one
				// node above this one
				m_liquid_queue.push_back(m_rel_block_pos + v3s16(x, 0, z));
			}
		}
	}
}

inline bool LiquidLogicClassic::isLiquidHorizontallyFlowable(
	MMVManip *vm, u32 vi, v3s16 em)
{
	u32 vi_neg_x = vi;
	VoxelArea::add_x(em, vi_neg_x, -1);
	if (vm->m_data[vi_neg_x].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_nx = m_ndef->get(vm->m_data[vi_neg_x]);
		if (c_nx.floodable && !c_nx.isLiquid())
			return true;
	}
	u32 vi_pos_x = vi;
	VoxelArea::add_x(em, vi_pos_x, +1);
	if (vm->m_data[vi_pos_x].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_px = m_ndef->get(vm->m_data[vi_pos_x]);
		if (c_px.floodable && !c_px.isLiquid())
			return true;
	}
	u32 vi_neg_z = vi;
	VoxelArea::add_z(em, vi_neg_z, -1);
	if (vm->m_data[vi_neg_z].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_nz = m_ndef->get(vm->m_data[vi_neg_z]);
		if (c_nz.floodable && !c_nz.isLiquid())
			return true;
	}
	u32 vi_pos_z = vi;
	VoxelArea::add_z(em, vi_pos_z, +1);
	if (vm->m_data[vi_pos_z].getContent() != CONTENT_IGNORE) {
		const ContentFeatures &c_pz = m_ndef->get(vm->m_data[vi_pos_z]);
		if (c_pz.floodable && !c_pz.isLiquid())
			return true;
	}
	return false;
}

// This code is very similar to scanColumn
// TODO: Factorize
void LiquidLogicClassic::scanVoxelManip(UniqueQueue<v3s16> *liquid_queue,
	MMVManip *vm, v3s16 nmin, v3s16 nmax)
{
	bool isignored, isliquid, wasignored, wasliquid, waschecked, waspushed;
	const v3s16 &em  = vm->m_area.getExtent();

	for (s16 z = nmin.Z + 1; z <= nmax.Z - 1; z++)
	for (s16 x = nmin.X + 1; x <= nmax.X - 1; x++) {
		wasignored = true;
		wasliquid = false;
		waschecked = false;
		waspushed = false;

		u32 vi = vm->m_area.index(x, nmax.Y, z);
		for (s16 y = nmax.Y; y >= nmin.Y; y--) {
			isignored = vm->m_data[vi].getContent() == CONTENT_IGNORE;
			isliquid = m_ndef->get(vm->m_data[vi]).isLiquid();

			if (isignored || wasignored || isliquid == wasliquid) {
				// Neither topmost node of liquid column nor topmost node below column
				waschecked = false;
				waspushed = false;
			} else if (isliquid) {
				// This is the topmost node in the column
				bool ispushed = false;
				if (isLiquidHorizontallyFlowable(vm, vi, em)) {
					liquid_queue->push_back(v3s16(x, y, z));
					ispushed = true;
				}
				// Remember waschecked and waspushed to avoid repeated
				// checks/pushes in case the column consists of only this node
				waschecked = true;
				waspushed = ispushed;
			} else {
				// This is the topmost node below a liquid column
				u32 vi_above = vi;
				VoxelArea::add_y(em, vi_above, 1);
				if (!waspushed && (m_ndef->get(vm->m_data[vi]).floodable ||
						(!waschecked && isLiquidHorizontallyFlowable(vm, vi_above, em)))) {
					// Push back the lowest node in the column which is one
					// node above this one
					liquid_queue->push_back(v3s16(x, y + 1, z));
				}
			}

			wasliquid = isliquid;
			wasignored = isignored;
			VoxelArea::add_y(em, vi, -1);
		}
	}
}

void LiquidLogicClassic::scanVoxelManip(MMVManip *vm, v3s16 nmin, v3s16 nmax)
{
	scanVoxelManip(&m_liquid_queue, vm, nmin, nmax);
}

void LiquidLogicClassic::transform(
	std::map<v3s16, MapBlock*> &modified_blocks,
	ServerEnvironment *env)
{
	u32 loopcount = 0;
	u32 initial_size = m_liquid_queue.size();

	std::deque<v3s16> must_reflow;
	std::vector<std::pair<v3s16, MapNode> > changed_nodes;

	u32 liquid_loop_max = g_settings->getS32("liquid_loop_max");
	u32 loop_max = liquid_loop_max;

	while (m_liquid_queue.size() != 0)
	{
		// This should be done here so that it is done when continue is used
		if (loopcount >= initial_size || loopcount >= loop_max)
			break;
		loopcount++;

		/*
			Get a queued transforming liquid node
		*/
		v3s16 p0 = m_liquid_queue.front();
		m_liquid_queue.pop_front();

		// Get source node information
		MapNode n0 = m_map->getNode(p0);

		/*
			Collect information about current node
		 */
		s8 liquid_level = -1;
		// The liquid node which will be placed there if
		// the liquid flows into this node.
		content_t liquid_kind = CONTENT_IGNORE;
		// The node which will be placed there if liquid
		// can't flow into this node.
		content_t floodable_node = CONTENT_AIR;
		const ContentFeatures &cf = m_ndef->get(n0);
		LiquidType liquid_type = cf.liquid_type;
		switch (liquid_type) {
			case LIQUID_SOURCE:
				liquid_level = LIQUID_LEVEL_SOURCE;
				liquid_kind = m_ndef->getId(cf.liquid_alternative_flowing);
				break;
			case LIQUID_FLOWING:
				liquid_level = (n0.param2 & LIQUID_LEVEL_MASK);
				liquid_kind = n0.getContent();
				break;
			case LIQUID_NONE:
				// if this node is 'floodable', it *could* be transformed
				// into a liquid, otherwise, continue with the next node.
				if (!cf.floodable)
					continue;
				floodable_node = n0.getContent();
				liquid_kind = CONTENT_AIR;
				break;
		}

		/*
			Collect information about the environment
		 */
		const v3s16 *dirs = g_6dirs;
		NodeNeighbor sources[6]; // surrounding sources
		int num_sources = 0;
		NodeNeighbor flows[6]; // surrounding flowing liquid nodes
		int num_flows = 0;
		NodeNeighbor airs[6]; // surrounding air
		int num_airs = 0;
		NodeNeighbor neutrals[6]; // nodes that are solid or another kind of liquid
		int num_neutrals = 0;
		bool flowing_down = false;
		bool ignored_sources = false;
		for (u16 i = 0; i < 6; i++) {
			NeighborType nt = NEIGHBOR_SAME_LEVEL;
			switch (i) {
				case 1:
					nt = NEIGHBOR_UPPER;
					break;
				case 4:
					nt = NEIGHBOR_LOWER;
					break;
				default:
					break;
			}
			v3s16 npos = p0 + dirs[i];
			NodeNeighbor nb(m_map->getNode(npos), nt, npos);
			const ContentFeatures &cfnb = m_ndef->get(nb.n);
			switch (m_ndef->get(nb.n.getContent()).liquid_type) {
				case LIQUID_NONE:
					if (cfnb.floodable) {
						airs[num_airs++] = nb;
						// if the current node is a water source the neighbor
						// should be enqueded for transformation regardless of whether the
						// current node changes or not.
						if (nb.t != NEIGHBOR_UPPER && liquid_type != LIQUID_NONE)
							m_liquid_queue.push_back(npos);
						// if the current node happens to be a flowing node, it will start to flow down here.
						if (nb.t == NEIGHBOR_LOWER)
							flowing_down = true;
					} else {
						neutrals[num_neutrals++] = nb;
						if (nb.n.getContent() == CONTENT_IGNORE) {
							// If node below is ignore prevent water from
							// spreading outwards and otherwise prevent from
							// flowing away as ignore node might be the source
							if (nb.t == NEIGHBOR_LOWER)
								flowing_down = true;
							else
								ignored_sources = true;
						}
					}
					break;
				case LIQUID_SOURCE:
					// if this node is not (yet) of a liquid type, choose the first liquid type we encounter
					if (liquid_kind == CONTENT_AIR)
						liquid_kind = m_ndef->getId(cfnb.liquid_alternative_flowing);
					if (m_ndef->getId(cfnb.liquid_alternative_flowing) != liquid_kind) {
						neutrals[num_neutrals++] = nb;
					} else {
						// Do not count bottom source, it will screw things up
						if(dirs[i].Y != -1)
							sources[num_sources++] = nb;
					}
					break;
				case LIQUID_FLOWING:
					// if this node is not (yet) of a liquid type, choose the first liquid type we encounter
					if (liquid_kind == CONTENT_AIR)
						liquid_kind = m_ndef->getId(cfnb.liquid_alternative_flowing);
					if (m_ndef->getId(cfnb.liquid_alternative_flowing) != liquid_kind) {
						neutrals[num_neutrals++] = nb;
					} else {
						flows[num_flows++] = nb;
						if (nb.t == NEIGHBOR_LOWER)
							flowing_down = true;
					}
					break;
			}
		}

		/*
			decide on the type (and possibly level) of the current node
		 */
		content_t new_node_content;
		s8 new_node_level = -1;
		s8 max_node_level = -1;

		u8 range = m_ndef->get(liquid_kind).liquid_range;
		if (range > LIQUID_LEVEL_MAX + 1)
			range = LIQUID_LEVEL_MAX + 1;

		if ((num_sources >= 2 && m_ndef->get(liquid_kind).liquid_renewable) || liquid_type == LIQUID_SOURCE) {
			// liquid_kind will be set to either the flowing alternative of the node (if it's a liquid)
			// or the flowing alternative of the first of the surrounding sources (if it's air), so
			// it's perfectly safe to use liquid_kind here to determine the new node content.
			new_node_content = m_ndef->getId(m_ndef->get(liquid_kind).liquid_alternative_source);
		} else if (num_sources >= 1 && sources[0].t != NEIGHBOR_LOWER) {
			// liquid_kind is set properly, see above
			max_node_level = new_node_level = LIQUID_LEVEL_MAX;
			if (new_node_level >= (LIQUID_LEVEL_MAX + 1 - range))
				new_node_content = liquid_kind;
			else
				new_node_content = floodable_node;
		} else if (ignored_sources && liquid_level >= 0) {
			// Maybe there are neighbouring sources that aren't loaded yet
			// so prevent flowing away.
			new_node_level = liquid_level;
			new_node_content = liquid_kind;
		} else {
			// no surrounding sources, so get the maximum level that can flow into this node
			for (u16 i = 0; i < num_flows; i++) {
				u8 nb_liquid_level = (flows[i].n.param2 & LIQUID_LEVEL_MASK);
				switch (flows[i].t) {
					case NEIGHBOR_UPPER:
						if (nb_liquid_level + WATER_DROP_BOOST > max_node_level) {
							max_node_level = LIQUID_LEVEL_MAX;
							if (nb_liquid_level + WATER_DROP_BOOST < LIQUID_LEVEL_MAX)
								max_node_level = nb_liquid_level + WATER_DROP_BOOST;
						} else if (nb_liquid_level > max_node_level) {
							max_node_level = nb_liquid_level;
						}
						break;
					case NEIGHBOR_LOWER:
						break;
					case NEIGHBOR_SAME_LEVEL:
						if ((flows[i].n.param2 & LIQUID_FLOW_DOWN_MASK) != LIQUID_FLOW_DOWN_MASK &&
								nb_liquid_level > 0 && nb_liquid_level - 1 > max_node_level)
							max_node_level = nb_liquid_level - 1;
						break;
				}
			}

			u8 viscosity = m_ndef->get(liquid_kind).liquid_viscosity;
			if (viscosity > 1 && max_node_level != liquid_level) {
				// amount to gain, limited by viscosity
				// must be at least 1 in absolute value
				s8 level_inc = max_node_level - liquid_level;
				if (level_inc < -viscosity || level_inc > viscosity)
					new_node_level = liquid_level + level_inc/viscosity;
				else if (level_inc < 0)
					new_node_level = liquid_level - 1;
				else if (level_inc > 0)
					new_node_level = liquid_level + 1;
				if (new_node_level != max_node_level)
					must_reflow.push_back(p0);
			} else {
				new_node_level = max_node_level;
			}

			if (max_node_level >= (LIQUID_LEVEL_MAX + 1 - range))
				new_node_content = liquid_kind;
			else
				new_node_content = floodable_node;

		}

		/*
			check if anything has changed. if not, just continue with the next node.
		 */
		if (new_node_content == n0.getContent() &&
				(m_ndef->get(n0.getContent()).liquid_type != LIQUID_FLOWING ||
				((n0.param2 & LIQUID_LEVEL_MASK) == (u8)new_node_level &&
				((n0.param2 & LIQUID_FLOW_DOWN_MASK) == LIQUID_FLOW_DOWN_MASK)
				== flowing_down)))
			continue;

		/*
			update the current node
		 */
		 MapNode n00 = n0;
		//bool flow_down_enabled = (flowing_down && ((n0.param2 & LIQUID_FLOW_DOWN_MASK) != LIQUID_FLOW_DOWN_MASK));
		if (m_ndef->get(new_node_content).liquid_type == LIQUID_FLOWING) {
			// set level to last 3 bits, flowing down bit to 4th bit
			n0.param2 = (flowing_down ? LIQUID_FLOW_DOWN_MASK : 0x00) | (new_node_level & LIQUID_LEVEL_MASK);
		} else {
			// set the liquid level and flow bit to 0
			n0.param2 = ~(LIQUID_LEVEL_MASK | LIQUID_FLOW_DOWN_MASK);
		}

		// change the node.
		n0.setContent(new_node_content);

		// on_flood() the node
		if (floodable_node != CONTENT_AIR) {
			if (env->getScriptIface()->node_on_flood(p0, n00, n0))
				continue;
		}

		// Ignore light (because calling voxalgo::update_lighting_nodes)
		n0.setLight(LIGHTBANK_DAY, 0, m_ndef);
		n0.setLight(LIGHTBANK_NIGHT, 0, m_ndef);

		// Find out whether there is a suspect for this action
		std::string suspect;
		if (m_gamedef->rollback())
			suspect = m_gamedef->rollback()->getSuspect(p0, 83, 1);

		if (m_gamedef->rollback() && !suspect.empty()) {
			// Blame suspect
			RollbackScopeActor rollback_scope(m_gamedef->rollback(), suspect, true);
			// Get old node for rollback
			RollbackNode rollback_oldnode(m_map, p0, m_gamedef);
			// Set node
			m_map->setNode(p0, n0);
			// Report
			RollbackNode rollback_newnode(m_map, p0, m_gamedef);
			RollbackAction action;
			action.setSetNode(p0, rollback_oldnode, rollback_newnode);
			m_gamedef->rollback()->reportAction(action);
		} else {
			// Set node
			m_map->setNode(p0, n0);
		}

		v3s16 blockpos = getNodeBlockPos(p0);
		MapBlock *block = m_map->getBlockNoCreateNoEx(blockpos);
		if (block != NULL) {
			modified_blocks[blockpos] =  block;
			changed_nodes.emplace_back(p0, n00);
		}

		/*
			enqueue neighbors for update if neccessary
		 */
		switch (m_ndef->get(n0.getContent()).liquid_type) {
			case LIQUID_SOURCE:
			case LIQUID_FLOWING:
				// make sure source flows into all neighboring nodes
				for (u16 i = 0; i < num_flows; i++)
					if (flows[i].t != NEIGHBOR_UPPER)
						m_liquid_queue.push_back(flows[i].p);
				for (u16 i = 0; i < num_airs; i++)
					if (airs[i].t != NEIGHBOR_UPPER)
						m_liquid_queue.push_back(airs[i].p);
				break;
			case LIQUID_NONE:
				// this flow has turned to air; neighboring flows might need to do the same
				for (u16 i = 0; i < num_flows; i++)
					m_liquid_queue.push_back(flows[i].p);
				break;
		}
	}

	for (auto &iter : must_reflow)
		m_liquid_queue.push_back(iter);

	voxalgo::update_lighting_nodes(m_map, changed_nodes, modified_blocks);

	/* ----------------------------------------------------------------------
	 * Manage the queue so that it does not grow indefinately
	 */
	u16 time_until_purge = g_settings->getU16("liquid_queue_purge_time");

	if (time_until_purge == 0)
		return; // Feature disabled

	time_until_purge *= 1000;	// seconds -> milliseconds

	u64 curr_time = porting::getTimeMs();
	u32 prev_unprocessed = m_unprocessed_count;
	m_unprocessed_count = m_liquid_queue.size();

	// if unprocessed block count is decreasing or stable
	if (m_unprocessed_count <= prev_unprocessed) {
		m_queue_size_timer_started = false;
	} else {
		if (!m_queue_size_timer_started)
			m_inc_trending_up_start_time = curr_time;
		m_queue_size_timer_started = true;
	}

	// Account for curr_time overflowing
	if (m_queue_size_timer_started && m_inc_trending_up_start_time > curr_time)
		m_queue_size_timer_started = false;

	/* If the queue has been growing for more than liquid_queue_purge_time seconds
	 * and the number of unprocessed blocks is still > liquid_loop_max then we
	 * cannot keep up; dump the oldest blocks from the queue so that the queue
	 * has liquid_loop_max items in it
	 */
	if (m_queue_size_timer_started
			&& curr_time - m_inc_trending_up_start_time > time_until_purge
			&& m_unprocessed_count > liquid_loop_max) {

		size_t dump_qty = m_unprocessed_count - liquid_loop_max;

		infostream << "transformLiquids(): DUMPING " << dump_qty
		           << " blocks from the queue" << std::endl;

		while (dump_qty--)
			m_liquid_queue.pop_front();

		m_queue_size_timer_started = false; // optimistically assume we can keep up now
		m_unprocessed_count = m_liquid_queue.size();
	}
}
