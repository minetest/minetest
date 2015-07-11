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

#include "areastore.h"

class TestAreaStore : public TestBase {
public:
	TestAreaStore() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestAreaStore"; }

	void runTests(IGameDef *gamedef);

	void genericStoreTest(AreaStore *store);
	void testVectorStore();
	void testSpatialStore();
};

static TestAreaStore g_test_instance;

void TestAreaStore::runTests(IGameDef *gamedef)
{
	TEST(testVectorStore);
#if USE_SPATIAL
	TEST(testSpatialStore);
#endif
}

////////////////////////////////////////////////////////////////////////////////

void TestAreaStore::testVectorStore()
{
	VectorAreaStore store;
	genericStoreTest(&store);
}

void TestAreaStore::testSpatialStore()
{
#if USE_SPATIAL
	SpatialAreaStore store;
	genericStoreTest(&store);
#endif
}

void TestAreaStore::genericStoreTest(AreaStore *store)
{
	Area a(v3s16(-10, -3, 5), v3s16(0, 29, 7));
	a.id = 1;
	Area b(v3s16(-5, -2, 5), v3s16(0, 28, 6));
	b.id = 2;
	Area c(v3s16(-7, -3, 6), v3s16(-1, 27, 7));
	c.id = 3;
	std::vector<Area *> res;

	UASSERTEQ(size_t, store->size(), 0);
	store->reserve(2); // sic
	store->insertArea(a);
	store->insertArea(b);
	store->insertArea(c);
	UASSERTEQ(size_t, store->size(), 3);

	store->getAreasForPos(&res, v3s16(-1, 0, 6));
	UASSERTEQ(size_t, res.size(), 3);
	res.clear();
	store->getAreasForPos(&res, v3s16(0, 0, 7));
	UASSERTEQ(size_t, res.size(), 1);
	UASSERTEQ(u32, res[0]->id, 1);
	res.clear();

	store->removeArea(1);

	store->getAreasForPos(&res, v3s16(0, 0, 7));
	UASSERTEQ(size_t, res.size(), 0);
	res.clear();

	store->insertArea(a);

	store->getAreasForPos(&res, v3s16(0, 0, 7));
	UASSERTEQ(size_t, res.size(), 1);
	UASSERTEQ(u32, res[0]->id, 1);
	res.clear();

	store->getAreasInArea(&res, v3s16(-10, -3, 5), v3s16(0, 29, 7), false);
	UASSERTEQ(size_t, res.size(), 3);
	res.clear();

	store->getAreasInArea(&res, v3s16(-100, 0, 6), v3s16(200, 0, 6), false);
	UASSERTEQ(size_t, res.size(), 0);
	res.clear();

	store->getAreasInArea(&res, v3s16(-100, 0, 6), v3s16(200, 0, 6), true);
	UASSERTEQ(size_t, res.size(), 3);
	res.clear();

	store->removeArea(1);
	store->removeArea(2);
	store->removeArea(3);

	Area d(v3s16(-100, -300, -200), v3s16(-50, -200, -100));
	d.id = 4;
	d.data = "Hi!";
	store->insertArea(d);

	store->getAreasForPos(&res, v3s16(-75, -250, -150));
	UASSERTEQ(size_t, res.size(), 1);
	UASSERTEQ(u32, res[0]->id, 4);
	UASSERTEQ(u16, res[0]->data.size(), 3);
	UASSERT(strncmp(res[0]->data.c_str(), "Hi!", 3) == 0);
	res.clear();

	store->removeArea(4);
}
