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

#include <algorithm>

#include "invertedindex.h"
#include "log.h"

class DualIndexListIteratorUnion : public IndexListIterator
{
public:
	DualIndexListIteratorUnion(IndexListIterator *a, IndexListIterator *b) : m_set{a, b}
	{
		update();
	}

	virtual ~DualIndexListIteratorUnion()
	{
		if (m_set[0]->wasAllocated())
			delete m_set[0];
		if (m_set[1]->wasAllocated())
			delete m_set[1];
	}

	virtual CollisionFace nextFace(f32 *offset = nullptr)
	{
		if (m_current & CURRENT_A)
		{
			CollisionFace face = m_set[0]->nextFace(offset);

			if (face == COLLISION_FACE_NONE)
				m_current &= ~CURRENT_A;
			else
				return face;
		}

		if (m_current & CURRENT_B)
			return m_set[1]->nextFace(offset);

		return COLLISION_FACE_NONE;
	}

	virtual bool forward()
	{
		if (m_current & MATCH_A)
			m_set[0]->forward();
		if (m_current & MATCH_B)
			m_set[1]->forward();
		return update();
	}

	virtual bool skipForward(u32 id)
	{
		if (m_set[0]->hasNext())
			m_set[0]->skipForward(id);
		if (m_set[1]->hasNext())
			m_set[1]->skipForward(id);
		return update();
	}

protected:
	bool update()
	{
		if (!m_set[0]->hasNext())
		{
			if (!m_set[1]->hasNext())
				return m_hasnext = false;
			else
			{
				m_current = MATCH_B | CURRENT_B;
				m_next = m_set[1]->peek();
				return m_hasnext = true;
			}
		}

		if (!m_set[1]->hasNext())
		{
			m_current = MATCH_A | CURRENT_A;
			m_next = m_set[0]->peek();
			return m_hasnext = true;
		}

		// If the value at 0 is less than or equal to the value at 1,
		// compare() will return false and we will assign m_current = 0,
		// which means that the next item being returned is at 0.
		// Otherwise, compare() will return true, so we will assign m_current = 1 ,
		// skipping 0 for now and only returning the item at 1.
		m_next = m_set[0]->peek();
		u32 b = m_set[1]->peek();

		if (m_next < b)
			m_current = MATCH_A | CURRENT_A;
		else if (b < m_next)
		{
			m_next = b;
			m_current = MATCH_B | CURRENT_B;
		} else
			m_current = MATCH_A | CURRENT_A | MATCH_B | CURRENT_B;

		return m_hasnext = true;
	}

	static const u32 MATCH_A = 1;
	static const u32 MATCH_B = 2;
	static const u32 CURRENT_A = 4;
	static const u32 CURRENT_B = 8;
	IndexListIterator *m_set[2];
	u32 m_current;
};

class DualIndexListIteratorIntersection : public IndexListIterator
{
public:
	DualIndexListIteratorIntersection(IndexListIterator *a, IndexListIterator *b) : m_iter_a(a), m_iter_b(b)
	{
		update();
	}

	virtual ~DualIndexListIteratorIntersection()
	{
		if (m_iter_a->wasAllocated())
			delete m_iter_a;
		if (m_iter_b->wasAllocated())
			delete m_iter_b;
	}

	virtual CollisionFace nextFace(f32 *offset = nullptr)
	{
		if(m_current == 0)
		{
			CollisionFace face = m_iter_a->nextFace(offset);
			if (face != COLLISION_FACE_NONE)
				return face;

			m_current = 1;
		}

		return m_iter_b->nextFace(offset);
	}

	virtual bool forward()
	{
		return m_iter_a->forward() && m_iter_b->forward() && update();
	}

	virtual bool skipForward(u32 id)
	{
		return m_iter_a->skipForward(id) && m_iter_b->skipForward(id) && update();
	}

protected:
	bool update()
	{
		if (!m_iter_a->hasNext() || !m_iter_b->hasNext())
			return m_hasnext = false;

		m_next = m_iter_a->peek();
		u32 b = m_iter_b->peek();

		while (m_next != b)
			if (m_next < b)
			{
				if (!m_iter_a->skipForward(b))
					return m_hasnext = false;
				m_next = m_iter_a->peek();
			} else
			{
				if (!m_iter_b->skipForward(m_next))
					return m_hasnext = false;
				b = m_iter_b->peek();
			}
				
		m_current = 0;
		return m_hasnext = true;
	}

	IndexListIterator *m_iter_a;
	IndexListIterator *m_iter_b;
	u32 m_current;
};

class MultiIndexListIteratorUnion : public IndexListIterator
{
public:
	MultiIndexListIteratorUnion(std::vector<IndexListIterator *> *set) : m_set(set)
	{
		if(m_set->empty())
			return;

		m_hasnext = true;
		std::make_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		m_next = m_set->front()->peek();
	}

	virtual ~MultiIndexListIteratorUnion()
	{
		for(u32 i = 0; i < m_set->size(); i++)
			if (m_set->at(i)->wasAllocated())
				delete m_set->at(i);

		delete m_set;
	}

	virtual CollisionFace nextFace(f32 *offset = nullptr)
	{
		if (m_set->empty() || m_set->front()->peek() > m_next)
			return COLLISION_FACE_NONE;

		CollisionFace face = m_set->front()->nextFace(offset);
		std::pop_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		if (m_set->back()->forward())
			std::push_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		else
		{
			if (m_set->back()->wasAllocated())
				delete m_set->back();
			m_set->pop_back();
		}

		return face;
	}

	virtual bool forward()
	{
		u32 next;
		while (!m_set->empty() && (next = m_set->front()->peek()) == m_next)
		{
			std::pop_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
			if (m_set->back()->forward())
				std::push_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
			else
			{
				if (m_set->back()->wasAllocated())
					delete m_set->back();
				m_set->pop_back();
			}
		}
		m_next = next;
		return m_hasnext = !m_set->empty();
	}
			
	virtual bool skipForward(u32 id)
	{
		while (!m_set->empty() && (m_next = m_set->front()->peek()) < id)
		{
			std::pop_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
			if (m_set->back()->skipForward(id))
				std::push_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
			else
			{
				if (m_set->back()->wasAllocated())
					delete m_set->back();
				m_set->pop_back();
			}
		}
		return m_hasnext = !m_set->empty();
	}
			
protected:
	// Invariant: m_set contains iterators that have not reached end.
	std::vector<IndexListIterator *> *m_set;
};

class MultiIndexListIteratorIntersection : public IndexListIterator
{
public:
	MultiIndexListIteratorIntersection(std::vector<IndexListIterator *> *set) : m_set(set)
	{
		if(m_set->empty())
			return;

		std::make_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		// After make_heap, the maximal value will be in the second
		// half of m_set;
		m_max = m_set->back()->peek();
		u32 id;

		for (u32 i = m_set->size() / 2; i < m_set->size() - 1; i++)
			if ((id = m_set->at(i)->peek()) > m_max)
				m_max = id;

		update();
	}

	virtual ~MultiIndexListIteratorIntersection()
	{
		for(u32 i = 0; i < m_set->size(); i++)
			if (m_set->at(i)->wasAllocated())
				delete m_set->at(i);

		delete m_set;
	}

	virtual CollisionFace nextFace(f32 *offset = nullptr)
	{
		if (m_set->front()->peek() > m_next)
			return COLLISION_FACE_NONE;

		CollisionFace face = m_set->front()->nextFace(offset);
		
		while (face == COLLISION_FACE_NONE)
		{
			forwardFirst();
			if (m_set->empty() || m_set->front()->peek() > m_next)
				return COLLISION_FACE_NONE;
			face = m_set->front()->nextFace(offset);
		}
			
		return face;
	}

	virtual bool forward()
	{
		while (!m_last && !m_set->empty() && m_set->front()->peek() == m_next)
			forwardFirst();
		
		return update();
	}
			
	virtual bool skipForward(u32 id)
	{
		while (!m_last && !m_set->empty() && m_set->front()->peek() < id)
			skipForwardFirst(id);

		return update();
	}
			
protected:
	void forwardFirst()
	{
		std::pop_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		if (m_set->back()->forward())
		{
			u32 id = m_set->back()->peek();
			if (id > m_max)
				m_max = id;

			std::push_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		} else
		{
			m_last = true;
			if (m_set->back()->wasAllocated())
				delete m_set->back();
			m_set->pop_back();
		}
	}

	void skipForwardFirst(u32 id)
	{
		std::pop_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		if (m_set->back()->skipForward(id))
		{
			u32 id = m_set->back()->peek();
			if (id > m_max)
				m_max = id;

			std::push_heap(m_set->begin(), m_set->end(), IndexListIterator::compare);
		} else
		{
			m_last = true;
			if (m_set->back()->wasAllocated())
				delete m_set->back();
			m_set->pop_back();
		}
	}

	bool update()
	{

		while (!m_last && !m_set->empty() && m_set->front()->peek() != m_max)
			skipForwardFirst(m_max);

		m_next = m_max;
		return m_hasnext = !m_last;
	}
		
	// Invariant: m_set contains iterators that have not reached end.
	std::vector<IndexListIterator *> *m_set;
	u32 m_max;
	bool m_last = false;
};

IndexListIterator *IndexListIteratorSet::getUnion()
{
	IndexListIterator *iter;

	switch(m_set->size())
	{
	case 0:
		return (new IndexListIterator())->markAllocated();

	case 1:
		iter = m_set->back();
		m_set->pop_back();
		return iter;

	case 2:
		iter = (new DualIndexListIteratorUnion(m_set->front(), m_set->back()))->markAllocated();
		m_set->clear();
		return iter;

	default:
		iter =  (new MultiIndexListIteratorUnion(m_set))->markAllocated();
		m_set = new std::vector<IndexListIterator *>();
		return iter;
	}
}

IndexListIterator *IndexListIteratorSet::getIntersection()
{
	IndexListIterator *iter;

	switch(m_set->size())
	{
	case 0:
		return (new IndexListIterator())->markAllocated();

	case 1:
		iter = m_set->back();
		m_set->pop_back();
		return iter;

	case 2:
		iter = (new DualIndexListIteratorIntersection(m_set->front(), m_set->back()))->markAllocated();
		m_set->clear();
		return iter;

	default:
		iter = (new MultiIndexListIteratorIntersection(m_set))->markAllocated();
		m_set = new std::vector<IndexListIterator *>();
		return iter;
	}
}

bool IndexListIteratorDifference::forwardB()
{
	m_next = m_iter_a->peek();

	while (m_iter_b->skipForward(m_next) && m_iter_b->peek() == m_next)
	{
		// id is in both lists, so skip past it.
		if (!m_iter_a->forward())
			return m_hasnext = false;

		m_next = m_iter_a->peek();
	}

	return m_hasnext = true;
}

u32 InvertedIndex::index(const aabb3f box)
{
	// Assign id to box
	u32 id = m_count++;

	// Add box to indices.
	findAttributeIndex(COLLISION_BOX_MIN_X, box.MinEdge.X)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MIN_Y, box.MinEdge.Y)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MIN_Z, box.MinEdge.Z)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MAX_X, box.MaxEdge.X)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MAX_Y, box.MaxEdge.Y)->push_back(id);
	findAttributeIndex(COLLISION_BOX_MAX_Z, box.MaxEdge.Z)->push_back(id);

	// Update max width.
	v3f width = box.MaxEdge - box.MinEdge;
	if (width.X > m_maxWidth.X)
		m_maxWidth.X = width.X;
	if (width.Y > m_maxWidth.Y)
		m_maxWidth.Y = width.Y;
	if (width.Z > m_maxWidth.Z)
		m_maxWidth.Z = width.Z;

	return id;
}

// Find the index of the first entry >= offset
s32 InvertedIndex::lowerAttributeBound(CollisionFace face, f32 offset) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::lower_bound(idx.begin(), idx.end(), offset, lowerCompare) - idx.begin();
}

s32 InvertedIndex::lowerAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::lower_bound(idx.begin() + begin, idx.begin() + end, offset, lowerCompare) - idx.begin();
}

s32 InvertedIndex::upperAttributeBound(CollisionFace face, f32 offset) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::upper_bound(idx.begin(), idx.end(), offset, upperCompare) - idx.begin();
}

s32 InvertedIndex::upperAttributeBound(CollisionFace face, f32 offset, u32 begin, u32 end) const
{
	const std::vector<AttributeIndex> &idx=m_index[face];
	return std::upper_bound(idx.begin() + begin, idx.begin() + end, offset, upperCompare) - idx.begin();
}

const AttributeIndex *InvertedIndex::getAttributeIndex(CollisionFace face, u32 handle) const
{
	rawstream << "getAttributeIndex " << m_index[face].size() << std::endl;
	return &m_index[face].at(handle);
}

void InvertedIndex::addToSet(CollisionFace face, u32 a, u32 b, IndexListIteratorSet *set)
{
	rawstream << "adding (" << a << ", " << b << ") (";
	if (b < a)
		std::swap(a, b);
	rawstream << a << ", " << b << ")";
	while (a < b)
	{
		rawstream << ' ' << a;
		AttributeIndex &idx = m_index[face].at(a++);
		set->add(face, idx.first, &idx.second);
	}
	rawstream << std::endl;
}

void InvertedIndex::getInterval(CollisionFace face, f32 a, f32 b, IndexListIteratorSet *set)
{
	addToSet(face, upperAttributeBound(face, a), lowerAttributeBound(face, b), set);
}

void InvertedIndex::getHalfOpenInterval(CollisionFace face, f32 a, f32 b, IndexListIteratorSet *set)
{
	if (b < a)
		addToSet(face, upperAttributeBound(face, a), upperAttributeBound(face, b), set);
	else
		addToSet(face, lowerAttributeBound(face, a), lowerAttributeBound(face, b), set);
}

std::vector<u32> *InvertedIndex::findAttributeIndex(CollisionFace face, f32 offset)
{
	std::vector<AttributeIndex> &idx = m_index[face];

	if (idx.empty())
	{
		idx.emplace_back(offset, std::vector<u32>());
		rawstream << "Emplaced " << face << " " << offset << " " << idx.front().second.size() << std::endl;
		return &idx.front().second;
	}

	std::vector<AttributeIndex>::iterator i = std::lower_bound(idx.begin(), idx.end(), offset, lowerCompare);

	if (i->first == offset)
		return &i->second;

	std::vector<u32> *vec = &idx.emplace(i, offset, std::vector<u32>())->second;
	rawstream << "Emplaced " << face << " " << offset << " " << vec->size() << std::endl;
	return vec;
}
