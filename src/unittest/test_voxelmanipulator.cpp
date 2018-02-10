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

#include <algorithm>

#include "gamedef.h"
#include "log.h"
#include "voxel.h"

class TestVoxelManipulator : public TestBase {
public:
	TestVoxelManipulator() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestVoxelManipulator"; }

	void runTests(IGameDef *gamedef);

	void testVoxelArea();
	void testVoxelManipulator(const NodeDefManager *nodedef);
};

static TestVoxelManipulator g_test_instance;

void TestVoxelManipulator::runTests(IGameDef *gamedef)
{
	TEST(testVoxelArea);
	TEST(testVoxelManipulator, gamedef->getNodeDefManager());
}

////////////////////////////////////////////////////////////////////////////////

void TestVoxelManipulator::testVoxelArea()
{
	VoxelArea a(v3s16(-1,-1,-1), v3s16(1,1,1));
	UASSERT(a.index(0,0,0) == 1*3*3 + 1*3 + 1);
	UASSERT(a.index(-1,-1,-1) == 0);

	VoxelArea c(v3s16(-2,-2,-2), v3s16(2,2,2));
	// An area that is 1 bigger in x+ and z-
	VoxelArea d(v3s16(-2,-2,-3), v3s16(3,2,2));

	std::list<VoxelArea> aa;
	d.diff(c, aa);

	// Correct results
	std::vector<VoxelArea> results;
	results.emplace_back(v3s16(-2,-2,-3), v3s16(3,2,-3));
	results.emplace_back(v3s16(3,-2,-2), v3s16(3,2,2));

	UASSERT(aa.size() == results.size());

	infostream<<"Result of diff:"<<std::endl;
	for (std::list<VoxelArea>::const_iterator
			it = aa.begin(); it != aa.end(); ++it) {
		it->print(infostream);
		infostream << std::endl;

		std::vector<VoxelArea>::iterator j;
		j = std::find(results.begin(), results.end(), *it);
		UASSERT(j != results.end());
		results.erase(j);
	}
}


void TestVoxelManipulator::testVoxelManipulator(const NodeDefManager *nodedef)
{
	VoxelManipulator v;

	v.print(infostream, nodedef);

	infostream << "*** Setting (-1,0,-1)=2 ***" << std::endl;
	v.setNodeNoRef(v3s16(-1,0,-1), MapNode(t_CONTENT_GRASS));

	v.print(infostream, nodedef);
	UASSERT(v.getNode(v3s16(-1,0,-1)).getContent() == t_CONTENT_GRASS);

	infostream << "*** Reading from inexistent (0,0,-1) ***" << std::endl;

	EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,0,-1)));
	v.print(infostream, nodedef);

	infostream << "*** Adding area ***" << std::endl;

	VoxelArea a(v3s16(-1,-1,-1), v3s16(1,1,1));
	v.addArea(a);
	v.print(infostream, nodedef);

	UASSERT(v.getNode(v3s16(-1,0,-1)).getContent() == t_CONTENT_GRASS);
	EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,1,1)));
}
