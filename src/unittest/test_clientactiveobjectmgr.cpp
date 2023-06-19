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

#include <catch.hpp>

class TestClientActiveObject : public ClientActiveObject
{
public:
	TestClientActiveObject() : ClientActiveObject(0, nullptr, nullptr) {}
	~TestClientActiveObject() = default;
	ActiveObjectType getType() const { return ACTIVEOBJECT_TYPE_TEST; }
	virtual void addToScene(ITextureSource *tsrc, scene::ISceneManager *smgr) {}
};

TEST_CASE("test client active object manager")
{
	client::ActiveObjectMgr caomgr;
	auto tcao1 = new TestClientActiveObject();
	REQUIRE(caomgr.registerObject(tcao1) == true);

	SECTION("When we register a client object, "
			"then it should be assigned a unique ID.")
	{
		for (int i = 0; i < UINT8_MAX; ++i) {
			auto other_tcao = new TestClientActiveObject();
			caomgr.registerObject(other_tcao);
			CHECK(other_tcao->getId() != tcao1->getId());
		}
	}

	SECTION("two registered objects")
	{
		auto tcao2 = new TestClientActiveObject();
		REQUIRE(caomgr.registerObject(tcao2) == true);
		auto tcao2_id = tcao2->getId();

		auto obj1 = caomgr.getActiveObject(tcao1->getId());
		REQUIRE(obj1 != nullptr);

		auto obj2 = caomgr.getActiveObject(tcao2_id);
		REQUIRE(obj2 != nullptr);

		SECTION("When we query an object by its ID, "
				"then we should get back an object with that ID.")
		{
			CHECK(obj1->getId() == tcao1->getId());
			CHECK(obj2->getId() == tcao2->getId());
		}

		SECTION("When we register and query for an object, "
				"its memory location should not have changed.")
		{
			CHECK(obj1 == tcao1);
			CHECK(obj2 == tcao2);
		}
	}

	SECTION("Given an object has been removed, "
			"when we query for it, "
			"then we should get nullptr.")
	{
		caomgr.removeObject(tcao1->getId());
		CHECK(caomgr.getActiveObject(tcao1->getId()) == nullptr);
	}

	caomgr.clear();
}
