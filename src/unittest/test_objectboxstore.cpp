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
#include "util/objectboxstore.h"

#include <cstddef>

class TestObjectBoxStore : public TestBase {
public:
	TestObjectBoxStore() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestBoxStore"; }

	void runTests(IGameDef *gamedef);

	void testConstructor();
	void testInsertion();
	void testRemoval();
	void testClear();
};

static TestObjectBoxStore g_test_instance;

void TestObjectBoxStore::runTests(IGameDef *gamedef)
{
	TEST(testConstructor);
	TEST(testInsertion);
	TEST(testRemoval);
	TEST(testClear);
}

void TestObjectBoxStore::testConstructor() {
	ObjectBoxStore store {};
	UASSERTEQ(std::size_t, store.size(), 0);
}

void TestObjectBoxStore::testInsertion() {
	ObjectBoxStore store {};
	UASSERT(store.insert(0,
		{ { -10.0f, -3.0f, 5.0f }, { 0.0f, 29.0f, 7.0f } }
	));
	UASSERTEQ(std::size_t, store.size(), 1);

	UASSERT(store.insert(2,
		{ { -12.0f, -5.5f, -16.0f }, { -8.0f, 2.0f, 6.5f } }
	));
	UASSERTEQ(std::size_t, store.size(), 2);

	UASSERT(!store.insert(2,
		{ { -11.0f, 8.0f, -3.0f }, { 5.0f, 32.0f, 8.0f } }
	));
	UASSERTEQ(std::size_t, store.size(), 2);

	std::vector<u16> res {};
	store.getInArea(&res,
		{ { -9.0f, -1.5f, -17.0f}, { -9.0f, 1.5f, 8.0f} }
	);
	UASSERTEQ(std::size_t, res.size(), 2);
}

void TestObjectBoxStore::testRemoval() {
	ObjectBoxStore store {};
	store.insert(8, { { -10.0f, -3.0f, 5.0f }, { 0.0f, 29.0f, 7.0f } });
	store.insert(4, { { -12.0f, -5.5f, -16.0f }, { -8.0f, 2.0f, 6.5f } });

	UASSERT(store.remove(8));
	UASSERTEQ(std::size_t, store.size(), 1);

	std::vector<u16> res {};
	store.getInArea(&res,
		{ { -9.0f, -1.5f, -17.0f}, { -9.0f, 1.5f, 8.0f} }
	);
	UASSERTEQ(std::size_t, res.size(), 1);

	UASSERT(!store.remove(8));

	UASSERT(!store.insert(4,
		{ { -12.0f, -5.5f, -16.0f }, { -8.0f, 2.0f, 6.5f } }
	));

	UASSERT(store.insert(8,
		{ { -10.0f, -3.0f, 5.0f }, { 0.0f, 29.0f, 7.0f } }
	));
}

void TestObjectBoxStore::testClear() {
	ObjectBoxStore store {};

	store.clear();
	UASSERTEQ(std::size_t, store.size(), 0)

	store.insert(0, { { -10.0f, -3.0f, 5.0f }, { 0.0f, 29.0f, 7.0f } });
	store.insert(2, { { -12.0f, -5.5f, -16.0f }, { -8.0f, 2.0f, 6.5f } });

	store.clear();
	UASSERTEQ(std::size_t, store.size(), 0)

	std::vector<u16> res {};
	store.getInArea(&res,
		{ { -9.0f, -1.5f, -17.0f}, { -9.0f, 1.5f, 8.0f} }
	);
	UASSERTEQ(std::size_t, res.size(), 0);
}
