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

#include "test.h"

#include "collision.h"
#include "log.h"
#include "mockgame.h"

class TestCollision : public TestBase {
public:
	TestCollision() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestCollision"; }

	void runTests(IGameDef *gamedef);

	void testAxisAlignedCollision();
	void testCollisionMoveSimple_collide();
	void testCollisionMoveSimple_stepup();
	void testCollisionMoveSimple_ceiling();
	void testCollisionMoveSimple_shortstepup();
	void testCollisionMoveSimple_giantstepup();
	void testCollisionMoveSimple_giantceiling();
	void testCollisionMoveSimple_stepuproom();
	void testCollisionMoveSimple_longclimb();
	void testCollisionMoveSimple_bouncy();
	void testCollisionMoveSimple_corner();
};

static TestCollision g_test_instance;

void TestCollision::runTests(IGameDef *gamedef)
{
	TEST(testAxisAlignedCollision);
	TEST(testCollisionMoveSimple_collide);
	TEST(testCollisionMoveSimple_stepup);
	TEST(testCollisionMoveSimple_ceiling);
	TEST(testCollisionMoveSimple_shortstepup);
	TEST(testCollisionMoveSimple_giantstepup);
	TEST(testCollisionMoveSimple_giantceiling);
	TEST(testCollisionMoveSimple_stepuproom);
	TEST(testCollisionMoveSimple_longclimb);
	TEST(testCollisionMoveSimple_bouncy);
	TEST(testCollisionMoveSimple_corner);
}

////////////////////////////////////////////////////////////////////////////////

void TestCollision::testAxisAlignedCollision()
{
	for (s16 bx = -3; bx <= 3; bx++)
	for (s16 by = -3; by <= 3; by++)
	for (s16 bz = -3; bz <= 3; bz++) {
		// X-
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx-2, by, bz, bx-1, by+1, bz+1);
			v3f v(1, 0, 0);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 1.000) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx-2, by, bz, bx-1, by+1, bz+1);
			v3f v(-1, 0, 0);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == -1);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx-2, by+1.5, bz, bx-1, by+2.5, bz-1);
			v3f v(1, 0, 0);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == -1);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx-2, by-1.5, bz, bx-1.5, by+0.5, bz+1);
			v3f v(0.5, 0.1, 0);
			f32 dtime = 3.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 3.000) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx-2, by-1.5, bz, bx-1.5, by+0.5, bz+1);
			v3f v(0.5, 0.1, 0);
			f32 dtime = 3.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 3.000) < 0.001);
		}

		// X+
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx+2, by, bz, bx+3, by+1, bz+1);
			v3f v(-1, 0, 0);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 1.000) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx+2, by, bz, bx+3, by+1, bz+1);
			v3f v(1, 0, 0);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == -1);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx+2, by, bz+1.5, bx+3, by+1, bz+3.5);
			v3f v(-1, 0, 0);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == -1);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx+2, by-1.5, bz, bx+2.5, by-0.5, bz+1);
			v3f v(-0.5, 0.2, 0);
			f32 dtime = 2.5f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 1);  // Y, not X!
			UASSERT(fabs(dtime - 2.500) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+1, by+1, bz+1);
			aabb3f m(bx+2, by-1.5, bz, bx+2.5, by-0.5, bz+1);
			v3f v(-0.5, 0.3, 0);
			f32 dtime = 2.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 2.000) < 0.001);
		}

		// TODO: Y-, Y+, Z-, Z+

		// misc
		{
			aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
			aabb3f m(bx+2.3, by+2.29, bz+2.29, bx+4.2, by+4.2, bz+4.2);
			v3f v(-1./3, -1./3, -1./3);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 0.9) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
			aabb3f m(bx+2.29, by+2.3, bz+2.29, bx+4.2, by+4.2, bz+4.2);
			v3f v(-1./3, -1./3, -1./3);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 1);
			UASSERT(fabs(dtime - 0.9) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
			aabb3f m(bx+2.29, by+2.29, bz+2.3, bx+4.2, by+4.2, bz+4.2);
			v3f v(-1./3, -1./3, -1./3);
			f32 dtime = 1.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 2);
			UASSERT(fabs(dtime - 0.9) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
			aabb3f m(bx-4.2, by-4.2, bz-4.2, bx-2.3, by-2.29, bz-2.29);
			v3f v(1./7, 1./7, 1./7);
			f32 dtime = 17.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 0);
			UASSERT(fabs(dtime - 16.1) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
			aabb3f m(bx-4.2, by-4.2, bz-4.2, bx-2.29, by-2.3, bz-2.29);
			v3f v(1./7, 1./7, 1./7);
			f32 dtime = 17.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 1);
			UASSERT(fabs(dtime - 16.1) < 0.001);
		}
		{
			aabb3f s(bx, by, bz, bx+2, by+2, bz+2);
			aabb3f m(bx-4.2, by-4.2, bz-4.2, bx-2.29, by-2.29, bz-2.3);
			v3f v(1./7, 1./7, 1./7);
			f32 dtime = 17.0f;
			UASSERT(axisAlignedCollision(s, m, v, &dtime) == 2);
			UASSERT(fabs(dtime - 16.1) < 0.001);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void TestCollision::testCollisionMoveSimple_collide()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);

	// Set up test
	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(3.f * BS, 0.f * BS, 0.f * BS);
	
	// Run simple test
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		BS*0.25, box_0,
		0.0f, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	UASSERT(res.collides);
	UASSERT(pos_f.equals(v3f(11.f * BS, 10.f * BS, 10.f * BS), 0.01));
	UASSERT(speed_f.equals(v3f(), 0.01));
}

void TestCollision::testCollisionMoveSimple_stepup()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);

	// Set up test
	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 0.f * BS);
	
	// Run simple test
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		1.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	UASSERT(!res.collides);
	UASSERT(pos_f.equals(v3f(12.f * BS, 11.f * BS, 10.f * BS), 0.01));
	UASSERT(speed_f.equals(v3f(4.f, 0.f, 0.f) * BS, 0.01));
}

void TestCollision::testCollisionMoveSimple_ceiling()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);
	map.setMockNode(v3s16(11, 11, 10), solid);

	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 0.f * BS);
	
	// 1. stepbox is used to detect collisions with the ceiling when
	// stepping up, but it is computed at time dtime instead of 
	// nearest_dtime, so is checking in the wrong location.
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		1.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	EXPECT_FAIL(UASSERT(res.collides));
	EXPECT_FAIL(UASSERT(pos_f.equals(v3f(11.f * BS, 10.f * BS, 10.f * BS), 0.01)));
	EXPECT_FAIL(UASSERT(speed_f.equals(v3f(), 0.01)));
}

void TestCollision::testCollisionMoveSimple_giantstepup()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);
	map.setMockNode(v3s16(12, 11, 10), solid);
	map.setMockNode(v3s16(12, 12, 10), solid);

	// Set up test
	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 0.f * BS);
	
	// Run simple test
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		3.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	EXPECT_FAIL(UASSERT(!res.collides));
	EXPECT_FAIL(UASSERT(pos_f.equals(v3f(12.f * BS, 13.f * BS, 10.f * BS), 0.01)));
	EXPECT_FAIL(UASSERT(speed_f.equals(v3f(4.f, 0.f, 0.f) * BS, 0.01)));
}

void TestCollision::testCollisionMoveSimple_giantceiling()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);
	map.setMockNode(v3s16(12, 11, 10), solid);
	map.setMockNode(v3s16(12, 12, 10), solid);
	map.setMockNode(v3s16(11, 13, 10), solid);

	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 0.f * BS);
	
	// 5. The bounding box used to select nodes for potential collisions 
	// may not be large enough if there are step ups.
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		3.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	UASSERT(res.collides);
	UASSERT(pos_f.equals(v3f(11.f * BS, 10.f * BS, 10.f * BS), 0.01));
	UASSERT(speed_f.equals(v3f(), 0.01));
}

void TestCollision::testCollisionMoveSimple_stepuproom()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);
	map.setMockNode(v3s16(12, 11, 10), solid);

	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 0.f * BS);
	
	// 2. Even if stepbox were in the location of the collision, 
	// it would only check adequate clearance at the step up, 
	// not enough clearance to actually go on top of the collision box.
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		1.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	UASSERT(res.collides);
	UASSERT(pos_f.equals(v3f(11.f * BS, 10.f * BS, 10.f * BS), 0.01));
	UASSERT(speed_f.equals(v3f(), 0.01));
}

void TestCollision::testCollisionMoveSimple_longclimb()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);
	map.setMockNode(v3s16(13, 11, 10), solid);
	map.setMockNode(v3s16(14, 12, 10), solid);
	map.setMockNode(v3s16(14, 13, 10), solid);
	map.setMockNode(v3s16(15, 13, 10), solid);
	map.setMockNode(v3s16(16, 14, 10), solid);

	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(12.f * BS, 0.f * BS, 0.f * BS);
	
	// 3. After a step up is identified, the search for additional
	// collisions, possibly with step ups, continues along the original 
	// trajectory instead of the stepped up trajectory. That is, 
	// the entity travels through solid nodes and only remembers that
	// when it reaches dtime it should jump up.
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		1.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	EXPECT_FAIL(UASSERT(res.collides));
	EXPECT_FAIL(UASSERT(pos_f.equals(v3f(13.f * BS, 12.f * BS, 10.f * BS), 0.01)));
	EXPECT_FAIL(UASSERT(speed_f.equals(v3f(), 0.01)));
}

void TestCollision::testCollisionMoveSimple_shortstepup()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), solid);

	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.5f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 0.f * BS);
	
	// 7. If the entity is higher than the bottom of the collision box, 
	// it can step onto the collision box with a smaller step than 
	// the full height of the collision box. However, the code steps it up 
	// the full amount anyway.
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		1.1f*BS, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	UASSERT(!res.collides);
	UASSERT(pos_f.equals(v3f(12.f * BS, 11.f * BS, 10.f * BS), 0.01));
	UASSERT(speed_f.equals(v3f(4.0f, 0.f, 0.f)*BS, 0.01));
}

void TestCollision::testCollisionMoveSimple_bouncy()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
        cf.name = "bouncy";
        cf.walkable = true;
        cf.groups["bouncy"] = 100;
	content_t bouncy = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 10), bouncy);
	map.setMockNode(v3s16(7, 10, 10), solid);

	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(10.f * BS, 0.f * BS, 0.f * BS);
	
	// 5. The bounding box used to select nodes for potential collisions 
	// may not be large enough if there are bounces.
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		0.25*BS, box_0,
		0.f, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	EXPECT_FAIL(UASSERT(!res.collides));
	EXPECT_FAIL(UASSERT(pos_f.equals(v3f(8.f * BS, 10.f * BS, 10.f * BS), 0.01)));
	EXPECT_FAIL(UASSERT(speed_f.equals(v3f(), 0.01)));
}

void TestCollision::testCollisionMoveSimple_corner()
{
	// Create test environment
	MockEnvironment env(std::cerr);
	ContentFeatures cf;
	cf.name = "solid";
	cf.walkable = true;
	content_t solid = env.getMockGameDef()->registerContent(cf.name, cf);
	MockMap &map = env.getMockMap();
	
	map.setMockNode(v3s16(12, 10, 12), solid);

	// Set up test
	aabb3f box_0(-.5f * BS, -.5f * BS, -.5f * BS, .5f * BS, .5f * BS, .5f * BS);
	v3f pos_f(10.f * BS, 10.f * BS, 10.f * BS);
	v3f speed_f(4.f * BS, 0.f * BS, 4.f * BS);
	
	// Run simple test
	collisionMoveResult res = collisionMoveSimple(&env, env.getGameDef(),
		BS*0.25, box_0,
		0.0f, 0.5f,
		&pos_f, &speed_f,
		v3f(), nullptr,
		false);
	rawstream << "Now at (" << pos_f.X << ',' << pos_f.Y << ',' << pos_f.Z << ')' << std::endl;
	EXPECT_FAIL(UASSERT(res.collides));
	EXPECT_FAIL(UASSERT(pos_f.equals(v3f(11.f * BS, 10.f * BS, 11.f * BS), 0.01)));
	EXPECT_FAIL(UASSERT(speed_f.equals(v3f(), 0.01)));
}
