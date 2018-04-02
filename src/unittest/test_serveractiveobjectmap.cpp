/*
Minetest
Copyright (C) 2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

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

#include "server/serveractiveobjectmap.h"
#include "content_sao.h"

class TestServerActiveObjectMap : public TestBase
{
public:
	TestServerActiveObjectMap() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestServerActiveObjectMap"; }

	void runTests(IGameDef *gamedef);

	void testAddObject();
	void testRemoveObject();
	void testUpdateObject();
	void testGetObject();
	void testIsFreeID();
	void testGetFreeID();
	void testGetObjectsInsideRadius();
	void testGetObjectsTouchingBox();
};

static TestServerActiveObjectMap g_test_instance;

void TestServerActiveObjectMap::runTests(IGameDef *gamedef)
{
	TEST(testAddObject);
	TEST(testRemoveObject);
	TEST(testUpdateObject);
	TEST(testGetObject);
	TEST(testIsFreeID);
	TEST(testGetFreeID);
	TEST(testGetObjectsInsideRadius);
	TEST(testGetObjectsTouchingBox);
}

void TestServerActiveObjectMap::testAddObject()
{
	ServerActiveObjectMap saom;
	UASSERT(saom.getObjects().empty());

	LuaEntitySAO ob1(nullptr, v3f(0,0,0), "", "");
	saom.addObject(&ob1);

	UASSERT(saom.getObjects().size() == 1);
	bool found = false;
	for (const auto &pair : saom.getObjects()) {
		UASSERT(pair.second.object == &ob1);
		found = true;
	}
	UASSERT(found);
}

void TestServerActiveObjectMap::testRemoveObject()
{
	ServerActiveObjectMap saom;
	UASSERT(saom.getObjects().empty());

	LuaEntitySAO ob1(nullptr, v3f(0,0,0), "", "");
	saom.addObject(&ob1);

	UASSERT(saom.getObjects().size() == 1);
	bool found = false;
	for (const auto &pair : saom.getObjects()) {
		UASSERT(pair.second.object == &ob1);
		found = true;
	}
	UASSERT(found);

	saom.removeObject(&ob1);
}

void TestServerActiveObjectMap::testUpdateObject()
{
	// @TODO
}

void TestServerActiveObjectMap::testGetObject()
{
	// @TODO
}

void TestServerActiveObjectMap::testIsFreeID()
{
	ServerActiveObjectMap saom;
	UASSERT(!saom.isFreeId(0));
	UASSERT(saom.isFreeId(1));
	UASSERT(saom.isFreeId(2));
}

void TestServerActiveObjectMap::testGetFreeID()
{
	ServerActiveObjectMap saom;
	u16 first_id = saom.getFreeId();
	UASSERT(first_id > 0);
	UASSERT(saom.getFreeId() > first_id);
}

void TestServerActiveObjectMap::testGetObjectsInsideRadius()
{
	ServerActiveObjectMap saom;

	auto results = saom.getObjectsInsideRadius(v3f(0,0,0), 5);
	UASSERT(results.empty());

	LuaEntitySAO ob1(nullptr, v3f(0,0,0), "", "");
	saom.addObject(&ob1);
	UASSERT(results.size() == 1);

	LuaEntitySAO ob2(nullptr, v3f(-1,-1,-1), "", "");
	saom.addObject(&ob2);
	UASSERT(results.size() == 2);

	LuaEntitySAO ob3(nullptr, v3f(4.9,0,0), "", "");
	saom.addObject(&ob3);
	UASSERT(results.size() == 3);

	LuaEntitySAO ob4(nullptr, v3f(5,0,0), "", "");
	saom.addObject(&ob4);
	UASSERT(results.size() == 3);

	LuaEntitySAO ob5(nullptr, v3f(3,3,3), "", "");
	saom.addObject(&ob5);
	UASSERT(results.size() == 3);
}

void TestServerActiveObjectMap::testGetObjectsTouchingBox()
{
	// @TODO
}
