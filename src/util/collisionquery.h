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
struct CollisionQueryContextDetail
{
	u16 valid_faces = 0;
	f32 face_offset[6] = {0.f, 0.f, 0.f, 0.f, 0.f, 0.f};

	CollisionQueryContextDetail(u16 face_mask, CollisionFace face, f32 offset) : valid_faces(face_mask)
	{
		face_offset[face] = offset;
	}

	CollisionQueryContextDetail() {}

	CollisionQueryContextDetail(const CollisionQueryContextDetail &copy) :
			valid_faces(copy.valid_faces),
			face_offset{
					copy.face_offset[0], copy.face_offset[1], copy.face_offset[2],
					copy.face_offset[3], copy.face_offset[4], copy.face_offset[5]
				} {}

	CollisionQueryContextDetail(CollisionQueryContextDetail &&move) :
			valid_faces(move.valid_faces),
			face_offset{
					move.face_offset[0], move.face_offset[1], move.face_offset[2],
					move.face_offset[3], move.face_offset[4], move.face_offset[5]
				} {}
};

enum CollisionType
{
	COLLISION_ENTRY,
	COLLISION_STATIC,
	COLLISION_EXIT,
};

struct Collision
{
	u16 context_id;
	CollisionType type;
	CollisionFace face;
	u32 id;
	f32 overlap;
	f32 dtime; // Time of the collision *if* known otherwise 0
	Collision() : context_id(), type(), face(), id(), overlap(), dtime() {}
	Collision(u16 ctx, CollisionFace face, u32 id, f32 overlap, f32 dtime, CollisionType type=COLLISION_ENTRY) :
			context_id(ctx), type(type), face(face), id(id), overlap(overlap), dtime(dtime) {}
};

// CollisionQueryContext does not have any time awareness, so it will set
// dtime=0 in any returned Collisions.
class CollisionQueryContext
{
public:
	CollisionQueryContext(u16 context_id, aabb3f box, const InvertedIndex *index, std::vector<Collision> *collisions=nullptr);

	const InvertedIndex *getInvertedIndex() { return m_index; }
	f32 getFaceOffset(CollisionFace face) { return m_face_offset[face]; }
	u32 moveAdd(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions, f32 dtime=0);
	u32 moveRemove(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions, f32 dtime=0);
	u32 add(CollisionFace face, u32 begin, u32 end, std::vector<Collision> *collisions=nullptr, f32 dtime=0, CollisionType type=COLLISION_ENTRY);
	u32 add(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions=nullptr, f32 dtime=0, CollisionType type=COLLISION_ENTRY);
	u32 remove(CollisionFace face, u32 begin, u32 end, std::vector<Collision> *collisions=nullptr, f32 dtime=0, CollisionType type=COLLISION_EXIT);
	u32 remove(CollisionFace face, f32 offset, const std::vector<u32> *ids, std::vector<Collision> *collisions=nullptr, f32 dtime=0, CollisionType type=COLLISION_EXIT);

protected:
	u32 init(CollisionFace min_face, f32 min_offset, CollisionFace max_face, f32 max_offset, f32 width, std::vector<Collision> *collisions);
	u32 registerCollision(u32 id, CollisionFace face, std::vector<Collision> *collisions, f32 dtime=0, CollisionType type=COLLISION_ENTRY) const;

public:
	static const u16 testBitmask[];
	static const CollisionFace opposingFace[];

protected:
	static const u16 setBitmask[];
	static const u16 unsetBitmask[];
	const InvertedIndex *m_index;
	const u16 m_ctx;
	f32 m_face_offset[6];
	f32 m_width[6];
	std::unordered_map<u32, CollisionQueryContextDetail> m_active;
};

class CollisionQuery
{
public:
	CollisionQuery(CollisionQueryContext *context, f32 dtime, bool context_owner=false) : m_context(context), m_dtime(dtime), m_endtime(dtime), m_context_owner(context_owner) {}

	virtual ~CollisionQuery()
	{
		if (m_context_owner)
			delete m_context;
	}

	f32 estimate() const { return m_dtime; }
	virtual u32 getCollisions(f32 dtime, std::vector<Collision> *collisions=nullptr) { return 0; }

	// Factory functions
	static CollisionQuery *getLinearQuery(aabb3f box, f32 dtime, v3f velocity, InvertedIndex *index, std::vector<Collision> *collisions=nullptr);
	static CollisionQuery *get1dQuery(aabb3f box, f32 dtime, CollisionFace face, f32 velocity, InvertedIndex *index, std::vector<Collision> *collisions=nullptr);

protected:
	CollisionQueryContext *m_context;
	f32 m_dtime;
	f32 m_endtime;
	bool m_context_owner;
};
