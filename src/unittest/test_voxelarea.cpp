/*
Minetest
Copyright (C) 2018 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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
#include "voxel.h"

class TestVoxelArea : public TestBase {
public:
	TestVoxelArea() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestVoxelArea"; }

	void runTests(IGameDef *gamedef);

	void test_index_xyz_all_pos();
	void test_index_xyz_x_neg();
	void test_index_xyz_y_neg();
	void test_index_xyz_z_neg();
	void test_index_xyz_xy_neg();
	void test_index_xyz_xz_neg();
	void test_index_xyz_yz_neg();
	void test_index_xyz_all_neg();
	void test_index_v3s16_all_pos();
	void test_index_v3s16_x_neg();
	void test_index_v3s16_y_neg();
	void test_index_v3s16_z_neg();
	void test_index_v3s16_xy_neg();
	void test_index_v3s16_xz_neg();
	void test_index_v3s16_yz_neg();
	void test_index_v3s16_all_neg();
	void test_add_x();
	void test_add_y();
	void test_add_z();
	void test_add_p();
};

static TestVoxelArea g_test_instance;

void TestVoxelArea::runTests(IGameDef *gamedef)
{
	TEST(test_index_xyz_all_pos);
	TEST(test_index_xyz_x_neg);
	TEST(test_index_xyz_y_neg);
	TEST(test_index_xyz_z_neg);
	TEST(test_index_xyz_xy_neg);
	TEST(test_index_xyz_xz_neg);
	TEST(test_index_xyz_yz_neg);
	TEST(test_index_xyz_all_neg);
	TEST(test_index_v3s16_all_pos);
	TEST(test_index_v3s16_x_neg);
	TEST(test_index_v3s16_y_neg);
	TEST(test_index_v3s16_z_neg);
	TEST(test_index_v3s16_xy_neg);
	TEST(test_index_v3s16_xz_neg);
	TEST(test_index_v3s16_yz_neg);
	TEST(test_index_v3s16_all_neg);
	TEST(test_add_x);
	TEST(test_add_y);
	TEST(test_add_z);
	TEST(test_add_p);
}

void TestVoxelArea::test_index_xyz_all_pos()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(156, 25, 236), 155);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(156, 25, 236), 1267138774);
}

void TestVoxelArea::test_index_xyz_x_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(-147, 25, 366), -148);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(-147, 25, 366), -870244825);
}

void TestVoxelArea::test_index_xyz_y_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(247, -269, 100), 246);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(247, -269, 100), -989760747);
}

void TestVoxelArea::test_index_xyz_z_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(244, 336, -887), 243);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(244, 336, -887), -191478876);
}

void TestVoxelArea::test_index_xyz_xy_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(-365, -47, 6978), -366);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(-365, -47, 6978), 1493679101);
}

void TestVoxelArea::test_index_xyz_yz_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(66, -58, -789), 65);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(66, -58, -789), 1435362734);
}

void TestVoxelArea::test_index_xyz_xz_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(-36, 589, -992), -37);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(-36, 589, -992), -1934371362);
}

void TestVoxelArea::test_index_xyz_all_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(-88, -99, -1474), -89);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(-88, -99, -1474), -1343473846);
}

void TestVoxelArea::test_index_v3s16_all_pos()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(156, 25, 236)), 155);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(156, 25, 236)), 1267138774);
}

void TestVoxelArea::test_index_v3s16_x_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(-147, 25, 366)), -148);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(-147, 25, 366)), -870244825);
}

void TestVoxelArea::test_index_v3s16_y_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(247, -269, 100)), 246);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(247, -269, 100)), -989760747);
}

void TestVoxelArea::test_index_v3s16_z_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(244, 336, -887)), 243);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(244, 336, -887)), -191478876);
}

void TestVoxelArea::test_index_v3s16_xy_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(-365, -47, 6978)), -366);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(-365, -47, 6978)), 1493679101);
}

void TestVoxelArea::test_index_v3s16_yz_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(66, -58, -789)), 65);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(66, -58, -789)), 1435362734);
}

void TestVoxelArea::test_index_v3s16_xz_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(-36, 589, -992)), -37);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(-36, 589, -992)), -1934371362);
}

void TestVoxelArea::test_index_v3s16_all_neg()
{
	VoxelArea v1;
	UASSERTEQ(s32, v1.index(v3s16(-88, -99, -1474)), -89);

	VoxelArea v2(v3s16(756, 8854, -875), v3s16(-147, -9547, 669));
	UASSERTEQ(s32, v2.index(v3s16(-88, -99, -1474)), -1343473846);
}

void TestVoxelArea::test_add_x()
{
	v3s16 extent;
	u32 i = 4;
	VoxelArea::add_x(extent, i, 8);
	UASSERTEQ(u32, i, 12)
}

void TestVoxelArea::test_add_y()
{
	v3s16 extent(740, 16, 87);
	u32 i = 8;
	VoxelArea::add_y(extent, i, 88);
	UASSERTEQ(u32, i, 65128)
}

void TestVoxelArea::test_add_z()
{
	v3s16 extent(114, 80, 256);
	u32 i = 4;
	VoxelArea::add_z(extent, i, 8);
	UASSERTEQ(u32, i, 72964)
}

void TestVoxelArea::test_add_p()
{
	v3s16 extent(33, 14, 742);
	v3s16 a(15, 12, 369);
	u32 i = 4;
	VoxelArea::add_p(extent, i, a);
	UASSERTEQ(u32, i, 170893)
}
