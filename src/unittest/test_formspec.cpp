/*
Minetest
Copyright (C) 2020 rubenwardy

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

#include "gui/formspec/FormspecElement.h"

class TestFormspec : public TestBase
{
public:
	TestFormspec() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestFormspec"; }

	void runTests(IGameDef *gamedef);

	void testFormspecElement();
};

static TestFormspec g_test_instance;

void TestFormspec::runTests(IGameDef *gamedef)
{
	TEST(testFormspecElement);
}

#define VC_EQUALS(one, two) UASSERT((one - two).getLengthSQ() < 0.01f)

void TestFormspec::testFormspecElement()
{
	{
		FormspecElement element{"1.1,2.2;3.3,4.4;name;Label;value"};

		UASSERT(element.size() == 5);
		VC_EQUALS(element.getV2f(0), v2f32(1.1f, 2.2f));
		VC_EQUALS(element.getV2f(1), v2f32(3.3f, 4.4f));
		UASSERT(element.getString(2) == "name");
		UASSERT(element.getString(3) == "Label");
		UASSERT(element.getString(4) == "value");
	}

	{
		FormspecElement element{"name;one=1;two=2;three=3"};

		UASSERT(element.size() == 4);
		UASSERT(element.getString(0) == "name");

		auto parameters = element.getParameters(1);
		UASSERT(parameters.size() == 3);
		UASSERT(parameters["one"] == "1");
		UASSERT(parameters["two"] == "2");
		UASSERT(parameters["three"] == "3");
	}
}
