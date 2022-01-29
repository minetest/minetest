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

#include "irr_aabb3d.h"
#include "util/numeric.h"
#include "util/spatial_tools.h"

#include <RTree.h>

#include <cstddef>
#include <iterator>
#include <map>
#include <vector>

template <typename T, typename U = u32>
class SpatialStore
{
private:
	spatial::RTree<float, sp_util::TaggedBBox<U>, 3, 4, 3,
			sp_util::TaggedBBoxIndexable<U>>
			m_tree;
	std::map<U, T> m_spacesMap;

	bool contains(U id) const { return (m_spacesMap.find(id) != m_spacesMap.end()); }

	U getNextId() const
	{
		U free_id = 0;
		for (const auto &item : m_spacesMap) {
			if (item.first > free_id)
				return free_id; // Found gap

			free_id = item.first + 1;
		}
		return free_id;
	}

public:
	SpatialStore() : m_tree{}, m_spacesMap{} {}

	SpatialStore(const SpatialStore &) = delete;
	SpatialStore<T, U> &operator=(const SpatialStore &) = delete;

	virtual ~SpatialStore() {}

	std::size_t size() const { return m_spacesMap.size(); }

	bool insert(U id, const T &space)
	{
		if (contains(id))
			return false;

		m_spacesMap.insert({id, space});
		sp_util::TaggedBBox<U> obj{sp_util::get_spatial_region(space), id};
		m_tree.insert(obj);

		return true;
	}

	bool remove(U id)
	{
		const auto iter{m_spacesMap.find(id)};
		if (iter == m_spacesMap.end())
			return false;

		T space{iter->second};
		m_spacesMap.erase(iter);
		sp_util::TaggedBBox<U> obj{sp_util::get_spatial_region(space), id};
		return m_tree.remove(obj);
	}

	void clear()
	{
		m_spacesMap.clear();
		m_tree.clear();
	}

	std::vector<U> getInArea(T space)
	{
		std::vector<sp_util::TaggedBBox<U>> results{};
		std::vector<U> object_ids{};
		m_tree.query(spatial::intersects<3>(sp_util::get_spatial_region(space)),
				std::back_inserter(results));
		object_ids.reserve(results.size());
		for (const sp_util::TaggedBBox<U> &obj : results) {
			object_ids.push_back(obj.idTag);
		}

		return object_ids;
	}

	void getInArea(std::vector<U> *result, T space) { *result = getInArea(space); }

	void getIntersectingLine(std::vector<U> *result, v3f from, v3f to)
	{
		throw "Not Implemented";
	}
};

using ObjectBoxStore = SpatialStore<aabb3f, u16>;
