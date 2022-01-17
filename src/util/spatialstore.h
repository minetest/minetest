/*
Minetest
Copyright (C) 2022 est31 <mtest31@outlook.com>

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

//#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include "util/areastore.h"
//#include "noise.h" // for PcgRandom
//#include "util/container.h"
//#include "util/numeric.h"
//#include "util/serialize.h"

#include <spatialindex/SpatialIndex.h>
//#include <spatialindex/Point.h>
//#include <spatialindex/RTree.h>

#include <memory>
//#include <cstddef>
#include <map>
#include <vector>
//#include <utility>

#define AST_SMALLER_EQ_AS(p, q) (((p).X <= (q).X) && ((p).Y <= (q).Y) && ((p).Z <= (q).Z))

#define AST_OVERLAPS_IN_DIMENSION(amine, amaxe, b, d) \
	(!(((amine).d > (b)->maxedge.d) || ((amaxe).d < (b)->minedge.d)))

#define AST_CONTAINS_PT(a, p) (AST_SMALLER_EQ_AS((a)->minedge, (p)) && \
	AST_SMALLER_EQ_AS((p), (a)->maxedge))

#define AST_CONTAINS_Box(amine, amaxe, b)         \
	(AST_SMALLER_EQ_AS((amine), (b)->minedge) \
	&& AST_SMALLER_EQ_AS((b)->maxedge, (amaxe)))

#define AST_BoxS_OVERLAP(amine, amaxe, b)                \
	(AST_OVERLAPS_IN_DIMENSION((amine), (amaxe), (b), X) && \
	AST_OVERLAPS_IN_DIMENSION((amine), (amaxe), (b), Y) &&  \
	AST_OVERLAPS_IN_DIMENSION((amine), (amaxe), (b), Z))

namespace {

	template <class T>
	void get_doubles_from_point(const T &from, double (& to)[3]) {
		to[0] = from.X;
		to[1] = from.Y;
		to[2] = from.Z;
	}

	SpatialIndex::Region get_spatial_region(const aabb3f &space)
	{
		double coordsMin[3];
		double coordsMax[3];
		get_doubles_from_point(space.MinEdge, coordsMin);
		get_doubles_from_point(space.MaxEdge, coordsMax);
		return SpatialIndex::Region(coordsMin, coordsMax, 3);
	}
	SpatialIndex::Region get_spatial_region(const Area &space)
	{
		double coordsMin[3];
		double coordsMax[3];
		get_doubles_from_point(space.minedge, coordsMin);
		get_doubles_from_point(space.maxedge, coordsMax);
		return SpatialIndex::Region(coordsMin, coordsMax, 3);
	}

	SpatialIndex::LineSegment get_spatial_line_segment(const v3f &from, const v3f &to)
	{
		double coordsMin[3];
		double coordsMax[3];
		get_doubles_from_point(from, coordsMin);
		get_doubles_from_point(to, coordsMax);
		return SpatialIndex::LineSegment(coordsMin, coordsMax, 3);
	}
}

template <class Space, class ID = u32> class SpatialStore : public SpatialIndex::IVisitor {
public:
	SpatialStore()
			:
		m_storageManager(SpatialIndex::StorageManager::createNewMemoryStorageManager()),
		m_tree(),
		m_spacesMap(),
		m_result(nullptr)
	{
		SpatialIndex::id_type unused_id;
		m_tree = std::unique_ptr<SpatialIndex::ISpatialIndex>(
			SpatialIndex::RTree::createNewRTree(
				*m_storageManager,
				.7, // Fill factor
				100, // Index capacity
				100, // Leaf capacity
				3, // dimension
				SpatialIndex::RTree::RV_RSTAR,
				unused_id
			)
		);
	}

	virtual ~SpatialStore() {}

	std::size_t size() const {
		return m_spacesMap.size();
	}

	bool insert(ID id, const Space &space) {
		if (! m_spacesMap.insert({id, space}).second)
			return false; // id already in map
		m_tree->insertData(
			0,
			nullptr,
			get_spatial_region(space),
			(u32) id
		);
		return true;
	}

	bool remove(ID id) {
		const auto iter = m_spacesMap.find(id);
		if (iter == m_spacesMap.end())
			return false;
		Space space = iter->second;
		m_spacesMap.erase(iter);
		return m_tree->deleteData(
			get_spatial_region(space),
			(u32) id
		);
	}

	virtual void visitNode(const SpatialIndex::INode &in) {}

	virtual void visitData(const SpatialIndex::IData &in)
	{
		m_result->push_back(in.getIdentifier());
	}

	virtual void visitData(std::vector<const SpatialIndex::IData *> &v)
	{
		for (const auto &item : v)
			m_result->push_back((ID) item->getIdentifier());
	}

	void getInArea(std::vector<ID> *result, Space space)
	{
		m_result = result;
		m_tree->intersectsWithQuery(get_spatial_region(space), *this);
		m_result = nullptr;
	}

	void getIntersectingLine(std::vector<ID> *result, v3f from, v3f to)
	{
		m_result = result;
		m_tree->intersectsWithQuery(get_spatial_line_segment(from, to), *this);
		m_result = nullptr;
	}

private:
	std::unique_ptr<SpatialIndex::IStorageManager> m_storageManager;
	std::unique_ptr<SpatialIndex::ISpatialIndex> m_tree;
	std::map<ID, Space> m_spacesMap;
	std::vector<ID> *m_result; // null except during visitation

	ID getNextId() const {
		ID free_id = 0;
		for (const auto &item : m_spacesMap) {
			if (item.first > free_id)
				return free_id; // Found gap

			free_id = item.first + 1;
		}
		return free_id;
	}
};
