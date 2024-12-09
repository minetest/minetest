// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

#include "test.h"
#include "catch.h"
#include "collision.h"
#include "dummymap.h"
#include "environment.h"
#include "irrlicht_changes/printing.h"
#include "irrlichttypes.h"


const double EPSILON = 0.001;

class TestCollision : public TestBase {
public:
	TestCollision() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestCollision"; }

	void runTests(IGameDef *gamedef);

	void testCollisionMoveSimple(IGameDef *gamedef);
};

static TestCollision g_test_instance;

void TestCollision::runTests(IGameDef *gamedef)
{
	TEST(testCollisionMoveSimple, gamedef);
}

namespace {
	class TestEnvironment : public Environment {
		DummyMap map;
	public:
		TestEnvironment(IGameDef *gamedef)
			: Environment(gamedef), map(gamedef, {-1, -1, -1}, {1, 1, 1})
		{
			map.fill({-1, -1, -1}, {1, 1, 1}, MapNode(CONTENT_AIR));
		}

		void step(f32 dtime) override {}

		Map &getMap() override { return map; }

		void getSelectedActiveObjects(const core::line3d<f32> &shootline_on_map,
			std::vector<PointedThing> &objects,
			const std::optional<Pointabilities> &pointabilities) override {}
	};
}

#define UASSERTEQ_F(actual, expected) do { \
		f32 a = (actual); \
		f32 e = (expected); \
		UTEST(fabsf(a - e) <= 0.0001f, "actual: %.f expected: %.f", a, e) \
	} while (0)

#define UASSERTEQ_V3F(actual, expected) do { \
		v3f va = (actual); \
		v3f ve = (expected); \
		UASSERTEQ_F(va.X, ve.X); UASSERTEQ_F(va.Y, ve.Y); UASSERTEQ_F(va.Z, ve.Z); \
	} while (0)

#define fpos(x,y,z) (BS * v3f(x, y, z))


////////////////////////////////////////////////////////////////////////////////
TEST_CASE("Test axis aligned collision with unit cube.", "[collision]")
{
	f32 bx = GENERATE(-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f);
	f32 by = GENERATE(-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f);
	f32 bz = GENERATE(-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f);

	INFO("Testing with static cube at (" << bx << ", " << by << ", " << bz << ").");

	aabb3f s{bx, by, bz, bx+1.0f, by+1.0f, bz+1.0f};

	// The following set of tests is for boxes translated in the -X direction
	// from the static cube, possibly with additional offsets.
	SECTION("Given a unit cube translated by -2 units on the x-axis, "
			"when it moves 1 unit per step in the +x direction for 1 step, "
			"then it should collide on the X axis within epsilon of 1 step.")
	{
		aabb3f m{bx-2.0f, by, bz, bx-1.0f, by+1.0f, bz+1.0f};
		f32 dtime = 1.0f;
		CHECK(0 == axisAlignedCollision(s, m, v3f{1.0f, 0.0f, 0.0f}, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 1.0) < EPSILON);
	}

	SECTION("Given a unit cube translated by -2 units on the x-axis, "
			"when it moves 1 unit per step in the -x direction for 1 step, "
			"then it should never collide.")
	{
		aabb3f m{bx-2.0f, by, bz, bx-1.0f, by+1.0f, bz+1.0f};
		f32 dtime = 1.0f;
		CHECK(-1 == axisAlignedCollision(s, m, v3f{-1.0f, 0.0f, 0.0f}, &dtime));
	}


	SECTION("Given a unit cube translated by -2 units on the x-axis "
			"and 1.5 units on the y-axis, "
			"when it moves 1 unit per step in the +x direction for 1 step, "
			"then it should never collide.")
	{
		aabb3f m{bx-2.0f, by+1.5f, bz, bx-1.0f, by+2.5f, bz+1.0f};
		f32 dtime = 1.0f;
		CHECK(-1 == axisAlignedCollision(s, m, v3f{1.0f, 0.0f, 0.0f}, &dtime));
	}

	SECTION("Given a 0.5x2x1 cube translated by -2 units on the x-axis "
			"and -1.5 units on the y-axis, "
			"when it moves 0.5 units per step in the +x direction "
			"and 0.1 units per step in the +y direction for 3 steps, "
			"then it should collide on the X axis within epsilon of 3 steps.")
	{
		aabb3f m{bx-2.0f, by-1.5f, bz, bx-1.5f, by+0.5f, bz+1.0f};
		f32 dtime = 3.0f;
		CHECK(0 == axisAlignedCollision(s, m, v3f{0.5f, 0.1f, 0}, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 3.0) < EPSILON);
	}

	// The following set of tests is for boxes translated in the +X direction
	// from the static cube, possibly with additional offsets. They are not
	// all mirror images of the tests for the -X direction.

	SECTION("Given a unit cube translated by +2 units on the x-axis, "
			"when it moves 1 unit per step in the -x direction for 1 step, "
			"then it should collide on the X axis within epsilon of 1 step.")
	{
		aabb3f m{bx+2.0f, by, bz, bx+3.0f, by+1.0f, bz+1.0f};
		f32 dtime = 1.0f;
		CHECK(0 == axisAlignedCollision(s, m, v3f{-1.0f, 0.0f, 0.0f}, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 1.0) < EPSILON);
	}

	SECTION("Given a unit cube translated by +2 units on the x-axis, "
			"when it moves 1 unit per step in the +x direction for 1 step, "
			"then it should never collide.")
	{
		aabb3f m{bx+2.0f, by, bz, bx+3.0f, by+1.0f, bz+1.0f};
		f32 dtime = 1.0f;
		CHECK(-1 == axisAlignedCollision(s, m, v3f{1.0f, 0.0f, 0.0f}, &dtime));
	}

	SECTION("Given a unit cube translated by +2 units on the x-axis "
			"and 1.5 units on the z-axis, "
			"when it moves 1 unit per step in the -x direction for 1 step, "
			"then it should never collide.")
	{
		aabb3f m{bx+2.0f, by, bz+1.5f, bx+3.0f, by+1.0f, bz+3.5f};
		f32 dtime = 1.0f;
		CHECK(-1 == axisAlignedCollision(s, m, v3f{-1.0f, 0.0f, 0.0f}, &dtime));
	}

	SECTION("Given a 0.5x1x1 cube translated by +2 units on the x-axis "
			"and -1.5 units on the y-axis, "
			"when it moves 0.5 units per step in the -x direction "
			"and 0.2 units per step in the +y direction for 3 steps, "
			"then it should collide on the Y axis within epsilon of 2.5 steps.")
	{
		// This test is interesting because the Y-faces are the first to collide.
		aabb3f m{bx+2.0f, by-1.5f, bz, bx+2.5f, by-0.5f, bz+1.0f};
		f32 dtime = 2.5f;
		CHECK(1 == axisAlignedCollision(s, m, v3f{-0.5f, 0.2f, 0}, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 2.5) < EPSILON);
	}

	SECTION("Given a 0.5x1x1 cube translated by +2 units on the x-axis "
			"and -1.5 units on the y-axis, "
			"when it moves 0.5 units per step in the -x direction "
			"and 0.3 units per step in the +y direction for 3 steps, "
			"then it should collide on the X axis within epsilon of 2.0 steps.")
	{
		aabb3f m{bx+2.0f, by-1.5f, bz, bx+2.5f, by-0.5f, bz+1.0f};
		f32 dtime = 2.0f;
		CHECK(0 == axisAlignedCollision(s, m, v3f{-0.5f, 0.3f, 0}, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 2.0) < EPSILON);
	}
}

TEST_CASE("Test axis aligned collision with 2x2x2 cube.", "[collision]")
{
	f32 bx = GENERATE(-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f);
	f32 by = GENERATE(-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f);
	f32 bz = GENERATE(-3.0f, -2.0f, -1.0f, 0.0f, 1.0f, 2.0f, 3.0f);

	INFO("Testing with static cube at (" << bx << ", " << by << ", " << bz << ").");

	aabb3f s{bx, by, bz, bx+2.0f, by+2.0f, bz+2.0f};

	// The following set of tests checks small floating point offsets
	// colliding at the corner of two boxes, to ensure the function under
	// test can detect which axis collided first.

	SECTION("Collides on X axis near (+X,+Y,+Z) corner.")
	{
		aabb3f m{bx+2.3f, by+2.29f, bz+2.29f, bx+4.2f, by+4.2f, bz+4.2f};
		v3f v{-1.0f/3.0f, -1.0f/3.0f, -1.0/3.0f};
		f32 dtime = 1.0f;
		CHECK(0 == axisAlignedCollision(s, m, v, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 0.9) < EPSILON);
	}

	SECTION("Collides on Y axis near (+X,+Y,+Z) corner.")
	{
		aabb3f m{bx+2.29f, by+2.3f, bz+2.29f, bx+4.2f, by+4.2f, bz+4.2f};
		v3f v{-1.0f/3.0f, -1.0f/3.0f, -1.0/3.0f};
		f32 dtime = 1.0f;
		CHECK(1 == axisAlignedCollision(s, m, v, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 0.9) < EPSILON);
	}

	SECTION("Collides on Z axis near (+X,+Y,+Z) corner.")
	{
		aabb3f m{bx+2.29f, by+2.29f, bz+2.3f, bx+4.2f, by+4.2f, bz+4.2f};
		v3f v{-1.0f/3.0f, -1.0f/3.0f, -1.0/3.0f};
		f32 dtime = 1.0f;
		CHECK(2 == axisAlignedCollision(s, m, v, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 0.9) < EPSILON);
	}

	SECTION("Collides on X axis near (-X,-Y,-Z) corner.")
	{
		aabb3f m{bx-4.2f, by-4.2f, bz-4.2f, bx-2.3f, by-2.29f, bz-2.29f};
		v3f v{1.0f/7.0f, 1.0f/7.0f, 1.0/7.0f};
		f32 dtime = 17.0f;
		CHECK(0 == axisAlignedCollision(s, m, v, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 16.1) < EPSILON);
	}

	SECTION("Collides on Y axis near (-X,-Y,-Z) corner.")
	{
		aabb3f m{bx-4.2f, by-4.2f, bz-4.2f, bx-2.29f, by-2.3f, bz-2.29f};
		v3f v{1.0f/7.0f, 1.0f/7.0f, 1.0/7.0f};
		f32 dtime = 17.0f;
		CHECK(1 == axisAlignedCollision(s, m, v, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 16.1) < EPSILON);
	}

	SECTION("Collides on Z axis near (-X,-Y,-Z) corner.")
	{
		aabb3f m{bx-4.2f, by-4.2f, bz-4.2f, bx-2.29f, by-2.29f, bz-2.3f};
		v3f v{1.0f/7.0f, 1.0f/7.0f, 1.0/7.0f};
		f32 dtime = 17.0f;
		CHECK(2 == axisAlignedCollision(s, m, v, &dtime));
		CHECK(std::fabs(static_cast<double>(dtime) - 16.1) < EPSILON);
	}
}


void TestCollision::testCollisionMoveSimple(IGameDef *gamedef)
{
	auto env = std::make_unique<TestEnvironment>(gamedef);
	g_collision_problems_encountered = false;

	for (s16 x = 0; x < MAP_BLOCKSIZE; x++)
	for (s16 z = 0; z < MAP_BLOCKSIZE; z++)
		env->getMap().setNode({x, 0, z}, MapNode(t_CONTENT_STONE));

	v3f pos, speed, accel;
	const aabb3f box(fpos(-0.1f, 0, -0.1f), fpos(0.1f, 1.4f, 0.1f));
	collisionMoveResult res;

	/* simple movement with accel */
	pos   = fpos(4, 1, 4);
	speed = fpos(0, 0, 0);
	accel = fpos(0, 1, 0);
	res = collisionMoveSimple(env.get(), gamedef, box, 0.0f, 1.0f,
		&pos, &speed, accel);

	UASSERT(!res.touching_ground || !res.collides || !res.standing_on_object);
	UASSERT(res.collisions.empty());
	// FIXME: it's easy to tell that this should be y=1.5f, but our code does it wrong.
	// It's unclear if/how this will be fixed.
	UASSERTEQ_V3F(pos, fpos(4, 2, 4));
	UASSERTEQ_V3F(speed, fpos(0, 1, 0));

	/* standing on ground */
	pos   = fpos(0, 0.5f, 0);
	speed = fpos(0, 0, 0);
	accel = fpos(0, -9.81f, 0);
	res = collisionMoveSimple(env.get(), gamedef, box, 0.0f, 0.04f,
		&pos, &speed, accel);

	UASSERT(res.collides);
	UASSERT(res.touching_ground);
	UASSERT(!res.standing_on_object);
	UASSERTEQ_V3F(pos, fpos(0, 0.5f, 0));
	UASSERTEQ_V3F(speed, fpos(0, 0, 0));
	UASSERT(res.collisions.size() == 1);
	{
		auto &ci = res.collisions.front();
		UASSERTEQ(int, ci.type, COLLISION_NODE);
		UASSERTEQ(int, ci.axis, COLLISION_AXIS_Y);
		UASSERTEQ(v3s16, ci.node_p, v3s16(0, 0, 0));
	}

	/* not moving never collides */
	pos   = fpos(0, -100, 0);
	speed = fpos(0, 0, 0);
	accel = fpos(0, 0, 0);
	res = collisionMoveSimple(env.get(), gamedef, box, 0.0f, 1/60.0f,
		&pos, &speed, accel);
	UASSERT(!res.collides);

	/* collision in ignore */
	pos   = fpos(0, -100, 0);
	speed = fpos(5, 0, 0);
	accel = fpos(0, 0, 0);
	res = collisionMoveSimple(env.get(), gamedef, box, 0.0f, 1/60.0f,
		&pos, &speed, accel);
	UASSERTEQ_V3F(speed, fpos(0, 0, 0));
	UASSERT(!res.collides); // FIXME this is actually inconsistent
	UASSERT(res.collisions.empty());

	// TODO things to test:
	// standing_on_object, multiple collisions, bouncy, stepheight

	// No warnings should have been raised during our test.
	UASSERT(!g_collision_problems_encountered);
}
