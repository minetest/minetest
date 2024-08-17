/*
Minetest
Copyright (C) 2024, ExeVirus <nodecastmt@gmail.com>

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

#include "spatial_map.h"
#include <algorithm>

namespace server
{

// all inserted entires go into the uncached vector
void SpatialMap::insert(u16 id, const v3f &pos)
{
	if (m_iterators_stopping_insertion_and_deletion) {
		m_pending_operations.emplace_back(pos, id, true);
		return;
	}

	SpatialKey key(pos);
	m_cached.insert({ key, id });
}

// Invalidates upon position update
void SpatialMap::updatePosition(u16 id, const v3f &oldPos, const v3f &newPos)
{
	// Try to leave early if already in the same bucket:
	auto range = m_cached.equal_range(SpatialKey(newPos));
	for (auto it = range.first; it != range.second; ++it) {
		if (it->second == id) {
			return; // all good, let's get out of here
		}
	}

	remove(id, oldPos); // remove from old cache position
	insert(id, newPos); // reinsert
}

void SpatialMap::remove(u16 id, const v3f &pos)
{
	if (m_iterators_stopping_insertion_and_deletion) {
		m_pending_operations.emplace_back(pos, id, false);
		return;
	}

	SpatialKey key(pos);
	if(m_cached.find(key) != m_cached.end()) {
		auto range = m_cached.equal_range(key);
		for (auto it = range.first; it != range.second; ++it) {
			if (it->second == id) {
				m_cached.erase(it);
				return; // Erase and leave early
			}
		}
	}

	remove(id); // can be expensive, try not to hit this
}

void SpatialMap::remove(u16 id)
{
	if(m_iterators_stopping_insertion_and_deletion) {
		m_pending_operations.emplace_back(v3f(), id, false);
		return;
	}

	for (auto it = m_cached.begin(); it != m_cached.end(); ++it) {
		if (it->second == id) {
			m_cached.erase(it);
			return; // Erase and leave early
		}
	}
}

void SpatialMap::removeAll()
{
	if(m_iterators_stopping_insertion_and_deletion) {
		m_remove_all = true;
	} else {
		m_cached.clear();
	}
}

// Note the use of a lambda callback instead of a vector of IDs to operate upon.
// In performance testing, the lambda allocation was faster than building and working with a vector
void SpatialMap::getRelevantObjectIds(const aabb3f &box, const std::function<void(u16 id)> &callback)
{
	if (m_cached.empty()) return;

	// When searching, we must round to maximum extent of relevant mapblock indexes.
	// Since we're using floats, always assume +-1
	auto low = [](f32 val) -> s16 {
		s16 _val = static_cast<s16>(val / BS);
		return (_val >> 4) - 1;
	};
	auto high = [](f32 val) -> s16 {
		s16 _val = static_cast<s16>(val / BS);
		return (_val >> 4) + 1;
	};

	v3s16 min(low(box.MinEdge.X), low(box.MinEdge.Y), low(box.MinEdge.Z)),
		max(high(box.MaxEdge.X), high(box.MaxEdge.Y), high(box.MaxEdge.Z));

	// We should only iterate using this spatial map when there are at least 1 objects per mapblocks to check.
	// Otherwise, might as well just iterate.

	v3s16 diff = max - min;
	uint64_t number_of_mapblocks_to_check = std::abs(diff.X) * std::abs(diff.Y) * std::abs(diff.Z);
	if(number_of_mapblocks_to_check <= m_cached.size()) { // might be worth it
		for (s16 x = min.X; x < max.X;x++) {
			for (s16 y = min.Y; y < max.Y;y++) {
				for (s16 z = min.Z; z < max.Z;z++) {
					SpatialKey key(x,y,z, false); // false == pass through the values without shrink
					if (m_cached.find(key) != m_cached.end()) {
						m_iterators_stopping_insertion_and_deletion++;
						auto range = m_cached.equal_range(key);
						for (auto &it = range.first; it != range.second; ++it) {
							callback(it->second);
						}
						m_iterators_stopping_insertion_and_deletion--;
						handleInsertsAndDeletes();
					}
				}
			}
		}
	} else { // let's just iterate, it'll be faster
		m_iterators_stopping_insertion_and_deletion++;
		for (auto it = m_cached.begin(); it != m_cached.end(); ++it) {
			callback(it->second);
		}
		m_iterators_stopping_insertion_and_deletion--;
		handleInsertsAndDeletes();
	}
}

void SpatialMap::handleInsertsAndDeletes()
{
	if (m_iterators_stopping_insertion_and_deletion)
		return;

	if(!m_remove_all) {
		for (auto &op : m_pending_operations) {
			if (op.insert)
				insert(op.id, op.pos);
			else
				remove(op.id, op.pos);
		}
	} else {
		m_cached.clear();
		m_remove_all = false;
	}
	m_pending_operations.clear();
}

} // namespace server
