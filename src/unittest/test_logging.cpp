/*
Minetest

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

using std::ostream;

class TestLogging : public TestBase
{
public:
	TestLogging() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestLogging"; }

	void runTests(IGameDef *gamedef);

	void testNullCheck();
	void testBitCheck();
};

static TestLogging g_test_instance;

void TestLogging::runTests(IGameDef *gamedef)
{
	TEST(testNullCheck);
	TEST(testBitCheck);
}

////////////////////////////////////////////////////////////////////////////////

void TestLogging::testNullCheck()
{
	CaptureLogOutput capture(g_logger);

	infostream << "Testing " << (const char*)0 << std::endl;

	auto logs = capture.take();

	UASSERTEQ(size_t, logs.size(), 1);
	UASSERTEQ(std::string, logs[0].text, "Testing (null)");
}

class ForceEofBit {};
class ForceFailBit {};
class ForceBadBit {};

ostream& operator<<(ostream& os, ForceEofBit)
{
	os.setstate(std::ios::eofbit);
	return os;
}

ostream& operator<<(ostream& os, ForceFailBit)
{
	os.setstate(std::ios::failbit);
	return os;
}

ostream& operator<<(ostream& os, ForceBadBit)
{
	os.setstate(std::ios::badbit);
	return os;
}

void TestLogging::testBitCheck()
{
	CaptureLogOutput capture(g_logger);

	infostream << "EOF is " << ForceEofBit{} << std::endl;
	infostream << "Fail is " << ForceFailBit{} << std::endl;
	infostream << "Bad is " << ForceBadBit{} << std::endl;

	auto logs = capture.take();
	UASSERTEQ(size_t, logs.size(), 3);
	UASSERTEQ(std::string, logs[0].text, "EOF is (ostream:eofbit)");
	UASSERTEQ(std::string, logs[1].text, "Fail is (ostream:failbit)");
	UASSERTEQ(std::string, logs[2].text, "Bad is (ostream:badbit)");
}
