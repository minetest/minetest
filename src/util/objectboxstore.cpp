/*
Minetest
Copyright (C) 2015 est31 <mtest31@outlook.com>

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

#include "irr_aabb3d.h"
#include "irr_v3d.h"
#include "util/container.h"
#include "util/objectboxstore.h"

#include <spatialindex/Point.h>
#include <spatialindex/SpatialIndex.h>
#include <spatialindex/RTree.h>

#include <map>
#include <utility>
#include <vector>

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

static inline SpatialIndex::Region get_spatial_region(const v3f minedge,
		const v3f maxedge)
{
	const double p_low[] = {(double)minedge.X,
		(double)minedge.Y, (double)minedge.Z};
	const double p_high[] = {(double)maxedge.X, (double)maxedge.Y,
		(double)maxedge.Z};
	return SpatialIndex::Region(p_low, p_high, 3);
}

static SpatialIndex::LineSegment get_spatial_line_segment(const v3f from, const v3f to)
{
	const double p_low[] = {(double)from.X,
		(double)from.Y, (double)from.Z};
	const double p_high[] = {(double)to.X, (double)to.Y,
		(double)to.Z};
	return SpatialIndex::LineSegment(p_low, p_high, 3);
}

/*const aabb3f *ObjectBoxStore::getBox(u16 id) const
{
	BoxMap::const_iterator it = box_map.find(id);
	if (it == box_map.end())
		return nullptr;
	return &it->second;
}*/

bool ObjectBoxStore::insert(u16 id, aabb3f box)
{
	if (!box_map.insert(std::make_pair(id, box)).second)
		// ID is not unique
		return false;
	m_tree->insertData(0, nullptr, get_spatial_region(box.MinEdge, box.MaxEdge), (u32)id);
	return true;
}

bool ObjectBoxStore::remove(u16 id)
{
	std::map<u16, aabb3f>::iterator itr = box_map.find(id);
	if (itr != box_map.end()) {
		aabb3f box = itr->second;
		bool result = m_tree->deleteData(get_spatial_region(box.MinEdge, box.MaxEdge), id);
		box_map.erase(itr);
		return result;
	} else {
		return false;
	}
}

void ObjectBoxStore::getInArea(std::vector<u16> *result, aabb3f area) const
{
	VectorResultVisitor visitor(result, this);
	m_tree->intersectsWithQuery(get_spatial_region(area.MinEdge, area.MaxEdge), visitor);
}

void ObjectBoxStore::getIntersectingLine(std::vector<u16> *result, v3f from, v3f to) const
{
	VectorResultVisitor visitor(result, this);
	m_tree->intersectsWithQuery(get_spatial_line_segment(from, to), visitor);
}

ObjectBoxStore::~ObjectBoxStore()
{
	delete m_tree;
}

ObjectBoxStore::ObjectBoxStore()
{
	m_storagemanager =
		SpatialIndex::StorageManager::createNewMemoryStorageManager(); // TODO must this be deleted?
	SpatialIndex::id_type id;
	m_tree = SpatialIndex::RTree::createNewRTree(
		*m_storagemanager,
		.7, // Fill factor
		100, // Index capacity
		100, // Leaf capacity
		3, // dimension :)
		SpatialIndex::RTree::RV_RSTAR,
		id);
}
