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

#include "util/numeric.h"
#include "util/spatial_tools.h"

#include <spatialindex/SpatialIndex.h>

#include <cstddef>
#include <memory>
#include <map>
#include <vector>

template <typename T, typename U = u32>
class SpatialStore : public SpatialIndex::IVisitor {
public:

	SpatialStore()
		: m_tree { createTree() }
		, m_spacesMap {}
		, m_result {}
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
		m_tree->insertData(
			0,
			nullptr,
			sp_convert::get_spatial_region(space),
			static_cast<u32>(id)
		);

		return true;
	}

	bool remove(U id) {
		const auto iter = m_spacesMap.find(id);
		if (iter == m_spacesMap.end())
			return false;
		T space = iter->second;
		m_spacesMap.erase(iter);
		return m_tree->deleteData(
			sp_convert::get_spatial_region(space),
			(u32) id
		);
	}

	void clear() {
		using SpatialIndex::ISpatialIndex;
		m_spacesMap.clear();
		m_tree = std::unique_ptr<ISpatialIndex> { createTree() };
	}

	virtual void visitNode(const SpatialIndex::INode &in) {}

	virtual void visitData(const SpatialIndex::IData &in)
	{
		m_result->push_back(in.getIdentifier());
	}

	virtual void visitData(std::vector<const SpatialIndex::IData *> &v)
	{
		for (const auto &item : v)
			m_result->push_back((U) item->getIdentifier());
	}

	void getInArea(std::vector<U> *result, T space)
	{
		m_result = result;
		m_tree->intersectsWithQuery(sp_convert::get_spatial_region(space), *this);
		m_result = nullptr;
	}

	void getIntersectingLine(std::vector<U> *result, v3f from, v3f to)
	{
		m_result = result;
		m_tree->intersectsWithQuery(sp_convert::get_spatial_line_segment(from, to), *this);
		m_result = nullptr;
	}

private:
	std::unique_ptr<SpatialIndex::ISpatialIndex> m_tree;
	std::map<U, T> m_spacesMap;
	std::vector<U> *m_result; // null except during visitation

	static SpatialIndex::ISpatialIndex* createTree() {
		using SpatialIndex::StorageManager
			::createNewMemoryStorageManager;

		SpatialIndex::id_type unused_id {};
		return SpatialIndex::RTree::createNewRTree(
			*createNewMemoryStorageManager(),
			0.7,
			100,
			100,
			3,
			SpatialIndex::RTree::RV_RSTAR,
			unused_id
		);
	}

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
