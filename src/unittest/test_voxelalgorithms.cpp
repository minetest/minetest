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

#include "gamedef.h"
#include "voxelalgorithms.h"
#include "util/numeric.h"

class TestVoxelAlgorithms : public TestBase {
public:
	TestVoxelAlgorithms() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestVoxelAlgorithms"; }

	void runTests(IGameDef *gamedef);

	void testVoxelLineIterator(const NodeDefManager *ndef);
};

static TestVoxelAlgorithms g_test_instance;

void TestVoxelAlgorithms::runTests(IGameDef *gamedef)
{
	const NodeDefManager *ndef = gamedef->getNodeDefManager();

	TEST(testVoxelLineIterator, ndef);
}

////////////////////////////////////////////////////////////////////////////////

void TestVoxelAlgorithms::testVoxelLineIterator(const NodeDefManager *ndef)
{
	// Test some lines
	// Do not test lines that start or end on the border of
	// two voxels as rounding errors can make the test fail!
	std::vector<core::line3d<opos_t> > lines;
	for (opos_t x = -9.1; x < 9; x += 3.124) {
	for (opos_t y = -9.2; y < 9; y += 3.123) {
	for (opos_t z = -9.3; z < 9; z += 3.122) {
		lines.emplace_back(-x, -y, -z, x, y, z);
	}
	}
	}
	lines.emplace_back(0, 0, 0, 0, 0, 0);
	// Test every line
	std::vector<core::line3d<opos_t> >::iterator it = lines.begin();
	for (; it < lines.end(); it++) {
		core::line3d<opos_t> l = *it;

		// Initialize test
		voxalgo::VoxelLineIterator iterator(l.start, oposToV3f(l.getVector()));

		//Test the first voxel
		v3pos_t start_voxel = floatToInt(l.start, 1);
		UASSERT(iterator.m_current_node_pos == start_voxel);

		// Values for testing
		v3pos_t end_voxel = floatToInt(l.end, 1);
		v3pos_t voxel_vector = end_voxel - start_voxel;
		int nodecount = abs(voxel_vector.X) + abs(voxel_vector.Y)
			+ abs(voxel_vector.Z);
		int actual_nodecount = 0;
		v3pos_t old_voxel = iterator.m_current_node_pos;

		while (iterator.hasNext()) {
			iterator.next();
			actual_nodecount++;
			v3pos_t new_voxel = iterator.m_current_node_pos;
			// This must be a neighbor of the old voxel
			UASSERTEQ(f32, (new_voxel - old_voxel).getLengthSQ(), 1);
			// The line must intersect with the voxel
			v3opos_t voxel_center = posToOpos(iterator.m_current_node_pos, (f32)1);
			aabb3o box(voxel_center - v3opos_t(0.5, 0.5, 0.5),
				voxel_center + v3opos_t(0.5, 0.5, 0.5));
			UASSERT(box.intersectsWithLine(l));
			// Update old voxel
			old_voxel = new_voxel;
		}

		// Test last node
		UASSERT(iterator.m_current_node_pos == end_voxel);
		// Test node count
		UASSERTEQ(int, actual_nodecount, nodecount);
	}
}
