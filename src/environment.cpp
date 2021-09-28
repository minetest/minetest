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

#include <fstream>
#include "environment.h"
#include "collision.h"
#include "raycast.h"
#include "scripting_server.h"
#include "server.h"
#include "daynightratio.h"
#include "emerge.h"


Environment::Environment(IGameDef *gamedef):
	m_time_of_day_speed(0.0f),
	m_day_count(0),
	m_gamedef(gamedef)
{
	m_cache_enable_shaders = g_settings->getBool("enable_shaders");
	m_cache_active_block_mgmt_interval = g_settings->getFloat("active_block_mgmt_interval");
	m_cache_abm_interval = g_settings->getFloat("abm_interval");
	m_cache_nodetimer_interval = g_settings->getFloat("nodetimer_interval");
	m_cache_abm_time_budget = g_settings->getFloat("abm_time_budget");

	m_time_of_day = g_settings->getU32("world_start_time");
	m_time_of_day_f = (float)m_time_of_day / 24000.0f;
}

u32 Environment::getDayNightRatio()
{
	MutexAutoLock lock(this->m_time_lock);
	if (m_enable_day_night_ratio_override)
		return m_day_night_ratio_override;
	return time_to_daynight_ratio(m_time_of_day_f * 24000, m_cache_enable_shaders);
}

void Environment::setTimeOfDaySpeed(float speed)
{
	m_time_of_day_speed = speed;
}

void Environment::setDayNightRatioOverride(bool enable, u32 value)
{
	MutexAutoLock lock(this->m_time_lock);
	m_enable_day_night_ratio_override = enable;
	m_day_night_ratio_override = value;
}

void Environment::setTimeOfDay(u32 time)
{
	MutexAutoLock lock(this->m_time_lock);
	if (m_time_of_day > time)
		++m_day_count;
	m_time_of_day = time;
	m_time_of_day_f = (float)time / 24000.0;
}

u32 Environment::getTimeOfDay()
{
	MutexAutoLock lock(this->m_time_lock);
	return m_time_of_day;
}

float Environment::getTimeOfDayF()
{
	MutexAutoLock lock(this->m_time_lock);
	return m_time_of_day_f;
}

bool Environment::line_of_sight(v3f pos1, v3f pos2, v3s16 *p)
{
	// Iterate trough nodes on the line
	voxalgo::VoxelLineIterator iterator(pos1 / BS, (pos2 - pos1) / BS);
	do {
		MapNode n = getMap().getNode(iterator.m_current_node_pos);

		// Return non-air
		if (n.param0 != CONTENT_AIR) {
			if (p)
				*p = iterator.m_current_node_pos;
			return false;
		}
		iterator.next();
	} while (iterator.m_current_index <= iterator.m_last_index);
	return true;
}

/*
	Check if a node is pointable
*/
inline static bool isPointableNode(const MapNode &n,
	const NodeDefManager *nodedef , bool liquids_pointable)
{
	const ContentFeatures &features = nodedef->get(n);
	return features.pointable ||
	       (liquids_pointable && features.isLiquid());
}

void Environment::continueRaycast(RaycastState *state, PointedThing *result)
{
	const NodeDefManager *nodedef = getMap().getNodeDefManager();
	if (state->m_initialization_needed) {
		// Add objects
		if (state->m_objects_pointable) {
			std::vector<PointedThing> found;
			getSelectedActiveObjects(state->m_shootline, found);
			for (const PointedThing &pointed : found) {
				state->m_found.push(pointed);
			}
		}
		// Set search range
		core::aabbox3d<s16> maximal_exceed = nodedef->getSelectionBoxIntUnion();
		state->m_search_range.MinEdge = -maximal_exceed.MaxEdge;
		state->m_search_range.MaxEdge = -maximal_exceed.MinEdge;
		// Setting is done
		state->m_initialization_needed = false;
	}

	// The index of the first pointed thing that was not returned
	// before. The last index which needs to be tested.
	s16 lastIndex = state->m_iterator.m_last_index;
	if (!state->m_found.empty()) {
		lastIndex = state->m_iterator.getIndex(
			floatToInt(state->m_found.top().intersection_point, BS));
	}

	Map &map = getMap();
	// If a node is found, this is the center of the
	// first nodebox the shootline meets.
	v3f found_boxcenter(0, 0, 0);
	// The untested nodes are in this range.
	core::aabbox3d<s16> new_nodes;
	while (state->m_iterator.m_current_index <= lastIndex) {
		// Test the nodes around the current node in search_range.
		new_nodes = state->m_search_range;
		new_nodes.MinEdge += state->m_iterator.m_current_node_pos;
		new_nodes.MaxEdge += state->m_iterator.m_current_node_pos;

		// Only check new nodes
		v3s16 delta = state->m_iterator.m_current_node_pos
			- state->m_previous_node;
		if (delta.X > 0) {
			new_nodes.MinEdge.X = new_nodes.MaxEdge.X;
		} else if (delta.X < 0) {
			new_nodes.MaxEdge.X = new_nodes.MinEdge.X;
		} else if (delta.Y > 0) {
			new_nodes.MinEdge.Y = new_nodes.MaxEdge.Y;
		} else if (delta.Y < 0) {
			new_nodes.MaxEdge.Y = new_nodes.MinEdge.Y;
		} else if (delta.Z > 0) {
			new_nodes.MinEdge.Z = new_nodes.MaxEdge.Z;
		} else if (delta.Z < 0) {
			new_nodes.MaxEdge.Z = new_nodes.MinEdge.Z;
		}

		// For each untested node
		for (s16 x = new_nodes.MinEdge.X; x <= new_nodes.MaxEdge.X; x++)
		for (s16 y = new_nodes.MinEdge.Y; y <= new_nodes.MaxEdge.Y; y++)
		for (s16 z = new_nodes.MinEdge.Z; z <= new_nodes.MaxEdge.Z; z++) {
			MapNode n;
			v3s16 np(x, y, z);
			bool is_valid_position;

			n = map.getNode(np, &is_valid_position);
			if (!(is_valid_position && isPointableNode(n, nodedef,
					state->m_liquids_pointable))) {
				continue;
			}

			PointedThing result;

			std::vector<aabb3f> boxes;
			n.getSelectionBoxes(nodedef, &boxes,
				n.getNeighbors(np, &map));

			// Is there a collision with a selection box?
			bool is_colliding = false;
			// Minimal distance of all collisions
			float min_distance_sq = 10000000;
			// ID of the current box (loop counter)
			u16 id = 0;

			v3f npf = intToFloat(np, BS);
			// This loop translates the boxes to their in-world place.
			for (aabb3f &box : boxes) {
				box.MinEdge += npf;
				box.MaxEdge += npf;

				v3f intersection_point;
				v3s16 intersection_normal;
				if (!boxLineCollision(box, state->m_shootline.start,
						state->m_shootline.getVector(), &intersection_point,
						&intersection_normal)) {
					++id;
					continue;
				}

				f32 distanceSq = (intersection_point
					- state->m_shootline.start).getLengthSQ();
				// If this is the nearest collision, save it
				if (min_distance_sq > distanceSq) {
					min_distance_sq = distanceSq;
					result.intersection_point = intersection_point;
					result.intersection_normal = intersection_normal;
					result.box_id = id;
					found_boxcenter = box.getCenter();
					is_colliding = true;
				}
				++id;
			}
			// If there wasn't a collision, stop
			if (!is_colliding) {
				continue;
			}
			result.type = POINTEDTHING_NODE;
			result.node_undersurface = np;
			result.distanceSq = min_distance_sq;
			// Set undersurface and abovesurface nodes
			f32 d = 0.002 * BS;
			v3f fake_intersection = result.intersection_point;
			// Move intersection towards its source block.
			if (fake_intersection.X < found_boxcenter.X) {
				fake_intersection.X += d;
			} else {
				fake_intersection.X -= d;
			}
			if (fake_intersection.Y < found_boxcenter.Y) {
				fake_intersection.Y += d;
			} else {
				fake_intersection.Y -= d;
			}
			if (fake_intersection.Z < found_boxcenter.Z) {
				fake_intersection.Z += d;
			} else {
				fake_intersection.Z -= d;
			}
			result.node_real_undersurface = floatToInt(
				fake_intersection, BS);
			result.node_abovesurface = result.node_real_undersurface
				+ result.intersection_normal;
			// Push found PointedThing
			state->m_found.push(result);
			// If this is nearer than the old nearest object,
			// the search can be shorter
			s16 newIndex = state->m_iterator.getIndex(
				result.node_real_undersurface);
			if (newIndex < lastIndex) {
				lastIndex = newIndex;
			}
		}
		// Next node
		state->m_previous_node = state->m_iterator.m_current_node_pos;
		state->m_iterator.next();
	}
	// Return empty PointedThing if nothing left on the ray
	if (state->m_found.empty()) {
		result->type = POINTEDTHING_NOTHING;
	} else {
		*result = state->m_found.top();
		state->m_found.pop();
	}
}

void Environment::stepTimeOfDay(float dtime)
{
	MutexAutoLock lock(this->m_time_lock);

	// Cached in order to prevent the two reads we do to give
	// different results (can be written by code not under the lock)
	f32 cached_time_of_day_speed = m_time_of_day_speed;

	f32 speed = cached_time_of_day_speed * 24000. / (24. * 3600);
	m_time_conversion_skew += dtime;
	u32 units = (u32)(m_time_conversion_skew * speed);
	bool sync_f = false;
	if (units > 0) {
		// Sync at overflow
		if (m_time_of_day + units >= 24000) {
			sync_f = true;
			++m_day_count;
		}
		m_time_of_day = (m_time_of_day + units) % 24000;
		if (sync_f)
			m_time_of_day_f = (float)m_time_of_day / 24000.0;
	}
	if (speed > 0) {
		m_time_conversion_skew -= (f32)units / speed;
	}
	if (!sync_f) {
		m_time_of_day_f += cached_time_of_day_speed / 24 / 3600 * dtime;
		if (m_time_of_day_f > 1.0)
			m_time_of_day_f -= 1.0;
		if (m_time_of_day_f < 0.0)
			m_time_of_day_f += 1.0;
	}
}

u32 Environment::getDayCount()
{
	// Atomic<u32> counter
	return m_day_count;
}
