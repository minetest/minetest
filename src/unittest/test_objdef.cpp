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
	void testClone();
};

static TestObjDef g_test_instance;

void TestObjDef::runTests(IGameDef *gamedef)
{
	TEST(testHandles);
	TEST(testAddGetSetClear);
	TEST(testClone);
}

////////////////////////////////////////////////////////////////////////////////

/* Minimal implementation of ObjDef and ObjDefManager subclass */

class MyObjDef : public ObjDef
{
public:
	ObjDef *clone() const
	{
		auto def = new MyObjDef();
		ObjDef::cloneTo(def);
		def->testvalue = testvalue;
		return def;
	};

	u32 testvalue;
};

class MyObjDefManager : public ObjDefManager
{
public:
	MyObjDefManager(ObjDefType type) : ObjDefManager(NULL, type){};
	MyObjDefManager *clone() const
	{
		auto mgr = new MyObjDefManager();
		ObjDefManager::cloneTo(mgr);
		return mgr;
	};

protected:
	MyObjDefManager(){};
};

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

	obj0 = new MyObjDef;
	obj0->name = "foobar";
	hObj0 = testmgr.add(obj0);
	UASSERT(hObj0 != OBJDEF_INVALID_HANDLE);
	UASSERTEQ(u32, obj0->index, 0);

	obj1 = new MyObjDef;
	obj1->name = "FooBaz";
	hObj1 = testmgr.add(obj1);
	UASSERT(hObj1 != OBJDEF_INVALID_HANDLE);
	UASSERTEQ(u32, obj1->index, 1);

	obj2 = new MyObjDef;
	obj2->name = "asdf";
	hObj2 = testmgr.add(obj2);
	UASSERT(hObj2 != OBJDEF_INVALID_HANDLE);
	UASSERTEQ(u32, obj2->index, 2);

	obj3 = new MyObjDef;
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

void TestObjDef::testClone()
{
	MyObjDefManager testmgr(OBJDEF_GENERIC);
	ObjDefManager *mgrcopy;
	MyObjDef *obj, *temp2;
	ObjDef *temp1;
	ObjDefHandle hObj;

	obj = new MyObjDef;
	obj->testvalue = 0xee00ff11;
	hObj = testmgr.add(obj);
	UASSERT(hObj != OBJDEF_INVALID_HANDLE);

	mgrcopy = testmgr.clone();
	UASSERT(mgrcopy);
	UASSERTEQ(ObjDefType, mgrcopy->getType(), testmgr.getType());
	UASSERTEQ(size_t, mgrcopy->getNumObjects(), testmgr.getNumObjects());

	// 1) check that the same handle is still valid on the copy
	temp1 = mgrcopy->get(hObj);
	UASSERT(temp1);
	UASSERT(temp1 == mgrcopy->getRaw(0));
	// 2) check that the copy has the correct C++ class
	temp2 = dynamic_cast<MyObjDef *>(temp1);
	UASSERT(temp2);
	// 3) check that it was correctly copied
	UASSERTEQ(u32, obj->testvalue, temp2->testvalue);
	// 4) check that it was copied AT ALL (not the same)
	UASSERT(obj != temp2);

	testmgr.clear();
	mgrcopy->clear();
	delete mgrcopy;
}
