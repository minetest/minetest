/*
Minetest
Copyright (C) 2022 JosiahWI <josiah_vanderzee@medacombb.net>

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

#include "irrlichttypes.h"
#include "util/numeric.h"
#include "util/spatial_tools.h"

#include <../lib/THST/RTree.h>

#include <cstddef>
#include <iterator>
#include <map>
#include <vector>

struct Object {
	u32 id;
	spatial::BoundingBox<double, 3> bbox;
};

static bool operator==(const Object &a, const Object &b)
{
	return a.id == b.id;
}

// helps to get the bounding of the items inserted
struct Indexable {
	const double *min(const Object &value) const { return value.bbox.min; }
	const double *max(const Object &value) const { return value.bbox.max; }
};


template <typename T, typename U = u32>
class SpatialStore
{
public:

	SpatialStore()
		: m_tree {}
		, m_spacesMap {}
	{}

	SpatialStore(const SpatialStore&) = delete;
	SpatialStore<T, U>& operator =(const SpatialStore&) = delete;

	virtual ~SpatialStore() {}

	std::size_t size() const {
		return m_spacesMap.size();
	}

	bool insert(U id, const T &space) {
		if (contains(id))
			return false;

		m_spacesMap.insert({ id, space });
		Object obj { static_cast<u32>(id), sp_convert::get_spatial_region(space) };
		m_tree.insert(obj);

		return true;
	}

	bool remove(U id) {
		const auto iter { m_spacesMap.find(id) };
		if (iter == m_spacesMap.end())
			return false;

		T space { iter->second };
		m_spacesMap.erase(iter);
		Object obj { static_cast<u32>(id), sp_convert::get_spatial_region(space) };
		return m_tree.remove(obj);
	}

	void clear() {
		m_spacesMap.clear();
		m_tree.clear();
	}

	std::vector<U> getInArea(T space)
	{
		std::vector<Object> results {};
		std::vector<U> object_ids {};
		m_tree.query(
			spatial::intersects<3>(sp_convert::get_spatial_region(space)),
			std::back_inserter(results)
		);
		object_ids.reserve(results.size());
		for (const Object &obj : results) {
			object_ids.push_back(obj.id);
		}

		return object_ids;
	}

	void getInArea(std::vector<U> *result, T space)
	{
		*result = getInArea(space);
	}

	void getIntersectingLine(std::vector<U> *result, v3f from, v3f to)
	{
		
		/*
		m_result = result;
		m_tree->intersectsWithQuery(sp_convert::get_spatial_line_segment(from, to), *this);
		m_result = nullptr;
		*/
	}

private:
	// tree must be destructed first because it accesses the storage manager
	spatial::RTree<double, Object, 3, 4, 3, Indexable> m_tree;
	std::map<U, T> m_spacesMap;

	bool contains(U id) const {
		return (m_spacesMap.find(id) != m_spacesMap.end());
	}

	U getNextId() const {
		U free_id = 0;
		for (const auto &item : m_spacesMap) {
			if (item.first > free_id)
				return free_id; // Found gap

			free_id = item.first + 1;
		}
		return free_id;
	}
};

using ObjectBoxStore = SpatialStore<aabb3f, u16>;
