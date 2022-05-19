/*
Minetest
Copyright (C) 2021 The Minetest Authors

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
#include "util/string.h"
#include "exceptions.h"

class TestExceptions : public TestBase
{
public:
	TestExceptions() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestExceptions"; }

	void runTests(IGameDef *gamedef);

	void testConstructors();
};

static TestExceptions g_test_instance;

void TestExceptions::runTests(IGameDef *gamedef)
{
	TEST(testConstructors);
}

DEFINE_EXCEPTION(CustomException1)
DEFINE_EXCEPTION_DEFTEXT(CustomException2, "The Default Message")

////////////////////////////////////////////////////////////////////////////////

void TestExceptions::testConstructors()
{
	// Test printf-style constructor
	size_t a = 123;
	std::string msg = "!!!";
	try {
		throw BaseException("Size=%zu and msg=%s", a, msg.c_str());
		UASSERT(false);
	} catch (const BaseException &e) {
		UASSERTEQ(std::string, e.what(), "Size=123 and msg=!!!");
	}

	// Test std::string constructor
	try {
		throw BaseException(std::string("Test std::string"));
		UASSERT(false);
	} catch (const BaseException &e) {
		UASSERTEQ(std::string, e.what(), "Test std::string");
	}

	// Test custom exception
	try {
		throw CustomException1("Test 1");
		UASSERT(false);
	} catch (const CustomException1 &e) {
		UASSERTEQ(std::string, e.what(), "Test 1");
	}

	// Test exception with default message
	try {
		throw CustomException2();
		UASSERT(false);
	} catch (const CustomException2 &e) {
		UASSERTEQ(std::string, e.what(), "The Default Message");
	}
}
