/*
Minetest
Copyright (C) 2010-2014 kwolekr, Ryan Kwolek <kwolekr@minetest.net>

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

#include "exceptions.h"
#include "objdef.h"

class TestObjDef : public TestBase
{
public:
	TestObjDef() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestObjDef"; }

	void runTests(IGameDef *gamedef);

	void testHandles();
	void testAddGetSetClear();
};

static TestObjDef g_test_instance;

void TestObjDef::runTests(IGameDef *gamedef)
{
	TEST(testHandles);
	TEST(testAddGetSetClear);
}

////////////////////////////////////////////////////////////////////////////////

void TestObjDef::testHandles()
{
	u32 uid = 0;
	u32 index = 0;
	ObjDefType type = OBJDEF_GENERIC;

	ObjDefHandle handle = ObjDefManager::createHandle(9530, OBJDEF_ORE, 47);

	UASSERTEQ(ObjDefHandle, 0xAF507B55, handle);

	UASSERT(ObjDefManager::decodeHandle(handle, &index, &type, &uid));

	UASSERTEQ(u32, 9530, index);
	UASSERTEQ(u32, 47, uid);
	UASSERTEQ(ObjDefHandle, OBJDEF_ORE, type);
}

void TestObjDef::testAddGetSetClear()
{
	ObjDefManager testmgr(NULL, OBJDEF_GENERIC);
	ObjDefHandle hObj0, hObj1, hObj2, hObj3;
	ObjDef *obj0, *obj1, *obj2, *obj3;

	UASSERTEQ(ObjDefType, testmgr.getType(), OBJDEF_GENERIC);

	obj0 = new ObjDef;
	obj0->name = "foobar";
	hObj0 = testmgr.add(obj0);
	UASSERT(hObj0 != OBJDEF_INVALID_HANDLE);
	UASSERTEQ(u32, obj0->index, 0);

	obj1 = new ObjDef;
	obj1->name = "FooBaz";
	hObj1 = testmgr.add(obj1);
	UASSERT(hObj1 != OBJDEF_INVALID_HANDLE);
	UASSERTEQ(u32, obj1->index, 1);

	obj2 = new ObjDef;
	obj2->name = "asdf";
	hObj2 = testmgr.add(obj2);
	UASSERT(hObj2 != OBJDEF_INVALID_HANDLE);
	UASSERTEQ(u32, obj2->index, 2);

	obj3 = new ObjDef;
	obj3->name = "foobaz";
	hObj3 = testmgr.add(obj3);
	UASSERT(hObj3 == OBJDEF_INVALID_HANDLE);

	UASSERTEQ(size_t, testmgr.getNumObjects(), 3);

	UASSERT(testmgr.get(hObj0) == obj0);
	UASSERT(testmgr.getByName("FOOBAZ") == obj1);

	UASSERT(testmgr.set(hObj0, obj3) == obj0);
	UASSERT(testmgr.get(hObj0) == obj3);
	delete obj0;

	testmgr.clear();
	UASSERTEQ(size_t, testmgr.getNumObjects(), 0);
}
