/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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
#include "irr_v3d.h"
#include <iostream>
#include <cassert>
#include "exceptions.h"
#include "mapnode.h"
#include <set>
#include <list>
#include "util/basic_macros.h"

class NodeDefManager;

// For VC++
#undef min
#undef max

/*
	A fast voxel manipulator class.

	In normal operation, it fetches more map when it is requested.
	It can also be used so that all allowed area is fetched at the
	start, using ManualMapVoxelManipulator.

	Not thread-safe.
*/

/*
	Debug stuff
*/
extern u64 emerge_time;
extern u64 emerge_load_time;

/*
	This class resembles aabbox3d<s16> a lot, but has inclusive
	edges for saner handling of integer sizes
*/
class VoxelArea
{
public:
	// Starts as zero sized
	VoxelArea() = default;

	VoxelArea(const v3s16 &min_edge, const v3s16 &max_edge):
		MinEdge(min_edge),
		MaxEdge(max_edge)
	{
		cacheExtent();
	}

	VoxelArea(const v3s16 &p):
		MinEdge(p),
		MaxEdge(p)
	{
		cacheExtent();
	}

	/*
		Modifying methods
	*/

	void addArea(const VoxelArea &a) noexcept
	{
		if (hasEmptyExtent())
		{
			*this = a;
			return;
		}
		if(a.MinEdge.X < MinEdge.X) MinEdge.X = a.MinEdge.X;
		if(a.MinEdge.Y < MinEdge.Y) MinEdge.Y = a.MinEdge.Y;
		if(a.MinEdge.Z < MinEdge.Z) MinEdge.Z = a.MinEdge.Z;
		if(a.MaxEdge.X > MaxEdge.X) MaxEdge.X = a.MaxEdge.X;
		if(a.MaxEdge.Y > MaxEdge.Y) MaxEdge.Y = a.MaxEdge.Y;
		if(a.MaxEdge.Z > MaxEdge.Z) MaxEdge.Z = a.MaxEdge.Z;
		cacheExtent();
	}

	void addPoint(const v3s16 &p) noexcept
	{
		if(hasEmptyExtent())
		{
			MinEdge = p;
			MaxEdge = p;
			cacheExtent();
			return;
		}
		if(p.X < MinEdge.X) MinEdge.X = p.X;
		if(p.Y < MinEdge.Y) MinEdge.Y = p.Y;
		if(p.Z < MinEdge.Z) MinEdge.Z = p.Z;
		if(p.X > MaxEdge.X) MaxEdge.X = p.X;
		if(p.Y > MaxEdge.Y) MaxEdge.Y = p.Y;
		if(p.Z > MaxEdge.Z) MaxEdge.Z = p.Z;
		cacheExtent();
	}

	// Pad with d nodes
	void pad(const v3s16 &d) noexcept
	{
		MinEdge -= d;
		MaxEdge += d;
	}

	/*
		const methods
	*/

	const v3s16 &getExtent() const noexcept
	{
		return m_cache_extent;
	}

	/* Because MaxEdge and MinEdge are included in the voxel area an empty extent
	 * is not represented by (0, 0, 0), but instead (-1, -1, -1)
	 */
	bool hasEmptyExtent() const noexcept
	{
		return MaxEdge - MinEdge == v3s16(-1, -1, -1);
	}

	s32 getVolume() const noexcept
	{
		return (s32)m_cache_extent.X * (s32)m_cache_extent.Y * (s32)m_cache_extent.Z;
	}

	bool contains(const VoxelArea &a) const noexcept
	{
		// No area contains an empty area
		// NOTE: Algorithms depend on this, so do not change.
		if(a.hasEmptyExtent())
			return false;

		return(
			a.MinEdge.X >= MinEdge.X && a.MaxEdge.X <= MaxEdge.X &&
			a.MinEdge.Y >= MinEdge.Y && a.MaxEdge.Y <= MaxEdge.Y &&
			a.MinEdge.Z >= MinEdge.Z && a.MaxEdge.Z <= MaxEdge.Z
		);
	}
	bool contains(v3s16 p) const noexcept
	{
		return(
			p.X >= MinEdge.X && p.X <= MaxEdge.X &&
			p.Y >= MinEdge.Y && p.Y <= MaxEdge.Y &&
			p.Z >= MinEdge.Z && p.Z <= MaxEdge.Z
		);
	}
	bool contains(s32 i) const noexcept
	{
		return (i >= 0 && i < getVolume());
	}
	bool operator==(const VoxelArea &other) const noexcept
	{
		return (MinEdge == other.MinEdge
				&& MaxEdge == other.MaxEdge);
	}

	VoxelArea operator+(const v3s16 &off) const noexcept
	{
		return {MinEdge+off, MaxEdge+off};
	}

	VoxelArea operator-(const v3s16 &off) const noexcept
	{
		return {MinEdge-off, MaxEdge-off};
	}

	/*
		Returns 0-6 non-overlapping areas that can be added to
		a to make up this area.

		a: area inside *this
	*/
	void diff(const VoxelArea &a, std::list<VoxelArea> &result)
	{
		/*
			This can result in a maximum of 6 areas
		*/

		// If a is an empty area, return the current area as a whole
		if(a.getExtent() == v3s16(0,0,0))
		{
			VoxelArea b = *this;
			if(b.getVolume() != 0)
				result.push_back(b);
			return;
		}

		assert(contains(a));	// pre-condition

		// Take back area, XY inclusive
		{
			v3s16 min(MinEdge.X, MinEdge.Y, a.MaxEdge.Z+1);
			v3s16 max(MaxEdge.X, MaxEdge.Y, MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take front area, XY inclusive
		{
			v3s16 min(MinEdge.X, MinEdge.Y, MinEdge.Z);
			v3s16 max(MaxEdge.X, MaxEdge.Y, a.MinEdge.Z-1);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take top area, X inclusive
		{
			v3s16 min(MinEdge.X, a.MaxEdge.Y+1, a.MinEdge.Z);
			v3s16 max(MaxEdge.X, MaxEdge.Y, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take bottom area, X inclusive
		{
			v3s16 min(MinEdge.X, MinEdge.Y, a.MinEdge.Z);
			v3s16 max(MaxEdge.X, a.MinEdge.Y-1, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take left area, non-inclusive
		{
			v3s16 min(MinEdge.X, a.MinEdge.Y, a.MinEdge.Z);
			v3s16 max(a.MinEdge.X-1, a.MaxEdge.Y, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

		// Take right area, non-inclusive
		{
			v3s16 min(a.MaxEdge.X+1, a.MinEdge.Y, a.MinEdge.Z);
			v3s16 max(MaxEdge.X, a.MaxEdge.Y, a.MaxEdge.Z);
			VoxelArea b(min, max);
			if(b.getVolume() != 0)
				result.push_back(b);
		}

	}

	/*
		Translates position from virtual coordinates to array index
	*/
	s32 index(s16 x, s16 y, s16 z) const noexcept
	{
		s32 i = (s32)(z - MinEdge.Z) * m_cache_extent.Y * m_cache_extent.X
			+ (y - MinEdge.Y) * m_cache_extent.X
			+ (x - MinEdge.X);
		return i;
	}
	s32 index(v3s16 p) const noexcept
	{
		return index(p.X, p.Y, p.Z);
	}

	/**
	 * Translate index in the X coordinate
	 */
	static void add_x(const v3s16 &extent, u32 &i, s16 a) noexcept
	{
		i += a;
	}

	/**
	 * Translate index in the Y coordinate
	 */
	static void add_y(const v3s16 &extent, u32 &i, s16 a) noexcept
	{
		i += a * extent.X;
	}

	/**
	 * Translate index in the Z coordinate
	 */
	static void add_z(const v3s16 &extent, u32 &i, s16 a) noexcept
	{
		i += a * extent.X * extent.Y;
	}

	/**
	 * Translate index in space
	 */
	static void add_p(const v3s16 &extent, u32 &i, v3s16 a) noexcept
	{
		i += a.Z * extent.X * extent.Y + a.Y * extent.X + a.X;
	}

	/*
		Print method for debugging
	*/
	void print(std::ostream &o) const
	{
		o << PP(MinEdge) << PP(MaxEdge) << "="
			<< m_cache_extent.X << "x" << m_cache_extent.Y << "x" << m_cache_extent.Z
			<< "=" << getVolume();
	}

	// Edges are inclusive
	v3s16 MinEdge = v3s16(1,1,1);
	v3s16 MaxEdge;
private:
	void cacheExtent() noexcept
	{
		m_cache_extent = MaxEdge - MinEdge + v3s16(1,1,1);
	}

	v3s16 m_cache_extent = v3s16(0,0,0);
};

// unused
#define VOXELFLAG_UNUSED   (1 << 0)
// no data about that node
#define VOXELFLAG_NO_DATA  (1 << 1)
// Algorithm-dependent
#define VOXELFLAG_CHECKED1 (1 << 2)
// Algorithm-dependent
#define VOXELFLAG_CHECKED2 (1 << 3)
// Algorithm-dependent
#define VOXELFLAG_CHECKED3 (1 << 4)
// Algorithm-dependent
#define VOXELFLAG_CHECKED4 (1 << 5)

enum VoxelPrintMode
{
	VOXELPRINT_NOTHING,
	VOXELPRINT_MATERIAL,
	VOXELPRINT_WATERPRESSURE,
	VOXELPRINT_LIGHT_DAY,
};

class VoxelManipulator
{
public:
	VoxelManipulator() = default;
	virtual ~VoxelManipulator();

	/*
		These are a bit slow and shouldn't be used internally.
		Use m_data[m_area.index(p)] instead.
	*/
	MapNode getNode(const v3s16 &p)
	{
		VoxelArea voxel_area(p);
		addArea(voxel_area);

		if (m_flags[m_area.index(p)] & VOXELFLAG_NO_DATA) {
			/*dstream<<"EXCEPT: VoxelManipulator::getNode(): "
					<<"p=("<<p.X<<","<<p.Y<<","<<p.Z<<")"
					<<", index="<<m_area.index(p)
					<<", flags="<<(int)m_flags[m_area.index(p)]
					<<" is inexistent"<<std::endl;*/
			throw InvalidPositionException
			("VoxelManipulator: getNode: inexistent");
		}

		return m_data[m_area.index(p)];
	}
	MapNode getNodeNoEx(const v3s16 &p)
	{
		VoxelArea voxel_area(p);
		addArea(voxel_area);

		if (m_flags[m_area.index(p)] & VOXELFLAG_NO_DATA) {
			return {CONTENT_IGNORE};
		}

		return m_data[m_area.index(p)];
	}
	MapNode getNodeNoExNoEmerge(const v3s16 &p) const noexcept
	{
		if (!m_area.contains(p))
			return {CONTENT_IGNORE};
		if (m_flags[m_area.index(p)] & VOXELFLAG_NO_DATA)
			return {CONTENT_IGNORE};
		return m_data[m_area.index(p)];
	}
	// Stuff explodes if non-emerged area is touched with this.
	// Emerge first, and check VOXELFLAG_NO_DATA if appropriate.
	MapNode & getNodeRefUnsafe(const v3s16 &p) noexcept
	{
		return m_data[m_area.index(p)];
	}

	const MapNode & getNodeRefUnsafeCheckFlags(const v3s16 &p) const noexcept
	{
		s32 index = m_area.index(p);

		if (m_flags[index] & VOXELFLAG_NO_DATA)
			return ContentIgnoreNode;

		return m_data[index];
	}

	u8 & getFlagsRefUnsafe(const v3s16 &p) noexcept
	{
		return m_flags[m_area.index(p)];
	}

	bool exists(const v3s16 &p) const noexcept
	{
		return m_area.contains(p) &&
			!(m_flags[m_area.index(p)] & VOXELFLAG_NO_DATA);
	}

	void setNode(const v3s16 &p, const MapNode &n)
	{
		VoxelArea voxel_area(p);
		addArea(voxel_area);

		m_data[m_area.index(p)] = n;
		m_flags[m_area.index(p)] &= ~VOXELFLAG_NO_DATA;
	}
	// TODO: Should be removed and replaced with setNode
	void setNodeNoRef(const v3s16 &p, const MapNode &n)
	{
		setNode(p, n);
	}

	/*
		Set stuff if available without an emerge.
		Return false if failed.
		This is convenient but slower than playing around directly
		with the m_data table with indices.
	*/
	bool setNodeNoEmerge(const v3s16 &p, MapNode n) noexcept
	{
		if(!m_area.contains(p))
			return false;
		m_data[m_area.index(p)] = n;
		return true;
	}

	/*
		Control
	*/

	virtual void clear();

	void print(std::ostream &o, const NodeDefManager *nodemgr,
			VoxelPrintMode mode=VOXELPRINT_MATERIAL);

	void addArea(const VoxelArea &area);

	/*
		Copy data and set flags to 0
		dst_area.getExtent() <= src_area.getExtent()
	*/
	void copyFrom(MapNode *src, const VoxelArea& src_area,
			v3s16 from_pos, v3s16 to_pos, const v3s16 &size);

	// Copy data
	void copyTo(MapNode *dst, const VoxelArea& dst_area,
			v3s16 dst_pos, v3s16 from_pos, const v3s16 &size);

	/*
		Algorithms
	*/

	void clearFlag(u8 flag);

	/*
		Member variables
	*/

	/*
		The area that is stored in m_data.
		addInternalBox should not be used if getExtent() == v3s16(0,0,0)
		MaxEdge is 1 higher than maximum allowed position
	*/
	VoxelArea m_area;

	/*
		Lock to prevent addArea() when m_data is being directly accessed
		This is to prevent invalid memory access
	*/
	bool m_area_locked = false;

	/*
		nullptr if data size is 0 (extent (0,0,0))
		Data is stored as [z*h*w + y*h + x]
	*/
	MapNode *m_data = nullptr;

	/*
		Flags of all nodes
	*/
	u8 *m_flags = nullptr;

	static const MapNode ContentIgnoreNode;
};
