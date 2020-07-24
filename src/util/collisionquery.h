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

#include <unordered_map>
#include <vector>

#include "util/invertedindex.h"

// Detail for each collision box in the context.
struct CollisionContextDetail
{
	u16 valid_faces = 0;
	f32 face_offset[6] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

	CollisionContextDetail(u16 face_mask, CollisionFace face, f32 offset) : valid_faces(face_mask)
	{
		face_offset[face] = offset;
	}

	CollisionContextDetail() {}

	CollisionContextDetail(const CollisionContextDetail &copy) :
			valid_faces(copy.valid_faces),
			face_offset{
					copy.face_offset[0], copy.face_offset[1], copy.face_offset[2],
					copy.face_offset[3], copy.face_offset[4], copy.face_offset[5]
				} {}

	CollisionContextDetail(CollisionContextDetail &&move) :
			valid_faces(move.valid_faces),
			face_offset{
					move.face_offset[0], move.face_offset[1], move.face_offset[2],
					move.face_offset[3], move.face_offset[4], move.face_offset[5]
				} {}
};

class CollisionContext
{
public:
	CollisionContext(aabb3f box, InvertedIndex *index);

	u32 addIndexList(IndexListIterator *index);
	u32 subtractIndexList(IndexListIterator *index);

	// Get the bitmask of valid faces. For each valid face, put the
	// distance between that face of the entity and the opposing face
	// of the collision box.
	u16 getValidFaces(u32 id, f32 offset[6]) const
	{
		std::unordered_map<u32, CollisionContextDetail>::const_iterator  entry = m_active.find(id);
		if (entry == m_active.end())
			return 0;

		u16 valid = entry->second.valid_faces;
		for (u32 f = 0; f < 6; f++)
			if ((valid & setBitmask[f]) == setBitmask[f])
				offset[f] = m_face_offset[f] - entry->second.face_offset[f];
		return valid;
	}

	static const u16 setBitmask[];
	static const u16 unsetBitmask[];
	static const CollisionFace opposingFace[];

protected:
	f32 m_face_offset[6] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};
	std::unordered_map<u32, CollisionContextDetail> m_active;
	std::vector<u32> collisions;
};

/* TODO *
struct IndexScan
{
	CollisionFace face;
	f32 time;
	// Physics can be incorporated with a function instead of scale.
	const f32 begin;
	const f32 timeScale;
	u32 handle;
	const i32 increment;
	const u32 end;
	AttributeIndex *next;

	IndexScan(CollisionFace face, f32 time, f32 begin, f32 timeScale, u32 handle, i32 increment, u32 end) :
			face(face), time(time), begin(begin), timeScale(timeScale),
			handle(handle), increment(increment), end(end) {}
	IndexScan(const IndexScan &other) = default;
};

class QueryScan
{
	QueryScan(InvertedIndex *index) : m_index(index) {}

	// Scan the offsets in interval [begin, end)
	bool addScan(CollisionFace face, f32 begin, f32 velocity, f32 dtime);

	// Perform the scan.
	// You MUST call these function is order.
	// nextFace() will tell you what face, if any, the next AttributeIndex
	// involves. COLLISION_FACE_NONE will indicate the scan is done.
	CollisionFace nextFace()
	{
		if (m_scans.empty())
			return COLLISION_FACE_NONE;
		if (!heap)
			std::make_heap(m_scans->begin(), m_scans->end(), compare);
		return m_index->front().face;
	}

	// If nextFace() did not return COLLISION_FACE_NONE, optionally call
	// nextTime() to get the specific 
	f32 nextTime() const
	{
		return m_index->front().time;
	}

	// If nextFace() did not return COLLISION_FACE_NONE, call next() to
	// get the AttributeIndex. nextTime() should not be called after this
	// until nextFace() is called.
	AttributeIndex *next();

protected:
	static bool compare(IndexScan a, IndexScan b) { return a.time < b.time; }
	InvertedIndex *m_index;
	std::vector<IndexScan> m_scans;
	bool m_heap;
}
* TODO */
