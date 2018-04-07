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

	LuaEntitySAO ob1(nullptr, v3f(0, 0, 0), "", "");
	ob1.setId(saom.getFreeId());
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

	LuaEntitySAO ob1(nullptr, v3f(0, 0, 0), "", "");
	ob1.setId(saom.getFreeId());
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
	ServerActiveObjectMap saom;
	LuaEntitySAO ob1(nullptr, v3f(1, 0, 0), "", "");
	ob1.accessObjectProperties()->physical = true;
	ob1.setId(saom.getFreeId());
	saom.addObject(&ob1);
	UASSERT(saom.getObjectsInsideRadius(v3f(0, 0, 0), 2).size() == 1);
	UASSERT(saom.getObjectsInsideRadius(v3f(6, 0, 0), 2).size() == 0);
	ob1.setBasePosition(v3f(5, 0, 0));
	saom.updateObject(&ob1);
	UASSERT(saom.getObjectsInsideRadius(v3f(0, 0, 0), 2).size() == 0);
	UASSERT(saom.getObjectsInsideRadius(v3f(6, 0, 0), 2).size() == 1);
}

void TestServerActiveObjectMap::testGetObject()
{
	ServerActiveObjectMap saom;
	LuaEntitySAO ob1(nullptr, v3f(0, 0, 0), "", "");
	u16 id = saom.getFreeId();
	ob1.setId(id);
	saom.addObject(&ob1);
	UASSERT(saom.getObject(0) == nullptr);
	UASSERT(saom.getObject(id) == &ob1);
	UASSERT(saom.getObject(id + 1) == nullptr);
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
#define ADD_OBJECT_IMPL(name, pos)                                                       \
	LuaEntitySAO name(nullptr, pos, "", "");                                         \
	name.accessObjectProperties()->physical = true;                                  \
	name.setId(saom.getFreeId());                                                    \
	saom.addObject(&name)
#define ADD_OBJECT(pos) ADD_OBJECT_IMPL(NEWNAME(ob), pos)
#define OBJECT_COUNT (saom.getObjectsInsideRadius(v3f(0, 0, 0), 5).size())

	UASSERT(OBJECT_COUNT == 0);

	ADD_OBJECT(v3f(0, 0, 0));
	UASSERT(OBJECT_COUNT == 1);

	ADD_OBJECT(v3f(-1, -1, -1));
	UASSERT(OBJECT_COUNT == 2);

	ADD_OBJECT(v3f(4.9, 0, 0));
	UASSERT(OBJECT_COUNT == 3);

	ADD_OBJECT(v3f(5.1, 0, 0));
	UASSERT(OBJECT_COUNT == 3);

	ADD_OBJECT(v3f(3, 3, 3));
	UASSERT(OBJECT_COUNT == 3);
}

void TestServerActiveObjectMap::testGetObjectsTouchingBox()
{
	ServerActiveObjectMap saom;

	LuaEntitySAO ob1(nullptr, v3f(1 * BS, 0, 0), "", "");
	ob1.accessObjectProperties()->physical = true;
	// Collision boxes are in nodes, not in world units:
	ob1.accessObjectProperties()->collisionbox = {-1, -1, -1, 1, 1, 1};
	ob1.setId(saom.getFreeId());
	saom.addObject(&ob1);

	LuaEntitySAO ob2(nullptr, v3f(1.5 * BS, 2.5 * BS, 0), "", "");
	ob2.accessObjectProperties()->physical = true;
	ob2.accessObjectProperties()->collisionbox = {-0.5, -0.5, -1, 3.5, 2, 1};
	ob2.setId(saom.getFreeId());
	saom.addObject(&ob2);

	std::vector<u16> list;

	list = saom.getObjectsTouchingBox(
			{2.1 * BS, -1.0 * BS, -1.0 * BS, 2.5 * BS, 1.0 * BS, 1.0 * BS});
	UASSERT(list.size() == 0);

	// intersecting ob1
	list = saom.getObjectsTouchingBox(
			{1.9 * BS, -1.0 * BS, -1.0 * BS, 2.5 * BS, 1.0 * BS, 1.0 * BS});
	UASSERT(list.size() == 1 && list[0] == ob1.getId());

	// intersecting ob2
	list = saom.getObjectsTouchingBox(
			{2.1 * BS, -1.0 * BS, -1.0 * BS, 2.5 * BS, 2.1 * BS, 1.0 * BS});
	UASSERT(list.size() == 1 && list[0] == ob2.getId());

	// contained in ob1
	list = saom.getObjectsTouchingBox(
			{1.5 * BS, -0.1 * BS, -0.1 * BS, 1.9 * BS, 0.1 * BS, 0.1 * BS});
	UASSERT(list.size() == 1 && list[0] == ob1.getId());

	// contains ob2
	list = saom.getObjectsTouchingBox(
			{0.9 * BS, 1.5 * BS, -5.0 * BS, 6.0 * BS, 20.0 * BS, 500.0 * BS});
	UASSERT(list.size() == 1 && list[0] == ob2.getId());

	// intersecting both
	list = saom.getObjectsTouchingBox(
			{1.9 * BS, -1.0 * BS, -1.0 * BS, 2.5 * BS, 2.1 * BS, 1.0 * BS});
	UASSERT(list.size() == 2);
}
