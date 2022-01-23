/*
Minetest
Copyright (C) 2022 JosiahWI <josiah_vanderzee@mediacombb.net>

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
#include "util/spatialstore.h"

#include <cstddef>
#include <vector>

class TestObjectBoxStore : public TestBase
{
public:
	TestObjectBoxStore() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestBoxStore"; }

	void runTests(IGameDef *gamedef);

	void testConstructor();
	void testInsertion();
	void testRemoval();
	void testClear();
	void testGetInArea();
};

static TestObjectBoxStore g_test_instance;

void TestObjectBoxStore::runTests(IGameDef *gamedef)
{
	TEST(testConstructor);
	TEST(testInsertion);
	TEST(testRemoval);
	TEST(testClear);
	TEST(testGetInArea);
}

void TestObjectBoxStore::testConstructor()
{
	ObjectBoxStore store{};
	UASSERTEQ(std::size_t, store.size(), 0);
}

void TestObjectBoxStore::testInsertion()
{
	ObjectBoxStore store{};
	UASSERT(store.insert(0, {{-10.0f, -3.0f, 5.0f}, {0.0f, 29.0f, 7.0f}}));
	UASSERTEQ(std::size_t, store.size(), 1);

	UASSERT(store.insert(2, {{-12.0f, -5.5f, -16.0f}, {-8.0f, 2.0f, 6.5f}}));
	UASSERTEQ(std::size_t, store.size(), 2);

	UASSERT(!store.insert(2, {{-11.0f, 8.0f, -3.0f}, {5.0f, 32.0f, 8.0f}}));
	UASSERTEQ(std::size_t, store.size(), 2);

	std::vector<u16> res{};
	store.getInArea(&res, {{-9.0f, -1.5f, -17.0f}, {-9.0f, 1.5f, 8.0f}});
	UASSERTEQ(std::size_t, res.size(), 2);
}

void TestObjectBoxStore::testRemoval()
{
	ObjectBoxStore store{};
	store.insert(8, {{-10.0f, -3.0f, 5.0f}, {0.0f, 29.0f, 7.0f}});
	store.insert(4, {{-12.0f, -5.5f, -16.0f}, {-8.0f, 2.0f, 6.5f}});

	UASSERT(store.remove(8));
	UASSERTEQ(std::size_t, store.size(), 1);

	std::vector<u16> res{};
	store.getInArea(&res, {{-9.0f, -1.5f, -17.0f}, {-9.0f, 1.5f, 8.0f}});
	UASSERTEQ(std::size_t, res.size(), 1);

	UASSERT(!store.remove(8));

	UASSERT(!store.insert(4, {{-12.0f, -5.5f, -16.0f}, {-8.0f, 2.0f, 6.5f}}));

	UASSERT(store.insert(8, {{-10.0f, -3.0f, 5.0f}, {0.0f, 29.0f, 7.0f}}));
}

void TestObjectBoxStore::testClear()
{
	ObjectBoxStore store{};

	store.clear();
	UASSERTEQ(std::size_t, store.size(), 0)

	store.insert(0, {{-10.0f, -3.0f, 5.0f}, {0.0f, 29.0f, 7.0f}});
	store.insert(2, {{-12.0f, -5.5f, -16.0f}, {-8.0f, 2.0f, 6.5f}});

	store.clear();
	UASSERTEQ(std::size_t, store.size(), 0)

	std::vector<u16> res{};
	store.getInArea(&res, {{-9.0f, -1.5f, -17.0f}, {-9.0f, 1.5f, 8.0f}});
	UASSERTEQ(std::size_t, res.size(), 0);
}

void TestObjectBoxStore::testGetInArea()
{
	ObjectBoxStore store{};
	store.insert(0, {{-5.7f, 6.0f, -13.3f}, {12.6f, 10.2f, 2.0f}});
	store.insert(1, {{-8.3f, -1.1f, -14.5f}, {-5.7f, 6.0f, -13.3f}});

	// no intersection.
	std::vector<u16> res{store.getInArea({{-5.6f, 3.0f, -14.0f}, {-2.0f, 5.9f, 0.5f}})};
	UASSERTEQ(std::size_t, res.size(), 0);

	// touches at 1 point.
	store.getInArea(&res, {{12.6f, 3.4f, -15.5f}, {16.0f, 6.0f, -13.3f}});
	UASSERTEQ(std::size_t, res.size(), 1);
	res.clear();

	// intersection with one area
	store.getInArea(&res, {{9.8f, 3.4f, -15.5f}, {16.0f, 7.5f, -11.0f}});
	UASSERTEQ(std::size_t, res.size(), 1);
	res.clear();

	std::vector<u16> directReturn{
			store.getInArea({{9.9f, 2.0f, -15.5f}, {15.5f, 6.1f, -11.0f}})};
	UASSERTEQ(std::size_t, directReturn.size(), 1);

	// entirely contained inside area
	store.getInArea(&res, {{9.8f, 7.0f, -10.4f}, {11.1f, 9.5f, 0.0f}});
	UASSERTEQ(std::size_t, res.size(), 1);
	res.clear();

	// entirely contains area and intersects second area
	store.getInArea(&res, {{-8.0f, 5.6f, -14.0f}, {13.4f, 10.9f, 3.0f}});
	UASSERTEQ(std::size_t, res.size(), 2);
	res.clear();
}
