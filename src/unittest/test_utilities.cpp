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

#include "util/numeric.h"
#include "util/string.h"

class TestUtilities : public TestBase {
public:
	TestUtilities() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestUtilities"; }

	void runTests(IGameDef *gamedef);

	void testAngleWrapAround();
	void testLowercase();
	void testTrim();
	void testIsYes();
	void testRemoveStringEnd();
	void testUrlEncode();
	void testUrlDecode();
	void testPadString();
	void testStartsWith();
	void testStrEqual();
	void testStringTrim();
	void testStrToIntConversion();
	void testStringReplace();
	void testStringAllowed();
	void testUTF8();
	void testWrapRows();
	void testIsNumber();
	void testIsPowerOfTwo();
	void testMyround();
};

static TestUtilities g_test_instance;

void TestUtilities::runTests(IGameDef *gamedef)
{
	TEST(testAngleWrapAround);
	TEST(testLowercase);
	TEST(testTrim);
	TEST(testIsYes);
	TEST(testRemoveStringEnd);
	TEST(testUrlEncode);
	TEST(testUrlDecode);
	TEST(testPadString);
	TEST(testStartsWith);
	TEST(testStrEqual);
	TEST(testStringTrim);
	TEST(testStrToIntConversion);
	TEST(testStringReplace);
	TEST(testStringAllowed);
	TEST(testUTF8);
	TEST(testWrapRows);
	TEST(testIsNumber);
	TEST(testIsPowerOfTwo);
	TEST(testMyround);
}

////////////////////////////////////////////////////////////////////////////////

inline float ref_WrapDegrees180(float f)
{
	// This is a slower alternative to the wrapDegrees_180() function;
	// used as a reference for testing
	float value = fmodf(f + 180, 360);
	if (value < 0)
		value += 360;
	return value - 180;
}


inline float ref_WrapDegrees_0_360(float f)
{
	// This is a slower alternative to the wrapDegrees_0_360() function;
	// used as a reference for testing
	float value = fmodf(f, 360);
	if (value < 0)
		value += 360;
	return value < 0 ? value + 360 : value;
}


void TestUtilities::testAngleWrapAround()
{
	UASSERT(fabs(modulo360f(100.0) - 100.0) < 0.001);
	UASSERT(fabs(modulo360f(720.5) - 0.5) < 0.001);
	UASSERT(fabs(modulo360f(-0.5) - (-0.5)) < 0.001);
	UASSERT(fabs(modulo360f(-365.5) - (-5.5)) < 0.001);

	for (float f = -720; f <= -360; f += 0.25) {
		UASSERT(fabs(modulo360f(f) - modulo360f(f + 360)) < 0.001);
	}

	for (float f = -1440; f <= 1440; f += 0.25) {
		UASSERT(fabs(modulo360f(f) - fmodf(f, 360)) < 0.001);
		UASSERT(fabs(wrapDegrees_180(f) - ref_WrapDegrees180(f)) < 0.001);
		UASSERT(fabs(wrapDegrees_0_360(f) - ref_WrapDegrees_0_360(f)) < 0.001);
		UASSERT(wrapDegrees_0_360(fabs(wrapDegrees_180(f) - wrapDegrees_0_360(f))) < 0.001);
	}
}


void TestUtilities::testLowercase()
{
	UASSERT(lowercase("Foo bAR") == "foo bar");
}


void TestUtilities::testTrim()
{
	UASSERT(trim("") == "");
	UASSERT(trim("dirt_with_grass") == "dirt_with_grass");
	UASSERT(trim("\n \t\r  Foo bAR  \r\n\t\t  ") == "Foo bAR");
	UASSERT(trim("\n \t\r    \r\n\t\t  ") == "");
}


void TestUtilities::testIsYes()
{
	UASSERT(is_yes("YeS") == true);
	UASSERT(is_yes("") == false);
	UASSERT(is_yes("FAlse") == false);
	UASSERT(is_yes("-1") == true);
	UASSERT(is_yes("0") == false);
	UASSERT(is_yes("1") == true);
	UASSERT(is_yes("2") == true);
}


void TestUtilities::testRemoveStringEnd()
{
	const char *ends[] = {"abc", "c", "bc", "", NULL};
	UASSERT(removeStringEnd("abc", ends) == "");
	UASSERT(removeStringEnd("bc", ends) == "b");
	UASSERT(removeStringEnd("12c", ends) == "12");
	UASSERT(removeStringEnd("foo", ends) == "");
}


void TestUtilities::testUrlEncode()
{
	UASSERT(urlencode("\"Aardvarks lurk, OK?\"")
			== "%22Aardvarks%20lurk%2C%20OK%3F%22");
}


void TestUtilities::testUrlDecode()
{
	UASSERT(urldecode("%22Aardvarks%20lurk%2C%20OK%3F%22")
			== "\"Aardvarks lurk, OK?\"");
}


void TestUtilities::testPadString()
{
	UASSERT(padStringRight("hello", 8) == "hello   ");
}

void TestUtilities::testStartsWith()
{
	UASSERT(str_starts_with(std::string(), std::string()) == true);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string()) == true);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string("the")) == true);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string("The")) == false);
	UASSERT(str_starts_with(std::string("the sharp pickaxe"),
		std::string("The"), true) == true);
	UASSERT(str_starts_with(std::string("T"), std::string("The")) == false);
}

void TestUtilities::testStrEqual()
{
	UASSERT(str_equal(narrow_to_wide("abc"), narrow_to_wide("abc")));
	UASSERT(str_equal(narrow_to_wide("ABC"), narrow_to_wide("abc"), true));
}


void TestUtilities::testStringTrim()
{
	UASSERT(trim("  a") == "a");
	UASSERT(trim("   a  ") == "a");
	UASSERT(trim("a   ") == "a");
	UASSERT(trim("") == "");
}


void TestUtilities::testStrToIntConversion()
{
	UASSERT(mystoi("123", 0, 1000) == 123);
	UASSERT(mystoi("123", 0, 10) == 10);
}


void TestUtilities::testStringReplace()
{
	std::string test_str;
	test_str = "Hello there";
	str_replace(test_str, "there", "world");
	UASSERT(test_str == "Hello world");
	test_str = "ThisAisAaAtest";
	str_replace(test_str, 'A', ' ');
	UASSERT(test_str == "This is a test");
}


void TestUtilities::testStringAllowed()
{
	UASSERT(string_allowed("hello", "abcdefghijklmno") == true);
	UASSERT(string_allowed("123", "abcdefghijklmno") == false);
	UASSERT(string_allowed_blacklist("hello", "123") == true);
	UASSERT(string_allowed_blacklist("hello123", "123") == false);
}

void TestUtilities::testUTF8()
{
	UASSERT(wide_to_utf8(utf8_to_wide("")) == "");
	UASSERT(wide_to_utf8(utf8_to_wide("the shovel dug a crumbly node!"))
		== "the shovel dug a crumbly node!");
}

void TestUtilities::testWrapRows()
{
	UASSERT(wrap_rows("12345678",4) == "1234\n5678");
}


void TestUtilities::testIsNumber()
{
	UASSERT(is_number("123") == true);
	UASSERT(is_number("") == false);
	UASSERT(is_number("123a") == false);
}


void TestUtilities::testIsPowerOfTwo()
{
	UASSERT(is_power_of_two(0) == false);
	UASSERT(is_power_of_two(1) == true);
	UASSERT(is_power_of_two(2) == true);
	UASSERT(is_power_of_two(3) == false);
	for (int exponent = 2; exponent <= 31; ++exponent) {
		UASSERT(is_power_of_two((1 << exponent) - 1) == false);
		UASSERT(is_power_of_two((1 << exponent)) == true);
		UASSERT(is_power_of_two((1 << exponent) + 1) == false);
	}
	UASSERT(is_power_of_two((u32)-1) == false);
}

void TestUtilities::testMyround()
{
	UASSERT(myround(4.6f) == 5);
	UASSERT(myround(1.2f) == 1);
	UASSERT(myround(-3.1f) == -3);
	UASSERT(myround(-6.5f) == -7);
}

