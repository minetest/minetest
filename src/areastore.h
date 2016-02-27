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

#ifndef AREASTORE_H_
#define AREASTORE_H_

#include "irr_v3d.h"
#include "noise.h" // for PcgRandom
#include <map>
#include <list>
#include <vector>
#include <istream>
#include "util/container.h"
#include "util/numeric.h"
#ifndef ANDROID
	#include "cmake_config.h"
#endif
#if USE_SPATIAL
	#include <spatialindex/SpatialIndex.h>
	#include "util/serialize.h"
#endif

#define AST_EXTREMIFY(min, max, pa, pb) \
	(min).X = MYMIN((pa).X, (pb).X);    \
	(min).Y = MYMIN((pa).Y, (pb).Y);    \
	(min).Z = MYMIN((pa).Z, (pb).Z);    \
	(max).X = MYMAX((pa).X, (pb).X);    \
	(max).Y = MYMAX((pa).Y, (pb).Y);    \
	(max).Z = MYMAX((pa).Z, (pb).Z);

#define AREA_ID_INVALID 0

struct Area {
	Area(const v3s16 &minedge, const v3s16 &maxedge)
	{
		this->minedge = minedge;
		this->maxedge = maxedge;
	}

	Area() {}

	void extremifyEdges()
	{
		v3s16 nminedge;
		v3s16 nmaxedge;

		AST_EXTREMIFY(nminedge, nmaxedge, minedge, maxedge)

		maxedge = nmaxedge;
		minedge = nminedge;
	}

	u32 id;
	v3s16 minedge;
	v3s16 maxedge;
	std::string data;
};

std::vector<std::string> get_areastore_typenames();

class AreaStore {
protected:
	// TODO change to unordered_map when we can
	std::map<u32, Area> areas_map;
	void invalidateCache();
	virtual void getAreasForPosImpl(std::vector<Area *> *result, v3s16 pos) = 0;
	bool cache_enabled; // don't write to this from subclasses, only read.
public:
	virtual void insertArea(const Area &a) = 0;
	virtual void reserve(size_t count) {};
	virtual bool removeArea(u32 id) = 0;
	void getAreasForPos(std::vector<Area *> *result, v3s16 pos);
	virtual void getAreasInArea(std::vector<Area *> *result,
		v3s16 minedge, v3s16 maxedge, bool accept_overlap) = 0;

#if 0
	// calls a passed function for every stored area, until the
	// callback returns true. If that happens, it returns true,
	// if the search is exhausted, it returns false
	virtual bool forEach(bool (*callback)(void *args, Area *a), void *args) const = 0;
#endif

	virtual ~AreaStore()
	{}

	AreaStore() :
		cache_enabled(true),
		m_cacheblock_radius(64),
		m_res_cache(1000, &cacheMiss, this),
		m_highest_id(0)
	{
	}

	void setCacheParams(bool enabled, u8 block_radius, size_t limit);

	u32 getFreeId(v3s16 minedge, v3s16 maxedge);
	const Area *getArea(u32 id) const;
	u16 size() const;
#if 0
	bool deserialize(std::istream &is);
	void serialize(std::ostream &is) const;
#endif
private:
	static void cacheMiss(void *data, const v3s16 &mpos, std::vector<Area *> *dest);
	u8 m_cacheblock_radius; // if you modify this, call invalidateCache()
	LRUCache<v3s16, std::vector<Area *> > m_res_cache;
	u32 m_highest_id;

};


class VectorAreaStore : public AreaStore {
protected:
	virtual void getAreasForPosImpl(std::vector<Area *> *result, v3s16 pos);
public:
	virtual void insertArea(const Area &a);
	virtual void reserve(size_t count);
	virtual bool removeArea(u32 id);
	virtual void getAreasInArea(std::vector<Area *> *result,
		v3s16 minedge, v3s16 maxedge, bool accept_overlap);
	// virtual bool forEach(bool (*callback)(void *args, Area *a), void *args) const;
private:
	std::vector<Area *> m_areas;
};

#if USE_SPATIAL

class SpatialAreaStore : public AreaStore {
protected:
	virtual void getAreasForPosImpl(std::vector<Area *> *result, v3s16 pos);
public:
	SpatialAreaStore();
	virtual void insertArea(const Area &a);
	virtual bool removeArea(u32 id);
	virtual void getAreasInArea(std::vector<Area *> *result,
		v3s16 minedge, v3s16 maxedge, bool accept_overlap);
	// virtual bool forEach(bool (*callback)(void *args, Area *a), void *args) const;

	virtual ~SpatialAreaStore();
private:
	SpatialIndex::ISpatialIndex *m_tree;
	SpatialIndex::IStorageManager *m_storagemanager;

	class VectorResultVisitor : public SpatialIndex::IVisitor {
	private:
		SpatialAreaStore *m_store;
		std::vector<Area *> *m_result;
	public:
		VectorResultVisitor(std::vector<Area *> *result, SpatialAreaStore *store)
		{
			m_store = store;
			m_result = result;
		}

		virtual void visitNode(const SpatialIndex::INode &in)
		{
		}

		virtual void visitData(const SpatialIndex::IData &in)
		{
			u32 id = in.getIdentifier();

			std::map<u32, Area>::iterator itr = m_store->areas_map.find(id);
			assert(itr != m_store->areas_map.end());
			m_result->push_back(&itr->second);
		}

		virtual void visitData(std::vector<const SpatialIndex::IData *> &v)
		{
			for (size_t i = 0; i < v.size(); i++)
				visitData(*(v[i]));
		}

		~VectorResultVisitor() {}
	};
};

#endif

#endif /* AREASTORE_H_ */
