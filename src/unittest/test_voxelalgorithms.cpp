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

class TestVoxelAlgorithms : public TestBase {
public:
	TestVoxelAlgorithms() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestVoxelAlgorithms"; }

	void runTests(IGameDef *gamedef);

	void testPropogateSunlight(INodeDefManager *ndef);
	void testClearLightAndCollectSources(INodeDefManager *ndef);
};

static TestVoxelAlgorithms g_test_instance;

void TestVoxelAlgorithms::runTests(IGameDef *gamedef)
{
	INodeDefManager *ndef = gamedef->getNodeDefManager();

	TEST(testPropogateSunlight, ndef);
	TEST(testClearLightAndCollectSources, ndef);
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

	v.setNodeNoRef(v3s16(0,0,0), MapNode(CONTENT_STONE));

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

	v.setNodeNoRef(v3s16(1,3,2), MapNode(CONTENT_STONE));

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
	v.setNodeNoRef(v3s16(0,0,0), MapNode(CONTENT_STONE));
	v.setNodeNoRef(v3s16(1,1,1), MapNode(CONTENT_TORCH));

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
