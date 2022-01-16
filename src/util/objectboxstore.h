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

#pragma once

#ifndef ANDROID
	#include "cmake_config.h"
#endif

#include "irr_v3d.h"
#include "irr_aabb3d.h"
#include "noise.h" // for PcgRandom
#include "util/container.h"
#include "util/numeric.h"
#include "util/serialize.h"

#include <spatialindex/SpatialIndex.h>

#include <cstddef>
#include <map>
#include <vector>

class ObjectBoxStore {
public:
	ObjectBoxStore();

	~ObjectBoxStore();

	void reserve(std::size_t count) {};

	std::size_t size() const { return box_map.size(); }

	/// Add an box to the store.
	/// Updates the box's ID if it hasn't already been set.
	/// @return Whether the box insertion was successful.
	bool insert(u16 id, aabb3f box);

	/// Removes an box from the store by ID.
	/// @return Whether the box was in the store and removed.
	bool remove(u16 id);

	/// Finds boxes that are completely contained inside the box defined
	/// by the passed edges.
	void getInArea(std::vector<u16> *result, aabb3f area) const;

	/// Finds boxes that intersect with the line.
	void getIntersectingLine(std::vector<u16> *result, v3f from, v3f to) const;

protected:

	// Note: This can't be an unordered_map, since all
	// references would be invalidated on rehash.
	using BoxMap = std::map<u16, aabb3f>;
	BoxMap box_map;

private:
	SpatialIndex::ISpatialIndex *m_tree;
	SpatialIndex::IStorageManager *m_storagemanager;

	u16 getNextID() const;

	class VectorResultVisitor : public SpatialIndex::IVisitor {
	public:
		VectorResultVisitor(std::vector<u16> *result, const ObjectBoxStore *store) :
			m_store(store),
			m_result(result)
		{}
		~VectorResultVisitor() {}

		virtual void visitNode(const SpatialIndex::INode &in) {}

		virtual void visitData(const SpatialIndex::IData &in)
		{
			u32 id = in.getIdentifier();
			m_result->push_back((u16)id);
		}

		virtual void visitData(std::vector<const SpatialIndex::IData *> &v)
		{
			for (std::size_t i = 0; i < v.size(); i++)
				visitData(*(v[i]));
		}

	private:
		const ObjectBoxStore *m_store;
		std::vector<u16> *m_result;
	};
};
