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
#include "mock_serveractiveobject.h"
#include <algorithm>
#include <queue>

#include "server/activeobjectmgr.h"

#include "profiler.h"


class TestServerActiveObjectMgr : public TestBase
{
public:
	TestServerActiveObjectMgr() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestServerActiveObjectMgr"; }

	void runTests(IGameDef *gamedef);

	void testFreeID();
	void testRegisterObject();
	void testRemoveObject();
	void testGetObjectsInsideRadius();
	void testGetAddedActiveObjectsAroundPos();
};

static TestServerActiveObjectMgr g_test_instance;

void TestServerActiveObjectMgr::runTests(IGameDef *gamedef)
{
	TEST(testFreeID);
	TEST(testRegisterObject)
	TEST(testRemoveObject)
	TEST(testGetObjectsInsideRadius);
	TEST(testGetAddedActiveObjectsAroundPos);
}

void clearSAOMgr(server::ActiveObjectMgr *saomgr)
{
	auto clear_cb = [](ServerActiveObject *obj, u16 id) {
		delete obj;
		return true;
	};
	saomgr->clear(clear_cb);
}

////////////////////////////////////////////////////////////////////////////////

void TestServerActiveObjectMgr::testFreeID()
{
	server::ActiveObjectMgr saomgr;
	std::vector<u16> aoids;

	u16 aoid = saomgr.getFreeId();
	// Ensure it's not the same id
	UASSERT(saomgr.getFreeId() != aoid);

	aoids.push_back(aoid);

	// Register basic objects, ensure we never found
	for (u8 i = 0; i < UINT8_MAX; i++) {
		// Register an object
		auto sao = new MockServerActiveObject();
		saomgr.registerObject(sao);
		aoids.push_back(sao->getId());

		// Ensure next id is not in registered list
		UASSERT(std::find(aoids.begin(), aoids.end(), saomgr.getFreeId()) ==
				aoids.end());
	}

	clearSAOMgr(&saomgr);
}

void TestServerActiveObjectMgr::testRegisterObject()
{
	server::ActiveObjectMgr saomgr;
	auto sao = new MockServerActiveObject();
	UASSERT(saomgr.registerObject(sao));

	u16 id = sao->getId();

	auto saoToCompare = saomgr.getActiveObject(id);
	UASSERT(saoToCompare->getId() == id);
	UASSERT(saoToCompare == sao);

	sao = new MockServerActiveObject();
	UASSERT(saomgr.registerObject(sao));
	UASSERT(saomgr.getActiveObject(sao->getId()) == sao);
	UASSERT(saomgr.getActiveObject(sao->getId()) != saoToCompare);

	clearSAOMgr(&saomgr);
}

void TestServerActiveObjectMgr::testRemoveObject()
{
	server::ActiveObjectMgr saomgr;
	auto sao = new MockServerActiveObject();
	UASSERT(saomgr.registerObject(sao));

	u16 id = sao->getId();
	UASSERT(saomgr.getActiveObject(id) != nullptr)

	saomgr.removeObject(sao->getId());
	UASSERT(saomgr.getActiveObject(id) == nullptr);

	clearSAOMgr(&saomgr);
}

void TestServerActiveObjectMgr::testGetObjectsInsideRadius()
{
	server::ActiveObjectMgr saomgr;
	static const v3f sao_pos[] = {
			v3f(10, 40, 10),
			v3f(740, 100, -304),
			v3f(-200, 100, -304),
			v3f(740, -740, -304),
			v3f(1500, -740, -304),
	};

	for (const auto &p : sao_pos) {
		saomgr.registerObject(new MockServerActiveObject(nullptr, p));
	}

	std::vector<ServerActiveObject *> result;
	saomgr.getObjectsInsideRadius(v3f(), 50, result, nullptr);
	UASSERTCMP(int, ==, result.size(), 1);

	result.clear();
	saomgr.getObjectsInsideRadius(v3f(), 750, result, nullptr);
	UASSERTCMP(int, ==, result.size(), 2);

	result.clear();
	saomgr.getObjectsInsideRadius(v3f(), 750000, result, nullptr);
	UASSERTCMP(int, ==, result.size(), 5);

	result.clear();
	auto include_obj_cb = [](ServerActiveObject *obj) {
		return (obj->getBasePosition().X != 10);
	};

	saomgr.getObjectsInsideRadius(v3f(), 750000, result, include_obj_cb);
	UASSERTCMP(int, ==, result.size(), 4);

	clearSAOMgr(&saomgr);
}

void TestServerActiveObjectMgr::testGetAddedActiveObjectsAroundPos()
{
	server::ActiveObjectMgr saomgr;
	static const v3f sao_pos[] = {
			v3f(10, 40, 10),
			v3f(740, 100, -304),
			v3f(-200, 100, -304),
			v3f(740, -740, -304),
			v3f(1500, -740, -304),
	};

	for (const auto &p : sao_pos) {
		saomgr.registerObject(new MockServerActiveObject(nullptr, p));
	}

	std::queue<u16> result;
	std::set<u16> cur_objects;
	saomgr.getAddedActiveObjectsAroundPos(v3f(), 100, 50, cur_objects, result);
	UASSERTCMP(int, ==, result.size(), 1);

	result = std::queue<u16>();
	cur_objects.clear();
	saomgr.getAddedActiveObjectsAroundPos(v3f(), 740, 50, cur_objects, result);
	UASSERTCMP(int, ==, result.size(), 2);

	clearSAOMgr(&saomgr);
}
