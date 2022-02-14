/*
Minetest

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

#include <cstdio>
#include "mapblock.h"

class TestMap : public TestBase
{
public:
	TestMap() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestMap"; }

	void runTests(IGameDef *gamedef);

	void testMaxMapgenLimit();
};

static TestMap g_test_instance;

void TestMap::runTests(IGameDef *gamedef)
{
	TEST(testMaxMapgenLimit);
}

////////////////////////////////////////////////////////////////////////////////

void TestMap::testMaxMapgenLimit()
{
	// limit must end on a mapblock boundary
	UASSERTEQ(int, MAX_MAP_GENERATION_LIMIT % MAP_BLOCKSIZE, MAP_BLOCKSIZE - 1);

	// objectpos_over_limit should do exactly this except the last node
	// actually spans from LIMIT-0.5 to LIMIT+0.5
	float limit_times_bs = MAX_MAP_GENERATION_LIMIT * BS;
	UASSERT(objectpos_over_limit(v3f(limit_times_bs-BS/2)) == false);
	UASSERT(objectpos_over_limit(v3f(limit_times_bs)) == false);
	UASSERT(objectpos_over_limit(v3f(limit_times_bs+BS/2)) == false);
	UASSERT(objectpos_over_limit(v3f(limit_times_bs+BS)) == true);

	UASSERT(objectpos_over_limit(v3f(-limit_times_bs+BS/2)) == false);
	UASSERT(objectpos_over_limit(v3f(-limit_times_bs)) == false);
	UASSERT(objectpos_over_limit(v3f(-limit_times_bs-BS/2)) == false);
	UASSERT(objectpos_over_limit(v3f(-limit_times_bs-BS)) == true);

	// blockpos_over_max_limit
	s16 limit_block = MAX_MAP_GENERATION_LIMIT / MAP_BLOCKSIZE;
	UASSERT(blockpos_over_max_limit(v3s16(limit_block)) == false);
	UASSERT(blockpos_over_max_limit(v3s16(limit_block+1)) == true);
	UASSERT(blockpos_over_max_limit(v3s16(-limit_block)) == false);
	UASSERT(blockpos_over_max_limit(v3s16(-limit_block-1)) == true);
}
