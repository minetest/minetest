/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "profiler.h"

class TestProfiler : public TestBase
{
public:
	TestProfiler() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestProfiler"; }

	void runTests(IGameDef *gamedef);

	void testProfilerAverage();
};

static TestProfiler g_test_instance;

void TestProfiler::runTests(IGameDef *gamedef)
{
	TEST(testProfilerAverage);
}

////////////////////////////////////////////////////////////////////////////////

void TestProfiler::testProfilerAverage()
{
	Profiler p;

	p.avg("Test1", 1.f);
	UASSERT(p.getValue("Test1") == 1.f);

	p.avg("Test1", 2.f);
	UASSERT(p.getValue("Test1") == 1.5f);

	p.avg("Test1", 3.f);
	UASSERT(p.getValue("Test1") == 2.f);

	p.avg("Test1", 486.f);
	UASSERT(p.getValue("Test1") == 123.f);

	p.avg("Test1", 8);
	UASSERT(p.getValue("Test1") == 100.f);

	p.avg("Test1", 700);
	UASSERT(p.getValue("Test1") == 200.f);

	p.avg("Test1", 10000);
	UASSERT(p.getValue("Test1") == 1600.f);

	p.avg("Test2", 123.56);
	p.avg("Test2", 123.58);

	UASSERT(p.getValue("Test2") == 123.57f);
}
