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

#pragma once

#include <functional>
#include <vector>
#include <unordered_set>
#include "irrlichttypes_bloated.h"
#include "constants.h"

namespace server
{
class SpatialMap
{
public:
	void insert(u16 id, const v3f &pos, bool postIteration = false);
	void remove(u16 id, const v3f &pos, bool postIteration = false);
	void remove(u16 id);
	void removeAll();
	void updatePosition(u16 id, const v3f &oldPos, const v3f &newPos);
	void getRelevantObjectIds(const aabb3f &box, const std::function<void(u16 id)> &callback);
	void handleInsertsAndDeletes();

protected:
	struct SpatialKey {
		u16 padding_or_optional_id{0};
		s16 x;
		s16 y;
		s16 z;

		SpatialKey(s16 _x, s16 _y, s16 _z, bool _shrink = true) {
			if(_shrink) {
				x = _x >> 4;
				y = _y >> 4;
				z = _z >> 4;
			} else {
				x = _x;
				y = _y;
				z = _z;
			}
		}
		SpatialKey(const v3f &_pos) : SpatialKey(_pos.X / BS, _pos.Y / BS, _pos.Z / BS){}
		// The following use case is for storing pending insertions and deletions while iterating
		// using the extra 16 bit padding makes keeping track of them super efficient for hashing.
		SpatialKey(const v3f &_pos, const u16 id) : SpatialKey(_pos.X / BS, _pos.Y / BS, _pos.Z / BS, false){
			padding_or_optional_id = id;
		}

		bool operator==(const SpatialKey &other) const {
			return (x == other.x && y == other.y && z == other.z);
		}
	};

	struct SpatialKeyHash {
		auto operator()(const SpatialKey &key) const -> size_t {
			return std::hash<size_t>()(*reinterpret_cast<const size_t*>(&key));
		}
	};

	std::unordered_multimap<SpatialKey, u16, SpatialKeyHash> m_cached;
	std::unordered_set<SpatialKey, SpatialKeyHash> m_pending_inserts;
	std::unordered_set<SpatialKey, SpatialKeyHash> m_pending_deletes;
	bool m_remove_all{false};
	u64 m_iterators_stopping_insertion_and_deletion{0};
};
} // namespace server
