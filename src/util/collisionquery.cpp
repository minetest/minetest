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

#include "collisionquery.h"
#include "log.h"

// Match NodeDef.
const u16 CollisionContext::testBitmask[] = {
		1,	// COLLISION_MAX_Y
		2,	// COLLISION_MIN_Y
		4,	// COLLISION_MIN_Z
		8, // COLLISION_MIN_X
		16,       // COLLISION_MAX_Z
		32,	// COLLISION_MAX_X
		64,     // COLLISION_FACE_X
		128,    // COLLISION_FACE_Y
		256,    // COLLISION_FACE_Z
		64 | 128 | 256, // COLLISION_FACE_XYZ
	};
const u16 CollisionContext::setBitmask[] = {
		1 | 128,	// COLLISION_MAX_Y
		2 | 128,	// COLLISION_MIN_Y
		4 | 256,	// COLLISION_MIN_Z
		8 | 64, // COLLISION_MIN_X
		16 | 256,       // COLLISION_MAX_Z
		32 | 64,	// COLLISION_MAX_X
	};
const u16 CollisionContext::unsetBitmask[] = {
		1 | 2 | 128,    // COLLISION_MAX_Y
		1 | 2 | 128,    // COLLISION_MIN_Y
		4 | 16 | 256,   // COLLISION_MIN_Z
		8 | 32 | 64,    // COLLISION_MIN_X
		4 | 16 | 256,   // COLLISION_MAX_Z
		8 | 32 | 64,    // COLLISION_MAX_X
	};
const CollisionFace CollisionContext::opposingFace[] = {
		COLLISION_BOX_MIN_Y,
		COLLISION_BOX_MAX_Y,
		COLLISION_BOX_MAX_Z,
		COLLISION_BOX_MAX_X,
		COLLISION_BOX_MIN_Z,
		COLLISION_BOX_MIN_X,
	};

CollisionContext::CollisionContext(aabb3f box, InvertedIndex *index)
{
	// Store face offsets for the box.
	m_face_offset[COLLISION_FACE_MIN_X] = box.MinEdge.X;
	m_face_offset[COLLISION_FACE_MIN_Y] = box.MinEdge.Y;
	m_face_offset[COLLISION_FACE_MIN_Z] = box.MinEdge.Z;
	m_face_offset[COLLISION_FACE_MAX_X] = box.MaxEdge.X;
	m_face_offset[COLLISION_FACE_MAX_Y] = box.MaxEdge.Y;
	m_face_offset[COLLISION_FACE_MAX_Z] = box.MaxEdge.Z;

	// Search the InvertedIndex for boxes that overlap with this box
	// on any one dimension.
	// Criteria: box.min - maxwidth < collision.min < box.max
	// && box.min < collision.max < box.max + maxwidth
	v3f width = index->getMaxWidth();

	IndexListIteratorSet pos, neg;

	index->getInterval(COLLISION_FACE_MIN_X, box.MinEdge.X, box.MaxEdge.X + width.X, &pos);
	index->getInterval(COLLISION_FACE_MAX_X, box.MaxEdge.X, box.MaxEdge.X + 2 * width.X, &neg);
	IndexListIteratorDifference diff(pos.getUnion(), neg.getUnion());
	addIndexList(&diff);

	index->getInterval(COLLISION_FACE_MAX_X, box.MinEdge.X - width.X, box.MaxEdge.X, &pos);
	index->getInterval(COLLISION_FACE_MIN_X, box.MinEdge.X - width.X * 2, box.MinEdge.X, &neg);
	diff.restart(pos.getUnion(), neg.getUnion());
	addIndexList(&diff);

	index->getInterval(COLLISION_FACE_MIN_Y, box.MinEdge.Y, box.MaxEdge.Y + width.Y, &pos);
	index->getInterval(COLLISION_FACE_MAX_Y, box.MaxEdge.Y, box.MaxEdge.Y + 2 * width.Y, &neg);
	diff.restart(pos.getUnion(), neg.getUnion());
	addIndexList(&diff);

	index->getInterval(COLLISION_FACE_MAX_Y, box.MinEdge.Y - width.Y, box.MaxEdge.Y, &pos);
	index->getInterval(COLLISION_FACE_MIN_Y, box.MinEdge.Y - width.Y * 2, box.MinEdge.Y, &neg);
	diff.restart(pos.getUnion(), neg.getUnion());
	addIndexList(&diff);

	index->getInterval(COLLISION_FACE_MIN_Z, box.MinEdge.Z, box.MaxEdge.Z + width.Z, &pos);
	index->getInterval(COLLISION_FACE_MAX_Z, box.MaxEdge.Z, box.MaxEdge.Z + 2 * width.Z, &neg);
	diff.restart(pos.getUnion(), neg.getUnion());
	addIndexList(&diff);

	index->getInterval(COLLISION_FACE_MAX_Z, box.MinEdge.Z - width.Z, box.MaxEdge.Z, &pos);
	index->getInterval(COLLISION_FACE_MIN_Z, box.MinEdge.Z - width.Z * 2, box.MinEdge.Z, &neg);
	diff.restart(pos.getUnion(), neg.getUnion());
	addIndexList(&diff);
}

u32 CollisionContext::addIndexList(IndexListIterator *index)
{
	u32 count = 0;

	if (index->hasNext())
		do
		{	
			u32 id = index->peek();
			f32 offset;
			CollisionFace face = index->nextFace(&offset);
			

			if (face != COLLISION_FACE_NONE && m_active.find(id) == m_active.end())
				m_active.emplace(std::piecewise_construct, std::tuple<u32>(id), std::tuple<>());

			while (face != COLLISION_FACE_NONE)
			{
				m_active[id].valid_faces |= setBitmask[face];
				m_active[id].face_offset[face] = offset;
				
				face = index->nextFace(&offset);
			}

			if ((m_active[id].valid_faces & setBitmask[COLLISION_FACE_XYZ]) == setBitmask[COLLISION_FACE_XYZ])
			{
				collisions.push_back(id);
				count++;
			}
		} while (index->forward());

	return count;
}

u32 CollisionContext::subtractIndexList(IndexListIterator *index)
{
	u32 count = 0;
	if (index->hasNext())
		do
		{	
			u32 id = index->peek();
			CollisionFace face = index->nextFace();

			if (face != COLLISION_FACE_NONE && m_active.find(id) == m_active.end())
				continue;

			while (face != COLLISION_FACE_NONE)
			{
				if ((m_active[id].valid_faces & setBitmask[COLLISION_FACE_XYZ]) == setBitmask[COLLISION_FACE_XYZ])
					count++;

				m_active[id].valid_faces &= ~unsetBitmask[face];
				m_active[id].face_offset[face] = 0.f;
				m_active[id].face_offset[opposingFace[face]] = 0.f;
				
				face = index->nextFace();
			}

			if (!m_active[id].valid_faces)
				m_active.erase(id);
		} while (index->forward());

	return count;
}

