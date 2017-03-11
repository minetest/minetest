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

#ifndef VOXELALGORITHMS_HEADER
#define VOXELALGORITHMS_HEADER

#include "voxel.h"
#include "mapnode.h"
#include "util/container.h"
#include "util/cpp11_container.h"

class Map;
class ServerMap;
class MapBlock;
class MMVManip;

namespace voxalgo
{

// TODO: Move unspreadLight and spreadLight from VoxelManipulator to here

void setLight(VoxelManipulator &v, VoxelArea a, u8 light,
		INodeDefManager *ndef);

void clearLightAndCollectSources(VoxelManipulator &v, VoxelArea a,
		enum LightBank bank, INodeDefManager *ndef,
		std::set<v3s16> & light_sources,
		std::map<v3s16, u8> & unlight_from);

struct SunlightPropagateResult
{
	bool bottom_sunlight_valid;

	SunlightPropagateResult(bool bottom_sunlight_valid_):
		bottom_sunlight_valid(bottom_sunlight_valid_)
	{}
};

SunlightPropagateResult propagateSunlight(VoxelManipulator &v, VoxelArea a,
		bool inexistent_top_provides_sunlight,
		std::set<v3s16> & light_sources,
		INodeDefManager *ndef);

/*!
 * Updates the lighting on the map.
 * The result will be correct only if
 * no nodes were changed except the given ones.
 * Before calling this procedure make sure that all new nodes on
 * the map have zero light level!
 *
 * \param oldnodes contains the MapNodes that were replaced by the new
 * MapNodes and their positions
 * \param modified_blocks output, contains all map blocks that
 * the function modified
 */
void update_lighting_nodes(
	Map *map,
	std::vector<std::pair<v3s16, MapNode> > &oldnodes,
	std::map<v3s16, MapBlock*> &modified_blocks);

/*!
 * Updates borders of the given mapblock.
 * Only updates if the block was marked with incomplete
 * lighting and the neighbor is also loaded.
 *
 * \param block the block to update
 * \param modified_blocks output, contains all map blocks that
 * the function modified
 */
void update_block_border_lighting(Map *map, MapBlock *block,
	std::map<v3s16, MapBlock*> &modified_blocks);

/*!
 * Copies back nodes from a voxel manipulator
 * to the map and updates lighting.
 * For server use only.
 *
 * \param modified_blocks output, contains all map blocks that
 * the function modified
 */
void blit_back_with_light(ServerMap *map, MMVManip *vm,
	std::map<v3s16, MapBlock*> *modified_blocks);

/*!
 * Corrects the light in a map block.
 * For server use only.
 *
 * \param block the block to update
 */
void repair_block_light(ServerMap *map, MapBlock *block,
	std::map<v3s16, MapBlock*> *modified_blocks);

/*!
 * This class iterates trough voxels that intersect with
 * a line. The collision detection does not see nodeboxes,
 * every voxel is a cube and is returned.
 * This iterator steps to all nodes exactly once.
 */
struct VoxelLineIterator
{
public:
	//! Starting position of the line in world coordinates.
	v3f m_start_position;
	//! Direction and length of the line in world coordinates.
	v3f m_line_vector;
	/*!
	 * Each component stores the next smallest positive number, by
	 * which multiplying the line's vector gives a vector that ends
	 * on the intersection of two nodes.
	 */
	v3f m_next_intersection_multi;
	/*!
	 * Each component stores the smallest positive number, by which
	 * m_next_intersection_multi's components can be increased.
	 */
	v3f m_intersection_multi_inc;
	/*!
	 * Direction of the line. Each component can be -1 or 1 (if a
	 * component of the line's vector is 0, then there will be 1).
	 */
	v3s16 m_step_directions;
	//! Position of the current node.
	v3s16 m_current_node_pos;
	//! If true, the next node will intersect the line, too.
	bool m_has_next;

	/*!
	 * Creates a voxel line iterator with the given line.
	 * @param start_position starting point of the line
	 * in voxel coordinates
	 * @param line_vector    length and direction of the
	 * line in voxel coordinates. start_position+line_vector
	 * is the end of the line
	 */
	VoxelLineIterator(const v3f &start_position,const v3f &line_vector);

	/*!
	 * Steps to the next voxel.
	 * Updates m_current_node_pos and
	 * m_previous_node_pos.
	 * Note that it works even if hasNext() is false,
	 * continuing the line as a ray.
	 */
	void next();

	/*!
	 * Returns true if the next voxel intersects the given line.
	 */
	inline bool hasNext() const { return m_has_next; }
};

} // namespace voxalgo



#endif

