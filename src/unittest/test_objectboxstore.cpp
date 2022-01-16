/*
Minetest
Copyright (C) 2015 est31, <MTest31@outlook.com>

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

class TestObjectBoxStore : public TestBase {
public:
	TestObjectBoxStore() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestBoxStore"; }

	void runTests(IGameDef *gamedef);

	void genericStoreTest(ObjectBoxStore *store);
	void testSpatialStore();
};

static TestObjectBoxStore g_test_instance;

void TestObjectBoxStore::runTests(IGameDef *gamedef)
{
	TEST(testSpatialStore);
}

////////////////////////////////////////////////////////////////////////////////

void TestObjectBoxStore::testSpatialStore()
{
	ObjectBoxStore store;
	genericStoreTest(&store);
}

void TestObjectBoxStore::genericStoreTest(ObjectBoxStore *store)
{
	aabb3f a(v3f(-10, -3, 5), v3f(0, 29, 7));
	aabb3f b(v3f(-5, -2, 5), v3f(0, 28, 6));
	aabb3f c(v3f(-7, -3, 6), v3f(-1, 27, 7));
	std::vector<u16> res;

	UASSERTEQ(size_t, store->size(), 0);
	store->reserve(2); // sic
	store->insert(1, a);
	store->insert(2, b);
	store->insert(3, c);
	UASSERTEQ(size_t, store->size(), 3);

	store->remove(1);

	store->insert(1, a);

	store->getInArea(&res, { v3f(-10, -3, 5), v3f(0, 29, 7) });
	UASSERTEQ(size_t, res.size(), 3);
	res.clear();

	store->getInArea(&res, { v3f(-100, 0, 6), v3f(200, 0, 6) });
	UASSERTEQ(size_t, res.size(), 0);
	res.clear();

	store->getInArea(&res, { v3f(-100, 0, 6), v3f(200, 0, 6) });
	UASSERTEQ(size_t, res.size(), 3);
	res.clear();

	store->remove(1);
	store->remove(2);
	store->remove(3);
}
