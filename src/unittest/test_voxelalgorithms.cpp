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

	void testPropogateSunlight(INodeDefManager *ndef);
	void testClearLightAndCollectSources(INodeDefManager *ndef);
	void testVoxelLineIterator(INodeDefManager *ndef);
};

static TestVoxelAlgorithms g_test_instance;

void TestVoxelAlgorithms::runTests(IGameDef *gamedef)
{
	INodeDefManager *ndef = gamedef->getNodeDefManager();

	TEST(testPropogateSunlight, ndef);
	TEST(testClearLightAndCollectSources, ndef);
	TEST(testVoxelLineIterator, ndef);
}

////////////////////////////////////////////////////////////////////////////////

void TestVoxelAlgorithms::testPropogateSunlight(INodeDefManager *ndef)
{
	VoxelManipulator v;

	for (u16 z = 0; z < 3; z++)
	for (u16 y = 0; y < 3; y++)
	for (u16 x = 0; x < 3; x++) {
		v3s16 p(x,y,z);
		v.setNodeNoRef(p, MapNode(CONTENT_AIR));
	}

	VoxelArea a(v3s16(0,0,0), v3s16(2,2,2));

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, true, light_sources, ndef);
		//v.print(dstream, ndef, VOXELPRINT_LIGHT_DAY);
		UASSERT(res.bottom_sunlight_valid == true);
		UASSERT(v.getNode(v3s16(1,1,1)).getLight(LIGHTBANK_DAY, ndef)
				== LIGHT_SUN);
	}

	v.setNodeNoRef(v3s16(0,0,0), MapNode(t_CONTENT_STONE));

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, true, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
		UASSERT(v.getNode(v3s16(1,1,1)).getLight(LIGHTBANK_DAY, ndef)
				== LIGHT_SUN);
	}

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, false, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
		UASSERT(v.getNode(v3s16(2,0,2)).getLight(LIGHTBANK_DAY, ndef)
				== 0);
	}

	v.setNodeNoRef(v3s16(1,3,2), MapNode(t_CONTENT_STONE));

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, true, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
		UASSERT(v.getNode(v3s16(1,1,2)).getLight(LIGHTBANK_DAY, ndef)
				== 0);
	}

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, false, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
		UASSERT(v.getNode(v3s16(1,0,2)).getLight(LIGHTBANK_DAY, ndef)
				== 0);
	}

	{
		MapNode n(CONTENT_AIR);
		n.setLight(LIGHTBANK_DAY, 10, ndef);
		v.setNodeNoRef(v3s16(1,-1,2), n);
	}

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, true, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
	}

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, false, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
	}

	{
		MapNode n(CONTENT_AIR);
		n.setLight(LIGHTBANK_DAY, LIGHT_SUN, ndef);
		v.setNodeNoRef(v3s16(1,-1,2), n);
	}

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, true, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == false);
	}

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, false, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == false);
	}

	v.setNodeNoRef(v3s16(1,3,2), MapNode(CONTENT_IGNORE));

	{
		std::set<v3s16> light_sources;
		voxalgo::setLight(v, a, 0, ndef);
		voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
				v, a, true, light_sources, ndef);
		UASSERT(res.bottom_sunlight_valid == true);
	}
}

void TestVoxelAlgorithms::testClearLightAndCollectSources(INodeDefManager *ndef)
{
	VoxelManipulator v;

	for (u16 z = 0; z < 3; z++)
	for (u16 y = 0; y < 3; y++)
	for (u16 x = 0; x < 3; x++) {
		v3s16 p(x,y,z);
		v.setNode(p, MapNode(CONTENT_AIR));
	}

	VoxelArea a(v3s16(0,0,0), v3s16(2,2,2));
	v.setNodeNoRef(v3s16(0,0,0), MapNode(t_CONTENT_STONE));
	v.setNodeNoRef(v3s16(1,1,1), MapNode(t_CONTENT_TORCH));

	{
		MapNode n(CONTENT_AIR);
		n.setLight(LIGHTBANK_DAY, 1, ndef);
		v.setNode(v3s16(1,1,2), n);
	}

	{
		std::set<v3s16> light_sources;
		std::map<v3s16, u8> unlight_from;
		voxalgo::clearLightAndCollectSources(v, a, LIGHTBANK_DAY,
				ndef, light_sources, unlight_from);
		//v.print(dstream, ndef, VOXELPRINT_LIGHT_DAY);
		UASSERT(v.getNode(v3s16(0,1,1)).getLight(LIGHTBANK_DAY, ndef) == 0);
		UASSERT(light_sources.find(v3s16(1,1,1)) != light_sources.end());
		UASSERT(light_sources.size() == 1);
		UASSERT(unlight_from.find(v3s16(1,1,2)) != unlight_from.end());
		UASSERT(unlight_from.size() == 1);
	}
}

void TestVoxelAlgorithms::testVoxelLineIterator(INodeDefManager *ndef)
{
	// Test some lines
	// Do not test lines that start or end on the border of
	// two voxels as rounding errors can make the test fail!
	std::vector<core::line3d<f32> > lines;
	for (f32 x = -9.1; x < 9; x += 3.124) {
	for (f32 y = -9.2; y < 9; y += 3.123) {
	for (f32 z = -9.3; z < 9; z += 3.122) {
		lines.push_back(core::line3d<f32>(-x, -y, -z, x, y, z));
	}
	}
	}
	lines.push_back(core::line3d<f32>(0, 0, 0, 0, 0, 0));
	// Test every line
	std::vector<core::line3d<f32> >::iterator it = lines.begin();
	for (; it < lines.end(); it++) {
		core::line3d<f32> l = *it;

		// Initialize test
		voxalgo::VoxelLineIterator iterator(l.start, l.getVector());

		//Test the first voxel
		v3s16 start_voxel = floatToInt(l.start, 1);
		UASSERT(iterator.m_current_node_pos == start_voxel);

		// Values for testing
		v3s16 end_voxel = floatToInt(l.end, 1);
		v3s16 voxel_vector = end_voxel - start_voxel;
		int nodecount = abs(voxel_vector.X) + abs(voxel_vector.Y)
			+ abs(voxel_vector.Z);
		int actual_nodecount = 0;
		v3s16 old_voxel = iterator.m_current_node_pos;

		while (iterator.hasNext()) {
			iterator.next();
			actual_nodecount++;
			v3s16 new_voxel = iterator.m_current_node_pos;
			// This must be a neighbor of the old voxel
			UASSERTEQ(f32, (new_voxel - old_voxel).getLengthSQ(), 1);
			// The line must intersect with the voxel
			v3f voxel_center = intToFloat(iterator.m_current_node_pos, 1);
			aabb3f box(voxel_center - v3f(0.5, 0.5, 0.5),
				voxel_center + v3f(0.5, 0.5, 0.5));
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
