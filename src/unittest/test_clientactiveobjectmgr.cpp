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

#include "client/activeobjectmgr.h"
#include <algorithm>
#include "test.h"

#include "profiler.h"

class TestClientActiveObject : public ClientActiveObject
{
public:
	TestClientActiveObject() : ClientActiveObject(0, nullptr, nullptr) {}
	~TestClientActiveObject() = default;
	ActiveObjectType getType() const { return ACTIVEOBJECT_TYPE_TEST; }
	virtual void addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr) {}
};

class TestSelectableClientActiveObject : public ClientActiveObject
{
public:
	TestSelectableClientActiveObject(aabb3f _selection_box)
		: ClientActiveObject(0, nullptr, nullptr)
		, selection_box(_selection_box)
	{}

	~TestSelectableClientActiveObject() = default;
	ActiveObjectType getType() const override { return ACTIVEOBJECT_TYPE_TEST; }
	void addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr) override {}
	bool getSelectionBox(aabb3f *toset) const override { *toset = selection_box; return true; }
	const v3f getPosition() const override { return position; }

	v3f position;
	aabb3f selection_box;
};

class TestClientActiveObjectMgr : public TestBase
{
public:
	TestClientActiveObjectMgr() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestClientActiveObjectMgr"; }

	void runTests(IGameDef *gamedef);

	void testFreeID();
	void testRegisterObject();
	void testRemoveObject();
	void testGetActiveSelectableObjects();
};

static TestClientActiveObjectMgr g_test_instance;

void TestClientActiveObjectMgr::runTests(IGameDef *gamedef)
{
	TEST(testFreeID);
	TEST(testRegisterObject)
	TEST(testRemoveObject)
	TEST(testGetActiveSelectableObjects)
}

////////////////////////////////////////////////////////////////////////////////

void TestClientActiveObjectMgr::testFreeID()
{
	client::ActiveObjectMgr caomgr;
	std::vector<u16> aoids;

	u16 aoid = caomgr.getFreeId();
	// Ensure it's not the same id
	UASSERT(caomgr.getFreeId() != aoid);

	aoids.push_back(aoid);

	// Register basic objects, ensure we never found
	for (u8 i = 0; i < UINT8_MAX; i++) {
		// Register an object
		auto tcao_u = std::make_unique<TestClientActiveObject>();
		auto tcao = tcao_u.get();
		caomgr.registerObject(std::move(tcao_u));
		aoids.push_back(tcao->getId());

		// Ensure next id is not in registered list
		UASSERT(std::find(aoids.begin(), aoids.end(), caomgr.getFreeId()) ==
				aoids.end());
	}

	caomgr.clear();
}

void TestClientActiveObjectMgr::testRegisterObject()
{
	client::ActiveObjectMgr caomgr;
	auto tcao_u = std::make_unique<TestClientActiveObject>();
	auto tcao = tcao_u.get();
	UASSERT(caomgr.registerObject(std::move(tcao_u)));

	u16 id = tcao->getId();

	auto tcaoToCompare = caomgr.getActiveObject(id);
	UASSERT(tcaoToCompare->getId() == id);
	UASSERT(tcaoToCompare == tcao);

	tcao_u = std::make_unique<TestClientActiveObject>();
	tcao = tcao_u.get();
	UASSERT(caomgr.registerObject(std::move(tcao_u)));
	UASSERT(caomgr.getActiveObject(tcao->getId()) == tcao);
	UASSERT(caomgr.getActiveObject(tcao->getId()) != tcaoToCompare);

	caomgr.clear();
}

void TestClientActiveObjectMgr::testRemoveObject()
{
	client::ActiveObjectMgr caomgr;
	auto tcao_u = std::make_unique<TestClientActiveObject>();
	auto tcao = tcao_u.get();
	UASSERT(caomgr.registerObject(std::move(tcao_u)));

	u16 id = tcao->getId();
	UASSERT(caomgr.getActiveObject(id) != nullptr)

	caomgr.removeObject(tcao->getId());
	UASSERT(caomgr.getActiveObject(id) == nullptr)

	caomgr.clear();
}

void TestClientActiveObjectMgr::testGetActiveSelectableObjects()
{
	client::ActiveObjectMgr caomgr;
	auto obj_u = std::make_unique<TestSelectableClientActiveObject>(
			aabb3f{v3f{-1, -1, -1}, v3f{1, 1, 1}});
	auto obj = obj_u.get();
	UASSERT(caomgr.registerObject(std::move(obj_u)));

	auto assert_obj_selected = [&] (v3f a, v3f b) {
		auto actual = caomgr.getActiveSelectableObjects({a, b});
		UASSERTEQ(auto, actual.size(), 1u);
		UASSERTEQ(auto, actual.at(0).obj, obj);
	};

	auto assert_obj_missed = [&] (v3f a, v3f b) {
		auto actual = caomgr.getActiveSelectableObjects({a, b});
		UASSERTEQ(auto, actual.size(), 0u);
	};

	float x = 12, y = 3, z = 6;
	obj->position = {x, y, z};

	assert_obj_selected({0, 0, 0}, {x-1, y-1, z-1});
	assert_obj_selected({0, 0, 0}, {2*(x-1), 2*(y-1), 2*(z-1)});
	assert_obj_selected({0, 0, 0}, {2*(x+1), 2*(y-1), 2*(z+1)});
	assert_obj_selected({0, 0, 0}, {20, 5, 10});

	assert_obj_selected({30, -12, 17}, {x+1, y+1, z-1});
	assert_obj_selected({30, -12, 17}, {x, y+1, z});
	assert_obj_selected({30, -12, 17}, {-6, 20, -5});
	assert_obj_selected({30, -12, 17}, {-8, 20, -7});

	assert_obj_selected({-21, 6, -13}, {x+1.4f, y, z});
	assert_obj_selected({-21, 6, -13}, {x-1.4f, y, z});
	assert_obj_missed({-21, 6, -13}, {x-3.f, y, z});

	assert_obj_selected({-21, 6, -13}, {x, y-1.4f, z});
	assert_obj_selected({-21, 6, -13}, {x, y+1.4f, z});
	assert_obj_missed({-21, 6, -13}, {x, y+3.f, z});

	assert_obj_selected({-21, 6, -13}, {x, y, z+1.4f});
	assert_obj_selected({-21, 6, -13}, {x, y, z-1.4f});
	assert_obj_missed({-21, 6, -13}, {x, y, z-3.f});

	caomgr.clear();
}
