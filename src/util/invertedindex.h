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

#include "irrlichttypes_bloated.h"
#include "log.h"

/**
 * Inverted indices are normally used for large scale information retrieval like
 * Web search engines to enable low-latency queries of massive information
 * sets while minimizing index storage costs. This implementation is based
 * on STL vectors rather than compressed skiplists, so the computational 
 * burden is lower at the cost of using more memory.
 *
 * The purpose of this is to enable efficient search for collisions during
 * entity motion. This version supports only first-order physics (velocity)
 * but a future version could probably support more advance physics without
 * a marked performance hit.
 *
 * In a regular data structure, data is represented as objects that have
 * attributes and relationships. In an inverted index, data is represented as
 * attributes and relationships that have sets of objects. As a result, the
 * space requirement is about the same either way, but it becomes
 * possible to use set operations to find objects that have combinations
 * of attributes. This implementation use this feature to find boxes
 * that an entity would collide in order of collision as an entity moves
 * along a trajectory. It also enables efficient queries to see what is
 * near an entity, e.g. is the entity standing on ground.
 *
 * Collision boxes are represented using u32, starting with 0 in the order
 * they are indexed. The collision code thresholds velocities to 5000. in
 * each dimension, giving a 0.5 second search space of at most about
 * 256 x 256 x 256 = 2^24 nodes. Assuming an average of one collision boxes
 * per node, u16 would allow indexing a cubic region 40 nodes on a side and
 * u32 allows up to 1625 on a side.
 *
 * The attributes of a collision box are the offsets of its six faces.
 * (In a future physics-aware implementation, it may be expeditious to index
 * physical properties like "bouncy" or "static friction.") So, the Id of each
 * collision box appears in 6 indexes, one for each face. Notice that the
 * space requirement is 6 u32 for each box, which is the same space as storing
 * each box as 6 f32.
 *
 * To enable searching by the offset of a specific face, we keep one sorted
 * list of offsets for each face. This allows efficiently scanning the offsets
 * along a trajectory. For example, suppose that an entity moves in the
 * positive x direction. One index contains the MinEdge.X values that are 
 * indexed. As the MaxEdge.X of the entity moves along the trajectory, 
 * this index gives all of the offsets where a collision of the entity's
 * max X face is possible, and the set of ids at each offset gives the
 * entities that might collide. Meanwhile, when the minimum edge of the 
 * entity passes a value of MaxEdge.X, there is a set of Ids that can no
 * longer collide.
 *
 * The specific search criteria is represented as a Query. Different queries
 * are useful for different situations. For example, a query can be created
 * that checks to see if the entity is currently near a collision box. Another
 * query might ask if an entity can step up over an obstacle. The specific
 * query used would depend on which direction(s) have collided obstacles.
 *
 * A query operates on a specific context, which contains the current box
 * of an entity, and three sets of entities that could collide with it, one
 * for each dimension. The context also includes the relative rates of motion
 * in each dimension so that offsets can be scaled to time. The context is
 * essentially the working memory used during the query, so it is structured
 * as a HashMap so that information from inverted indexes can be efficiently
 * incorporated. It stores whatever attributes of a collision box are known,
 * and removes information once it is no longer relevant. A collision can be 
 * recognized when the entry for an Id has information from all three
 * dimensions. An adjacent collision box can be recognized when the entry
 * has information from two dimensions and a search of the inverted index for
 * the third index finds it nearby.
 * **/

// It would be nice if we could be more generic, but the query processing
// really needs to know the semantics of the attributes.
enum CollisionFace
{
	COLLISION_FACE_NONE = -1,
	COLLISION_FACE_MAX_Y,
	COLLISION_FACE_MIN_Y,
	COLLISION_FACE_MIN_Z,
	COLLISION_FACE_MIN_X,
	COLLISION_FACE_MAX_Z,
	COLLISION_FACE_MAX_X,
	COLLISION_FACE_X,
	COLLISION_FACE_Y,
	COLLISION_FACE_Z,
	COLLISION_FACE_XYZ,
	// Collision box faces are entered in the same order as their opposing
	// counterparts on the entity.
	COLLISION_BOX_MIN_Y = 0,
	COLLISION_BOX_MAX_Y,
	COLLISION_BOX_MAX_Z,
	COLLISION_BOX_MAX_X,
	COLLISION_BOX_MIN_Z,
	COLLISION_BOX_MIN_X,
};

/**
 * Access to a list of Ids kept in sorted order.
 *
 * Caller should invoke hasNext() and if it returns true invoke peek() to
 * get the next Id and forward() to advance past it.
 *
 * The behavior of peek() is undefined if hasNext() would have returned false.
 **/
class IndexListIterator
{
public:
	IndexListIterator() {}
	virtual ~IndexListIterator() {}
	bool hasNext() { return m_hasnext; }
	u32 peek() { return m_next; }

	// Return the next face of the current id. Most iterators only
	// return one value per id, but unions and intersections may return
	// several.
	// If offset is not nullptr, the offset of the face is copied there.
	virtual CollisionFace nextFace(f32 *offset = nullptr) { return COLLISION_FACE_NONE; }

	// Go forward.
	// Returns true if the end of the list was not reached.
	virtual bool forward() { return false; }

	// Go forward until m_next >= id.
	// Returns true if the end of list was not reached.
	virtual bool skipForward(u32 id) { return false; }
	
	// Backwards sort to get min heap behavior from max heap implementation.
	static bool compare(IndexListIterator *a, IndexListIterator *b) { return a->peek() > b->peek(); }

	// Flag whether this object was allocated and should be deleted.
	IndexListIterator *markAllocated()
	{
		m_allocated = true;
		return this;
	}

	bool wasAllocated() { return m_allocated; }

protected:
	bool m_hasnext = false;
	u32 m_next;
	bool m_allocated = false;
};

class SingleIndexListIterator : public IndexListIterator
{
public:
	// This object does NOT take responsibility for deleting vec.
	SingleIndexListIterator(CollisionFace face, f32 offset, const std::vector<u32> *vec) :
			m_face(face), m_offset(offset), m_list(vec), m_iter(vec->begin())
	{
		m_next_face = m_face;
		if ((m_hasnext = m_iter < m_list->end()))
			m_next = *m_iter;
	}

	virtual CollisionFace nextFace(f32 *offset = nullptr)
	{
		CollisionFace face = m_next_face;
		if (offset != nullptr && face != COLLISION_FACE_NONE)
			*offset = m_offset;
		m_next_face = COLLISION_FACE_NONE;
		return face;
	}

	virtual bool forward()
	{
		m_next_face = m_face;
		if ((m_hasnext = ++m_iter < m_list->end()))
			m_next = *m_iter;
		return m_hasnext;
	}

	virtual bool skipForward(u32 id)
	{
		m_next_face = m_face;
		m_iter = std::lower_bound(m_iter, m_list->end(), id);
		if ((m_hasnext = m_iter < m_list->end()))
			m_next = *m_iter;
		return m_hasnext;
	}


protected:
	CollisionFace m_face;
	f32 m_offset;
	const std::vector<u32> *m_list;
	std::vector<u32>::const_iterator m_iter; // Iterator positioned at next
	CollisionFace m_next_face;
};

class IndexListIteratorSet
{
public:
	IndexListIteratorSet() : m_set(new std::vector<IndexListIterator *>()) {}

	~IndexListIteratorSet()
	{
		for(u32 i = 0; i < m_set->size(); i++)
			if (m_set->at(i)->wasAllocated())
				delete m_set->at(i);
		delete m_set;
	}

	void add(CollisionFace face, f32 offset, const std::vector<u32> *vec)
	{
		if (!vec->empty())
			m_set->push_back((new SingleIndexListIterator(face, offset, vec))->markAllocated());

	}

	void add(IndexListIterator *iter)
	{
		if (iter->hasNext())
			m_set->push_back(iter);
	}

	// Return an IndexListIterator over the union/intersection of sources.
	// Clear the sources. This object can be reused on a new set of sources.
	IndexListIterator *getUnion();
	IndexListIterator *getIntersection();

protected:
	std::vector<IndexListIterator *> *m_set;
};

class IndexListIteratorDifference : public IndexListIterator
{
public:
	IndexListIteratorDifference(IndexListIterator *a, IndexListIterator *b) :
			m_iter_a(a), m_iter_b(b)
	{
		if (a->hasNext())
			forwardB();
	}

	virtual ~IndexListIteratorDifference()
	{
		if (m_iter_a->wasAllocated())
			delete m_iter_a;
		if (m_iter_b->wasAllocated())
			delete m_iter_b;
	}

	bool restart(IndexListIterator *a, IndexListIterator *b)
	{
		if (m_iter_a->wasAllocated())
			delete m_iter_a;
		if (m_iter_b->wasAllocated())
			delete m_iter_b;
		m_iter_a = a;
		m_iter_b = b;
		if (a->hasNext())
			return forwardB();
		return false;
	}

	virtual CollisionFace nextFace(f32 *offset = nullptr)
	{
		return m_iter_a->nextFace(offset);
	}

	virtual bool forward()
	{
		if ((m_hasnext = m_iter_a->forward()))
			return forwardB();
		return false;
	}

	virtual bool skipForward(u32 id)
	{
		if ((m_hasnext = m_iter_a->skipForward(id)))
			return forwardB();
		return false;
	}

protected:
	bool forwardB();

	IndexListIterator *m_iter_a;	// Return only ids found in this list.
	IndexListIterator *m_iter_b;	// Exclude ids found in this list.
};

typedef std::pair<f32, std::vector<u32>> AttributeIndex;

class InvertedIndex
{
public:
	// Add the box to the inverted index. Return its Id.
	// Ids are sequential starting with 0.
	u32 index(const aabb3f box);

	// Get the maximal difference between the MaxEdge and MinEdge.
	v3f getMaxWidth() const { return m_maxWidth; }
	
	// Get handles for offset ranges.
	s32 lowerAttributeBound(CollisionFace face, f32 offset) const;
	s32 lowerAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const;
	s32 upperAttributeBound(CollisionFace face, f32 offset) const;
	s32 upperAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const;
	s32 lowerBackAttributeBound(CollisionFace face, f32 offset) const;
	s32 lowerBackAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const;
	s32 upperBackAttributeBound(CollisionFace face, f32 offset) const;
	s32 upperBackAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const;

	// Get the AttributeIndex for a handle.
	const AttributeIndex *getAttributeIndex(CollisionFace face, u32 handle) const;

	// Get the open interval (a, b) or the closed interval [b, a] from
	// the specified face into the provided set.
	void getInterval(CollisionFace face, f32 a, f32 b, IndexListIteratorSet *set);
	// Get [a, b) or (b, a]
	void getHalfOpenInterval(CollisionFace face, f32 a, f32 b, IndexListIteratorSet *set);

protected:
	std::vector<u32> *findAttributeIndex(CollisionFace face, f32 offset);
	void addToSet(CollisionFace face, u32 a, u32 b, IndexListIteratorSet *set);
	static bool lowerCompare(AttributeIndex a, f32 offset) { return a.first < offset; }
	static bool lowerCompareBack(AttributeIndex a, f32 offset) { return a.first > offset; }
	static bool upperCompare(f32 offset, AttributeIndex b) { return b.first > offset; }
	static bool upperCompareBack(f32 offset, AttributeIndex b) { return b.first < offset; }

	std::vector<AttributeIndex> m_index[6];
	u32 m_count;
	v3f m_maxWidth;
};

// Detail for each collision box in the context.
struct CollisionContextDetail
{
	u16 valid_faces;
	f32 face_offset[6];

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
	f32 m_face_offset[6];
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
