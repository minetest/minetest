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
#include "mockgame.h"

class TestCollision : public TestBase {
public:
	TestCollision() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestCollision"; }

	void runTests(IGameDef *gamedef);

	void testAxisAlignedCollision();
	void testCollisionMoveSimple();
};

static TestCollision g_test_instance;

void TestCollision::runTests(IGameDef *gamedef)
{
	TEST(testAxisAlignedCollision);
	TEST(testCollisionMoveSimple);
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

void setDef(std::string &definition, s16 x, s16 y, s16 z, char c)
{
	s16 b = y / MAP_BLOCKSIZE;
        y -= b * MAP_BLOCKSIZE;
	definition[b * MapBlock::nodecount + z * MapBlock::zstride + y * MapBlock::ystride + x] = c;
}

void TestCollision::testCollisionMoveSimple()
{
	std::string definition(MapBlock::nodecount, ' ');
        setDef(definition, 12, 10, 10, 'S');
        setDef(definition, 13, 10, 10, 'S');
        setDef(definition, 11, 11, 10, 'S');
        setDef(definition, 12, 11, 10, 'S');
        setDef(definition, 13, 11, 10, 'S');
	MockEnvironment env(std::cerr);
	env.getMockMap().CreateSector(v2s16(0,0), definition);
	aabb3f box_0(9.5f, 9.5f, 9.5f, 10.5f, 10.5f, 10.5f);
	v3f pos_f(10.f, 10.f, 10.f);
	v3f speed_f(3.f, 0.f, 0.f);
	
//	UASSERT(collisionMoveSimple(&env, env.getGameDef(),
//                BS*0.25, box_0,
//                0.0f, 0.5f,
//                &pos_f, &speed_f,
//                v3f(), NULL,
//                false).collides);
//	UASSERT(pos_f.equals(v3f(11.f, 10.f, 10.f), 0.01));
//	UASSERT(speed_f.equals(v3f(), 0.01));
}
