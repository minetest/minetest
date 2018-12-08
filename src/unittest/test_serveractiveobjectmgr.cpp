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

#include "server/activeobjectmgr.h"
#include <algorithm>
#include "test.h"

#include "profiler.h"

class TestServerActiveObject : public ServerActiveObject
{
public:
	TestServerActiveObject() : ServerActiveObject(nullptr, v3f()) {}
	~TestServerActiveObject() = default;
	ActiveObjectType getType() const override { return ACTIVEOBJECT_TYPE_TEST; }
	bool getCollisionBox(aabb3f *toset) const override { return false; }
	bool getSelectionBox(aabb3f *toset) const override { return false; }
	bool collideWithObjects() const override { return false; }
};

class TestServerActiveObjectMgr : public TestBase
{
public:
	TestServerActiveObjectMgr() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestServerActiveObjectMgr"; }

	void runTests(IGameDef *gamedef);

	void testFreeID();
	void testRegisterObject();
	void testRemoveObject();
};

static TestServerActiveObjectMgr g_test_instance;

void TestServerActiveObjectMgr::runTests(IGameDef *gamedef)
{
	TEST(testFreeID);
	TEST(testRegisterObject)
	TEST(testRemoveObject)
}

////////////////////////////////////////////////////////////////////////////////

void TestServerActiveObjectMgr::testFreeID()
{
	server::ActiveObjectMgr caomgr;
	std::vector<u16> aoids;

	u16 aoid = caomgr.getFreeId();
	// Ensure it's not the same id
	UASSERT(caomgr.getFreeId() != aoid);

	aoids.push_back(aoid);

	// Register basic objects, ensure we never found
	for (u8 i = 0; i < UINT8_MAX; i++) {
		// Register an object
		auto tcao = new TestServerActiveObject();
		caomgr.registerObject(tcao);
		aoid = tcao->getId();

		// Ensure next id is not in registered list
		UASSERT(std::find(aoids.begin(), aoids.end(), caomgr.getFreeId()) == aoids.end());
	}
}

void TestServerActiveObjectMgr::testRegisterObject()
{
	server::ActiveObjectMgr caomgr;
	auto tcao = new TestServerActiveObject();
	UASSERT(caomgr.registerObject(tcao));

	u16 id = tcao->getId();

	auto tcaoToCompare = caomgr.getActiveObject(id);
	UASSERT(tcaoToCompare->getId() == id);
	UASSERT(tcaoToCompare == tcao);

	tcao = new TestServerActiveObject();
	UASSERT(caomgr.registerObject(tcao));
	UASSERT(caomgr.getActiveObject(tcao->getId()) == tcao);
	UASSERT(caomgr.getActiveObject(tcao->getId()) != tcaoToCompare);
}

void TestServerActiveObjectMgr::testRemoveObject()
{
	server::ActiveObjectMgr caomgr;
	auto tcao = new TestServerActiveObject();
	UASSERT(caomgr.registerObject(tcao));
	UASSERT(caomgr.getActiveObject(tcao->getId()) != nullptr)

	caomgr.removeObject(tcao->getId());
	UASSERT(caomgr.getActiveObject(tcao->getId()) == nullptr)
}
