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

#include <cstring>

#include "test.h"

#include "util/collisionquery.h"
#include "log.h"

class TestCollisionQuery : public TestBase {
public:
	TestCollisionQuery() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestCollisionQuery"; }

	void runTests(IGameDef *gamedef);

	void testCollisionQueryContext();

protected:
	static const std::vector<u32> cases[];
};

static TestCollisionQuery g_test_instance;

void TestCollisionQuery::runTests(IGameDef *gamedef)
{
	TEST(testCollisionQueryContext);
}

////////////////////////////////////////////////////////////////////////////////

void TestCollisionQuery::testCollisionQueryContext()
{
	InvertedIndex index;

	index.index(aabb3f(-.5f, -.5f, -.5f, .5f, .5f, .5f));
	index.index(aabb3f(0.f, -.5f, .5f, 1.5f, 2.5f, .65f));
	index.index(aabb3f(.3f, .5f, -.5f, .5f, 1.5f, .6f));
	index.index(aabb3f(.0f, .0f, .0f, 2.5f, .5f, 1.5f));
	index.index(aabb3f(17.f, -.5f, -3.5f, 18.f, .5f, -1.5f));

	std::vector<Collision> collisions;
	CollisionQueryContext ctx(75, aabb3f(0.2f, 1.0f, -.5f, 1.f, 2.5f, .3f), &index, &collisions);
	// Collides with box 2 only
	u16 MX = CollisionQueryContext::testBitmask[COLLISION_FACE_X];
	u16 MY = CollisionQueryContext::testBitmask[COLLISION_FACE_Y];
	u16 MZ = CollisionQueryContext::testBitmask[COLLISION_FACE_Z];
	u16 MinX = CollisionQueryContext::testBitmask[COLLISION_FACE_MIN_X] | MX;
	u16 MinY = CollisionQueryContext::testBitmask[COLLISION_FACE_MIN_Y] | MY;
	u16 MinZ = CollisionQueryContext::testBitmask[COLLISION_FACE_MIN_Z] | MZ;
	u16 MaxX = CollisionQueryContext::testBitmask[COLLISION_FACE_MAX_X] | MX;
	u16 MaxY = CollisionQueryContext::testBitmask[COLLISION_FACE_MAX_Y] | MY;
	u16 MaxZ = CollisionQueryContext::testBitmask[COLLISION_FACE_MAX_Z] | MZ;
	f32 offset[6];
	UASSERTEQ(u16, ctx.getValidFaces(0, offset), MinX|MaxX|MinZ|MaxZ);
	UASSERTEQ(u16, ctx.getValidFaces(1, offset), MinX|MaxX|MinY|MaxY);
	UASSERTEQ(u16, ctx.getValidFaces(2, offset), MinX|MaxX|MinY|MaxY|MinZ|MaxZ);
	UASSERTEQ(u16, ctx.getValidFaces(3, offset), MinX|MaxX|MinZ|MaxZ);
	UASSERTEQ(u16, ctx.getValidFaces(4, offset), 0);
	UASSERTEQ(u32, collisions.size(), 3);
	UASSERTEQ(u32, collisions.at(0).context_id, 75);
	UASSERTEQ(u32, collisions.at(0).face, COLLISION_FACE_MAX_X);
	UASSERTEQ(u32, collisions.at(0).id, 2);
	UASSERT(irr::core::equals(collisions.at(0).offset, 0.7f));
	UASSERTEQ(f32, collisions.at(0).dtime, 0);
	UASSERTEQ(u32, collisions.at(1).context_id, 75);
	UASSERTEQ(u32, collisions.at(1).face, COLLISION_FACE_MAX_Y);
	UASSERTEQ(u32, collisions.at(1).id, 2);
	UASSERT(irr::core::equals(collisions.at(1).offset, 2.f));
	UASSERTEQ(f32, collisions.at(1).dtime, 0);
	UASSERTEQ(u32, collisions.at(2).context_id, 75);
	UASSERTEQ(u32, collisions.at(2).face, COLLISION_FACE_MIN_Z);
	UASSERTEQ(u32, collisions.at(2).id, 2);
	UASSERT(irr::core::equals(collisions.at(2).offset, 1.1f));
	UASSERTEQ(f32, collisions.at(2).dtime, 0);
}

const std::vector<u32> TestCollisionQuery::cases[] = {
		std::vector<u32>{10, 20, 30, 40, 50, 60, 70, 80, 90, 100},
		std::vector<u32>{4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40},
		std::vector<u32>{6, 9, 12, 15, 18, 21, 24, 27, 30},
		std::vector<u32>{10, 15, 20, 25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75, 80, 85, 90, 95, 100},
		std::vector<u32>{14, 21, 28, 35, 42, 49, 56, 63, 70},
		std::vector<u32>{22, 33, 44, 55, 66, 77, 88, 99},
		std::vector<u32>{26, 39},
		std::vector<u32>{2, 3, 5, 7, 11, 13, 17, 19, 23},
		std::vector<u32>{2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30},
	};
