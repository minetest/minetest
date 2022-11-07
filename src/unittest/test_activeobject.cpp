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

#include "mock_activeobject.h"

class TestActiveObject : public TestBase
{
public:
	TestActiveObject() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestActiveObject"; }

	void runTests(IGameDef *gamedef);

	void testAOAttributes();
};

static TestActiveObject g_test_instance;

void TestActiveObject::runTests(IGameDef *gamedef)
{
	TEST(testAOAttributes);
}

void TestActiveObject::testAOAttributes()
{
	MockActiveObject ao(44);
	UASSERT(ao.getId() == 44);

	ao.setId(558);
	UASSERT(ao.getId() == 558);
}
