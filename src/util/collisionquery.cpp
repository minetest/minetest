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
const u16 CollisionQueryContext::testBitmask[] = {
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
const u16 CollisionQueryContext::setBitmask[] = {
		1 | 128,	// COLLISION_MAX_Y
		2 | 128,	// COLLISION_MIN_Y
		4 | 256,	// COLLISION_MIN_Z
		8 | 64, // COLLISION_MIN_X
		16 | 256,       // COLLISION_MAX_Z
		32 | 64,	// COLLISION_MAX_X
	};
const u16 CollisionQueryContext::unsetBitmask[] = {
		1 | 2 | 128,    // COLLISION_MAX_Y
		1 | 2 | 128,    // COLLISION_MIN_Y
		4 | 16 | 256,   // COLLISION_MIN_Z
		8 | 32 | 64,    // COLLISION_MIN_X
		4 | 16 | 256,   // COLLISION_MAX_Z
		8 | 32 | 64,    // COLLISION_MAX_X
	};
const CollisionFace CollisionQueryContext::opposingFace[] = {
		COLLISION_BOX_MIN_Y,
		COLLISION_BOX_MAX_Y,
		COLLISION_BOX_MAX_Z,
		COLLISION_BOX_MAX_X,
		COLLISION_BOX_MIN_Z,
		COLLISION_BOX_MIN_X,
	};

CollisionQueryContext::CollisionQueryContext(u16 ctx, aabb3f box, const InvertedIndex *index, std::vector<Collision> *collisions) :
		m_index(index), m_ctx(ctx)
{
	v3f width = index->getMaxWidth();
	u32 base;
	if (collisions)
		base = collisions->size();

	init(COLLISION_FACE_MIN_X, box.MinEdge.X, COLLISION_FACE_MAX_X, box.MaxEdge.X, width.X, collisions);
	init(COLLISION_FACE_MIN_Y, box.MinEdge.Y, COLLISION_FACE_MAX_Y, box.MaxEdge.Y, width.Y, collisions);
	init(COLLISION_FACE_MIN_Z, box.MinEdge.Z, COLLISION_FACE_MAX_Z, box.MaxEdge.Z, width.Z, collisions);
	
	if (collisions)
	{
		if (m_active.empty())
		{
			// All collisions were ephemeral
			collisions->resize(base);
			return;
		}

		u32 i = base;
		while (i < collisions->size())
		{
			// Compress, keeping only true collisions.
			u32 id = collisions->at(base).id;
			if (m_active.find(id) != m_active.end() && (m_active[id].valid_faces & COLLISION_FACE_XYZ) == COLLISION_FACE_XYZ)
				collisions->at(base++) = collisions->at(i);
			i++;
		}
		collisions->resize(base);
	}
}

u32 CollisionQueryContext::init(CollisionFace min_face, f32 min_offset, CollisionFace max_face, f32 max_offset, f32 width, std::vector<Collision> *collisions)
{
	// Store face offsets for the box.
	m_face_offset[min_face] = min_offset;
	m_face_offset[max_face] = max_offset;

	// Store widths for the box, to ensure a stable width during movement.
	m_width[min_face] = max_offset - min_offset;
	m_width[max_face] = min_offset - max_offset;

	u32 min_handle0 = m_index->lowerAttributeBound(min_face, min_offset - width);
	u32 min_handle1 = m_index->lowerAttributeBound(min_face, min_offset);
	u32 min_handle3 = m_index->upperAttributeBound(min_face, max_offset + width);
	u32 max_handle0 = m_index->lowerAttributeBound(max_face, min_offset - width);
	u32 max_handle2 = m_index->upperAttributeBound(max_face, max_offset);
	u32 max_handle3 = m_index->upperAttributeBound(max_face, max_offset + width);

	u32 count = add(min_face, min_handle1, min_handle3, collisions, 0, COLLISION_STATIC)
			+ add(max_face, max_handle0, max_handle2, collisions, 0, COLLISION_STATIC); 
	remove(min_face, min_handle0, min_handle1); 
	remove(max_face, max_handle2, max_handle3); 
	return count;
}

u32 CollisionQueryContext::moveAdd(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions, f32 dtime)
{
	// Move the face
	m_face_offset[face] = offset;
	m_face_offset[opposingFace[face]] = offset + m_width[face];

	// Add to the context
	return add(face, offset, ids, collisions, dtime, COLLISION_ENTRY);
}

u32 CollisionQueryContext::moveRemove(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions, f32 dtime)
{
	// Move the face
	m_face_offset[face] = offset;
	m_face_offset[opposingFace[face]] = offset + m_width[face];

	// Add to the context
	return remove(face, offset, ids, collisions, dtime, COLLISION_EXIT);
}

u32 CollisionQueryContext::add(CollisionFace face, u32 begin, u32 end, std::vector<Collision> *collisions, f32 dtime, CollisionType type)
{
	u32 count = 0;
	while (begin < end)
	{
		const AttributeIndex *ai = m_index->getAttributeIndex(face, begin++);
		count += add(face, ai->first, &ai->second, collisions, type);
	}
	return count;
}

u32 CollisionQueryContext::add(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions, f32 dtime, CollisionType type)
{
	u32 count = 0;

	// Process each box.
	for (u32 i = 0; i < ids->size(); i++)
	{
		u32 id = ids->at(i);
		if (m_active.empty() || m_active.find(id) == m_active.end())
			m_active.emplace(std::piecewise_construct, std::tuple<u32>(id), std::tuple<>());

		m_active.at(id).valid_faces |= setBitmask[face];
		m_active.at(id).face_offset[face] = offset;

		if ((m_active.at(id).valid_faces & testBitmask[COLLISION_FACE_XYZ]) == testBitmask[COLLISION_FACE_XYZ])
		{
			rawstream << "Collision noticed at " << m_active.at(id).valid_faces << std::endl;
			count += registerCollision(id, face, collisions, dtime, type);
		}
	}

	return count;
}

u32 CollisionQueryContext::remove(CollisionFace face, u32 begin, u32 end, std::vector<Collision> *collisions, f32 dtime, CollisionType type)
{
	u32 count = 0;
	while (begin < end)
	{
		const AttributeIndex *ai = m_index->getAttributeIndex(face, begin++);
		count += remove(face, ai->first, &ai->second, collisions, type);
	}
	return count;
}

u32 CollisionQueryContext::remove(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions, f32 dtime, CollisionType type)
{
	u32 count = 0;

	// Process each box.
	for (u32 i = 0; i < ids->size(); i++)
	{
		u32 id = ids->at(i);
		if (m_active.empty() || m_active.find(id) == m_active.end())
			m_active.emplace(std::piecewise_construct, std::tuple<u32>(id), std::tuple<>());

		m_active.at(id).face_offset[face] = offset;

		if ((m_active.at(id).valid_faces & testBitmask[COLLISION_FACE_XYZ]) == testBitmask[COLLISION_FACE_XYZ])
		{
			rawstream << "Collision noticed at " << m_active.at(id).valid_faces << std::endl;
			count += registerCollision(id, face, collisions, dtime, type);
		}

		m_active.at(id).valid_faces |= unsetBitmask[face];
		if (!m_active.at(id).valid_faces)
			m_active.erase(id);
	}

	return count;
}

u32 CollisionQueryContext::registerCollision(u32 id, CollisionFace face, std::vector<Collision> *collisions, f32 dtime, CollisionType type) const
{
	u32 count = 0;
	const CollisionQueryContextDetail &detail = m_active.at(id);

	for (int f = 0; f < 6; f++)
	{
		CollisionFace ff = static_cast<CollisionFace>(f);
		f32 overlap =  detail.face_offset[f] - m_face_offset[f];
		if (ff == face || ((detail.valid_faces & testBitmask[f]) && irr::core::equals(overlap, 0.f)))
		{	
			rawstream << "Collision time " << dtime << " face " << ff << " box " << id << std::endl;
			if (collisions)
			{
				if (ff == COLLISION_FACE_MAX_X || ff == COLLISION_FACE_MAX_Y || ff == COLLISION_FACE_MAX_Z)
					overlap = -overlap;
				collisions->emplace_back(m_ctx, ff, id, overlap, dtime, type);
			}
			count++;
		}
	}
	return count;
}

class ForwardQuery : public CollisionQuery
{
public:
	ForwardQuery(CollisionQueryContext *ctx, CollisionFace face, f32 v, f32 dtime, bool context_owner=false) : CollisionQuery(ctx, dtime, context_owner), m_face(face), m_scale(1.f/v)
	{
		m_offset = ctx->getFaceOffset(face);
		init(v);
		update();
	}
	
	virtual u32 getCollisions(f32 dtime, std::vector<Collision> *collisions)
	{
		const InvertedIndex *index = m_context->getInvertedIndex();
		bool notdone = true;
		u32 count = 0;
		while (notdone && m_dtime <= dtime)
		{
			const AttributeIndex *ai = index->getAttributeIndex(m_face, m_handle);
			count += m_context->moveAdd(m_face, ai->first, &ai->second, collisions, m_dtime);
			advance();
			notdone = update();
		}
		return count;
	}
	
protected:
	virtual void init(f32 v)
	{
		const InvertedIndex *index = m_context->getInvertedIndex();
		m_handle = index->lowerAttributeBound(m_face, m_offset);
		m_end = index->upperAttributeBound(m_face, m_offset + v * m_endtime);
	}
	virtual void advance() { m_handle++; }
	virtual bool update()
	{
		if (m_handle < m_end)
		{
			const InvertedIndex *index = m_context->getInvertedIndex();
			m_dtime = (index->getAttributeIndex(m_face, m_handle)->first - m_offset) * m_scale;
			return true;
		}

		m_dtime = m_end;
		return false;
	}
		
	CollisionFace m_face;
	f32 m_scale;
	f32 m_offset;
	u32 m_handle;
	u32 m_end;
};

class BackwardQuery : public ForwardQuery
{
public:
	// v should be negative
	BackwardQuery(CollisionQueryContext *ctx, CollisionFace face, f32 v, f32 dtime, bool context_owner=false) : ForwardQuery(ctx, face, v, dtime, context_owner) {}
	
protected:
	virtual void init(f32 v)
	{
		const InvertedIndex *index = m_context->getInvertedIndex();
		m_handle = index->lowerBackAttributeBound(m_face, m_offset);
		m_end = index->upperBackAttributeBound(m_face, m_offset + v * m_endtime);
	}
	virtual void advance() { m_handle--; }
	virtual bool update()
	{
		if (m_handle > m_end)
		{
			const InvertedIndex *index = m_context->getInvertedIndex();
			m_dtime = (index->getAttributeIndex(m_face, m_handle)->first - m_offset) * m_scale;
			return true;
		}

		m_dtime = m_end;
		return false;
	}
		
};



/* TODO
CollisionQuery *CollisionQuery::getLinearQuery(aabb3f box, f32 dtime, v3f velocity, InvertedIndex *index, std::vector<Collision> *collisions)
{
	return nullptr;
}
*/

CollisionQuery *CollisionQuery::get1dQuery(aabb3f box, f32 dtime, CollisionFace face, f32 velocity, InvertedIndex *index, std::vector<Collision> *collisions)
{
	CollisionQueryContext *c = new CollisionQueryContext(0, box, index, collisions);

	if (velocity < 0)
	{
		if (face == COLLISION_FACE_X)
			face = COLLISION_FACE_MIN_X;
		else if (face == COLLISION_FACE_Y)
			face = COLLISION_FACE_MIN_Y;
		else if (face == COLLISION_FACE_Z)
			face = COLLISION_FACE_MIN_Z;
		return new BackwardQuery(c, face, velocity, dtime, true);
	}
	if (velocity > 0)
	{
		if (face == COLLISION_FACE_X)
			face = COLLISION_FACE_MAX_X;
		else if (face == COLLISION_FACE_Y)
			face = COLLISION_FACE_MAX_Y;
		else if (face == COLLISION_FACE_Z)
			face = COLLISION_FACE_MAX_Z;
		return new ForwardQuery(c, face, velocity, dtime, true);
	}
	return new CollisionQuery(c, dtime, true);
}
