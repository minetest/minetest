/*
Minetest
Copyright (C) 2021, DS

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

#include "guid.h"

class TestGUID : public TestBase
{
public:
	TestGUID() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestGUID"; }

	void runTests(IGameDef *gamedef);

	void testGetValidity();
	void testGenerate();
};

static TestGUID g_test_instance;

void TestGUID::runTests(IGameDef *gamedef)
{
	TEST(testGetValidity);
}

////////////////////////////////////////////////////////////////////////////////

void TestGUID::testGetValidity()
{
#define ASSERT_VALIDITY(str, validity) \
		UASSERT(GUIdGenerator::getValidity(str) == GUIdGenerator::validity)

	ASSERT_VALIDITY("v1guid1", Valid);
	ASSERT_VALIDITY("", Old);
	ASSERT_VALIDITY("v1guid", Invalid);
	ASSERT_VALIDITY("v1guid3456789987654323456789876543456789834578909870007654", Valid);
	ASSERT_VALIDITY("v1guid0523", Invalid);
	ASSERT_VALIDITY("v1guid12340", Valid);
	ASSERT_VALIDITY("v1gsd12340", Invalid);
	ASSERT_VALIDITY("v2guid12340", Invalid);
	ASSERT_VALIDITY("v1guid0", Invalid);

#undef TEST_VALIDITY
}

void TestGUID::testGenerate()
{
	GUIdGenerator generator;
	UASSERT(generator.peekNext() == "");
	UASSERT(generator.generateNext() == "");

	UASSERT(generator.seed("foo") == GUIdGenerator::Invalid);
	UASSERT(generator.generateNext() == "");

	UASSERT(generator.seed("") == GUIdGenerator::Old);
	UASSERT(generator.generateNext() == "");

	UASSERT(generator.seed("v1guid0") == GUIdGenerator::Invalid);
	UASSERT(generator.generateNext() == "");

	UASSERT(generator.seed("v1guid1") == GUIdGenerator::Valid);
	UASSERT(generator.peekNext() == "v1guid1");
	UASSERT(generator.peekNext() == "v1guid1");
	UASSERT(generator.generateNext() == "v1guid1");
	UASSERT(generator.generateNext() == "v1guid2");
	UASSERT(generator.peekNext() == "v1guid2");
	UASSERT(generator.peekNext() == "v1guid2");
	UASSERT(generator.generateNext() == "v1guid3");
	UASSERT(generator.generateNext() == "v1guid4");
	UASSERT(generator.generateNext() == "v1guid5");
	UASSERT(generator.generateNext() == "v1guid6");
	UASSERT(generator.generateNext() == "v1guid7");
	UASSERT(generator.generateNext() == "v1guid8");
	UASSERT(generator.generateNext() == "v1guid9");
	UASSERT(generator.generateNext() == "v1guid10");
	UASSERT(generator.generateNext() == "v1guid11");
	UASSERT(generator.generateNext() == "v1guid12");

	UASSERT(generator.seed("v1guid1000000000000") == GUIdGenerator::Valid);
	UASSERT(generator.generateNext() == "v1guid1000000000000");
	UASSERT(generator.generateNext() == "v1guid1000000000001");
	UASSERT(generator.generateNext() == "v1guid1000000000002");
	UASSERT(generator.generateNext() == "v1guid1000000000003");

	UASSERT(generator.seed("v1guid1000000000000000000000000000000000000000"
			"0000000000000000000000000000000000000000000000000000000000000"
			"000000000000000000000000000000000000000000000000") == GUIdGenerator::Valid);
	UASSERT(generator.generateNext() == "v1guid1000000000000000000000000000000000000000"
			"0000000000000000000000000000000000000000000000000000000000000"
			"000000000000000000000000000000000000000000000000");
	UASSERT(generator.generateNext() == "v1guid1000000000000000000000000000000000000000"
			"0000000000000000000000000000000000000000000000000000000000000"
			"000000000000000000000000000000000000000000000001");
	UASSERT(generator.generateNext() == "v1guid1000000000000000000000000000000000000000"
			"0000000000000000000000000000000000000000000000000000000000000"
			"000000000000000000000000000000000000000000000002");

	UASSERT(generator.seed("v1guid9999999999999999998") == GUIdGenerator::Valid);
	UASSERT(generator.generateNext() == "v1guid9999999999999999998");
	UASSERT(generator.generateNext() == "v1guid9999999999999999999");
	UASSERT(generator.generateNext() == "v1guid10000000000000000000");
	UASSERT(generator.generateNext() == "v1guid10000000000000000001");
	UASSERT(generator.generateNext() == "v1guid10000000000000000002");
}
