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
	void insert(u16 id, const v3f &pos);
	void remove(u16 id, const v3f &pos);
	void remove(u16 id);
	void removeAll();
	void updatePosition(u16 id, const v3f &oldPos, const v3f &newPos);
	void getRelevantObjectIds(const aabb3f &box, const std::function<void(u16 id)> &callback);

protected:
	void handleInsertsAndDeletes();

	struct SpatialKey {
		u16 padding{0};
		s16 x, y, z;

		// In Node Coordinates, shrink = false: for getRelevantObjectIds() iteration
		SpatialKey(s16 _x, s16 _y, s16 _z, bool shrink = true) {
			if (shrink) {
				x = _x >> 4;
				y = _y >> 4;
				z = _z >> 4;
			}
			else {
				x = _x;
				y = _y;
				z = _z;
			}
		}
		// In Entity coordinates
		SpatialKey(const v3f &_pos) : SpatialKey(_pos.X / BS, _pos.Y / BS, _pos.Z / BS){}

		bool operator==(const SpatialKey &other) const {
			return (x == other.x && y == other.y && z == other.z);
		}
	};

	static_assert(sizeof(SpatialKey) == sizeof(u64), "SpatialKey must align with 8 bytes, i.e. uint64_t");

	struct SpatialKeyHash {
		auto operator()(const SpatialKey& key) const -> u64 {
			return std::hash<u64>()(*reinterpret_cast<const u64*>(&key));
		}
	};

	// Used for storing Inserts and Deletes while iterating
	struct PendingOperation {
		v3f pos;
		u16 id;
		bool insert;

		// Truncate, but keep in entity coords, that conversion is handled in SpatialKey
		PendingOperation(const v3f& _pos, u16 _id, bool is_being_inserted) {
			pos = _pos;
			id = _id;
			insert = is_being_inserted; // false = removed
		}
	};

	std::unordered_multimap<SpatialKey, u16, SpatialKeyHash> m_cached;
	std::vector<PendingOperation> m_pending_operations;
	bool m_remove_all{false};
	u64 m_iterators_stopping_insertion_and_deletion{0};
};
} // namespace server
