/*
Minetest
Copyright (C) 2023 Vitaliy Lobachevskiy

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

#include "mesh_compare.h"

// This is a self-test to ensure proper functionality of the vertex
// building functions (`Triangle`, `Quad`) and its validation function
// `checkMeshEqual` in preparation for the tests in test_content_mapblock.cpp
class TestMeshCompare : public TestBase {
public:
	TestMeshCompare() { TestManager::registerTestModule(this); }
	const char *getName() override { return "TestMeshCompare"; }

	void runTests(IGameDef *gamedef) override {
		TEST(testTriangle);
		TEST(testQuad);
	}

	void testTriangle() {
		UASSERT(checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
		}, {0, 1, 2}, {
			Triangle{{
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			}},
		}));
		UASSERT(checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
		}, {2, 0, 1}, {
			Triangle{{
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			}},
		}));
		UASSERT(!checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
		}, {0, 2, 1}, {
			Triangle{{
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			}},
		}));

		UASSERT(checkMeshEqual({
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
		}, {0, 1, 2}, {
			Triangle{{
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			}},
		}));
		UASSERT(!checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
		}, {0, 1, 2}, {
			Triangle{{
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			}},
		}));
	}

	void testQuad() {
		UASSERT(checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
			{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
		}, {0, 1, 2, 0, 2, 3}, {
			Quad{{
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
			}},
		}));
		UASSERT(checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
			{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
		}, {2, 3, 0, 1, 2, 0}, {
			Quad{{
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
			}},
		}));
		UASSERT(checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
			{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
		}, {2, 3, 1, 0, 1, 3}, {
			Quad{{
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
			}},
		}));
		UASSERT(checkMeshEqual({
			{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
			{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
			{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
			{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
		}, {3, 0, 1, 1, 2, 3}, {
			Quad{{
				{{1., 0., 0.}, {3., 0., 0.}, 1, {0., 0.}},
				{{0., 1., 0.}, {2., 0., 0.}, 2, {0., 0.}},
				{{0., 0., 1.}, {1., 0., 0.}, 3, {0., 0.}},
				{{1., -1., 1.}, {4., 0., 0.}, 4, {0., 0.}},
			}},
		}));
	}
};

static TestMeshCompare mesh_compare_test;
