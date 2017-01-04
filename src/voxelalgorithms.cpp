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

#include "voxelalgorithms.h"
#include "nodedef.h"
#include "mapblock.h"
#include "map.h"

namespace voxalgo
{

void setLight(VoxelManipulator &v, VoxelArea a, u8 light,
		INodeDefManager *ndef)
{
	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
	{
		v3s16 p(x,y,z);
		MapNode &n = v.getNodeRefUnsafe(p);
		n.setLight(LIGHTBANK_DAY, light, ndef);
		n.setLight(LIGHTBANK_NIGHT, light, ndef);
	}
}

void clearLightAndCollectSources(VoxelManipulator &v, VoxelArea a,
		enum LightBank bank, INodeDefManager *ndef,
		std::set<v3s16> & light_sources,
		std::map<v3s16, u8> & unlight_from)
{
	// The full area we shall touch
	VoxelArea required_a = a;
	required_a.pad(v3s16(0,0,0));
	// Make sure we have access to it
	v.addArea(a);

	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	for(s32 y=a.MinEdge.Y; y<=a.MaxEdge.Y; y++)
	{
		v3s16 p(x,y,z);
		MapNode &n = v.getNodeRefUnsafe(p);
		u8 oldlight = n.getLight(bank, ndef);
		n.setLight(bank, 0, ndef);

		// If node sources light, add to list
		u8 source = ndef->get(n).light_source;
		if(source != 0)
			light_sources.insert(p);

		// Collect borders for unlighting
		if((x==a.MinEdge.X || x == a.MaxEdge.X
		|| y==a.MinEdge.Y || y == a.MaxEdge.Y
		|| z==a.MinEdge.Z || z == a.MaxEdge.Z)
		&& oldlight != 0)
		{
			unlight_from[p] = oldlight;
		}
	}
}

SunlightPropagateResult propagateSunlight(VoxelManipulator &v, VoxelArea a,
		bool inexistent_top_provides_sunlight,
		std::set<v3s16> & light_sources,
		INodeDefManager *ndef)
{
	// Return values
	bool bottom_sunlight_valid = true;

	// The full area we shall touch extends one extra at top and bottom
	VoxelArea required_a = a;
	required_a.pad(v3s16(0,1,0));
	// Make sure we have access to it
	v.addArea(a);

	s16 max_y = a.MaxEdge.Y;
	s16 min_y = a.MinEdge.Y;

	for(s32 x=a.MinEdge.X; x<=a.MaxEdge.X; x++)
	for(s32 z=a.MinEdge.Z; z<=a.MaxEdge.Z; z++)
	{
		v3s16 p_overtop(x, max_y+1, z);
		bool overtop_has_sunlight = false;
		// If overtop node does not exist, trust heuristics
		if(!v.exists(p_overtop))
			overtop_has_sunlight = inexistent_top_provides_sunlight;
		else if(v.getNodeRefUnsafe(p_overtop).getContent() == CONTENT_IGNORE)
			overtop_has_sunlight = inexistent_top_provides_sunlight;
		// Otherwise refer to it's light value
		else
			overtop_has_sunlight = (v.getNodeRefUnsafe(p_overtop).getLight(
					LIGHTBANK_DAY, ndef) == LIGHT_SUN);

		// Copy overtop's sunlight all over the place
		u8 incoming_light = overtop_has_sunlight ? LIGHT_SUN : 0;
		for(s32 y=max_y; y>=min_y; y--)
		{
			v3s16 p(x,y,z);
			MapNode &n = v.getNodeRefUnsafe(p);
			if(incoming_light == 0){
				// Do nothing
			} else if(incoming_light == LIGHT_SUN &&
					ndef->get(n).sunlight_propagates){
				// Do nothing
			} else if(ndef->get(n).sunlight_propagates == false){
				incoming_light = 0;
			} else {
				incoming_light = diminish_light(incoming_light);
			}
			u8 old_light = n.getLight(LIGHTBANK_DAY, ndef);

			if(incoming_light > old_light)
				n.setLight(LIGHTBANK_DAY, incoming_light, ndef);

			if(diminish_light(incoming_light) != 0)
				light_sources.insert(p);
		}

		// Check validity of sunlight at top of block below if it
		// hasn't already been proven invalid
		if(bottom_sunlight_valid)
		{
			bool sunlight_should_continue_down = (incoming_light == LIGHT_SUN);
			v3s16 p_overbottom(x, min_y-1, z);
			if(!v.exists(p_overbottom) ||
					v.getNodeRefUnsafe(p_overbottom
							).getContent() == CONTENT_IGNORE){
				// Is not known, cannot compare
			} else {
				bool overbottom_has_sunlight = (v.getNodeRefUnsafe(p_overbottom
						).getLight(LIGHTBANK_DAY, ndef) == LIGHT_SUN);
				if(sunlight_should_continue_down != overbottom_has_sunlight){
					bottom_sunlight_valid = false;
				}
			}
		}
	}

	return SunlightPropagateResult(bottom_sunlight_valid);
}

/*!
 * A direction.
 * 0=X+
 * 1=Y+
 * 2=Z+
 * 3=Z-
 * 4=Y-
 * 5=X-
 * 6=no direction
 * Two directions are opposite only if their sum is 5.
 */
typedef u8 direction;
/*!
 * Relative node position.
 * This represents a node's position in its map block.
 * All coordinates must be between 0 and 15.
 */
typedef v3s16 relative_v3;
/*!
 * Position of a map block (block coordinates).
 * One block_pos unit is as long as 16 node position units.
 */
typedef v3s16 mapblock_v3;

//! Contains information about a node whose light is about to change.
struct ChangingLight {
	//! Relative position of the node in its map block.
	relative_v3 rel_position;
	//! Position of the node's block.
	mapblock_v3 block_position;
	//! Pointer to the node's block.
	MapBlock *block;
	/*!
	 * Direction from the node that caused this node's changing
	 * to this node.
	 */
	direction source_direction;

	ChangingLight() :
		rel_position(),
		block_position(),
		block(NULL),
		source_direction(6)
	{}

	ChangingLight(relative_v3 rel_pos, mapblock_v3 block_pos,
		MapBlock *b, direction source_dir) :
		rel_position(rel_pos),
		block_position(block_pos),
		block(b),
		source_direction(source_dir)
	{}
};

/*!
 * A fast, priority queue-like container to contain ChangingLights.
 * The ChangingLights are ordered by the given light levels.
 * The brightest ChangingLight is returned first.
 */
struct LightQueue {
	//! For each light level there is a vector.
	std::vector<ChangingLight> lights[LIGHT_SUN + 1];
	//! Light of the brightest ChangingLight in the queue.
	u8 max_light;

	/*!
	 * Creates a LightQueue.
	 * \param reserve for each light level that many slots are reserved.
	 */
	LightQueue(size_t reserve)
	{
		max_light = LIGHT_SUN;
		for (u8 i = 0; i <= LIGHT_SUN; i++) {
			lights[i].reserve(reserve);
		}
	}

	/*!
	 * Returns the next brightest ChangingLight and
	 * removes it from the queue.
	 * If there were no elements in the queue, the given parameters
	 * remain unmodified.
	 * \param light light level of the popped ChangingLight
	 * \param data the ChangingLight that was popped
	 * \returns true if there was a ChangingLight in the queue.
	 */
	bool next(u8 &light, ChangingLight &data)
	{
		while (lights[max_light].empty()) {
			if (max_light == 0) {
				return false;
			}
			max_light--;
		}
		light = max_light;
		data = lights[max_light].back();
		lights[max_light].pop_back();
		return true;
	}

	/*!
	 * Adds an element to the queue.
	 * The parameters are the same as in ChangingLight's constructor.
	 * \param light light level of the ChangingLight
	 */
	inline void push(u8 light, const relative_v3 &rel_pos,
		const mapblock_v3 &block_pos, MapBlock *block,
		direction source_dir)
	{
		assert(light <= LIGHT_SUN);
		lights[light].push_back(
			ChangingLight(rel_pos, block_pos, block, source_dir));
	}
};

/*!
 * This type of light queue is for unlighting.
 * A node can be pushed in it only if its raw light is zero.
 * This prevents pushing nodes twice into this queue.
 * The light of the pushed ChangingLight must be the
 * light of the node before unlighting it.
 */
typedef LightQueue UnlightQueue;
/*!
 * This type of light queue is for spreading lights.
 * While spreading lights, all the nodes in it must
 * have the same light as the light level the ChangingLights
 * were pushed into this queue with. This prevents unnecessary
 * re-pushing of the nodes into the queue.
 * If a node doesn't let light trough but emits light, it can be added
 * too.
 */
typedef LightQueue ReLightQueue;

/*!
 * neighbor_dirs[i] points towards
 * the direction i.
 * See the definition of the type "direction"
 */
const static v3s16 neighbor_dirs[6] = {
	v3s16(1, 0, 0), // right
	v3s16(0, 1, 0), // top
	v3s16(0, 0, 1), // back
	v3s16(0, 0, -1), // front
	v3s16(0, -1, 0), // bottom
	v3s16(-1, 0, 0), // left
};

/*!
 * Transforms the given map block offset by one node towards
 * the specified direction.
 * \param dir the direction of the transformation
 * \param rel_pos the node's relative position in its map block
 * \param block_pos position of the node's block
 */
bool step_rel_block_pos(direction dir, relative_v3 &rel_pos,
	mapblock_v3 &block_pos)
{
	switch (dir) {
	case 0:
		if (rel_pos.X < MAP_BLOCKSIZE - 1) {
			rel_pos.X++;
		} else {
			rel_pos.X = 0;
			block_pos.X++;
			return true;
		}
		break;
	case 1:
		if (rel_pos.Y < MAP_BLOCKSIZE - 1) {
			rel_pos.Y++;
		} else {
			rel_pos.Y = 0;
			block_pos.Y++;
			return true;
		}
		break;
	case 2:
		if (rel_pos.Z < MAP_BLOCKSIZE - 1) {
			rel_pos.Z++;
		} else {
			rel_pos.Z = 0;
			block_pos.Z++;
			return true;
		}
		break;
	case 3:
		if (rel_pos.Z > 0) {
			rel_pos.Z--;
		} else {
			rel_pos.Z = MAP_BLOCKSIZE - 1;
			block_pos.Z--;
			return true;
		}
		break;
	case 4:
		if (rel_pos.Y > 0) {
			rel_pos.Y--;
		} else {
			rel_pos.Y = MAP_BLOCKSIZE - 1;
			block_pos.Y--;
			return true;
		}
		break;
	case 5:
		if (rel_pos.X > 0) {
			rel_pos.X--;
		} else {
			rel_pos.X = MAP_BLOCKSIZE - 1;
			block_pos.X--;
			return true;
		}
		break;
	}
	return false;
}

/*
 * Removes all light that is potentially emitted by the specified
 * light sources. These nodes will have zero light.
 * Returns all nodes whose light became zero but should be re-lighted.
 *
 * \param bank the light bank in which the procedure operates
 * \param from_nodes nodes whose light is removed
 * \param light_sources nodes that should be re-lighted
 * \param modified_blocks output, all modified map blocks are added to this
 */
void unspread_light(Map *map, INodeDefManager *nodemgr, LightBank bank,
	UnlightQueue &from_nodes, ReLightQueue &light_sources,
	std::map<v3s16, MapBlock*> &modified_blocks)
{
	// Stores data popped from from_nodes
	u8 current_light;
	ChangingLight current;
	// Data of the current neighbor
	mapblock_v3 neighbor_block_pos;
	relative_v3 neighbor_rel_pos;
	// A dummy boolean
	bool is_valid_position;
	// Direction of the brightest neighbor of the node
	direction source_dir;
	while (from_nodes.next(current_light, current)) {
		// For all nodes that need unlighting

		// There is no brightest neighbor
		source_dir = 6;
		// The current node
		const MapNode &node = current.block->getNodeNoCheck(
			current.rel_position, &is_valid_position);
		const ContentFeatures &f = nodemgr->get(node);
		// If the node emits light, it behaves like it had a
		// brighter neighbor.
		u8 brightest_neighbor_light = f.light_source + 1;
		for (direction i = 0; i < 6; i++) {
			//For each neighbor

			// The node that changed this node has already zero light
			// and it can't give light to this node
			if (current.source_direction + i == 5) {
				continue;
			}
			// Get the neighbor's position and block
			neighbor_rel_pos = current.rel_position;
			neighbor_block_pos = current.block_position;
			MapBlock *neighbor_block;
			if (step_rel_block_pos(i, neighbor_rel_pos, neighbor_block_pos)) {
				neighbor_block = map->getBlockNoCreateNoEx(neighbor_block_pos);
				if (neighbor_block == NULL) {
					continue;
				}
			} else {
				neighbor_block = current.block;
			}
			// Get the neighbor itself
			MapNode neighbor = neighbor_block->getNodeNoCheck(neighbor_rel_pos,
				&is_valid_position);
			const ContentFeatures &neighbor_f = nodemgr->get(
				neighbor.getContent());
			u8 neighbor_light = neighbor.getLightRaw(bank, neighbor_f);
			// If the neighbor has at least as much light as this node, then
			// it won't lose its light, since it should have been added to
			// from_nodes earlier, so its light would be zero.
			if (neighbor_f.light_propagates && neighbor_light < current_light) {
				// Unlight, but only if the node has light.
				if (neighbor_light > 0) {
					neighbor.setLight(bank, 0, neighbor_f);
					neighbor_block->setNodeNoCheck(neighbor_rel_pos, neighbor);
					from_nodes.push(neighbor_light, neighbor_rel_pos,
						neighbor_block_pos, neighbor_block, i);
					// The current node was modified earlier, so its block
					// is in modified_blocks.
					if (current.block != neighbor_block) {
						modified_blocks[neighbor_block_pos] = neighbor_block;
					}
				}
			} else {
				// The neighbor can light up this node.
				if (neighbor_light < neighbor_f.light_source) {
					neighbor_light = neighbor_f.light_source;
				}
				if (brightest_neighbor_light < neighbor_light) {
					brightest_neighbor_light = neighbor_light;
					source_dir = i;
				}
			}
		}
		// If the brightest neighbor is able to light up this node,
		// then add this node to the output nodes.
		if (brightest_neighbor_light > 1 && f.light_propagates) {
			brightest_neighbor_light--;
			light_sources.push(brightest_neighbor_light, current.rel_position,
				current.block_position, current.block,
				(source_dir == 6) ? 6 : 5 - source_dir
				/* with opposite direction*/);
		}
	}
}

/*
 * Spreads light from the specified starting nodes.
 *
 * Before calling this procedure, make sure that all ChangingLights
 * in light_sources have as much light on the map as they have in
 * light_sources (if the queue contains a node multiple times, the brightest
 * occurrence counts).
 *
 * \param bank the light bank in which the procedure operates
 * \param light_sources starting nodes
 * \param modified_blocks output, all modified map blocks are added to this
 */
void spread_light(Map *map, INodeDefManager *nodemgr, LightBank bank,
	LightQueue &light_sources, std::map<v3s16, MapBlock*> &modified_blocks)
{
	// The light the current node can provide to its neighbors.
	u8 spreading_light;
	// The ChangingLight for the current node.
	ChangingLight current;
	// Position of the current neighbor.
	mapblock_v3 neighbor_block_pos;
	relative_v3 neighbor_rel_pos;
	// A dummy boolean.
	bool is_valid_position;
	while (light_sources.next(spreading_light, current)) {
		spreading_light--;
		for (direction i = 0; i < 6; i++) {
			// This node can't light up its light source
			if (current.source_direction + i == 5) {
				continue;
			}
			// Get the neighbor's position and block
			neighbor_rel_pos = current.rel_position;
			neighbor_block_pos = current.block_position;
			MapBlock *neighbor_block;
			if (step_rel_block_pos(i, neighbor_rel_pos, neighbor_block_pos)) {
				neighbor_block = map->getBlockNoCreateNoEx(neighbor_block_pos);
				if (neighbor_block == NULL) {
					continue;
				}
			} else {
				neighbor_block = current.block;
			}
			// Get the neighbor itself
			MapNode neighbor = neighbor_block->getNodeNoCheck(neighbor_rel_pos,
				&is_valid_position);
			const ContentFeatures &f = nodemgr->get(neighbor.getContent());
			if (f.light_propagates) {
				// Light up the neighbor, if it has less light than it should.
				u8 neighbor_light = neighbor.getLightRaw(bank, f);
				if (neighbor_light < spreading_light) {
					neighbor.setLight(bank, spreading_light, f);
					neighbor_block->setNodeNoCheck(neighbor_rel_pos, neighbor);
					light_sources.push(spreading_light, neighbor_rel_pos,
						neighbor_block_pos, neighbor_block, i);
					// The current node was modified earlier, so its block
					// is in modified_blocks.
					if (current.block != neighbor_block) {
						modified_blocks[neighbor_block_pos] = neighbor_block;
					}
				}
			}
		}
	}
}

/*!
 * Returns true if the node gets sunlight from the
 * node above it.
 *
 * \param pos position of the node.
 */
bool is_sunlight_above(Map *map, v3s16 pos, INodeDefManager *ndef)
{
	bool sunlight = true;
	mapblock_v3 source_block_pos;
	relative_v3 source_rel_pos;
	getNodeBlockPosWithOffset(pos + v3s16(0, 1, 0), source_block_pos,
		source_rel_pos);
	// If the node above has sunlight, this node also can get it.
	MapBlock *source_block = map->getBlockNoCreateNoEx(source_block_pos);
	if (source_block == NULL) {
		// But if there is no node above, then use heuristics
		MapBlock *node_block = map->getBlockNoCreateNoEx(getNodeBlockPos(pos));
		if (node_block == NULL) {
			sunlight = false;
		} else {
			sunlight = !node_block->getIsUnderground();
		}
	} else {
		bool is_valid_position;
		MapNode above = source_block->getNodeNoCheck(source_rel_pos,
			&is_valid_position);
		if (is_valid_position) {
			if (above.getContent() == CONTENT_IGNORE) {
				// Trust heuristics
				if (source_block->getIsUnderground()) {
					sunlight = false;
				}
			} else if (above.getLight(LIGHTBANK_DAY, ndef) != LIGHT_SUN) {
				// If the node above doesn't have sunlight, this
				// node is in shadow.
				sunlight = false;
			}
		}
	}
	return sunlight;
}

static const LightBank banks[] = { LIGHTBANK_DAY, LIGHTBANK_NIGHT };

void update_lighting_nodes(Map *map, INodeDefManager *ndef,
	std::vector<std::pair<v3s16, MapNode> > &oldnodes,
	std::map<v3s16, MapBlock*> &modified_blocks)
{
	// For node getter functions
	bool is_valid_position;

	// Process each light bank separately
	for (s32 i = 0; i < 2; i++) {
		LightBank bank = banks[i];
		UnlightQueue disappearing_lights(256);
		ReLightQueue light_sources(256);
		// For each changed node process sunlight and initialize
		for (std::vector<std::pair<v3s16, MapNode> >::iterator it =
				oldnodes.begin(); it < oldnodes.end(); ++it) {
			// Get position and block of the changed node
			v3s16 p = it->first;
			relative_v3 rel_pos;
			mapblock_v3 block_pos;
			getNodeBlockPosWithOffset(p, block_pos, rel_pos);
			MapBlock *block = map->getBlockNoCreateNoEx(block_pos);
			if (block == NULL || block->isDummy()) {
				continue;
			}
			// Get the new node
			MapNode n = block->getNodeNoCheck(rel_pos, &is_valid_position);
			if (!is_valid_position) {
				break;
			}

			// Light of the old node
			u8 old_light = it->second.getLight(bank, ndef);

			// Add the block of the added node to modified_blocks
			modified_blocks[block_pos] = block;

			// Get new light level of the node
			u8 new_light = 0;
			if (ndef->get(n).light_propagates) {
				if (bank == LIGHTBANK_DAY && ndef->get(n).sunlight_propagates
					&& is_sunlight_above(map, p, ndef)) {
					new_light = LIGHT_SUN;
				} else {
					new_light = ndef->get(n).light_source;
					for (int i = 0; i < 6; i++) {
						v3s16 p2 = p + neighbor_dirs[i];
						bool is_valid;
						MapNode n2 = map->getNodeNoEx(p2, &is_valid);
						if (is_valid) {
							u8 spread = n2.getLight(bank, ndef);
							// If the neighbor is at least as bright as
							// this node then its light is not from
							// this node.
							// Its light can spread to this node.
							if (spread > new_light && spread >= old_light) {
								new_light = spread - 1;
							}
						}
					}
				}
			} else {
				// If this is an opaque node, it still can emit light.
				new_light = ndef->get(n).light_source;
			}

			if (new_light > 0) {
				light_sources.push(new_light, rel_pos, block_pos, block, 6);
			}

			if (new_light < old_light) {
				// The node became opaque or doesn't provide as much
				// light as the previous one, so it must be unlighted.

				// Add to unlight queue
				n.setLight(bank, 0, ndef);
				block->setNodeNoCheck(rel_pos, n);
				disappearing_lights.push(old_light, rel_pos, block_pos, block,
					6);

				// Remove sunlight, if there was any
				if (bank == LIGHTBANK_DAY && old_light == LIGHT_SUN) {
					for (s16 y = p.Y - 1;; y--) {
						v3s16 n2pos(p.X, y, p.Z);

						MapNode n2;

						n2 = map->getNodeNoEx(n2pos, &is_valid_position);
						if (!is_valid_position)
							break;

						// If this node doesn't have sunlight, the nodes below
						// it don't have too.
						if (n2.getLight(LIGHTBANK_DAY, ndef) != LIGHT_SUN) {
							break;
						}
						// Remove sunlight and add to unlight queue.
						n2.setLight(LIGHTBANK_DAY, 0, ndef);
						map->setNode(n2pos, n2);
						relative_v3 rel_pos2;
						mapblock_v3 block_pos2;
						getNodeBlockPosWithOffset(n2pos, block_pos2, rel_pos2);
						MapBlock *block2 = map->getBlockNoCreateNoEx(
							block_pos2);
						disappearing_lights.push(LIGHT_SUN, rel_pos2,
							block_pos2, block2,
							4 /* The node above caused the change */);
					}
				}
			} else if (new_light > old_light) {
				// It is sure that the node provides more light than the previous
				// one, unlighting is not necessary.
				// Propagate sunlight
				if (bank == LIGHTBANK_DAY && new_light == LIGHT_SUN) {
					for (s16 y = p.Y - 1;; y--) {
						v3s16 n2pos(p.X, y, p.Z);

						MapNode n2;

						n2 = map->getNodeNoEx(n2pos, &is_valid_position);
						if (!is_valid_position)
							break;

						// This should not happen, but if the node has sunlight
						// then the iteration should stop.
						if (n2.getLight(LIGHTBANK_DAY, ndef) == LIGHT_SUN) {
							break;
						}
						// If the node terminates sunlight, stop.
						if (!ndef->get(n2).sunlight_propagates) {
							break;
						}
						relative_v3 rel_pos2;
						mapblock_v3 block_pos2;
						getNodeBlockPosWithOffset(n2pos, block_pos2, rel_pos2);
						MapBlock *block2 = map->getBlockNoCreateNoEx(
							block_pos2);
						// Mark node for lighting.
						light_sources.push(LIGHT_SUN, rel_pos2, block_pos2,
							block2, 4);
					}
				}
			}

		}
		// Remove lights
		unspread_light(map, ndef, bank, disappearing_lights, light_sources,
			modified_blocks);
		// Initialize light values for light spreading.
		for (u8 i = 0; i <= LIGHT_SUN; i++) {
			const std::vector<ChangingLight> &lights = light_sources.lights[i];
			for (std::vector<ChangingLight>::const_iterator it = lights.begin();
					it < lights.end(); it++) {
				MapNode n = it->block->getNodeNoCheck(it->rel_position,
					&is_valid_position);
				n.setLight(bank, i, ndef);
				it->block->setNodeNoCheck(it->rel_position, n);
			}
		}
		// Spread lights.
		spread_light(map, ndef, bank, light_sources, modified_blocks);
	}
}

VoxelLineIterator::VoxelLineIterator(
	const v3f &start_position,
	const v3f &line_vector) :
	m_start_position(start_position),
	m_line_vector(line_vector),
	m_next_intersection_multi(10000.0f, 10000.0f, 10000.0f),
	m_intersection_multi_inc(10000.0f, 10000.0f, 10000.0f),
	m_step_directions(1.0f, 1.0f, 1.0f)
{
	m_current_node_pos = floatToInt(m_start_position, 1);

	if (m_line_vector.X > 0) {
		m_next_intersection_multi.X = (floorf(m_start_position.X - 0.5) + 1.5
			- m_start_position.X) / m_line_vector.X;
		m_intersection_multi_inc.X = 1 / m_line_vector.X;
	} else if (m_line_vector.X < 0) {
		m_next_intersection_multi.X = (floorf(m_start_position.X - 0.5)
			- m_start_position.X + 0.5) / m_line_vector.X;
		m_intersection_multi_inc.X = -1 / m_line_vector.X;
		m_step_directions.X = -1;
	}

	if (m_line_vector.Y > 0) {
		m_next_intersection_multi.Y = (floorf(m_start_position.Y - 0.5) + 1.5
			- m_start_position.Y) / m_line_vector.Y;
		m_intersection_multi_inc.Y = 1 / m_line_vector.Y;
	} else if (m_line_vector.Y < 0) {
		m_next_intersection_multi.Y = (floorf(m_start_position.Y - 0.5)
			- m_start_position.Y + 0.5) / m_line_vector.Y;
		m_intersection_multi_inc.Y = -1 / m_line_vector.Y;
		m_step_directions.Y = -1;
	}

	if (m_line_vector.Z > 0) {
		m_next_intersection_multi.Z = (floorf(m_start_position.Z - 0.5) + 1.5
			- m_start_position.Z) / m_line_vector.Z;
		m_intersection_multi_inc.Z = 1 / m_line_vector.Z;
	} else if (m_line_vector.Z < 0) {
		m_next_intersection_multi.Z = (floorf(m_start_position.Z - 0.5)
			- m_start_position.Z + 0.5) / m_line_vector.Z;
		m_intersection_multi_inc.Z = -1 / m_line_vector.Z;
		m_step_directions.Z = -1;
	}

	m_has_next = (m_next_intersection_multi.X <= 1)
		|| (m_next_intersection_multi.Y <= 1)
		|| (m_next_intersection_multi.Z <= 1);
}

void VoxelLineIterator::next()
{
	if ((m_next_intersection_multi.X < m_next_intersection_multi.Y)
			&& (m_next_intersection_multi.X < m_next_intersection_multi.Z)) {
		m_next_intersection_multi.X += m_intersection_multi_inc.X;
		m_current_node_pos.X += m_step_directions.X;
	} else if ((m_next_intersection_multi.Y < m_next_intersection_multi.Z)) {
		m_next_intersection_multi.Y += m_intersection_multi_inc.Y;
		m_current_node_pos.Y += m_step_directions.Y;
	} else {
		m_next_intersection_multi.Z += m_intersection_multi_inc.Z;
		m_current_node_pos.Z += m_step_directions.Z;
	}

	m_has_next = (m_next_intersection_multi.X <= 1)
			|| (m_next_intersection_multi.Y <= 1)
			|| (m_next_intersection_multi.Z <= 1);
}

} // namespace voxalgo

