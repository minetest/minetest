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

	void test_add_x();
	void test_add_y();
	void test_add_z();
	void test_add_p();
};

static TestVoxelArea g_test_instance;

void TestVoxelArea::runTests(IGameDef *gamedef)
{
	TEST(test_add_x);
	TEST(test_add_y);
	TEST(test_add_z);
	TEST(test_add_p);
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
